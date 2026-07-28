/********
 * @ 文件: ela_button_drv.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-27
 * @ 版本: 1.0.0
 * @ 说明: 按键硬件驱动层，GPIO 读取原语
 ********/

#include "ela_button_drv.h"

/* button drv start */

/********
 * @ 输入: id: 按键编号 (1 或 2)
 * @ 输出: true=按下, false=未按下
 * @ 说明: 读取指定按键的 GPIO 电平
 ********/
bool button_drv_read_pin(uint8_t id)
{
    switch (id)
    {
        case 1:
            return HAL_GPIO_ReadPin(BUTTON1_PORT,
                                    BUTTON1_PIN)
                   == GPIO_PIN_RESET;
        case 2:
            return HAL_GPIO_ReadPin(BUTTON2_PORT,
                                    BUTTON2_PIN)
                   == GPIO_PIN_RESET;
        default:
            return false;
    }
}

/* button drv end */

