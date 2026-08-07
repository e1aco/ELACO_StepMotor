/*****************************************************************************
 * @文件: ela_pow_det_drv.c
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: 电源电压检测硬件驱动层，ADC1 读取原语
 ****************************************************************************/

#include "ela_pow_det_drv.h"
#include "adc.h"

/* ==== 接口实现 ==== */
/********
 * @输出: ADC 原始值 (0-4095)
 * @说明: 启动 ADC1 单次转换，轮询等待转换完成，返回原始值
 ********/
uint16_t DRV_PowDet_ReadAdc(void)

{
    uint16_t value = 0;

    if (HAL_ADC_Start(&hadc1) != HAL_OK)
        return 0;

    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
        value = HAL_ADC_GetValue(&hadc1);
    }

    HAL_ADC_Stop(&hadc1);
    return value;
}






