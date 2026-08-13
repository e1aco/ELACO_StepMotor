/*****************************************************************************
 * @文件: encoder_calibrator_usr.c
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v1.0
 * @说明: 编码器校准用户层（状态机：拖回→正转采样→消隙→反转采样→校验→生成校准表写 Flash）
 *   校准思想：电机 FOC 矢量转整圈，编码器在每整步机械位置采 16 次平均；
 *   正反两圈消除机械间隙后线性插值出 raw 角度→51200 细分步映射表写入 Flash
 * @平台: STM32F103RET6
 * @依赖: mt6816_usr, tb67h450_usr, flash_drv, uart_drv
 ****************************************************************************/
#include "encoder_calibrator_usr.h"
#include "mt6816_usr.h"
#include "tb67h450_usr.h"
#include "flash_drv.h"
#include "uart_drv.h"
#include "cycle_usr.h"
#include "sys_drv.h"
#include <stdio.h>
#include <stdlib.h>

/* ==== 常量定义 ==== */
#define CALI_HARD_STEPS        200U    /* 步进电机一圈 200 整步（1.8° 步距角，依据 require.md 电机参数） */
#define CALI_SAMPLE_PER_STEP   16U     /* 每个整步位置采样 16 次取平均（复刻参考 encoder_calibrator.c） */
#define CALI_AUTO_SPEED        2U      /* 自动跑位移动速度（细分步/tick） */
#define CALI_FINE_SPEED        1U      /* 细采样移动速度（细分步/tick） */
#define CALI_ENC_RESOLUTION    16384U  /* 编码器 14bit 分辨率 0~16383（依据 .cl/memory/ mt6816_resolution） */
#define CALI_SOFT_DIVIDE       256U    /* 每整步细分 256 微步（依据 .cl/memory/ SOFT_DIVIDE_NUM=256） */
#define CALI_SUBDIVIDE_STEPS   51200U  /* 一圈细分步 = 200×256（依据 .cl/memory/ MOTOR_SUBDIVIDE_STEPS） */
#define CALI_CALI_CURRENT      2000U   /* 校准驱动电流 mA（依据 .cl/memory/ config_default_calib_current=2000） */
#define CALI_FREE_WHEEL_CNT    (CALI_SOFT_DIVIDE * 20U)   /* 消隙回程前继续前进 20 整步（复刻参考） */

/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
static bool     s_cali_triggered = false;
static bool     s_cali_is_calibrated = false;
static int32_t  s_cali_error = 0;
static uint8_t  s_cali_state = 0;
static uint32_t s_go_pos = 0;
static bool     s_go_dir = true;
static uint16_t s_sample_cnt = 0;
static uint16_t s_sample_raw[CALI_SAMPLE_PER_STEP];
static uint16_t s_sample_fwd[CALI_HARD_STEPS + 1U];
static uint16_t s_sample_rev[CALI_HARD_STEPS + 1U];
static int32_t  s_rcd_x = 0;
static int32_t  s_rcd_y = 0;
static uint32_t s_result_num = 0;

/* 校准表指针（指向 Flash 中校准数据地址） */
static uint16_t *s_cali_table = (uint16_t *)DRV_FLASH_CALI_ADDR;

/* ==== 内部工具 ==== */
/**
 * @输出 无
 * @说明 校验采集数据：方向性/连续性/单跳变点，通过则置 cali_error=0
 * 复刻参考 encoder_calibrator.c S_CheckData；循环域运算用 cycle_usr 通用函数
 */
