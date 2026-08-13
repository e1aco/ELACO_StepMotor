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

## [2026-08-13] 任务: /cl check 全量检测
- **模式**: /cl check
- **现象**: 全量检测库侧+项目侧
- **发现**:
  - 库侧：节点/索引/drv/ref 全部登记一致，无矛盾、无失效锚点、无值落库；replication.md/table_ptr.md 为方法论节点，"推导路径"字段豁免
  - 项目侧：datasheet 索引与 pages 完全一致（MT6816 30页+TB67H450 22页）；memory 无"待推导"项，仅 7 项"待实测确认"（vref/can/pid/enable/死区3）；motor 额定 2A 与硬件池一致；task 89 [c] 与 motor_usr 文件对应
  - 唯一漂移：新写 motor_usr.h/c 文件头 `@作者: CL`，全库其余为小写 `cl` → 已统一为 `cl`（机器安全类自动修复）
- **验证结果**: 修复后 Keil 编译 0E/0W ✅
- **经验引用**: 无新节点（命名大小写差异属常规格式项）

## [2026-08-13] 任务: motor 基础闭环震动调参（纯P→Kd阻尼→死区归零）+ 验收
- **模式**: /cl code 手动调试接管（任务 89）
- **现象**: 上电短按 SW1 后电机持续震动；SW2 看似无效；LED 常亮偶尔灭
- **尝试**:
  - 第1轮: 怀疑按键位置 → 代码核对 SW1=PB2/SW2=PB1 映射正确；发现 gpio.c 按键 GPIO_NOPULL 无上拉（参考 PULLUP）→ 硬件隐患非主因
  - 第2轮: 接串口遥测判读——位置稳定 0.25 但 vel ±2圈/s 摆、cur 符号翻转 → 定位正确但位置环极限环
  - 第3轮: STOP 模式对照——cur=0 时 vel 恒 0 → 电流是抖源，非编码器噪声
  - 第4轮: 加 Kd=250 速度阻尼 → 好一点但震；Kd=400 → 出现 FINISH 仍自激
  - 第5轮: 关键矛盾——位置恒定+速度大摆=速度估计把编码器微抖放大成假速度，Kd 响应出电流自激
  - 第6轮: err 死区映射 P 项归零 → 能停但到位前震 ~3s
  - 第7轮: **死区输出直接归零（P+Kd 全停）→ 无电流=无抖源 → 立即收敛**
- **手动调试记录**: 用户逐轮烧录回传遥测；评价"效果无敌了"
- **最终方案**: Kd=400 阻尼（接近过程防过冲）+ POS_DEADBAND=128 内输出归零（到位断电）
- **验证结果**: 到位瞬间 FINISH，vel=0/cur=0 稳定；精度实测 pos 稳定 0.248 vs 目标 0.250 → 偏差 0.002 圈≈0.72° < 死区 0.9° ✅
- **经验引用**: 应沉淀知识节点（极限环诊断链：电流→微抖→速度放大→自激；死区归零治本）

## [2026-08-13] 任务: 全量验收（button + motor 基础闭环）
- **模式**: /cl end（批量验收）
- **现象**: 用户指令"验收当前所有任务"
- **尝试**: 任务 88 button 确认手动调试完成；任务 89 motor 震动调参收敛+精度实测达标，均转 [✓]
- **手动调试记录**: 无代码修改（调参已在上一条记录）
- **最终方案**: require.md 任务 88/89 验收通过，回填时间
- **验证结果**: require.md 2026-08-13 组 button/motor 两 [✓]；遗留：motion_planner(90)/motor完善(91)/配置持久化(92)/can_cmd(93)/main装配/校准Init加固
- **经验引用**: 无新节点（调参节点已在验收前一记录沉淀）
