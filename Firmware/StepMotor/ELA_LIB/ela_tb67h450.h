#ifndef _ELA_TB67H450_H_
#define _ELA_TB67H450_H_

#include "stdbool.h" // C库
#include "stdint.h"

#include "elaco_main.h" // HAL库
#include "ela_sinform.h" // 正弦表头文件

/* -- 驱动电流获取结构体 */ 
typedef struct {
    unsigned short sinMapPtr;
    short sinMapData;
    unsigned short dacValue12Bits;
} TB67H450_CURRENT_T;

extern TB67H450_CURRENT_T gov_tb67h450_a_st; // A相电流获取结构体
extern TB67H450_CURRENT_T gov_tb67h450_b_st; // B相电流获取结构体

/* -1- 电机驱动引脚控制宏定义 */ 
#define AP_High()   HAL_GPIO_WritePin(GPIOA, HW_ELEC_AP_Pin, GPIO_PIN_SET);
#define AP_Low()    HAL_GPIO_WritePin(GPIOA, HW_ELEC_AP_Pin, GPIO_PIN_RESET);
#define AM_High()   HAL_GPIO_WritePin(GPIOA, HW_ELEC_AM_Pin, GPIO_PIN_SET);
#define AM_Low()    HAL_GPIO_WritePin(GPIOA, HW_ELEC_AM_Pin, GPIO_PIN_RESET);
#define BP_High()   HAL_GPIO_WritePin(GPIOA, HW_ELEC_BP_Pin, GPIO_PIN_SET);
#define BP_Low()    HAL_GPIO_WritePin(GPIOA, HW_ELEC_BP_Pin, GPIO_PIN_RESET);
#define BM_High()   HAL_GPIO_WritePin(GPIOA, HW_ELEC_BM_Pin, GPIO_PIN_SET);
#define BM_Low()    HAL_GPIO_WritePin(GPIOA, HW_ELEC_BM_Pin, GPIO_PIN_RESET);
#define EN_High()   HAL_GPIO_WritePin(GPIOA, HW_ELEC_EN_Pin, GPIO_PIN_SET);
#define EN_Low()    HAL_GPIO_WritePin(GPIOA, HW_ELEC_EN_Pin, GPIO_PIN_RESET);

/* -2- 函数定义 */ 
static void TB67H450_SetDire_A(bool statusAp, bool statusAm);
static void TB67H450_SetDire_B(bool statusBp, bool statusBm);
void TB67H450_SetFocCurrentVector(uint32_t directionInCount, int32_t current_mA);
void TB67H450_Brake(void);
void TB67H450_Sleep(void);

#endif
