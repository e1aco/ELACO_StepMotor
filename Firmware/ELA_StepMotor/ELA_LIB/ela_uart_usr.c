/********
 * @ 文件: ela_uart_usr.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-27
 * @ 版本: 1.0.1
 * @ 说明: USART3 DMA 收发模块应用层接口
 ********/

#include "ela_uart_usr.h"
#include "ela_uart_drv.h"
#include "usart.h"
#include <stdio.h>

static uint16_t s_rx_buf_pos;

UART_DMA_RX_T g_uart3_dma_rx_st;
UART_DMA_TX_T g_uart3_dma_tx_st;

/* ela_uart usr start */

/********
 * @ 说明: USART3 DMA 接收空闲中断处理，将 DMA 缓冲区数据搬入 RX 队列
 * @ 注意: 由 USART3_IRQHandler 在 HAL_UART_IRQHandler 之前调用。
 *        必须在 HAL 读 DR 之前检查 IDLE，否则 HAL 会误清 IDLE 标志
 ********/
void ela_uart3_dma_rx_idle_handler(void)
{
    uint16_t ndtr;
    uint16_t wr_ptr;
    uint16_t len;

    if (0 == __HAL_UART_GET_FLAG(&huart3, UART_FLAG_IDLE))
    {
        return;
    }

    __HAL_UART_CLEAR_IDLEFLAG(&huart3);

    ndtr   = __HAL_DMA_GET_COUNTER(huart3.hdmarx);
    wr_ptr = UART_DMA_RX_BUF_SIZE - ndtr;

    if (wr_ptr > s_rx_buf_pos)
    {
        len = wr_ptr - s_rx_buf_pos;
        ela_uart_queue_put_buf(&g_uart3_dma_rx_st.queue,
                               &g_uart3_dma_rx_st.dma_buf[s_rx_buf_pos],
                               len);
    }
    else if (wr_ptr < s_rx_buf_pos)
    {
        len = UART_DMA_RX_BUF_SIZE - s_rx_buf_pos;
        ela_uart_queue_put_buf(&g_uart3_dma_rx_st.queue,
                               &g_uart3_dma_rx_st.dma_buf[s_rx_buf_pos],
                               len);
        if (wr_ptr > 0)
        {
            ela_uart_queue_put_buf(&g_uart3_dma_rx_st.queue,
                                   g_uart3_dma_rx_st.dma_buf,
                                   wr_ptr);
        }
    }

    s_rx_buf_pos = wr_ptr;
}

/********
 * @ 说明: 检查 TX 队列，有数据则启动 DMA 发送
 * @ 注意: 在 HAL_UART_TxCpltCallback 和各 Send 函数末尾调用，
 *        形成链式自动发送
 ********/
void ela_uart3_dma_tx_continue(void)
{
    uint16_t len;

    if (g_uart3_dma_tx_st.dma_busy)
    {
        return;
    }

    len = ela_uart_queue_count(&g_uart3_dma_tx_st.queue);
    if (0 == len)
    {
        return;
    }

    if (len > UART_DMA_TX_BUF_SIZE)
    {
        len = UART_DMA_TX_BUF_SIZE;
    }

    len = ela_uart_queue_get_buf(&g_uart3_dma_tx_st.queue,
                                 g_uart3_dma_tx_st.dma_buf, len);
    if (len > 0)
    {
        g_uart3_dma_tx_st.dma_busy = true;
        HAL_UART_Transmit_DMA(&huart3, g_uart3_dma_tx_st.dma_buf, len);
    }
}

/********
 * @ 说明: 初始化 USART3 DMA 收发，启动循环接收
 * @ 注意: 调用后 DMA 自动填充 RX 缓冲区，IDLE 中断将数据搬入 RX 队列
 ********/
void ela_uart3_dma_init(void)
{
    ela_uart_queue_init(&g_uart3_dma_rx_st.queue);
    s_rx_buf_pos = 0;

    ela_uart_queue_init(&g_uart3_dma_tx_st.queue);
    g_uart3_dma_tx_st.dma_busy = false;

    HAL_UART_Receive_DMA(&huart3, g_uart3_dma_rx_st.dma_buf,
                         UART_DMA_RX_BUF_SIZE);
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);
}

/********
 * @ 输入: data: 源数据; len: 发送长度
 * @ 输出: 实际入队字节数
 * @ 说明: 非阻塞发送，数据先入 TX 队列，由 DMA 自动搬送
 ********/
uint16_t ela_uart3_dma_send_buf(uint8_t *data, uint16_t len)
{
    uint16_t written;

    written = ela_uart_queue_put_buf(&g_uart3_dma_tx_st.queue, data, len);
    ela_uart3_dma_tx_continue();

    return written;
}

/********
 * @ 输入: byte: 发送字节
 * @ 输出: true 成功, false 队列满
 * @ 说明: 非阻塞发送单个字节
 ********/
bool ela_uart3_dma_send_byte(uint8_t byte)
{
    if (!ela_uart_queue_put(&g_uart3_dma_tx_st.queue, byte))
    {
        return false;
    }
    ela_uart3_dma_tx_continue();
    return true;
}

/********
 * @ 输入: data: 目标缓冲区; len: 读取长度
 * @ 输出: 实际读出字节数
 * @ 说明: 从 RX 队列读取数据。无数据时返回 0
 ********/
uint16_t ela_uart3_dma_recv_buf(uint8_t *data, uint16_t len)
{
    return ela_uart_queue_get_buf(&g_uart3_dma_rx_st.queue, data, len);
}

/********
 * @ 输入: byte: 接收字节指针
 * @ 输出: true 成功, false 队列空
 * @ 说明: 从 RX 队列读取单个字节
 ********/
bool ela_uart3_dma_recv_byte(uint8_t *byte)
{
    return ela_uart_queue_get(&g_uart3_dma_rx_st.queue, byte);
}

/********
 * @ 输出: RX 队列中可读字节数
 * @ 说明: 查询当前已接收但未被应用层读取的数据长度
 ********/
uint16_t ela_uart3_dma_available(void)
{
    return ela_uart_queue_count(&g_uart3_dma_rx_st.queue);
}

/********
 * @ 说明: 初始化 printf 重定向，禁用 stdout 行缓存
 * @ 注意: 调用后 printf 输出将通过 fputc / __io_putchar 发送到 USART3
 ********/
void ela_uart_printf_init(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
}

/* ela_uart usr end */

