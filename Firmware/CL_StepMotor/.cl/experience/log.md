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

## 遗留
- `USR_EncoderCalibrator_Init()` 判据过弱（仅查首值），残留脏数据会误判已校准 → 已沉淀知识节点，待后续加固为全表扫描校验
