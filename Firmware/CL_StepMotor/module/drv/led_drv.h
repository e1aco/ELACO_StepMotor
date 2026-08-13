/*****************************************************************************
 * @文件: led_drv.h
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v1.0
 * @说明: LED 硬件驱动层（GPIO 开关原语）
 * @平台: STM32F103RET6
 * @依赖: HAL_GPIO
 ****************************************************************************/
#ifndef LED_DRV_H
#define LED_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ==== 常量定义 ==== */
#define DRV_LED1           0U   /* LED1 = PB13 */
#define DRV_LED2           1U   /* LED2 = PB12 */

/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
/* ==== 接口 ==== */
void DRV_LED_Init(void);
void DRV_LED_Set(uint8_t id, bool state);

#ifdef __cplusplus
}
#endif

#endif /* LED_DRV_H */
