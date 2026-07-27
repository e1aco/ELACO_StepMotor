# /ela new — 新功能开发

> 核心入口。分析已有架构 → 盘问 → 三档分流 → 编码。

## 触发
```
/ela new <功能描述>
/ela new <功能描述> --light
/ela new <功能描述> --std
/ela new <功能描述> --deep
```

## 三档概览

| 档位 | 适用 | 产出 | 工作流 |
|------|------|------|--------|
| **light** | <2h、单文件、bugfix | quick-plan.md | `workflows/dev-light.md` |
| **standard**（默认） | 跨模块特性 | brainstorm + milestones | `workflows/dev-standard.md` |
| **deep** | 系统级、新外设、重构 | 5 阶段文件 | `workflows/discussion.md` |

## 流程（总入口）

**Step -1：CodeGraph 预检**
- 按 `workflows/codegraph.md` 场景 A 建索引（如有必要）

**Step 0：清晰度检查（grill）**
- 读 `commands/grill.md`
- 模糊信号：歧义词、隐含前提、多方案未选、与代码矛盾、用户不确定词
- 有信号必须进入 grill，不许跳过

**Step 1：档位推荐**
AI 根据描述自动推荐，用户确认：

| 信号 | 推荐 |
|------|------|
| ≤30 字 / "修复""调整""优化" | light |
| "实现""接入""添加模块" | standard |
| "架构""协议栈""状态机""新硬件" | deep |
| **嵌入式 + 新外设** | **deep 强制** |

**Step 2：模块联想**
- 扫描 `modules/MODULES.md`，关键词/语义匹配
- 匹配到 A/B 级模块 → 提示直接复用

**Step 3：编码规范预检（嵌入式）**
- 加载 `references/ela-rules-quick.md`
- drv 文件决策：有硬件操作 → 拆 drv/usr 两文件

**Step 4：执行对应工作流**
- light → `workflows/dev-light.md`
- standard → `workflows/dev-standard.md`
- deep → `workflows/discussion.md`

**Step 5：编码后**
- 14 项规范检查
- Keil 工程同步（嵌入式 + 新 .c 文件时）
- 引导 `/ela verify`

## 相关
- `commands/grill.md` — 盘问模式
- `commands/verify.md` — 步骤验证
- `workflows/dev-light.md` / `dev-standard.md` / `discussion.md`
- `references/ela-rules-quick.md` — 编码规范
