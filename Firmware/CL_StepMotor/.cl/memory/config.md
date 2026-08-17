# .cl/memory �?配置推导值（CL_StepMotor, 复刻 StepMotorCtrl_42�?
> 生成阶段强制使用；值按项目推导，链可回溯（器件→datasheet→memory）。来�? 推导 / 移植-待重推导�?> 日期: 2026-08-12

## 时钟链（HSE 8MHz �?PLL×9 = 72MHz，F103 �?FPU�?
- clock_sysclk = 72MHz  依据: HSE 8MHz × PLLMUL=9（本�?main.c SystemClock_Config PLLMUL=RCC_PLL_MUL9）。参考板�?12MHz×MUL6 �?72MHz，两�?TIM 一致。晶�?8MHz 已实测确�? 日期: 2026-08-12  来源: 推导+用户确认
- clock_apb1 = 36MHz  依据: HCLK_DIV2（main.c RCC_ClkInitStruct.APB1CLKDivider=HCLK_DIV2，F103 APB1 上限 36MHz�? 日期: 2026-08-12  来源: 推导
- clock_apb2 = 72MHz  依据: HCLK_DIV1（main.c APB2CLKDivider=DIV1�? 日期: 2026-08-12  来源: 推导
- clock_timer = 72MHz  依据: APB1 预分频≠1 时定时器时钟 ×2，TIM1/2/4 �?APB1�?6MHz×2�? 日期: 2026-08-12  来源: 推导
- clock_adc = 12MHz  依据: PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6�?2MHz/6�?4MHz 上限�? 日期: 2026-08-12  来源: 推导

## TIM2 �?步进电流 PWM（两�?FOC，H �?TB67H450�?
- tim2_prescaler = 0  依据: 复刻参�?tim.c htim2.Init.Prescaler=0  日期: 2026-08-12  来源: 推导
- tim2_period = 1023  依据: 复刻参�?tim.c htim2.Init.Period=1023（PWM 分辨�?10bit，两�?CH3/CH4 共用 ARR�? 日期: 2026-08-12  来源: 推导
- tim2_pwm_freq = 70.3kHz  依据: 72MHz/(0+1)/(1023+1) = 70.31kHz�? 相电�?PWM 载波（参�?TB67H450_SetTwoCoilsCurrent �?12bit �?>2 �?10bit 匹配�? 日期: 2026-08-12  来源: 推导
- tim2_ch3 = PB10(PWM_B)  ch4 = PB11(PWM_A)  依据: require.md 引脚映射 + 参�?tim.c HAL_TIM_MspPostInit(PB10/11)  日期: 2026-08-12  来源: 推导

