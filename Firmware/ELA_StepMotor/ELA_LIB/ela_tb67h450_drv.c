/********
 * @ 文件: ela_tb67h450_drv.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-23
 * @ 版本: 1.0.0
 * @ 说明: TB67H450 电机驱动硬件层，PWM 与方向引脚原语
 ********/

#include "ela_tb67h450_drv.h"
#include "tim.h"

/* tb67h450 drv start */

/********
 * @ 输入: current_a: A 相 PWM 占空比
 *         current_b: B 相 PWM 占空比
 * @ 说明: 设置两相 PWM 占空比
 ********/
void tb67h450_drv_set_two_coils_current(
    uint16_t current_a, uint16_t current_b)
{
    __HAL_TIM_SET_COMPARE(&htim2,
                          TIM_CHANNEL_4, current_a);
    __HAL_TIM_SET_COMPARE(&htim2,
                          TIM_CHANNEL_3, current_b);
}

/********
 * @ 输入: status_ap: A+ 引脚状态
 *         status_am: A- 引脚状态
 * @ 说明: 设置 A 相方向引脚电平
 ********/
void tb67h450_drv_set_dire_a(bool status_ap, bool status_am)
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
 * @ 输入: status_bp: B+ 引脚状态
 *         status_bm: B- 引脚状态
 * @ 说明: 设置 B 相方向引脚电平
 ********/
void tb67h450_drv_set_dire_b(bool status_bp, bool status_bm)
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
 * @ 输入: step: 步进电机驱动步序值 (0-3)
 * @ 说明: 单相励磁驱动一步
 ********/
void tb67h450_drv_drive_step(unsigned char step)
{
    switch (step)
    {
        case 0:
            tb67h450_drv_set_dire_a(1, 0);
            tb67h450_drv_set_dire_b(0, 0);
            break;
        case 1:
            tb67h450_drv_set_dire_a(0, 0);
            tb67h450_drv_set_dire_b(1, 0);
            break;
        case 2:
            tb67h450_drv_set_dire_a(0, 1);
            tb67h450_drv_set_dire_b(0, 0);
            break;
        case 3:
            tb67h450_drv_set_dire_a(0, 0);
            tb67h450_drv_set_dire_b(0, 1);
            break;
        default:
            break;
    }
}

/* tb67h450 drv end */

