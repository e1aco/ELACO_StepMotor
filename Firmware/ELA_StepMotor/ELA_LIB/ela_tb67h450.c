/********
 * @ 文件: ela_tb67h450.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-17
 * @ 版本: 1.0.0
 * @ 说明: TB67H450 步进电机 FOC 驱动
 ********/

#include "ela_tb67h450.h"
#include "ela_tb67h450_drv.h"
#include "tim.h"

TB67H450_CURRENT_T g_tb67h450_a_st; /* A相电流结构体 */
TB67H450_CURRENT_T g_tb67h450_b_st; /* B相电流结构体 */

/* ela_tb67h450 hlp start */

/********
 * @ 输入: value: 需要取绝对值的有符号数
 * @ 输出: 绝对值
 * @ 说明: 计算 short 类型绝对值
 ********/
static short tb67h450_abs(short value)
{
    return (value > 0) ? value : -value;
}

/* ela_tb67h450 hlp end */
//----------------------------------------------------------------------------------
/* ela_tb67h450 usr start */

/********
 * @ 输入: direction: 电流矢量方向 (0 ~ 1023)
 *         current_ma: 电流幅值 (mA)
 * @ 说明: 设置 FOC 电流矢量，查正弦表并计算
 *         PWM 占空比后输出
 ********/
void ela_tb67h450_set_foc_current(unsigned short direction,
                                  short current_ma)
{
    unsigned short dac_reg;
    short current_abs;
    short abs_a;
    short abs_b;

    /* 1. 计算A/B相的正弦表指针 */
    /* 1024个位置对应0-360度，取模1024得到指针 */
    g_tb67h450_b_st.sin_map_ptr =
        direction & SINE_MASK;
    g_tb67h450_a_st.sin_map_ptr =
        (g_tb67h450_b_st.sin_map_ptr + 256) & SINE_MASK;

    /* 2. 查正弦表 */
    g_tb67h450_a_st.sin_map_data =
        sin_form[g_tb67h450_a_st.sin_map_ptr];
    g_tb67h450_b_st.sin_map_data =
        sin_form[g_tb67h450_b_st.sin_map_ptr];

    /* 3. 计算DAC值: 电流(mA) 转 DAC值(0-4095) */
    /* DAC = 电流 × (4095/3300) ≈ 电流 × 1.24 */
    /* 5083 >> 12 ≈ 1.24，先乘再移位避免浮点 */
    current_abs = tb67h450_abs(current_ma);
    dac_reg = (unsigned short)((current_abs * DAC_SCALE_FACTOR) >> SIN_SCALE);
    dac_reg = dac_reg & DAC_MASK;

    /* 取绝对值再乘 */
    abs_a = tb67h450_abs(g_tb67h450_a_st.sin_map_data);
    abs_b = tb67h450_abs(g_tb67h450_b_st.sin_map_data);

    /* 电流值乘以sin/cos */
    g_tb67h450_a_st.dac_value =
        (unsigned short)((dac_reg * abs_a) >> SIN_SCALE);
    g_tb67h450_b_st.dac_value =
        (unsigned short)((dac_reg * abs_b) >> SIN_SCALE);

    /* 4. 设置PWM占空比，控制电流大小 */
    tb67h450_drv_set_two_coils_current(
        g_tb67h450_a_st.dac_value,
        g_tb67h450_b_st.dac_value);

    /* 5. 设置方向引脚 */
    if (g_tb67h450_a_st.sin_map_data > 0)
    {
        tb67h450_drv_set_dire_a(true, false);
    }
    else if (g_tb67h450_a_st.sin_map_data < 0)
    {
        tb67h450_drv_set_dire_a(false, true);
    }
    else
    {
        tb67h450_drv_set_dire_a(true, true);
    }

    if (g_tb67h450_b_st.sin_map_data > 0)
    {
        tb67h450_drv_set_dire_b(true, false);
    }
    else if (g_tb67h450_b_st.sin_map_data < 0)
    {
        tb67h450_drv_set_dire_b(false, true);
    }
    else
    {
        tb67h450_drv_set_dire_b(true, true);
    }
}

/********
 * @ 说明: 刹车模式，两相通入相同电流
 ********/
void ela_tb67h450_brake(void)
{
    tb67h450_drv_set_two_coils_current(0, 0);
    tb67h450_drv_set_dire_a(true, true);
    tb67h450_drv_set_dire_b(true, true);
}

/********
 * @ 说明: 休眠模式，两相断开
 ********/
void ela_tb67h450_sleep(void)
{
    tb67h450_drv_set_two_coils_current(0, 0);
    tb67h450_drv_set_dire_a(false, false);
    tb67h450_drv_set_dire_b(false, false);
}

/* ela_tb67h450 usr end */