## TB67H450 �?电流环（VREF 恒流斩波 �?2 相正�?FOC�?
- tb67h450_imax = 3.5A  依据: TB67H450 datasheet p1 最大电�?3.5A（绝对最大）  日期: 2026-08-12  来源: 推导
- tb67h450_vref_gain = 1/10  依据: TB67H450 datasheet p10 "Vref gain: 1/10.0"（Iout(max) = Vref/(gain×RRS)�? 日期: 2026-08-12  来源: 推导
- tb67h450_rrs = 0.1Ω  依据: require.md 硬件�?电流采样电阻:0.1R  日期: 2026-08-12  来源: 推导
- tb67h450_vref = 3.3V  依据: MCU VDD 3.3V �?VREF 分压给驱动（待实测确认板�?VREF 实际接法�? 日期: 2026-08-12  来源: 推导
- tb67h450_current_coef = 5083>>12  依据: 复刻参�?tb67h450.c dac_reg = current_mA × 5083 >> 12（mA�?2bit DAC；�?095/3300�?.24 满量�?3.3A，注�?DAC=电流×(4095/3300)×1.24"�? 日期: 2026-08-12  来源: 推导
- motor_rated_current = 2A  依据: require.md 硬件�?42 步进电机 电流:2A  日期: 2026-08-12  来源: 推导
- motor_step_angle = 1.8°  依据: require.md 硬件池（200 整步/圈）  日期: 2026-08-12  来源: 推导
- sin_pi_m2_len = 1025   dpix=1024   dpiybit=12   依据: 复刻参�?sin_form.h + 公式生成核对（value[i]=round(4096×sin(i·π/512))，与参考逐值一致；**整电周期 1024 �?+ 末尾回绕点，�?1/4 �?*；幅�?4096=2^12�? 日期: 2026-08-12  来源: 推导
- sin_pi_m2_drv_symbol = USR_sin_pi_m2 / USR_SIN_PI_M2_DPIX / USR_SIN_PI_M2_DPIYBIT / USR_SIN_PI_M2_LEN  依据: cl 命名归一（USR_ 前缀，表�?FOC 算法业务层数据）  日期: 2026-08-12  来源: 推导
- tb67h450_phase_off = 256  依据: 参�?tb67h450.c s_phaseA.sinMapPtr=(B+256)&0x3FF（A 领先 B 90°�?024/4�? 日期: 2026-08-12  来源: 推导
- tb67h450_oc_10bit = 2  依据: 参�?tb67h450.c SetTwoCoilsCurrent currentA>>2�?2bit DAC�?0bit TIM ARR=1023�? 日期: 2026-08-12  来源: 推导

## MT6816 �?磁性编码器（SPI 模式3, 14bit 绝对角度�?
- mt6816_resolution = 16384  依据: datasheet p1 "14位绝对角度数�? 0~16383  日期: 2026-08-12  来源: 推导
- mt6816_spi_mode = 模式3 (CPOL=1, CPHA=1)  依据: datasheet p20 "SPI使用模式3（CPOL=1, CPHA=1）传输数�?  日期: 2026-08-12  来源: 推导
- mt6816_spi_frame = 16bit, MSB first  依据: datasheet SPI �?16bit + 复刻参�?mt6816.c�?x80|CMD�?<8 16 位收�? 日期: 2026-08-12  来源: 推导
- mt6816_spi_prescaler = 8 (9MHz)  依据: 复刻参�?spi.c SPI_BAUDRATEPRESCALER_8�?2MHz/8=9MHz，datasheet TSCK�?4ns �?�?5.6MHz 满足�? 日期: 2026-08-12  来源: 推导
- mt6816_cmd_angle = 0x83�?x80|0x03�? cmd_raw_angle = 0x84�?x80|0x04�? 依据: 复刻参�?mt6816.c MT6816_CMD_ANGLE=0x03 / CMD_RAW_ANGLE=0x04  日期: 2026-08-12  来源: 推导
- mt6816_raw_angle = raw_data>>2�?4bit bit2~15�? 依据: 复刻参�?mt6816.c + 偶校验校验通过判定  日期: 2026-08-12  来源: 推导
- mt6816_parity = 偶数校验�? 表示数据奇校验失败）  依据: 复刻参�?mt6816.c CalcParity==0 通过  日期: 2026-08-12  来源: 推导
- mt6816_no_mag = bit1（无磁场标志�? 依据: 复刻参�?mt6816.c  日期: 2026-08-12  来源: 推导
- encoder_power_monitor = PA0 ADC1_IN0（POW_MT6816�? 依据: require.md 引脚映射（编码器供电监测�? 日期: 2026-08-12  来源: 推导
- mt6816_spi_spe_enable = 寄存器级传输前必须先�?SPE(CR1 bit6=0x40)；F103 HAL_SPI_TransmitReceive 每次调用前自�?__HAL_SPI_ENABLE 兜底，手写寄存器版漏掉则 SCK 不产生、RXNE 永不置位、死循环（写 DR 无效�?DR 读回 0 是铁证）  依据: 2026-08-15 /cl tim T3 调参实测（HAL 28µs→寄存器 12µs，缺 SPE 时首次读角度卡死�? 日期: 2026-08-15  来源: 实测

