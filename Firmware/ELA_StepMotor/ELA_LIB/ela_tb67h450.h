/********
 * @ 文件: ela_tb67h450.h
 * @ 作者: ELACO
 * @ 日期: 2026-07-17
 * @ 版本: 1.0.0
 * @ 说明: TB67H450 步进电机驱动头文件
 * @ 依赖: ela_sinform
 ********/

#ifndef ELA_TB67H450_H
#define ELA_TB67H450_H

#include <stdbool.h>
#include <stdint.h>
#include "ela_sinform.h"
#include "ela_tb67h450_drv.h"

#define SINE_MASK       0x03FF
#define DAC_MASK        0x0FFF
#define DAC_SCALE_FACTOR 5083

/********
 * @ 说明: 驱动电流获取结构体
 ********/
typedef struct {
    unsigned short sin_map_ptr;
    short sin_map_data;
    unsigned short dac_value;
} TB67H450_CURRENT_T;

extern TB67H450_CURRENT_T g_tb67h450_a_st; /* A相电流结构体 */
extern TB67H450_CURRENT_T g_tb67h450_b_st; /* B相电流结构体 */

/* 函数声明 */
void ela_tb67h450_set_foc_current(
    unsigned short direction, short current_ma);
void ela_tb67h450_brake(void);
void ela_tb67h450_sleep(void);

#endif

