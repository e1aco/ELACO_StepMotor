#include "ela_uart.h"

/* 串口接收相关变量 */
unsigned char gov_rx1_offset = 0;            // 接收数据偏移量，配合空闲中断计算本次接收长度
unsigned char gov_rx1_buf[DMA_BUF_SIZE];     // 串口 DMA 接收缓冲区

typedef struct __FILE FILE;


/****
 * @ 原型: int fputc(int ch, FILE *f)
 * @ 输入: int ch: 要发送的字符; FILE *f: 文件指针（未使用）
 * @ 输出: int: 返回发送的字符
 * @ 说明: 重定向 printf 函数到串口输出
 ********/
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart1, (unsigned char*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/****
 * @ 原型: int __io_putchar(int ch)
 * @ 输入: int ch: 要发送的字符
 * @ 输出: int: 返回发送的字符
 * @ 说明: Newlib 底层写字符函数，用于 `_write` → `__io_putchar` 路径的 printf 重定向
 ********/
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart1, (unsigned char*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
