#include "elaco_main.h"

//-------------------------------------------------------------------
/* -1- 主循环函数部分 */

/****
 * @ 原型: void elaco_main(void)
 * @ 输入: void
 * @ 输出: void
 * @ 说明: 主循环函数，完成外设初始化后进入无限循环
 ********/
void elaco_main(void)
{
		/* ELA库函数初始化 */
    Button_Init(); // 初始化按键状态
    MT6816_Init(); // 初始化 MT6816 编码器
		
		/* 开启定时器中断 */
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE); // 开启 USART1 空闲中断
    HAL_UART_Receive_DMA(&huart1, gov_rx1_buf, DMA_BUF_SIZE); // 设置 DMA 接收
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3); // 启动 TIM2 通道3 PWM 输出
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4); // 启动 TIM2 通道4 PWM 输出
    HAL_TIM_Base_Start_IT(&htim4); // 启动 TIM4 20kHz 定时中断


    while (1)
    {
        //Button_Scan_Proc(); // 更新按键状态
        Cali_Table_Generate_Proc(); // 校准表生成进程（空闲时不执行操作）
        
    }
}

//-------------------------------------------------------------------
/* -2- 中断回调函数部分 */

/****
 * @ 原型: void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
 * @ 输入: UART_HandleTypeDef *huart: 串口句柄指针
 * @ 输出: void
 * @ 说明: 串口 DMA 接收半完成中断回调函数
 ********/
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        uint8_t Length = DMA_BUF_SIZE / 2 - gov_rx1_offset;
        HAL_UART_Transmit(huart, gov_rx1_buf+gov_rx1_offset, Length, HAL_MAX_DELAY);
        gov_rx1_offset += Length;
    }
}

/****
 * @ 原型: void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
 * @ 输入: UART_HandleTypeDef *huart: 串口句柄指针
 * @ 输出: void
 * @ 说明: 串口 DMA 接收完成中断回调函数
 ********/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
				uint8_t Length = DMA_BUF_SIZE - gov_rx1_offset;
        HAL_UART_Transmit(huart, gov_rx1_buf+gov_rx1_offset, Length, HAL_MAX_DELAY);
        gov_rx1_offset = 0;
    }
}

/****
 * @ 原型: void USER_UART_IRQHandler(UART_HandleTypeDef *huart)
 * @ 输入: UART_HandleTypeDef *huart: 串口句柄指针
 * @ 输出: void
 * @ 说明: 串口空闲中断处理函数，需要在 USART1_IRQHandler() 中调用
 * @ 注意: 用于配合 DMA 实现不定长数据接收，通过空闲中断标记帧结束
 ********/
void USER_UART_IRQHandler(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        if (__HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE) != RESET)
        {
            __HAL_UART_CLEAR_IDLEFLAG(huart); // 清除空闲中断标志
            uint8_t Length = DMA_BUF_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart1_rx) - gov_rx1_offset; // 计算本次接收长度
            HAL_UART_Transmit(huart, gov_rx1_buf+gov_rx1_offset, Length, HAL_MAX_DELAY);
            gov_rx1_offset += Length; // 更新偏移量
        }
		}
}

/****
 * @ 原型: void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
 * @ 输入: TIM_HandleTypeDef *htim: 定时器句柄指针
 * @ 输出: void
 * @ 说明: 定时器溢出中断回调函数，用于周期性任务调度
 * @ 注意: TIM1=100Hz, TIM4=20kHz
 ********/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        //Tim1Callback100Hz();
    }
    else if (htim->Instance == TIM4)
    {
        Calibration_Proc();  // 通过转动电机获取准确的编码器各个步(一共200步)的准确数据
    }
}

/****
 * @ 原型: void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
 * @ 输入: CAN_HandleTypeDef *hcan: CAN句柄指针
 * @ 输出: void
 * @ 说明: CAN 接收 FIFO0 消息挂起中断回调函数
 ********/
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  CAN_RxHeaderTypeDef rx_header;
  uint8_t rx_data[CAN_LENGTH];
  uint8_t ret = FALSE;

  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK) 
  {
    ret=Queue_Insert(&gov_can_queue_st,rx_data);
    if(ret == TRUE)
    {
      gov_can_count++;
    }
    else
    {
      // 队列已满，处理溢出情况
    }
  }
}
