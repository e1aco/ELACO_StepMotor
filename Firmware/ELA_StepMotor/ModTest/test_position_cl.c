/********
 * @ 文件: test_position_cl.c
 * @ 作者: ELACO
 * @ 日期: 2026-08-01
 * @ 版本: 1.0.0
 * @ 说明: MT6816 编码器 + TB67H450 闭环定位测试。
 *         以编码器为目标（每段前进 90° = 4096 计数），
 *         P + 误差累加器控制器在 TIM4 4kHz 更新，
 *         收敛于死区内即判定到位。
 * @ 注意: 与 test_tb67h450.c 冲突（均定义 HAL_TIM_
 *         PeriodElapsedCallback），编译前需排除
 *         test_tb67h450.c
 ********/

#include "elaco_main.h"
#include "test_position_cl.h"
#include "ela_tb67h450_usr.h"
#include "ela_mt6816_usr.h"
#include "ela_uart_usr.h"
#include "tim.h"
#include <stdio.h>

/********
 * @ 说明: 电机参数
 *         50 极对，1 圈 = 51200 微步；编码器 1 圈 = 16384 计数
 ********/
#define POLE_PAIRS      50
#define REV_STEPS       (POLE_PAIRS * 1024)
#define ENC_RESOLUTION  16384
#define SEG_ENC         (ENC_RESOLUTION / 4)   /* 90° */

/* 闭环控制参数 */
#define CTRL_DIV        5       /* 每 5 个 20kHz tick 控制一次 = 4kHz */
#define KP_SHIFT        6       /* Kp = 1/64 */
#define MAX_DELTA       4       /* 每控制周期最大步进（微步单位） */
#define ERR_ACC_MAX     (MAX_DELTA << KP_SHIFT)   /* 累加器上限 ±256，防 windup */
#define DEADBAND        4       /* 收敛死区（编码器计数），到位判定收紧到 ±4 */
#define INBAND_CONFIRM  3       /* 连续带内次数，确认稳定到位 */
#define ENC_GLITCH_MAX  16      /* 相邻周期最大可信位移，超限视为毛刺 */
#define DRIVE_MA        2000
#define HOLD_MA         2000
#define SETTLE_DELAY    500
#define TIMEOUT_MS      5000
#define POS_TOL        8       /* 定位判据：±8 计数 ≈ 0.18° */

/* 闭环状态 */
static volatile int s_cl_step = 0;
static volatile int s_cl_target = 0;   /* 目标位移量（段内 delta，无回绕） */
static volatile int s_cl_seg_start = 0; /* 段起始原始编码器值 */
static volatile int s_cl_err_acc = 0;
static volatile unsigned char s_cl_running = 0;
static volatile unsigned char s_cl_arrived = 0;
static unsigned char s_cl_tick = 0;
static unsigned char s_cl_inband_cnt = 0;

/* 诊断变量（由 ISR 更新，主循环读取） */
static volatile int s_cl_last_err = 0;
static volatile int s_cl_last_enc = 0;
static volatile int s_cl_ctrl_cnt = 0;
static volatile int s_cl_prev_delta = 0;    /* 上一控制周期位移，速度估计用 */

/* test_position_cl hlp start */

/********
 * @ 输出: 编码器角度值（3 次取中值）
 * @ 说明: 读取 MT6816 三次，取中值滤波
 ********/
static unsigned short test_position_cl_read_median(void)
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
 * @ 说明: 计算编码器角度差，自动处理 0->16383 的回绕
 ********/
