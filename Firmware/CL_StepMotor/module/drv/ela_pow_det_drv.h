/*****************************************************************************
 * @文件: ela_pow_det_drv.h
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: 电源电压检测硬件驱动层，ADC 读取原语
 ****************************************************************************/

#ifndef ELA_POW_DET_DRV_H
#define ELA_POW_DET_DRV_H

#include <stdint.h>
#include "main.h"

/* ==== 常量定义 ==== */
#define POW_DET_ADC_RESOLUTION  4095
#define POW_DET_VREF            3.3f

/* ==== 接口 ==== */


uint16_t DRV_PowDet_ReadAdc(void);

#endif






