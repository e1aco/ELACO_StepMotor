#include "encoder_calibrator.h"
#include "mt6816.h"
#include "tb67h450.h"
#include "stockpile_f103cb.h"
#include <stdlib.h>

/* ==================== 常量定义 ==================== */
#define HARD_STEPS          200     /* 步进电机一圈200步（1.8°步距角）*/
#define SAMPLE_PER_STEP     16      /* 每个机械位置采样16次取平均 */
#define AUTO_SPEED          2       /* 自动校准时的移动速度 */
#define FINE_SPEED          1       /* 精调时的移动速度 */
#define ENC_RESOLUTION      16384   /* 编码器14位分辨率（2^14=16384）*/
#define SOFT_DIVIDE         256       /* 软细分数，将1步再细分成256个微步 */
#define SUBDIVIDE_STEPS     51200     /* 一圈细分步数 = HARD_STEPS * SOFT_DIVIDE = 51200 */

/* ==================== 全局变量 ==================== */
static bool cali_triggered = false;      /* 校准触发标志 */
static bool cali_is_calibrated = false;  /* 是否已校准完成 */
static int cali_error = 0;               /* 错误码：0=无错误，1=方向错误，2=数据不连续，3=相位错误，4=数量错误 */
static int cali_state = 0;               /* 状态机状态：0空闲,1正向准备,2正向测量,3反向返回,4消除间隙,5反向测量,6计算 */

static uint32_t go_pos = 0;              /* 当前目标位置（细分步数，0-400）*/
static bool go_dir = true;               /* 旋转方向：true=正转，false=反转 */
static uint16_t sample_cnt = 0;          /* 当前已采样次数计数 */
static uint16_t sample_raw[SAMPLE_PER_STEP];     /* 原始采样数据缓冲区 */
static uint16_t sample_fwd[HARD_STEPS + 1];      /* 正向测量数据（每个机械步位置的平均角度）*/
static uint16_t sample_rev[HARD_STEPS + 1];      /* 反向测量数据（每个机械步位置的平均角度）*/
static int32_t rcd_x = 0, rcd_y = 0;     /* 记录角度跳跃点的位置 */
static uint32_t result_num = 0;          /* 生成的校准表点数计数 */

/* 校准表指针（指向Flash中存储校准数据的地址）*/
static uint16_t* cali_table = (uint16_t*)STOCKPILE_APP_CALI_ADDR;

/* ==================== 辅助函数 ==================== */

/**
 * @brief 循环取模运算
 * @param a 被除数
 * @param b 除数
 * @return (a + b) % b，确保结果在[0, b)范围内
 */
static uint32_t CycleMod(uint32_t a, uint32_t b)
{
    return (a + b) % b;
}

/**
 * @brief 循环减法（处理角度环绕）
 * @param a 角度A
 * @param b 角度B
 * @param cyc 一圈的总刻度数（如16384）
 * @return a和b的最短差值，范围(-cyc/2, cyc/2]
 * @example 角度10°和350°，差值为20°而不是-340°
 */
static int32_t CycleSub(int32_t a, int32_t b, int32_t cyc)
{
    int32_t sub = a - b;
    if (sub > (cyc >> 1)) sub -= cyc;      /* 超过半圈，减一圈 */
    if (sub < (-(cyc >> 1))) sub += cyc;   /* 低于负半圈，加一圈 */
    return sub;
}

/**
 * @brief 循环平均（处理角度环绕）
 * @param a 角度A
 * @param b 角度B
 * @param cyc 一圈的总刻度数
 * @return 两个角度的平均值，正确处理0°附近的环绕
 * @example 10°和350°的平均值是0°而不是180°
 */
static int32_t CycleAvg(int32_t a, int32_t b, int32_t cyc)
{
    int32_t sub = a - b;
    int32_t ave = (a + b) >> 1;
    if (abs(sub) > (cyc >> 1)) {
        if (ave >= (cyc >> 1))
            ave -= (cyc >> 1);
        else
            ave += (cyc >> 1);
    }
    return ave;
}

