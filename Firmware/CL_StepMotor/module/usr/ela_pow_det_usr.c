/*****************************************************************************
 * @文件: ela_pow_det_usr.c
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: 电源电压检测应用层，ADC 值转电压并串口打印
 ****************************************************************************/

#include "ela_pow_det_usr.h"
#include "ela_pow_det_drv.h"
#include <stdio.h>

#define POW_DET_TICK_INTERVAL_MS    1000

static uint32_t s_last_tick = 0;

/* ==== 接口实现 ==== */
/********
 * @说明: 初始化电源电压检测模块，记录初始时间戳
 ********/
void USR_PowDet_Init(void)

{
    s_last_tick = 0;
}

/********
 * @说明: 每秒读取一次 ADC，计算电压值并串口打印。
 *         在主循环中调用，非阻塞
 ********/
void USR_PowDet_Tick(void)
{
    uint32_t now = HAL_GetTick();

    if ((now - s_last_tick) < POW_DET_TICK_INTERVAL_MS)
        return;

    s_last_tick = now;

    uint16_t adc_val = DRV_PowDet_ReadAdc();
    float voltage = (float)adc_val / POW_DET_ADC_RESOLUTION * POW_DET_VREF;

    printf("[POW_DET] ADC: %4u, Voltage: %.3f V\r\n",
           adc_val, voltage);
}






