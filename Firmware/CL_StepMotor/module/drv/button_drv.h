/*****************************************************************************
 * @文件: button_drv.h
 * @作者: cl
 * @日期: 2026-08-13
 * @版本: v1.0
 * @说明: 按键硬件驱动层（GPIO 读原语，无业务状态机）
 * @平台: STM32F103RET6
 * @依赖: HAL_GPIO
 ****************************************************************************/
#ifndef BUTTON_DRV_H
#define BUTTON_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ==== 常量定义 ==== */
#define DRV_BUTTON1   1U   /* SW1 = PB2 */
#define DRV_BUTTON2   2U   /* SW2 = PB1 */

/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
/* ==== 接口 ==== */
bool DRV_Button_ReadPin(uint8_t id);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_DRV_H */