/**
 * @brief 多数据循环平均
 * @param data 数据数组
 * @param len 数组长度
 * @param cyc 一圈的总刻度数
 * @return 所有数据的平均值，正确处理角度环绕
 */
static int32_t CycleDataAvg(const uint16_t* data, uint16_t len, int32_t cyc)
{
    int32_t sum = data[0];
    for (uint16_t i = 1; i < len; i++) {
        int32_t diff = data[i];
        int32_t sub = data[i] - data[0];
        /* 以第一个数据为基准，处理其他数据的环绕 */
        if (sub > (cyc >> 1)) diff = data[i] - cyc;
        if (sub < (-(cyc >> 1))) diff = data[i] + cyc;
        sum += diff;
    }
    sum = sum / len;
    /* 将结果归一化到[0, cyc)范围 */
    if (sum < 0) sum += cyc;
    if (sum > cyc) sum -= cyc;
    return sum;
}

/* ==================== 数据检查 ==================== */

/**
 * @brief 检查校准数据的有效性
 * @details 检查内容：
 *          1. 数据是否单调递增/递减（方向检查）
 *          2. 相邻点差值是否在合理范围内（连续性检查）
 *          3. 一圈中是否只有一个相位跳跃点
 * @note 如果检查失败，会设置cali_error错误码
 */
static void CheckData(void)
{
    int32_t sub;
    int32_t step_res = ENC_RESOLUTION / HARD_STEPS;
    uint32_t step_num = 0;

    /* 1. 正向和反向数据平均 */
    for (int i = 0; i < HARD_STEPS + 1; i++) {
        sample_fwd[i] = CycleAvg(sample_fwd[i], sample_rev[i], ENC_RESOLUTION);
    }

    /* 2. 检查方向 */
    sub = CycleSub(sample_fwd[0], sample_fwd[HARD_STEPS - 1], ENC_RESOLUTION);
    if (sub == 0) {
        cali_error = 1;
        printf("Error: Direction zero\r\n");
        return;
    }
    go_dir = (sub > 0);

    /* 3. 检查连续性 */
    for (int i = 1; i < HARD_STEPS; i++) {
        sub = CycleSub(sample_fwd[i], sample_fwd[i-1], ENC_RESOLUTION);
        if (abs(sub) > (step_res * 3 / 2)) {
            cali_error = 2;
            printf("Error: Continuity large, i=%d\r\n", i);
            return;
        }
        if (abs(sub) < (step_res * 1 / 2)) {
            cali_error = 2;
            printf("Error: Continuity small, i=%d\r\n", i);
            return;
        }
        if (sub == 0) {
            cali_error = 1;
            printf("Error: Zero delta, i=%d\r\n", i);
            return;
        }
        if ((sub > 0) != go_dir) {
            cali_error = 1;
            printf("Error: Direction mismatch, i=%d\r\n", i);
            return;
        }
    }

    /* 4. 找跳跃点：直接用原始值判断过零 */
    for (int i = 0; i < HARD_STEPS; i++) {
        int32_t curr = sample_fwd[i];
        int32_t next = sample_fwd[i + 1];
        if (curr > (ENC_RESOLUTION * 3 / 4) && next < (ENC_RESOLUTION / 4)) {
            step_num++;
            rcd_x = i;
            rcd_y = (ENC_RESOLUTION - 1) - curr;
        }
    }

    if (step_num != 1) {
        cali_error = 3;
        printf("Error: Phase step, num=%lu\r\n", step_num);
    } else {
        cali_error = 0;
        printf("CheckData PASS, rcd_x=%ld, rcd_y=%ld\r\n", rcd_x, rcd_y);
    }
}
//static void CheckData(void)
//{
//    int32_t sub;
//    int32_t step_res = ENC_RESOLUTION / HARD_STEPS;
//    char buf[128];
//
//    // 1. 平均
//    for (int i = 0; i < HARD_STEPS + 1; i++) {
//        sample_fwd[i] = CycleAvg(sample_fwd[i], sample_rev[i], ENC_RESOLUTION);
//    }
//
//    // 2. 方向检查
//    sub = CycleSub(sample_fwd[0], sample_fwd[HARD_STEPS - 1], ENC_RESOLUTION);
//    if (sub == 0) {
//        cali_error = 1;
//        Uart_SendString("Error: Direction zero\r\n");
//        return;
//    }
//    go_dir = (sub > 0);
//
//    // 3. 连续性检查
//    for (int i = 1; i < HARD_STEPS; i++) {
//        sub = CycleSub(sample_fwd[i], sample_fwd[i-1], ENC_RESOLUTION);
//        if (abs(sub) > (step_res * 3 / 2)) {
//            cali_error = 2;
//            Uart_SendString("Error: Continuity too large\r\n");
//            return;
//        }
//        if (abs(sub) < (step_res * 1 / 2)) {
//            cali_error = 2;
//            Uart_SendString("Error: Continuity too small\r\n");
//            return;
//        }
//        if (sub == 0) {
//            cali_error = 1;
//            Uart_SendString("Error: Zero delta\r\n");
//            return;
//        }
//        if ((sub > 0) && (!go_dir)) {
//            cali_error = 1;
//            Uart_SendString("Error: Direction mismatch\r\n");
//            return;
//        }
//        if ((sub < 0) && (go_dir)) {
//            cali_error = 1;
//            Uart_SendString("Error: Direction mismatch\r\n");
//            return;
//        }
//    }
//
//    // 4. 跳跃点检测（宽松版）
//    uint32_t step_num = 0;
//    for (int i = 0; i < HARD_STEPS; i++) {
//        int32_t curr = sample_fwd[i];
//        int32_t next = sample_fwd[i+1];
//        if (curr > 12000 && next < 4000) {
//            step_num++;
//            rcd_x = i;
//            rcd_y = (ENC_RESOLUTION - 1) - curr;
//        }
//    }
//
//    if (step_num == 0) {
//        // 如果没找到跳跃点，强制设一个
//        step_num = 1;
//        rcd_x = 100;
//        rcd_y = 0;
//        Uart_SendString("Warning: No jump point found, forced\r\n");
//    } else if (step_num > 1) {
//        cali_error = 3;
//        Uart_SendString("Error: Multiple jump points\r\n");
//        return;
//    }
//
//    cali_error = 0;
//    Uart_SendString("CheckData PASS\r\n");
//}
/* ==================== 生成校准表 ==================== */

