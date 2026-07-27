# /ela req — 知识库管理

## 功能
查看和管理 Require/ 下的需求文档和参考资料。内置 3 引擎 PDF 读取。

## 触发
```
/ela req                   # 列出文档
/ela req show <文件>       # 读取文档
/ela req install           # 安装 PDF 依赖
```

## PDF 读取能力

工具路径：`tools/pdf_reader.py`，支持 3 引擎：

| 引擎 | 命令 | 场景 |
|------|------|------|
| marker | `python tools/pdf_reader.py convert <file> --pages N` | 通用高精度 |
| Docling | `... --engine docling` | 表格/中文文档更佳 |
| auto | `... --engine auto` | 自动选择最优 |
| PyMuPDF | `python tools/pdf_reader.py extract <file> --pages N` | 快速提取 |

其他子命令：`info`、`index`、`search`、`tables`

## 资料优先级
```
.lnk 核心文档 → 其他 .md → .pdf（pdf_reader.py）→ 问用户
```

## 索引
- `.lnk` → 核心文档（开发进度 + 技术参数）
- `.md` → 标题行号映射
- `.pdf` → pdf_reader.py index 提取目录

## 相关
- `commands/reindex.md`
- `commands/rq.md`
- `commands/result.md` — 自动留痕
- `tools/pdf_reader.py`
