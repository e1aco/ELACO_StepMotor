# Simulation — ELACO 步进电机 MATLAB 仿真（与固件协同）

基于硬件参数的电机仿真：**固件控制算法 1:1 复刻 + 电机电气-机械模型**，用于验证/整定固件参数、参数辨识、算法预研，并为 C 代码移植提供对照。

> 状态：脚本版（阶段 1）已完成；Simulink 模型（阶段 2）提供骨架生成脚本。
> ⚠️ 固件代码正在修改中 —— 本目录**只读固件、不改固件**；`[TBC]` 标记处待代码稳定后复核。

## 目录结构

```
Simulation/
├── README.md
├── params/                    # 硬件参数（改这里即可整定）
│   ├── motor_params.m         # 电机：R/L/保持力矩/步进角（require.md）+ J/B/detent（估算）
│   ├── drive_params.m         # TB67H450：VM=24V、12bit DAC、正弦表
│   ├── encoder_params.m       # MT6816：14bit -> 51200 细分步、噪声模型
│   └── control_params.m       # 固件控制参数镜像（motor_usr.h 常量 + main.c 配置）
├── model/
│   ├── plant_step_motor.m     # 两相混合式步进电机电气-机械模型
│   ├── tb67h450_drive.m       # 复刻 USR_TB67H450_SetFocCurrentVector
│   ├── mt6816_encoder.m       # 复刻编码器 14bit + raw*25/8 映射
│   ├── firmware_controller.m  # 复刻 USR_Motor_Tick20kHz 12 步流程（核心）
│   ├── firmware_controller_init.m
│   └── build_simulink_model.m # 生成 Simulink 模型（需 Simulink 许可）
└── scripts/
    ├── run_pos_step.m         # 位置阶跃 90/180/270/360°（复刻 SW2 验收）
    ├── run_pos_step_dce.m     # DCE 积分保持对照（到位精度优化A，目标 0.08~0.1°）
    ├── run_vel_ramp.m         # 速度模式 0.5/1.0 圈/s
    ├── run_cur_step.m         # 电流模式 200/800mA
    ├── run_tune_gain_sweep.m  # posKp/pidKp/pidKd 网格扫描整定
    ├── run_identify.m         # 参数辨识（Kt/J/B，电流阶跃激励）
    └── run_compare_telemetry.m# 遥测 CSV 对比（数据就绪即可用）
```

## 快速开始

```matlab
cd Simulation
run('scripts/run_pos_step.m')        % 位置阶跃用例（复刻板上 SW2 验收）
run('scripts/run_tune_gain_sweep.m') % 增益扫描整定（约 1~3 分钟）
run('scripts/run_identify.m')        % 参数辨识流程验证
```

## 仿真架构

```
firmware_controller(20kHz, 复刻固件 motor_usr.c)
   -> tb67h450_drive(复刻 FOC 查表: sin表1024点/A相超前B相90°/mA->12bit DAC)
   -> plant_step_motor(电气: L di/dt = v-Ri-e; 机械: J dω/dt = Te-Bω-Td)
   -> mt6816_encoder(14bit量化 + 校准映射 + 可选磁干扰噪声)
   -> 回 firmware_controller —— 闭环
```

## 参数来源

| 参数 | 值 | 来源 |
|---|---|---|
| 步进角 | 1.8°（200 步/圈，50 对极） | require.md |
| 保持力矩 | 0.43 Nm | require.md |
| 相电阻 R | 2 Ω | require.md |
| 相电感 L | 3.6 mH | require.md |
| 额定电流 | 2 A | require.md |
| 供电电压 VM | 24 V | 用户确认 |
| 转子惯量 J | 5.4e-6 kg·m² | 42 系列典型值（估算） |
| 粘滞摩擦 B | 2e-3 Nm·s/rad | 估算（欠阻尼下限 3e-4 会失步，见调试记录） |
| 库仑摩擦 Tf | 0.02 Nm | 估算（同上） |
| 齿槽转矩 | 0.03 Nm | 42 典型 ~7% 保持力矩（估算） |
| Kt | 0.152 Nm/A | 由保持力矩反推（估算） |
| 控制参数 | posKp=32768 / pidKp=10 / pidKd=400 | main.c s_motor_config |

**估算参数（J/B/Tdetent/Kt）待辨识**：`run_identify.m` 用电流阶跃响应拟合；有固件遥测 CSV 后 `run_compare_telemetry.m` 校准闭环。

## 固件 <-> 仿真对照表（复刻范围）

| 固件 | 仿真 | 说明 |
|---|---|---|
| motor_usr.c Tick20kHz（12 步） | firmware_controller.m 主函数 | 逐帧 1:1 |
| S_CompensateAdvancedAngle | s_compensate_advanced_angle | 三段斜率 >>20 |
| S_CalcPositionCascade | s_calc_position_cascade | 死区/滞回/MIN_VEL/绕回/假速度直驱 |
| S_CalcVelocityP | s_calc_velocity_p | Kp/Kd 定点格式 [TBC] |
| S_CalcCurrentToOutput | s_calc_current_to_output | 电流模式 |
| motor.c CalcDceToOutput（参考） | s_calc_dce_to_output | DCE 积分保持对照（use_dce=true，变体 B） |
| motion_planner 4 tracker | pl_*_tracker | Current/Velocity 完整；Position/Trajectory 梯形近似 [TBC] |
| tb67h450_usr SetFocCurrentVector | tb67h450_drive | 查表/相位/DAC 映射 |
| 堵转/过载/状态机 | fw_state_machine + 步骤 11/12 | 1:1 |

