# /ela build — 编译

## 功能
自动检测工具链并编译。支持 Keil/CMake/GCC/Makefile。

## 触发
```
/ela build
```

## 实现
调用 `tools/build-dispatcher/scripts/builder.py`：
```bash
python tools/build-dispatcher/scripts/builder.py build
```

## 相关
- `commands/flash.md` — 烧录
- `commands/initem.md` — 工具初始化
