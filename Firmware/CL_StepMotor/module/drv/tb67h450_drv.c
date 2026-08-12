/*****************************************************************************
 * @文件: tb67h450_drv.c
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v2.0
 * @说明: TB67H450 两相步进驱动硬件原语（TIM2 电流 PWM 比较 + 方向脚 GPIO，无业务逻辑）
 * @平台: STM32F103RET6 (TIM2_CH3=PB10/PWM_B, TIM2_CH4=PB11/PWM_A)
 * @依赖: HAL_TIM, HAL_GPIO
 ****************************************************************************/
#include "tb67h450_drv.h"
#include "tim.h"
#include "main.h"

/* ==== 常量定义 ==== */
#define DRV_TB67H450_OC_10BIT   2U   /* 12bit DAC→10bit TIM 比较值右移位数(ARR=1023) */

/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
/* ==== 内部工具 ==== */
/* ==== 接口实现 ==== */
/**
 * @输入 无
 * @输出 无
 * @说明 启动 TIM2 PWM(CH3/CH4) 并将方向脚置低（两相输出关断、不励磁）
 *   必须在使用 SetCoilCurrent 之前调用；PWM 启动与参考 main.c 一致
 */
void DRV_TB67H450_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
    DRV_TB67H450_SetDirectionA(false, false);
    DRV_TB67H450_SetDirectionB(false, false);
}

/**
 * @输入 duty_a_12bit: A相12bit占空比(0~4095); duty_b_12bit: B相12bit占空比(0~4095)
 * @输出 无
 * @说明 写 TIM2 比较寄存器输出两相电流 PWM（12bit>>2 → 10bit 匹配 ARR=1023）
 * 依据 .cl/memory/ tim2_period=1023(PWM 分辨率 10bit) + tb67h450_oc_10bit=2
 *   参考 tb67h450.c SetTwoCoilsCurrent currentA>>2
 */
void DRV_TB67H450_SetCoilCurrent(uint16_t duty_a_12bit, uint16_t duty_b_12bit)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, duty_a_12bit >> DRV_TB67H450_OC_10BIT);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, duty_b_12bit >> DRV_TB67H450_OC_10BIT);
}

/**
 * @输入 status_ap: A+ 方向脚电平; status_am: A- 方向脚电平
 * @输出 无
 * @说明 设置 A 相 H 桥方向输入（AP=PA1, AM=PA2）
 */
void DRV_TB67H450_SetDirectionA(bool status_ap, bool status_am)
{
    HAL_GPIO_WritePin(AP_GPIO_Port, AP_Pin,
                      (status_ap) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AM_GPIO_Port, AM_Pin,
                      (status_am) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @输入 status_bp: B+ 方向脚电平; status_bm: B- 方向脚电平
 * @输出 无
 * @说明 设置 B 相 H 桥方向输入（BP=PC2, BM=PC3）
 */
void DRV_TB67H450_SetDirectionB(bool status_bp, bool status_bm)
{
    HAL_GPIO_WritePin(BP_GPIO_Port, BP_Pin,
                      (status_bp) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BM_GPIO_Port, BM_Pin,
                      (status_bm) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
