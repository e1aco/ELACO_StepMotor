/********
 * @ 文件: test_tb67h450.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-20
 * @ 版本: 1.0.4
 * @ 说明: TB67H450 电机驱动测试，用 TIM4 中断
 *         驱动电机旋转一圈
 ********/

#include "elaco_main.h"
#include "test_tb67h450.h"
#include "ela_tb67h450.h"
#include "ela_mt6816.h"
#include "ela_uart.h"
#include "tim.h"
#include <stdio.h>

/********
 * @ 说明: 一圈对应的电角度步数
 *         50极对 × 1024 = 51200
 ********/
#define POLE_PAIRS 50
#define REV_STEPS (POLE_PAIRS * 1024)

/* 旋转控制 */
static volatile unsigned long s_step = 0;
static volatile unsigned long s_target = 0;
static volatile unsigned char s_running = 0;
static volatile unsigned char s_prescaler = 0;

/********
 * @ 说明: TIM4 中断回调
 *         TIM4 原始频率 20kHz，每 4 次中断
 *         执行一步，实际 5kHz，约 10 秒转一圈
 ********/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4)
    {
        s_prescaler++;
        if (s_prescaler < 4)
        {
            return;
        }
        s_prescaler = 0;

        if ((s_step < s_target))
        {
            unsigned short elec_angle =
                (unsigned short)(s_step & SINE_MASK); // 将步数转为1024
            ela_tb67h450_set_foc_current(elec_angle, 1000);
            s_step++;
        }
        else
        {
            s_running = 0;
            ela_tb67h450_brake();
        }
    }
}

/********
 * @ 说明: TB67H450 电机测试，启动 PWM 和
 *         TIM4 中断后驱动电机旋转一圈
 ********/
void test_tb67h450(void)
{
    printf("TB67H450 test start\r\n");

    /* 启动 TIM2 PWM */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
    printf("PWM started\r\n");
    ela_tb67h450_set_foc_current(0, 1000);

    /* 启动 TIM4 中断 */
    s_step = 1;
    s_target = REV_STEPS;
    s_running = 1;
    HAL_TIM_Base_Start_IT(&htim4);
    printf("Rotating 1 revolution...\r\n");

    /* 等待旋转完成 */
    while (s_running)
    {
    }

    printf("Done, motor stopped\r\n");

    while (1)
    {
    }
}
