/********
 * @ 文件: ela_uart.h
 * @ 作者: ELACO
 * @ 日期: 2026-07-17
 * @ 版本: 1.0.0
 * @ 说明: USART3 DMA 收发模块，提供 printf 重定向和非阻塞收发接口
 ********/

#ifndef ELA_UART_H
#define ELA_UART_H

#include <stdbool.h>
#include <stdint.h>

#include "ela_uart_queue.h"
#include "ela_uart_drv.h"
#include "usart.h"

extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_usart3_tx;

#define UART_DMA_RX_BUF_SIZE 256
#define UART_DMA_TX_BUF_SIZE 256

/********
 * @ 说明: USART3 DMA 发送结构体
 ********/
typedef struct UART_DMA_TX
{
    UART_QUEUE_T  queue;
    volatile bool dma_busy;
    uint8_t       dma_buf[UART_DMA_TX_BUF_SIZE];
} UART_DMA_TX_T;

/********
 * @ 说明: USART3 DMA 接收结构体
 ********/
typedef struct UART_DMA_RX
{
    UART_QUEUE_T queue;
    uint8_t      dma_buf[UART_DMA_RX_BUF_SIZE];
} UART_DMA_RX_T;

extern UART_DMA_RX_T g_uart3_dma_rx_st;
extern UART_DMA_TX_T g_uart3_dma_tx_st;

void     ela_uart3_dma_init(void);
uint16_t ela_uart3_dma_send_buf(uint8_t *data, uint16_t len);
bool     ela_uart3_dma_send_byte(uint8_t byte);
uint16_t ela_uart3_dma_recv_buf(uint8_t *data, uint16_t len);
bool     ela_uart3_dma_recv_byte(uint8_t *byte);
uint16_t ela_uart3_dma_available(void);
void     ela_uart3_dma_rx_idle_handler(void);
void     ela_uart3_dma_tx_continue(void);
void     ela_uart_printf_init(void);

#endif

