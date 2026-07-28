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
#include "ela_tb67h450_usr.h"
#include "ela_mt6816_usr.h"
#include "ela_uart_usr.h"
#include "tim.h"
#include <stdio.h>

/********
 * @ 说明: 一圈对应的电角度步数
 *         50极对 × 1024 = 51200
 ********/
#define POLE_PAIRS 50

// 电角度1024 整步1024 微步256
#define REV_STEPS (POLE_PAIRS * 1024)

/* 旋转控制 */
static volatile unsigned long s_step = 0;
static volatile unsigned long s_target = 0;
static volatile unsigned char s_running = 0;
static volatile unsigned char s_prescaler = 0;

///********
// * @ 说明: TIM4 中断回调
// *         TIM4 原始频率 20kHz
// ********/
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
//    if (htim->Instance == TIM4)
//    {
//        s_step = s_step + 2;
//        ela_tb67h450_set_foc_current(s_step, 2000);
//        if (s_step >= REV_STEPS)
//        {
//            s_running = 0;
//            ela_tb67h450_brake();
//        }
//    }
//}

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
    s_step = 0;
    s_target = REV_STEPS;
    s_running = 1;
    HAL_TIM_Base_Start_IT(&htim4);
    printf("Rotating 1 revolution...\r\n");
	
    /* 等待旋转完成 */
    while (1)
    {
			if(s_running == 0)
				HAL_TIM_Base_Stop_IT(&htim4);		
    }
}
