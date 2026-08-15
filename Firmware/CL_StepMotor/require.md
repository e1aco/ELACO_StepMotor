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
Python: C:\Users\electronic\AppData\Local\Programs\Python\Python312\python.exe  # cl tools 统一用此路径（python 不在 PATH）

# 调试与烧录 (Debug/Flash)
调试器: ST-Link (pyocd 0.45.1, 当前未检测到连接, 接线后重插)
调试回传接口: UART (USART3, 115200-8-N-1)  [已确认]
烧录工具: pyocd/ST-Link
烧录指令: python tools/flash.py --flash <每次编译产出的 hex 路径> --target stm32f103rc   # pyocd 0.45.1 内置仅 stm32f103rc（F103RE 同族可烧，2026-08-15 实测通过）
复位/运行: python tools/flash.py --reset --target stm32f103rc
调试回传读取: python tools/serial_monitor.py --port <COM口> --baud 115200

> CAN 波特率差异（memory 标注）：参考项目 can.c 900kbps(PSC4/1+5+4TQ) vs 本工程 CubeMX 500kbps(PSC6/1+9+2TQ)，复刻时按实际总线确认。

# 项目全局目标 (Global Goal)
复刻项目E:\Desktop\XM\ELACO_StepMotor\Reference\zhjStepMotor\Firmware\StepMotorCtrl_42
我在硬件中对于软件的影响仅仅是主控芯片换成了STM32F103RET6，其他硬件不变，所以软件的架构和逻辑应该与原项目保持一致。
要求是按照工程师的思维一步一步的复刻这个项目，每次做完一步就生成报告，然后让我去读取项目，以达到让我可以完美的知道这样一个项目是如何一步一步实现的。
同时锻炼了我的项目思维，增加了对这个项目的认识同时还能增加cl skill的经验，此外还可以反馈出cl skill的不足，让我去改进。

# 时序测量表 (Timing Measurement)
> 预算=时钟分母推导；测试方法: 软件=板端 DWT/SysTick 回传，硬件=Saleae，外部=示波器/逻辑分析仪。
> 状态: [ ] 未测 / [x] 已测合格 / [!] 阻塞/需外部仪器 / [expired] 代码变更待重测。
> 回传格式: `[TM] <tag> <值> <单位> <周期数>`（同 tag 多行取最大值=最坏情况）。

| # | 条目 | 位置/探针位置 | 方法 | 预算 | 实测 | 测试周期 | 状态 |
|---|------|--------------|------|------|------|---------|------|
| T1 | TIM4 20kHz 电机闭环 ISR 总耗时 | TIM4_IRQHandler 入口/出口 | 软件 | 50µs（周期 1/20kHz=50µs，72MHz/(71+1)/(49+1)） | 37µs（2026-08-15 DWT） | 每次变更都测 | [x] |
| T2 | TIM1 100Hz 遥测 ISR 总耗时 | TIM1_UP_IRQHandler 入口/出口 | 软件 | 1000µs（周期 10ms 的 10% 占空；8/15 Saleae 实测 ~2.3ms 超限待复测） | 86µs（2026-08-15 DWT；调参前 2262µs 超限，调参：%f 定点化+发送移主循环） | 每次变更都测 | [x] |
| T3 | MT6816 SPI 单次读（含≤3 重试） | USR_MT6816_UpdateAngle 入口/出口 | 软件 | 15µs（9MHz 每帧 1.78µs × 2 帧 × 3 重试=10.7µs + HAL 轮询余量；TSCK≥64ns） | 12µs（2026-08-15 DWT；调参：HAL 28µs→寄存器级+SPE 前置使能，见下注） | 每次变更都测 | [x] |
| T4 | ADC1 编码器供电采样（HAL Start+Poll 路径） | tim_test 测试序列（业务未接入，临时触发） | 软件 | 29µs（memory adc1_ch0_sampling 239.5cycles@12MHz 参考；CubeMX 实际 1.5cycles 待核对） | 27µs（2026-08-15 DWT） | 每次变更都测 | [x] |

# 任务队列 (Task Queue)
> 状态标记说明:
> [ ] 未开始
> [x] AI 认为已完成（需人工验收）
> [✓] 人工验收通过
> [!] 阻塞待分析
> [c] Code Only — 已生成代码，调试由你手动接管

> 格式：任务按日期分组，标题一句话；细节由 /cl run 向用户提问澄清。

