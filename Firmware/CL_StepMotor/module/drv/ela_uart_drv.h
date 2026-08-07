/*****************************************************************************
 * @文件: ela_uart_drv.h
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: UART 硬件驱动层，printf 重定向原语
 ****************************************************************************/

#ifndef ELA_UART_DRV_H
#define ELA_UART_DRV_H

#include <stdio.h>

/* ==== 接口 ==== */
/********
 * @输入: ch: 要发送的字符
 * @输出: 发送的字符
 * @说明: 重定向 printf 到 USART3 (ARM Compiler 路径)
 ********/


int fputc(int ch, FILE *f);

/********
 * @输入: ch: 要发送的字符
 * @输出: 发送的字符
 * @说明: 重定向 printf 到 USART3 (Newlib 路径)
 ********/
int __io_putchar(int ch);

#endif






