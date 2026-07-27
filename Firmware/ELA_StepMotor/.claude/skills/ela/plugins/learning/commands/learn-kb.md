# 命令: /ela learn kb [子命令]

## 功能

知识库管理：沉淀调试经验、解决方案、技术要点，支持检索复用。

## 触发

```
/ela learn kb                          # 知识库总览
/ela learn kb search <关键词>           # 搜索知识库
/ela learn kb add                      # 手动添加条目
/ela learn kb add --type debug         # 指定类型添加
/ela learn kb list                     # 列出所有条目
/ela learn kb show <slug>              # 查看条目详情
/ela learn kb delete <slug>            # 删除条目
```

**示例**

```
/ela learn kb search "CAN 通信 错误"
/ela learn kb add --type debug
/ela learn kb list --type concept
```

## 参数

| 参数 | 必填 | 说明 |
|------|------|------|
| search <关键词> | 搜索时必填 | 搜索关键词（支持中英文） |
| add | 添加时必填 | 进入交互式添加流程 |
| --type <type> | 否 | 筛选类型：debug / concept / snippet |
| --tag <tag> | 否 | 筛选标签 |
| list | 否 | 列出所有条目 |
| show <slug> | 否 | 查看条目详情 |
| delete <slug> | 否 | 删除条目 |

## 知识库结构

```
<STATE_DIR>/learning/kb/
├── debug/                   # 调试知识库（问题+解决方案）
│   └── <slug>.md
├── concepts/                # 概念知识库（技术要点）
│   └── <slug>.md
└── snippets/                # 代码片段（可复用代码）
    └── <slug>.md
```

## 子命令详解

### 1. 知识库总览（无参数）

输出示例：

```
📚 知识库

调试经验: 12 条
  最近: can-bus-timeout (2026-07-20)
  最近: spi-collision (2026-07-18)

技术概念: 8 条
  最近: dma-transfer (2026-07-15)

代码片段: 5 条
  最近: ring-buffer (2026-07-12)

搜索: /ela learn kb search <关键词>
添加: /ela learn kb add
```

### 2. 搜索（search）

**实现**：简单文本搜索（grep 风格）

1. 遍历 `kb/` 下所有 `.md` 文件
2. 匹配文件名 + 内容中的关键词
3. 返回匹配条目的摘要 + 文件路径

**输出格式**：

```
🔍 搜索: "CAN 通信"

找到 3 条结果:

1. [debug] can-bus-timeout
   问题: CAN 通信超时，节点无响应
   解决: 检查终端电阻 + 波特率配置
   路径: .ela/learning/kb/debug/can-bus-timeout.md

2. [debug] can-error-passive
   问题: CAN 进入 Error Passive 状态
   解决: 检查总线负载 + 重发次数
   路径: .ela/learning/kb/debug/can-error-passive.md

3. [concept] can-protocol-basics
   要点: CAN 2.0A/B 帧格式、仲裁机制
   路径: .ela/learning/kb/concepts/can-protocol-basics.md

输入编号查看详情，或 /ela learn kb show <slug>
```

### 3. 添加（add）

**交互式流程**：

```
📝 添加知识条目

类型: [debug] / concept / snippet
> debug

标题: CAN 通信超时问题
> CAN 通信超时，节点无响应

问题/背景:
> 节点 A 发送 CAN 帧后超时无响应，示波器看不到 ACK

解决方案/核心要点:
> 1. 检查总线终端电阻（120Ω × 2）
> 2. 检查波特率配置是否一致
> 3. 检查节点是否进入 Bus-Off

适用场景:
> CAN 通信调试，节点无响应

标签（逗号分隔）: can, 调试, 通信

✅ 已保存: .ela/learning/kb/debug/can-bus-timeout.md
```

**条目模板**：

```markdown
# <标题>

**类型**: debug | concept | snippet
**创建时间**: YYYY-MM-DD
**标签**: <tag1>, <tag2>

## 问题/背景
<遇到什么问题或想记录什么知识>

## 解决方案/核心要点
<怎么解决的或关键知识点>

## 适用场景
<什么情况下可以用这个>

## 相关链接
- 来源：<URL 或文档>
- 相关条目：<其他 kb 条目 slug>

## 变更记录
- YYYY-MM-DD: 初始创建
```

### 4. 列出（list）

按类型列出所有条目，支持 `--type` 和 `--tag` 筛选。

### 5. 查看详情（show）

显示条目完整内容。

### 6. 删除（delete）

删除前确认：「确定删除 <标题>？(Y/n)」

## 自动沉淀触发

**与 /ela result 集成**：

在 `commands/result.md` 中，每次 `/ela result` 执行后（无论通过或失败），追加询问：

```
💡 是否将此次经验沉淀到知识库？
   类型: debug（调试经验）/ concept（技术要点）/ snippet（代码片段）/ 跳过
```

用户选择后执行 `/ela learn kb add --type <选择>`。

## 检索优先级

在项目开发中遇到问题时，AI 应按以下顺序查找：

```
1. 知识库 debug/ — 优先，最精准（之前踩过的坑）
2. 知识库 concepts/ — 技术要点
3. 知识库 snippets/ — 可复用代码
4. 项目 Require/ 文档
5. 网上搜索（需用户确认）
```

## 相关文件

- `templates/kb-entry.md` — 条目模板
- `templates/debug-kb.md` — 调试条目模板
- `commands/learn-teach.md` — 教学命令
- `commands/learn-status.md` — 学习进度
- `../PLUGIN.md` — 学习模式插件入口
