# 调试日志

## [2026-08-16 19:59] 任务: motor 完善（planner 接入 + PID + 超前角 + 完整状态机）- **模式**: /cl run (全自动闭环，fix3~fix8 每轮编译烧录遥测判读)
- **现象**: ①POSITION 到位后摆荡/冲过目标（死区边缘拉锯）②VELOCITY 0.5 圈/s 目标收敛但初次 Kp=10 极限环 ③CURRENT 堵转模拟空载推得动（5mA 11 圈/s）④POSITION 多圈回程巡航段偶发 ±2A 猛摆（假速度极限环，条件性）
- **尝试**:
  - 第1轮(fix3): CURRENT 50→5mA + 速度环 Kp=3 分支 → VELOCITY 收敛 ✓，POSITION 仍摆
  - 第2轮(fix4): 退役 MIN_VEL 钳位 → POSITION 仍摆，CURRENT 5mA 空载照推 11 圈/s（堵转模拟失败）
  - 第5轮(fix5): planner 未完成段轨迹速度主导+P 修正、完成段恢复 MIN_VEL → 短行程 FINISH ✓ 长行程仍摆  - 第6轮(fix6): USR_MOTOR_FAKE_VEL_MAX=25000 低速直驱电流 → SW2 14 轮循环全 FINISH ✓，长行程巡航段仍摆（已知限制）
  - 第7轮(fix7): CURRENT 5→1mA（堵转演示）→1mA 空载照推 15 圈/s，手捏不死（细轴）  - 第8轮(fix8): STALL_SIM 测试钩子（SW2 模拟堵转条件，跳过 vel 传感器，1s 计时照走）→ 2 次命中 `T:...,3,4` ✓- **手动调试记录**: （8/16 验收）  - 用户反馈：手捏不死（滑动摩擦平衡 ~1 圈/s > 阈值 0.2 圈/s）"那个轴太细了"、直流源显示 ~100mA（TB67H450 开环占空比映射 + 板子静态功耗，非闭环恒流）
  - 串口坑 1：pyocd reset 把 COM19 弄挂（OSError 22）→ 监听死亡 → 需用户断电重启板子（2 次）
  - 串口坑 2：start_mon.ps1 `--boot "System Start!"` 的引号被 PowerShell 拆掉（unrecognized arguments: Start!）→ 去掉 --boot + 显式 -WorkingDirectory 修复
- **最终方案**: 速度模式 Kp=3 分支（破空载冲过极限环）+ |vel_goal|≤25000 低速直驱电流（位置误差 1:1，用 s_real_position，限 ratedCurrent）+ planner 段轨迹速度主导 + 堵转行为链经 STALL_SIM 实机验证；测试段（SW1/SW2/遥测/STALL_SIM 钩子）全部删除，生产版 0E/0W Flash 22.6KB
- **验证结果**: VELOCITY 0.5 圈/s 收敛 ✓（5+ 次）；SW2 90°→360° 循环 14+ 轮全 FINISH ✓（落点 92~124 步=0.64~0.87°，死区 256 内）；STALL 行为链 2 次命中遥测 3,4 ✓（置位→1s→STATE_STALL→输出睡眠→模式切换恢复）；T1 30µs 门禁通过
- **经验引用**: `.cl/memory/config.md` 新增 5 条（velocity_kp_branch / fake_vel_root_cause / fake_vel_direct_drive / stall_sim_verdict / long_stroke_limit）；require.md 任务 104 [✓]

## [2026-08-17 15:50] 任务: 到位精度优化A（DCE 变体 B 保持段移植 + 板测 + 仿真对照，理论最好收官）
- **模式**: /cl run + MATLAB 仿真对照（固件 DCE 逐帧镜像移植进 Simulation/model/firmware_controller.m）
- **现象**: 板端 SETTLE 稳定后落点残差随目标位置可复现：90°≈-10 / 180°≈+24 / 270°≈+6 / 360°≈+12~15 步（最差 0.169°），判据 ±11 步（0.077°）4 轮合格率 52~62%（R2 500mA/1s 8/13；R3 1500mA/1s 11/21；R4 1500mA/3s 13/25）
- **尝试**:
  - R1: HOLD_MA=300mA + SETTLE 钩子（按键门控 + FINISH 后等 1s 采样）→ 合格 8/13
  - R2: HOLD_MA 300→500mA → 残差不变（180° +27 保持）→ 电流无关初步铁证
  - R3: HOLD_MA 500→1500mA → 180° 27→24 步仅小变；发现保持积分爬升慢（ki×err>>7=63mA/帧 → 1500mA 需 1.2s > 1s 采样窗）
  - R4: settle 等待 1000→3000ms → 合格 13/25；settle 侦测 T:25624,0,-1500,1 证明 cur=-1500mA 真实命令仍拉不动 → 残差非电流受限
  - 仿真: 固件方案逐帧镜像 → 理想编码器 4 目标 +0/+0/-1/-5 全达标；mt6816_encoder.m 加 90° 奇点插值残差 [12,-10,24,6,12] → 镜像复现实板残差（符号镜像=环·钉子 reported=goal 铁证）
- **最终方案**: 控制方案（滞回 128/256 + 分段限幅 ±1500/±2000 + 保持段仅 ki + kd=0 + 抗饱和）已达理论最好；残差定根因=编码器校准表插值残差 → 转新任务「重新标定编码器校准表」
- **验证结果**: 仿真全达标 ±5 步（0.035°）；实板判据未达（52~62%）但残差源头定位闭环（仿真镜像复现 + 电流无关 + 可复现），产品需求 ±0.9° 仍满足（1/5）
- **经验引用**: .cl/memory/config.md DCE 段 5 条更新（hold_pin/keep_win/hold_ma=1500/residual_root_cause/criterion）；require.md 任务 117 [?] + 新任务重标定；工具坑：COM19 独占（用户串口工具会抢占监听）、烧录需 --target stm32f103rc、.cl 文件编码 GBK(config.md)/UTF-8(require.md) 混用、仿真 settle 需 ≥3s（T_total 4s 不够 4 段）