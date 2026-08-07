/*****************************************************************************
 * @文件: ela_tb67h450_drv.c
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: TB67H450 电机驱动硬件层，PWM 与方向引脚原语
 ****************************************************************************/

#include "ela_tb67h450_drv.h"
#include "tim.h"

/* ==== 接口实现 ==== */
/********
 * @输入: current_a: A 相电流值 (DAC 12-bit 0~4095)
 *         current_b: B 相电流值 (DAC 12-bit 0~4095)
 * @说明: 设置两相电流，右移2位把 12-bit DAC 值
 *         映射到 TIM2 10-bit PWM (ARR=1023)
 ********/
void DRV_TB67H450_SetTwoCoilsCurrent(
    uint16_t current_a, uint16_t current_b)

{
    __HAL_TIM_SET_COMPARE(&htim2,
                          TIM_CHANNEL_4, current_a >> 2);
    __HAL_TIM_SET_COMPARE(&htim2,
                          TIM_CHANNEL_3, current_b >> 2);
}

/********
 * @输入: status_ap: A+ 引脚状态
 *         status_am: A- 引脚状态
 * @说明: 设置 A 相方向引脚电平
 ********/
void DRV_TB67H450_SetDireA(bool status_ap, bool status_am)
{
    if (status_ap)
    {
        AP_HIGH();
    }
    else
    {
        AP_LOW();
    }

    if (status_am)
    {
        AM_HIGH();
    }
    else
    {
        AM_LOW();
    }
}

/********
 * @输入: status_bp: B+ 引脚状态
 *         status_bm: B- 引脚状态
 * @说明: 设置 B 相方向引脚电平
 ********/
void DRV_TB67H450_SetDireB(bool status_bp, bool status_bm)
{
    if (status_bp)
    {
        BP_HIGH();
    }
    else
    {
        BP_LOW();
    }

    if (status_bm)
    {
        BM_HIGH();
    }
    else
    {
        BM_LOW();
    }
}

/********
 * @输入: step: 步进电机驱动步序值 (0-3)
 * @说明: 单相励磁驱动一步
 ********/
void DRV_TB67H450_DriveStep(unsigned char step)
{
    switch (step)
    {
        case 0:
            DRV_TB67H450_SetDireA(1, 0);
            DRV_TB67H450_SetDireB(0, 0);
            break;
        case 1:
            DRV_TB67H450_SetDireA(0, 0);
            DRV_TB67H450_SetDireB(1, 0);
            break;
        case 2:
            DRV_TB67H450_SetDireA(0, 1);
            DRV_TB67H450_SetDireB(0, 0);
            break;
        case 3:
            DRV_TB67H450_SetDireA(0, 0);
            DRV_TB67H450_SetDireB(0, 1);
            break;
        default:
            break;
    }
}