static void S_CheckData(void)
{
    int32_t sub;
    int32_t step_res = (int32_t)(CALI_ENC_RESOLUTION / CALI_HARD_STEPS);
    uint32_t step_num = 0U;
    char buf[48];
    int32_t i;

    /* 1. 正向/反向平均（消机械间隙） */
    for (i = 0; i < (int32_t)(CALI_HARD_STEPS + 1U); i++)
    {
        s_sample_fwd[i] = (uint16_t)USR_Cycle_Avg(s_sample_fwd[i], s_sample_rev[i], CALI_ENC_RESOLUTION);
    }

    /* 2. 判断方向（首末角差） */
    sub = USR_Cycle_Sub(s_sample_fwd[0], s_sample_fwd[CALI_HARD_STEPS - 1U], CALI_ENC_RESOLUTION);
    if (sub == 0)
    {
        s_cali_error = 1;
        DRV_Uart_SendString("Error: Direction zero\r\n");
        return;
    }
    s_go_dir = (sub > 0);

    /* 3. 连续性检查：每整步间编码器增量应接近 step_res 且方向一致 */
    for (i = 1; i < (int32_t)CALI_HARD_STEPS; i++)
    {
        sub = USR_Cycle_Sub(s_sample_fwd[i], s_sample_fwd[i - 1], CALI_ENC_RESOLUTION);
        if (abs(sub) > (step_res * 3 / 2))
        {
            s_cali_error = 2;
            sprintf(buf, "Error: Continuity large, i=%d\r\n", (int)i);
            DRV_Uart_SendString(buf);
            return;
        }
        if (abs(sub) < (step_res * 1 / 2))
        {
            s_cali_error = 2;
            sprintf(buf, "Error: Continuity small, i=%d\r\n", (int)i);
            DRV_Uart_SendString(buf);
            return;
        }
        if (sub == 0)
        {
            s_cali_error = 1;
            sprintf(buf, "Error: Zero delta, i=%d\r\n", (int)i);
            DRV_Uart_SendString(buf);
            return;
        }
        if ((sub > 0) != s_go_dir)
        {
            s_cali_error = 1;
            sprintf(buf, "Error: Direction mismatch, i=%d\r\n", (int)i);
            DRV_Uart_SendString(buf);
            return;
        }
    }

    /* 4. 跳变点检测：0° 磁点（编码器值回绕处），按方向分流 */
    for (i = 0; i < (int32_t)CALI_HARD_STEPS; i++)
    {
        int32_t curr = s_sample_fwd[i];
        int32_t next = s_sample_fwd[i + 1];
        if (s_go_dir)
        {
            /* 正向：值从 >3/4 圈跳落到 <1/4 圈处，rcd_y 为距 0 点的正向距离 */
            if ((curr > (int32_t)(CALI_ENC_RESOLUTION * 3 / 4)) && (next < (int32_t)(CALI_ENC_RESOLUTION / 4)))
            {
                step_num++;
                s_rcd_x = i;
                s_rcd_y = (int32_t)(CALI_ENC_RESOLUTION - 1U) - curr;
            }
        }
        else
        {
            /* 反向：值从 <1/4 圈跳升到 >3/4 圈处，rcd_y 为距 0 点的反向距离 */
            if ((curr < (int32_t)(CALI_ENC_RESOLUTION / 4)) && (next > (int32_t)(CALI_ENC_RESOLUTION * 3 / 4)))
            {
                step_num++;
                s_rcd_x = i;
                s_rcd_y = curr;
            }
        }
    }

    if (step_num != 1U)
    {
        s_cali_error = 3;
        sprintf(buf, "Error: Phase step, num=%lu\r\n", (unsigned long)step_num);
        DRV_Uart_SendString(buf);
    }
    else
    {
        s_cali_error = 0;
        sprintf(buf, "S_CheckData PASS, rcd_x=%ld, rcd_y=%ld\r\n", (long)s_rcd_x, (long)s_rcd_y);
        DRV_Uart_SendString(buf);
    }
}

/**
 * @输出 无
 * @说明 生成校准表：按步距把编码器线性插值成 51200 细分步映射，写入 Flash
 * 复刻参考 encoder_calibrator.c S_GenerateTable
 */
