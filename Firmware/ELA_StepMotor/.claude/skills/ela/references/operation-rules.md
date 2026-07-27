# 操作规则参考

> 由 SKILL.md 按需引用，**不自动加载到上下文**。
> 遇到对应问题时才读取本文件。

---

## 文件编辑安全规则

编辑 `.c` / `.h` / `.py` 等代码文件时，**必须先检查缩进类型**，避免 Edit 工具匹配失败：

```
检查文件缩进方式：
  cat -A <文件> | head -30
  → ^I 开头的行 = Tab 缩进
  → 空格开头的行 = 空格缩进
  → 混合存在 = 危险！

处理方式：
  纯空格缩进 → 可以用 Edit 工具
  包含 Tab 缩进或中文字符 → 用 sed 或 python 做编辑，禁止用 Edit 工具
```

---

## 脚本调用路径规则

AI 在 Bash 中调用外部脚本/Python 时，必须遵循以下路径格式：

```bash
# ❌ 错误：斜杠路径 + 无引号（Python 不认识，空格会截断）
python /e/path/file name.py

# ✅ 正确：Windows 盘符路径 + 双引号包裹
python "E:/path/file name.py"

# ✅ 正确：用 cygpath 转换
python "$(cygpath -w /e/path/file name.py)"
```

**特别注意**：
- Bash 里的 `/e/xxx` 路径传给 Windows 原生程序（Python、OpenOCD 等）时，必须转为 `E:/xxx` 格式
- 路径中有空格时必须加双引号
- 不确定时先 `echo "$PATH"` 检查

---

## 脚本路径解析规则

所有脚本路径使用 `<BASE_DIR>/` 作为 skill 根目录标记，AI 必须将其替换为实际路径：

```bash
# 方法1（推荐）— 从 git 仓库根目录解析
cd $(git rev-parse --show-toplevel)
python .claude/skills/em/<BASE_DIR> 之后的路径

# 示例：<BASE_DIR>/tools/xxx.py →
python $(git rev-parse --show-toplevel)/.claude/skills/em/tools/xxx.py
```

`<BASE_DIR>` 在项目中对应 `.claude/skills/em`（指向 ELACO_RULES/em-skill 的快捷方式）。

---

## require-index.md 维护规则

`require-index.md` 是知识库入口，控制在 ≤50 行，`/em rec` 自动加载。

**触发更新**（任一即执行 `/em index`）：

| 触发点 | 自动/手动 |
|--------|-----------|
| `/em result` 通过 + 留痕后 | ✅ 自动（result.md step 11） |
| `/em rq` 修改核心文档后 | ✅ 自动（rq.md step 4） |
| 手动新增/删除 Require/ 文件 | ⚡ 手动 `/em index` |
| PDF 内容变更 | ⚡ 手动 `/em index` |

**索引内容结构**（严格按模板 `templates/require-index.md`）：

```markdown
## 文档清单
| 文件 | 类型 | 摘要 |          ← 所有 Require/ 文件

## 内容定位
### <核心文档.md>                         ← .lnk 目标
| 行范围 | 内容 |                         ← ## 章节标题 + 行号

### <数据手册.pdf>
| 页范围 | 内容 |                         ← PDF 目录/章节
```

**行范围映射规则**：
- `.md` 文件：按 `##`/`###` 标题分节，记录 `L12-L35` 格式的行号范围
- 留痕追加内容后，**后续章节的行号偏移必须重新计算**
- `.lnk` 核心文档优先，排在文档清单最前
- 超过 50 行时，只保留最新 5 个文档的定位 + 常用速查（删除旧文档定位）

**未找到 `.lnk` 时不阻塞**：无快捷方式文件时跳过留痕，不阻断流程。
