/*****************************************************************************
 * @文件: mt6816_drv.h
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v2.0
 * @说明: MT6816 磁性编码器硬件驱动层（SPI 读角度原语 + 偶校验协议，无业务）
 * @平台: STM32F103RET6 (SPI1, CS=PA4)
 * @依赖: HAL_SPI, HAL_GPIO
 ****************************************************************************/
#ifndef MT6816_DRV_H
#define MT6816_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ==== 常量定义 ==== */
#define DRV_MT6816_RESOLUTION   16384U   /* 14bit 绝对角度 0~16383 */

/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
/* ==== 接口 ==== */
bool     DRV_MT6816_ReadAngle(uint16_t *raw_angle, bool *no_mag_flag);
uint16_t DRV_MT6816_TestRead(void);

#ifdef __cplusplus
}
#endif

#endif /* MT6816_DRV_H */
