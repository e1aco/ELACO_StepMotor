/*****************************************************************************
 * @文件: button_usr.c
 * @作者: cl
 * @日期: 2026-08-13
 * @版本: v1.0
 * @说明: 按键用户层（去抖/边沿检测/单击/长按事件状态机，只调 DRV 原语）
 * @平台: STM32F103RET6
 * @依赖: button_drv, sys_drv
 ****************************************************************************/
#include "button_usr.h"
#include "button_drv.h"
#include "sys_drv.h"

/* ==== 常量定义 ==== */
#define USR_BUTTON_NUM       2U   /* 按键数量 */
#define USR_BUTTON_LONG_MS   3000U /* 长按判定阈值(ms) */

/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
static bool     s_pressed[USR_BUTTON_NUM + 1U];   /* 当前按下状态 */
static uint32_t s_press_time[USR_BUTTON_NUM + 1U];/* 按下起始 tick */
static bool     s_click_flag[USR_BUTTON_NUM + 1U];/* 单击事件挂起 */
static bool     s_long_flag[USR_BUTTON_NUM + 1U]; /* 长按事件挂起 */
static bool     s_last_state[USR_BUTTON_NUM + 1U];/* 上次扫描电平 */

/* ==== 内部工具 ==== */
/* ==== 接口实现 ==== */
/**
 * @输入 无
 * @输出 无
 * @说明 初始化：读取各按键初始电平并清空事件标志
 */
void USR_Button_Init(void)
{
    uint8_t i;

    for (i = 1U; i <= USR_BUTTON_NUM; i++)
    {
        s_last_state[i] = DRV_Button_ReadPin(i);
        s_pressed[i]    = false;
        s_press_time[i] = 0U;
        s_click_flag[i] = false;
        s_long_flag[i]  = false;
    }
}

/**
 * @输入 无
 * @输出 无
 * @说明 100Hz(10ms) 周期扫描：边沿检测（按下/释放）+ 长按计时。
 *   释放时未长按 → 记单击；按住达阈值 → 记长按并取消单击
 * 依据 .cl/memory/ button_scan_freq=100Hz + button_long_press_ms=3000
 */
void USR_Button_Tick(void)
{
    uint32_t now = DRV_Sys_GetTickMs();
    uint8_t  i;

    for (i = 1U; i <= USR_BUTTON_NUM; i++)
    {
        bool cur = DRV_Button_ReadPin(i);

        /* 按下沿：记录按下时间，清事件 */
        if (cur && !s_last_state[i])
        {
            s_pressed[i]    = true;
            s_press_time[i] = now;
            s_click_flag[i] = false;
            s_long_flag[i]  = false;
        }

        /* 释放沿：未长按过 → 单击 */
        if (!cur && s_last_state[i])
        {
            s_pressed[i] = false;
            if (!s_long_flag[i])
            {
                s_click_flag[i] = true;
            }
        }

        /* 按住中：超阈值判长按 */
        if (s_pressed[i])
        {
            if (!s_long_flag[i] && (now - s_press_time[i]) >= USR_BUTTON_LONG_MS)
            {
                s_long_flag[i]  = true;
                s_click_flag[i] = false;
            }
        }

        s_last_state[i] = cur;
    }
}

/**
 * @输入 id: USR_BUTTON1/USR_BUTTON2
 * @输出 true=有单击事件(已清除) false=无
 * @说明 读取单击事件并自动清除（一次按键只消费一次）
 */
bool USR_Button_GetClick(uint8_t id)
{
    if (id < 1U || id > USR_BUTTON_NUM)
    {
        return false;
    }
    if (s_click_flag[id])
    {
        s_click_flag[id] = false;
        return true;
    }
    return false;
}

/**
 * @输入 id: USR_BUTTON1/USR_BUTTON2
 * @输出 true=有长按事件(已清除) false=无
 * @说明 读取长按事件并自动清除（一次按键只消费一次）
 */
bool USR_Button_GetLong(uint8_t id)
{
    if (id < 1U || id > USR_BUTTON_NUM)
    {
        return false;
    }
    if (s_long_flag[id])
    {
        s_long_flag[id] = false;
        return true;
    }
    return false;
}

/**
 * @输入 id: USR_BUTTON1/USR_BUTTON2
 * @输出 true=当前按住 false=未按住
 * @说明 实时查询当前是否按住（供上电双键组合判断等）
 */
bool USR_Button_IsPressed(uint8_t id)
{
    if (id < 1U || id > USR_BUTTON_NUM)
    {
        return false;
    }
    return DRV_Button_ReadPin(id);
}
