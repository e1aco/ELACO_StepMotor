/********
 * @ 文件: ela_uart_queue.h
 * @ 作者: ELACO
 * @ 日期: 2026-07-17
 * @ 版本: 1.0.0
 * @ 说明: 字节级环形缓冲区，用于 UART RX/TX 流数据
 ********/

#ifndef ELA_UART_QUEUE_H
#define ELA_UART_QUEUE_H

#include <stdbool.h>

#define UART_QUEUE_SIZE 256

/********
 * @ 说明: UART 环形队列结构体
 ********/
typedef struct UART_QUEUE
{
    unsigned char buffer[UART_QUEUE_SIZE];
    unsigned short head;   /* 写入位置 */
    unsigned short tail;   /* 读取位置 */
    unsigned short count;  /* 当前字节数 */
} UART_QUEUE_T;

void           ela_uart_queue_init(UART_QUEUE_T *me);
bool           ela_uart_queue_put(UART_QUEUE_T *me, unsigned char byte);
bool           ela_uart_queue_get(UART_QUEUE_T *me, unsigned char *byte);
unsigned short ela_uart_queue_put_buf(UART_QUEUE_T *me,
                                      unsigned char *data,
                                      unsigned short len);
unsigned short ela_uart_queue_get_buf(UART_QUEUE_T *me,
                                      unsigned char *data,
                                      unsigned short len);
bool           ela_uart_queue_is_full(UART_QUEUE_T *me);
bool           ela_uart_queue_is_empty(UART_QUEUE_T *me);
unsigned short ela_uart_queue_count(UART_QUEUE_T *me);

#endif

