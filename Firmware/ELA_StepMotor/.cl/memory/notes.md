# 项目记忆 (memory)

> 本项目级修正与惯用法，/cl run 每轮闭环后更新；跨项目通用的经验放全局 generalize/。

## 硬件事实 (有据可查)
- 芯片: STM32F103RET6 (LQFP64, Cortex-M3, 无 FPU)，72MHz 主频 (8MHz HSE × 9)
- 时钟: HSE=8MHz，SYSCLK 72MHz；APB1=36MHz，APB2=72MHz (TIM1 挂 APB2；TIM2 挂 APB1)
- 编码器: MT6816CT-ACD (SPI1: PA5/PA6/PA7，CS=PA4)，30 页手册已提取
- 驱动: TB67H450FNG (H 桥，PWM_A=TIM2_CH4 PB11，PWM_B=TIM2_CH3 PB10，AP/AM/BP/BM=PA1/PA2/PC2/PC3)，22 页手册已提取
- 电流采样电阻: 0.1R；电机: 42 步进 1.8°/0.43Nm/2A
- 回传: USART3 (PC10/PC11) 115200-8-N-1；CAN: PA11/PA12

## 既有闭环栈（ELA_LIB，本工程自身代码）
- `ela_motor_usr.{c,h}`: `Motor_Tick20kHz()` 20kHz 入口 + PID_t 结构 + DCE 双闭环/速度环
- `ela_mt6816.{c,h}` / `ela_mt6816_drv` / `ela_mt6816_usr`: 编码器 SPI
- `ela_tb67h450.{c,h}` / `_drv` / `_usr`: H 桥 FOC 驱动（PWM_A/B、方向、电流结构体）
- `ela_motion_planner_usr` / `ela_motion_run_usr`: 梯形规划
- `ela_calibration`/`elaco_calibration_usr`、`ela_cyclecal`: 校准（20kHz tick 分频 4kHz）
- `ela_pow_det_drv/usr`: 编码器供电监测 (POW_MT6816 = ADC1_IN0)
- `ela_button_drv/usr`, `ela_stockfile_drv/usr`, `ela_can_queue`, `ela_uart*`: 外围
- 命名体系为 `ELA_*` / `Motor_*`（非全局库的 `USR_MOTOR_*`），任务直接基于既有栈开发

> ⚠️ 全局 code_repository 的 closed_loop/pid/motor_ctrl/timer_drv 标注"移植自 ELACO ELA_LIB"，但判断依据是**项目名**——其实际来源是 **TMC5160_StepMotor** 项目（验证项目字段），接口为 `USR_CLOSEDLOOP_/USR_MOTOR_/USR_PID_/DRV_TIMER_*`。本工程 ELA_LIB 与全局库仅目录同名，**并非源头**，API 亦不相同（本工程为 `Motor_Tick20kHz`/`ELA_*`）。复用需按接口适配，不可直接照搬。

## 惯用法
- 本任务 active 闭环 = `ela_motion_run_usr.c` 的 I+D 控制器（TIM4 20kHz ISR），非 `ela_motor_usr.c` DCE（未接线）
- 控制器实质 = P(err/64 微步/计数) + D(vel>>1)，err_acc 自回退 → 非真积分

## 推导配置值 (2026-08-11, /cl run 第 1 轮)
- 单位: 1 计数=0.02197°; 1 微步=0.00703°; 3.125 微步/计数; 死区±8计数=±0.176°=±25微步
- 时钟分母: TIM4 20kHz (50µs), MICROSTEPLAP=51200/圈, ENC_RESOLUTION=16384/圈
- 速度: max_delta=4 微步/tick → 562°/s; 慢区±1 → 140°/s
- 力矩预算: 0.43Nm@2A 额定; 1.5A→0.32Nm; 0.6A→0.13Nm; 加速需求 mNm 级 → 电流过剩
- DAC 满量程 3300mA(4095), DAC_SCALE_FACTOR=5083, 2000mA≈60% 占空比
- 第 1 轮目标: DRIVE_MA=1500, HOLD_MA=600, HOLD_MAX_DELTA=1, SLOW_ERR=128
- 第 1 轮实测 (2026-08-11): 到位稳定无振荡; 稳态 err 0°=+38 / 90°=+14 / 180°=-9 / 270°=-22 (计数), 全超死区±8
  → 根因: 保持态 cmd=err>>2 clamp±1 微步 = 1.6mNm 转矩 << 静摩擦~30mNm
- 第 2 轮 (2026-08-11): 保持态 cmd=err*2 clamp±8, 180° 进死区, 90° 11, 0° 回绕跳, 270° 无改善
  → 放大增益有效果但不彻底
- 第 3 轮 (2026-08-11): 保持态改"目标磁点 stepT 基准 + err 积分", 磁场停 stepT±8
  → err 仍 -33/+11/-22: 8 微步×600mA=12.7mNm << 实际摩擦, 保持力不足
