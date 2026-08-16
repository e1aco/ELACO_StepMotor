/*****************************************************************************
 * @文件: tim_test.c
 * @作者: cl
 * @日期: 2026-08-15
 * @版本: v1.0
 * @说明: 时序测量测试层（/cl tim 板端测量驱动，量产可剔除）
 *        时间基 DWT CYCCNT @72MHz（÷72 → µs），32 位回绕 59.6s 测量窗口内不越界
 *        SysTick 优先级临时提到最高（NVIC_SetPriority(SysTick_IRQn,0)），
 *        不影响 HAL_GetTick 计数（SysTick 中断体仅 +1）
 *        单测隔离：每轮只激活 TM_ACTIVE 一条，其余探针 O(1) 直接返回
 * @平台: STM32F103RET6
 * @依赖: HAL_DWT(CoreDebug), uart_drv(回传), adc.h(T4)
 ****************************************************************************/
#ifdef CL_TIMING_MEASURE

#include "tim_test.h"
#include "uart_drv.h"
#include "adc.h"
#include "main.h"
#include <stdio.h>

/* ==== 常量定义 ==== */
#define TM_ACTIVE        TM_TIM4_ISR     /* 本轮被测条目（/cl tim 逐条修改） */
#define TM_CYCCNT_PER_US 72U          /* 72MHz 周期计数 → µs */
#define TM_REPORT_MAX    1000U        /* 20kHz/读函数类回传阈值（50ms 一轮） */
#define TM_REPORT_100HZ  100U         /* 100Hz 类回传阈值（1s 一轮） */
#define TM_ADC_TRIGGER   1000U        /* 主循环触发次数间隔（约 10ms@20kHz 主循环） */

/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
static volatile uint32_t s_start[4];
static volatile uint32_t s_max_us[4];
static volatile uint32_t s_cnt[4];
static volatile uint32_t s_pending[4];
static volatile uint32_t s_adc_loop_cnt = 0;

/* ==== 内部工具 ==== */
/**
 * @输入 无
 * @输出 当前时刻（µs，DWT CYCCNT 单调计数）
 * @说明 时间基：DWT->CYCCNT 周期计数 ÷72 → µs
 * 依据 .cl/memory/ clock_sysclk=72MHz（HSE 8MHz × PLLMUL=9）
 */
static uint32_t S_GetUs(void)
{
    return DWT->CYCCNT / TM_CYCCNT_PER_US;
}

/**
 * @输入 tag: 被测条目
 * @输出 条目名称（回传用）
 * @说明 名称映射表
 */
static const char *S_TagName(TM_Tag tag)
{
    switch (tag)
    {
    case TM_TIM4_ISR:    return "TIM4_ISR";
    case TM_TIM1_ISR:    return "TIM1_ISR";
    case TM_MT6816_READ: return "MT6816_READ";
    case TM_ADC_SAMPLE:  return "ADC_SAMPLE";
    default:             return "UNKNOWN";
    }
}

/* ==== 接口实现 ==== */
/**
 * @输入 无
 * @输出 无
 * @说明 测量初始化：开 DWT CYCCNT + SysTick 优先级提到最高（用户确认临时提高）
 */
void TEST_TIM_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    NVIC_SetPriority(SysTick_IRQn, 0);   /* 0=最高（Cortex-M3 数值越小优先级越高） */
}

/**
 * @输入 tag: 被测条目
 * @输出 无
 * @说明 测量起点（ISR 入口/函数入口调用；非激活条目直接返回）
 */
void TEST_TIM_Start(TM_Tag tag)
{
    if ((uint32_t)tag != TM_ACTIVE)
    {
        return;
    }
    s_start[tag] = S_GetUs();
}

/**
 * @输入 tag: 被测条目
 * @输出 无
 * @说明 测量终点：累加最大耗时，达回传阈值置 pending（主循环统一回传，避免
 *        测量窗口内打印自污染）
 */
void TEST_TIM_Stop(TM_Tag tag)
{
    uint32_t delta;
    uint32_t threshold;

    if ((uint32_t)tag != TM_ACTIVE)
    {
        return;
    }
    delta = S_GetUs() - s_start[tag];
    if (delta > s_max_us[tag])
    {
        s_max_us[tag] = delta;
    }
    s_cnt[tag]++;
    threshold = (TM_TIM1_ISR == tag) ? TM_REPORT_100HZ : TM_REPORT_MAX;
    if (s_cnt[tag] >= threshold)
    {
        s_pending[tag] = 1;
    }
}

/**
 * @输入 无
 * @输出 无
 * @说明 主循环统一回传：[TM] <tag> <max_us> us <周期数>，同 tag 多行取最大=最坏
 */
void TEST_TIM_Report(void)
{
    uint32_t i;
    char buf[48];

    for (i = 0; i < 4U; i++)
    {
        if (s_pending[i])
        {
            s_pending[i] = 0;
            sprintf(buf, "[TM] %s %lu us %lu\r\n",
                    S_TagName((TM_Tag)i),
                    (unsigned long)s_max_us[i],
                    (unsigned long)s_cnt[i]);
            DRV_Uart_SendString(buf);
            s_max_us[i] = 0;
            s_cnt[i] = 0;
        }
    }
}

/**
 * @输入 无
 * @输出 无
 * @说明 T4 测试序列：主循环节流触发 ADC 采样（Start+Poll 全路径）。
 *        业务暂未接入（供电监测未用），实测为 CubeMX 配置路径耗时
 * 依据 .cl/memory/ clock_adc=12MHz + adc.c MX_ADC1_Init（SamplingTime=1.5cycles）
 */
void TEST_TIM_AdcSampleTask(void)
{
    uint32_t adc_value;

    if (++s_adc_loop_cnt < TM_ADC_TRIGGER)
    {
        return;
    }
    s_adc_loop_cnt = 0;

    TEST_TIM_Start(TM_ADC_SAMPLE);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    adc_value = HAL_ADC_GetValue(&hadc1);
    (void)adc_value;
    TEST_TIM_Stop(TM_ADC_SAMPLE);
}

#endif /* CL_TIMING_MEASURE */
