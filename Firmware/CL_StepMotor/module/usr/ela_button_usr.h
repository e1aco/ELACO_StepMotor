/*****************************************************************************
 * @文件: ela_button_usr.h
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: 按键应用层，消抖状态机与事件 API
 ****************************************************************************/

#ifndef ELA_BUTTON_USR_H
#define ELA_BUTTON_USR_H

#include <stdbool.h>
#include <stdint.h>

/* ==== 接口 ==== */


void USR_Button_Init(void);
void USR_Button_Tick(void);
bool USR_Button_GetClick(uint8_t id);
bool USR_Button_GetLong(uint8_t id);
bool USR_Button_GetBothLong(void);
bool USR_Button_IsPressed(uint8_t id);

#endif