/**
 * @brief 生成校准映射表并写入Flash
 * @details 根据采集的数据，通过线性插值生成16384个点的校准表
 *          校准表功能：原始角度 → 校准后角度
 * @note 校准成功后会自动设置cali_is_calibrated标志
 */
static void GenerateTable(void)
{
    int32_t data;
    uint16_t val;

    result_num = 0;

    /* 擦除并开始写入Flash */
    Stockpile_Flash_Data_Empty(&stockpile_quick_cali);
    Stockpile_Flash_Data_Begin(&stockpile_quick_cali);

    if (go_dir) {
        /* 正转方向：线性插值生成校准表 */
        for (int x = rcd_x; x < rcd_x + HARD_STEPS + 1; x++) {
            /* 计算相邻两个采样点之间的角度差 */
            data = CycleSub(sample_fwd[CycleMod(x+1, HARD_STEPS)],
                            sample_fwd[CycleMod(x, HARD_STEPS)], ENC_RESOLUTION);

            /* 确定插值范围 */
            int start_y = (x == rcd_x) ? rcd_y : 0;
            int end_y = (x == rcd_x + HARD_STEPS) ? rcd_y : data;

            /* 在两点之间线性插值 */
            for (int y = start_y; y < end_y; y++) {
                val = CycleMod(SOFT_DIVIDE * x + SOFT_DIVIDE * y / data, SUBDIVIDE_STEPS);
                Stockpile_Flash_Data_Write_Data16(&stockpile_quick_cali, &val, 1);
                result_num++;
            }
        }
    } else {
        /* 反转方向：线性插值生成校准表 */
        for (int x = rcd_x + HARD_STEPS; x > rcd_x - 1; x--) {
            data = CycleSub(sample_fwd[CycleMod(x, HARD_STEPS)],
                            sample_fwd[CycleMod(x+1, HARD_STEPS)], ENC_RESOLUTION);

            int start_y = (x == rcd_x + HARD_STEPS) ? rcd_y : 0;
            int end_y = (x == rcd_x) ? rcd_y : data;

            for (int y = start_y; y < end_y; y++) {
                val = CycleMod(SOFT_DIVIDE * (x+1) - SOFT_DIVIDE * y / data, SUBDIVIDE_STEPS);
                Stockpile_Flash_Data_Write_Data16(&stockpile_quick_cali, &val, 1);
                result_num++;
            }
        }
    }

    Stockpile_Flash_Data_End(&stockpile_quick_cali);
		printf("GenerateTable done, result_num=%lu, cali_table[0]=%u\r\n", result_num, cali_table[0]);

    /* 检查生成的点数是否正确 */
    if (result_num == ENC_RESOLUTION) cali_is_calibrated = true;
    else cali_error = 4;
}

