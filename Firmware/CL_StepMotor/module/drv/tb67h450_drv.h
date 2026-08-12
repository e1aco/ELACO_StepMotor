/*****************************************************************************
 * @文件: tb67h450_drv.h
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v2.0
 * @说明: TB67H450 两相步进驱动硬件原语（TIM2 电流 PWM + 方向脚，无业务逻辑）
 * @平台: STM32F103RET6 (TIM2_CH3=PB10/PWM_B, TIM2_CH4=PB11/PWM_A)
 * @依赖: HAL_TIM, HAL_GPIO
 ****************************************************************************/
#ifndef TB67H450_DRV_H
#define TB67H450_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ==== 常量定义 ==== */
/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
/* ==== 接口 ==== */
void DRV_TB67H450_Init(void);
void DRV_TB67H450_SetCoilCurrent(uint16_t duty_a_12bit, uint16_t duty_b_12bit);
void DRV_TB67H450_SetDirectionA(bool status_ap, bool status_am);
void DRV_TB67H450_SetDirectionB(bool status_bp, bool status_bm);

#ifdef __cplusplus
}
#endif

#endif /* TB67H450_DRV_H */
