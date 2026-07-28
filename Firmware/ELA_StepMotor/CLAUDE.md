# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ELACO stepper motor driver firmware for STM32F103RET6 (LQFP64, Cortex-M3). Drives a 4-phase stepper motor via dual TB67H450FNG H-bridge chips with MT6816 14-bit magnetic encoder feedback. Communicates over CAN bus and Modbus RTU/ASCII (RS-485).

## Build System

- **IDE**: Keil MDK-ARM (uVision 5) — open `MDK-ARM/ELA_StepMotor.uvprojx`
- **Code generation**: STM32CubeMX — open `ELA_StepMotor.ioc`, regenerate code when peripherals change. User code goes between `USER CODE BEGIN/END` guards.
- **Compiler**: ARM Compiler 5 or 6 (per Keil project settings)
- **No CLI build or Makefile** — build exclusively from Keil IDE

## Hardware Architecture

### MCU & Clock
- STM32F103RET6, currently running on **HSI (8MHz)** during development — PLL disabled, `FLASH_LATENCY_0`. Target production clock: 72MHz via HSE 12MHz ×6 PLL.
- Startup file: `startup_stm32f103xe.s`

### Pin Assignments (from `Core/Inc/main.h`)
| Signal | Pin | Label |
|--------|-----|-------|
| Coil A+ | PA1 | AP |
| Coil A- | PA2 | AM |
| Coil B+ | PC2 | BP |
| Coil B- | PC3 | BM |
| PWM A (TIM2_CH4) | PB11 | PWM_A |
| PWM B (TIM2_CH3) | PB10 | PWM_B |
| SPI1 CS (MT6816) | PA4 | SPI_CS |
| SPI1 SCK/MISO/MOSI | PA5/PA6/PA7 | — |
| RS-485 TX (USART1) | PA9 | RS485_TX |
| RS-485 RX (USART1) | PA10 | RS485_RX |
| CAN RX/TX | PA11/PA12 | — |
| SW1/SW2 | PB1/PB2 | Buttons |
| LED1/LED2 | PB12/PB13 | — |
| ADC CH0 (voltage) | PA0 | POW_MT6816 |

### Timer Allocation
| Timer | Purpose | Config |
|-------|---------|--------|
| TIM1 | 100Hz periodic interrupt | Internal clock |
| TIM2 | PWM generation CH3/CH4 | Stepper coil current control |
| TIM3 | General purpose | Interrupt enabled, used for app timing |
| TIM4 | 20kHz periodic interrupt | Internal clock |
| TIM7 | Modbus 3.5-char timeout | 50µs tick base (via htim3 alias in porttimer.c) |

### Peripheral Details
- **SPI1**: Mode 3 (CPOL=1, CPHA=1), 16-bit frames, 1 Mbps (prescaler 8), software NSS. Used for MT6816 magnetic encoder. MT6816 protocol: two separate 16-bit SPI transactions per read, each within its own CS assertion. ANGLE register returns high byte, RAW_ANGLE register returns low byte; combined into 16-bit value for parity check.
- **USART1**: RS-485 Modbus RTU/ASCII, half-duplex (no DE/RE pin handling yet in port code)
- **USART3**: DMA RX (circular) + DMA TX (normal) — available for debug/expansion
- **CAN1**: 166kbps, all 4 interrupt sources enabled (TX, RX0, RX1, SCE)
- **ADC1**: DMA1_CH1, circular mode, CH0 (PA0) voltage sampling

## Software Architecture

### Directory Layout
```
Core/           — CubeMX-generated HAL peripheral init (do not hand-edit)
ELA_LIB/        — Application library (ela_* prefix), user-developed modules
FreeModbus/     — FreeModbus protocol stack + STM32 port layer
ModTest/        — Test harness modules (#ifdef ModTest in elaco_main.c)
MDK-ARM/        — Keil uVision 5 project files, startup code, RTE
```

### Initialization Flow (`Core/Src/main.c`)
```
HAL_Init → SystemClock_Config → MX_GPIO_Init → MX_DMA_Init
→ MX_ADC1_Init → MX_CAN_Init → MX_SPI1_Init → MX_TIM1_Init
→ MX_TIM2_Init → MX_TIM4_Init → MX_USART1_UART_Init
→ MX_USART3_UART_Init → MX_TIM3_Init → elaco_main() → while(1)
```

