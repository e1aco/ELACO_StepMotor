/********
 * @ 文件: test_openloop.c
 * @ 作者: ELACO
 * @ 日期: 2026-08-02
 * @ 版本: 1.0.0
 * @ 说明: 最简开环转动验证：TIM4 20kHz 中断驱动电机
 *         s_step += 2 连续转一圈（51200 微步），验证
 *         电机是否能够平稳转起、转完一整圈后制动。
 *         不读取编码器，不闭环。
 * @ 注意: 与 elaco_main.c 的 TIM4 回调冲突，ModTest
 *         下 elaco_main.c 回调需用 #ifndef ModTest 包裹
 ********/

#include "elaco_main.h"
#include "test_openloop.h"
#include "ela_tb67h450_usr.h"
#include "ela_uart_usr.h"
#include "tim.h"
#include <stdio.h>

/********
 * @ 说明: 电机参数。50 极对，1 圈 = 50*1024 = 51200 微步
 ********/
#define POLE_PAIRS     50
#define REV_STEPS      (POLE_PAIRS * 1024)

/* 旋转控制 */
static volatile unsigned long s_step = 0;
static volatile unsigned char s_running = 0;

/********
 * @ 说明: TIM4 20kHz 周期中断回调，开环驱动电机转一圈
 * @ 注意: 校准程序启用时由 elaco_main.c 提供此回调，
 *         ModTest 下本实现用 #ifndef ModTest 排除避免重复定义
 ********/
#ifndef ModTest
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4)
    {
        if (s_running)
        {
            s_step = s_step + 2;
            ela_tb67h450_set_foc_current(
                (unsigned int)s_step, 2000);
            if (s_step >= REV_STEPS)
            {
                s_running = 0;
                ela_tb67h450_brake();
            }
        }
    }
}
#endif

/********
 * @ 说明: 开环转动一圈验证主函数。
 *         启动 PWM + TIM4 中断，电机连续转一圈后制动
 ********/
void test_openloop(void)
{
    printf("--- OpenLoop Rotation Test Start ---\r\n");

    /* 启动 TIM2 PWM */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
    printf("PWM started (2000mA)\r\n");

    /* 启动 TIM4 中断，开环转一圈 */
    s_step = 0;
    s_running = 1;
    HAL_TIM_Base_Start_IT(&htim4);
    printf("Rotating 1 revolution...\r\n");

    /* 等待旋转完成 */
    while (1)
    {
        if (s_running == 0)
        {
            HAL_TIM_Base_Stop_IT(&htim4);
            printf("--- Rotation Done (51200 steps) ---\r\n");
            while (1)
            {
            }
        }
    }
}
