/*****************************************************************************
 * @文件: tb67h450_usr.c
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v1.0
 * @说明: TB67H450 用户层（两相正弦 FOC 电流矢量算法：查表分解/缩放/方向判断，只调 DRV 原语）
 * @平台: STM32F103RET6
 * @依赖: tb67h450_drv, sin_form_usr
 ****************************************************************************/
#include "tb67h450_usr.h"
#include "tb67h450_drv.h"
#include "sin_form_usr.h"

/* ==== 常量定义 ==== */
#define USR_TB67H450_ELEC_MASK   0x000003FFU   /* 电角度 10bit 掩码(0~1023=一电周期) */
#define USR_TB67H450_PHASE_OFF   256U          /* A/B 相 90° 相位差(1024/4) */
#define USR_TB67H450_CUR_COEF    5083U         /* mA→12bit DAC 系数(dac=current×5083>>12) */
#define USR_TB67H450_CUR_MASK    0x00000FFFU   /* 12bit DAC 掩码(0~4095) */

/* ==== 类型定义 ==== */
/* 单相电流矢量中间量 */
typedef struct {
    uint16_t sin_map_ptr;      /* 电角度查表指针(0~1023) */
    int16_t  sin_map_data;     /* 正弦表值(-4096~4096) */
    uint16_t dac_value_12bits; /* 12bit 占空比(0~4095) */
} TB67H450_Phase_T;

/* ==== 全局实例 ==== */
static TB67H450_Phase_T s_phase_a;
static TB67H450_Phase_T s_phase_b;

/* ==== 内部工具 ==== */
/* ==== 接口实现 ==== */
/**
 * @输入 无
 * @输出 无
 * @说明 初始化驱动（透传 DRV 原语：PWM 启动 + 方向脚置低不励磁）
 */
void USR_TB67H450_Init(void)
{
    DRV_TB67H450_Init();
}

/**
 * @输入 direction_in_count: 电角度(0~1023); current_mA: 电流幅度(mA)
 * @输出 无
 * @说明 设置 FOC 电流矢量：电角度查正弦表分 A/B 两相(90° 相差)，电流幅度 mA→12bit DAC，
 *   输出两相 PWM 占空比 + 方向脚
 * 依据 .cl/memory/ tb67h450_current_coef=5083>>12(mA→DAC, 满量程 3.3A@4095)
 *   + sin_pi_m2 整电周期正弦表(1024 点, 幅值 4096) + tb67h450_phase_off=256
 *   A 相领先 B 相 90°(ptr+256)；sin>0 正向 / sin<0 反向 / =0 同高刹车
 *   仅调 DRV_TB67H450_* 原语，无直接寄存器操作
 */
void USR_TB67H450_SetFocCurrentVector(uint32_t direction_in_count, int32_t current_mA)
{
    uint32_t dac_reg;
    int32_t current_abs;
    int16_t abs_a;
    int16_t abs_b;

    /* 1. 电角度 → A/B 相查表指针（B 相=输入角度，A 相=B+90°） */
    s_phase_b.sin_map_ptr = direction_in_count & USR_TB67H450_ELEC_MASK;
    s_phase_a.sin_map_ptr = (s_phase_b.sin_map_ptr + USR_TB67H450_PHASE_OFF)
                            & USR_TB67H450_ELEC_MASK;

    /* 2. 查正弦表 */
    s_phase_a.sin_map_data = USR_sin_pi_m2[s_phase_a.sin_map_ptr];
    s_phase_b.sin_map_data = USR_sin_pi_m2[s_phase_b.sin_map_ptr];

    /* 3. 电流幅度 mA → 12bit DAC（dac = mA×5083>>12，满量程 3300mA→4095） */
    current_abs = (current_mA > 0) ? current_mA : -current_mA;
    dac_reg = (uint32_t)((uint32_t)current_abs * USR_TB67H450_CUR_COEF) >> 12;
    dac_reg = dac_reg & USR_TB67H450_CUR_MASK;

    /* 4. 乘正弦幅度得各相 12bit 占空比（>>12 归一正弦幅值 4096） */
    abs_a = (s_phase_a.sin_map_data > 0) ? s_phase_a.sin_map_data
                                         : -s_phase_a.sin_map_data;
    abs_b = (s_phase_b.sin_map_data > 0) ? s_phase_b.sin_map_data
                                         : -s_phase_b.sin_map_data;
    s_phase_a.dac_value_12bits = (uint16_t)((dac_reg * (uint32_t)abs_a)
                                            >> USR_SIN_PI_M2_DPIYBIT);
    s_phase_b.dac_value_12bits = (uint16_t)((dac_reg * (uint32_t)abs_b)
                                            >> USR_SIN_PI_M2_DPIYBIT);

    /* 5. 输出两相 PWM 占空比 */
    DRV_TB67H450_SetCoilCurrent(s_phase_a.dac_value_12bits,
                                s_phase_b.dac_value_12bits);

    /* 6. 方向脚：sin>0 → (P=1,M=0)；sin<0 → (P=0,M=1)；sin=0 → 同高刹车 */
    if (s_phase_a.sin_map_data > 0)
    {
        DRV_TB67H450_SetDirectionA(true, false);
    }
    else if (s_phase_a.sin_map_data < 0)
    {
        DRV_TB67H450_SetDirectionA(false, true);
    }
    else
    {
        DRV_TB67H450_SetDirectionA(true, true);
    }

    if (s_phase_b.sin_map_data > 0)
    {
        DRV_TB67H450_SetDirectionB(true, false);
    }
    else if (s_phase_b.sin_map_data < 0)
    {
        DRV_TB67H450_SetDirectionB(false, true);
    }
    else
    {
        DRV_TB67H450_SetDirectionB(true, true);
    }
}

/**
 * @输入 无
 * @输出 无
 * @说明 睡眠：PWM=0 且方向脚全低（输出关断、线圈不励磁）
 */
void USR_TB67H450_Sleep(void)
{
    DRV_TB67H450_SetCoilCurrent(0, 0);
    DRV_TB67H450_SetDirectionA(false, false);
    DRV_TB67H450_SetDirectionB(false, false);
}

/**
 * @输入 无
 * @输出 无
 * @说明 刹车：PWM=0 且方向脚同高（H 桥刹车，线圈短路制动）
 */
void USR_TB67H450_Brake(void)
{
    DRV_TB67H450_SetCoilCurrent(0, 0);
    DRV_TB67H450_SetDirectionA(true, true);
    DRV_TB67H450_SetDirectionB(true, true);
}
