/*****************************************************************************
 * @文件: uart_drv.c
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v1.0
 * @说明: 调试串口驱动层（USART3 115200 发送原语）
 * @平台: STM32F103RET6
 * @依赖: HAL_UART, usart.h
 ****************************************************************************/
#include "uart_drv.h"
#include "usart.h"
#include <string.h>

/* ==== 内部工具 ==== */
/* ==== 全局实例 ==== */
/* ==== 接口实现 ==== */
/**
 * @输入 data: 数据指针; len: 长度
 * @输出 无
 * @说明 通过 USART3 阻塞发送调试回传数据
 */
void DRV_Uart_SendBlocking(const uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&huart3, data, len, HAL_MAX_DELAY);
}

/**
 * @输入 str: 以 '\0' 结尾字符串
 * @输出 无
 * @说明 通过 USART3 阻塞发送字符串（调试回传）
 */
void DRV_Uart_SendString(const char *str)
{
    DRV_Uart_SendBlocking((const uint8_t *)str, (uint16_t)strlen(str));
}
