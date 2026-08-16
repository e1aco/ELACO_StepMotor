# 调试日志

## [2026-08-16 19:59] 任务: motor 完善（planner 接入 + PID + 超前角 + 完整状态机）
- **模式**: /cl run (全自动闭环，fix3~fix8 每轮编译烧录遥测判读)
- **现象**: ①POSITION 到位后摆荡/冲过目标（死区边缘拉锯）②VELOCITY 0.5 圈/s 目标收敛但初次 Kp=10 极限环 ③CURRENT 堵转模拟空载推得动（5mA 11 圈/s）④POSITION 多圈回程巡航段偶发 ±2A 猛摆（假速度极限环，条件性）
- **尝试**:
  - 第1轮(fix3): CURRENT 50→5mA + 速度环 Kp=3 分支 → VELOCITY 收敛 ✓，POSITION 仍摆
  - 第2轮(fix4): 退役 MIN_VEL 钳位 → POSITION 仍摆，CURRENT 5mA 空载照推 11 圈/s（堵转模拟失败）
  - 第3轮(fix5): planner 未完成段轨迹速度主导+P 修正、完成段恢复 MIN_VEL → 短行程 FINISH ✓ 长行程仍摆
  - 第4轮(fix6): USR_MOTOR_FAKE_VEL_MAX=25000 低速直驱电流 → SW2 14 轮循环全 FINISH ✓，长行程巡航段仍摆（已知限制）
  - 第5轮(fix7): CURRENT 5→1mA（堵转演示）→ 1mA 空载照推 15 圈/s，手捏不死（细轴）
  - 第6轮(fix8): STALL_SIM 测试钩子（SW2 模拟堵转条件，跳过 vel 传感器，1s 计时照走）→ 2 次命中 `T:...,3,4` ✓
- **手动调试记录**: （8/16 验收）
  - 用户反馈：手捏不死（滑动摩擦平衡 ~1 圈/s > 阈值 0.2 圈/s）、"那个轴太细了"、直流源显示 ~100mA（TB67H450 开环占空比映射 + 板子静态功耗，非闭环恒流）
  - 串口坑 1：pyocd reset 把 COM19 弄挂（OSError 22）→ 监听死亡 → 需用户断电重启板子（2 次）
  - 串口坑 2：start_mon.ps1 `--boot "System Start!"` 的引号被 PowerShell 拆掉（unrecognized arguments: Start!）→ 去掉 --boot + 显式 -WorkingDirectory 修复
  - 修改文件: `motor_usr.c`（S_CalcVelocityP Kp 分支、S_CalcPositionCascade 直驱、堵转条件临时钩子）、`motor_usr.h`、`Core/Src/main.c`（测试段）
- **最终方案**: 速度模式 Kp=3 分支（破空载冲过极限环）+ |vel_goal|≤25000 低速直驱电流（位置误差 1:1，用 s_real_position，限 ratedCurrent）+ planner 段轨迹速度主导 + 堵转行为链经 STALL_SIM 实机验证；测试段（SW1/SW2/遥测/STALL_SIM 钩子）全部删除，生产版 0E/0W Flash 22.6KB
- **验证结果**: VELOCITY 0.5 圈/s 收敛 ✓（5+ 次）；SW2 90°→360° 循环 14+ 轮全 FINISH ✓（落点 92~124 步=0.64~0.87°，死区 256 内）；STALL 行为链 2 次命中遥测 3,4 ✓（置位→1s→STATE_STALL→输出睡眠→模式切换恢复）；T1 30µs 门禁通过
- **经验引用**: `.cl/memory/config.md` 新增 5 条（velocity_kp_branch / fake_vel_root_cause / fake_vel_direct_drive / stall_sim_verdict / long_stroke_limit）；require.md 任务 104 [✓]