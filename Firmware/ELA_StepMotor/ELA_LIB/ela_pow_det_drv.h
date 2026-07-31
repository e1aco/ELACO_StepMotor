/********
 * @ 文件: ela_pow_det_drv.h
 * @ 作者: ELACO
 * @ 日期: 2026-07-31
 * @ 版本: 1.0.0
 * @ 说明: 电源电压检测硬件驱动层，ADC 读取原语
 ********/

#ifndef ELA_POW_DET_DRV_H
#define ELA_POW_DET_DRV_H

#include <stdint.h>
#include "main.h"

#define POW_DET_ADC_RESOLUTION  4095
#define POW_DET_VREF            3.3f

uint16_t pow_det_drv_read_adc(void);

#endif
