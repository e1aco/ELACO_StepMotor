#include "ela_tb67h450.h"

TB67H450_CURRENT_T gov_tb67h450_a_st; // A相电流获取结构体结果
TB67H450_CURRENT_T gov_tb67h450_b_st; // B相电流获取结构体结果

//----------------------------------------------------------
/* -1- 辅助函数 */

/****
 * @ 原型: void TB67H450_SetTwoCoilsCurrent(unsigned short currentA, unsigned short currentB)  
 * @ 输入: unsigned short currentA: 线圈A的电流值; unsigned short currentB: 线圈B的电流值
 * @ 输出: void
 * @ 说明: 设置两个线圈的电流值
 ********/
static void TB67H450_SetTwoCoilsCurrent(unsigned short currentA, unsigned short currentB)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, currentA);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, currentB);
}

/****
 * @ 原型: static void TB67H450_SetDire_A(bool statusAp, bool statusAm) 
 * @ 输入: bool statusAp: A相正极状态; bool statusAm: A相负极状态
 * @ 输出: void
 * @ 说明: 设置A相输入状态，根据statusAp和statusAm的值控制AP和AM引脚的电平，实现A相的正反转控制
 ********/
static void TB67H450_SetDire_A(bool statusAp, bool statusAm)
{
    if (statusAp) {
        AP_High();
    } 
    else 
    {
        AP_Low();
    }
    
    if (statusAm) {
        AM_High();
    } 
    else 
    {
        AM_Low();
    }
}

/****
 * @ 原型: static void TB67H450_SetDire_B(bool statusBp, bool statusBm)
 * @ 输入: bool statusBp: B相正极状态; bool statusBm: B相负极状态
 * @ 输出: void
 * @ 说明: 设置B相输入状态，根据statusBp和statusBm的值控制BP和BM引脚的电平，实现B相的正反转控制
 ********/
static void TB67H450_SetDire_B(bool statusBp, bool statusBm)
{
    if (statusBp) {
        BP_High();
    } else {
        BP_Low();
    }
    
    if (statusBm) {
        BM_High();
    } else {
        BM_Low();
    }
}

//-------------------------------------------------------------------------
/* -2- 用户调用函数 */

void TB67H450_SetFocCurrentVector(uint32_t directionInCount, int32_t current_mA)
{
    uint32_t dac_reg;
    int32_t current_abs;
    
    /* 1. 计算A/B相的正弦表指针 */
    gov_tb67h450_b_st.sinMapPtr = directionInCount & 0x000003FF; // 1024个位置对应0-360度，取模1024得到指针
    gov_tb67h450_a_st.sinMapPtr = (gov_tb67h450_a_st.sinMapPtr + 256) & 0x000003FF;
    
    /* 2. 查正弦表 */
    gov_tb67h450_a_st.sinMapData = sin_form[gov_tb67h450_a_st.sinMapPtr];
    gov_tb67h450_b_st.sinMapData = sin_form[gov_tb67h450_b_st.sinMapPtr];
    
    /* 3. 计算DAC值 */
    current_abs = (current_mA > 0) ? current_mA : -current_mA;
		// 电流(mA) 转 DAC值 (0-4095)
		// 公式: DAC = 电流 × (4095 / 3300) ≈ 电流 × 1.24
		// 5083 >> 12 = 5083 / 4096 ≈ 1.24
    dac_reg = (uint32_t)(current_abs * 5083) >> SIN_SCALE; // 先乘以放大倍数再右移，避免浮点运算,这个5083是凑出来的,要求是1.24,1.24*4096=5083
    dac_reg = dac_reg & 0x00000FFF; // 取低12位
    
    /* 取绝对值再乘 */
    int16_t absA = (gov_tb67h450_a_st.sinMapData > 0) ? gov_tb67h450_a_st.sinMapData : -gov_tb67h450_a_st.sinMapData;
    int16_t absB = (gov_tb67h450_b_st.sinMapData > 0) ? gov_tb67h450_b_st.sinMapData : -gov_tb67h450_b_st.sinMapData;
    
    gov_tb67h450_a_st.dacValue12Bits = (uint32_t)(dac_reg * absA) >> SIN_SCALE; // 电流值乘以sin
    gov_tb67h450_b_st.dacValue12Bits = (uint32_t)(dac_reg * absB) >> SIN_SCALE; // 电流值乘以cos
    
    /* 4. 设置PWM占空比 */
    TB67H450_SetTwoCoilsCurrent(gov_tb67h450_a_st.dacValue12Bits, gov_tb67h450_b_st.dacValue12Bits); // 设置PWM占空比，控制电流大小
    
    /* 5. 设置方向引脚 */
    if (gov_tb67h450_a_st.sinMapData > 0) {
        TB67H450_SetDire_A(true, false);
    } else if (gov_tb67h450_a_st.sinMapData < 0) {
        TB67H450_SetDire_A(false, true);
    } else {
        TB67H450_SetDire_A(true, true);
    }
    
    if (gov_tb67h450_b_st.sinMapData > 0) {
        TB67H450_SetDire_B(true, false);
    } else if (gov_tb67h450_b_st.sinMapData < 0) {
        TB67H450_SetDire_B(false, true);
    } else {
        TB67H450_SetDire_B(true, true);
    }
}

void TB67H450_Brake(void)
{
    TB67H450_SetTwoCoilsCurrent(0, 0); // 设置PWM占空比为0，停止电流输出s
    TB67H450_SetDire_A(true, true); // 刹车
    TB67H450_SetDire_B(true, true);
}

void TB67H450_Sleep(void)
{
    TB67H450_SetTwoCoilsCurrent(0, 0); // 设置PWM占空比为0，停止电流输出
    TB67H450_SetDire_A(false, false); // 刹车
    TB67H450_SetDire_B(false, false);
}
