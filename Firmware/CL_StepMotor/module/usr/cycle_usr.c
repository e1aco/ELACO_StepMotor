/*****************************************************************************
 * @文件: cycle_usr.c
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v1.0
 * @说明: 循环域运算工具实现（角度/计数回绕处理的取模/求差/平均）
 * @平台: 通用（无硬件依赖，任意模块可引用）
 * @依赖: stdlib.h(供 abs)
 ****************************************************************************/
#include "cycle_usr.h"
#include <stdlib.h>

/* ==== 常量定义 ==== */
/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
/* ==== 内部工具 ==== */
/* ==== 接口实现 ==== */
/**
 * @输入 a 角度; b 模数
 * @输出 (a+b)%b，恒为 [0, b)
 * @说明 循环取模（角度域 0~b 回绕，负数安全）
 */
uint32_t USR_Cycle_Mod(int32_t a, int32_t b)
{
    return (uint32_t)((a + b) % b);
}

/**
 * @输入 a 角度A; b 角度B; cyc 一圈刻度
 * @输出 最短循环差，范围 (-cyc/2, cyc/2]
 * @说明 循环角度求差（10° 与 350° 差值为 20° 而非 -340°）
 */
int32_t USR_Cycle_Sub(int32_t a, int32_t b, int32_t cyc)
{
    int32_t sub = a - b;
    if (sub > (cyc >> 1))
    {
        sub -= cyc;
    }
    if (sub < (-(cyc >> 1)))
    {
        sub += cyc;
    }
    return sub;
}

/**
 * @输入 a 角度A; b 角度B; cyc 一圈刻度
 * @输出 两角度的循环平均值
 * @说明 正确处理跨 0 点（350° 与 10° 平均为 0° 而非 180°）
 */
int32_t USR_Cycle_Avg(int32_t a, int32_t b, int32_t cyc)
{
    int32_t sub = a - b;
    int32_t ave = (a + b) >> 1;
    if (abs(sub) > (cyc >> 1))
    {
        if (ave >= (cyc >> 1))
        {
            ave -= (cyc >> 1);
        }
        else
        {
            ave += (cyc >> 1);
        }
    }
    return ave;
}

/**
 * @输入 data 采样数组; len 数组长度; cyc 一圈刻度
 * @输出 循环域数组平均值（[0, cyc)）
 * @说明 以 data[0] 为基准把各样本换算到循环域再平均
 */
int32_t USR_Cycle_DataAvg(const uint16_t *data, uint16_t len, int32_t cyc)
{
    int32_t sum = data[0];
    int32_t i;
    for (i = 1; i < len; i++)
    {
        int32_t sub = data[i] - data[0];
        int32_t diff = data[i];
        if (sub > (cyc >> 1))
        {
            diff = data[i] - cyc;
        }
        if (sub < (-(cyc >> 1)))
        {
            diff = data[i] + cyc;
        }
        sum += diff;
    }
    sum = sum / (int32_t)len;
    if (sum < 0)
    {
        sum += cyc;
    }
    if (sum > cyc)
    {
        sum -= cyc;
    }
    return sum;
}
