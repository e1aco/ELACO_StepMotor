# .cl/memory — 配置推导值（CL_StepMotor, 复刻 StepMotorCtrl_42）

> 生成阶段强制使用；值按项目推导，链可回溯（器件→datasheet→memory）。来源: 推导 / 移植-待重推导。
> 日期: 2026-08-12

## 时钟链（HSE 8MHz → PLL×9 = 72MHz，F103 无 FPU）

- clock_sysclk = 72MHz  依据: HSE 8MHz × PLLMUL=9（本板 main.c SystemClock_Config PLLMUL=RCC_PLL_MUL9）。参考板为 12MHz×MUL6 亦=72MHz，两板 TIM 一致。晶振 8MHz 已实测确认  日期: 2026-08-12  来源: 推导+用户确认
- clock_apb1 = 36MHz  依据: HCLK_DIV2（main.c RCC_ClkInitStruct.APB1CLKDivider=HCLK_DIV2，F103 APB1 上限 36MHz）  日期: 2026-08-12  来源: 推导
- clock_apb2 = 72MHz  依据: HCLK_DIV1（main.c APB2CLKDivider=DIV1）  日期: 2026-08-12  来源: 推导
- clock_timer = 72MHz  依据: APB1 预分频≠1 时定时器时钟 ×2，TIM1/2/4 挂 APB1（36MHz×2）  日期: 2026-08-12  来源: 推导
- clock_adc = 12MHz  依据: PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6（72MHz/6≤14MHz 上限）  日期: 2026-08-12  来源: 推导

## TIM2 — 步进电流 PWM（两相 FOC，H 桥 TB67H450）

- tim2_prescaler = 0  依据: 复刻参考 tim.c htim2.Init.Prescaler=0  日期: 2026-08-12  来源: 推导
- tim2_period = 1023  依据: 复刻参考 tim.c htim2.Init.Period=1023（PWM 分辨率 10bit，两相 CH3/CH4 共用 ARR）  日期: 2026-08-12  来源: 推导
- tim2_pwm_freq = 70.3kHz  依据: 72MHz/(0+1)/(1023+1) = 70.31kHz，2 相电流 PWM 载波（参考 TB67H450_SetTwoCoilsCurrent 用 12bit 值>>2 → 10bit 匹配）  日期: 2026-08-12  来源: 推导
- tim2_ch3 = PB10(PWM_B)  ch4 = PB11(PWM_A)  依据: require.md 引脚映射 + 参考 tim.c HAL_TIM_MspPostInit(PB10/11)  日期: 2026-08-12  来源: 推导

## TB67H450 — 电流环（VREF 恒流斩波 → 2 相正弦 FOC）

- tb67h450_imax = 3.5A  依据: TB67H450 datasheet p1 最大电流 3.5A（绝对最大）  日期: 2026-08-12  来源: 推导
- tb67h450_vref_gain = 1/10  依据: TB67H450 datasheet p10 "Vref gain: 1/10.0"（Iout(max) = Vref/(gain×RRS)）  日期: 2026-08-12  来源: 推导
- tb67h450_rrs = 0.1Ω  依据: require.md 硬件池 电流采样电阻:0.1R  日期: 2026-08-12  来源: 推导
- tb67h450_vref = 3.3V  依据: MCU VDD 3.3V 经 VREF 分压给驱动（待实测确认板级 VREF 实际接法）  日期: 2026-08-12  来源: 推导
- tb67h450_current_coef = 5083>>12  依据: 复刻参考 tb67h450.c dac_reg = current_mA × 5083 >> 12（mA→12bit DAC；×4095/3300≈1.24 满量程 3.3A，注释"DAC=电流×(4095/3300)×1.24"）  日期: 2026-08-12  来源: 推导
- motor_rated_current = 2A  依据: require.md 硬件池 42 步进电机 电流:2A  日期: 2026-08-12  来源: 推导
- motor_step_angle = 1.8°  依据: require.md 硬件池（200 整步/圈）  日期: 2026-08-12  来源: 推导
- sin_pi_m2_len = 1025   dpix=1024   dpiybit=12   依据: 复刻参考 sin_form.h + 公式生成核对（value[i]=round(4096×sin(i·π/512))，与参考逐值一致；**整电周期 1024 点 + 末尾回绕点，非 1/4 表**；幅值 4096=2^12）  日期: 2026-08-12  来源: 推导
- sin_pi_m2_drv_symbol = USR_sin_pi_m2 / USR_SIN_PI_M2_DPIX / USR_SIN_PI_M2_DPIYBIT / USR_SIN_PI_M2_LEN  依据: cl 命名归一（USR_ 前缀，表属 FOC 算法业务层数据）  日期: 2026-08-12  来源: 推导
- tb67h450_phase_off = 256  依据: 参考 tb67h450.c s_phaseA.sinMapPtr=(B+256)&0x3FF（A 领先 B 90°，1024/4）  日期: 2026-08-12  来源: 推导
- tb67h450_oc_10bit = 2  依据: 参考 tb67h450.c SetTwoCoilsCurrent currentA>>2（12bit DAC→10bit TIM ARR=1023）  日期: 2026-08-12  来源: 推导

