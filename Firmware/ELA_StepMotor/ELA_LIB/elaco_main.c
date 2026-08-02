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
#include "mb.h"
#include "tim.h"

#ifdef ModTest
    #include "test_mt6816.h"
    // #include "test_tb67h450.h"  /* 启用 test_position 时禁用 */
    // #include "test_position.h"
    // #include "test_position_cl.h"
    // #include "test_pid.h"
    // #include "test_openloop.h"
#endif

/* elaco_main usr start */

/* LED 点亮电平：默认低电平点亮，硬件验证时按实际极性调整 */
#define LED_ON_LEVEL  GPIO_PIN_RESET

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

#ifdef ModTest
    // test_mt6816();
    // test_tb67h450();
    // test_position();
    //test_position_cl();
    // test_position_noise();
    // test_pid();
    // test_openloop();
    /* 校准表采集验证：上电自动触发开环采集一圈正+一圈反 */
    elaco_calibration_start();
#endif

    while (1)
    {
        ela_button_tick();
        elaco_calibration_table_generate_proc();

        /* 双键同时长按 3s → 手动触发校准 */
        if (ela_button_get_both_long())
        {
            elaco_calibration_start();
        }

        ela_pow_det_tick();
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
 * @ 说明: TIM4 20kHz 周期中断回调，驱动校准数据采集
 * @ 注意: 唯一实现（ModTest 下 test_openloop 已禁用，
 *         elaco_main 统一提供此回调）
 ********/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (&htim4 == htim)
    {
        elaco_calibration_proc();
    }
}

/* elaco_main cac end */

