/*****************************************************************************
 * @文件: tb67h450_usr.h
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v1.0
 * @说明: TB67H450 用户层（两相正弦 FOC 电流矢量算法，只调 DRV 原语）
 * @平台: STM32F103RET6
 * @依赖: tb67h450_drv, sin_form_usr
 ****************************************************************************/
#ifndef TB67H450_USR_H
#define TB67H450_USR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ==== 常量定义 ==== */
/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
/* ==== 接口 ==== */
void USR_TB67H450_Init(void);
void USR_TB67H450_SetFocCurrentVector(uint32_t direction_in_count, int32_t current_mA);
void USR_TB67H450_Sleep(void);
void USR_TB67H450_Brake(void);

#ifdef __cplusplus
}
#endif

#endif /* TB67H450_USR_H */
