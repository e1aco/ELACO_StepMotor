# /ela new — 新功能开发

> 核心入口。分析已有架构 → 盘问 → 三档分流 → 编码。

## 触发
```
/ela new <功能描述>
/ela new <功能描述> --light / --std / --deep
```

## 三档概览

| 档位 | 适用 | 产出 | 工作流 |
|------|------|------|--------|
| light | <2h、单文件、bugfix | quick-plan.md | `workflows/dev-light.md` |
| standard（默认） | 跨模块特性 | brainstorm + milestones | `workflows/dev-standard.md` |
| deep | 系统级、新外设、重构 | 5 阶段文件 | `workflows/discussion.md` |

## 流程

**Step -2：知识库检索**
- 扫描 KB 有无相关经验（涉及关键词匹配）
- 命中 → 提示用户参考

**Step -1：CodeGraph 预检**
- 按 `workflows/codegraph.md` 场景 A 建索引

**Step 0：清晰度检查（grill）**
- 读 `commands/grill.md`，检测模糊信号
- 有信号必须进入 grill，不许跳过

**Step 1：档位推荐**
AI 根据描述自动推荐，用户确认：

| 信号 | 推荐 |
|------|------|
| ≤30 字 / "修复""调整""优化" | light |
| "实现""接入""添加模块" | standard |
| "架构""协议栈""新硬件" / 多并列名词 | deep |
| **嵌入式 + 新外设** | **deep 强制** |

**Step 2：模块联想**
- 两阶段匹配：关键词表 → AI 语义回退
- 匹配到 A/B 级模块 → 提示复用（含递归依赖解析）
- 语义匹配命中后 → 自动扩展关键词表

**Step 3：编码规范预检（嵌入式）**
- 加载 `references/ela-rules-quick.md`
- **Drv 文件决策树**：涉及硬件操作（GPIO/SPI/UART/I2C/TIM/ADC/DAC/中断）？→ 是则拆 drv/usr 两文件，否则单文件。不确定时一律按有硬件拆两文件
- **Keil 预检**：检测 .uvprojx 中有无 ELA_LIB 组和 IncludePath

**Step 4：执行对应工作流**

**Step 5：编码后质量关卡**
- 14 项规范检查（函数分层、命名、注释等）
- **Keil 工程同步**（新增 .c 文件自动加入 .uvprojx）：
  1. 列出本次新建的 .c 文件（`git status --short *.c` 或手动对比）
  2. 执行 keil-config 脚本：
     ```bash
     python <BASE_DIR>/plugins/embedded/tools/keil-config/scripts/keil_config.py --scan <项目根目录>
     ```
  3. 验证脚本输出确认新文件已加入对应 Group
  4. 若脚本不可用，提示用户手动在 Keil 中添加文件
- 引导 `/ela verify`

## 相关
- `commands/grill.md`
- `commands/verify.md`
- `workflows/dev-light.md` / `dev-standard.md` / `discussion.md`
- `references/ela-rules-quick.md`
