/********
 * @ 文件: ela_uart_drv.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-23
 * @ 版本: 1.0.0
 * @ 说明: UART 硬件驱动层，printf 重定向原语
 ********/

#include "ela_uart_drv.h"
#include "ela_uart.h"

/* uart drv start */

/********
 * @ 说明: 重定向 printf 到 USART3 (ARM Compiler / Keil 路径)
 * @ 注意: 每字符阻塞发送，调试信息量大时可能影响实时性
 ********/
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart3, (unsigned char *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/********
 * @ 说明: 重定向 printf 到 USART3 (Newlib / PicoLibC _write 路径)
 * @ 注意: 覆盖 syscalls.c 中的弱定义
 ********/
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart3, (unsigned char *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

/* uart drv end */

