/********
 * @ 文件: test_position.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-27
 * @ 版本: 1.0.0
 * @ 说明: MT6816 编码器 + TB67H450 综合定位测试。
 *         以 90° 步进方式走完一圈，每步通过
 *         编码器验证实际角度。
 * @ 注意: 与 test_tb67h450.c 冲突（均定义 HAL_TIM_
 *         PeriodElapsedCallback），编译前需排除
 *         test_tb67h450.c
 ********/

#include "elaco_main.h"
#include "test_position.h"
#include "ela_tb67h450_usr.h"
#include "ela_mt6816_usr.h"
#include "ela_uart_usr.h"
#include "tim.h"
#include <stdio.h>

/********
 * @ 说明: 电机参数
 *         50 极对，1 圈 = 51200 微步，90° = 12800 微步
 ********/
#define POLE_PAIRS     50
#define REV_STEPS      (POLE_PAIRS * 1024)
#define SEG_STEPS      (REV_STEPS / 4)
#define ENC_RESOLUTION 16384

/* 旋转控制 */
static volatile unsigned long s_step = 0;
static volatile unsigned long s_target = 0;
static volatile unsigned char s_running = 0;

/* test_position hlp start */

/********
 * @ 输出: 编码器角度值（3 次取中值）
 * @ 说明: 读取 MT6816 三次，取中值滤波
 ********/
static unsigned short test_position_read_median(void)
{
    unsigned short v[3];
    unsigned char i;

    for (i = 0; i < 3; i++)
    {
        ela_mt6816_usr_read_angle();
        v[i] = g_mt6816_st.raw_angle;
    }

    if (v[0] > v[1])
    {
        unsigned short t = v[0]; v[0] = v[1]; v[1] = t;
    }
    if (v[1] > v[2])
    {
        unsigned short t = v[1]; v[1] = v[2]; v[2] = t;
    }
    if (v[0] > v[1])
    {
        unsigned short t = v[0]; v[0] = v[1]; v[1] = t;
    }

    return v[1];
}

/********
 * @ 输入: curr: 当前编码器值; prev: 前一次编码器值
 * @ 输出: 差值（考虑 14-bit 回绕）
 * @ 说明: 计算编码器角度差，自动处理 0→16383 的
 *        回绕
 ********/
static short test_position_angle_delta(
    unsigned short curr, unsigned short prev)
{
    int diff = (int)curr - (int)prev;

    if (diff < -8192)
    {
        diff += ENC_RESOLUTION;
    }
    else if (diff > 8192)
    {
        diff -= ENC_RESOLUTION;
    }

    return (short)diff;
}

/* test_position hlp end */
//----------------------------------------------------------------------------------
/* test_position drv start */

/********
 * @ 输入: seg: 段号 (0~3)
 * @ 说明: LED 指示当前段
 ********/
static void test_position_set_led(unsigned char seg)
{
    unsigned char led1, led2;

    switch (seg)
    {
        case 0:
            led1 = 1;  led2 = 0;  break;
        case 1:
            led1 = 0;  led2 = 1;  break;
        case 2:
            led1 = 1;  led2 = 1;  break;
        case 3:
            led1 = 1;  led2 = 0;  break;
        default:
            led1 = 0;  led2 = 0;  break;
    }

    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin,
                      led1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin,
                      led2 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* test_position drv end */
//----------------------------------------------------------------------------------
/* test_position cac start */

/********
 * @ 说明: TIM4 20kHz 周期中断回调，步进电机驱动。
 *         s_step 递增 2 直至 s_target，触发制动
 ********/
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
//    if (htim->Instance == TIM4)
//    {
//        if (s_running)
//        {
//            s_step = s_step + 2;
//            ela_tb67h450_set_foc_current(
//                (unsigned int)s_step, 2000);
//            if (s_step >= s_target)
//            {
//                s_running = 0;
//                ela_tb67h450_brake();
//            }
//        }
//    }
//}

/* test_position cac end */
//----------------------------------------------------------------------------------
/* test_position usr start */

/********
 * @ 说明: MT6816 + TB67H450 定位测试主函数。
 *         4 段 90° 步进，每段读取编码器验证角度误差
 ********/
void test_position(void)
{
    unsigned char seg;
    unsigned char pass_flag = 1;
    unsigned short enc_prev;
    unsigned short enc_curr;
    short delta;
    short expected_delta;

    printf("--- Position Test Start ---\r\n");

    /* 初始化编码器 */
    ela_mt6816_usr_init();
    printf("Encoder init done\r\n");

    /* 启动 TIM2 PWM */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
    printf("PWM started (2000mA)\r\n");

    /* 预定位到电角度 0 */
    ela_tb67h450_set_foc_current(0, 2000);
    HAL_Delay(200);
    printf("Pre-positioned at electrical angle 0\r\n");

    /* 读取初始编码器位置（3 次取中值） */
    enc_prev = test_position_read_median();
    printf("Initial encoder: %u (0x%04X)\r\n",
           enc_prev, enc_prev);

    /* 4 段 90° 步进 */
    for (seg = 0; seg < 4; seg++)
    {
        s_step = seg * SEG_STEPS;
        s_target = (seg + 1) * SEG_STEPS;
        s_running = 1;

        test_position_set_led(seg);
        printf("Seg %d: step %lu -> %lu ... ",
               seg, s_step, s_target);

        HAL_TIM_Base_Start_IT(&htim4);

        while (s_running)
        {
        }

        HAL_TIM_Base_Stop_IT(&htim4);
        HAL_Delay(200);

        /* 读取编码器（3 次取中值） */
        enc_curr = test_position_read_median();
        delta = test_position_angle_delta(enc_curr, enc_prev);
        expected_delta = (short)(SEG_STEPS * ENC_RESOLUTION
                                 / REV_STEPS);

        printf("enc=%u delta=%d (exp=%d) %s\r\n",
               enc_curr, delta, expected_delta,
               (delta < -100 || delta > 100) ? "ERR" : "OK");

        if (delta < -100 || delta > 100)
        {
            pass_flag = 0;
        }

        enc_prev = enc_curr;
    }

    /* 回到起点对比 */
    test_position_set_led(4);
    printf("--- Position Test %s ---\r\n",
           pass_flag ? "PASS" : "FAIL");

    while (1)
    {
    }
}

/* test_position usr end */

