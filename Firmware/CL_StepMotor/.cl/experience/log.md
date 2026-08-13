# 调试日志

## [2026-08-12] 任务: 编码器校准闭环跑通（采样→校验→生成校准表写 Flash）
- **模式**: /cl run（物理闭环）
- **现象**: 上电按键不触发校准；多次校准 rcd_x 漂移
- **尝试**:
  - 第1轮: pyocd 烧录失败（Get IDCODE error）→ 用户手动烧录
  - 第2轮: 上电按键无效 → 诊断 Flash 校准区残留 0xC001 → `USR_EncoderCalibrator_Init()` 仅查首值误判"已校准" → `Trigger()` 被 `!s_cali_is_calibrated` 拦截
  - 第3轮: CubeProgrammer 擦除 sector 239~254（校准区 32K）→ 重烧 → 上电按键触发成功
  - 第4轮: 校准完整跑通 `CheckData PASS rcd_x=83 rcd_y=77` → `GenerateTable done result_num=16384` → 写 Flash 复位
- **手动调试记录**:
  - 用户手动转动轴 → 再次校准 rcd_x 变 87→83（绝对基准漂移），rcd_y=77 稳定证明测量可靠
- **最终方案**: Flash 校准区擦净 + 校准流程完整跑通，表写入验证通过
- **验证结果**: result_num=16384（全覆盖）；Flash 抽样 raw0=0x53F3 raw4096=0x8558 raw8192=0xB7A4 raw12288=0x21DA raw16375=0x53D6（单调递增/回绕闭合/锚点自洽）✅
- **经验引用**: 更新 `knowledge/mt6816.md`（故障视角 2 行 + 验证锚点 4 行 + 锚点词）+ `knowledge/index.md` 登记（`encoder:calib_verify` / `calib:trigger_no_effect` / `encoder:rcd_x_drift`）
- **验收**: 2026-08-12 人工验收通过 → require.md 标 [✓]（校准表写 Flash 验证通过，result_num=16384 锚点自洽）

## [2026-08-12] 任务: 全量验收（运行骨架 / MT6816 / TB67H450 / 定时装配 / 编码器校准 / 配置删除）
- **模式**: /cl end（批量验收）
- **现象**: 2026-08-12 用户指令"验收到所有内容"
- **尝试**: 依次将 `[c]`/`[x]` 项改为 `[✓]`：运行骨架（打印链路 OK）、MT6816 分层、TB67H450+sin_form、定时装配 TIM1/TIM4、编码器校准；配置实例化/电机最小闭环为删除项，确认删除即验收
- **手动调试记录**: 无代码修改（用户确认）
- **最终方案**: 全部已完成项验收通过，回填验收时间 2026-08-12
- **验证结果**: require.md 2026-08-12 组全部 [✓] ✅
- **经验引用**: 沉淀 `knowledge/mt6816.md` 故障视角新增 `calib:init_weak`（Init 判据过弱仅查首值 → 全表扫描校验）

## 遗留
- `USR_EncoderCalibrator_Init()` 判据过弱（仅查首值），残留脏数据会误判已校准 → 已沉淀知识节点，待后续加固为全表扫描校验

## [2026-08-13] 任务: /cl check 全量检测 + 风格统一（去 BOM）
- **模式**: /cl check（风格漂移统一动作）
- **现象**: 全量检测发现 11 个 module 文件带 UTF-8 BOM（格式门禁要求无 BOM）
- **尝试**: 用户确认后去 BOM：led_drv.c/h、mt6816_drv.h、tb67h450_drv.c/h、uart_drv.h、mt6816_usr.h、sin_form_usr.c/h、tb67h450_usr.c/h（纯转码，无正文改动）
- **验证结果**: 剩余 BOM=0；Keil 全量 --rebuild 0E/0W，Flash 19.2KB/RAM 3.5KB ✅
- **经验引用**: 库侧/项目侧其余项（节点索引、guard、函数头、编码）全部通过；memory 4 项"待实测确认"（vref/can/pid/enable）待对应外设接线实测