### ELA_LIB Modules (in `ELA_LIB/`)
- **`elaco_main.c/h`** — Application entry point. `elaco_main()` is called before `while(1)` in main.c. Calls `Uart_PrintfInit()` at startup. Centralizes HAL interrupt callbacks in the `cac` section. Header guards: `_ELACO_MAIN_H_`.
- **`ela_uart.c/h`** — Serial communication module. Redirects `printf` to USART3 (PC10/PC11, debug port) via `fputc`/`__io_putchar`. USART1 RS-485 DMA is configured by CubeMX.
- **`ela_can_queue.c/h`** — Fixed-frame queue (48 × 8 bytes) for CAN command buffering. Circular queue with front/rear pointers. Each `Queue_Insert` copies an 8-byte CAN frame.
- **`ela_uart_queue.c/h`** — Byte-level ring buffer (256 bytes) for UART RX/TX streaming data. Supports single-byte `Put`/`Get` and bulk `PutBuf`/`GetBuf` for ISR-friendly frame transfers.
- **`ela_mt6816.c/h`** — MT6816 magnetic encoder driver. `MT6816_Init()` / `MT6816_GetAngle()` populate global struct `g_mt6816_st` (type `MT6816_ANGLE_T`) with 14-bit angle, magnet status, and parity-validated data. Retries up to 3 SPI reads until even parity (MT6816 protocol requirement).

Planned but not yet created (referenced in project notes): `ela_button`, `ela_tb67h450`.

### FreeModbus Integration
- **Port layer** in `FreeModbus/modbus/port*.c`:
  - `portserial.c` — USART1 RS-485, interrupt-driven TX/RX via `USART1_IRHandler()` (note: NOT the CubeMX-generated `USART1_IRQHandler` — manually defined)
  - `porttimer.c` — TIM7 for 3.5-character timeout (uses variable name `htim3` aliased to TIM7 via `htim3.Instance = TIM7`)
  - `portevent.c` — Simple queue-based event posting
  - `port.c` — Critical section stubs (empty — no RTOS)
- **Config**: `mbconfig.h` enables RTU + ASCII, all standard function codes. TCP disabled.
- **Warning**: `porttimer.c` declares `htim3.Instance = TIM7` — this is intentional aliasing, not a bug. The Modbus timer callback lives in `TIM7_IRQHandler()`.
- **Warning**: `portserial.c` provides its own `USART1_IRHandler()` (note spelling: no 'Q'). The CubeMX-generated `USART1_IRQHandler` in `stm32f1xx_it.c` calls `HAL_UART_IRQHandler` and may conflict. Ensure only one USART1 ISR is active.

### Test Harness (`ModTest/`)
- Conditionally compiled via `#define ModTest` in `elaco_main.h`
- `test_mt6816.c` — calls `MT6816_Init()` then loops `MT6816_GetAngle()`

## Naming & Code Style

> **Authoritative source**: `E:\Desktop\XM\ELACO_RULES\rules.md`

### 一、文件命名
- 库文件统一 `ela_` 前缀（如 `ela_button.c`）
- 主文件保持 `elaco_main.c/h`
- 头文件宏：`ELA_XXX_H`（全大写，无前导下划线）
- 每个 `.c` 和 `.h` 文件末尾必须空一行，否则 Keil 报 Warning

### 二、函数命名
- **公开函数**: `snake_case`，格式 `ela_模块_动作_细节`
  - 正确：`ela_uart_send_string`、`ela_mt6816_read_angle`
  - 错误：~~`Uart_SendString`~~（PascalCase）
- **static 内部函数**: 不加 `ela_` 前缀，格式 `模块_动作_细节`
- **HAL 回调**: 保持原命名不修改
- **文件-模块绑定**: `ela_xxx.c` 内只允许提供 `xxx` 或 `ela_xxx` 前缀函数

### 三、变量命名
| 类型 | 前缀 | 示例 |
|------|------|------|
| 全局变量 | `g_` | `g_rx1_offset` |
| 静态变量 | `s_` | `s_pressed` |
| 结构体全局 | `g_` + `_st` | `g_mt6816_st` |
| 状态机全局 | `g_` + `_fsm` | `g_motor_mode_fsm` |
| 局部变量 | 无前缀 | `rx_buffer`、`temp_value` |

