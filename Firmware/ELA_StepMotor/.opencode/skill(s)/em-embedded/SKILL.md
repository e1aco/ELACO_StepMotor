---
name: em
description: 嵌入式项目开发管家 — 编译→烧录→串口一键验证流程。触发词：em, 编译, 烧录, 串口, verify, build, flash, serial
---

# EM 嵌入式开发管家 (v3.1)

你接收到的参数：`$ARGUMENTS`

## 快速开始

```
/em help              # 查看所有命令
/em rec               # 恢复项目
/em new <描述>         # 新功能开发
/em verify s1          # 验证第1步
```

---

## 命令路由

读取 `$ARGUMENTS` 第一个词作为子命令，剩余部分作为参数。

| 子命令 | 动作 | 详细流程 |
|--------|------|----------|
| `help` | 显示帮助 | 见下方"命令列表" |
| `init` | 初始化项目 | 读取 `commands/init.md` |
| `si` | 存量接入 | 读取 `commands/si.md` |
| `rec` | 恢复项目 | 读取 `commands/rec.md` |
| `stat` | 查看状态 | 读取 `commands/stat.md` |
| `sessions` | 会话历史 | 读取 `commands/sessions.md` |
| `new` | 新功能开发 | 读取 `commands/new.md`，三档分流 |
| `disc` | 讨论模式 | 读取 `commands/disc.md` |
| `verify` | 步骤验证 | 读取 `commands/verify.md`，嵌入式注入编译→烧录→串口 |
| `result` | 记录结果 | 读取 `commands/result.md` |
| `arch` | 归档 | 读取 `commands/arch.md` |
| `sum` | 上下文摘要 | 读取 `commands/sum.md` |
| `pi` | 项目索引 | 读取 `commands/pi.md` |
| `gi` | 全局索引 | 读取 `commands/gi.md` |
| `sw` | 跨项目切换 | 读取 `commands/sw.md` |
| `migrate` | 迁移 .emv2 → .em | 读取 `commands/migrate.md` |
| `migrate-state` | 瘦身 state.md | 读取 `commands/migrate-state.md` |

**嵌入式插件命令：**

| 子命令 | 动作 |
|--------|------|
| `initem` | 工具环境初始化（探测 Keil/OpenOCD/串口路径） |
| `build` | 编译（等同 verify 的编译阶段） |
| `flash` | 烧录（等同 verify 的烧录阶段） |
| `serial` | 串口监控（等同 verify 的串口阶段） |

---

## 执行规则

### 1. 命令文件路由

对于通用命令，执行前先读取对应命令文件获取详细指令：

```
命令文件路径: .claude/skills/embedded-project-manager/EM-SKILL/commands/<子命令>.md
```

### 2. 嵌入式 verify 流程

当执行 `verify` 时，额外读取嵌入式插件的验证子流程：

```
嵌入式验证: .claude/skills/embedded-project-manager/EM-SKILL/plugins/embedded/workflows/verify-embedded.md
```

验证流程（嵌入式模式自动注入）：
1. **编译** → `python .claude/skills/build-keil/scripts/keil_builder.py --project MDK-ARM/ELA_StepMotor.uvprojx --detect`
2. **烧录** → `python .claude/skills/flash-openocd/scripts/openocd_flasher.py --artifact <产物> --interface stlink --target target/stm32f1x.cfg`
3. **串口** → `python .claude/skills/serial-monitor/scripts/serial_monitor.py --port <COM> --baud 115200`

### 3. 工具路径

所有工具路径从 `.em_skill.json` 读取。执行前检查该文件是否存在。

### 4. 状态目录

项目状态存储在 `.em/` 目录（优先）或 `.emv2/` 目录（兼容）：
```
.em/
├── state.md           # 最小状态（≤50行）
├── project.json       # 项目配置
├── project-spec.md    # 项目规格单
├── sessions/          # 会话记录
├── discussion/        # 讨论目录
├── checkpoints/       # HVR 检查点
└── logs/              # 日志
```

---

## 命令列表

```
项目管理:
  init <name>         项目初始化
  si <path>           存量接入
  rec                 恢复项目（只读 state.md）
  stat [-v]           状态查看
  sessions            会话历史
  migrate             迁移 .emv2/ → .em/
  migrate-state       一键生成 state.md

开发流程:
  new <描述>           新功能开发（三档：轻/中/重）
  disc [话题]          讨论模式
  verify s<N>         验证步骤（编译→烧录→串口）
  result <结果>        记录验证结果

工具/索引:
  initem              工具环境初始化
  arch                归档（自动 tag + CHANGELOG）
  sum                 上下文摘要
  pi / gi             项目/全局索引
  sw <项目>           跨项目切换
  help [命令]          查看帮助

嵌入式快捷命令:
  build               编译
  flash               烧录
  serial              串口监控
```

---

## Git 集成

- `verify` 完成后提议 commit（用户确认后执行）
- `arch` 归档时自动打 tag + 更新 CHANGELOG
- 禁止自动 push（用户手动推送）
