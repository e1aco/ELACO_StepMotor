# 工作流: Keil 工程配置

> 自动将新 .c 文件同步到 Keil .uvprojx 工程中
> **凡是编码阶段新增 .c 文件，都必须执行此流程。**

## 触发时机（强制）

| 触发点 | 说明 |
|--------|------|
| `/ela init` 创建新目录后 | ✅ 已有 |
| `/ela new` 编码阶段新增 .c 文件后 | ⚠️ **编码完成后必须立即执行** |
| `/ela verify` 验证通过且有新文件产生 | 🔲 自动检查并同步 |
| 手动 `/ela keil-config` | ✅ 已有

## 同步范围

脚本自动扫描以下目录，将新增 .c 文件同步到对应 Keil Group：

| 源目录 | Keil Group | IncludePath |
|--------|-----------|-------------|
| `../ELA_LIB/` | `ELA_LIB` | `../ELA_LIB` |
| `../ModTest/` | `ModTest` | `../ModTest` |
| （可扩展） | | |

- **只添加 `.c` 文件**（`.h` 通过 IncludePath 自动找到）
- **不删除**已有文件（只增不删）
- 目录不存在时自动跳过，不报错

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
2. 列出新增 .c 文件（`git status` 或对比目录清单）
3. 手动编辑 .uvprojx，在对应 Group 下追加 `<File>` 节点
4. 确认 IncludePath 包含对应的源目录

## 输出格式

```
🔧 Keil 工程配置

[1/3] 扫描 ELA_LIB/...
  ✅ 发现 4 个 .c 文件

[2/3] 同步 .c 文件到 ELA_LIB 组...
  ✅ elaco_main.c    已存在
  ✅ ela_can_drv.c   已添加

[3/3] 同步 ModTest 组...
  ✅ test_position.c  已添加

━━━━━━━━━━━━━━━━━━━━━━━━━━━━
✅ Keil 工程配置完成
   文件路径: 相对于 .uvprojx
━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```