static void S_GenerateTable(void)
{
    int32_t data;
    uint16_t val;
    char buf[48];
    int32_t x;
    int32_t y;

    s_result_num = 0U;

    /* 擦除分区并开始写入 */
    DRV_Flash_AreaEmpty(&g_flash_quick_cali);
    DRV_Flash_AreaBegin(&g_flash_quick_cali);

    if (s_go_dir)
    {
        /* 正转：rcd_x 处 0° 磁点，逐段线性插值 raw→细分步 */
        for (x = s_rcd_x; x < s_rcd_x + (int32_t)CALI_HARD_STEPS + 1; x++)
        {
            data = USR_Cycle_Sub(s_sample_fwd[USR_Cycle_Mod(x + 1, (int32_t)CALI_HARD_STEPS)],
                                 s_sample_fwd[USR_Cycle_Mod(x, (int32_t)CALI_HARD_STEPS)],
                                 CALI_ENC_RESOLUTION);

            int32_t start_y = (x == s_rcd_x) ? s_rcd_y : 0;
            int32_t end_y = (x == s_rcd_x + (int32_t)CALI_HARD_STEPS) ? s_rcd_y : data;

            for (y = start_y; y < end_y; y++)
            {
                val = (uint16_t)USR_Cycle_Mod((int32_t)CALI_SOFT_DIVIDE * x + (int32_t)CALI_SOFT_DIVIDE * y / data,
                                              (int32_t)CALI_SUBDIVIDE_STEPS);
                DRV_Flash_AreaWrite16(&g_flash_quick_cali, &val, 1U);
                s_result_num++;
            }
        }
    }
    else
    {
        /* 反转：同理反向插值 */
        for (x = s_rcd_x + (int32_t)CALI_HARD_STEPS; x > s_rcd_x - 1; x--)
        {
            data = USR_Cycle_Sub(s_sample_fwd[USR_Cycle_Mod(x, (int32_t)CALI_HARD_STEPS)],
                                 s_sample_fwd[USR_Cycle_Mod(x + 1, (int32_t)CALI_HARD_STEPS)],
                                 CALI_ENC_RESOLUTION);

            int32_t start_y = (x == s_rcd_x + (int32_t)CALI_HARD_STEPS) ? s_rcd_y : 0;
            int32_t end_y = (x == s_rcd_x) ? s_rcd_y : data;

            for (y = start_y; y < end_y; y++)
            {
                val = (uint16_t)USR_Cycle_Mod((int32_t)CALI_SOFT_DIVIDE * (x + 1) - (int32_t)CALI_SOFT_DIVIDE * y / data,
                                              (int32_t)CALI_SUBDIVIDE_STEPS);
                DRV_Flash_AreaWrite16(&g_flash_quick_cali, &val, 1U);
                s_result_num++;
            }
        }
    }

    DRV_Flash_AreaEnd(&g_flash_quick_cali);
    sprintf(buf, "S_GenerateTable done, result_num=%lu, cali_table[0]=%u\r\n",
            (unsigned long)s_result_num, (unsigned)s_cali_table[0]);
    DRV_Uart_SendString(buf);

    if (s_result_num == CALI_ENC_RESOLUTION)
    {
        s_cali_is_calibrated = true;
    }
    else
    {
        s_cali_error = 4;
    }
}

/* ==== 接口实现 ==== */
/**
 * @输入 无
 * @输出 无
 * @说明 初始化：检查 Flash 是否已有有效校准表，有则挂给 MT6816
 */
void USR_EncoderCalibrator_Init(void)
{
    uint16_t first = s_cali_table[0];
    if ((first != 0xFFFFU) && (first != 0U))
    {
        s_cali_is_calibrated = true;
        USR_MT6816_SetCalibrationData(s_cali_table);
    }
}

/**
 * @输入 无
 * @输出 无
 * @说明 触发校准：仅在未校准且未触发时置位（由按键上电同按调用）
 */
void USR_EncoderCalibrator_Trigger(void)
{
		if (!s_cali_is_calibrated && !s_cali_triggered)
    {
        s_cali_triggered = true;
        s_cali_state = 0;
        s_cali_error = 0;
        s_go_pos = 0;
        s_sample_cnt = 0;
    }
}

/**
 * @输入 无
 * @输出 无
 * @说明 20kHz tick 状态机：驱动电机走位 + 采样编码器（每 50µs 一次）
 * 复刻参考 encoder_calibrator.c EncoderCalibrator_Tick20kHz
 */
