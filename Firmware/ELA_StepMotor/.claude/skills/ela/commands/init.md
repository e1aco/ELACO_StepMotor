# /ela init — 项目初始化

## 功能
从零开始新项目。自动识别类型、创建 `.ela/` 结构、建 CodeGraph 索引。

## 触发
```
/ela init <name>
/ela init <name> --type=embedded
```

## 流程

1. **检测**：当前目录已有 `.ela/` 或 `.em/` → 提示已初始化
2. **创建 `.ela/`**：state.md / project.json / project-spec.md / decisions.md / problem-log.md + sessions/ discussion/ checkpoints/ history/ logs/
3. **类型判定**：自动检测 Keil/CubeMX/ESP-IDF → embedded，否则 general
4. **按类型分支**：
   - general → 输出完成报告
   - embedded → 加载 chip-learning 工作流 + 创建 ELA_LIB/ + 提示 keil-config
5. **Require 知识库**：扫描 Require/ → 生成 `require-index.md` → 检测 .lnk 快捷方式
6. **CodeGraph**：按 `workflows/codegraph.md` 场景 A 建索引
7. **写 state.md**：S0，下一步 `/ela new <功能描述>`

## 相关
- `commands/si.md` — 存量导入
- `commands/initem.md` — 嵌入式工具初始化
