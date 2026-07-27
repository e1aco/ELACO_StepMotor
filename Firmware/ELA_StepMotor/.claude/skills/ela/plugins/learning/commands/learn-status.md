# 命令: /ela learn status

## 功能

查看学习模式状态：进行中的课程、最近完成、知识库统计。

## 触发

```
/ela learn status                  # 总览
/ela learn status -v               # 全景（含知识库详情）
/ela learn status <topic-slug>     # 单课程详情
```

## 参数

| 参数 | 必填 | 说明 |
|------|------|------|
| topic-slug | 否 | 课程标识（缺省则显示总览） |
| -v | 否 | 全景模式 |

## 执行流程

### 步骤 1: 检查学习模式启用

读 `<STATE_DIR>/learning/` 目录：

- 不存在 → 提示"无学习模式，请 `/ela learn teach <topic>` 启用"
- 存在 → 进入步骤 2

### 步骤 2: 按模式分支

#### 模式 A: 无参数（总览）

读取 `<STATE_DIR>/learning/teach/` 下所有课程目录，扫描 `course-state.md`。

输出示例：

```
📚 学习进度

进行中:
  - CAN总线协议 (第3章/共6章) — 概念掌握: 8/12
  - FreeRTOS调度 (第1章/共4章) — 刚开始

最近完成:
  - ✅ STM32 GPIO (2026-07-15)
  - ✅ UART通信 (2026-07-10)

知识库:
  - 调试经验: 12 条
  - 技术概念: 8 条
  - 代码片段: 5 条

下一步: /ela learn teach CAN总线协议 继续学习
```

**数据来源**：
- 进行中课程 → `teach/*/course-state.md` 中 `[ ]` 标记的当前章节
- 最近完成 → `_index.json` 中 `status: "completed"` 的条目
- 知识库统计 → `kb/debug/`、`kb/concepts/`、`kb/snippets/` 文件计数

#### 模式 B: -v（全景）

输出四段：

1. **进行中课程**（所有未完成课程的详细进度）
2. **最近完成**（最近 5 个完成的课程）
3. **知识库统计**（按类型 + 标签分布）
4. **弱概念复习队列**（需要复习的概念列表）

#### 模式 C: 指定 topic-slug（详情）

输出示例：

```
📖 CAN总线协议  [进行中]

当前章节: 第3章 — 错误处理机制
完成进度: 3/6 章节
概念掌握: 8/12 (67%)
创建日期: 2026-07-18

章节状态:
  ✅ 1. 基础概念 — 已掌握
  ✅ 2. 帧格式 — 已掌握
  📝 3. 错误处理 — 进行中 (50%)
  ⬜ 4. 波特率计算
  ⬜ 5. 终端电阻配置
  ⬜ 6. 实战调试

弱概念:
  - Error Passive 状态转换
  - 重发次数配置

下一步: /ela learn teach CAN总线协议 继续第3章
```

## 状态文件格式

**_index.json** 结构：

```json
{
  "version": 1,
  "courses": [
    {
      "slug": "can-bus-protocol",
      "title": "CAN总线协议",
      "status": "active",
      "current_chapter": 3,
      "total_chapters": 6,
      "mastered_concepts": 8,
      "total_concepts": 12,
      "created_at": "2026-07-18",
      "last_activity": "2026-07-20"
    }
  ],
  "completed": [
    {
      "slug": "stm32-gpio",
      "title": "STM32 GPIO",
      "completed_at": "2026-07-15"
    }
  ]
}
```

## 相关文件

- `commands/learn-teach.md` — 教学命令
- `commands/learn-kb.md` — 知识库命令
- `../PLUGIN.md` — 学习模式插件入口
