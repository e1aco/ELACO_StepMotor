# ELA-SKILL 插件索引

> 扩展插件在此注册。命令已统一到 `commands/` 目录，插件聚焦工具/脚本/MCP 服务。

## 已注册插件

| 插件 | 路径 | 启用条件 | 提供能力 |
|------|------|----------|----------|
| [embedded](embedded/PLUGIN.md) | `plugins/embedded/` | `project.json.type == "embedded"` 或检测到 Keil/CubeMX/ESP-IDF | 编译/烧录/串口工具脚本 + 串口 MCP server + C 编码规范 |
| [learning](learning/PLUGIN.md) | `plugins/learning/` | `project.json.type == "learning"` 或检测到 `.ela/learning/` | 交互式教学工作流 + 知识库模板 |

> 插件命令现已平铺到 `commands/` 目录。插件本身的工具/脚本/MCP 服务仍放在 `plugins/` 下物理解耦。

## 插件开发规范

1. **路径**：`plugins/<plugin-name>/`
2. **入口文档**：`<plugin>/PLUGIN.md`
3. **工具**：`tools/<tool-name>.py`
4. **模板**：`templates/<template-name>.md`
5. **命令**：注册到 `commands/<cmd-name>.md`，不在插件目录下保留命令副本

## 添加新插件

1. 创建 `plugins/<new-plugin>/` 目录
2. 写 `PLUGIN.md`（含 frontmatter）
3. 命令文件放到 `commands/` 而非插件目录
4. 在本 INDEX.md 表格追加一行
