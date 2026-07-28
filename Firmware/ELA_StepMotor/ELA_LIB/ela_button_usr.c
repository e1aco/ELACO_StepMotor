/********
 * @ 文件: ela_button_usr.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-27
 * @ 版本: 1.0.0
 * @ 说明: 按键应用层，消抖状态机与事件 API
 * @ 依赖: ela_button_drv
 ********/

#include "ela_button_usr.h"
#include "ela_button_drv.h"

#define BUTTON_NUM      2
#define LONG_PRESS_MS   3000

static bool s_pressed[BUTTON_NUM + 1];
static uint32_t s_press_time[BUTTON_NUM + 1];
static bool s_click_flag[BUTTON_NUM + 1];
static bool s_long_flag[BUTTON_NUM + 1];
static bool s_last_state[BUTTON_NUM + 1];

/* button usr start */

/********
 * @ 说明: 初始化按键状态，读取初始电平
 ********/
void ela_button_init(void)
{
    for (uint8_t i = 1; i <= BUTTON_NUM; i++)
    {
        s_last_state[i] = button_drv_read_pin(i);
        s_pressed[i] = false;
        s_press_time[i] = 0;
        s_click_flag[i] = false;
        s_long_flag[i] = false;
    }
}

/********
 * @ 说明: 按键扫描，在主循环或 10ms 定时器中调用。
 *         检测按下/释放/长按状态并设置事件标志
 ********/
void ela_button_tick(void)
{
    uint32_t now = HAL_GetTick();

    for (uint8_t i = 1; i <= BUTTON_NUM; i++)
    {
        bool cur = button_drv_read_pin(i);

        if (cur && !s_last_state[i])
        {
            s_pressed[i] = true;
            s_press_time[i] = now;
            s_click_flag[i] = false;
            s_long_flag[i] = false;
        }

        if (!cur && s_last_state[i])
        {
            s_pressed[i] = false;
            if (!s_long_flag[i])
            {
                s_click_flag[i] = true;
            }
        }

        if (s_pressed[i])
        {
            if (!s_long_flag[i]
                && (now - s_press_time[i]) >= LONG_PRESS_MS)
            {
                s_long_flag[i] = true;
                s_click_flag[i] = false;
            }
        }

        s_last_state[i] = cur;
    }
}

/********
 * @ 输入: id: 按键编号 (1 或 2)
 * @ 输出: true=检测到单击
 * @ 说明: 读取单击标志，读取后自动清除
 ********/
bool ela_button_get_click(uint8_t id)
{
    if ((id < 1) || (id > BUTTON_NUM))
        return false;

    if (s_click_flag[id])
    {
        s_click_flag[id] = false;
        return true;
    }
    return false;
}

/********
 * @ 输入: id: 按键编号 (1 或 2)
 * @ 输出: true=检测到长按
 * @ 说明: 读取长按标志，读取后自动清除
 ********/
bool ela_button_get_long(uint8_t id)
{
    if ((id < 1) || (id > BUTTON_NUM))
        return false;

    if (s_long_flag[id])
    {
        s_long_flag[id] = false;
        return true;
    }
    return false;
}

/********
 * @ 输入: id: 按键编号 (1 或 2)
 * @ 输出: true=当前按下
 * @ 说明: 直接读取按键当前电平
 ********/
bool ela_button_is_pressed(uint8_t id)
{
    return button_drv_read_pin(id);
}

/* button usr end */