static short test_position_cl_angle_delta(
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

/********
 * @ 输入: v0,v1,v2: 三个采样值
 * @ 输出: 中值
 * @ 说明: 三点中值滤波（作用于连续无回绕位置，不跨回绕）
 ********/

/* test_position_cl hlp end */
//----------------------------------------------------------------------------------
/* test_position_cl drv start */

/********
 * @ 输入: seg: 段号 (0~3)
 * @ 说明: LED 指示当前段
 ********/
static void test_position_cl_set_led(unsigned char seg)
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

/* test_position_cl drv end */
//----------------------------------------------------------------------------------
/* test_position_cl cac start */

/********
 * @ 说明: TIM4 20kHz 周期中断回调，闭环位置控制。
 *         每 5 tick（4kHz）读一次编码器；相对段起始原始值
 *         做回绕差得位移（无累加链，无跨段漂移），按误差修正
 *         电角度步进；误差累加器带 ±256 上限（防 windup），
 *         连续 INBAND_CONFIRM 次落在死区即判定到位并保持
 ********/
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
//    int r;
//    int delta;
//    int err;
//    int cmd;

//    if (htim->Instance != TIM4)
//    {
//        return;
//    }

//    if (!s_cl_running)
//    {
//        return;
//    }

//    if (++s_cl_tick < CTRL_DIV)
//    {
//        return;
//    }
//    s_cl_tick = 0;

//    /* 每控制周期读 3 次、对原始位置取中值（滤 SPI 毛刺），
//     * 相对段起始原始值做回绕差，无累加链、无跨段漂移 */
//    r = test_position_cl_read_median();

//    delta = test_position_cl_angle_delta(
//        (unsigned short)r, (unsigned short)s_cl_seg_start);
//    err = s_cl_target - delta;

//    s_cl_last_enc = r;
//    s_cl_last_err = err;
//    s_cl_ctrl_cnt++;

//    {
//        int vel = delta - s_cl_prev_delta;   /* 速度（计数/控制周期） */
//        s_cl_prev_delta = delta;

//        if (err < DEADBAND && err > -DEADBAND)
//        {
//            if (s_cl_inband_cnt < INBAND_CONFIRM)
//            {
//                s_cl_inband_cnt++;
//                ela_tb67h450_set_foc_current(
//                    (unsigned int)s_cl_step, HOLD_MA);
//                return;
//            }

//            s_cl_arrived = 1;
//            s_cl_err_acc = 0;
//            ela_tb67h450_set_foc_current(
//                (unsigned int)s_cl_step, HOLD_MA);
//            return;
//        }

//        s_cl_inband_cnt = 0;

//        s_cl_err_acc += err;

//        if (s_cl_err_acc > ERR_ACC_MAX)
//        {
//            s_cl_err_acc = ERR_ACC_MAX;
//        }
//        else if (s_cl_err_acc < -ERR_ACC_MAX)
//        {
//            s_cl_err_acc = -ERR_ACC_MAX;
//        }

//        cmd = s_cl_err_acc >> KP_SHIFT;

//        /* 速度阻尼：抵消转子欠阻尼振荡（±10~40 计数极限环）。
//         * vel 单位是计数/周期，1 微步 ≈ 0.32 计数，故右移 1 相当阻尼系数 ~1.5 */
//        cmd -= (vel >> 1);

//        /* 接近目标（|err|<64 计数）时降至 1 微步/周期，
//         * 避免高速冲入死区造成过冲-反弹 */
//        if (err < 64 && err > -64)
//        {
//            if (cmd > 1) cmd = 1;
//            else if (cmd < -1) cmd = -1;
//        }
//        else if (cmd > MAX_DELTA)
//        {
//            cmd = MAX_DELTA;
//        }
//        else if (cmd < -MAX_DELTA)
//        {
//            cmd = -MAX_DELTA;
//        }

//        s_cl_err_acc -= cmd << KP_SHIFT;

//        /* 编码器递增方向对应 s_cl_step 递减方向 */
//        s_cl_step -= cmd;
//        ela_tb67h450_set_foc_current(
//            (unsigned int)s_cl_step, DRIVE_MA);
//    }
//}

/* test_position_cl cac end */
//----------------------------------------------------------------------------------
/* test_position_cl usr start */

/********
 * @ 说明: MT6816 + TB67H450 闭环定位测试主函数。
 *         4 段 90° 步进，每段以编码器为目标闭环收敛，
 *         验证实际到位误差
 ********/
void test_position_cl(void)
{
    unsigned char seg;
    unsigned char pass_flag = 1;
    unsigned short enc_prev;
    unsigned short enc_curr;
    short delta;
    short expected_delta;

    printf("--- Position Test CL Start ---\r\n");

    /* 初始化编码器 */
    ela_mt6816_usr_init();
    printf("Encoder init done\r\n");

    /* 启动 TIM2 PWM */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
    printf("PWM started (%dmA)\r\n", DRIVE_MA);

    /* 预定位到电角度 0 */
    ela_tb67h450_set_foc_current(0, DRIVE_MA);
    HAL_Delay(200);
    printf("Pre-positioned at electrical angle 0\r\n");

    /* 读取初始编码器位置（3 次取中值） */
    enc_prev = test_position_cl_read_median();
    printf("Initial encoder: %u (0x%04X)\r\n",
           enc_prev, enc_prev);

    /* 初始基准由段循环内统一建立（s_cl_seg_start） */

    /* 4 段 90° 闭环步进 */
    for (seg = 0; seg < 4; seg++)
    {
        unsigned long timeout = 0;

        s_cl_seg_start = enc_prev;      /* 段起始原始值，回绕差基准 */
        s_cl_target = SEG_ENC;          /* 目标位移量，无回绕 */
        s_cl_err_acc = 0;
        s_cl_inband_cnt = 0;
        s_cl_arrived = 0;
        s_cl_running = 1;

        test_position_cl_set_led(seg);
        printf("Seg %d: target +%d (enc %u) ... ",
               seg, SEG_ENC, enc_prev);

        HAL_TIM_Base_Start_IT(&htim4);

        while (!s_cl_arrived && timeout < TIMEOUT_MS)
        {
            HAL_Delay(1);
            timeout++;
        }

        if (!s_cl_arrived)
        {
            HAL_TIM_Base_Stop_IT(&htim4);
            s_cl_running = 0;
            printf("TIMEOUT raw=%d step=%d "
                   "last_enc=%d last_err=%d ctrl=%d\r\n",
                   s_cl_last_enc, s_cl_step,
                   s_cl_last_enc, s_cl_last_err,
                   s_cl_ctrl_cnt);
            ela_tb67h450_brake();
            pass_flag = 0;
            break;
        }

        /* 到位后保持闭环在带内，稳定后再测量（避免开环悬停漂移） */
        HAL_Delay(SETTLE_DELAY);
        enc_curr = test_position_cl_read_median();
        {
            unsigned char k;
            unsigned int acc = 0;
            for (k = 0; k < 10; k++)
            {
                acc += test_position_cl_read_median();
                printf("settle[%d]=%d acc=%d\r\n",
                       k, (int)test_position_cl_angle_delta(
                           test_position_cl_read_median(),
                           (unsigned short)s_cl_seg_start),
                       s_cl_err_acc);
                HAL_Delay(30);
            }
            enc_curr = (unsigned short)((acc + 5) / 10);
        }
        HAL_TIM_Base_Stop_IT(&htim4);
        s_cl_running = 0;
        ela_tb67h450_brake();
        delta = test_position_cl_angle_delta(enc_curr, enc_prev);
        expected_delta = (short)SEG_ENC;

        printf("enc=%u delta=%d isr_last=%d drft=%d (exp=%d) step=%d %lums %s\r\n",
               enc_curr, delta,
               SEG_ENC - s_cl_last_err,       /* ISR 到位时看到的位移 */
               (SEG_ENC - s_cl_last_err) - delta, /* 到位后漂移量 */
               expected_delta, s_cl_step,
               timeout,
               (delta < expected_delta - POS_TOL ||
                delta > expected_delta + POS_TOL) ? "ERR" : "OK");

        if (delta < expected_delta - POS_TOL ||
            delta > expected_delta + POS_TOL)
        {
            pass_flag = 0;
        }

        enc_prev = enc_curr;
    }

    /* 回到起点对比 */
    test_position_cl_set_led(4);
    printf("--- Position Test CL %s ---\r\n",
           pass_flag ? "PASS" : "FAIL");

    while (1)
    {
    }
}

/********
 * @ 说明: 噪声底诊断。两相：
 *         A) 电机不上电，静止读 100 次原始编码器；
 *         B) 预定位上电保持，静止读 100 次。
 *         输出 min/max/极差，判断读数噪声 vs 机械偏差
 ********/
void test_position_noise(void)
{
    unsigned char i;
    unsigned int min_v, max_v;
    unsigned int v;

    printf("--- Encoder Noise Floor Test ---\r\n");
    ela_mt6816_usr_init();

    /* 相 A：不上电，静止读数 */
    min_v = 0xFFFF;
    max_v = 0;
    for (i = 0; i < 100; i++)
    {
        ela_mt6816_usr_read_angle();
        v = g_mt6816_st.raw_angle;
        if (v < min_v) min_v = v;
        if (v > max_v) max_v = v;
        HAL_Delay(2);
    }
    printf("A off: min=%u max=%u span=%d\r\n",
           min_v, max_v, (int)(max_v - min_v));

    /* 相 B：预定位上电保持，静止读数 */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
    ela_tb67h450_set_foc_current(0, DRIVE_MA);
    HAL_Delay(200);

    min_v = 0xFFFF;
    max_v = 0;
    for (i = 0; i < 100; i++)
    {
        ela_mt6816_usr_read_angle();
        v = g_mt6816_st.raw_angle;
        if (v < min_v) min_v = v;
        if (v > max_v) max_v = v;
        if ((i % 20) == 0) printf("B[%d]=%u\r\n", i, v);
        HAL_Delay(2);
    }
    printf("B hold: min=%u max=%u span=%d\r\n",
           min_v, max_v, (int)(max_v - min_v));

    ela_tb67h450_brake();
    printf("--- Noise Test Done ---\r\n");

    while (1)
    {
    }
}

/* test_position_cl usr end */
