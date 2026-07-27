# /ela initem — 嵌入式工具初始化

## 功能
注册串口/烧录/编译工具路径。一次性操作。

## 触发
```
/ela initem
```

## 流程

1. 检测工具路径：Keil UV4 / OpenOCD / arm-none-eabi-gcc / 串口
2. 注册到 tool_config.py
3. 自动下载 OpenOCD（如缺失）
4. 注册 serial-mcp MCP server
5. 注册全局 CLAUDE.md 触发器

## 相关
- `commands/build.md`
- `commands/flash.md`
- `commands/serial.md`
