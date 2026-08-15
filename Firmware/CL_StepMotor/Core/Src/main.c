/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "can.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "led_drv.h"
#include "uart_drv.h"
#include "button_usr.h"
#include "tb67h450_usr.h"
#include "mt6816_usr.h"
#include "encoder_calibrator_usr.h"
#include "motor_usr.h"
#include "tim_test.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* 电机配置：默认值按 .cl/memory/ 推导（config_usr 未建，先硬编码默认，任务4 后接 EEPROM） */
static Motor_Config_T s_motor_config = {
    .encoderHomeOffset = 0,
    .ratedCurrent = 2000,                          /* 电流限幅 2000mA（42 步进额定 2A） */
    .ratedVelocity = 2 * USR_MOTOR_SUBDIVIDE_STEPS, /* 速度限幅 2圈/s（步进低速区间，防失步） */
    .posKp = 32768,                                /* 位置环增益 32（128 实测：减速窗口缩至 672 步<滑行 512→冲过目标；32→窗口 3072 步，MIN_VEL 已解死区边缘拉锯） */
    .pidKp = 10,                                   /* 速度环增益 10（5 实测堵转输出上限 636mA 推不动齿隙摩擦→卡死；10→1270mA 冲破；MIN_VEL 防假速度拉锯） */
    .pidKd = 400,                                  /* 速度环阻尼 400（8/13 已验证；800 实测饱和成 bang-bang 激励振荡） */
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* 定时心跳计数（中断回调内维护） */
static volatile uint32_t s_tick_20khz_cnt = 0;  /* 20kHz tick 计数 */
static uint32_t s_tick_100hz_cnt = 0;           /* 100Hz tick 计数 */
/* 遥测/按键事件消息缓冲：ISR 内格式化，主循环发送
 * （避免 ISR 内 sprintf 浮点 + 阻塞串口发送，T2 超预算调参 2026-08-15） */
static char s_tele_buf[96];
static volatile uint8_t s_tele_ready = 0;
static char s_key_buf[24];
static volatile uint8_t s_key_ready = 0;
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_CAN_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  /* 1. 时序测量初始化（宏版本；生产版空实现） */
  TEST_TIM_Init();
  /* 2. LED 状态指示初始化 */
  DRV_LED_Init();
  /* 2. 点亮 LED1 证明系统活着 */
  DRV_LED_Set(DRV_LED1, true);
  /* 3. 按键初始化：读取初始电平并清空事件 */
  USR_Button_Init();
  /* 4. 调试回传打印启动信息 */
  DRV_Uart_SendString("System Start!\r\n");
  /* 5. TB67H450 驱动初始化：启动 TIM2 PWM(CH3/CH4) + 方向脚全低(不励磁) */
  USR_TB67H450_Init();
  /* 6. MT6816 编码器初始化：读一次角度 */
  USR_MT6816_Init();
  /* 7. 编码器校准初始化：读 Flash 判断是否已有有效校准表 */
  USR_EncoderCalibrator_Init();
  /* 8. 电机闭环初始化：注入默认配置（电流限幅 1000mA / 速度限幅 30圈/s / P 环增益） */
  USR_Motor_SetConfig(&s_motor_config);
  USR_Motor_Init();
  /* 9. 启动定时中断：TIM1=100Hz(心跳/慢速任务)、TIM4=20kHz(电机控制 tick) */
  HAL_TIM_Base_Start_IT(&htim1);
  HAL_TIM_Base_Start_IT(&htim4);
  DRV_Uart_SendString("Timer tick started!\r\n");
  /* 10. 上电同按 SW1+SW2 触发编码器校准（复刻参考 main.c Button_IsPressed 双键逻辑） */
  if (USR_Button_IsPressed(USR_BUTTON1) && USR_Button_IsPressed(USR_BUTTON2))
  {
      USR_EncoderCalibrator_Trigger();
      DRV_Uart_SendString("Cali triggered!\r\n");
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* 时序测量回传 + T4 ADC 采样序列（宏版本；生产版空实现） */
    TEST_TIM_Report();
    TEST_TIM_AdcSampleTask();
    /* 按键事件/遥测消息发送（ISR 只格式化，发送在主循环） */
    if (s_key_ready)
    {
        s_key_ready = 0;
        DRV_Uart_SendString(s_key_buf);
    }
    if (s_tele_ready)
    {
        s_tele_ready = 0;
        DRV_Uart_SendString(s_tele_buf);
    }
    /* 编码器校准主循环任务（校准完成时校验数据并写 Flash） */
    USR_EncoderCalibrator_TickMainLoop();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
 * @输入 htim: 产生更新事件的定时器句柄
 * @输出 无
 * @说明 HAL 定时器周期中断回调：按实例分派 100Hz / 20kHz tick
 *   TIM1 周期=72MHz/(71+1)/(9999+1)=100Hz；TIM4=72MHz/(71+1)/(49+1)=20kHz
 * 依据 .cl/memory/ control_frequency=20000 + tim.c MX_TIM1_Init/MX_TIM4_Init
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (TIM1 == htim->Instance)
    {
        /* 100Hz 按键扫描：边沿检测 + 单击/长按事件 */
        USR_Button_Tick();

        /* 100Hz 按键测试：验证电机闭环（单击/长按事件驱动命令，测试段验收后删） */
        if (USR_Button_GetClick(USR_BUTTON1))
        {
            if (USR_Motor_GetMode() != MODE_STOP)
            {
                USR_Motor_SetMode(MODE_STOP);
                sprintf(s_key_buf, "MODE_STOP\r\n");
                s_key_ready = 1;
            }
            else
            {
                USR_Motor_SetMode(MODE_COMMAND_POSITION);
                sprintf(s_key_buf, "MODE_POSITION\r\n");
                s_key_ready = 1;
            }
        }
        /* 100Hz 按键测试：SW2 循环目标 90°→180°→270°→360°→90°（全程测试） */
        static uint8_t s_deg_idx = 3U;   /* 初值 3：首次按 SW2 → (3+1)%4=0 → 90° */
        if (USR_Button_GetClick(USR_BUTTON2))
        {
            s_deg_idx = (s_deg_idx + 1U) % 4U;
            USR_Motor_SetPosition((int32_t)12800 * (s_deg_idx + 1U));
            sprintf(s_key_buf, "POS_%u\r\n", (unsigned)(90U * (s_deg_idx + 1U)));
            s_key_ready = 1;
        }
        if (USR_Button_GetLong(USR_BUTTON2))
        {
            USR_Motor_SetPosition(0);
            USR_Motor_SetVelocity(0);
            USR_Motor_SetCurrent(0);
            sprintf(s_key_buf, "STOP\r\n");
            s_key_ready = 1;
        }
        if (USR_Button_GetLong(USR_BUTTON1))
        {
            DRV_Uart_SendString("RESET\r\n");
            HAL_NVIC_SystemReset();
        }

        /* 100Hz 遥测：位置/速度/电流/模式/状态（10 次=100ms 一次）
         * 定点化打印（×1000 拆整数/小数），避免软件浮点 %f（无 FPU 开销大） */
        static uint8_t s_tele_cnt = 0;
        if (++s_tele_cnt >= 10U)
        {
            s_tele_cnt = 0U;
            float pos, vel, cur;
            uint8_t mode, state;
            long p, v, c;
            unsigned raw, rec;
            long rp, rv, rd;
            USR_Motor_GetTelemetry(&pos, &vel, &cur, &mode, &state);
            p = (long)(pos * 1000.0f);
            v = (long)(vel * 1000.0f);
            c = (long)(cur * 1000.0f);
            raw = (unsigned)USR_MT6816_GetRawAngle();
            rec = (unsigned)USR_MT6816_GetRectifiedAngle();
            rp = (long)USR_Motor_GetRawPosition();
            rv = (long)USR_Motor_GetRawVelocity();
            rd = (long)USR_Motor_GetRawDelta();
            sprintf(s_tele_buf, "T:%ld.%03ld,%ld.%03ld,%ld.%03ld,%u,%u,%u,%u,%ld,%ld,%ld\r\n",
                    p / 1000, (p % 1000 < 0) ? -(p % 1000) : (p % 1000),
                    v / 1000, (v % 1000 < 0) ? -(v % 1000) : (v % 1000),
                    c / 1000, (c % 1000 < 0) ? -(c % 1000) : (c % 1000),
                    (unsigned)mode, (unsigned)state, raw, rec,
                    rp, rv, rd);
            s_tele_ready = 1;
        }

        /* 100Hz 心跳：LED1 翻转 */
        if (++s_tick_100hz_cnt >= 100U)
        {
            s_tick_100hz_cnt = 0U;
            DRV_LED_Set(DRV_LED1, (s_tick_20khz_cnt & 1U) ? true : false);
        }
    }
    else if (TIM4 == htim->Instance)
    {
        /* 20kHz 电机控制 tick：校准触发→校准状态机；否则→电机闭环 */
        s_tick_20khz_cnt++;
        if (USR_EncoderCalibrator_IsTriggered())
        {
            USR_EncoderCalibrator_Tick20kHz();
        }
        else
        {
            USR_Motor_Tick20kHz();
        }
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
