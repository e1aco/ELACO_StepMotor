---
plugin: embedded
version: 1.2.1
description: 嵌入式开发场景插件 — 串口、烧录、编译、芯片学习、嵌入式 verify 流程、C 编码规范
min_skill_version: 3.4.0
author: zjq <2339311136@qq.com>
license: MIT
prefix: ""  # S15-A: user-facing prefix; empty = 顶层命令（/ela initem 等）

# S15-D: 移除 5 类文件探测项（*.uvprojx / *.uvproj / *.ioc / sdkconfig / platformio.ini）
# 加载策略改为 lazy-load + 命令前缀触发；不再做文件系统 stat 探测
# 一次性项目类型识别由 /ela init / /ela si 完成，结果缓存到 <STATE_DIR>/cache/project-profile.json
# 兼容性保留 type==embedded 检测作为 legacy hint（不影响加载）
enabled_when:
  - project.json.type == "embedded"  # legacy: 仅作 default-plugin-hint，不控制加载

# 提供能力清单
provides:
  commands:
    - name: initem
      file: commands/initem.md
      summary: 工具初始化（OpenOCD/Keil/串口路径注册）
    - name: build
      file: commands/build.md
      summary: 编译（自动检测 Keil/CMake/GCC）
    - name: flash
      file: commands/flash.md
      summary: OpenOCD 烧录
    - name: serial
      file: commands/serial.md
      summary: 串口监控（CLI/MCP）

  workflows:
    - name: chip-learning
      file: workflows/chip-learning.md
      inject_into: [init, si, new]
      summary: 芯片识别 + 工具链学习
    - name: keil-config
      file: workflows/keil-config.md
      inject_into: [init, new, verify]
      summary: Keil 工程配置（ELA_LIB 组 + IncludePath），代码变更后自动同步
    - name: verify-embedded
      file: workflows/verify-embedded.md
      inject_into: verify
      summary: 编译→烧录→串口三连子流程

  tools:
    - name: build-dispatcher
      path: tools/build-dispatcher/scripts/builder.py
      kind: python
      summary: 统一编译入口（自动检测 Keil/CMake/Makefile）
    - name: build-cmake
      path: tools/build-dispatcher/scripts/cmake_builder.py
      kind: python
      summary: CMake/GCC 编译脚本
    - name: build-keil
      path: tools/build-keil/scripts/keil_builder.py
      kind: python
    - name: flash-openocd
      path: tools/flash-openocd/scripts/openocd_flasher.py
      kind: python
    - name: serial-monitor
      path: tools/serial-monitor/scripts/serial_monitor.py
      kind: python
    - name: tool-config
      path: tools/shared/tool_config.py
      kind: python
      internal: true   # 内部共享，不直接调用
    - name: keil-config
      path: tools/keil-config/scripts/keil_config.py
      kind: python
      summary: ELA_LIB 新文件同步到 .uvprojx

  templates:
    - name: serial-log-reference
      path: templates/serial-log-reference.md
    - name: serial-config
      path: templates/serial_config.json
    - name: hvr-template-embedded
      path: templates/hvr-template-embedded.md

  mcp_servers:
    - name: serial-mcp
      config: mcp-servers/serial-mcp.json
      transport: stdio
      command: python tools/serial-mcp/mcp_server.py
      capabilities: [serial_read, serial_status, serial_send, serial_log_file]

  hooks:
    - event: PostToolUse
      matcher: Bash
      script: hooks/log-build.sh
      description: 编译/烧录后记录到 logs/

# 依赖与冲突
depends:
  - core
conflicts: []

# 元数据
tags: [embedded, mcu, stm32, gd32, keil, openocd, serial]
keywords: 单片机 嵌入式 串口 烧录 编译 芯片学习
---

# EM-SKILL · 嵌入式插件

> 嵌入式开发场景的完整能力集合，**通用核（EM-SKILL）与本插件物理解耦**。
> 通用项目（type=general）不加载本插件，零负担。

> **规范版本**：v1.0（遵循 [PLUGIN-SPEC](../../docs/PLUGIN-SPEC.md)）
> **SKILL 版本要求**：≥ 3.0.0

## 插件加载机制

