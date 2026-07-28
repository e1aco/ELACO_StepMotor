/********
 * @ 文件: ela_uart_queue.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-17
 * @ 版本: 1.0.0
 * @ 说明: 字节级环形缓冲区实现
 ********/

#include "ela_uart_queue.h"

/* ela_uart_queue usr start */

/********
 * @ 说明: 初始化队列为空
 ********/
void ela_uart_queue_init(UART_QUEUE_T *me)
{
    me->head = 0;
    me->tail = 0;
    me->count = 0;
}

/********
 * @ 输入: me: 队列指针; byte: 写入字节
 * @ 输出: true 成功, false 队列满
 * @ 说明: 向队尾写入一个字节
 ********/
bool ela_uart_queue_put(UART_QUEUE_T *me, unsigned char byte)
{
    if (me->count >= UART_QUEUE_SIZE)
    {
        return false;
    }
    me->buffer[me->head] = byte;
    me->head = (me->head + 1) % UART_QUEUE_SIZE;
    me->count++;
    return true;
}

/********
 * @ 输入: me: 队列指针; byte: 读出字节
 * @ 输出: true 成功, false 队列空
 * @ 说明: 从队头读出一个字节
 ********/
bool ela_uart_queue_get(UART_QUEUE_T *me, unsigned char *byte)
{
    if (0 == me->count)
    {
        return false;
    }
    *byte = me->buffer[me->tail];
    me->tail = (me->tail + 1) % UART_QUEUE_SIZE;
    me->count--;
    return true;
}

/********
 * @ 输入: me: 队列指针; data: 源数据; len: 写入长度
 * @ 输出: 实际写入字节数
 * @ 说明: 批量写入，适用于 ISR 中整帧入队
 ********/
unsigned short ela_uart_queue_put_buf(UART_QUEUE_T *me,
                                      unsigned char *data,
                                      unsigned short len)
{
    unsigned short i;
    unsigned short avail;

    avail = UART_QUEUE_SIZE - me->count;
    if (len > avail)
    {
        len = avail;
    }

    for (i = 0; i < len; i++)
    {
        me->buffer[me->head] = data[i];
        me->head = (me->head + 1) % UART_QUEUE_SIZE;
    }
    me->count += len;
    return len;
}

/********
 * @ 输入: me: 队列指针; data: 目标缓冲区; len: 读取长度
 * @ 输出: 实际读出字节数
 * @ 说明: 批量读取，适用于应用层整帧出队
 ********/
unsigned short ela_uart_queue_get_buf(UART_QUEUE_T *me,
                                      unsigned char *data,
                                      unsigned short len)
{
    unsigned short i;

    if (len > me->count)
    {
        len = me->count;
    }

    for (i = 0; i < len; i++)
    {
        data[i] = me->buffer[me->tail];
        me->tail = (me->tail + 1) % UART_QUEUE_SIZE;
    }
    me->count -= len;
    return len;
}

/********
 * @ 输出: true 满, false 未满
 * @ 说明: 判断队列是否已满
 ********/
bool ela_uart_queue_is_full(UART_QUEUE_T *me)
{
    return (me->count >= UART_QUEUE_SIZE);
}

/********
 * @ 输出: true 空, false 非空
 * @ 说明: 判断队列是否为空
 ********/
bool ela_uart_queue_is_empty(UART_QUEUE_T *me)
{
    return (0 == me->count);
}

/********
 * @ 输出: 当前队列中的字节数
 * @ 说明: 返回队列中可读的字节数
 ********/
unsigned short ela_uart_queue_count(UART_QUEUE_T *me)
{
    return me->count;
}

/* ela_uart_queue usr end */