## [2026-08-13] 任务: static 函数命名归一（s_→S_）
- **模式**: /cl check 追加（用户指出命名漂移后确认执行）
- **现象**: `led_drv.c` 的 `s_SetLed` 违反 code_style §6（static 函数应 `S_`+PascalCase，`s_` 属变量前缀）
- **尝试**: 全量扫描 module 所有 static 函数名，共 3 文件 5 处：`s_SetLed`→`S_SetLed`(led_drv.c)、`s_SpiXfer16`→`S_SpiXfer16`、`s_CalcParity`→`S_CalcParity`(mt6816_drv.c)、`CheckData`→`S_CheckData`、`GenerateTable`→`S_GenerateTable`(encoder_calibrator_usr.c)；`\b` 边界替换防误伤
- **验证结果**: 复扫 0 残留；Keil 重编译 0E/0W ✅
- **经验引用**: 补 knowledge 检查项认知——static 函数命名漂移（`s_`/裸名开头）属自动修复类，后续 /cl check 应扫描 `static` 函数名清单

## [2026-08-13] 修复: PowerShell Get-Content 误读损坏 UTF-8 中文（重命名事故）
- **模式**: 事故复盘 / 修复
- **现象**: 用户报告"Encode in UTF-8 without signature 查看部分文件中文乱码"
- **根因**: static 重命名时用 PowerShell `Get-Content`（默认 ANSI/GBK 读）+ `[IO.File]::WriteAllText`（UTF-8 写）→ UTF-8 中文经 GBK 误读再写回，形成永久乱码（`文件`→`鏂囦欢`）。破坏范围=3 个重命名文件（led_drv.c / mt6816_drv.c / encoder_calibrator_usr.c）；BOM 剥离用字节级 ReadAllBytes/WriteAllBytes 不受影响
- **尝试**: `git checkout` 恢复 3 文件 → Python(UTF-8) 重做 5 处重命名 + 去 BOM
- **验证结果**: 中文恢复、无 BOM、全库 0 非UTF-8；重编译 0E/0W ✅
- **经验引用**: 严禁用 `Get-Content`/`Set-Content`/`WriteAllText` 处理含中文的源文件（PS 默认编码是 ANSI）；一切中文文件读写必须走 Python `encoding='utf-8'` 或 Read/Edit 工具。已沉淀为纪律

## [2026-08-13] 任务: 按键-LED 测试（/cl code → /cl end）
- **模式**: /cl code（Code Only）→ /cl end（验收）
- **现象**: 新增按键测试（BTN1→LED1 / BTN2→LED2，短按 toggle，长按闪烁 3 次），验证按键事件链路；用户实测：单击/长按事件正确，但**按键灵敏度偏低**（松手后响应有延迟，主观感受"消抖时间太长"）
- **尝试**:
  - 第1轮: 单击事件由释放沿改为按下沿触发 → 用户认为影响长按语义，改回释放沿
  - 第2轮: 20kHz 中断空闲态不读 SPI（防抢占主循环拖慢事件消费）→ 实测无明显改善，改回
- **最终方案**: 保持原始设计（释放沿触发 + 空闲态照常读 SPI）；确认灵敏度偏低源于 TIM1 100Hz 扫描周期 10ms 粒度上限，属参考设计固有延迟，用户判定"目前就这样，满足使用需求"
- **手动调试记录**: 无代码保留（两轮尝试均已回退），测试段按约定删除恢复 main.c 原样
- **验证结果**: 单击/长按/IsPressed 事件链路正确；测试段删除后重编译 0E/0W，Flash 19.2KB/RAM 3.5KB ✅；require.md 已改 [✓]
- **经验引用**: 按键扫描频率=100Hz（10ms 粒度）是单击/长按事件响应延迟下限，如需更跟手需提高扫描频率（挂更高频 tick），但会占更多中断时间；用户接受现状
