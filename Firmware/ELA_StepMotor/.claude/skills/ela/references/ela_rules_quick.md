# ELA_LIB 编码规范速查（≤70 行）

> 完整版: `references/ela_rules.md`（380+ 行，按需加载）
> 本文件供 `/em new` 和 `/em verify` 时快速参考。

## 编码前检查（强制）

```
编码前必须完成：
1. 加载本文件（ela_rules_quick.md）
2. 检查命名规范（文件/函数/变量/类型/宏）
3. 查阅相关数据手册（硬件相关时）
4. 确认 drv 决策（是否需要拆分）
```

## 文件位置（铁律）

| 文件 | 位置 | 说明 |
|------|------|------|
| `elaco_main.c/h` | `ELA_LIB/` | 应用入口 |
| `ela_*.c/h` | `ELA_LIB/` | 所有模块文件 |
| CubeMX 文件 | `Core/Src/` | **禁止修改** |

## 命名

| 类型 | 规则 | 示例 |
|------|------|------|
| 文件 | `ela_` 前缀 | ela_button.c |
| 函数（对外） | `ela_模块_动作_细节` snake_case | ela_uart_send_string |
| 函数（static） | `模块_动作_细节` 无前缀 | uart_transmit_byte |
| 全局变量 | `g_` 前缀 | g_rx1_offset |
| 结构体全局 | `g_` + `_st` 后缀 | g_mt6816_st |
| 状态机全局 | `g_` + `_fsm` 后缀 | g_motor_mode_fsm |
| 静态变量 | `s_` 前缀 | s_last_state |
| 结构体类型 | 全大写 + 名词 | MOTOR_DATA |
| 枚举类型 | 全大写 + MODE/STATE/STATUS | MOTOR_STATE |
| 宏 | 全大写 + _MASK/_SIZE | DMA_BUF_SIZE |
| 位操作 | `BIT(n)` 宏，1UL 防溢出 | BIT(0) |

## 注释格式

```c
/****
 * @ 输入: param: 说明
 * @ 输出: 返回值说明
 * @ 说明: 功能描述
 * @ 注意: 可重入性/中断安全性（可选）
 ********/
```

## drv 文件决策流程（编码前必做）

```
模块是否涉及硬件操作（HAL/寄存器/引脚/中断/DMA）？
├── 否 → 单文件 ela_xxx.c/h（纯算法/数据结构/协议转换）
└── 是 → 拆分为：
        ├── ela_xxx_drv.c/h  ← 引脚宏 + HAL 原语
        └── ela_xxx_usr.c/h  ← 业务逻辑（include _drv.h）

⚠️ 不确定是否有硬件依赖？→ 一律先按有硬件的拆两文件
   确认确实不需要后，再合并也不迟。
```

参考实际拆分的项目文件（`ela_tmc5160_drv` + `ela_tmc5160_usr`）。

## 函数分层 → 文件映射

| 层 | 文件 | 说明 |
|----|------|------|
| drv | `ela_xxx_drv.c/h` | 独立文件，只碰 HAL，含引脚宏 |
| usr | `ela_xxx.c/h` 或 `ela_xxx_usr.c/h` | 独立文件，业务逻辑 |
| hlp | 放在 usr 文件中 | 纯计算 |
| cac | `elaco_main.c` | 仅回调分发 |

**drv 和 usr 必须分文件，不允许混写。**

## 格式

- 缩进 4 空格，不 Tab
- Egyptian 大括号
- 行宽 ≤ 84 字符
- CRLF 换行
- 文件末尾空一行
- Yoda 条件: `if (NULL != ptr)`

## 头文件包含顺序（.c）

1. 本模块头文件 ela_xxx.h
2. HAL 头文件 main.h/spi.h 等
3. 标准库 stdbool.h/string.h 等

## 关键规则

- **volatile**: ISR 共享变量必须声明
- **栈**: 局部数组 >64 字节用 static
- **Magic Number**: 必须宏定义（NULL/1/-1/sizeof 除外）
- **ISR**: 禁止阻塞 API/malloc/printf
- **错误处理**: 致命→Error_Handler()，非致命→返回错误码
- **配置**: 三态模式 CONFIG_RESTORE/OK/COMMIT
- **段标记**: `/* 模块名_层 start/end */`（如 `/* tmc5160 drv start */`），不写 `module`
- **drv 暴露**: 推荐非 static 直接暴露（Pattern B），不强制包一层
- **注释同步**: 函数行为变更时，`@ 说明`/`@ 注意`/`@ 依赖` 必须同步更新
- **禁止盲目复制**: 从其他版本迁移代码时，必须先按目标规范重命名，禁止原样搬运
- **跨模块防撞**: 新增宏/类型前，扫描目标模块已有定义，禁止同名不同值
- **elaco_main 不碰 HAL**: elaco_main.c 禁止直接 include HAL 头文件操作外设，必须通过 drv 层

