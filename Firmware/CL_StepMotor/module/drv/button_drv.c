/*****************************************************************************
 * @文件: button_drv.c
 * @作者: cl
 * @日期: 2026-08-13
 * @版本: v1.0
 * @说明: 按键硬件驱动层（GPIO 读原语，低有效归一化，无业务状态机）
 * @平台: STM32F103RET6
 * @依赖: HAL_GPIO, main.h
 ****************************************************************************/
#include "button_drv.h"
#include "main.h"

/* ==== 内部工具 ==== */
/* ==== 全局实例 ==== */
/* ==== 接口实现 ==== */
/**
 * @输入 id: DRV_BUTTON1/SW1(PB2) 或 DRV_BUTTON2/SW2(PB1)
 * @输出 true=按下 false=未按下
 * @说明 读取按键引脚电平，低有效归一化为按下状态
 * 依据 .cl/memory/ button_active_level=低有效(复刻参考 button.c ReadPin==RESET 判按下)
 */
bool DRV_Button_ReadPin(uint8_t id)
{
    if (DRV_BUTTON1 == id)
    {
        return (GPIO_PIN_RESET == HAL_GPIO_ReadPin(SW1_GPIO_Port, SW1_Pin));
    }
    if (DRV_BUTTON2 == id)
    {
        return (GPIO_PIN_RESET == HAL_GPIO_ReadPin(SW2_GPIO_Port, SW2_Pin));
    }
    return false;
}
