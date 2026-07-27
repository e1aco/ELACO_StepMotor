/****
 * @ 文件: elaco_main.c
 * @ 作者: ELACO
 * @ 日期: {{DATE}}
 * @ 版本: 1.0.0
 * @ 说明: 应用主入口，初始化和 HAL 回调集中定义
 ********/

#include "elaco_main.h"

/* 用户模块头文件将自动添加在此处 */

/* elaco_main usr start */

/****
 * @ 说明: 主循环函数
 ********/
void elaco_main(void)
{
    /* 初始化代码将自动添加在此处 */

    /* === 弱化测试入口（按需取消注释，测试完请删除 === */
    /* ela_xxx_test();  ← 自带 while 循环，调用后不返回 */

    while (1)
    {

    }
}

/* elaco_main usr end */
//----------------------------------------------------------------------------------
/* elaco_main cac start */

/****
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

/****
 * @ 说明: USART3 DMA 循环接收满回调
 ********/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (USART3 == huart->Instance)
    {
        /* 循环 DMA 每 256 字节触发一次，数据已在 IDLE 时处理 */
    }
}

/* elaco_main cac end */