/* ==================== 公共函数 ==================== */

/**
 * @brief 初始化编码器校准模块
 * @details 检查Flash中是否已有有效的校准数据
 */
void EncoderCalibrator_Init(void)
{
    /* 检查Flash中是否有有效校准数据（第一个字不是0xFFFF也不是0）*/
    uint16_t first = cali_table[0];
    if (first != 0xFFFF && first != 0) {
        cali_is_calibrated = true;
				MT6816_SetCalibrationData(cali_table);
    }
//	    uint16_t first = cali_table[0];
//    printf("cali_table[0]=%u (0x%04X)\r\n", first, first);
//    if (first != 0xFFFF && first != 0) {
//        cali_is_calibrated = true;
//			MT6816_SetCalibrationData(cali_table);
//    }
//    printf("cali_init: calibrated=%d\r\n", cali_is_calibrated);
}

/**
 * @brief 触发编码器校准
 * @details 只有在未校准且未触发时才会开始校准
 * @note 通常由按钮按下事件调用
 */
void EncoderCalibrator_Trigger(void)
{
    if (!cali_is_calibrated && !cali_triggered) {
        cali_triggered = true;
        cali_state = 0;
        cali_error = 0;
        go_pos = 0;
        sample_cnt = 0;
    }
}

/**
 * @brief 20kHz高频中断处理函数
 * @details 执行校准状态机，控制电机运动并采集编码器数据
 * @note 必须在20kHz定时器中断中调用（每50μs一次）
 */
/**
 * @brief 编码器校准函数，每20kHz调用一次
 * 该函数用于执行编码器的自动校准过程，包括正向和反向测量，并处理校准状态机
 */
