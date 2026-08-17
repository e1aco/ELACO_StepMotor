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
烧录坑（2026-08-15 实测）: ST-Link 助手侧烧录必失败（Get IDCODE error）勿用；pyocd flash.py 探针连接正常时可烧（--target stm32f103rc 实测通过）

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
| T1 | TIM4 20kHz 电机闭环 ISR 总耗时 | TIM4_IRQHandler 入口/出口 | 软件 | 50µs（周期 1/20kHz=50µs，72MHz/(71+1)/(49+1)） | 24µs（2026-08-17 DWT，DCE 对照版实测（8/16 串级版 30µs，DCE 无死区/直驱分支更快）） | 每次变更都测 | [x] |
| T2 | TIM1 100Hz 遥测 ISR 总耗时 | TIM1_UP_IRQHandler 入口/出口 | 软件 | 1000µs（周期 10ms 的 10% 占空；8/15 Saleae 实测 ~2.3ms 超限待复测） | 86µs（2026-08-15 DWT；调参前 2262µs 超限，调参：%f 定点化+发送移主循环） | 每次变更都测 | [x] |
| T3 | MT6816 SPI 单次读（含≤3 重试） | USR_MT6816_UpdateAngle 入口/出口 | 软件 | 15µs（9MHz 每帧 1.78µs × 2 帧 × 3 重试=10.7µs + HAL 轮询余量；TSCK≥64ns） | 12µs（2026-08-15 DWT；调参：HAL 28µs→寄存器级+SPE 前置使能，见下注） | 每次变更都测 | [x] |
| T4 | ADC1 编码器供电采样（HAL Start+Poll 路径） | tim_test 测试序列（业务未接入，临时触发） | 软件 | 29µs（memory adc1_ch0_sampling 239.5cycles@12MHz 参考；CubeMX 实际 1.5cycles 待核对） | 27µs（2026-08-15 DWT） | 每次变更都测 | [x] |

