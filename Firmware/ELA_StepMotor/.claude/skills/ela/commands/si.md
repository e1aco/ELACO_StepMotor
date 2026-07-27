# /ela si — 存量项目导入

## 功能
分析现有代码结构，重建项目状态。自动识别芯片/外设/驱动层。

## 触发
```
/ela si [路径]
```

## 流程

1. **进入项目目录**
2. **分析代码**：芯片型号 → 已初始化外设 → 驱动层 → 应用层 → git 状态
3. **芯片学习**：识别厂商（ST/GD/WCH）→ 写入 `~/.claude/chips.json`
4. **创建 `.ela/`**：project-spec.md + state.md + sessions/
5. **Require 知识库**：同 `init.md` 步骤 5
6. **CodeGraph**：按 `workflows/codegraph.md` 场景 A 建索引
7. **输出审计报告**

## 相关
- `commands/init.md` — 全新项目初始化
