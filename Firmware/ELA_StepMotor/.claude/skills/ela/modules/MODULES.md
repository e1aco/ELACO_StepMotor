# EM-SKILL 模块库

> 可复用代码模块，新项目初始化时自动复制到 `ELA_LIB/`

## 模块清单

| 模块 | 等级 | 依赖 | 文件 | 说明 |
|------|------|------|------|------|
| ela_queue | A | 无 | ela_queue.h/c | 通用循环队列，零硬件依赖 |

## 等级说明

| 等级 | 说明 | 移植工作量 |
|------|------|-----------|
| A | 纯计算/数据结构，零 HAL 依赖 | 直接复制 |
| B | 核心纯 C，drv 区段少量 HAL | 改引脚配置 |
| C | HAL 依赖较多 | 改引脚 + SPI/CAN handle |
| D | 协议/地址绑定项目 | 大量适配 |
| E | 项目专属，不复用 | 不移植 |

## 使用方式

### 新项目初始化时
`/em init` 自动检测 modules/ 下的 A 级模块，提示是否复制到新项目的 ELA_LIB/。

### 手动导入
```
/em module add ela_queue
```

## 模块结构要求

每个模块必须包含：
- 文件头注释（ela_rules 格式）
- 头文件独立（不 include elaco_main.h）
- drv 和 usr 分文件：`ela_xxx_drv.c/h` + `ela_xxx.c/h`
- drv 文件只含 HAL 操作，usr 文件只含业务逻辑