## 编译器标志（推荐）

| 工具链 | 标志 | 作用 |
|--------|------|------|
| **GCC** | `-Wall -Wextra -Wpedantic` | 开启大部分警告 |
| GCC | `-Werror` | 警告升错误（CI 环境强制） |
| GCC | `-Wunused -Wconversion -Wsign-conversion` | 检测未使用变量/隐式类型转换 |
| GCC | `-fstack-usage -Wstack-usage=256` | 检测栈使用超限 |
| Keil | `--c99 --diag_warning=111,223` | 开启 C99 + 指定警告 |
| Keil | `--diag_error=111,223,188` | 指定警告升错误 |

**建议**：在项目 `CMakeLists.txt` 或 Keil 工程中启用以上标志。
`/em verify` 时会自动检查这些标志是否已启用。

## 模块独立性（复用关键）

- **头文件独立**: ela_xxx.h 不准 include elaco_main.h
- **drv 自包含**: 只有 drv 区段能碰 HAL
- **移植**: 复制模块后只改 drv 区段的 HAL handle 和引脚
- **跨模块引用**: 允许直接依赖，但必须在文件头注释加 `@ 依赖: 模块名`
- **Keil 同步**: 新增 .c 文件后立即执行 `/em keil-config` 同步到工程
- **文档同步**: 修改 CAN 协议相关代码后，必须同步更新 `Require/*.lnk` 指向的协议文档

## elaco_main.c 职责

- **关系**: CubeMX `main.c` 保留 `main()` + 初始化，`elaco_main()` 在 `USER CODE BEGIN 2` 被调用
- **允许**: 主循环 `while(1)` + HAL 回调 + 模块初始化调用
- **禁止**: 业务逻辑、协议解析、测试代码
- **行数**: ≤ 200 行

## 测试规范

| 类型 | 用途 | 文件命名 | 位置 | 运行环境 |
|------|------|----------|------|----------|
| **弱化测试** | 硬件快速验证（电机/通信/传感器）| `ela_xxx_test.c` | `TEXT/` | MCU 真机 |
| **单元测试** | 纯算法逻辑验证 | `ela_xxx_utest.c` | `TEXT/` | PC 编译 |

### 弱化测试要点
- 函数 `ela_xxx_test(void)` 自带 `while(1)` 循环，运行后不返回
- 在 `elaco_main()` 的 `while(1)` 前调用，替代主循环
- 不写断言/日志/printf，结果由人眼观察
- 测试完必须删除测试调用行，不留在 `elaco_main.c`

```c
void elaco_main(void)
{
    ela_xxx_drv_init();
    /* ela_xxx_test(); */    /* ← 需要时取消注释，测完删掉 */
    while (1) { }
}
```

### 文件规则
- TEXT/ 下 .c **加入** `.uvprojx` 工程组（单独建 `TEXT` 组存放）
- 测试完成后手动从 Keil 工程移除

## 代码验证（模块复用前必过）

| 检查项 | 要点 |
|--------|------|
| 文件头注释 | 5 字段全填（文件/作者/日期/版本/说明）|
| 头文件保护 | `ELA_XXX_H`，无前导下划线 |
| 头文件独立 | 不 include elaco_main.h |
| 函数命名 | 对外: `ela_模块_动作`，static: `模块_动作` |
| 变量命名 | 全局 `g_`，静态 `s_`，结构体 `g_`+`_st` |
| 类型命名 | 全大写 + `_T` |
| 宏命名 | 全大写 + 下划线 |
| 注释格式 | `/**** @输入/@输出/@说明 ****/` |
| drv 独立文件 | `ela_xxx_drv.c/h` 只含 HAL 操作 |
| usr 独立文件 | `ela_xxx.c/h` 只含业务逻辑 |
| 无 Magic Number | 数字常量必须宏定义 |
| 文档同步 | 修改协议相关代码后，更新 `Require/*.lnk` 指向的文档 |
| Keil 同步 | 新增 .c 文件后执行 `keil-config` |
