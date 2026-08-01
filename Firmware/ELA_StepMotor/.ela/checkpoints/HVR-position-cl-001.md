# 人工验证请求 [HVR-position-cl-001] — 嵌入式扩展模板

> 嵌入式项目专用（type=embedded）。
> 本次验证：位置闭环 test_position_cl 4 段 90° 定位精度达标。

**验证类型**: 功能验证（位置闭环精度）
**所属步骤**: S4 — 位置闭环控制
**前置条件**: ModTest 组编译含 test_position_cl.c/h（test_tb67h450.c 已排除，避免回调冲突）
**芯片**: STM32F103RET6
**调试器**: ST-Link

---

## 一、嵌入式执行记录（编译结果引用 new.md Step 5，烧录/串口由 AI 执行）

| 步骤 | 工具 | 结果 | 关键产物/字段 |
|------|------|------|---------------|
| 编译 | Keil UV4 -b | ✅ 成功 | 错误:0 警告:17 / `ELA_StepMotor.axf` |
| 烧录 | OpenOCD + stlink | ✅ 成功 | interface=stlink / verified |
| 串口 | COM6 115200 PowerShell Job | ✅ 抓到完整日志 | `logs/position_cl33*.log`（3 次运行） |

### 启动日志节选（serial-monitor 抓取）
```
--- Position Test CL Start ---
Encoder init done
PWM started (2000mA)
Pre-positioned at electrical angle 0
Initial encoder: 14477 (0x388D)
Seg 0: target +4096 (enc 14477) ... settle[0]=4094 acc=0 ... enc=6190 delta=4100 isr_last=4102 drft=2 (exp=4096) step=-11763 565ms OK
...
--- Position Test CL PASS ---
```

---

## 二、操作清单（人工执行）

### 步骤 1：烧录前
- [x] 编译通过（0 Error），固件已生成
- [x] 硬件：电机空载直连，无减速，板子螺丝固定于电机，MT6816 读电机后轴磁环

### 步骤 2：硬件连接
- [x] ST-Link SWD 连接 STM32F103RET6
- [x] COM6 串口线（USART3, 115200）
- [x] 驱动器供电（TB67H450FNG 双 H 桥，PWM TIM2_CH3/CH4）
- [x] MT6816 4 线 SPI（CS=PA4, SCK=PA5, MISO=PA6, MOSI=PA7）

### 步骤 3：上电观察
- [x] 上电后自动进入 test_position_cl，LED 指示当前段
- [x] 电机依次步进 4 段 90°
- [x] 串口打印每段 enc/delta/isr_last/drft/PASS-FAIL

### 步骤 4：波形检查（如有逻辑分析仪）
- [ ] N/A（无逻辑分析仪，以编码器读数为准）

---

## 三、预期结果
- 4 段各前进 4096 计数（90°），delta ∈ [4088, 4104]（±8 计数 ≈ 0.18°）
- 无 TIMEOUT（每段 < 5s）
- drft ≤ 3（ISR 与测量一致，无到位后漂移）
- 电机到位后无持续振荡（settle 采样稳定在带内）

---

## 四、实际结果（人工填写）

### 串口输出
```
--- Position Test CL PASS ---   （cl33 / cl33b / cl33c 三次连续运行均 PASS）
```

三次运行 delta 汇总：
| 运行 | Seg0 | Seg1 | Seg2 | Seg3 |
|------|------|------|------|------|
| cl33 | 4100 | 4093 | 4093 | 4098 |
| cl33b | 4100 | 4100 | 4093 | 4096 |
| cl33c | 4100 | 4093 | 4095 | 4098 |

全部 16 段 delta ∈ [4093, 4100] ⊂ [4088, 4104]，drft ∈ [-1, 3]。

### 波形观察
- [ ] N/A

### 物理现象
- [x] 电机 4 段 90° 步进平滑无失步
- [x] 到位后安静无抖动（速度阻尼生效）
- [x] 全程无超时

---

## 五、结论

- [x] ✅ 通过
- [ ] ❌ 失败
- [ ] ⚠️ 部分通过（备注）：________________

---

## 六、AI 工具执行记录

| 时间 | 工具 | 命令 | 结果 |
|------|------|------|------|
| 2026-08-02 | UV4 | `UV4.exe -j0 -b ELA_StepMotor.uvprojx`（cl33） | ✅ 0 Error 17 Warning |
| 2026-08-02 | OpenOCD | `openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c program ... verify reset exit` | ✅ verified |
| 2026-08-02 | PowerShell | COM6 115200 串口抓包（Start-Job 内开串口） | ✅ 3 次完整日志 |

---

## 七、共同决策（AI + 用户）

- 电机参数确认：42 步进 1.8° 0.43Nm 2Ω 3.6mH 2A 285g，空载直连无减速，MT6816 直读电机后轴磁环。
- POS_TOL ±8 计数为硬需求，不可放宽（0.18°）。
- 达标配置：MAX_DELTA=4 / KP_SHIFT=6 / DEADBAND=4 / INBAND_CONFIRM=3 / HOLD_MA=2000 / SETTLE_DELAY=500ms / 接近目标降速 / 速度阻尼（cmd -= vel>>1）。
- 测量方式：到位后保持闭环带内 → settle 多采样（10×30ms）均值。

---

**提交结果命令**：
- 通过：`/ela result position-cl-001-通过`
- 失败：`/ela result position-cl-001-失败-<现象描述>`