## USART �?调试回传与命令（复刻参考用 USART1 DMA，本项目调试回传 USART3�?
- usart3_baud = 115200  依据: require.md 调试�?+ usart.c MX_USART3_UART_Init BaudRate=115200  日期: 2026-08-12  来源: 推导
- usart3_8N1  no_flow_control  依据: usart.c WordLength=8B/StopBits=1/Parity=None  日期: 2026-08-12  来源: 推导
- usart3_rx_dma = DMA1_Channel3 CIRCULAR  依据: usart.c（调试回传接�?DMA 环形�? 日期: 2026-08-12  来源: 推导
- usart3_tx_dma = DMA1_Channel2 NORMAL  依据: usart.c  日期: 2026-08-12  来源: 推导
- usart1_baud = 115200  依据: 复刻参�?usart.c（RS485 命令通道，PA9/PA10 映射�? 日期: 2026-08-12  来源: 推导

## CAN �?命令总线（参考项�?CAN1 PB8/PB9 remap；本项目 PA11/PA12 默认映射�?
- can1_pins = PA11(RX)/PA12(TX)  依据: require.md 引脚映射（默�?CAN1，无 remap�? 日期: 2026-08-12  来源: 推导
- can1_bitrate = 500kbps  依据: 复刻参�?can.c 需按本项目 Prescaler=6/BS1=9TQ/BS2=2TQ 核对�?6MHz/(6×(1+9+2))=500kbps；参�?Prescaler=4/BS1=5TQ/BS2=4TQ �?900kbps，两者不一致，待实测确认实际总线�? 日期: 2026-08-12  来源: 推导

## ADC1 �?编码器供电监�?
- adc1_ch0_sampling = 239.5cycles  依据: 复刻参�?adc.c ADC_SAMPLETIME_239CYCLES_5（低内阻源稳定采样；本项�?CubeMX 默认 1.5cycles 待核对）  日期: 2026-08-12  来源: 推导

## 定时装配 �?控制 tick（motion_planner 积分基准�?
- control_frequency = 20000Hz  control_period_us = 50  依据: TIM4 PSC=71/Period=49 �?72MHz/(72×50)=20kHz（参�?TIM4 同款 20kHz 主控 tick，复刻时�?CubeMX tim.c 实测核对�? 日期: 2026-08-12  来源: 推导
- planner_tracker_list = CurrentTracker/VelocityTracker/PositionTracker/TrajectoryTracker（PositionInterpolator 参考死代码未调用，默认不移植，MODE_STEP_DIR 需时再补）  依据: 参�?motion_planner.c 函数调用图（PositionInterpolator 无调用点�? 日期: 2026-08-12  来源: 推导
- planner_global_decoupling = 参�?g_go_current/g_go_velocity/g_go_location/g_traj_go_* 外部全局 �?复刻为内�?s_go_* + getter（USR_MotionPlanner_*Tracker_GetGo*�? 依据: 分层规范（usr 模块内聚，收�?extern 全局耦合�? 日期: 2026-08-12  来源: 推导