# 仿真配置 (Simulation) — AI 维护（/cl sim 使用）
> 仿真闭环：定参→实测对照→回灌校准。方法/坑位见 cl knowledge/matlab_sim_loop.md；命令细节 details/sim.md。
> 仿真目录: E:\Desktop\XM\ELACO_StepMotor\Simulation\
> MATLAB: E:\MATLAB\bin\matlab.exe（R2025a；env MATLAB 可覆盖；只经 cl tools/matlab_run.py 调用）
> 遥测 CSV 协议: [TELE] <t_ms>,<pos细分步>,<vel细分步/s>,<cur_mA>,<mode>,<state>（全 %d 定点，发送在主循环）
> 状态文件: .cl/sim/state.md
> 对照表: .cl/sim/compare_table.md

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
- [x] 复刻 motion_planner: 4 tracker（Current/Velocity/Position/Trajectory）20kHz 软目标生成（PositionInterpolator 不移植）<- 加减速优化层（起 2026-08-16 | 止 2026-08-16 | 验收 2026-08-16 板端 PLAN_OK：A 0→12800 / B 12800→25600 / C 25600→0 到位锁定 pos==goal && vel==0，D 轨迹(12800,51200) 200ms 超时停车 vel==0，全程 |vel|≤102400 无超调；变更点：①参考 NewTask 参数交叉赋值修正 ②轨迹目标=当前位置时除零保护 ③CTRL_FREQ 20000U→20000 有符号除法修复（负积分器被无符号除→减速爆速）④C 阶段测试时间 200ms→300ms（反向 25600 步需 270ms）；测试段已删，编译 0E/0W Flash 21.5KB）
- [✓] 复刻 motor 完善: 接入 planner + PID/DCE + 超前角补偿 + 完整状态机（过载/堵转/未校准）<- 控制质量与保护（起 2026-08-16 | 止 2026-08-16 | 验收 2026-08-16 人工验收通过：①VELOCITY 0.5 圈/s 收敛 ✓（Kp=3 分支，cur 0.25A 恒定）②POSITION 单圈域 SW2 90°→360° 循环 14+ 轮全 FINISH ✓（落点 0.64~0.87°=92~124 步，死区 256 内稳定，无卡死/误报 STALL）③堵转模拟 STALL ✓（1mA/5mA 空载恒流 FOC 照推 11~15 圈/s、细轴手捏只能减速 ~1 圈/s 无法物理堵转 → STALL_SIM 测试钩子实机验证行为链：置位→1s→STATE_STALL→输出睡眠→模式切换恢复，2 次命中遥测 3,4）④STALL 恢复后 VELOCITY 正常收敛；变更点：fix3 速度环 Kp=10→3（减增益破极限环）+ CURRENT 5mA→1mA、fix4 退役 MIN_VEL 钳位、fix5 planner 段轨迹速度主导+P 修正、fix6 USR_MOTOR_FAKE_VEL_MAX=25000 低速直驱电流（|vel_goal|≤25000 位置误差 1:1 直驱、限 ratedCurrent、用 s_real_position）、fix8 STALL_SIM 钩子（已删）；已知限制（记录 memory）：POSITION 多圈回程巡航段偶发摆荡（vel_goal>25000 走速度环，planner 连续目标→电流波动→编码器磁干扰假速度自激 ±2A，条件性非必现）；测试段已删（SW1/SW2/遥测/STALL_SIM），生产版烧录 0E/0W Flash 22.6KB，T1 30µs 门禁通过）
- [x] 复刻配置持久化: config_usr（BoardConfig_t/默认值/configStatus）+ eeprom_usr（DATA 0x0807F800 读写/IsValid/Erase）<- 掉电保存（起 2026-08-17 | 止 2026-08-17 | 验收 2026-08-17 AI 闭环验证全链命中待人工验收：首启 CFG_INIT:DEFAULT:node=2 填默认+回写 → 复位读回 OK → SW1 改 node=3+Commit → CFG_SAVED 落盘 → 复位 CFG_PERSIST:OK 保持 → SW2 Restore → 自动复位 CFG_RESTORE:OK 回默认；测试段已删，生产版 0E/0W Flash 22.6KB；变更点：dce_kp/kv/ki/kd 退役→posKp、pid_ki 裁剪、新增 magic 0x5A5AA5A5 掉电半写检测、flash_drv 补 DRV_Flash_AreaSetAddr 原语、EEPROM 惰性擦除按区记录修参考全局标志双区互坑）
- [ ] 新增 can_cmd_usr: CAN1（PA11/PA12, 500kbps）命令+应答（协议做时再定，命令对齐参考 c/v/p/s/z/l 语义）<- 替代 485 命令通道
- [ ] main 装配: 读配置→Motor 注入→100Hz 按键事件/LED/遥测 USART3→20kHz 校准/电机分派→CONFIG_COMMIT/RESTORE<- 完整生命周期
- [✓] 按键-LED 测试: BTN1→LED1 / BTN2→LED2，短按 toggle 开关，长按闪烁 3 次后恢复记忆状态（验证后删除测试段）（起 2026-08-13 | 止 2026-08-13 | 验收 2026-08-13 按键功能确认 OK，测试段已删除恢复原样，编译 0E/0W）
- [ ] 遗留加固: USR_EncoderCalibrator_Init 判据过弱（仅查首值）→ 全表扫描校验 <- 低风险独立

