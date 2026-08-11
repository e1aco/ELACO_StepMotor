/********
 * @ 文件: elaco_main.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-17
 * @ 版本: 1.0.0
 * @ 说明: 应用主入口，初始化和 HAL 回调集中定义
 ********/

#include "elaco_main.h"
#include "ela_uart_usr.h"
#include "ela_pow_det_usr.h"
#include "ela_button_usr.h"
#include "ela_stockfile_usr.h"
#include "elaco_calibration_usr.h"
#include "ela_motion_run_usr.h"
#include "ela_mt6816_usr.h"
#include "ela_tb67h450_usr.h"
#include "ela_cyclecal.h"
#include "mb.h"
#include "tim.h"

/* elaco_main usr start */

/* LED 点亮电平：默认低电平点亮，硬件验证时按实际极性调整 */
#define LED_ON_LEVEL  GPIO_PIN_RESET

/* 自动校准模式（调试用，验证后移除）：上电延时 AUTO_CALI_DELAY_MS 后
 * 自动触发校准，校准完成（calitable_flag=true 且 IDLE）后自动进 demo。
 * 替代手动双键长按（硬件按键不便操作） */
#define AUTO_CALI_MODE          1
#define AUTO_CALI_DELAY_MS      5000

/* 停驻诊断模式（调试用，验证后移除）：磁场从小步连续逼近固定微步，
 * 每步停驻读编码器，验证 set_foc_current 的微步↔编码器真实映射与斜率方向 */
#define DIAG_PARK_MODE          0
#define DIAG_PARK_NUM           20
#define DIAG_PARK_HOLD_MS       400
#define DIAG_PARK_MA            1700
#define DIAG_PARK_STEP          256   /* 每步微步增量（小步避免丢步） */
#define DIAG_PARK_SAMPLE_MS     100

/* 编码器只读诊断（调试用）：磁场休眠，主循环持续打印编码器读数。
 * 用户手动转动电机轴，验证编码器读数连续/可重复（排查非确定性） */
#define DIAG_ENC_RO_MODE        0
#define DIAG_ENC_RO_MS          50

/* park 诊断结果数组（全局 volatile，供 pyocd --elf 定位读取，防优化） */
#if DIAG_PARK_MODE
volatile unsigned int g_diag_park_enc[DIAG_PARK_NUM] = {0};
#endif
/* 开环转圈诊断模式（调试用，验证后移除）：磁场以固定速率连续转动
 * 多圈，观察编码器是否跟随磁场同步转动。验证驱动输出与机械耦合 */
#define DIAG_SPIN_MODE          0
#define DIAG_SPIN_SPEED         20     /* 微步/5ms，4000 微步/s 慢速（避免丢步） */
#define DIAG_SPIN_MA            1700
#define DIAG_SPIN_TICK_MS       5
#define DIAG_SPIN_PRINT_MS      200
#define DIAG_SPIN_TURNS         1      /* 转动圈数 */
#define DIAG_SPIN_START         51200  /* 起点微步：51200 复刻校准 case 2 */

/* 精确步进诊断（调试用）：每次步进 1 全步（256 微步）停住读编码器，
 * 连续测一整圈，验证电机全步数与微步↔电角换算关系 */
#define DIAG_STEP_MODE          0
#define DIAG_STEP_MA            1500
#define DIAG_STEP_HOLD_MS       500
#define DIAG_STEP_PRINT_MS      100

/* 编码器监控诊断（调试用）：磁场停驻，高频读编码器，
 * 统计偶发跳变模式（确认坏读频率/持续 tick） */
#define DIAG_ENC_MON_MODE       0
#define DIAG_ENC_MON_MA         1700
#define DIAG_ENC_MON_HOLD_MS    10000

/* 测试：KEY1 单击轮切目标角度索引（0/90/180/270°） */
#if !(DIAG_PARK_MODE || DIAG_SPIN_MODE || DIAG_STEP_MODE || DIAG_ENC_MON_MODE || DIAG_ENC_RO_MODE)
#define KEY_TST_TARGETS  MOTION_RUN_TARGET_NUM
static unsigned char s_key_tst_idx = 0;
static uint32_t s_tick_last = 0;
static const unsigned short s_key_tst_targets[KEY_TST_TARGETS] = {
    MOTION_RUN_TGT_0DEG, MOTION_RUN_TGT_90DEG,
    MOTION_RUN_TGT_180DEG, MOTION_RUN_TGT_270DEG
};

/********
 * @ 说明: KEY1 单击轮切目标角度。仅在上电回零完成（闭环空闲）
 *         时响应，避免打断进行中的运动。到达后打印闭环数据
 ********/