- 第 4 轮 (2026-08-11): HOLD_MAX_DELTA=24, HOLD_MA=800, 90°=+8 180°=-4 达标, 270°=-17, 0° 回绕跳
- 第 5 轮 (2026-08-11): HOLD_MA=1200, 90°=+2 180°=-2 完美达标, 270°=-10 接近, 0° 回绕极限环(±304)
  0° 根因: 到位后编码器漂过 0 点(enc=3→16301→16080), 保持 clamp±24 无力修正 304 计数漂移
  校准表回绕区 (enc 16320..16383) 斜率连续正常, 非校准问题
- 第 6 轮 (2026-08-11): 保持态加大漂移重入(MOTION_RUN_REENTER_ERR=32, err 超限撤销到位回运行态逼近), HOLD_MA=1500
  4 角度全进死区但 0°/180°/270° 周期性漂移-重入振荡, 90° 零漂移稳定
- 冷分析 (2026-08-11): 漂移=位置依赖扰动源(非控制器 bug), 90°零漂移/270°第二差/方向负;
  与 POW_DET/GOTO 无因果, valid 全程无丢 → 疑似校准相位误差或机械耦合
- 自动校准尝试 (2026-08-11): 校准采集 F/R 数据严重错乱 (P[0] F=108 vs R=14806, 差~0.9圈),
  reset_microstep=51200 无效, 坏表写入 Flash (table[0]=0) → 系统不可用
  用户观察到校准时抖动; 判定开环跑圈采集不稳定, 重试校准

## 修正记录
- (2026-08-11) 供电: ST-Link 4.7V 供电导致 MT6816 编码器读数跳变/错乱（停驻零跳变证明），改独立 3.3V 后编码器正常
- (2026-08-11) 校准 DONE 状态机 bug: generate_proc 的 DONE 分支无 DONE→IDLE 转换，校准完成后 cali_step 卡 DONE，demo 永不启动（tgt=0 stepT=0 全默认值）。已修
- (2026-08-11) 闭环验证: 3.3V 下校准 F/R 正常(差~40)，90°/180° 100% 达标(err=0~5)，270° 70-100%(-8~-6)，0° 回绕边略超
- (2026-08-11) 校准表生成缺陷(未修): 
  - 表方向与实测相反（spin 实测 field↑→enc↑ 正斜率，表为负斜率）
  - 表映射偏移（table[56]=25699 vs spin 实测 field=780；table[3584]=14832 vs 16780）
  - 回绕段（enc 16350-16383）斜率异常(-1.03 vs 理论-3.125)
  - 表整体只覆盖 ~34240 微步(0.67圈)，非 51200
  - 交叉验证: 表值与 spin 实测 (field,enc) 对全部不匹配
  - 重写方案: 用 spin 实测数据重新推导映射，重写 generate_table 插值（当前复杂 e_hi/e_lo/dec 逻辑有误）
- (2026-08-11) runaway 机制(DBG 20kHz 缓冲定位): 0°/270° 保持态磁场微偏(stepT-cmd) → 转子被推离 → err 平滑递增(DBG: enc 16359→16302 err 25→82, cur 恒定) → err>REENTER 重入 → 运行态积分放大。非编码器跳变(停驻零跳变)、非供电
- (2026-08-11) 编码器手动验证: 读数完全正常(静止稳定/转动连续/回绕正常/可重复) → 排除编码器硬件问题
- (2026-08-11) set_foc_current 电周期: &0x3FF 正确(1024 微步=1 电流电周期=4 全步=90°电角/全步)。曾误改 &0x1FF(实测 field=0/512 同 idx 却不同编码器, 错误) → 回滚
- (2026-08-11) 校准表生成: 表基于校准数据自洽(table[avg[i]]=i*256 diff 0-3)，&0x3FF+3.3V 下重新校准后 90/180/270 完美(err 0~-8, 磁场精确回中 stepT)
- (2026-08-11) 校准表回绕连续化: 原 table[16383]=17440 vs table[0]=17688 差 248(应差 3) → 修 generate_table 环绕段低侧起点与高侧终点连续(差 3)
- (2026-08-11) 校准表环绕段数据异常: avg[jump]→avg[jump+1] 跨 162 计数(正常整步 81.92)，环绕段宽度 2 倍 → 表 0° 附近生成错误
- (2026-08-11) 校准表 0° 磁点偏移: table[0] 每次校准随机(26/24031/25628)，与真 0° 磁点偏 ~1196 微步(实测磁场 25041↔enc 62) → 0° 磁场错 → 编码器卡 62
- (2026-08-11) TIM4 NVIC 中断: ISER bit30(TIM4_IRQn) 未使能 → proc 不跑磁场不动(cur 恒定) → demo 启动处加 HAL_NVIC_EnableIRQ(TIM4_IRQn) 修复
- (2026-08-11) serial_monitor.py bug: ① out_fh wb 模式写 str 崩溃 → 改 w+utf-8 ② stdout GBK 无法编码 \ufffd → reconfigure utf-8 errors=replace
- 已知未解决: 校准表 0° 磁点偏移(~1196 微步) → 0° 卡死(enc 62 恒定)；需专项 0° 磁点校准或校准环绕段重写；90/180/270 需 NVIC 修复后复验
