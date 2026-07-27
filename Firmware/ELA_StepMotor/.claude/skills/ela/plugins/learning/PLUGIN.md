---
plugin: learning
description: 学习模式插件 — 交互式教学 + 知识库（调试经验/技术要点/代码片段沉淀与检索）
prefix: learn
provides:
  commands:
    - learn-teach      # /ela learn teach <topic> — 交互式教学
    - learn-kb         # /ela learn kb — 知识库管理
    - learn-status     # /ela learn status — 学习进度
  workflows:
    - teach-flow       # 教学工作流（基于 learn-everything）
    - kb-flow          # 知识库管理工作流
  tools:
    - build-html       # Markdown → HTML（离线阅读）
  templates:
    - kb-entry         # 知识库条目模板
    - debug-kb         # 调试条目模板
    - topic-deep-dive  # 深度梳理模板
    - topic-cheatsheet # 速查表模板
enabled_when:
  - project.json.type == "learning"
  - 或检测到 .ela/learning/ 目录存在
---

# EM-SKILL · 学习模式插件

> 交互式教学 + 知识库沉淀，让学到的东西可检索、可复用。
> **通用核（EM-SKILL）与本插件物理解耦**，通用项目不加载本插件。

## 核心能力

### 1. 交互式教学（/ela learn teach）

基于 learn-everything 的核心教学能力：

- **主题模式**：给一个主题，AI 调研并生成课程
- **文档模式**：给一份 PDF/文档，AI 读取并生成课程
- **续学模式**：说「继续」，恢复上次的课程

教学循环：讲解 → 示例 → 练习 → 反馈 → 复习

### 2. 知识库（/ela learn kb）

沉淀调试经验、技术要点、代码片段：

```
<STATE_DIR>/learning/kb/
├── debug/       # 调试经验（问题+解决方案）
├── concepts/    # 技术要点（概念+原理）
└── snippets/    # 代码片段（可复用代码）
```

**自动沉淀**：每次 `/ela result` 后询问是否沉淀到知识库。

**检索复用**：遇到问题时优先搜索知识库，之前踩过的坑不再踩。

## 插件加载机制

1. **项目级启用**：`<STATE_DIR>/project.json` 中 `"type": "learning"`
2. **自动检测**：`/ela learn teach` 或 `/ela learn kb` 时检测 `.ela/learning/` 目录 → 询问是否启用
3. **手动启用**：相关命令会写入 `project.json.type = "learning"`

## 插件入口路由

| 命令 | 文件 |
|------|------|
| `/ela learn teach` | `plugins/learning/commands/learn-teach.md` |
| `/ela learn kb` | `plugins/learning/commands/learn-kb.md` |
| `/ela learn status` | `plugins/learning/commands/learn-status.md` |

## 状态目录结构

```
<STATE_DIR>/learning/
├── teach/                  # 课程目录
│   └── <slug>/
│       ├── course-state.md # 课程状态（概念表、进度）
│       ├── course-map.md   # 课程地图
│       └── notes.md        # 学习笔记
├── kb/                     # 知识库
│   ├── debug/              # 调试经验
│   ├── concepts/           # 技术要点
│   └── snippets/           # 代码片段
└── _index.json             # 主题索引
```

## 工具

| 工具 | 路径 | 用途 |
|------|------|------|
| `build-html` | `plugins/learning/tools/build-html.py` | 主题 Markdown → 独立 HTML（离线可读）|

## 何时**不**用此插件

- 纯软件项目（不需要学习和知识沉淀）
- 单次性任务（用完即弃）
- 已有自己的笔记系统

## 卸载

物理卸载：删除 `plugins/learning/` 整目录即可。
项目级停用：把 `project.json.type` 改回 `"general"`。

## 相关文件

- `commands/learn-teach.md` — 交互式教学
- `commands/learn-kb.md` — 知识库管理
- `commands/learn-status.md` — 学习进度
- `workflows/teach-flow.md` — 教学工作流
- `workflows/kb-flow.md` — 知识库工作流
- 通用核入口：`../../SKILL.md`