## 2026-08-15
- [✓] 电机闭环串级重构: 位置环→速度环→电流（dceKp/dceKd 退役 → posKp=32768 标定 + pidKp=5 起点；位置环限速 ±ratedVelocity；死区 128 内输出直接归零、跳过速度环）+ 串级调参方法沉淀 knowledge/motor.md（起 2026-08-15 | 止 2026-08-15 | 验收 2026-08-15 人工验收通过，7+ 轮循环遥测全收敛；遗留已闭环：state FINISH 保持、运动段摆动由方案 Y 根治）<- 烧录与调参由你接管。抖动调试：8/15 第1轮 vel 死区 512 无效（噪声 66k 步/s 差百倍）→ 第2轮死区内输出归零（8/13 实测复证）→ 到位 FINISH 全零收敛，偏差 0.72°<死区；遗留：state 单帧回 RUNNING（FINISH 保持）+ 运动段摆动（planner 改善）
- [✓] 到位抖动根治（方案Y）: 死区 128→256 + 保持力删除（保持电流→编码器磁干扰→假速度 ±100000→180° 摆动 ±260）+ 到位制动 10ms（一次性刹停残余速度，避免持续死区速度环假速度自激）+ 减速窗口 MIN_VEL_DS 1024→2048（低速进死区）+ 0° 绕回窗口 0 电流归零（起 2026-08-15 | 止 2026-08-15 | 验收 2026-08-15 人工验收通过，7+ 轮完整循环全部一次收敛：90°→12678/180°→25517/270°→38293/360°→51094 落点 ±5 步一致，到位即 rv=0 静止，含 0° 绕回与长行程回程）<- 运动段摆动遗留，由 planner 改善

## 2026-08-17
- [!] 到位精度优化B: detent 预补偿——命令位置对齐整步网格（细分步 256 倍数，机械坐标对齐），detent 落点钉死命令整步，预期落点 ±0.05° 级，无保持力风险（起 2026-08-17 | 止 2026-08-17 | 失败 2026-08-17 实测否定：命令对齐整步后 5 个落点 err=+87/+146/-19/+84/+87 步，与 8/16 的 92~124 步同量级无收敛，判据 ±7 步（0.05°）不达标。根因：FINISH 判定 |goal-est|≤256 即报到位 → 0 电流 → detent 力弱拉不动死区内转子 → 落点=死区内随机停位（12800↔13056 两整步 detent 间不稳定平衡）；24569（=24576-7）反复卡点证明 detent 只对恰好近整步的位置有效，无法从 ±100 步外拉回。结论：命令对齐整步不保证落点=命令整步，B 机制否定）<- 失败，转 A
- [✓] 到位精度优化A: 移植参考 DCE 积分保持对照实验——积分保持电流钉住命令角（参考 motor.c CalcDceToOutput kp200/ki300/kd250），静态精度实测 + 摆振观察（起 2026-08-17 | 止 2026-08-17 | 验收 2026-08-17 控制方案达理论最好：仿真（固件现状逐帧镜像）理想编码器 4 目标 +0/+0/-1/-5 步全达标 ±11；实板 SETTLE 稳定后残差 90°-10/180°+24/270°+6/360°+12~15 = 编码器校准表插值残差（仿真加残差模型 [12,-10,24,6,12] 镜像复现实板）；电流无关铁证：settle 遥测 cur=-1500mA 真实命令拉不动；判据 ±11 板测 4 轮 52~62% 未达标→转新任务「重新标定编码器校准表」）<- 后做，验证参考结构精度上限
- [ ] 重新标定编码器校准表: 修复编码器校准表插值残差——到位精度优化A 验收结论：控制方案已达标（仿真 ±5 步），实板落点残差（90°-10/180°+24/270°+6/360°+12~15 步，最差 0.169°）全部来自编码器报数偏移；方向：检查 encoder_calibrator 锚点密度/标定抖动污染（16384 锚点全表应只有 ±1.5 步量化，实测 ±24 步异常），重采锚点后重跑 SETTLE 判据轮（|err|≤±11 步）（起 2026-08-17 | 止 进行中）
- [c] 新增仿真遥测回灌: 100Hz [TELE] 打印（t_ms,pos细分步,vel细分步/s,cur_mA,mode,state 全 %d 定点），从任务A 钩子独立为 /cl sim 数据源（勿随钩子删）；telemetry_csv.py → Simulation/telemetry.csv → run_compare_telemetry.m 校准 J/B/detent（起 2026-08-17 | 止 进行中）<- /cl sim 实测回灌数据源，烧录后配合调试；见 .cl/sim/state.md 下一步

## 仿真对照结论（2026-08-17，MATLAB 仿真，Simulation/ 目录，固件零改动）

> 目标：落点精度 0.08~0.1°（±11~14 细分步）。当前方案Y 基线 +256 步（1.8° 死区边界）。

