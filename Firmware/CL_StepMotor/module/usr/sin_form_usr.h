/*****************************************************************************
 * @文件: sin_form_usr.h
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v1.0
 * @说明: 正弦波形表数据模块头（整电周期 1024 点，幅值 4096=2^12，供 FOC 算法查表）
 * @平台: STM32F103RET6
 * @依赖: 无
 ****************************************************************************/
#ifndef SIN_FORM_USR_H
#define SIN_FORM_USR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ==== 常量定义 ==== */
#define USR_SIN_PI_M2_DPIX      1024U   /* 一电周期点数(电角度 0~1023) */
#define USR_SIN_PI_M2_DPIYBIT   12U     /* 表定点缩放位(幅值 4096=2^12) */
#define USR_SIN_PI_M2_LEN       1025U   /* 表长(1024 点 + 末尾 1 点回绕=0) */

/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
/* ==== 接口 ==== */
extern const int16_t USR_sin_pi_m2[USR_SIN_PI_M2_LEN];

#ifdef __cplusplus
}
#endif

#endif /* SIN_FORM_USR_H */
