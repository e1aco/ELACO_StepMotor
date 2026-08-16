/*****************************************************************************
 * @文件: tim_test.h
 * @作者: cl
 * @日期: 2026-08-15
 * @版本: v1.0
 * @说明: 时序测量测试层（/cl tim 板端测量驱动，量产可剔除）
 *        探针宏双版本：CL_TIMING_MEASURE 定义时有效，生产版空宏零开销
 * @平台: STM32F103RET6
 * @依赖: 无（生产版）
 ****************************************************************************/
#ifndef TIM_TEST_H
#define TIM_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ==== 常量定义 ==== */
/* ==== 类型定义 ==== */
#ifdef CL_TIMING_MEASURE
typedef enum
{
    TM_TIM4_ISR,    /* T1: TIM4 20kHz 电机闭环 ISR 总耗时 */
    TM_TIM1_ISR,    /* T2: TIM1 100Hz 遥测 ISR 总耗时 */
    TM_MT6816_READ, /* T3: MT6816 SPI 单次读（含重试） */
    TM_ADC_SAMPLE,  /* T4: ADC1 编码器供电采样 */
} TM_Tag;
#endif

/* ==== 全局实例 ==== */
/* ==== 接口 ==== */
#ifdef CL_TIMING_MEASURE
void TEST_TIM_Init(void);
void TEST_TIM_Start(TM_Tag tag);
void TEST_TIM_Stop(TM_Tag tag);
void TEST_TIM_Report(void);
void TEST_TIM_AdcSampleTask(void);
#define TEST_TM_START(tag) TEST_TIM_Start(TM_##tag)
#define TEST_TM_STOP(tag)  TEST_TIM_Stop(TM_##tag)
#define TEST_TM_TOGGLE()   TEST_TIM_Toggle()
#else
#define TEST_TIM_Init()          ((void)0)
#define TEST_TIM_Report()        ((void)0)
#define TEST_TIM_AdcSampleTask() ((void)0)
#define TEST_TM_START(tag) ((void)0)
#define TEST_TM_STOP(tag)  ((void)0)
#define TEST_TM_TOGGLE()   ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* TIM_TEST_H */