## MT6816 — 磁性编码器（SPI 模式3, 14bit 绝对角度）

- mt6816_resolution = 16384  依据: datasheet p1 "14位绝对角度数据" 0~16383  日期: 2026-08-12  来源: 推导
- mt6816_spi_mode = 模式3 (CPOL=1, CPHA=1)  依据: datasheet p20 "SPI使用模式3（CPOL=1, CPHA=1）传输数据"  日期: 2026-08-12  来源: 推导
- mt6816_spi_frame = 16bit, MSB first  依据: datasheet SPI 帧 16bit + 复刻参考 mt6816.c（0x80|CMD）<<8 16 位收发  日期: 2026-08-12  来源: 推导
- mt6816_spi_prescaler = 8 (9MHz)  依据: 复刻参考 spi.c SPI_BAUDRATEPRESCALER_8（72MHz/8=9MHz，datasheet TSCK≥64ns → ≤15.6MHz 满足）  日期: 2026-08-12  来源: 推导
- mt6816_cmd_angle = 0x83（0x80|0x03）  cmd_raw_angle = 0x84（0x80|0x04）  依据: 复刻参考 mt6816.c MT6816_CMD_ANGLE=0x03 / CMD_RAW_ANGLE=0x04  日期: 2026-08-12  来源: 推导
- mt6816_raw_angle = raw_data>>2（14bit bit2~15）  依据: 复刻参考 mt6816.c + 偶校验校验通过判定  日期: 2026-08-12  来源: 推导
- mt6816_parity = 偶数校验（1 表示数据奇校验失败）  依据: 复刻参考 mt6816.c CalcParity==0 通过  日期: 2026-08-12  来源: 推导
- mt6816_no_mag = bit1（无磁场标志）  依据: 复刻参考 mt6816.c  日期: 2026-08-12  来源: 推导
- encoder_power_monitor = PA0 ADC1_IN0（POW_MT6816）  依据: require.md 引脚映射（编码器供电监测）  日期: 2026-08-12  来源: 推导

## USART — 调试回传与命令（复刻参考用 USART1 DMA，本项目调试回传 USART3）

- usart3_baud = 115200  依据: require.md 调试区 + usart.c MX_USART3_UART_Init BaudRate=115200  日期: 2026-08-12  来源: 推导
- usart3_8N1  no_flow_control  依据: usart.c WordLength=8B/StopBits=1/Parity=None  日期: 2026-08-12  来源: 推导
- usart3_rx_dma = DMA1_Channel3 CIRCULAR  依据: usart.c（调试回传接收 DMA 环形）  日期: 2026-08-12  来源: 推导
- usart3_tx_dma = DMA1_Channel2 NORMAL  依据: usart.c  日期: 2026-08-12  来源: 推导
- usart1_baud = 115200  依据: 复刻参考 usart.c（RS485 命令通道，PA9/PA10 映射）  日期: 2026-08-12  来源: 推导

## CAN — 命令总线（参考项目 CAN1 PB8/PB9 remap；本项目 PA11/PA12 默认映射）

- can1_pins = PA11(RX)/PA12(TX)  依据: require.md 引脚映射（默认 CAN1，无 remap）  日期: 2026-08-12  来源: 推导
- can1_bitrate = 500kbps  依据: 复刻参考 can.c 需按本项目 Prescaler=6/BS1=9TQ/BS2=2TQ 核对（36MHz/(6×(1+9+2))=500kbps；参考 Prescaler=4/BS1=5TQ/BS2=4TQ → 900kbps，两者不一致，待实测确认实际总线）  日期: 2026-08-12  来源: 推导

## ADC1 — 编码器供电监测

- adc1_ch0_sampling = 239.5cycles  依据: 复刻参考 adc.c ADC_SAMPLETIME_239CYCLES_5（低内阻源稳定采样；本项目 CubeMX 默认 1.5cycles 待核对）  日期: 2026-08-12  来源: 推导

## 定时装配 — 控制 tick（motion_planner 积分基准）

