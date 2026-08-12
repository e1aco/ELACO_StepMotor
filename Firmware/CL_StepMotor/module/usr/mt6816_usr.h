/*****************************************************************************
 * @文件: mt6816_usr.h
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v1.0
 * @说明: MT6816 用户层（编码器角度业务：偶校验重试策略 + 校准表映射）
 * @平台: STM32F103RET6 (SPI1, CS=PA4)
 * @依赖: mt6816_drv
 ****************************************************************************/
#ifndef MT6816_USR_H
#define MT6816_USR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ==== 常量定义 ==== */
/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
/* ==== 接口 ==== */
bool     USR_MT6816_Init(void);
uint16_t USR_MT6816_UpdateAngle(void);
uint16_t USR_MT6816_GetRawAngle(void);
uint16_t USR_MT6816_GetRectifiedAngle(void);
bool     USR_MT6816_IsCalibrated(void);
void     USR_MT6816_SetCalibrationData(uint16_t *cali_data_ptr);

#ifdef __cplusplus
}
#endif

#endif /* MT6816_USR_H */
