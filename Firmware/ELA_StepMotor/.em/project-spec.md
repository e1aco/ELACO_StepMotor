# 项目规格单: ELA_StepMotor

## 项目概述
ELACO 步进电机驱动固件，基于 STM32F103RET6，驱动 4 相步进电机，配备 MT6816 14 位磁编码器反馈。通过 CAN 总线和 Modbus RTU/ASCII (RS-485) 通信。

## 步骤表

| 步骤 | 描述 | 状态 | 备注 |
|------|------|------|------|
| S1 | CubeMX 外设初始化 | ✅ 完成 | GPIO, DMA, ADC, CAN, SPI, TIM, USART |
| S2 | MT6816 磁编码器驱动 | ✅ 完成 | SPI 通信, 14位角度读取, 奇偶校验 |
| S3 | CAN 命令队列 | ✅ 完成 | 48帧循环队列, 8字节帧 |
| S4 | UART 通信模块 | ✅ 完成 | USART3 printf重定向, RS-485 DMA |
| S5 | 串口正弦表 | ✅ 完成 | ela_sinform.c |
| S6 | TB67H450 步进电机驱动 | ✅ 完成 | 双H桥PWM控制 |
| S7 | 步进电机控制逻辑 | 🔄 进行中 | 速度/位置控制算法 |
| S8 | Modbus RTU/ASCII 协议栈 | ⏳ 待开始 | FreeModbus 集成 |
| S9 | 按键输入处理 | ⏳ 待开始 | SW1/SW2 |
| S10 | LED 状态指示 | ⏳ 待开始 | LED1/LED2 |
| S11 | EEPROM 配置存储 | ⏳ 待开始 | 参数保存/恢复 |
| S12 | 整体联调测试 | ⏳ 待开始 | 全功能验证 |

## 硬件约束
- 时钟: HSI 8MHz (开发阶段), 目标 72MHz HSE+PLL
- 无 RTOS: 裸机 HAL + 中断驱动
- PWM: TIM2_CH3/CH4 控制电机线圈电流
- 编码器: SPI1 (MT6816, 1Mbps, Mode 3)
- 通信: USART1 (RS-485), CAN1 (166kbps)
