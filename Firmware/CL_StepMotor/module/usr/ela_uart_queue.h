/*****************************************************************************
 * @文件: ela_uart_queue.h
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: 字节级环形缓冲区，用于 UART RX/TX 流数据
 ****************************************************************************/

#ifndef ELA_UART_QUEUE_H
#define ELA_UART_QUEUE_H

#include <stdbool.h>

/* ==== 常量定义 ==== */
#define UART_QUEUE_SIZE 256

/* ==== 类型定义 ==== */
/********
 * @说明: UART 环形队列结构体
 ********/
typedef struct UART_QUEUE
{
    unsigned char buffer[UART_QUEUE_SIZE];
    unsigned short head;   /* 写入位置 */
    unsigned short tail;   /* 读取位置 */
    unsigned short count;  /* 当前字节数 */
} UART_QUEUE_T;

/* ==== 接口 ==== */



void           USR_UartQueue_Init(UART_QUEUE_T *me);
bool           USR_UartQueue_Put(UART_QUEUE_T *me, unsigned char byte);
bool           USR_UartQueue_Get(UART_QUEUE_T *me, unsigned char *byte);
unsigned short USR_UartQueue_PutBuf(UART_QUEUE_T *me,
                                      unsigned char *data,
                                      unsigned short len);
unsigned short USR_UartQueue_GetBuf(UART_QUEUE_T *me,
                                      unsigned char *data,
                                      unsigned short len);
bool           USR_UartQueue_IsFull(UART_QUEUE_T *me);
bool           USR_UartQueue_IsEmpty(UART_QUEUE_T *me);
unsigned short USR_UartQueue_Count(UART_QUEUE_T *me);

#endif






