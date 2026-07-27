# /ela serial — 串口监控

## 功能
串口日志监控。支持 CLI 自动化 + MCP GUI。

## 触发
```
/ela serial              # CLI 模式（自动抓启动日志）
/ela serial --mcp        # MCP GUI 模式
```

## 实现
- CLI：`tools/serial-monitor/scripts/serial_monitor.py`
- MCP：`plugins/embedded/tools/serial-mcp/`（tkinter + MCP server）

## MCP 工具
启用后提供 4 个 MCP 工具：`serial_read` / `serial_status` / `serial_send` / `serial_log_file`

## 相关
- `commands/initem.md` — 注册串口路径
- `plugins/embedded/PLUGIN.md` — 串口 MCP 详情
