#ifndef _ELACO_MAIN_H_
#define _ELACO_MAIN_H_

#include "stdbool.h" // C库
#include "string.h"
#include "stdlib.h"

#include "ela_uart.h" // ELA库
#include "ela_queue.h"
#include "ela_button.h"
#include "ela_mt6816.h"
#include "ela_tb67h450.h"
#include "ela_stockfile.h"
#include "ela_motor.h"
#include "ela_cyclecal.h"
#include "elaco_calibration.h"

#include "stm32f1xx_hal.h" // HAL库
#include "main.h"
#include "usart.h"
#include "tim.h"
#include "spi.h" 


extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart1_rx;

void elaco_main(void);
void USER_UART_IRQHandler(UART_HandleTypeDef *huart);

#endif