- control_frequency = 20000Hz  control_period_us = 50  依据: TIM4 PSC=71/Period=49 → 72MHz/(72×50)=20kHz（参考 TIM4 同款 20kHz 主控 tick，复刻时以 CubeMX tim.c 实测核对）  日期: 2026-08-12  来源: 推导
- planner_tracker_list = CurrentTracker/VelocityTracker/PositionTracker/TrajectoryTracker（PositionInterpolator 参考死代码未调用，默认不移植，MODE_STEP_DIR 需时再补）  依据: 参考 motion_planner.c 函数调用图（PositionInterpolator 无调用点）  日期: 2026-08-12  来源: 推导
- planner_global_decoupling = 参考 g_go_current/g_go_velocity/g_go_location/g_traj_go_* 外部全局 → 复刻为内部 s_go_* + getter（USR_MotionPlanner_*Tracker_GetGo*）  依据: 分层规范（usr 模块内聚，收敛 extern 全局耦合）  日期: 2026-08-12  来源: 推导

## Flash 分区（F103RE 大容量 512K，页 2K=0x800）

- flash_page_size = 0x800  依据: STM32F103RE 大容量 512K，页大小 2K（参考 stockpile_config.h 注释：RE=512K/64K + stm32f1xx_hal_flash_ex.h:246 STM32F103xE→0x800）  日期: 2026-08-12  来源: 推导
- cali_table_addr = 0x08077800  依据: 512K 尾部按 2K 页对齐向下分配：固件 0x08000000 起，CALI 32K(16页)=0x08077800~0x0807F7FF，顶部留 DATA 2K(1页)=0x0807F800~0x0807FFFF（原参考 0x08017C00 是 128K 芯片布局，RE 需重排）  日期: 2026-08-12  来源: 推导
- cali_table_size = 0x8000 (32K, 16K×2byte 校准表)  依据: 参考 stockpile_config.h STOCKPILE_APP_CALI_SIZE  日期: 2026-08-12  来源: 推导
- data_addr = 0x0807F800 (2K=1页)  依据: CALI 顶上一页，2K 页对齐，留给 boardConfig 持久化  日期: 2026-08-12  来源: 推导
## 编码器校准（encoder_calibrator_usr，校准过程参数）

- cali_hard_steps = 200   sample_per_step = 16   auto_speed = 2   fine_speed = 1  依据: 复刻参考 encoder_calibrator.c 常量  日期: 2026-08-12  来源: 推导
- cali_enc_resolution = 16384  soft_divide = 256  subdivide_steps = 51200  依据: 与 mt6816_resolution / SOFT_DIVIDE_NUM / MOTOR_SUBDIVIDE_STEPS 同源  日期: 2026-08-12  来源: 推导
- cali_current = 2000mA  依据: .cl/memory/ config_default_calib_current=2000（校准需足够力矩转两圈）  日期: 2026-08-12  来源: 推导
- cali_trigger = 上电同按 SW1(PB2)+SW2(PB1)  依据: 复刻参考 main.c 双键触发 + require.md 按键引脚（低有效）  日期: 2026-08-12  来源: 推导
- cali_table_format = uint16[16384]，index=raw 角度，value=细分步(0~51199)，未校准区=0xFFFF 依据: 参考 encoder_calibrator.c GenerateTable + mt6816_usr 校验逻辑  日期: 2026-08-12  来源: 推导

## Button — 按键事件（SW1=PB2 / SW2=PB1，低有效）

- button_num = 2  依据: 复刻参考 button.c BUTTON_NUM=2 + require.md 引脚映射（SW1/SW2 两键）  日期: 2026-08-13  来源: 推导
- button_id_map = 1→SW1(PB2) 2→SW2(PB1)  依据: 参考 button.c 用 PB12/PB2，本板 PB12=LED2(输出) 不可作按键，按 require.md 改 SW1=PB2/SW2=PB1  日期: 2026-08-13  来源: 推导
- button_scan_freq = 100Hz  依据: 参考 main.c Tim1Callback100Hz 调 Button_Tick（TIM1=100Hz 10ms 扫描）  日期: 2026-08-13  来源: 推导
- button_long_press_ms = 3000  依据: 复刻参考 button.c LONG_PRESS_MS=3000（3s 判长按）  日期: 2026-08-13  来源: 推导
- button_active_level = 低有效  依据: 复刻参考 button.c ReadPin == GPIO_PIN_RESET 判按下  日期: 2026-08-13  来源: 推导

## 配置默认值（boardConfig 编译期，EEPROM 持久化后置）

