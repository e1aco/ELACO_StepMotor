# 人工验证请求 [HVR-cali-table-001] — 嵌入式扩展模板

> 嵌入式项目专用（type=embedded）。
> 本次验证：S4 校准表生成 — 检查 + 线性插值 + 顺序写 Flash（16384 项）。

**验证类型**: 功能验证（校准表生成算法 + Flash 写入）
**所属步骤**: S4 — 检查 + 插值 + 写 Flash
**前置条件**: 当前 ModTest 构建（elaco_main.h `#define ModTest`）中 TIM4 ISR 由 test_position_cl.c 持有，
             `elaco_calibration_proc()` 尚未接入（S5 范围）→ 硬件上不能跑完整校准流程；
             S4 的 Flash 写入路径改由「合成数据 harness」直接驱动 `calibration_generate_table()` 验证。
**芯片**: STM32F103RET6
**调试器**: ST-Link

---

## 一、嵌入式执行记录（编译结果引用 new.md Step 5，烧录/串口由 AI 执行）

| 步骤 | 工具 | 结果 | 关键产物/字段 |
|------|------|------|---------------|
| 编译 | Keil UV4 -b | ✅ 成功 | 错误:0 警告:0 / Code=23920 / `ELA_StepMotor.axf`（build_s4.log） |
| 算法仿真 | Python 3.11.9 | ✅ 通过 | 5 组数据：len=16384、编码器严格 0..16383 升序、锚点误差=0、微步单调递减 |
| 烧录 | OpenOCD + stlink | ⏳ 待执行 | 需 S5 ISR 接线后跑完整流程；或先跑合成数据 harness |
| 串口 | COM5 115200 | ⏳ 待执行 | 见操作清单 |

---

## 二、操作清单（人工执行）

### 步骤 1：算法验证（AI 已执行，供复核）
- [x] Python 仿真 `s4_verify2.py`（5 组数据：reset∈{0,1234,51100}、E0∈{5000,12345}）：
  - `result_num == 16384`
  - 编码器严格升序 `0..16383`（表索引 == 编码器值，契合 seq_write 顺序写）
  - 锚点精确：`table[avg[k]] == reset + k*256`（误差 0，查表无系统偏差）
  - 微步随编码器递增单调递减（含环绕处平滑）

### 步骤 2：编译验证（AI 已执行）
- [x] `UV4.exe -j0 -b` → `0 Error(s), 0 Warning(s)`，Program Size: Code=23920

### 步骤 3：硬件写 Flash 验证（需 S5 或临时 harness）
> 阻断项：当前 TIM4 ISR 属 test_position_cl.c，未调用 `elaco_calibration_proc()`。
> 两个可选路径，任选其一：
> - **A（推荐）**：进入 S5 接入 ISR 后，双键长按触发完整校准 → 串口观察 COLLECT→CHECK→GENERATE→DONE → 复位读回
> - **B（提前验证）**：临时在 ModTest 中构造合成 `avg_fr_data[]` 调用 `calibration_generate_table()`，
>   复位后串口打印 `g_cali_table[0]`、`g_cali_table[8191]`、`g_cali_table[16383]` 验证落盘

准备:
- [ ] 硬件：电机空载直连无减速，MT6816 直读电机后轴磁环
- [ ] ST-Link SWD 连接 STM32F103RET6
- [ ] COM5 串口线（USART3, 115200）

运行（路径 B harness）:
- [ ] 烧录合成数据 harness 固件
- [ ] 复位后观察串口：打印 `reset_microstep` + 表首/中/尾三值 + `data_err`

### 步骤 4：上电读回验证
- [ ] 再次复位（不触发校准）
- [ ] 观察 LED1：有表时不常亮（`calitable_flag == true` 跳过提示）
- [ ] 串口读回 Flash：`g_cali_table[0]` 应为上次写入的 `reset_microstep` 值

---

## 三、预期结果
- 写表 `result_num == 16384`，`g_calibra_st.calitable_flag = true`
- 表首项 `g_cali_table[0] == reset_microstep`（复位对齐位置）
- 表项微步值 ∈ [0, 51199)，单调性：编码器升序 → 微步降序（环绕 16383→0 处连续）
- 上电读回与写入一致（`0xFFFF` 已消除，读回首项 == 写入值）
- 无 `data_err`（合法值 0；`2`=不连续/`3`=方向/跳点异常/`4`=长度错误）

---

## 四、实际结果（人工填写）

### 串口输出
```

```

### 波形观察
- [ ] N/A

### 物理现象
- [ ] 校准流程中电机平滑转动无失步
- [ ] 校准完成后电机停转无抖动

---

## 五、结论

- [ ] ✅ 通过
- [ ] ❌ 失败
- [ ] ⚠️ 部分通过（备注）：________________

---

## 六、AI 工具执行记录

| 时间 | 工具 | 命令 | 结果 |
|------|------|------|------|
| 2026-08-02 | UV4 | `UV4.exe -j0 -b ELA_StepMotor.uvprojx -o build_s4.log` | ✅ 0 Error 0 Warning，Code=23920 |
| 2026-08-02 | Python | `python s4_verify2.py` | ✅ 5 组全过（len/升序/锚点/单调） |
| 2026-08-02 | Python | `python s4_debug.py`（计数调试） | ✅ 定位到旧版降序写 bug，已修复 |

---

## 七、共同决策（AI + 用户）

- S4 验收标准（milestones.md）：16384 项写入校验通过 + 上电读回。算法层已仿真全过；
  Flash 落盘层受 S5 ISR 接线阻断，已给出路径 A/B。
- 表内容语义：微步值（含 reset_microstep 基准），运行时用 `pos_set & SINE_MASK` 取电角度，表索引 = 编码器值。
- 硬件方向约定：编码器随微步递增而递减（复位代码注释确认），表在环绕 0→16383 处分段拼接。

---

**提交结果命令**：
- 通过：`/ela result cali-table-001-通过`
- 失败：`/ela result cali-table-001-失败-<现象描述>`