### 四、结构体与类型
- 结构体类型：全大写 + 名词结尾（`MT6816_ANGLE`）
- 枚举类型：全大写 + 状态词结尾（`MOTOR_MODE`）
- 枚举值：全大写（`MODE_STOP`）

### 五、宏定义
- 功能常量：全大写 + 下划线（`DMA_BUF_SIZE`）
- 掩码：`_MASK` 后缀（`MT6816_STATUS_MASK`）
- **位操作宏**: `#define BIT(n) (1UL << (n))`，必须用 `1UL`
- **Magic Number**: 所有数字常量必须宏定义，例外：NULL、1、-1

### 六、函数文档注释格式
每个函数（包括 `static`）必须在定义处添加文档注释。`@ 输入` 和 `@ 输出` 对 void 函数可省略：
```c
/********
 * @ 输入: <参数名>: <参数说明>  (optional, omit for void)
 * @ 输出: <返回值说明>          (optional, omit for void)
 * @ 说明: <功能描述>
 * @ 注意: <可重入性、中断安全性等>  (optional)
 ********/
```

### 七、文件头注释格式
```c
/********
 * @ 文件: <文件名.c/h>
 * @ 作者: <作者名>
 * @ 日期: <YYYY-MM-DD>
 * @ 版本: <X.Y.Z>
 * @ 说明: <功能描述>
 ********/
```

### 八、代码格式
- 4空格缩进，无Tab
- Egyptian风格大括号（左大括号不单独换行）
- CRLF换行，行尾不留空格
- 行宽≤84字符（含注释）
- 文件末尾必须空一行

### 九、头文件包含顺序
**.c文件**:
1. `elaco_main.h`（中央包含头）
2. 自身模块头文件（如 `ela_button.h`）
3. ST HAL头文件（`main.h`、`spi.h`、`usart.h` 等）
4. 标准库头文件（`stdbool.h`、`string.h`、`stdint.h` 等）

**.h文件**: 必须自包含，不依赖外部 .h 文件先被包含

### 十、函数分层 (hlp → drv → usr → cac)
每个 `.c` 文件内的函数按以下四层分组，用84字符分隔符隔开：

```
/* <module> hlp end */
//----------------------------------------------------------------------------------
/* <module> drv start */
...
/* <module> drv end */
//----------------------------------------------------------------------------------
/* <module> usr start */
...
/* <module> usr end */
//----------------------------------------------------------------------------------
/* <module> cac start */
...
/* <module> cac end */
```

| 层 | 用途 | 判断标准 |
|---|---|---|
| **hlp** | 纯计算/算法 | 能在PC上编译运行 |
| **drv** | 硬件原语 | 1对1 HAL封装 |
| **usr** | 业务接口 | 组合多步操作 |
| **cac** | 回调/ISR | 只在elaco_main.c定义 |

- 省略没有函数的层
- `<module>` 是小写模块名（如 `can`、`eeprom`、`timer`）

### 十一、volatile 使用
- 中断共享变量必须 `volatile`
- 硬件寄存器映射必须 `volatile`
- 编译器可能优化掉的忙等循环必须 `volatile`

### 十二、内存对齐
- 默认自然对齐，不强制 `#pragma pack`
- 协议帧/Flash结构可用 `#pragma pack(push, 1)` + `#pragma pack(pop)`

### 十三、中断安全
- ISR禁止: `HAL_Delay`、`printf`、`malloc`、`free`
- ISR共享变量必须 `volatile`
- 临界区保护: `__disable_irq()` / `__enable_irq()`
- ISR应尽量简短，只做置标志或发信号

### 十四、错误处理
- 致命错误: `Error_Handler()` 停机
- 非致命错误: 记录日志 + 返回错误码
- **Yoda风格**: `if (HAL_OK != status)`（常量在左侧，防止误写赋值）

### 十五、栈规范
- 禁止递归
- 局部数组>64字节用 `static` 分配
- 函数局部变量总和≤栈预算80%

### 十六、宏 vs 内联函数
- 简单操作用宏，复杂逻辑用 `static inline`
- 宏必须加括号（参数和整体都要加）

### 十七、配置管理三态
```c
typedef enum {
    CONFIG_RESTORE = 0,  /* 恢复出厂 */
    CONFIG_OK,           /* 正常运行 */
    CONFIG_COMMIT        /* 提交保存 */
} CONFIG_STATUS;
```