- config_default_can = 2  default_mode = MODE_COMMAND_POSITION  current_limit = 1000mA  依据: 参考 main.c 配置无效分支默认值 + 参考 uart_cmd.c defaultNodeID=2  日期: 2026-08-12  来源: 推导
- config_default_velocity = 30圈/s = 30×51200 细分步/s  依据: 参考 main.c (velocityLimit=30*MOTOR_SUBDIVIDE_STEPS, MOTOR_SUBDIVIDE_STEPS=200×256=51200)  日期: 2026-08-12  来源: 推导
- config_default_velacc = 100圈/s² = 100×51200 细分步/s²  依据: 参考 main.c (velocityAcc=100*MOTOR_SUBDIVIDE_STEPS)  日期: 2026-08-12  来源: 推导
- config_default_calib_current = 2000mA  依据: 参考 main.c 默认 calibrationCurrent  日期: 2026-08-12  来源: 推导
- config_default_pid = kp5/ki30/kd0  dce = kp200/kv80/ki300/kd250  依据: 参考 main.c 配置无效分支默认值（闭环调参起点，待实测整定）  日期: 2026-08-12  来源: 推导
- config_default_enable = enableMotorOnBoot=false enableStallProtect=false  依据: 参考 main.c 默认（安全优先：上电不使能、不加堵转保护待实测）  日期: 2026-08-12  来源: 推导
- motor_usr_subdivide = MOTOR_HARD_STEPS=200 / SOFT_DIVIDE_NUM=256 / MOTOR_SUBDIVIDE_STEPS=51200  依据: 复刻参考 motor.h  日期: 2026-08-12  来源: 推导
## 电机最小闭环（motor_usr，raw 角度驱动占位）

- motor_usr_symbols = USR_Motor_Init / SetConfig / Tick20kHz / SetMode / SetPosition / SetVelocity / SetCurrent / SetDisable / SetBrake / GetState / GetPosition / GetVelocity / GetCurrent / GetMode / GetTelemetry / ClearStallFlag  依据: 复刻参考 motor.h 接口（USR_ 前缀）  日期: 2026-08-12  来源: 推导
- motor_enc_raw_scale = ×25/8 (51200/16384)  依据: 未校准(选项B) raw 14bit(0~16383) 线性缩放进 51200 细分步空间；校准后由校准表接管（表输出即 51200 空间）  日期: 2026-08-12  来源: 推导
- motor_foc_lead_90 = SOFT_DIVIDE_NUM=256  依据: 参考 motor.c CalcCurrentToOutput 正电流超前/负电流滞后 90°（细分步）  日期: 2026-08-12  来源: 推导
- motor_compensate_angle = 分段补偿 |±430| 步  依据: 参考 motor.c CompensateAdvancedAngle（vel 阈值 100k/1.3M/2.2M，斜率 262/105/52>>20）  日期: 2026-08-12  来源: 推导
- motor_minloop_control = 简单 P 环 current=Kp×err>>10 限 ±ratedCurrent（Kp=dce_kp=200, err 限 ±3200）  依据: 参考 DCE 输出量纲（kp×pError>>10），暂未接 PID/DCE 任务6  日期: 2026-08-12  来源: 推导
- motor_loop_damping = dceKd=400 速度阻尼 out=(Kp×err−Kd×(vel>>7))>>10，vel>>7 限 ±4000  依据: 实测整定——纯 P 无阻尼极限环震动；Kd=250 不足、400 收敛；对齐参考 CalcDceToOutput vError 量纲  日期: 2026-08-13  来源: 实测决策
- motor_loop_deadband_off = 到位死区 POS_DEADBAND=128 细分步(≈0.9°) 内输出直接归零（P+Kd 全停）  依据: 实测——到位后 FOC 电流引起编码器微抖被速度 IIR 放大成假速度→Kd 响应出电流→极限环；STOP 模式 cur=0 时 vel 恒 0 证实电流是抖源；死区输出归零=移除抖源  日期: 2026-08-13  来源: 实测决策
- motor_est_vel_filter = IIR 低通系数 1/32（integral += Δpos×20kHz + (v<<5 - v), v=integral>>5）  依据: 参考 motor.c 速度估计  日期: 2026-08-12  来源: 推导
- motor_state_min = STOP/RUNNING/FINISH（最小闭环无过载/堵转/未校准检测，任务6后补）  依据: 参考 motor.c 状态机裁剪  日期: 2026-08-12  来源: 推导
- motor_pos_deadband = 128 细分步（≈0.9°）  motor_vel_deadband = 512 细分步/s  motor_cur_deadband = 10mA  依据: 最小闭环无 planner，软目标=目标，需自判到位（参考靠 planner 平滑+soft==goal 判 FINISH，最小闭环改用死区判据）；死区内输出归零  日期: 2026-08-13  来源: 推导+实测（2026-08-13 精度实测 pos 稳定 0.248 vs 目标 0.250 → 偏差 0.002 圈≈0.72° < 死区 0.9°）
- motor_test_limits = currentLimit=2000mA（42 步进额定 2A）、velocityLimit=2圈/s（步进低速区间防失步，用户确认：10圈/s 太快）  依据: 用户反馈“步进电机限幅 2000mA/10圈/s 都太快”，改 2圈/s  日期: 2026-08-13  来源: 实测决策