## Flash 分区（F103RE 大容�?512K，页 2K=0x800�?
- flash_page_size = 0x800  依据: STM32F103RE 大容�?512K，页大小 2K（参�?stockpile_config.h 注释：RE=512K/64K + stm32f1xx_hal_flash_ex.h:246 STM32F103xE�?x800�? 日期: 2026-08-12  来源: 推导
- cali_table_addr = 0x08077800  依据: 512K 尾部�?2K 页对齐向下分配：固件 0x08000000 起，CALI 32K(16�?=0x08077800~0x0807F7FF，顶部留 DATA 2K(1�?=0x0807F800~0x0807FFFF（原参�?0x08017C00 �?128K 芯片布局，RE 需重排�? 日期: 2026-08-12  来源: 推导
- cali_table_size = 0x8000 (32K, 16K×2byte 校准�?  依据: 参�?stockpile_config.h STOCKPILE_APP_CALI_SIZE  日期: 2026-08-12  来源: 推导
- data_addr = 0x0807F800 (2K=1�?  依据: CALI 顶上一页，2K 页对齐，留给 boardConfig 持久�? 日期: 2026-08-12  来源: 推导
## 编码器校准（encoder_calibrator_usr，校准过程参数）

- cali_hard_steps = 200   sample_per_step = 16   auto_speed = 2   fine_speed = 1  依据: 复刻参�?encoder_calibrator.c 常量  日期: 2026-08-12  来源: 推导
- cali_enc_resolution = 16384  soft_divide = 256  subdivide_steps = 51200  依据: �?mt6816_resolution / SOFT_DIVIDE_NUM / MOTOR_SUBDIVIDE_STEPS 同源  日期: 2026-08-12  来源: 推导
- cali_current = 2000mA  依据: .cl/memory/ config_default_calib_current=2000（校准需足够力矩转两圈）  日期: 2026-08-12  来源: 推导
- cali_trigger = 上电同按 SW1(PB2)+SW2(PB1)  依据: 复刻参�?main.c 双键触发 + require.md 按键引脚（低有效�? 日期: 2026-08-12  来源: 推导
- cali_table_format = uint16[16384]，index=raw 角度，value=细分�?0~51199)，未校准�?0xFFFF 依据: 参�?encoder_calibrator.c GenerateTable + mt6816_usr 校验逻辑  日期: 2026-08-12  来源: 推导

## Button �?按键事件（SW1=PB2 / SW2=PB1，低有效�?
- button_num = 2  依据: 复刻参�?button.c BUTTON_NUM=2 + require.md 引脚映射（SW1/SW2 两键�? 日期: 2026-08-13  来源: 推导
- button_id_map = 1→SW1(PB2) 2→SW2(PB1)  依据: 参�?button.c �?PB12/PB2，本�?PB12=LED2(输出) 不可作按键，�?require.md �?SW1=PB2/SW2=PB1  日期: 2026-08-13  来源: 推导
- button_scan_freq = 100Hz  依据: 参�?main.c Tim1Callback100Hz �?Button_Tick（TIM1=100Hz 10ms 扫描�? 日期: 2026-08-13  来源: 推导
- button_long_press_ms = 3000  依据: 复刻参�?button.c LONG_PRESS_MS=3000�?s 判长按）  日期: 2026-08-13  来源: 推导
- button_active_level = 低有�? 依据: 复刻参�?button.c ReadPin == GPIO_PIN_RESET 判按�? 日期: 2026-08-13  来源: 推导

## 配置默认值（boardConfig 编译期，EEPROM 持久化后置）

- config_default_can = 2  default_mode = MODE_COMMAND_POSITION  current_limit = 1000mA  依据: 参�?main.c 配置无效分支默认�?+ 参�?uart_cmd.c defaultNodeID=2  日期: 2026-08-12  来源: 推导
- config_default_velocity = 30�?s = 30×51200 细分�?s  依据: 参�?main.c (velocityLimit=30*MOTOR_SUBDIVIDE_STEPS, MOTOR_SUBDIVIDE_STEPS=200×256=51200)  日期: 2026-08-12  来源: 推导
- config_default_velacc = 100�?s² = 100×51200 细分�?s²  依据: 参�?main.c (velocityAcc=100*MOTOR_SUBDIVIDE_STEPS)  日期: 2026-08-12  来源: 推导
- config_default_calib_current = 2000mA  依据: 参�?main.c 默认 calibrationCurrent  日期: 2026-08-12  来源: 推导
- config_default_pid = kp5/ki30/kd0  dce = kp200/kv80/ki300/kd250  依据: 参�?main.c 配置无效分支默认值（闭环调参起点，待实测整定�? 日期: 2026-08-12  来源: 推导
- config_default_enable = enableMotorOnBoot=false enableStallProtect=false  依据: 参�?main.c 默认（安全优先：上电不使能、不加堵转保护待实测�? 日期: 2026-08-12  来源: 推导
- motor_usr_subdivide = MOTOR_HARD_STEPS=200 / SOFT_DIVIDE_NUM=256 / MOTOR_SUBDIVIDE_STEPS=51200  依据: 复刻参�?motor.h  日期: 2026-08-12  来源: 推导
## 电机最小闭环（motor_usr，raw 角度驱动占位�?
- motor_usr_symbols = USR_Motor_Init / SetConfig / Tick20kHz / SetMode / SetPosition / SetVelocity / SetCurrent / SetDisable / SetBrake / GetState / GetPosition / GetVelocity / GetCurrent / GetMode / GetTelemetry / ClearStallFlag  依据: 复刻参�?motor.h 接口（USR_ 前缀�? 日期: 2026-08-12  来源: 推导
- motor_enc_raw_scale = ×25/8 (51200/16384)  依据: 未校�?选项B) raw 14bit(0~16383) 线性缩放进 51200 细分步空间；校准后由校准表接管（表输出即 51200 空间�? 日期: 2026-08-12  来源: 推导
- motor_foc_lead_90 = SOFT_DIVIDE_NUM=256  依据: 参�?motor.c CalcCurrentToOutput 正电流超�?负电流滞�?90°（细分步�? 日期: 2026-08-12  来源: 推导
- motor_compensate_angle = 分段补偿 |±430| �? 依据: 参�?motor.c CompensateAdvancedAngle（vel 阈�?100k/1.3M/2.2M，斜�?262/105/52>>20�? 日期: 2026-08-12  来源: 推导
- motor_minloop_control = 串级双环：位置环 err→速度目标（posKp=32768×err>>10 限±ratedVelocity）→ 速度�?err→电流（pidKp=10×err>>10 限±ratedCurrent�? 依据: 位置环标�?err=3200(POS_ERR_MAX)→额定速度 102400(2�?s)�?2768×3200>>10=102400；速度�?pidKp 5�?0 实测�? 输出上限 636mA 推不动齿隙摩擦→卡死�?0�?270mA 冲破，见 8/15 到位抖动根治�? 日期: 2026-08-15  来源: 推导+实测
- motor_cascade_poskp = 32768  依据: 串级外环标定：误差满量程 POS_ERR_MAX=3200 细分步→速度目标�?ratedVelocity=102400 细分�?s�?�?s），32768×3200>>10=102400；实�?128 时减速窗口缩�?672 �?滑行 512→冲过目标，32→窗�?3072 步正�? 日期: 2026-08-15  来源: 推导+实测
- motor_loop_damping = 速度环承担（pidKp×err 即阻尼；dceKd=400 退役→pidKd=400 速度环阻尼：速度误差→电流已含刹停；pidKd=800 实测饱和�?bang-bang 激励振荡）  依据: 8/13 实测 dceKd=400 收敛，串级重构将阻尼职责移交速度环，等效开环增�?posKp×pidKp>>10=160 �?�?dceKp=200 同量�? 日期: 2026-08-15  来源: 实测决策
- motor_loop_deadband_off = 到位死区 POS_DEADBAND=256 细分�?�?.8°)（目标�?.9°，静止区±0.35°实际落点 0.85° 内）内输出归�?制动；绕回窗口（目标≈编码器 0 �?51200±1024）内死区 256 + **0 电流**（保持力会把位置拽向 0 点毛刺最大处→摆动自激�? 依据: 8/15 方案Y 实测——死�?128 < 过冲 150~250 步→摆动�?0° 24 �?0° 33 行）；扩 256 吞过冲；删保持力（保持电流→编码器磁干扰→假速度±100000�?80°±260 摆动，cur-rv 同增同灭铁证）；进入死区 10ms 一次性制动刹停残余速度（持续死区速度环→假速度自激 30 行实测不可用）；0° 毛刺假速度会让制动输出推位置→绕回窗口不制�? 日期: 2026-08-15  来源: 实测决策
- motor_est_vel_filter = IIR 低通系�?1/32（integral += Δpos×20kHz + (v<<5 - v), v=integral>>5�? 依据: 参�?motor.c 速度估计  日期: 2026-08-12  来源: 推导
- motor_state_min = STOP/RUNNING/FINISH（最小闭环无过载/堵转/未校准检测，任务6后补�? 依据: 参�?motor.c 状态机裁剪  日期: 2026-08-12  来源: 推导
- motor_pos_deadband = 256 细分步（�?.8°，静止落�?0.85° 内）   motor_vel_deadband = 512 细分�?s   motor_cur_deadband = 10mA  依据: 8/15 方案Y——死�?128 不够（过�?150~250 步），扩 256 吞过�?+ 一次性制�?10ms 刹停 + 0 电流；vel/cur 死区 8/13 实测  日期: 2026-08-15  来源: 推导+实测
- motor_arrival_brake_ms = 10（进入死区后一次性速度环制动时长，20kHz �?200 帧内 10ms�? 依据: 8/15 方案Y——刹停残余速度防滑行过冲；一次性有限时长→无持续速度环→无假速度自激（持续死区速度�?8/15 实测 30 行自激不可用）�?° 绕回窗口不制动（毛刺假速度会让制动输出推位置→摆动�? 日期: 2026-08-15  来源: 实测决策
- motor_min_vel_push = MIN_VEL=30000 / WRAP 60000（细分步/s），�?|err|>MIN_VEL_DS=2048 �?err�? 时钳�? 依据: 静摩�?~250mA→vel_goal 下限�?0000（pidKp=10×30000>>10�?92mA 推得动）；MIN_VEL_DS=2048 减速窗口内 32×err 线性下坡低速进死区�?192@死区边缘），>2048 �?MIN_VEL 推进；绕回窗口用 60000�?° 段静摩擦大需强推�? 日期: 2026-08-15  来源: 实测决策
- motor_vel_goal_acc = 500（细分步/s²限斜率：每帧 vel_goal 变化 �?00�?00×20kHz=10M �?s² 加速度上限�? 依据: 8/15 方案Y—�?2×err 下坡限斜率整形防位置环阶跃冲量；实测到位速度�?5600 �?s 仍需 MIN_VEL_DS=2048 才冲不死�? 日期: 2026-08-15  来源: 实测决策
- motor_test_limits = currentLimit=2000mA�?2 步进额定 2A）、velocityLimit=2�?s（步进低速区间防失步，用户确认：10�?s 太快�? 依据: 用户反馈“步进电机限�?2000mA/10�?s 都太快”，�?2�?s  日期: 2026-08-13  来源: 实测决策
## motion_planner 实测值（2026-08-16 板端验证，任�?0�?
- planner_rated_velocity = 102400 细分�?s�?�?s�? 依据: 8/13 实测电机限速（用户确认 2�?s）；板测 A/B/C 加�?20ms 达限速后匀速、匀速段 vel �?102400 封顶  日期: 2026-08-16  来源: 实测
- planner_rated_velocity_acc = 5120000 细分�?s²�?00�?s²�? 依据: 参�?main.c velocityAcc=100*MOTOR_SUBDIVIDE_STEPS；板测加�?20ms 到限�?102400、减速距�?v²/2a=1024 �?与公式吻�? 日期: 2026-08-16  来源: 推导+实测
- planner_rated_current_acc = 2000  依据: 参�?main.c ratedCurrentAcc=2000 固定值（CurrentTracker 未在测试段驱动，仅配置注入）  日期: 2026-08-16  来源: 推导
- planner_trajectory_update_timeout = 200ms  依据: 参�?motor.c TrajectoryTracker_Init(200)；板�?D 阶段 200ms 仿真超时即停车（vel 20480�? 锁定�? 日期: 2026-08-16  来源: 推导+实测
- planner_speed_locking_brake = 5120�?ratedVelocityAcc/1000�? 依据: 参考推导（到位判据 |vel|�?120 直接�?0�? 日期: 2026-08-16  来源: 推导
- planner_signed_div_note = 积分器除法被除数必须为有符号：CTRL_FREQ 曾定�?20000U �?int32 负积分器(�?-5120000) 转无符号 4289847296 /20000U = +214492/�?�?减速变爆速（实测�?13 �?vel 102400�?1551636 指数爆涨，D 阶段 20480�?2704465）；�?20000（无 U）后 A/B/C 到位 vel==0 锁定、D 超时停车，PLAN_OK  日期: 2026-08-16  来源: 实测+复盘

## motor 完善�?026-08-16 验收，任�?04�?
- motor_velocity_kp_branch = 速度模式 Kp �?3、位置模式保�?10（S_CalcVelocityP 内按 mode 分支�? 依据: 8/16 VELOCITY 实测——Kp=10 �?500mA 推空载几�?ms 冲过目标→err 反号→反向满推→±1~3 �?s 往返极限环（周�?~200ms）；Kp=3 破环�?0.5 �?s 收敛（cur=0.255A 恒定，数值为 Kd±256mA 高频摆动+遥测采样相位锁定，功能正常）；POSITION 外环�?/15 已标定）保持 Kp=10  日期: 2026-08-16  来源: 实测
- motor_fake_vel_root_cause = 位置环极限环根因链：planner 连续软目标→vel_goal 每帧变→电流波动→编码器磁干扰假速度（�?5000~40000 �?s�?/15 记录 ±100000）→速度�?err 被污染→±2A 猛摆；VELOCITY 模式目标恒定→电流稳定→假速度�?→收敛；planner 段目标连续变化→自激→摆�?/15 �?planner �?MIN_VEL=30000 单向猛推收敛，planner �?MIN_VEL 冲过软目�? 依据: 8/16 fix3~fix6 多轮遥测对照  日期: 2026-08-16  来源: 实测+复盘
- motor_fake_vel_direct_drive = USR_MOTOR_FAKE_VEL_MAX=25000：|vel_goal|�?5000（低�?到位区）跳过速度环，位置误差 1:1 直驱电流（cur=goal-s_real_position mA，限 ±ratedCurrent，用 s_real_position 免超前角污染，s_vel_goal_last=0 防斜率残留），S_CalcCurrentToOutput 后直�?return；planner 未完成段 vel_goal=s_soft_velocity+((posKp×err)>>10) 轨迹速度主导+P 修正；planner 完成�?MIN_VEL 后同样判低速直�? 依据: 8/16 fix6——短行程 1273 �?FINISH（err=-113）、SW2 14 轮循环全 FINISH（落�?92~124 �?0.64~0.87°，死�?256 内）；长行程巡航�?vel_goal>25000 仍走速度环仍摆（已知限制�? 日期: 2026-08-16  来源: 实测
- motor_stall_sim_verdict = 堵转物理演示不可行：恒流 FOC 90° 领先角矢量跟�?无对抗力矩，1mA/5mA/50mA/200mA 空载都推得动�?mA�?5 �?s�?mA�?1 �?s�?0mA�?2 �?s�?00mA�?8 �?s，低电流反而略快疑与电流环动态有关）�?mm 细轴手捏只能减速到 ~1 �?s（滑动摩擦平衡）> 堵转阈�?0.2 �?s（STALL_VEL_MAX=10240），捏不死；TB67H450 为开环占空比映射（mA→DAC×5083>>12，无电流采样闭环），直流�?~100mA 含板子静态功�? 依据: 8/16 多轮实测（用�?手捏不死"+"轴太�?反馈）；STALL 行为链经 STALL_SIM 测试钩子实机验证：置位→1s 计时→STATE_STALL→输出睡眠（S_ZeroOutput+Sleep）→模式切换自动恢复（s_soft_new_curve �?s_is_stalled/s_stalled_time），2 次命中遥�?3,4；钩子验收后已删  日期: 2026-08-16  来源: 实测+复盘
- motor_long_stroke_limit = POSITION 多圈回程（planner 巡航段）偶发摆荡：vel_goal>25000 走速度环，planner 连续目标→电流波动→假速度自激 ±2A 猛摆（条件性非必现�?9:44 �?32 圈回程稳�?-1.7 �?s�?9:33 �?19 圈回程摆）；单圈域验收判据（SW2 循环）已过，长行程为已知限制，后续若需根治方向：巡航段电流规划器接管或直驱阈值覆盖巡�? 依据: 8/16 遥测对照  日期: 2026-08-16  来源: 实测

## DCE ���� B��2026-08-17 ����117��������մ�� ��1~7 �� �� �����ֲ �� ʵ��в����
- motor_dce_hold_pin = ���ֶ� focpos=location ������ǣ����ƽ�� �ȡ�goal���ŵ��ɸն� Kt��|I|��Nr �ֿ� detent������ɻ���/�������ֱ��ʾ��������˶��� est��256 ��ת�� + ���ֺ��ٶ��� kv��vError�����ֶλ��ֽ�λ���� ki��pError + ȥ kd��R2 ����γ��/�ٶ������֣�  ����: Simulation/model/firmware_controller.m ���� B ��������� 4 Ŀ����� +0/+0/-1/-5 ��ȫ��꣨��11�����ο�ԭ�� focpos=est��256 ���������=0 �� detent ���� +112~141 ��δ���  ����: 2026-08-17  ��Դ: �����Ƶ�+���ʵ��
- motor_dce_keep_win = ���� 256 / ��� 128��ϸ�ֲ���2:1 �ͻأ�����ع̼���  ����: R2 ��ֲ���ͻط��߽綶�������ֶγ���˲�� kd �ز� + ���ֺ��ٶ��� �� ��λ���� 45�� ��ǳ����  ����: 2026-08-17  ��Դ: �Ƶ�
- motor_dce_hold_ma = 1500mA��R1 300 �� R2 500 �� R3 1500�����������300mA detent ����������� ��27 ������ 500mA ���� +27 �� 1500mA �˶��� 27��24 ����С�䣩  ����: R3 ����������detent 0.03Nm/Kt 0.152��200mA �����ԣ�������ֻ������� ki��err>>7 = 63mA/֡ �� 1500mA �� ~1.2s �� TEST_A SETTLE �ȴ��� 3000ms  ����: 2026-08-17  ��Դ: ʵ�����
- motor_dce_residual_root_cause = ������вSETTLE �ȶ��� 90���-10/180���+24/270���+6/360���+12~15 ������� 0.169�㣬�о� ��11 �� 4 �� 52~62% �ϸ�Ϊ**������У׼����ֵ�в�**���Ǳջ��������� �����޹أ�300��500��1500mA �� 27��24��settle ң�� cur=-1500mA ��ʵ������������������ �������������ȫ��� ��5 �� �� �ڷ���������� 90�� ê�����Բ�ֵ�в� [12,-10,24,6,12] �󣬸���ʵ��в���� -24/+10/-3/-17��ʵ�� err Ϊ����ϵ������Ϊ����ϵ�����ž�����֤������ ����ϵ������ goal�����ֶ�ס reported=goal����������� = goal-�в�  ����: 2026-08-17  ��Դ: ʵ��+�������
- motor_dce_criterion = �о� |err|�ܡ�11 ϸ�ֲ� = 0.077�㣨�û� Q1A �İ壩����Ʒ���� ��0.9�㣬�ֲв� 0.169�� Ϊ���� 1/5���޷������ر궨�������� / ���������� / �ſ��оݣ����û����ߣ�  ����: 2026-08-17  ��Դ: �û�����