### 十八、数据交互模式
- **SetConfig整包**: 低频配置（初始化/参数更新）
- **直接读取**: 高频数据（传感器/遥测）

### 十九、测试规范
- hlp层函数建议单元测试
- 测试文件命名 `test_<模块名>.c`，放在 `ModTest/` 目录

## Key Constraints

- **CubeMX user code guards**: Never edit code between `USER CODE BEGIN/END` comments — it gets overwritten on code regeneration.
- **HSI clock**: Current code runs at 8MHz (HSI, PLL off). Production target is 72MHz HSE+PLL. This affects all timer prescaler calculations.
- **FreeModbus port ISR naming**: `portserial.c` defines `USART1_IRHandler()` (no 'Q'). The CubeMX-generated `USART1_IRQHandler` in `stm32f1xx_it.c` may need to be removed or disabled.
- **No RTOS**: Bare metal HAL with interrupt-driven concurrency. Critical sections are stubbed out (no preemption protection yet).

## Reference

- **Coding rules (authoritative)**: `E:\Desktop\XM\ELACO_RULES\rules.md`
- Reference project: `../../Reference/StepMotor/` (XM-003-08, contains schematics, PCB Gerber, firmware)
- Hardware notes: `../../Reference/StepMotor/CLAUDE.md`
- **Project state**: `.ela/state.md` (≤50 lines, essential status), `.ela/project-spec.md`, `.ela/decisions.md`

<!-- gitnexus:start -->
# GitNexus — Code Intelligence

This project is indexed by GitNexus as **ELA_StepMotor** (27088 symbols, 29098 relationships, 165 execution flows). Use the GitNexus MCP tools to understand code, assess impact, and navigate safely.

> Index stale? Run `node .gitnexus/run.cjs analyze` from the project root — it auto-selects an available runner. No `.gitnexus/run.cjs` yet? `npx gitnexus analyze` (npm 11 crash → `npm i -g gitnexus`; #1939).

## Always Do

- **MUST run impact analysis before editing any symbol.** Before modifying a function, class, or method, run `impact({target: "symbolName", direction: "upstream"})` and report the blast radius (direct callers, affected processes, risk level) to the user.
- **MUST run `detect_changes()` before committing** to verify your changes only affect expected symbols and execution flows. For regression review, compare against the default branch: `detect_changes({scope: "compare", base_ref: "main"})`.
- **MUST warn the user** if impact analysis returns HIGH or CRITICAL risk before proceeding with edits.
- When exploring unfamiliar code, use `query({search_query: "concept"})` to find execution flows instead of grepping. It returns process-grouped results ranked by relevance.
- When you need full context on a specific symbol — callers, callees, which execution flows it participates in — use `context({name: "symbolName"})`.
- For security review, `explain({target: "fileOrSymbol"})` lists taint findings (source→sink flows; needs `analyze --pdg`).

## Never Do

- NEVER edit a function, class, or method without first running `impact` on it.
- NEVER ignore HIGH or CRITICAL risk warnings from impact analysis.
- NEVER rename symbols with find-and-replace — use `rename` which understands the call graph.
- NEVER commit changes without running `detect_changes()` to check affected scope.

## Resources

| Resource | Use for |
|----------|---------|
| `gitnexus://repo/ELA_StepMotor/context` | Codebase overview, check index freshness |
| `gitnexus://repo/ELA_StepMotor/clusters` | All functional areas |
| `gitnexus://repo/ELA_StepMotor/processes` | All execution flows |
| `gitnexus://repo/ELA_StepMotor/process/{name}` | Step-by-step execution trace |

## CLI

| Task | Read this skill file |
|------|---------------------|
| Understand architecture / "How does X work?" | `.claude/skills/gitnexus/gitnexus-exploring/SKILL.md` |
| Blast radius / "What breaks if I change X?" | `.claude/skills/gitnexus/gitnexus-impact-analysis/SKILL.md` |
| Trace bugs / "Why is X failing?" | `.claude/skills/gitnexus/gitnexus-debugging/SKILL.md` |
| Rename / extract / split / refactor | `.claude/skills/gitnexus/gitnexus-refactoring/SKILL.md` |
| Tools, resources, schema reference | `.claude/skills/gitnexus/gitnexus-guide/SKILL.md` |
| Index, status, clean, wiki CLI commands | `.claude/skills/gitnexus/gitnexus-cli/SKILL.md` |

<!-- gitnexus:end -->