void USR_EncoderCalibrator_Tick20kHz(void)
{
    uint16_t raw;
    int32_t idx;

    /* 先更新一次编码器角度 */
    USR_MT6816_UpdateAngle();

    switch (s_cali_state)
    {
        case 0: /* 空闲：等待触发 */
            if (s_cali_triggered)
            {
                /* 上电电流矢量，开始跑位到起始位置 */
                USR_TB67H450_SetFocCurrentVector(s_go_pos, CALI_CALI_CURRENT);
                s_go_pos = CALI_SUBDIVIDE_STEPS;
                s_sample_cnt = 0;
                s_cali_state = 1;
            }
            break;

        case 1: /* 自动跑位：先转一圈建立位置 */
            s_go_pos += CALI_AUTO_SPEED;
            USR_TB67H450_SetFocCurrentVector(s_go_pos, CALI_CALI_CURRENT);
            if (s_go_pos == 2U * CALI_SUBDIVIDE_STEPS)
            {
                s_go_pos = CALI_SUBDIVIDE_STEPS;
                s_cali_state = 2;
            }
            break;

        case 2: /* 正转采样一圈 */
            if ((s_go_pos % CALI_SOFT_DIVIDE) == 0U)
            {
                raw = USR_MT6816_GetRawAngle();
                s_sample_raw[s_sample_cnt++] = raw;
                if (s_sample_cnt == CALI_SAMPLE_PER_STEP)
                {
                    /* 16 次平均存入正向表 */
                    idx = (int32_t)(s_go_pos - CALI_SUBDIVIDE_STEPS) / (int32_t)CALI_SOFT_DIVIDE;
                    s_sample_fwd[idx] = (uint16_t)USR_Cycle_DataAvg(s_sample_raw, CALI_SAMPLE_PER_STEP, CALI_ENC_RESOLUTION);
                    s_sample_cnt = 0;
                    s_go_pos += CALI_FINE_SPEED;
                }
            }
            else
            {
                s_go_pos += CALI_FINE_SPEED;
            }
            USR_TB67H450_SetFocCurrentVector(s_go_pos, CALI_CALI_CURRENT);
            if (s_go_pos > 2U * CALI_SUBDIVIDE_STEPS)
            {
                s_cali_state = 3;
            }
            break;

        case 3: /* 消隙回程准备：继续前进一小段再返回 */
            s_go_pos += CALI_FINE_SPEED;
            USR_TB67H450_SetFocCurrentVector(s_go_pos, CALI_CALI_CURRENT);
            if (s_go_pos == 2U * CALI_SUBDIVIDE_STEPS + CALI_FREE_WHEEL_CNT)
            {
                s_cali_state = 4;
            }
            break;

        case 4: /* 反向返回：越过起点消除机械间隙 */
            s_go_pos -= CALI_FINE_SPEED;
            USR_TB67H450_SetFocCurrentVector(s_go_pos, CALI_CALI_CURRENT);
            if (s_go_pos == 2U * CALI_SUBDIVIDE_STEPS)
            {
                s_cali_state = 5;
            }
            break;

        case 5: /* 反转采样一圈 */
            if ((s_go_pos % CALI_SOFT_DIVIDE) == 0U)
            {
                raw = USR_MT6816_GetRawAngle();
                s_sample_raw[s_sample_cnt++] = raw;
                if (s_sample_cnt == CALI_SAMPLE_PER_STEP)
                {
                    idx = (int32_t)(s_go_pos - CALI_SUBDIVIDE_STEPS) / (int32_t)CALI_SOFT_DIVIDE;
                    s_sample_rev[idx] = (uint16_t)USR_Cycle_DataAvg(s_sample_raw, CALI_SAMPLE_PER_STEP, CALI_ENC_RESOLUTION);
                    s_sample_cnt = 0;
                    s_go_pos -= CALI_FINE_SPEED;
                }
            }
            else
            {
                s_go_pos -= CALI_FINE_SPEED;
            }
            USR_TB67H450_SetFocCurrentVector(s_go_pos, CALI_CALI_CURRENT);
            if (s_go_pos < CALI_SUBDIVIDE_STEPS)
            {
                s_cali_state = 6;
            }
            break;

        case 6: /* 结束：断电不励磁 */
            USR_TB67H450_SetFocCurrentVector(0, 0);
            break;

        default:
            break;
    }
}

/**
 * @输入 无
 * @输出 无
 * @说明 主循环：校准完成后校验数据并生成校准表写 Flash
 */
void USR_EncoderCalibrator_TickMainLoop(void)
{
    char buf[48];
    uint16_t *p;

    if (s_cali_state != 6)
    {
        return;
    }

    /* 断电不励磁 */
    USR_TB67H450_Sleep();

    /* 校验数据 */
    S_CheckData();

    /* 校验通过则生成校准表 */
    if (s_cali_error == 0)
    {
        S_GenerateTable();
    }

    /* 复位状态 */
    s_cali_state = 0;
    s_cali_triggered = false;

    /* 校准成功则系统复位使校准表生效 */
    if (s_cali_error == 0)
    {
        p = (uint16_t *)g_flash_quick_cali.begin_add;
        sprintf(buf, "Flash check: [0]=%u, [1]=%u, [2]=%u\r\n", (unsigned)p[0], (unsigned)p[1], (unsigned)p[2]);
        DRV_Uart_SendString(buf);
        DRV_Sys_SystemReset();
    }
}

/**
 * @输入 无
 * @输出 true=已校准
 * @说明 是否已校准
 */
bool USR_EncoderCalibrator_IsCalibrated(void)
{
    return s_cali_is_calibrated;
}

/**
 * @输入 无
 * @输出 true=已触发校准
 * @说明 是否已触发
 */
bool USR_EncoderCalibrator_IsTriggered(void)
{
    return s_cali_triggered;
}

/**
 * @输入 raw_angle 原始角度 0~16383
 * @输出 校准后角度（细分步空间）；未校准则原样返回
 * @说明 校准表查询
 */
uint16_t USR_EncoderCalibrator_GetRectifiedAngle(uint16_t raw_angle)
{
    if (s_cali_is_calibrated)
    {
        return s_cali_table[raw_angle];
    }
    return raw_angle;
}
