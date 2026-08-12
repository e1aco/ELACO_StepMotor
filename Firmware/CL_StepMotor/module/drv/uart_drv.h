/*****************************************************************************
 * @文件: uart_drv.h
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v1.0
 * @说明: 调试串口驱动层（USART3 115200 发送原语）
 * @平台: STM32F103RET6
 * @依赖: HAL_UART
 ****************************************************************************/
#ifndef UART_DRV_H
#define UART_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ==== 常量定义 ==== */
/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
/* ==== 接口 ==== */
void DRV_Uart_SendBlocking(const uint8_t *data, uint16_t len);
void DRV_Uart_SendString(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* UART_DRV_H */
