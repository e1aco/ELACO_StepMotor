/*****************************************************************************
 * @文件: ela_pow_det_usr.h
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: 电源电压检测应用层，读取 ADC 并串口打印
 ****************************************************************************/

#ifndef ELA_POW_DET_USR_H
#define ELA_POW_DET_USR_H

#include <stdint.h>

/* ==== 接口 ==== */


void USR_PowDet_Init(void);
void USR_PowDet_Tick(void);

#endif






