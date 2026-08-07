/*****************************************************************************
 * @文件: ela_tb67h450_usr.h
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: TB67H450 步进电机应用层头文件
 ****************************************************************************/

#ifndef ELA_TB67H450_USR_H
#define ELA_TB67H450_USR_H

#include <stdbool.h>
#include <stdint.h>
#include "ela_sinform.h"
#include "ela_tb67h450_drv.h"

/* ==== 常量定义 ==== */
#define SINE_MASK       0x03FF // 1023
#define DAC_MASK        0x0FFF
#define DAC_SCALE_FACTOR 5083

/* ==== 类型定义 ==== */
/********
 * @说明: 驱动电流获取结构体
 ********/
typedef struct {
    unsigned short sin_map_ptr;
    short sin_map_data;
    unsigned short dac_value;
} TB67H450_CURRENT_T;

/* ==== 全局实例 ==== */
extern TB67H450_CURRENT_T g_tb67h450_a_st; /* A相电流结构体 */
extern TB67H450_CURRENT_T g_tb67h450_b_st; /* B相电流结构体 */

/* 函数声明 */
void USR_TB67H450_SetFocCurrent(
    unsigned int direction, short current_ma);

/* ==== 接口 ==== */



void USR_TB67H450_Brake(void);
void USR_TB67H450_Sleep(void);

#endif






