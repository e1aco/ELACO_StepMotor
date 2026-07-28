# ELA_StepMotor 规格说明

## 硬件平台
- MCU: STM32F103RET6 (Cortex-M3, LQFP64)
- 电机驱动: 双 TB67H450FNG H 桥
- 编码器: MT6816 14-bit 磁编码器 (SPI 模式3, 1Mbps)
- 通信: CAN (166kbps) + Modbus RTU/ASCII RS-485 (USART1)
- 调试口: USART3 DMA (printf 重定向)

## 引脚资源
- 线圈: Coil A+ (PA1), A- (PA2), B+ (PC2), B- (PC3)
- PWM: TIM2_CH3 (PB10), TIM2_CH4 (PB11)
- SPI1: CS (PA4), SCK (PA5), MISO (PA6), MOSI (PA7)
- 按键: SW1 (PB1), SW2 (PB2)
- LED: LED1 (PB12), LED2 (PB13)
- ADC: POW_MT6816 (PA0)

## 定时器分配
- TIM1: 100Hz 周期中断
- TIM2: PWM 生成 (CH3/CH4)
- TIM3: 通用定时
- TIM4: 20kHz 周期中断
- TIM7: Modbus 3.5 字符超时 (50µs tick, 通过 htim3 别名)

## 软件架构
- HAL 初始化 → elaco_main() → while(1)
- 模块分层: drv → usr → cac (回调集中 elaco_main.c)
- 无 RTOS, 裸机中断驱动

## 预留模块
- ela_button: 按键驱动
- ela_stockfile: 参数存储
- ela_cyclecal: 周期计算
- elaco_calibration: 校准表生成
