/*****************************************************************************
 * @文件: ela_uart_usr.h
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: USART3 DMA 收发模块，提供 printf 重定向和非阻塞收发接口
 ****************************************************************************/

#ifndef ELA_UART_USR_H
#define ELA_UART_USR_H

#include <stdbool.h>
#include <stdint.h>

#include "ela_uart_queue.h"
#include "ela_uart_drv.h"
#include "usart.h"

/* ==== 全局实例 ==== */
extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_usart3_tx;

/* ==== 常量定义 ==== */
#define UART_DMA_RX_BUF_SIZE 256
#define UART_DMA_TX_BUF_SIZE 256

/* ==== 类型定义 ==== */
/********
 * @说明: USART3 DMA 发送结构体
 ********/
typedef struct UART_DMA_TX
{
    UART_QUEUE_T  queue;
    volatile bool dma_busy;
    uint8_t       dma_buf[UART_DMA_TX_BUF_SIZE];
} UART_DMA_TX_T;

/********
 * @说明: USART3 DMA 接收结构体
 ********/
typedef struct UART_DMA_RX
{
    UART_QUEUE_T queue;
    uint8_t      dma_buf[UART_DMA_RX_BUF_SIZE];
} UART_DMA_RX_T;

extern UART_DMA_RX_T g_uart3_dma_rx_st;
extern UART_DMA_TX_T g_uart3_dma_tx_st;

/* ==== 接口 ==== */




void     USR_UART3_DmaInit(void);
uint16_t USR_UART3_DmaSendBuf(uint8_t *data, uint16_t len);
bool     USR_UART3_DmaSendByte(uint8_t byte);
uint16_t USR_UART3_DmaRecvBuf(uint8_t *data, uint16_t len);
bool     USR_UART3_DmaRecvByte(uint8_t *byte);
uint16_t USR_UART3_DmaAvailable(void);
void     USR_UART3_DmaRxIdleHandler(void);
void     USR_UART3_DmaTxContinue(void);
void     USR_UART_PrintfInit(void);

#endif






