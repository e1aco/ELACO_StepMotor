# /ela rec — 恢复项目

## 功能
最小代价加载项目状态。只读 `state.md`（≤50行）+ CodeGraph 架构快照。

## 触发
```
/ela rec
```

## 加载策略

| 层 | 内容 | 时机 |
|----|------|------|
| L0 | state.md（≤50行） | 总是 |
| L0.5 | require-index.md（≤50行） | 总是 |
| L1 | project.json（<20行） | 总是 |
| L1.5 | CodeGraph 架构概览 | 有 `.codegraph/` |
| L2 | project-spec / sessions / problem-log | 按需（用户问到才读） |

## 流程

1. **定位状态目录**：`.ela/` → `.em/` → `.emv2/`
2. **L0 加载**：读 state.md + project.json + require-index.md
3. **CodeGraph**：按 `workflows/codegraph.md` 场景 B 获取架构快照
4. **生成摘要**：5 行恢复摘要
5. **交互**：提示下一步可选动作

## 输出格式
```
📂 项目恢复 — <名称>
状态: <STATE_DIR> (<类型>)
步骤: S<N> — <状态>
架构: <顶层模块/入口点概览>
下一步: <state.md 第1条 action>
```