void EncoderCalibrator_Tick20kHz(void)
{
    uint16_t raw;  // 存储编码器原始角度值
    // 更新MT6816编码器角度值
    MT6816_UpdateAngle();
    // 校准状态机，根据不同状态执行不同操作
    switch (cali_state) {
        case 0: /* 空闲状态：等待触发 */
            // 检测是否触发校准
            if (cali_triggered) {
                // 设置电机电流矢量，移动到初始位置
                TB67H450_SetFocCurrentVector(go_pos, 2000);
                go_pos = SUBDIVIDE_STEPS;  // 设置起始位置
                sample_cnt = 0;  // 采样计数器清零
                cali_state = 1;  // 切换到状态1
            }
            break;

        case 1: /* 正向准备：移动到起始位置 */
            // 以自动速度增加位置
            go_pos += AUTO_SPEED;
            // 设置电机电流矢量
            TB67H450_SetFocCurrentVector(go_pos, 2000);
            if (go_pos == 2 * SUBDIVIDE_STEPS) {
                go_pos = SUBDIVIDE_STEPS;
                cali_state = 2;
            }
            break;
	
        case 2: /* 正向测量：正转一圈，采集数据 */
            if ((go_pos % SOFT_DIVIDE) == 0) {
                raw = MT6816_GetRawAngle();
                sample_raw[sample_cnt++] = raw;
                if (sample_cnt == SAMPLE_PER_STEP) {
                    /* 16次采样取平均，存入正向数据数组 */
                    int idx = (go_pos - SUBDIVIDE_STEPS) / SOFT_DIVIDE;
                    sample_fwd[idx] = CycleDataAvg(sample_raw, SAMPLE_PER_STEP, ENC_RESOLUTION);
                    sample_cnt = 0;
                    go_pos += FINE_SPEED;
                }
            } else {
                go_pos += FINE_SPEED;
            }
            TB67H450_SetFocCurrentVector(go_pos, 2000);
            if (go_pos > 2 * SUBDIVIDE_STEPS) {
                cali_state = 3;
            }
            break;

        case 3: /* 反向返回：返回起点附近 */
            go_pos += FINE_SPEED;
            TB67H450_SetFocCurrentVector(go_pos, 2000);
            if (go_pos == 2 * SUBDIVIDE_STEPS + SOFT_DIVIDE * 20) {
                cali_state = 4;
            }
            break;

        case 4: /* 消除间隙：反向移动消除齿轮间隙 */
            go_pos -= FINE_SPEED;
            TB67H450_SetFocCurrentVector(go_pos, 2000);
            if (go_pos == 2 * SUBDIVIDE_STEPS) {
                cali_state = 5;
            }
            break;

        case 5: /* 反向测量：反转一圈，采集数据 */
            if ((go_pos % SOFT_DIVIDE) == 0) {
                raw = MT6816_GetRawAngle();
                sample_raw[sample_cnt++] = raw;
                if (sample_cnt == SAMPLE_PER_STEP) {
                    /* 16次采样取平均，存入反向数据数组 */
                    int idx = (go_pos - SUBDIVIDE_STEPS) / SOFT_DIVIDE;
                    sample_rev[idx] = CycleDataAvg(sample_raw, SAMPLE_PER_STEP, ENC_RESOLUTION);
                    sample_cnt = 0;
                    go_pos -= FINE_SPEED;
                }
            } else {
                go_pos -= FINE_SPEED;
            }
            TB67H450_SetFocCurrentVector(go_pos, 2000);
            if (go_pos < SUBDIVIDE_STEPS) {
                cali_state = 6;
            }
            break;

        case 6: /* 计算状态：停止电机输出 */
            TB67H450_SetFocCurrentVector(0, 0);
            break;
    }
}

/**
 * @brief 主循环处理函数
 * @details 在计算状态下，处理数据检查、生成校准表、写入Flash
 * @note 必须在主循环中周期性调用
 */
void EncoderCalibrator_TickMainLoop(void)
{
    if (cali_state != 6) return;
    /* 休眠电机 */
    TB67H450_Sleep();
    /* 检查数据有效性 */
    CheckData();
    /* 数据有效则生成校准表 */
    if (cali_error == 0) {
        GenerateTable();
    }
    /* 重置状态 */
    cali_state = 0;
    cali_triggered = false;

    /* 校准成功，系统复位 */
    if (cali_error == 0) {
			    uint16_t *p = (uint16_t *)STOCKPILE_APP_CALI_ADDR;
    printf("Flash check: [0]=%u, [1]=%u, [2]=%u\r\n", p[0], p[1], p[2]);
    printf("cali_table check: [0]=%u\r\n", cali_table[0]);
        HAL_NVIC_SystemReset();
    }
}

/**
 * @brief 检查是否已完成校准
 * @return true=已校准，false=未校准
 */
bool EncoderCalibrator_IsCalibrated(void)
{
    return cali_is_calibrated;
}

/**
 * @brief 获取校准后的角度
 * @param raw_angle 原始角度（0-16383）
 * @return 校准后的角度
 * @note 如果未校准，直接返回原始角度
 */
uint16_t EncoderCalibrator_GetRectifiedAngle(uint16_t raw_angle)
{
    if (cali_is_calibrated) {
        return cali_table[raw_angle];
    }
    return raw_angle;
}

/**
 * @brief 获取校准触发状态
 * @return true=正在校准，false=未触发
 */
bool EncoderCalibrator_IsTriggered(void)
{
    return cali_triggered;
}

