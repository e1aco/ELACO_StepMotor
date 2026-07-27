/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "led.h"
#include "button.h"
#include "motor.h"
#include "tb67h450.h"
#include "mt6816.h"
#include "encoder_calibrator.h"
#include "stockpile_f103cb.h"
#include "configurations.h"
#include "uart_cmd.h"
#include "eeprom.h"
#include <stdio.h>
#include <string.h>
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
/* 板卡配置变量 */
BoardConfig_t boardConfig;
/* 电机配置 */
static Motor_Config_t motor_config;
/* 默认节点ID */
static uint16_t defaultNodeID = 1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Tim1Callback100Hz(void);
void Tim4Callback20kHz(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_USART1_UART_Init();
  MX_CAN_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
  /* 1. 初始化LED */
  LED_Init();
  /* 2. 初始化按钮 */
  Button_Init();
  /* 3. 初始化编码器校准模块 */
  EncoderCalibrator_Init();
	/* 4. 从 EEPROM 读取配置 */
  EEPROM_Read(0, &boardConfig, sizeof(BoardConfig_t));
	/* 5. 检查配置是否有效 */
	if (boardConfig.configStatus != CONFIG_OK) {
			/* 配置无效，使用默认值 */
			boardConfig.configStatus = CONFIG_OK;
			boardConfig.canNodeId = defaultNodeID;
			boardConfig.encoderHomeOffset = 0;
			boardConfig.defaultMode = MODE_COMMAND_POSITION;
			boardConfig.currentLimit = 1000;
			boardConfig.velocityLimit = 30 * MOTOR_SUBDIVIDE_STEPS;
			boardConfig.velocityAcc = 100 * MOTOR_SUBDIVIDE_STEPS;
			boardConfig.calibrationCurrent = 2000;
			boardConfig.dce_kp = 200;
			boardConfig.dce_kv = 80;
			boardConfig.dce_ki = 300;
			boardConfig.dce_kd = 250;
			boardConfig.pid_kp = 5;
			boardConfig.pid_ki = 30;
			boardConfig.pid_kd = 0;
			boardConfig.enableMotorOnBoot = false;
			boardConfig.enableStallProtect = false;
			
			/* 保存到 EEPROM */
			EEPROM_Write(0, &boardConfig, sizeof(BoardConfig_t));
	}  
  
  /* 6. 配置电机参数 */
  motor_config.motionParams.encoderHomeOffset = boardConfig.encoderHomeOffset;
  motor_config.motionParams.ratedCurrent = boardConfig.currentLimit;
  motor_config.motionParams.ratedVelocity = boardConfig.velocityLimit;
  motor_config.motionParams.ratedVelocityAcc = boardConfig.velocityAcc;
  motor_config.motionParams.ratedCurrentAcc = 2000;
  motor_config.motionParams.caliCurrent = boardConfig.calibrationCurrent;
  
  motor_config.ctrlParams.dce.kp = boardConfig.dce_kp;
  motor_config.ctrlParams.dce.kv = boardConfig.dce_kv;
  motor_config.ctrlParams.dce.ki = boardConfig.dce_ki;
  motor_config.ctrlParams.dce.kd = boardConfig.dce_kd;
	motor_config.ctrlParams.pid.kp = boardConfig.pid_kp;
	motor_config.ctrlParams.pid.ki = boardConfig.pid_ki;
	motor_config.ctrlParams.pid.kd = boardConfig.pid_kd;
  motor_config.ctrlParams.stallProtectSwitch = boardConfig.enableStallProtect;
  
  /* 7. 初始化电机 */
	Motor_SetConfig(&motor_config);
  Motor_Init();
  
	
  /* 8. 启动定时器 */
  HAL_Delay(100);
  HAL_TIM_Base_Start_IT(&htim1);  // 100Hz
  HAL_TIM_Base_Start_IT(&htim4);  // 20kHz
	
  /* 9. 检查是否触发校准（两个按钮同时按下）*/
  if (Button_IsPressed(1) && Button_IsPressed(2)) {
      EncoderCalibrator_Trigger();
  }
	Uart_SendString("System Start!\r\n");
  /* USER CODE END 2 */
	
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    /* 处理校准计算 */
    EncoderCalibrator_TickMainLoop();
		
		/* ==================== 处理配置保存/恢复 ==================== */
    if (boardConfig.configStatus == CONFIG_COMMIT) {
        /* 保存配置 */
        boardConfig.configStatus = CONFIG_OK;
        
        /* 擦除 Flash 分区 */
        Stockpile_Flash_Data_Empty(&stockpile_data);
        
        /* 写入配置 */
        EEPROM_Write(0, &boardConfig, sizeof(BoardConfig_t));
    } 
    else if (boardConfig.configStatus == CONFIG_RESTORE) {
        /* 保存配置并复位 */
        EEPROM_Write(0, &boardConfig, sizeof(BoardConfig_t));
        HAL_NVIC_SystemReset();
    }
    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
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
/* HAL 定时器周期中断回调 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1) {
        Tim1Callback100Hz();
    }
    else if (htim->Instance == TIM4) {
        Tim4Callback20kHz();
    }
}

/* 100Hz 定时器回调 */
void Tim1Callback100Hz(void)
{
    /* 清除中断标志 */
    __HAL_TIM_CLEAR_IT(&htim1, TIM_IT_UPDATE);
    
    /* 按钮扫描（10ms 一次）*/
    Button_Tick(); 
    
    // 处理按钮事件
    if (Button_GetClick(1)) {
        // 切换运行/停止
        if (Motor_GetMode() != MODE_STOP) {
            Motor_SetMode(MODE_STOP);
					  Uart_SendString("MODE_STOP\r\n");
        } else {
            Motor_SetMode(MODE_COMMAND_POSITION);
					  Uart_SendString("MODE_COMMAND_POSITION\r\n");
        }
    }
    if (Button_GetLong(1)) {
        Uart_SendString("HAL_NVIC_SystemReset\r\n");
        // 复位
        HAL_NVIC_SystemReset();
    }
    
    if (Button_GetClick(2)) {
        Uart_SendString("Motor_ClearStallFlag\r\n");
        Motor_ClearStallFlag();
    }
    if (Button_GetLong(2)) {
        Uart_SendString("Motor_Stop\r\n");
        Motor_SetPosition(0);
        Motor_SetVelocity(0);
        Motor_SetCurrent(0);
    }
    
    /* LED 状态更新 */
    LED_Tick(10, Motor_GetState());
		
		/* 上位机数据发送 */
    static uint8_t tele_cnt = 0;
    if (++tele_cnt >= 10) {
        tele_cnt = 0;
        float pos = Motor_GetPosition(false);
        float vel = Motor_GetVelocity();
        float cur = Motor_GetCurrent();
        uint8_t mode = Motor_GetMode();
        uint8_t state = Motor_GetState();
        printf("T:%.2f,%.2f,%.2f,%d,%d\r\n", pos, vel, cur, mode, state);
    }
}

/* 20kHz 定时器回调 */
void Tim4Callback20kHz(void)
{
    /* 清除中断标志 */
    __HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);
    
    /* 校准模式 vs 正常运行模式 */
    if (EncoderCalibrator_IsTriggered()) {
        EncoderCalibrator_Tick20kHz();   // 校准状态机
    } else {
        Motor_Tick20kHz();                // 电机控制
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

#ifdef  USE_FULL_ASSERT
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
