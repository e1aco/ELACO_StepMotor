# 工作流: Keil 工程配置

> 自动将 ELA_LIB/ 下的文件同步到 Keil .uvprojx 工程中
> **凡是新增 .c 文件后，都必须执行此流程。**

## 触发时机（强制）

| 触发点 | 说明 |
|--------|------|
| `/ela init` 创建 ELA_LIB 后 | ✅ 已有 |
| `/ela new` 添加新模块后 | ✅ 已有 |
| 编码阶段**新增** .c 文件后 | ⚠️ **必须立即执行 keil-config** |
| `/ela verify` 验证通过且有**新文件**产生 | 🔲 新增：自动检查并同步 |
| 手动 `/ela keil-config` | ✅ 已有

## 核心规则

- **只添加 `.c` 文件**到工程组（`.h` 文件通过 IncludePath 自动找到）
- IncludePath 必须包含 `../ELA_LIB`

## 执行方式

### 一键执行（推荐）

有专用脚本 `tools/keil-config/scripts/keil_config.py`，AI 直接调：

```bash
# 指定工程文件
python <BASE_DIR>/plugins/embedded/tools/keil-config/scripts/keil_config.py --project 工程.uvprojx

# 扫描当前目录自动找工程
python <BASE_DIR>/plugins/embedded/tools/keil-config/scripts/keil_config.py --scan .

# 预览模式（不写文件）
python <BASE_DIR>/plugins/embedded/tools/keil-config/scripts/keil_config.py --project 工程.uvprojx --dry-run
```

### 回退方案（无法调脚本时）

AI 手动描述操作步骤让用户执行：

1. 定位 .uvprojx → 扫描 `**/*.uvprojx`
2. 扫描 ELA_LIB/*.c
3. 手动编辑 .uvprojx，在 `<Group>ELA_LIB</Group>` 下追加 `<File>` 节点
4. 确认 IncludePath 含 `../ELA_LIB`

## 输出格式

```
🔧 Keil 工程配置

[1/3] 扫描 ELA_LIB/...
  ✅ 发现 4 个 .c 文件

[2/3] 同步 .c 文件到 ELA_LIB 组...
  ✅ elaco_main.c    已存在
  ✅ ela_can_drv.c   已添加
  ✅ ela_can_usr.c   已添加
  ✅ ela_queue.c     已添加

[3/3] 同步 IncludePath...
  ✅ ../ELA_LIB 已存在

━━━━━━━━━━━━━━━━━━━━━━━━━━━━
✅ Keil 工程配置完成
   工程组: ELA_LIB (4 个 .c 文件)
   IncludePath: 已包含 ../ELA_LIB
━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## 注意事项

- 不修改除 ELA_LIB 组和 IncludePath 外的任何内容
- 不删除已有文件（只增不删）
- `.h` 文件不加入工程组，通过 IncludePath 自动找到
- 文件路径使用相对于 .uvprojx 的路径（`../ELA_LIB/`）