## [TBC] 待代码稳定后复核清单（用户正在改代码）

1. `S_CalcPositionCascade` 精确分支（MIN_VEL 钳位位置、直驱用 s_real_position 还是 s_est_position）
2. `S_CalcVelocityP` Kp/Kd 的 Q 格式（当前 q_pidkp/q_pidkd=0 假设）
3. `s_foc_position` 更新方式（当前=命令微步积分推进，见下"调试记录"）
4. `PositionTracker`/`TrajectoryTracker` S 曲线细节（当前梯形近似）
5. 死区保持力宏是否已彻底移除（方案Y）
6. 死区滞回/制动时序细节

## 调试记录（2026-08 首轮闭环收敛）

1. **planner 顺序**：控制分派必须先于 planner（先用本帧软目标），否则死区电流=0 拖住启动 → 已交换步骤 8/9/10 顺序
2. **FOC 电角度死锁**：foc_position 曾跟随 est_position（编码器未动 → 电角度不动 → 无旋转磁场 → 死锁）；改为**命令微步积分推进**（foc_integral 累加速率，每帧取整进给，余量保留；速度环路径在 s_calc_velocity_p 内部推进，假速度直驱分支在 cascade 内推进，避免双重推进）
3. **bitshift ASSUMEDTYPE 报错**：MATLAB bitshift 对 double 要求 int32 范围内整数，C 中乘法溢出环绕（如 dvel*pidKd 可超 2^31）会直接报错 → 引入 `sra()`（wrap32 环绕 + 算术右移 floor 语义，与 C int32 >> 一致）替代全部 bitshift
4. **欠阻尼振荡失步**：Tc=6e-3/Bv=3e-4 时闭环发散（电机乱转、位置估算反向）；调至 Tc=0.02/Bv=2e-3 后 4 段阶跃全部 FINISH（误差=死区 256 步，符合"死区不保持力"设计）。真实值待 run_identify 用实测标定
5. **run_identify 方法**：电流阶跃合成验证发现滑行辨识被摩擦极限环污染（τ=J/B=2.7ms 极短）→ 改为双稳态差分（B/Tf）+ 加速段 LS（J），合成验证误差 J±1%、B±2%、Tf±10%；实测需 Kt（板端堵转校准）+ 遥测 CSV

## 到位精度优化A 对照结论（2026-08-17，DCE 积分保持）

目标：落点精度 0.08~0.1°（±11~14 细分步）。方案Y 现状 ±0.64~0.87°（死区 256 内 0 电流，detent 决定落点）。

| 方案 | 实现 | 落点误差（噪声 std=3） | 结论 |
|---|---|---|---|
| 方案Y 现状 | 死区 256 + 删保持力 | +256 步（1.8°，死区边界） | 基线 |
| DCE 参考原样 | motor.c CalcDceToOutput，focpos=est±256 | +112~141 步（0.3~1.0°） | 未达标——零误差时相位对齐力矩=0，落点仍 detent 主导 |
| **DCE 变体 B** | 运动段 est±256 旋转拖 + **保持段 focpos=goal 钉命令角** + 保持电流限幅 300mA | **±1~7 步（0.007~0.049°）** | **达标** |

变体 B 关键：驱动 dac=abs(电流)、方向全由 focpos 决定（plant 平衡 θ≡focpos mod 1024）。保持段相位钉命令角 → 恢复刚度 Kt·|I|·Nr 抵抗 detent（0.03Nm 需 ≥200mA）。零噪声时误差 0 步。

已在仿真实现（`cp.use_dce=true` 分支），未改固件；参数 kp200/kv80/ki300/kd250 取自参考 main.c:141-144，保持限幅 dce_hold_ma=300mA 为仿真增强（参考原样积分饱和 2000mA 发热）。板端移植需：运动段相位推进方式复核 + 保持段切换滞回防抖 + 实测调 dce_hold_ma。

## 整定回写流程（配合产生控制代码）

1. `run_tune_gain_sweep.m` 扫描 posKp/pidKp/pidKd → 指标表 → 推荐参数
2. 确认后手动回写 `config_usr.h`（`USR_CONFIG_DEF_*`）—— **回写前先与我确认**
3. 算法改进（如 DCE 积分保持）先在仿真验证 → 按对照表移植 C → 板端遥测对比闭环

## Simulink 用法（阶段 2，需 Simulink 许可）

```matlab
cd Simulation/model
build_simulink_model      % 生成 step_motor_closedloop.slx 骨架
```
生成后手动接线（Clock->CmdGen->System->Scope），函数体已自动注入，
逻辑与 run_*.m 共用同一套 model/*.m，结果一致。

## 已知假设与限制

- 电机 J/B/detent 为估算，靠辨识+实测校准
- 未建模：PWM 载波纹波、TB67H450 内部限流/死区、温度、齿隙弹性
- 编码器噪声默认关；开启（encoder_params.m noise_std=1~3）可复现板端"假速度"现象
- 无 Simulink Coder 依赖：代码协同走"参数回写 + 1:1 结构移植对照"