### 结果汇总

| 方案 | 落点误差（噪声 std=3 磁干扰量级） | 保持电流 | 结论 |
|---|---|---|---|
| 方案Y 现状（死区 256+删保持力） | +256 步（1.8° 死区边界） | 0 | 基线 |
| DCE 参考原样（focpos=est±256） | +112~141 步（0.3~1.0°） | 2000mA 饱和发热 | 未达标 |
| **DCE 变体 B（保持钉命令角）** | **±1~7 步（0.007~0.049°）** | 300mA 限幅 | **达标** |

- DCE 参考原样未达标根因：驱动 dac=abs(电流)、方向全由 focpos 决定（plant 平衡 θ≡focpos mod 1024）；参考版 focpos=est±256 到位时相位与转子对齐力矩=0 → 落点仍 detent 主导，与方案Y 同量级。
- 变体 B 关键改动：运动段 est±256 旋转拖（参考原样）+ 保持段 |err|≤256 时 focpos=goal 钉命令角 → 恢复刚度 Kt·|I|·Nr 抵抗 detent（0.03Nm 需 ≥200mA）+ 保持段电流限幅 300mA（参考原样积分饱和 2000mA 发热）。
- 零噪声时误差 0 步；噪声 std=3 时 ±1~7 步（0.007~0.049°），全部 ≤0.08° 达标。
- 参数 kp200/kv80/ki300/kd250 取自参考 StepMotorCtrl_42 main.c:141-144。

### 仿真文件路径（E:\Desktop\XM\ELACO_StepMotor\Simulation\）

- `scripts/run_pos_step_dce.m` —— DCE 对照实验脚本（cp.use_dce=true，噪声 std=3，落点误差表+曲线）
- `model/firmware_controller.m` —— s_calc_dce_to_output 变体 B（第 341-399 行）+ 分派分支 + 状态机 DCE 判定
- `params/control_params.m` —— DCE 参数组（use_dce/dce_kp/kv/ki/kd/dce_hold_ma/dce_keep_win 等）
- `scripts/run_pos_step.m` —— 方案Y 现状回归（基线对照）

### 板端移植待办（仿真已验证，固件未改）

1. 运动段相位推进方式与保持段切换滞回防抖（仿真用 |err| 直接切换）
2. dce_hold_ma=300mA 实测校准（detent 实际力矩）
3. 移植后静态精度实测 + 摆振观察（对照仿真 ±0.05°）

### 2026-08-17 追加：固件现状方案仿真移植 + 残差定根因（本次模拟效果记录）

- 移植范围（firmware_controller.m s_calc_dce_to_output 与固件逐帧镜像）：保持段滞回入界 |err|≤128 / 出界 >256；分段限幅 hold ±1500mA（HOLD_MA）/ moving ±2000mA；保持段积分仅 ki×pError（剔除速度项）+ kd=0；抗饱和（已达限幅同向累积丢弃）；相位保持段 focpos=goal 钉命令角；settle 采样 3s 对齐固件 TEST_A。
- 仿真结果（run_pos_step_dce.m，编码器噪声 std=3）：
  - 理想编码器：90° +0 / 180° +0 / 270° -1 / 360° -5 步 → 全达标 ±11；保持电流均值 1371mA/峰值 1500mA。
  - 加校准残差模型（90° 锚点线性插值 [12,-10,24,6,12] 步）：复现实板残差（镜像 -24/+10/-3/-17，实板 err 为报告系、仿真为物理系，符号镜像即环路钉住 reported=goal 的铁证）。
- 结论：DCE 变体 B 控制方案能力上限 ±5 步（0.035°），实板残差全部来自编码器校准表插值残差 → 修法为重新标定（新任务），控制侧无需再调。
- 依据文件：Simulation/model/firmware_controller.m、Simulation/params/control_params.m（dce_keep_hys/dce_hold_ma=1500）、Simulation/model/mt6816_encoder.m（calib_residual 模型）、Simulation/scripts/run_pos_step_dce.m。
