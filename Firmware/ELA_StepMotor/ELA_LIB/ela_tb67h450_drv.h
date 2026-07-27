/********
 * @ 文件: ela_tb67h450_drv.h
 * @ 作者: ELACO
 * @ 日期: 2026-07-23
 * @ 版本: 1.0.0
 * @ 说明: TB67H450 步进电机硬件驱动层，GPIO/PWM 原语
 ********/

#ifndef ELA_TB67H450_DRV_H
#define ELA_TB67H450_DRV_H

#include <stdbool.h>
#include <stdint.h>

/* 电机驱动引脚控制宏 */
#define AP_HIGH() \
    HAL_GPIO_WritePin(AP_GPIO_Port, AP_Pin, GPIO_PIN_SET)
#define AP_LOW() \
    HAL_GPIO_WritePin(AP_GPIO_Port, AP_Pin, GPIO_PIN_RESET)
#define AM_HIGH() \
    HAL_GPIO_WritePin(AM_GPIO_Port, AM_Pin, GPIO_PIN_SET)
#define AM_LOW() \
    HAL_GPIO_WritePin(AM_GPIO_Port, AM_Pin, GPIO_PIN_RESET)
#define BP_HIGH() \
    HAL_GPIO_WritePin(BP_GPIO_Port, BP_Pin, GPIO_PIN_SET)
#define BP_LOW() \
    HAL_GPIO_WritePin(BP_GPIO_Port, BP_Pin, GPIO_PIN_RESET)
#define BM_HIGH() \
    HAL_GPIO_WritePin(BM_GPIO_Port, BM_Pin, GPIO_PIN_SET)
#define BM_LOW() \
    HAL_GPIO_WritePin(BM_GPIO_Port, BM_Pin, GPIO_PIN_RESET)
#define EN_HIGH() \
    HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_SET)
#define EN_LOW() \
    HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET)

/********
 * @ 输入: current_a: A 相 PWM 占空比
 *         current_b: B 相 PWM 占空比
 * @ 说明: 设置两相 PWM 占空比
 ********/
void tb67h450_drv_set_two_coils_current(
    uint16_t current_a, uint16_t current_b);

/********
 * @ 输入: status_ap: A+ 引脚状态
 *         status_am: A- 引脚状态
 * @ 说明: 设置 A 相方向引脚电平
 ********/
void tb67h450_drv_set_dire_a(bool status_ap, bool status_am);

/********
 * @ 输入: status_bp: B+ 引脚状态
 *         status_bm: B- 引脚状态
 * @ 说明: 设置 B 相方向引脚电平
 ********/
void tb67h450_drv_set_dire_b(bool status_bp, bool status_bm);

#endif