static void elaco_main_key_angle_test(void)
{
    if (!ela_button_get_click(1))
    {
        return;
    }

    if (!ela_motion_run_is_idle())
    {
        printf("[KEY] busy, skip\r\n");
        return;
    }

    s_key_tst_idx++;
    if (s_key_tst_idx >= KEY_TST_TARGETS)
    {
        s_key_tst_idx = 0;
    }

    ela_motion_run_goto_target(s_key_tst_targets[s_key_tst_idx]);
    ela_motion_run_debug_print();
    printf("[KEY] goto %ddeg\r\n",
           (int)(s_key_tst_targets[s_key_tst_idx] / (ENC_RESOLUTION / 360)));
}

/********
 * @ 说明: 一次性 dump 校准表，定位 0°/270° 保持抖动区段的
 *         微步斜率异常。校准表 g_cali_table[enc] 存该编码器
 *         对应的绝对微步（51200 一圈），硬件约定编码器随微步
 *         递减，故表斜率 ≈ -3.125 微步/计数，异常区段斜率
 *         会明显偏离（过大/过小/非单调）
 ********/
static void elaco_main_dump_cali_table(void)
{
    int i, d;

    printf("[TBL] coarse scan (every 128), slope = val[+128]-val[]/128\r\n");
    for (i = 0; i < ENC_RESOLUTION; i += 128)
    {
        d = (int)cyclecal_diff(g_cali_table[(i + 128) % ENC_RESOLUTION],
                               g_cali_table[i], MICROSTEPLAP);
        printf("[TBL] enc=%4d val=%5u slope=%d.%02d\r\n",
               i, g_cali_table[i],
               (d / 128) * 4, abs(d % 128) * 100 / 128);
    }

    printf("[TBL] fine scan 0deg: enc 16320..16384/0..64\r\n");
    for (i = 16320; i < ENC_RESOLUTION; i++)
    {
        printf("[TBL] enc=%4d val=%5u\r\n", i, g_cali_table[i]);
    }
    for (i = 0; i < 64; i++)
    {
        printf("[TBL] enc=%4d val=%5u\r\n", i, g_cali_table[i]);
    }

    printf("[TBL] fine scan 270deg: enc 12224..12352\r\n");
    for (i = 12224; i < 12352; i++)
    {
        printf("[TBL] enc=%4d val=%5u\r\n", i, g_cali_table[i]);
    }

    printf("[TBL] fine scan 90deg: enc 4032..4160\r\n");
    for (i = 4032; i < 4160; i++)
    {
        printf("[TBL] enc=%4d val=%5u\r\n", i, g_cali_table[i]);
    }

    printf("[TBL] fine scan 180deg: enc 8128..8256\r\n");
    for (i = 8128; i < 8256; i++)
    {
        printf("[TBL] enc=%4d val=%5u\r\n", i, g_cali_table[i]);
    }
}
#endif

/********
 * @ 说明: 主循环函数
 ********/
void elaco_main(void)
{
    ela_uart_printf_init();
    ela_uart3_dma_init();
    ela_pow_det_init();
    ela_button_init();
    ela_stockfile_init();
    elaco_calibration_init();
    elaco_calibration_table_data_valid();

    /* 无校准表 → LED1 常亮提示需要校准 */
    if (false == g_calibra_st.calitable_flag)
    {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin,
                          LED_ON_LEVEL);
    }

    /* 正式模式：有校准表 → 上电闭环回零；无表 → LED 提示待校准 */
    ela_motion_run_init();

#if DIAG_PARK_MODE || DIAG_SPIN_MODE || DIAG_STEP_MODE || DIAG_ENC_MON_MODE || DIAG_ENC_RO_MODE
    /* 诊断模式：启动驱动 PWM，不进 demo。磁场由诊断主循环驱动 */
    ela_mt6816_usr_init();
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
#if DIAG_ENC_RO_MODE
    printf("[DIAG] encoder read-only, field sleep\r\n");
#elif DIAG_ENC_MON_MODE
    printf("[DIAG] enc monitor diagnosis\r\n");
#elif DIAG_STEP_MODE
    printf("[DIAG] step diagnosis: 256 microstep/fullstep, 1 turn\r\n");
#elif DIAG_SPIN_MODE
    printf("[DIAG] spin diagnosis, speed=%d ms/tick\r\n", DIAG_SPIN_TICK_MS);
#else
    printf("[DIAG] park diagnosis, %d positions x %d ms\r\n",
           DIAG_PARK_NUM, DIAG_PARK_HOLD_MS);
