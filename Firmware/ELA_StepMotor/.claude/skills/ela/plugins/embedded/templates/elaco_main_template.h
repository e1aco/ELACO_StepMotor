/****
 * @ 文件: elaco_main.h
 * @ 作者: ELACO
 * @ 日期: {{DATE}}
 * @ 版本: 1.0.0
 * @ 说明: 应用主入口头文件，中央包含头
 ********/

#ifndef ELACO_MAIN_H
#define ELACO_MAIN_H

#include "stdbool.h"
#include "string.h"
#include "stdlib.h"

/* 用户模块头文件将自动添加在此处 */

/* 芯片相关头文件（根据项目配置自动调整） */
#include "stm32f1xx_hal.h"
#include "main.h"
#include "usart.h"

extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_usart3_tx;

void elaco_main(void);

#endif