---
name: ela-skill
description: 项目开发管家 — 需求盘问 → 新功能开发(三档分流) → 步骤验证(HVR) → 知识沉淀。原生集成 CodeGraph MCP 代码智能，支持通用/嵌入式/学习三类项目。
version: 1.0.0
---

# ELA-SKILL

> 项目开发管家 | `/ela` 命令全家桶 | 原生 CodeGraph 代码智能

接收参数：`$ARGUMENTS`

---

## 快速开始

```
/ela init <name>       # 初始化新项目（自动建代码索引）
/ela rec               # 恢复已有项目状态
/ela new "加个串口"    # 新功能开发（分析→盘问→三档分流→编码）
```

**首次使用**：`/ela init <name>` 或 `/ela si <path>`
**日常**：`/ela rec` → `/ela new <描述>` → `/ela verify s<N>` → `/ela result`

---

## 命令速查

### 核心开发
| 命令 | 说明 |
|------|------|
| `/ela init <name>` | 初始化项目（自动检测类型 + CodeGraph 建索引） |
| `/ela si [path]` | 存量代码导入（扫描芯片/外设 + CodeGraph 建索引） |
| `/ela rec` | 恢复项目（只读 state.md ≤50行 + CodeGraph 架构快照） |
| `/ela new <desc>` | 新功能开发（codegraph 查架构 → 盘问 → 三档分流） |
| `/ela grill <topic>` | 盘问模式（一问一答澄清需求，写 CONTEXT.md + ADR） |
| `/ela disc <topic>` | 深度讨论（5 阶段：拆分→需求→硬件→头脑风暴→里程碑） |
| `/ela verify <step>` | 步骤验证（影响检查→ HVR → 编译→ 审查→ commit 提议） |
| `/ela result` | 记录验证结果（自动留痕 + 推进 + 知识库沉淀） |

### 项目管理
| 命令 | 说明 |
|------|------|
| `/ela stat [-v]` | 项目状态（默认极简；`-v` 全景含步骤表/会话/决策） |
| `/ela sw <name>` | 切换项目（自动 CodeGraph 增量同步） |
| `/ela arch` | 归档已完成步骤 |
| `/ela sum` | 生成上下文摘要 |
| `/ela help [cmd]` | 帮助 |

### 知识库
| 命令 | 说明 |
|------|------|
| `/ela req [show <file>] [install]` | 知识库管理（列表/显示PDF/安装依赖） |
| `/ela rq <content>` | 修改核心需求文档 |
| `/ela reindex` | 重建知识库索引 |
| `/ela module [list\|add\|audit]` | 模块管理（五级A~E，自动递归依赖） |

### 扩展
| 命令 | 说明 | 启用条件 |
|------|------|---------|
| `/ela build` | 编译（自动检测 Keil/CMake/GCC） | 嵌入式项目 |
| `/ela flash` | OpenOCD 烧录 | 嵌入式项目 |
| `/ela serial` | 串口监控（CLI/MCP） | 嵌入式项目 |
| `/ela initem` | 嵌入式工具初始化（路径注册） | 嵌入式项目 |
| `/ela learn [teach\|kb\|status]` | 交互式教学 + 知识库 | 学习模式 |

---

## 项目类型

| 类型 | 触发 | 能力 |
|------|------|------|
| **通用** | `init` 自动检测无特征 | 标准开发流程 + CodeGraph |
| **嵌入式** | 检测 Keil/CubeMX/ESP-IDF 或 `--type=embedded` | 注入 build/flash/serial + C 编码规范 |
| **学习** | `learn teach/kb/status` | 交互式教学 + 知识库沉淀检索 |

---

## 状态目录 (`.ela/`)

```
<项目>/.ela/
├── state.md            # 核心状态（≤50行）
├── project.json        # { name, type, created }
├── require-index.md    # 知识库索引（≤50行）
├── project-spec.md     # 规格说明
├── decisions.md        # 架构决策
├── problem-log.md      # 问题日志
├── sessions/           # 会话历史
├── discussion/         # 讨论记录
├── checkpoints/        # HVR 文件
└── logs/               # 串口/编译日志
```

**兼容**：AI 按 `.ela/` → `.em/` → `.emv2/` 优先级查找，旧项目零迁移。

---

## 核心原则

### 🚫 禁止自动访问外部网页
文件读取失败时，**必须先问用户**。

### 💡 先推荐方案，等确认再动手（最高优先级）
发现问题 → 诊断 → 列出 2-3 个方案（各带优缺点） → 等用户选 → 才执行。

唯一例外：用户明确说了"改/修/你来搞定/做吧"。

### 📐 代码规范（嵌入式项目）
生成代码前读 `references/ela-rules-quick.md`，严格遵循命名/注释/分层规范。

### 🔧 操作需知（按需读取，不自动加载）
| 场景 | 读此文件 |
|------|---------|
| 缩进检查 / 编辑安全 | `references/operations.md` |
| 脚本路径 / BASE_DIR | `references/operations.md` |
| require-index 维护规则 | `references/operations.md` |
| 完整编码规范 | `references/ela-rules.md` |

---

## 详细文档

commands/  workflows/  references/  templates/  plugins/

查看命令详细: `/ela help <命令>`
