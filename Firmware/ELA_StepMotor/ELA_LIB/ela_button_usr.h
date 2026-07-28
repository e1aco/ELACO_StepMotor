/********
 * @ 文件: ela_button_usr.h
 * @ 作者: ELACO
 * @ 日期: 2026-07-27
 * @ 版本: 1.0.0
 * @ 说明: 按键应用层，消抖状态机与事件 API
 * @ 依赖: ela_button_drv
 ********/

#ifndef ELA_BUTTON_USR_H
#define ELA_BUTTON_USR_H

#include <stdbool.h>
#include <stdint.h>

void ela_button_init(void);
void ela_button_tick(void);
bool ela_button_get_click(uint8_t id);
bool ela_button_get_long(uint8_t id);
bool ela_button_is_pressed(uint8_t id);

#endif