#endif
#elif AUTO_CALI_MODE
    /* 自动校准（调试用）：上电延时后自动触发校准，校准完成自动进 demo */
    printf("[AUTO-CALI] power-on, wait %d ms\r\n", AUTO_CALI_DELAY_MS);
    HAL_Delay(AUTO_CALI_DELAY_MS);
    printf("[AUTO-CALI] trigger calibration\r\n");
    elaco_calibration_start();
#else
    if (g_calibra_st.calitable_flag)
    {
        /* 启动电机驱动 PWM 与控制中断（TIM4 20kHz）。
         * 此前 PWM/中断仅在校准启动时开启，正式模式须显式启动 */
        ela_mt6816_usr_init();
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
        HAL_TIM_Base_Start_IT(&htim4);

        ela_motion_run_demo_start();
        printf("[MAIN] ctrl init, demo start (0/90/180/270 roundtrip)\r\n");

        /* 诊断：一次性 dump 校准表关键区段（验证后移除） */
        elaco_main_dump_cali_table();
    }
#endif

    printf("[MAIN] enter loop tick=%lu\r\n", (unsigned long)HAL_GetTick());

    while (1)
    {
#if DIAG_ENC_RO_MODE
        /* 编码器只读诊断：磁场休眠，每 DIAG_ENC_RO_MS 读编码器打印。
         * 用户手动转动电机轴，验证读数连续/可重复 */
        static unsigned long s_ro_last = 0;
        static unsigned char s_ro_first = 1;

        if (s_ro_first)
        {
            s_ro_first = 0;
            ela_tb67h450_sleep();
            printf("[ENC] read-only mode, turn shaft manually\r\n");
        }

        if ((HAL_GetTick() - s_ro_last) >= DIAG_ENC_RO_MS)
        {
            s_ro_last = HAL_GetTick();
            ela_mt6816_usr_read_angle();
            printf("[ENC] enc=%u valid=%d raw=%04X\r\n",
                   g_mt6816_st.raw_angle, g_mt6816_st.data_valid,
                   g_mt6816_st.raw_data);
        }
        continue;
#elif DIAG_ENC_MON_MODE
        /* 编码器监控：磁场停驻固定微步，高频读编码器，统计偶发跳变 */
        static unsigned short s_mon_last = 0xFFFF;
        static unsigned char s_mon_first = 1;
        static unsigned long s_mon_start = 0;
        static unsigned int s_mon_bad = 0;
        static unsigned int s_mon_total = 0;
        static unsigned int s_mon_badseq = 0;

        if (s_mon_first)
        {
            s_mon_first = 0;
            s_mon_start = HAL_GetTick();
            ela_tb67h450_set_foc_current(0, DIAG_ENC_MON_MA);
            printf("[DIAG] enc monitor start, field=0\r\n");
        }

        ela_mt6816_usr_read_angle();
        unsigned short cur = g_mt6816_st.raw_angle;
        if (0xFFFF != s_mon_last)
        {
            int d = (int)cur - (int)s_mon_last;
            if (d > 8192) d -= 16384;
            else if (d < -8192) d += 16384;
            if (d > 256 || d < -256)
            {
                s_mon_bad++;
                s_mon_badseq++;
                printf("[DIAG] BAD enc=%u last=%u jump=%d seq=%u\r\n",
                       cur, s_mon_last, d, s_mon_badseq);
            }
            else
            {
                s_mon_badseq = 0;
            }
        }
        s_mon_last = cur;
        s_mon_total++;

        /* 每 5 秒报告一次基线 */
        if ((HAL_GetTick() - s_mon_start) >= DIAG_ENC_MON_HOLD_MS)
        {
            printf("[DIAG] monitor done: total=%u bad=%u (%.3f%%)\r\n",
                   s_mon_total, s_mon_bad,
                   100.0 * s_mon_bad / (s_mon_total ? s_mon_total : 1));
            ela_tb67h450_sleep();
            HAL_Delay(0xFFFFFFFFUL);
        }
        continue;
#elif DIAG_STEP_MODE
        /* 精确步进诊断：每 DIAG_STEP_HOLD_MS 步进 1 全步(256 微步)，
         * 停住读编码器，测一整圈验证传动关系 */
        static unsigned int s_step_pos = 0;
        static unsigned long s_step_last = 0;
        static unsigned long s_step_last_print = 0;
        static unsigned char s_step_done = 0;
        static unsigned char s_step_first = 1;

        if (s_step_first)
        {
            s_step_first = 0;
            s_step_last = HAL_GetTick();
            ela_tb67h450_set_foc_current(s_step_pos, DIAG_STEP_MA);
        }

        /* 每 DIAG_STEP_PRINT_MS 打印当前编码器（停驻观测） */
        if ((HAL_GetTick() - s_step_last) < DIAG_STEP_HOLD_MS)
        {
            if ((HAL_GetTick() - s_step_last_print) >= DIAG_STEP_PRINT_MS)
            {
                s_step_last_print = HAL_GetTick();
                ela_mt6816_usr_read_angle();
                printf("[DIAG] field=%u enc=%u valid=%d\r\n",
                       s_step_pos, g_mt6816_st.raw_angle,
                       g_mt6816_st.data_valid);
            }
        }
        else if (!s_step_done)
        {
            /* 停驻满 → 步进 1 全步（256 微步） */
            s_step_pos += 256;
            if (s_step_pos >= MICROSTEPLAP)
            {
                s_step_pos -= MICROSTEPLAP;
            }
            s_step_last = HAL_GetTick();
            s_step_last_print = 0;
            ela_tb67h450_set_foc_current(s_step_pos, DIAG_STEP_MA);
            ela_mt6816_usr_read_angle();
            printf("[DIAG] STEP field=%u enc=%u valid=%d\r\n",
                   s_step_pos, g_mt6816_st.raw_angle,
                   g_mt6816_st.data_valid);

            /* 走满一整圈 → 停止 */
            if (0 == s_step_pos)
            {
                s_step_done = 1;
                printf("[DIAG] full turn done, hold\r\n");
                ela_tb67h450_sleep();
            }
        }
        continue;
#elif DIAG_SPIN_MODE
        /* 开环转圈诊断：磁场以固定速率连续转动多圈，观察编码器是否跟随。
         * 起点 DIAG_SPIN_START=51200 复刻校准 case 2 的磁场基准 */
        static unsigned int s_spin_pos = DIAG_SPIN_START;
        static unsigned long s_spin_last_tick = 0;
        static unsigned long s_spin_last_print = 0;
        static unsigned long s_spin_total = 0;
        static unsigned char s_spin_done = 0;
        static unsigned char s_spin_first = 1;
        static unsigned short s_spin_last_enc = 0xFFFF;
        static unsigned int s_spin_bad = 0;

        if (s_spin_first)
        {
            s_spin_first = 0;
            ela_tb67h450_set_foc_current(s_spin_pos, DIAG_SPIN_MA);
            ela_mt6816_usr_read_angle();
            printf("[DIAG] spin start field=%u enc=%u\r\n",
                   s_spin_pos, g_mt6816_st.raw_angle);
        }

        if (!s_spin_done)
        {
            if ((HAL_GetTick() - s_spin_last_tick) >= DIAG_SPIN_TICK_MS)
            {
                s_spin_last_tick = HAL_GetTick();
                s_spin_pos += DIAG_SPIN_SPEED;
                if (s_spin_pos >= MICROSTEPLAP)
                {
                    s_spin_pos -= MICROSTEPLAP;
                }
                ela_tb67h450_set_foc_current(s_spin_pos, DIAG_SPIN_MA);
                s_spin_total += DIAG_SPIN_SPEED;

                /* 每 tick 读编码器，监控偶发跳变（慢速下相邻 5ms 位移应 <20 计数） */
                ela_mt6816_usr_read_angle();
                unsigned short enc_now = g_mt6816_st.raw_angle;
                if (0xFFFF != s_spin_last_enc)
                {
                    int d = (int)enc_now - (int)s_spin_last_enc;
                    if (d > 8192) d -= 16384;
                    else if (d < -8192) d += 16384;
                    if (d > 256 || d < -256)
                    {
                        s_spin_bad++;
                        printf("[DIAG] BAD field=%u enc=%u last=%u jump=%d totalbad=%u\r\n",
                               s_spin_pos, enc_now, s_spin_last_enc, d, s_spin_bad);
                    }
                }
                s_spin_last_enc = enc_now;
            }

            /* 每 DIAG_SPIN_PRINT_MS 打印磁场位置 + 编码器读数 */
            if ((HAL_GetTick() - s_spin_last_print) >= DIAG_SPIN_PRINT_MS)
            {
                s_spin_last_print = HAL_GetTick();
                ela_mt6816_usr_read_angle();
                printf("[DIAG] field=%u enc=%u valid=%d\r\n",
                       s_spin_pos, g_mt6816_st.raw_angle,
                       g_mt6816_st.data_valid);
            }

            /* 转满 DIAG_SPIN_TURNS 圈 → 停止 */
            if (s_spin_total >= (DIAG_SPIN_TURNS * MICROSTEPLAP))
            {
                s_spin_done = 1;
                printf("[DIAG] spin %u turns done, bad=%u, hold last field\r\n",
                       DIAG_SPIN_TURNS, s_spin_bad);
                ela_tb67h450_sleep();
            }
        }
        continue;
#elif DIAG_PARK_MODE
        /* 停驻诊断：磁场从 0 起每步 +DIAG_PARK_STEP(256 微步) 停驻读编码器，
         * 小步连续逼近避免丢步，验证微步↔编码器斜率方向 */
        static unsigned int s_park_pos = 0;
        static unsigned char s_park_idx = 0;
        static unsigned long s_park_start = 0;
        static unsigned char s_park_first = 1;
        static unsigned char s_park_sampled = 0;

        if (s_park_first)
        {
            s_park_first = 0;
            s_park_start = HAL_GetTick();
            ela_tb67h450_set_foc_current(s_park_pos, DIAG_PARK_MA);
            s_park_sampled = 0;
        }

        /* 停驻稳定后采样一次编码器（存全局数组，pyocd 读取） */
        if (!s_park_sampled && (HAL_GetTick() - s_park_start) >= 200)
        {
            s_park_sampled = 1;
            ela_mt6816_usr_read_angle();
            g_diag_park_enc[s_park_idx] = g_mt6816_st.raw_angle;
        }

        /* 停驻满 → 小步进到下一位置 */
        if ((HAL_GetTick() - s_park_start) >= DIAG_PARK_HOLD_MS)
        {
            s_park_idx++;
            if (s_park_idx >= DIAG_PARK_NUM)
            {
                ela_tb67h450_sleep();
                HAL_Delay(0xFFFFFFFFUL);
            }
            s_park_pos += DIAG_PARK_STEP;
            if (s_park_pos >= MICROSTEPLAP) s_park_pos -= MICROSTEPLAP;
            s_park_start = HAL_GetTick();
            s_park_sampled = 0;
            ela_tb67h450_set_foc_current(s_park_pos, DIAG_PARK_MA);
        }
        continue;
#else
        ela_button_tick();
        elaco_calibration_table_generate_proc();

#if AUTO_CALI_MODE
        /* 校准完成（表有效且回到 IDLE）→ 启动 demo（仅一次） */
        static unsigned char s_auto_demo_started = 0;
        if (!s_auto_demo_started
            && g_calibra_st.calitable_flag
            && g_calibra_st.cali_step == CALI_STEP_IDLE)
        {
            s_auto_demo_started = 1;
            ela_mt6816_usr_init();
            HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
            HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
            HAL_TIM_Base_Start_IT(&htim4);
            ela_motion_run_demo_start();
            printf("[AUTO-CALI] cali done, demo start\r\n");

            /* 诊断：dump 新校准表关键区段，对比验证 */
            elaco_main_dump_cali_table();
        }
#endif

        /* 测试：周期打印闭环状态（诊断用，验证后移除） */
        if ((HAL_GetTick() - s_tick_last) >= 500)
        {
            s_tick_last = HAL_GetTick();
            ela_motion_run_debug_print();
        }

        /* 测试：KEY1 单击轮切 0/90/180/270° 观察闭环 */
        elaco_main_key_angle_test();

        /* 测试：演示往返调度（0/90/180/270 前进再返回），到位停 500ms */
        ela_motion_run_demo_poll();

        /* 双键同时长按 3s → 手动触发校准 */
        if (ela_button_get_both_long())
        {
            elaco_calibration_start();
        }

        ela_pow_det_tick();
#endif
    }
}

/* elaco_main usr end */
//----------------------------------------------------------------------------------
/* elaco_main cac start */

/********
 * @ 说明: USART3 DMA 发送完成回调，通知 TX 模块继续链式发送
 ********/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (USART3 == huart->Instance)
    {
        g_uart3_dma_tx_st.dma_busy = false;
        ela_uart3_dma_tx_continue();
    }
}

/********
 * @ 说明: USART3 DMA 循环接收满回调
 * @ 注意: 实际数据搬移由 IDLE 中断完成
 ********/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (USART3 == huart->Instance)
    {
        /* 循环 DMA 每 256 字节触发一次，数据已在 IDLE 时处理 */
    }
}

/********
 * @ 输入: htim: 触发中断的定时器句柄
 * @ 说明: TIM4 20kHz 周期中断回调。校准未运行时跑闭环控制，
 *         校准（开环跑圈）进行时暂停闭环，互斥分派
 ********/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (&htim4 == htim)
    {
        if (g_calibra_st.cali_step == CALI_STEP_IDLE)
        {
            ela_motion_run_proc();
        }
        else
        {
            elaco_calibration_proc();
        }
    }
}

/* elaco_main cac end */