1. **项目级启用**：`<STATE_DIR>/project.json` 中 `"type": "embedded"`
2. **自动检测**：`/em init` 或 `/ela si` 时检测 `enabled_when` 条件（Keil/CubeMX/ESP-IDF/PlatformIO 工程）→ 询问用户是否启用插件
3. **手动启用**：`/ela initem` 也会写入 `project.json.type = "embedded"`

详见 [PLUGIN-SPEC §1.3 启用条件](../../docs/PLUGIN-SPEC.md#13-enabled_when-语法)

## 插件入口路由

加载本插件后，SKILL.md 路由表自动追加以下命令：

| 命令 | 文件 |
|------|------|
| `/ela initem` | `commands/initem.md` |
| `/ela build` | `commands/build.md` |
| `/ela flash` | `commands/flash.md` |
| `/ela serial` | `commands/serial.md` |

并对以下通用命令注入嵌入式分支：

| 命令 | 注入点 |
|------|--------|
| `/ela verify` | 编译/烧录/串口子流程 → `workflows/verify-embedded.md` |
| `/em init` | 芯片选择 + 工具链选择 → `workflows/chip-learning.md` |
| `/ela si` | 嵌入式特征扫描 → `workflows/chip-learning.md` |
| `/ela new` | 嵌入式需求维度（硬件/协议/实时性）→ `workflows/chip-learning.md` |

## 嵌入式工具集

| 工具 ID | 路径 | 用途 | 类型 |
|---------|------|------|------|
| `build-dispatcher` | `tools/build-dispatcher/scripts/builder.py` | 统一编译入口（自动检测 Keil/CMake/Makefile）| python |
| `build-keil` | `tools/build-keil/scripts/keil_builder.py` | Keil 编译脚本 | python |
| `build-cmake` | `tools/build-dispatcher/scripts/cmake_builder.py` | CMake/GCC 编译脚本 | python |
| `flash-openocd` | `tools/flash-openocd/scripts/openocd_flasher.py` | OpenOCD 烧录脚本 | python |
| `serial-monitor` | `tools/serial-monitor/scripts/serial_monitor.py` | 串口 CLI（自动化抓日志）| python |
| `serial-mcp` | `tools/serial-mcp/` | 串口 GUI（tkinter + MCP）| mcp_server |
| `tool-config` | `tools/shared/tool_config.py` | 工具路径配置共享库（内部）| python |

调用方式：脚本通过 `~/.claude/settings.json` 中的路径白名单直接调用（`initem` 命令负责注册）。

## C 编码规范（嵌入式项目强制）

> 完整规范见 [`../../references/ela_rules.md`](../../references/ela_rules.md)
> **代码规范是最高优先级**，所有生成的代码必须严格符合规范，不符合则立即修正。

加载本插件后，所有嵌入式项目必须遵循以下核心规则：

### 文件与函数命名
| 类型 | 规则 | 示例 |
|------|------|------|
| 库文件 | `ela_` 前缀 | `ela_button.c`、`ela_uart.c` |
| 主文件 | 保持原名 | `elaco_main.c/h` |
| 头文件保护 | `ELA_XXX_H` | `ELA_BUTTON_H` |
| 对外函数 | `ela_模块_动作_细节` | `ela_uart_send_string` |
| static 函数 | `模块_动作_细节` | `uart_wait_tx_empty` |

### 变量命名
| 类型 | 前缀 | 示例 |
|------|------|------|
| 全局变量 | `g_` | `g_rx1_offset` |
| 静态变量 | `s_` | `s_pressed` |
| 结构体全局 | `g_` + `_st` | `g_mt6816_st` |
| 状态机全局 | `g_` + `_fsm` | `g_motor_mode_fsm` |

### 代码格式
- 缩进：4 空格（不用 Tab）
- 大括号：Egyptian 风格（左大括号不换行）
- 换行符：Windows CRLF
- 行宽：≤ 84 字符
- 文件末尾必须空一行

### 函数分层（hlp / drv / usr / cac）→ 两文件分拆

开始编码前先判断是否需要 drv 文件：

```
涉及硬件操作（HAL/寄存器/引脚）？
├── 否 → 单文件 ela_xxx.c/h（如 ela_queue）
└── 是 → 拆分为 ela_xxx_drv.c/h + ela_xxx_usr.c/h

⚠️ 不确定？→ 一律先按有硬件拆两文件
```

| 层 | 文件 | 内容 |
|----|------|------|
| **drv** | `ela_xxx_drv.c/h` | HAL 薄封装，1对1硬件操作，引脚宏定义 |
| **usr** | `ela_xxx_usr.c/h` | 组合多个 drv，用户级 API |
| **hlp** | 放在 usr 文件中 | 纯计算，无硬件依赖 |
| **cac** | `elaco_main.c` 内 | ISR 和 HAL 回调（只分发，不实现） |

参考实际拆分：`ela_tmc5160_drv.c/h` + `ela_tmc5160_usr.c/h`。

### 中断安全
- ISR 中禁止：`HAL_Delay`、`printf`、`malloc`、`free`
- ISR 应尽量简短，只做置标志
- 访问共享变量需关中断保护

### 错误处理
- 致命错误：调用 `Error_Handler()`
- 非致命错误：记录日志，返回错误码
- 条件判断使用 Yoda 风格：`if (HAL_OK != status)`

## MCP Server 集成

本插件提供 `serial-mcp` MCP server（stdio transport），提供 4 个工具：

| MCP 工具 | 说明 |
|----------|------|
| `serial_read` | 读取串口日志 |
| `serial_status` | 获取连接状态 |
| `serial_send` | 发送命令 |
| `serial_log_file` | 获取日志文件 |

启用方式：`/ela initem` 读取 `mcp-servers/serial-mcp.json` 并注册到 `~/.claude/settings.json`。

详见：[`mcp-servers/serial-mcp.json`](mcp-servers/serial-mcp.json)

## 嵌入式状态目录扩展

启用插件后，`<STATE_DIR>/` 额外使用以下目录：

```
<STATE_DIR>/
├── logs/               # 串口/编译日志（serial_<step>_<time>.log）
└── embedded/           # 可选：芯片配置、引脚映射等（自管理）
```

## 何时**不**用此插件

- 纯软件项目（Web/App/CLI/库）
- 没有真实硬件、不需要烧录的项目
- 单片机模拟器 / QEMU 跑的项目（视情况）

## 卸载

- 物理卸载：删除 `<BASE_DIR>/plugins/embedded/` 整目录
- 项目级停用：把 `project.json.type` 改回 `"general"`
- 冲突禁用：本插件无冲突项

## 相关文件

- `commands/initem.md` `commands/build.md` `commands/flash.md` `commands/serial.md`
- `workflows/chip-learning.md` `workflows/verify-embedded.md`
- `tools/{build-keil,flash-openocd,serial-mcp,serial-monitor,shared}/`
- `templates/{serial-log-reference.md, serial_config.json, hvr-template-embedded.md}`
- `mcp-servers/serial-mcp.json`
- `../../references/ela_rules.md` — C 编码规范（ELA_LIB）
- 通用核入口：[`../../SKILL.md`](../../SKILL.md)
- 插件规范：[`../../docs/PLUGIN-SPEC.md`](../../docs/PLUGIN-SPEC.md)
- 插件索引：[`../INDEX.md`](../INDEX.md)

## 变更日志

| 版本 | 日期 | 变更 |
|------|------|------|
| 1.2.1 | 2026-07-22 | 分层文档改为两文件分拆描述；keil-config inject_into 加入 verify；描述同步 |
| 1.2.0 | 2026-07-22 | 集成 ELACO_RULES C 编码规范（ela_rules.md/ela_rules_quick.md）；升级为正式插件 |
| 1.1.0 | 2026-07-21 | 集成 ELACO_RULES C 编码规范；添加 references/ela_rules.md |
| 1.0.0 | 2026-07-01 | 应用 PLUGIN-SPEC v1.0 规范；补全 frontmatter（version/requires/hooks/mcp_servers）；注册 build/flash/serial 三个新命令；添加 MCP server 配置引用 |
| 0.x | 2026-06-03 | S11 重构：物理解耦到 plugins/embedded/ |
| 0.x | 2026-05-09 | V0.1260509 工具整合（远程）|
