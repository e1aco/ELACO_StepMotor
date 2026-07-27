#include "ela_button.h"

#define BUTTON_NUM      2
#define LONG_PRESS_MS   3000

static bool pressed[BUTTON_NUM + 1]; // 按键按下标志
static unsigned int press_time[BUTTON_NUM + 1]; // 按键按下时间
static bool click_flag[BUTTON_NUM + 1]; // 按键单击标志
static bool long_flag[BUTTON_NUM + 1]; // 按键长按标志
static bool last_state[BUTTON_NUM + 1]; // 按键上一次状态


/****
 * @ 原型: static bool Readpin(unsigned char id)
 * @ 输入: unsigned char id: 按键编号，1或2
 * @ 输出: bool: 按键状态，true表示按下，false表示未按下
 * @ 说明: 读取指定编号的按键状态，返回true表示按下，false表示未按下
 ********/
static bool Readpin(unsigned char id)
{
    switch (id) {
        case 1: return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET;
        case 2: return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET;
        default: return false;
    }
}

/****
 * @ 原型: void Button_Init(void)
 * @ 输入: void
 * @ 输出: void
 * @ 说明: 初始化按键状态，在主循环开始前调用一次
 ********/
void Button_Init(void)
{
    for (int i = 1; i <= BUTTON_NUM; i++) {
        last_state[i] = Readpin(i); // 初始化时读取按键状态
        pressed[i] = false; // 按键未按下
        press_time[i] = 0;  // 按键按下时间
        click_flag[i] = false;  // 按键单击标志
        long_flag[i] = false;  // 按键长按标志
    }
}

/****
 * @ 原型: void button_scan_proc(void)
 * @ 输入: void
 * @ 输出: void
 * @ 说明: 按键状态更新函数，在主循环中定期调用，负责检测按键状态变化，更新按键事件标志
 ********/
void Button_Scan_Proc(void)
{
    unsigned int now = HAL_GetTick();

    for (int i = 1; i <= BUTTON_NUM; i++) {
        bool cur = Readpin(i);

        // 检测按键按下（上升沿：上一次未按下，当前已按下）
        if (cur == true && last_state[i] == false) {
            pressed[i] = true;
            press_time[i] = now;
            click_flag[i] = false;
            long_flag[i] = false;
        }

        // 检测按键释放（下降沿：上一次已按下，当前未按下）
        if (cur == false && last_state[i] == true) {
            pressed[i] = false;

            // 如果未触发长按，则记为一次单击
            if (long_flag[i] == false) {
                click_flag[i] = true;
            }
        }

        // 检测长按（持续按下时间超过 LONG_PRESS_MS）
        if (pressed[i] == true) {
            if (long_flag[i] == false && (now - press_time[i]) >= LONG_PRESS_MS) {
                long_flag[i] = true;
                click_flag[i] = false;  // 触发长按后清除单击标志，避免同时触发单击和长按
            }
        }

        last_state[i] = cur;
    }
}

/****
 * @ 原型: bool Button_GetClick(unsigned char id)
 * @ 输入: unsigned char id: 按键编号，1或2
 * @ 输出: bool: 检测到单击返回true，否则返回false
 * @ 说明: 获取指定按键的单击事件标志，读取后自动清除标志（一次性读取）
 ********/
bool Button_GetClick(unsigned char id)
{
    if (id < 1 || id > BUTTON_NUM) return false;
    if (click_flag[id]) {
        click_flag[id] = false;
        return true;
    }
    return false;
}

/****
 * @ 原型: bool Button_GetLong(unsigned char id)
 * @ 输入: unsigned char id: 按键编号，1或2
 * @ 输出: bool: 检测到长按返回true，否则返回false
 * @ 说明: 获取指定按键的长按事件标志，读取后自动清除标志（一次性读取）
 ********/
bool Button_GetLong(unsigned char id)
{
    if (id < 1 || id > BUTTON_NUM) return false;
    if (long_flag[id]) {
        long_flag[id] = false;
        return true;
    }
    return false;
}

/****
 * @ 原型: bool Button_IsPressed(unsigned char id)
 * @ 输入: unsigned char: 按键编号，1或2
 * @ 输出: bool: 按键当前处于按下状态返回true，否则返回false
 * @ 说明: 直接读取指定按键的当前电平状态，判断是否处于按下状态
 ********/
bool Button_IsPressed(unsigned char id)
{
    if (id == 1)
        return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET;
    if (id == 2)
        return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == GPIO_PIN_RESET;
    // return false;
    // 不能加入return false;  HAL_GPIO_ReadPin的返回值是SET和RESET，如果返回false会被误判为按键未按下
}
