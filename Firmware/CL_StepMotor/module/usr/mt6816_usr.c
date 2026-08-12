/*****************************************************************************
 * @文件: mt6816_usr.c
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v1.0
 * @说明: MT6816 用户层（编码器角度业务：偶校验重试策略 + 校准表映射）
 * @平台: STM32F103RET6 (SPI1, CS=PA4)
 * @依赖: mt6816_drv
 ****************************************************************************/
#include "mt6816_usr.h"
#include "mt6816_drv.h"
#include <stddef.h>

/* ==== 常量定义 ==== */
#define MT6816_RETRY_MAX   3U   /* 偶校验失败最大重试次数 */

/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
static uint16_t *s_cali_ptr = NULL;      /* 校准表指针 */
static uint16_t s_raw_angle = 0;         /* 原始角度 */
static uint16_t s_rectified_angle = 0;   /* 校准后角度 */

/* ==== 内部工具 ==== */
/* ==== 接口实现 ==== */
/**
 * @输入 无
 * @输出 true=校准表有效 false=未校准
 * @说明 初始化：先读一次角度并校验校准表是否可用
 *   （等效原复刻源 mt6816.c 的 MT6816_Init 行为）
 */
bool USR_MT6816_Init(void)
{
    USR_MT6816_UpdateAngle();

    if (NULL == s_cali_ptr)
    {
        return false;
    }
    return USR_MT6816_IsCalibrated();
}

/**
 * @输入 无
 * @输出 校准后角度(0~16383)，偶校验连续失败时保持上次值
 * @说明 读绝对角度：≤3 次重试直至偶校验通过，再经校准表映射
 */
uint16_t USR_MT6816_UpdateAngle(void)
{
    uint8_t i;
    bool parity_ok = false;

    for (i = 0; i < MT6816_RETRY_MAX; i++)
    {
        if (DRV_MT6816_ReadAngle(&s_raw_angle, NULL))
        {
            parity_ok = true;
            break;
        }
    }

    if (parity_ok)
    {
        /* 校准表映射（无校准表则原样返回原始角） */
        if (NULL != s_cali_ptr)
        {
            s_rectified_angle = s_cali_ptr[s_raw_angle];
        }
        else
        {
            s_rectified_angle = s_raw_angle;
        }
    }

    return s_rectified_angle;
}

/**
 * @输入 无
 * @输出 原始角度(未校准)
 * @说明 获取最近一次读取的原始角度
 */
uint16_t USR_MT6816_GetRawAngle(void)
{
    return s_raw_angle;
}

/**
 * @输入 无
 * @输出 校准后角度
 * @说明 获取最近一次读取的校准后角度
 */
uint16_t USR_MT6816_GetRectifiedAngle(void)
{
    return s_rectified_angle;
}

/**
 * @输入 无
 * @输出 true=校准表有效 false=未校准
 * @说明 检查校准表指针与数据完整性（全表无 0xFFFF 空槽）
 */
bool USR_MT6816_IsCalibrated(void)
{
    uint32_t i;

    if (NULL == s_cali_ptr)
    {
        return false;
    }
    for (i = 0; i < DRV_MT6816_RESOLUTION; i++)
    {
        if (0xFFFFU == s_cali_ptr[i])
        {
            return false;
        }
    }
    return true;
}

/**
 * @输入 cali_data_ptr: 校准表指针(16384×uint16)
 * @输出 无
 * @说明 设置校准表指针（在 Init 之前调用）
 */
void USR_MT6816_SetCalibrationData(uint16_t *cali_data_ptr)
{
    s_cali_ptr = cali_data_ptr;
}
