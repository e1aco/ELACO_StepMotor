/*****************************************************************************
 * @文件: led_drv.c
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v1.0
 * @说明: LED 硬件驱动层（GPIO 开关原语，无业务状态机）
 * @平台: STM32F103RET6
 * @依赖: HAL_GPIO, main.h
 ****************************************************************************/
#include "led_drv.h"
#include "main.h"

/* ==== 内部工具 ==== */
/**
 * @输入 state: true=亮 false=灭
 * @输出 无
 * @说明 点亮/熄灭指定 LED，封装 GPIO 极性差异
 */
static void s_SetLed(uint8_t id, bool state)
{
    GPIO_TypeDef *port;
    uint16_t      pin;

    if (DRV_LED1 == id)
    {
        port = LED1_GPIO_Port;
        pin  = LED1_Pin;
    }
    else
    {
        port = LED2_GPIO_Port;
        pin  = LED2_Pin;
    }

    HAL_GPIO_WritePin(port, pin, (state) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* ==== 全局实例 ==== */
/* ==== 接口实现 ==== */
/**
 * @输入 无
 * @输出 无
 * @说明 初始化两路 LED 为熄灭状态
 */
void DRV_LED_Init(void)
{
    s_SetLed(DRV_LED1, false);
    s_SetLed(DRV_LED2, false);
}

/**
 * @输入 id: DRV_LED1/DRV_LED2; state: true=亮 false=灭
 * @输出 无
 * @说明 控制指定 LED 亮灭
 */
void DRV_LED_Set(uint8_t id, bool state)
{
    s_SetLed(id, state);
}
