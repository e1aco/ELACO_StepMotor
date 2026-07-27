# /ela flash — 烧录

## 功能
通过 OpenOCD 烧录固件到目标芯片。自动检测调试器。

## 触发
```
/ela flash
```

## 实现
调用 `tools/flash-openocd/scripts/openocd_flasher.py`。

## 相关
- `commands/build.md` — 先编译后烧录
- `commands/serial.md` — 烧录后抓日志
