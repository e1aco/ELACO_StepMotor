#include "tb67h450.h"
#include "sin_form.h"
#include "tim.h"
#include "gpio.h"
#include "main.h"

/* 相位数据结构 */
typedef struct {
    uint16_t sinMapPtr;
    int16_t sinMapData;
    uint16_t dacValue12Bits;
} Phase_t;

/* 静态变量 */
static Phase_t s_phaseA;
static Phase_t s_phaseB;

/* 设置两相电流 */
static void TB67H450_SetTwoCoilsCurrent(uint16_t currentA, uint16_t currentB)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, currentA >> 2);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, currentB >> 2);
}

/* 设置A相输入 */
static void TB67H450_SetInputA(bool statusAp, bool statusAm)
{
    if (statusAp) {
        HAL_GPIO_WritePin(GPIOA, HW_ELEC_AP_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(GPIOA, HW_ELEC_AP_Pin, GPIO_PIN_RESET);
    }
    
    if (statusAm) {
        HAL_GPIO_WritePin(GPIOA, HW_ELEC_AM_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(GPIOA, HW_ELEC_AM_Pin, GPIO_PIN_RESET);
    }
}

/* 设置B相输入 */
static void TB67H450_SetInputB(bool statusBp, bool statusBm)
{
    if (statusBp) {
        HAL_GPIO_WritePin(GPIOA, HW_ELEC_BP_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(GPIOA, HW_ELEC_BP_Pin, GPIO_PIN_RESET);
    }
    
    if (statusBm) {
        HAL_GPIO_WritePin(GPIOA, HW_ELEC_BM_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(GPIOA, HW_ELEC_BM_Pin, GPIO_PIN_RESET);
    }
}

void TB67H450_SetFocCurrentVector(uint32_t _directionInCount, int32_t _current_mA)
{
    uint32_t dac_reg;
    int32_t current_abs;
    
    /* 1. 计算A/B相的正弦表指针 */
    s_phaseB.sinMapPtr = _directionInCount & 0x000003FF;
    s_phaseA.sinMapPtr = (s_phaseB.sinMapPtr + 256) & 0x000003FF;
    
    /* 2. 查正弦表 */
    s_phaseA.sinMapData = sin_pi_m2[s_phaseA.sinMapPtr];
    s_phaseB.sinMapData = sin_pi_m2[s_phaseB.sinMapPtr];
    
    /* 3. 计算DAC值 */
    current_abs = (_current_mA > 0) ? _current_mA : -_current_mA;
		// 电流(mA) 转 DAC值 (0-4095)
		// 公式: DAC = 电流 × (4095 / 3300) ≈ 电流 × 1.24
		// 5083 >> 12 = 5083 / 4096 ≈ 1.24
    dac_reg = (uint32_t)(current_abs * 5083) >> 12;
    dac_reg = dac_reg & 0x00000FFF;
    
    /* 取绝对值再乘 */
    int16_t absA = (s_phaseA.sinMapData > 0) ? s_phaseA.sinMapData : -s_phaseA.sinMapData;
    int16_t absB = (s_phaseB.sinMapData > 0) ? s_phaseB.sinMapData : -s_phaseB.sinMapData;
    
    s_phaseA.dacValue12Bits = (uint32_t)(dac_reg * absA) >> sin_pi_m2_dpiybit;
    s_phaseB.dacValue12Bits = (uint32_t)(dac_reg * absB) >> sin_pi_m2_dpiybit;
    
    /* 4. 设置PWM占空比 */
    TB67H450_SetTwoCoilsCurrent(s_phaseA.dacValue12Bits, s_phaseB.dacValue12Bits);
    
    /* 5. 设置方向引脚 */
    if (s_phaseA.sinMapData > 0) {
        TB67H450_SetInputA(true, false);
    } else if (s_phaseA.sinMapData < 0) {
        TB67H450_SetInputA(false, true);
    } else {
        TB67H450_SetInputA(true, true);
    }
    
    if (s_phaseB.sinMapData > 0) {
        TB67H450_SetInputB(true, false);
    } else if (s_phaseB.sinMapData < 0) {
        TB67H450_SetInputB(false, true);
    } else {
        TB67H450_SetInputB(true, true);
    }
}

void TB67H450_Sleep(void)
{
    TB67H450_SetTwoCoilsCurrent(0, 0);
    TB67H450_SetInputA(false, false);
    TB67H450_SetInputB(false, false);
}

void TB67H450_Brake(void)
{
    TB67H450_SetTwoCoilsCurrent(0, 0);
    TB67H450_SetInputA(true, true);
    TB67H450_SetInputB(true, true);
}

