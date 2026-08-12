/*****************************************************************************
 * @文件: encoder_calibrator_usr.h
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v1.0
 * @说明: 编码器校准用户层（采样→校验→生成校准表写 Flash，只调 DRV/USR 原语）
 * @平台: STM32F103RET6
 * @依赖: mt6816_usr, tb67h450_usr, flash_drv, uart_drv
 ****************************************************************************/
#ifndef ENCODER_CALIBRATOR_USR_H
#define ENCODER_CALIBRATOR_USR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ==== 常量定义 ==== */
/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
/* ==== 接口 ==== */
void     USR_EncoderCalibrator_Init(void);
void     USR_EncoderCalibrator_Trigger(void);
void     USR_EncoderCalibrator_Tick20kHz(void);
void     USR_EncoderCalibrator_TickMainLoop(void);
bool     USR_EncoderCalibrator_IsCalibrated(void);
bool     USR_EncoderCalibrator_IsTriggered(void);
uint16_t USR_EncoderCalibrator_GetRectifiedAngle(uint16_t raw_angle);

#ifdef __cplusplus
}
#endif

#endif /* ENCODER_CALIBRATOR_USR_H */
