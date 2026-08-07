/*****************************************************************************
 * @文件: elaco_main.c
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: 应用主入口，初始化和 HAL 回调集中定义
 ****************************************************************************/

#include "elaco_main.h"
#include "ela_uart_usr.h"
#include "ela_pow_det_usr.h"
#include "ela_button_usr.h"
#include "ela_stockfile_usr.h"
#include "elaco_calibration_usr.h"
#include "ela_motion_run_usr.h"
#include "ela_mt6816_usr.h"
#include "ela_cyclecal.h"
#include "tim.h"

/* LED 点亮电平：默认低电平点亮，硬件验证时按实际极性调整 */
#define LED_ON_LEVEL  GPIO_PIN_RESET

/* CL-TODO-RISE: 测试段（原 elaco_main_dump_cali_table / KEY_TST 轮切）已 #if 0 禁用。
 * 非正式功能，不参与编译。若复用于后续调试，去掉 #if 0 即可；
 * 正式发布前建议彻底删除（git 历史可找回）。 */
#if 0
/* 测试：KEY1 单击轮切目标角度索引（0/90/180/270°） */
#define KEY_TST_TARGETS  MOTION_RUN_TARGET_NUM
static unsigned char s_key_tst_idx = 0;
static uint32_t s_tick_last = 0;
static const unsigned short s_key_tst_targets[KEY_TST_TARGETS] = {
    MOTION_RUN_TGT_0DEG, MOTION_RUN_TGT_90DEG,
    MOTION_RUN_TGT_180DEG, MOTION_RUN_TGT_270DEG
};

/********
 * @说明: KEY1 单击轮切目标角度。仅在上电回零完成（闭环空闲）
 *         时响应，避免打断进行中的运动。到达后打印闭环数据
 ********/
static void s_key_angle_test(void)

{
    if (!USR_Button_GetClick(1))
    {
        return;
    }

    if (!USR_MotionRun_IsIdle())
    {
        printf("[KEY] busy, skip\r\n");
        return;
    }

    s_key_tst_idx++;
    if (s_key_tst_idx >= KEY_TST_TARGETS)
    {
        s_key_tst_idx = 0;
    }

    USR_MotionRun_GotoTarget(s_key_tst_targets[s_key_tst_idx]);
    USR_MotionRun_DebugPrint();
    printf("[KEY] goto %ddeg\r\n",
           (int)(s_key_tst_targets[s_key_tst_idx] / (ENC_RESOLUTION / 360)));
}

/********
 * @说明: 一次性 dump 校准表，定位 0°/270° 保持抖动区段的
 *         微步斜率异常。校准表 g_cali_table[enc] 存该编码器
 *         对应的绝对微步（51200 一圈），硬件约定编码器随微步
 *         递减，故表斜率 ≈ -3.125 微步/计数，异常区段斜率
 *         会明显偏离（过大/过小/非单调）
 ********/
static void s_dump_cali_table(void)
{
    int i, d;

    printf("[TBL] coarse scan (every 128), slope = val[+128]-val[]/128\r\n");
    for (i = 0; i < ENC_RESOLUTION; i += 128)
    {
        d = (int)USR_CycleCal_Diff(g_cali_table[(i + 128) % ENC_RESOLUTION],
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

/* ==== 接口实现 ==== */
/********
 * @说明: 主循环函数
 ********/
void USR_Main_Init(void)
{
    USR_UART_PrintfInit();
    USR_UART3_DmaInit();
    USR_PowDet_Init();
    USR_Button_Init();
    USR_Stockfile_Init();
    USR_Calibration_Init();
    USR_Calibration_TableDataValid();

    /* 无校准表 → LED1 常亮提示需要校准 */
    if (false == g_calibra_st.calitable_flag)
    {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin,
                          LED_ON_LEVEL);
    }

    /* 正式模式：有校准表 → 上电闭环回零；无表 → LED 提示待校准 */
    USR_MotionRun_Init();
    if (g_calibra_st.calitable_flag)
    {
        /* 启动电机驱动 PWM 与控制中断（TIM4 20kHz）。
         * 此前 PWM/中断仅在校准启动时开启，正式模式须显式启动 */
        USR_MT6816_Init();
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
        HAL_TIM_Base_Start_IT(&htim4);

        USR_MotionRun_DemoStart();
        printf("[MAIN] ctrl init, demo start (0/90/180/270 roundtrip)\r\n");

        /* CL-TODO-RISE: 一次性 dump 校准表测试段，已 #if 0 禁用（非正式功能）。
         * 去掉 #if 0 即可复用于后续调试；正式发布前建议彻底删除（git 可找回） */
        #if 0
        /* 诊断：一次性 dump 校准表关键区段（验证后移除） */
        s_dump_cali_table();
        #endif
    }

    printf("[MAIN] enter loop tick=%lu\r\n", (unsigned long)HAL_GetTick());

    while (1)
    {
        USR_Button_Tick();
        USR_Calibration_TableGenerateProc();

#if 0
        /* 测试：周期打印闭环状态（诊断用，验证后移除） */
        if ((HAL_GetTick() - s_tick_last) >= 500)
        {
            s_tick_last = HAL_GetTick();
            USR_MotionRun_DebugPrint();
        }

        /* 测试：KEY1 单击轮切 0/90/180/270° 观察闭环 */
        s_key_angle_test();
#endif

        /* 测试：演示往返调度（0/90/180/270 前进再返回），到位停 500ms */
        USR_MotionRun_DemoPoll();

        /* 双键同时长按 3s → 手动触发校准 */
        if (USR_Button_GetBothLong())
        {
            USR_Calibration_Start();
        }

        USR_PowDet_Tick();
    }
}


/********
 * @说明: USART3 DMA 发送完成回调，通知 TX 模块继续链式发送
 ********/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (USART3 == huart->Instance)
    {
        g_uart3_dma_tx_st.dma_busy = false;
        USR_UART3_DmaTxContinue();
    }
}

/********
 * @说明: USART3 DMA 循环接收满回调
 * @注意: 实际数据搬移由 IDLE 中断完成
 ********/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (USART3 == huart->Instance)
    {
        /* 循环 DMA 每 256 字节触发一次，数据已在 IDLE 时处理 */
    }
}

/********
 * @输入: htim: 触发中断的定时器句柄
 * @说明: TIM4 20kHz 周期中断回调。校准未运行时跑闭环控制，
 *         校准（开环跑圈）进行时暂停闭环，互斥分派
 ********/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (&htim4 == htim)
    {
        if (g_calibra_st.cali_step == CALI_STEP_IDLE)
        {
            USR_MotionRun_Proc();
        }
        else
        {
            USR_Calibration_Proc();
        }
    }
}







