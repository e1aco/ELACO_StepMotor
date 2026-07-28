/********
 * @ 文件: ela_button_drv.h
 * @ 作者: ELACO
 * @ 日期: 2026-07-27
 * @ 版本: 1.0.0
 * @ 说明: 按键硬件驱动层，GPIO 读取原语
 ********/

#ifndef ELA_BUTTON_DRV_H
#define ELA_BUTTON_DRV_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

#define BUTTON1_PIN   SW2_Pin
#define BUTTON1_PORT  SW2_GPIO_Port
#define BUTTON2_PIN   SW1_Pin
#define BUTTON2_PORT  SW1_GPIO_Port

bool button_drv_read_pin(uint8_t id);

#endif

