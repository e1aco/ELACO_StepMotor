# /ela verify — 步骤验证

## 功能
验证步骤完成质量。含影响检查 + HVR + 编译 + 审查 + commit 提议。

## 触发
```
/ela verify s<编号>     # 如 /ela verify s7
```

## 流程

**Step 1：状态预检**
- 读 state.md → 当前步骤标记为 🔄 验证中
- 嵌入式项目 → 加载 `references/ela-rules-quick.md`

**Step 2：影响检查**
- 有 `.codegraph/` → 调 `codegraph_impact` 查看修改影响范围
- 识别可能被波及的其他模块 → 加入 HVR 验证点

**Step 3：生成 HVR**
按 `workflows/hvr.md` 模板生成人工验证请求

**Step 4：编译器标志检查**
- GCC：`-Wall -Wextra -Werror -Wunused -Wconversion`
- Keil：`--diag_warning --diag_error`

**Step 5：并行执行通道**

```
Agent-A: 编译（调用 build-dispatcher，自动识别工具链）
Agent-B: 静态分析（14 项规范检查 + MISRA-C 子集）
Agent-C: 代码审查（逻辑正确性 + 边界条件 + 资源竞争）
```
嵌入式项目 → 加载 `workflows/verify-embedded.md` 注入烧录/串口子流程

**Step 6：commit 提议**
- git diff 生成提交信息
- 规则：无 `git push`、无 `--amend`、有 `Co-Authored-By`
- 等用户确认后执行

## 相关
- `commands/result.md` — 记录验证结果
- `workflows/hvr.md` — HVR 模板
- `workflows/verify-embedded.md` — 嵌入式子流程
