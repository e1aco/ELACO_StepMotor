/*****************************************************************************
 * @文件: button_usr.h
 * @作者: cl
 * @日期: 2026-08-13
 * @版本: v1.0
 * @说明: 按键用户层（去抖/边沿检测/单击/长按事件状态机，只调 DRV 原语）
 * @平台: STM32F103RET6
 * @依赖: button_drv
 ****************************************************************************/
#ifndef BUTTON_USR_H
#define BUTTON_USR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ==== 常量定义 ==== */
#define USR_BUTTON1   1U   /* SW1 = PB2 */
#define USR_BUTTON2   2U   /* SW2 = PB1 */

/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
/* ==== 接口 ==== */
void USR_Button_Init(void);
void USR_Button_Tick(void);           /* 每 10ms 调用一次(100Hz) */
bool USR_Button_GetClick(uint8_t id); /* 读取单击事件，读取后自动清除 */
bool USR_Button_GetLong(uint8_t id);  /* 读取长按事件，读取后自动清除 */
bool USR_Button_IsPressed(uint8_t id);/* 当前是否按下 */

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_USR_H */
