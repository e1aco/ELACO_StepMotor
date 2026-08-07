/*****************************************************************************
 * @文件: ela_tb67h450_drv.h
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: TB67H450 电机驱动硬件层，PWM 与方向引脚原语
 ****************************************************************************/

#ifndef ELA_TB67H450_DRV_H
#define ELA_TB67H450_DRV_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

/* ==== 常量定义 ==== */
/* A 相方向引脚 */
#define AP_HIGH() HAL_GPIO_WritePin(AP_GPIO_Port, AP_Pin, GPIO_PIN_SET)
#define AP_LOW()  HAL_GPIO_WritePin(AP_GPIO_Port, AP_Pin, GPIO_PIN_RESET)
#define AM_HIGH() HAL_GPIO_WritePin(AM_GPIO_Port, AM_Pin, GPIO_PIN_SET)
#define AM_LOW()  HAL_GPIO_WritePin(AM_GPIO_Port, AM_Pin, GPIO_PIN_RESET)

/* B 相方向引脚 */
#define BP_HIGH() HAL_GPIO_WritePin(BP_GPIO_Port, BP_Pin, GPIO_PIN_SET)
#define BP_LOW()  HAL_GPIO_WritePin(BP_GPIO_Port, BP_Pin, GPIO_PIN_RESET)
#define BM_HIGH() HAL_GPIO_WritePin(BM_GPIO_Port, BM_Pin, GPIO_PIN_SET)
#define BM_LOW()  HAL_GPIO_WritePin(BM_GPIO_Port, BM_Pin, GPIO_PIN_RESET)

/********
 * @输入: current_a: A 相 PWM 占空比
 *         current_b: B 相 PWM 占空比
 * @说明: 设置两相 PWM 占空比
 ********/
void DRV_TB67H450_SetTwoCoilsCurrent(
    uint16_t current_a, uint16_t current_b);

/* ==== 接口 ==== */
/********
 * @输入: status_ap: A+ 引脚状态
 *         status_am: A- 引脚状态
 * @说明: 设置 A 相方向引脚电平
 ********/


void DRV_TB67H450_SetDireA(bool status_ap, bool status_am);

/********
 * @输入: status_bp: B+ 引脚状态
 *         status_bm: B- 引脚状态
 * @说明: 设置 B 相方向引脚电平
 ********/
void DRV_TB67H450_SetDireB(bool status_bp, bool status_bm);

/********
 * @输入: step: 步进电机驱动步序值 (0-3)
 * @说明: 单相励磁驱动一步
 ********/
void DRV_TB67H450_DriveStep(unsigned char step);

#endif