## 2026-08-12
- [✓] 复刻运行骨架: LED _drv 外设原语 + UART3 _drv 发送 + main.c 挂载打印 System Start!（起 2026-08-12 | 止 2026-08-12 | 验收 2026-08-12 人工验收通过，打印链路验证 OK）<- 优先，先验证链路
- [✓] 复刻 MT6816: SPI 读绝对角 → 分层 mt6816_drv(原语+偶校验协议) / mt6816_usr(重试+校准映射)（起 2026-08-12 | 止 2026-08-12 | 验收 2026-08-12 人工验收通过）<- 传感（重构：初版照抄已按分层重写）
- [✓] 复刻 TB67H450: sin_form(→usr) + TIM2 电流 PWM → 分层 drv(原语)/usr(FOC 算法)（起 2026-08-12 | 止 2026-08-12 | 验收 2026-08-12 人工验收通过）<- 执行
- [✓] 定时装配: TIM1 100Hz + TIM4 20kHz 中断启动 + tick 回调（心跳 LED + 20kHz 计数打印）（起 2026-08-12 | 止 2026-08-12 | 验收 2026-08-12 人工验收通过）<- 屋顶，所有 20kHz 算法的节拍源，最先做
- [✓] 编码器校准: encoder_calibrator 采样→校验→生成校准表写 Flash（CALI 0x08077800）（起 2026-08-12 | 止 2026-08-12 | 验收 2026-08-12 校准表写 Flash 验证通过，result_num=16384 锚点自洽）<- 位置闭环基准

## 2026-08-13
> 顺序原则：先核心动作（电机闭环）→ 优化（规划）→ 外围（配置/通信）。不做 485（USART1 不用于命令），命令走 CAN（协议做时再定）。
- [✓] 复刻 button: 按键扫描（100Hz tick）→ button_usr（click/long/IsPressed 事件）<- 输入事件源，独立快（起 2026-08-13 | 止 2026-08-13 | 验收 2026-08-13 人工验收通过，按键事件与命令链路正常）
- [✓] 复刻 motor 基础闭环: 编码器 raw×25/8 映射 + FOC 电流输出 + 位置/速度/电流命令 + 速度估计 IIR + 基础状态机（P 环，电机能转/能停/能定位）<- 核心动作先跑通（起 2026-08-13 | 止 2026-08-13 | 验收 2026-08-13 人工验收通过，震动调参 3 轮收敛：Kd400 阻尼+死区输出归零；精度实测 ±0.72° < 死区 0.9°）
- [ ] 复刻 motion_planner: 4 tracker（Current/Velocity/Position/Trajectory）20kHz 软目标生成（PositionInterpolator 不移植）<- 加减速优化层
- [ ] 复刻 motor 完善: 接入 planner + PID/DCE + 超前角补偿 + 完整状态机（过载/堵转/未校准）<- 控制质量与保护
- [ ] 复刻配置持久化: config_usr（BoardConfig_t/默认值/configStatus）+ eeprom_usr（DATA 0x0807F800 读写/IsValid/Erase）<- 掉电保存
- [ ] 新增 can_cmd_usr: CAN1（PA11/PA12, 500kbps）命令+应答（协议做时再定，命令对齐参考 c/v/p/s/z/l 语义）<- 替代 485 命令通道
- [ ] main 装配: 读配置→Motor 注入→100Hz 按键事件/LED/遥测 USART3→20kHz 校准/电机分派→CONFIG_COMMIT/RESTORE<- 完整生命周期
- [✓] 按键-LED 测试: BTN1→LED1 / BTN2→LED2，短按 toggle 开关，长按闪烁 3 次后恢复记忆状态（验证后删除测试段）（起 2026-08-13 | 止 2026-08-13 | 验收 2026-08-13 按键功能确认 OK，测试段已删除恢复原样，编译 0E/0W）
- [ ] 遗留加固: USR_EncoderCalibrator_Init 判据过弱（仅查首值）→ 全表扫描校验 <- 低风险独立

## 2026-08-15
- [x] 电机闭环串级重构: 位置环→速度环→电流（dceKp/dceKd 退役 → posKp=32768 标定 + pidKp=5 起点；位置环限速 ±ratedVelocity；死区 128 内输出直接归零、跳过速度环）+ 串级调参方法沉淀 knowledge/motor.md（起 2026-08-15 | 止 2026-08-15）<- 烧录与调参由你接管。抖动调试：8/15 第1轮 vel 死区 512 无效（噪声 66k 步/s 差百倍）→ 第2轮死区内输出归零（8/13 实测复证）→ 到位 FINISH 全零收敛，偏差 0.72°<死区；遗留：state 单帧回 RUNNING（FINISH 保持）+ 运动段摆动（planner 改善）
- [✓] 到位抖动根治（方案Y）: 死区 128→256 + 保持力删除（保持电流→编码器磁干扰→假速度 ±100000→180° 摆动 ±260）+ 到位制动 10ms（一次性刹停残余速度，避免持续死区速度环假速度自激）+ 减速窗口 MIN_VEL_DS 1024→2048（低速进死区）+ 0° 绕回窗口 0 电流归零（起 2026-08-15 | 止 2026-08-15 | 验收 2026-08-15 人工验收通过，7+ 轮完整循环全部一次收敛：90°→12678/180°→25517/270°→38293/360°→51094 落点 ±5 步一致，到位即 rv=0 静止，含 0° 绕回与长行程回程）<- 运动段摆动遗留，由 planner 改善

