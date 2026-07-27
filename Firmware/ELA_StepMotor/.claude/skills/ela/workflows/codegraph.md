# 工作流: CodeGraph 知识图谱

> 被 `init` / `si` / `new` / `rec` / `sw` 引用。
> 不再每个命令文件写检测脚本，统一这三条场景。

---

## 检测

```bash
codegraph --version > /dev/null 2>&1 && echo "installed" || echo "not_found"
test -d .codegraph && echo "indexed" || echo "no_index"
```

## 场景 A：首次建索引

**触发**：`init`、`si`、`new`（自动步骤）

检测已安装 → 自动执行 `codegraph init`
已有 `.codegraph/` 则跳过全量建库，自动增量同步。

## 场景 B：获取架构概览

**触发**：`rec`（自动步骤）

项目已有 `.codegraph/` → 调用一次 `codegraph_explore` 获取：
- 顶层模块 / 目录结构（≤5 项）
- 核心入口点（main、关键驱动）
- 不读源码，仅符号级概览

注入到恢复摘要中。

## 场景 C：切换时同步

**触发**：`sw`（自动步骤）

检测目标项目的 `.codegraph/`：
- **有** → `codegraph init <项目目录>`（增量同步，静默）
- **无** → 检测已安装则询问用户是否建索引
