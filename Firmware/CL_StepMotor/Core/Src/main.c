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
#include "tb67h450_usr.h"
#include "mt6816_usr.h"
#include "encoder_calibrator_usr.h"
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
  /* 1. LED 状态指示初始化 */
  DRV_LED_Init();
  /* 2. 点亮 LED1 证明系统活着 */
  DRV_LED_Set(DRV_LED1, true);
  /* 3. 调试回传打印启动信息 */
  DRV_Uart_SendString("System Start!\r\n");
  /* 4. TB67H450 驱动初始化：启动 TIM2 PWM(CH3/CH4) + 方向脚全低(不励磁) */
  USR_TB67H450_Init();
  /* 5. MT6816 编码器初始化：读一次角度 */
  USR_MT6816_Init();
  /* 6. 编码器校准初始化：读 Flash 判断是否已有有效校准表 */
  USR_EncoderCalibrator_Init();
  /* 7. 固定小电流演示：电角度 128(45°) @ 200mA，示波器量 PB10/PB11 占空比 */
  /*    警告：电机已接时产生保持力矩，确保轴自由旋转、供电正常 */
  USR_TB67H450_SetFocCurrentVector(128U, 200);
  /* 8. 启动定时中断：TIM1=100Hz(心跳/慢速任务)、TIM4=20kHz(电机控制 tick) */
  HAL_TIM_Base_Start_IT(&htim1);
  HAL_TIM_Base_Start_IT(&htim4);
  DRV_Uart_SendString("Timer tick started!\r\n");
  /* 9. 上电同按 SW1+SW2 触发编码器校准（复刻参考 main.c Button_IsPressed 双键逻辑） */
  if ((HAL_GPIO_ReadPin(SW1_GPIO_Port, SW1_Pin) == GPIO_PIN_RESET) &&
      (HAL_GPIO_ReadPin(SW2_GPIO_Port, SW2_Pin) == GPIO_PIN_RESET))
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
  * in the RCC_OscInitStruct structure.
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
        /* 100Hz 心跳：LED1 翻转 + 每秒打印 20kHz 计数证明双 tick 在跑 */
        if (++s_tick_100hz_cnt >= 100U)
        {
            s_tick_100hz_cnt = 0U;
            DRV_LED_Set(DRV_LED1, (s_tick_20khz_cnt & 1U) ? true : false);
            char buf[32];
            sprintf(buf, "20kHz=%lu\r\n", (unsigned long)s_tick_20khz_cnt);
            DRV_Uart_SendString(buf);
        }
    }
    else if (TIM4 == htim->Instance)
    {
        /* 20kHz 电机控制 tick：驱动编码器校准状态机；电机闭环后续接入 */
        s_tick_20khz_cnt++;
        USR_EncoderCalibrator_Tick20kHz();
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
  * @param  line: assert_param error source line number
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
