#ifndef _ELA_BUTTON_H_
#define _ELA_BUTTON_H_

#include "elaco_main.h" // HAL库


void Button_Init(void);
void Button_Scan_Proc(void);
bool Button_GetClick(unsigned char id);    // 获取按键单击事件（一次性读取）
bool Button_GetLong(unsigned char id);     // 获取按键长按事件（一次性读取）
bool Button_IsPressed(unsigned char id);   // 直接读取按键当前电平状态


#endif
