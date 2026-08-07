/*****************************************************************************
 * @文件: ela_tb67h450_usr.c
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: TB67H450 电机驱动应用层接口，提供开源放大等功能
 ****************************************************************************/

#include "ela_tb67h450_usr.h"
#include "ela_tb67h450_drv.h"

/* ==== 全局实例 ==== */
/* ==== 接口实现 ==== */

/********
 * @输入: direction: 电角度位置; current_ma: 目标电流 mA
 * @说明: 设置 FOC 步进电流，含正弦查表、方向控制、
 *        PWM 占空比转换
 * @注意: direction 范围 [0, TB67H450_MICROSTEP_MAX)，
 *        每次调用重设
 ********/
void USR_TB67H450_SetFocCurrent(
    unsigned int direction, short current_ma)
{
    unsigned int idx;
    int sin_a, sin_b;
    long cur_a, cur_b;
    unsigned short pwm_a, pwm_b;

    idx = direction & 0x3FF;

    sin_a = sin_form[idx];
    sin_b = sin_form[(idx + SIN_FORM_SIZE / 4)
                     % SIN_FORM_SIZE];

    cur_a = (long)sin_a * current_ma >> SIN_SCALE;
    cur_b = (long)sin_b * current_ma >> SIN_SCALE;

    if (cur_a >= 0)
    {
        DRV_TB67H450_SetDireA(1, 0);
        /* mA 转 DAC 值 (0-4095, 满量程 3300mA) */
        pwm_a = (unsigned short)(cur_a * DAC_SCALE_FACTOR
                                  >> SIN_SCALE);
    }
    else
    {
        DRV_TB67H450_SetDireA(0, 1);
        pwm_a = (unsigned short)((-cur_a) * DAC_SCALE_FACTOR
                                  >> SIN_SCALE);
    }

    if (cur_b >= 0)
    {
        DRV_TB67H450_SetDireB(1, 0);
        /* mA 转 DAC 值 (0-4095, 满量程 3300mA) */
        pwm_b = (unsigned short)(cur_b * DAC_SCALE_FACTOR
                                  >> SIN_SCALE);
    }
    else
    {
        DRV_TB67H450_SetDireB(0, 1);
        pwm_b = (unsigned short)((-cur_b) * DAC_SCALE_FACTOR
                                  >> SIN_SCALE);
    }

    DRV_TB67H450_SetTwoCoilsCurrent(
        pwm_a & DAC_MASK, pwm_b & DAC_MASK);
}

/********
 * @说明: 制动，两相 PWM 置 0
 ********/
void USR_TB67H450_Brake(void)
{
    DRV_TB67H450_SetTwoCoilsCurrent(0, 0);
}

/********
 * @说明: 休眠，两相 PWM 置 0
 ********/
void USR_TB67H450_Sleep(void)
{
    DRV_TB67H450_SetTwoCoilsCurrent(0, 0);
}

/********
 * @说明: 开源放大步进电流驱动函数
 * @注意: 中断中调用（20kHz 定时器），需保持简短
 ********/
void TB67H450_OpenSourceAmplify(void)
{
    static unsigned char step = 0;
    static unsigned short pwm_a = 0;
    static unsigned short pwm_b = 0;
    static unsigned char pwm_cnt = 0;

    pwm_cnt++;

    switch (step)
    {
        case 0:
            pwm_a = 100;
            pwm_b = 0;
            DRV_TB67H450_SetDireA(1, 0);
            DRV_TB67H450_SetDireB(0, 0);
            break;
        case 1:
            pwm_a = 0;
            pwm_b = 100;
            DRV_TB67H450_SetDireA(0, 0);
            DRV_TB67H450_SetDireB(1, 0);
            break;
        case 2:
            pwm_a = 100;
            pwm_b = 0;
            DRV_TB67H450_SetDireA(0, 1);
            DRV_TB67H450_SetDireB(0, 0);
            break;
        case 3:
            pwm_a = 0;
            pwm_b = 100;
            DRV_TB67H450_SetDireA(0, 0);
            DRV_TB67H450_SetDireB(0, 1);
            break;
        default:
            break;
    }

    DRV_TB67H450_SetTwoCoilsCurrent(
        pwm_a & DAC_MASK, pwm_b & DAC_MASK);

    if (pwm_cnt > 50)
    {
        pwm_cnt = 0;
        step = (step + 1) % 4;
    }
}






