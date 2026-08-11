# 硬件资源池 (Hardware Pool)
芯片: STM32F103RET6 (LQFP64)
晶振频率: 8MHz HSE (HSI 8MHz)
关键器件:
- 电流采样电阻:0.1R
- 驱动芯片:TB67H450
- 编码器芯片:MT6816-ACD
- 42步进电机:
  1.步进脚:1.8°
  2.力矩:0.43Nm
  3.电阻:2Ohm
  4.电感:3.6mH
  5.轴径:5mm
  6.电流:2A
  7.重量:285g

# 引脚固定映射表 (Pin Mapping)
| 功能网标 | 引脚号 | 外设功能 |
| :------- | :----- | :------- |
| POW_MT6816 | PA0 | ADC1_IN0 (编码器供电监测)
AP         | PA1 | GPIO_Output
AM         | PA2 | GPIO_Output
SPI_CS     | PA4 | GPIO_Output (SPI 片选)
SPI1_SCK   | PA5 | SPI1_SCK
SPI1_MISO  | PA6 | SPI1_MISO
SPI1_MOSI  | PA7 | SPI1_MOSI
RS485_TX   | PA9 | USART1_TX
RX485_RX   | PA10 | USART1_RX
CAN_RX     | PA11 | CAN_RX
CAN_TX     | PA12 | CAN_TX
SWDIO      | PA13 | SYS_JTMS-SWDIO
SWCLK      | PA14 | SYS_JTCK-SWCLK
SW2        | PB1  | GPIO_Input
PWM_B      | PB10 | TIM2_CH3
PWM_A      | PB11 | TIM2_CH4
LED2       | PB12 | GPIO_Output
LED1       | PB13 | GPIO_Output
SW1        | PB2  | GPIO_Input
USART3_TX  | PC10 | USART3_TX
USART3_RX  | PC11 | USART3_RX
BP         | PC2  | GPIO_Output
BM         | PC3  | GPIO_Output
OSC_IN     | PD0  | RCC_OSC_IN (HSE)
OSC_OUT    | PD1  | RCC_OSC_OUT (HSE)

# 工具链池 (Toolchain Pool)
IDE/编译器: MDK-ARM V5.32 (Keil)
芯片型号: STM32F103RET6
优化等级: -O1 (CompilerOptimize=6)
FPU: 无 (F103 Cortex-M3)

# 调试与烧录 (Debug/Flash)
调试器: ST-Link (pyocd 0.45.1, 当前未检测到连接, 接线后重插)
调试回传接口: UART (USART3, 115200-8-N-1)
烧录工具: pyocd/ST-Link
烧录指令: python tools/flash.py --flash <每次编译产出的 hex 路径>
复位/运行: python tools/flash.py --reset
调试回传读取: python tools/serial_monitor.py --port <COM口> --baud 115200

# 项目全局目标 (Global Goal)
实现42步进电机闭环控制+梯形规划。

# 任务队列 (Task Queue)
> 状态标记说明:
> [ ] 未开始
> [x] AI 认为已完成（需人工验收）
> [✓] 人工验收通过
> [!] 阻塞待分析
> [c] Code Only — 已生成代码，调试由你手动接管

> 格式：任务按日期分组，标题一句话；细节由 /cl run 向用户提问澄清。

## 26-08-10
- [!] 使用20Khz频率 对电机进行闭环控制，运行4个角度，要求实现电机到位的稳准快。（起 2026-08-10 | 止 2026-08-11 | 验收 未通过）
  - 已解决: 供电(4.7V→3.3V)、校准DONE状态机、表回绕连续、TIM4 NVIC中断、serial_monitor编码bug
  - 待解决: 0°卡死(校准表0°磁点偏移~1196微步)、90/180/270需NVIC修复后复验、demo被0°卡住无法跑完
