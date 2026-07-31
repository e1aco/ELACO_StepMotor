/********
 * @ 文件: ela_pow_det_usr.h
 * @ 作者: ELACO
 * @ 日期: 2026-07-31
 * @ 版本: 1.0.0
 * @ 说明: 电源电压检测应用层，读取 ADC 并串口打印
 ********/

#ifndef ELA_POW_DET_USR_H
#define ELA_POW_DET_USR_H

#include <stdint.h>

void ela_pow_det_init(void);
void ela_pow_det_tick(void);

#endif
