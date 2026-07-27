# StepMotor 项目笔记

## 项目概述
基于 STM32F103CBT6 的步进电机驱动器固件，复刻 XM-003-08 参考项目，使用 MDK-ARM (Keil) 工具链。

## 硬件架构
- **MCU**: STM32F103CBT6 (LQFP48), 72MHz (HSE 12MHz x6 PLL)
- **驱动芯片**: TB67H450FNG x2 (两路 H 桥 → 四相步进电机)
- **编码器**: MT6816CT-ACD (14位磁编码器, SPI 通信)
- **CAN**: SN65HVD232DR (900kbps)
- **电源**: LP2992IM5-3.3 (5V→3.3V LDO)

### 引脚映射
| 功能 | 引脚 | 标签 |
|------|------|------|
| 步进电机 A+ | PA5 | HW_ELEC_AP |
| 步进电机 A- | PA4 | HW_ELEC_AM |
| 步进电机 B+ | PA3 | HW_ELEC_BP |
| 步进电机 B- | PA2 | HW_ELEC_BM |
| PWM_A (TIM2_CH3) | PB10 | IN_PWM_A |
| PWM_B (TIM2_CH4) | PB11 | IN_PWM_B |
| MT6816 CS | PA15 | SPI1_CS |
| SPI1 SCK/MISO/MOSI | PB3/PB4/PB5 | |
| USART1 TX/RX | PB6/PB7 | |
| CAN TX/RX | PB9/PB8 | |
| BUTTON_1 | PB12 | |
| BUTTON_2 | PB2 | |
| LED1/LED2 | PA12/PA11 | |
| ADC (电压采样) | PA0 | POWER_U |

### TB67H450 控制
- **IN1/IN2（GPIO）**：控制 H 桥正反转/刹车（PA2~PA5）
- **VREF（PWM→RC 滤波）**：控制相电流（TIM2_CH3→ Coil A, TIM2_CH4→ Coil B）

### 定时器配置
- **TIM1**: 72MHz/(71+1)/(9999+1) = **100Hz** (定时中断)
- **TIM2**: 72MHz/1024 ≈ **70.3kHz PWM** (CH3/CH4 步进驱动)
- **TIM4**: 72MHz/(71+1)/(49+1) = **20kHz** (定时中断)

## 软件结构

### ELA_LIB 模块
| 模块 | 文件 | 功能 |
|------|------|------|
| 主循环 | `elaco_main.c/h` | 初始化入口，中断回调集中管理 |
| 串口 | `ela_uart.c/h` | DMA 收发 + 空闲中断不定长接收，printf 重定向 |
| 按键 | `ela_button.c/h` | 2路按键检测，支持单击/长按(3秒) |
| 队列 | `ela_queue.c/h` | 48x8 循环队列，用于 CAN 命令缓存 |
| 编码器 | `ela_mt6816.c/h` | MT6816 SPI 读取，14位角度 + 状态解析 |
| 电机驱动 | `ela_tb67h450.c/h` | TB67H450 步进电机控制，支持 Wave/Full/Half 步进模式 |

### 外设初始化顺序 (main.c)
```
HAL_Init → SystemClock_Config → MX_GPIO_Init → MX_DMA_Init
→ MX_ADC1_Init → MX_USART1_UART_Init → MX_CAN_Init
→ MX_SPI1_Init → MX_TIM1_Init → MX_TIM2_Init → MX_TIM4_Init
→ elaco_main()
```

## 命名规范 (详见 `naming_rules.md`)
- 文件: `ela_` 前缀
- 函数: `Module_ActionDetails` PascalCase (如 `MT6816_ReadAngle`)
- _proc 函数: 全小写 `module_action_proc` (如 `button_scan_proc`)
- 全局变量: `all_` 前缀
- 缩进: 4空格，禁止 Tab

## SPI1 配置 (MT6816 通信)
- Mode 3 (CPOL=HIGH, CPHA=2EDGE), 16bit, 9Mbps, MSB First, Software NSS

## 当前开发状态
- [x] 基础工程搭建 (CubeMX + Keil)
- [x] 串口 DMA 收发 + 空闲中断
- [x] 按键驱动 (单击/长按)
- [x] CAN 命令队列
- [x] MT6816 编码器驱动
- [x] TB67H450 步进电机驱动 (Wave/Full/Half 模式)
- [ ] 电机闭环控制 (编码器反馈 + 步进驱动联动)
- [ ] CAN 通信协议实现

## 参考项目
- **XM-003-08/zhjStepMotor/** — 包含参考原理图、PCB Gerber、固件和文档
- 原理图 PDF: `XM-003-08/SCH_Schematic1_2026-05-20.pdf`
