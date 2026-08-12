/*****************************************************************************
 * @文件: cycle_usr.h
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v1.0
 * @说明: 循环域运算工具（角度/计数的取模、求差、平均，处理 0 点回绕）
 * @平台: 通用（无硬件依赖，任意模块可引用）
 * @依赖: stdint.h, stdlib.h
 ****************************************************************************/
#ifndef CYCLE_USR_H
#define CYCLE_USR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ==== 常量定义 ==== */
/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
/* ==== 接口 ==== */
uint32_t USR_Cycle_Mod(int32_t a, int32_t b);
int32_t  USR_Cycle_Sub(int32_t a, int32_t b, int32_t cyc);
int32_t  USR_Cycle_Avg(int32_t a, int32_t b, int32_t cyc);
int32_t  USR_Cycle_DataAvg(const uint16_t *data, uint16_t len, int32_t cyc);

#ifdef __cplusplus
}
#endif

#endif /* CYCLE_USR_H */
