# ELA_LIB 库命名与格式规范

## 一、文件命名与位置

### 文件位置规则（必须遵守）

| 文件类型 | 位置 | 可修改 | 说明 |
|----------|------|:---:|------|
| `elaco_main.c/h` | `ELA_LIB/` | ✅ | 应用入口 |
| `ela_*.c/h` | `ELA_LIB/` | ✅ | 所有模块文件 |
| CubeMX 文件 | `Core/` | ❌ | **禁止修改** |

### 禁止行为

- ❌ 修改 `Core/` 下任何文件（除 `main.c` 的 `USER CODE BEGIN 2` 添加 `elaco_main()` 调用）
- ❌ 在 `Core/` 下创建任何用户文件
- ❌ 将用户代码混入 CubeMX 生成文件

### 正确的目录结构

```
项目根目录/
├── Core/
│   ├── Src/
│   │   ├── main.c          ← CubeMX 管理，只改 USER CODE 区域
│   │   ├── can.c           ← CubeMX 管理，禁止修改
│   │   └── ...
│   └── Inc/
│       ├── main.h
│       └── ...
├── ELA_LIB/                ← 所有用户文件
│   ├── elaco_main.c/h      ← 应用入口
│   ├── ela_queue.c/h
│   ├── ela_can_drv.c/h
│   ├── ela_can_usr.c/h
│   └── ...
├── Drivers/                ← CubeMX HAL 库，禁止修改
└── .em/                    ← EM-SKILL 状态目录
```

### 文件命名
1. 库文件统一使用 ela_ 前缀，如 ela_button.c、ela_uart.c
2. 主函数文件不使用 ela_ 前缀，保持 elaco_main.c/h
3. 头文件使用宏防止重复包含，格式：ELA_XXX_H（全大写，无前导下划线），如 ELA_BUTTON_H

## 二、函数命名
1. 函数名使用 **snake_case**（全小写+下划线）命名，格式为 ela_模块_动作_细节
   - ela_：对外接口前缀
   - 模块：模块名，全小写，如 utton、uart、mt6816
   - 动作_细节：功能描述，单词间用下划线分隔
   - 正确：ela_uart_send_string、ela_mt6816_read_angle、ela_queue_is_full
   - 错误：~~Uart_SendString~~（PascalCase）、~~ela_uart_sendstring~~（单词未分隔）
2. **static 内部函数**：不加 ela_ 前缀，格式为 模块_动作_细节
   - 正确：uart_wait_tx_empty、utton_read_pin、mt6816_calc_parity
3. **drv 层模块内部函数**：声明在 `ela_xxx_drv.h` 中、仅被同模块 usr 层调用，格式为 **模块_drv_动作**
   - 正确：`tmc5160_drv_select_chip`、`tmc5160_drv_enable`
   - 原因：同模块内部调用不需要 `ela_` 层前缀，但保留 `_drv_` 标记硬件层身份
   - 判断标准：是否会被**其他模块**或 `elaco_main.c` 直接调用？是 → 加 `ela_` 前缀；否 → 不加
5. HAL 库回调函数（如 HAL_UART_RxCpltCallback）保持原命名，不修改
6. **文件-模块绑定**：ela_xxx.c 内只允许提供 xxx 或 ela_xxx 前缀的函数，禁止将其他模块的代码混入。例外：中断处理函数（如 USART1_IRQHandler）、HAL库重写的弱函数（如 HAL_UART_MspInit、HAL_UART_RxCpltCallback）不受此限

## 三、变量命名
1. **全局变量**：以 g_ 开头，如 g_rx1_offset、g_tx1_complete
2. **局部变量**：全小写，无前缀，如 rx_buffer、temp_value、tx_data
3. **静态变量**：以 s_ 前缀，如 s_pressed、s_last_state、s_timer
4. **static 函数规则**：所有不对外暴露的内部函数，必须在 .c 文件顶部声明为 static，且不得在 .h 头文件中声明
5. **结构体全局变量**：以 g_ 开头、_st 结尾，如 g_mt6816_st，便于识别结构体类型
6. **状态机全局变量**：以 g_ 开头、_fsm 结尾，如 g_motor_mode_fsm

## 四、结构体与类型命名
1. **结构体全局变量**：以 g_ 开头、_st 结尾，如 g_mt6816_st
2. **结构体类型**：全大写，以 `_T` 结尾（短类型别名），或以 全大写名词 结尾（复杂数据结构）
   - 短类型用 `_T`：`QUEUE_T`、`TMC5160_CHIP_T`、`MOTOR_CONFIG_T`
   - 复杂结构用名词：`MOTOR_DATA`、`TMC5160_MOTION_PROFILE`、`BUTTON_CONFIG`
   - 两者不混用：同一模块内统一风格
3. **枚举类型**：全大写，必须以 MODE/STATE/STATUS 结尾，如 MOTOR_MODE、MOTOR_STATE、CONFIG_STATUS
4. **枚举值**：全大写，如 MODE_STOP、STATE_RUNNING、CONFIG_OK
5. **区分规则**：结构体 = 名词（ANGLE/CONFIG/DATA），枚举 = 状态词（MODE/STATE/STATUS），通过后缀词即可区分类型

## 五、宏定义命名
1. 功能常量：全大写 + 下划线，如 DMA_BUF_SIZE、LONG_PRESS_MS
2. 掩码：全大写 + _MASK 后缀，如 MT6816_STATUS_MASK
3. 状态码：全大写 + 前缀，如 MT6816_STATUS_NORMAL
4. **位操作宏**：统一使用 BIT(n) 宏，必须用 1UL 防止16位MCU溢出
   - 定义：#define BIT(n) (1UL << (n))
   - 正确：#define CMD_START BIT(0) 或 #define CMD_START (1UL << 0)
   - 错误：~~#define BIT(n) (1 << n)~~（无括号且无UL）
   - 错误：~~struct { uint8_t start : 1; }~~（位域不可移植）
5. **Magic Number**：所有数字常量必须用宏定义
   - 允许的例外：NULL、1、-1、sizeof 的操作数（用于循环、条件判断、空指针、类型长度）
   - 必须宏定义的：数组长度、缓冲区大小、超时时间、计算系数、阈值等其余任何数值
   - 正确：#define BUF_SIZE 256 ... uint8_t buf[BUF_SIZE];
   - 错误：~~uint8_t buf[256];~~（应定义宏说明含义）

## 六、函数文档注释格式
每个对外公开的函数必须在定义处添加以下格式的文档注释：

`
/****
 * @ 输入: <参数名>: <参数说明>（可选，void时不写）
 * @ 输出: <返回值说明>（可选，void时不写）
 * @ 说明: <功能描述>
 * @ 注意: <注意事项>（可选，如可重入性、中断安全性）
 ********/
`

- @ 输入 — 每个参数名 + 冒号 + 说明；多个参数换行
- @ 输出 — 返回值和类型说明
- @ 说明 — 功能描述
- @ 注意 — 可选，标注是否可重入、是否可在ISR中调用等
- 注释以 /******** 开头，****/ 结尾
- 静态（static）函数也需要文档注释

## 七、文件头注释格式
每个 .c 和 .h 文件必须在文件开头添加以下格式的文件头注释：

`
/****
 * @ 文件: <文件名.c/h>
 * @ 作者: ELACO
 * @ 日期: <YYYY-MM-DD>
 * @ 版本: <X.Y.Z>
 * @ 说明: <功能描述>
 ********/
`

## 八、代码格式
1. **缩进**：统一使用 4 个空格，不使用 Tab
2. **大括号**：换行风格（Egyptian 风格），左大括号不单独换行
3. **换行符**：使用 Windows CRLF 格式
4. **行尾**：不留多余空格
5. **行宽**：每行不超过 84 字符（含注释）
6. **文件末尾**：每个 .c 和 .h 文件最后必须空一行，否则 Keil 编译会报 Warning

## 九、头文件包含顺序
### .c 文件包含顺序
1. **本模块头文件**：`ela_xxx.h`（如 `ela_button.h`）
2. **HAL 库头文件**：`main.h`、`spi.h`、`usart.h` 等（drv 文件需要）
3. **标准库头文件**：`stdbool.h`、`string.h`、`stdint.h` 等

> 注意：模块 .c 文件**不**包含 `elaco_main.h`，避免循环依赖。
> `elaco_main.h` 只供 `elaco_main.c` 自身使用，不含模块共享内容。

### .h 文件自包含规则
每个 .h 文件必须自包含，即自己包含所需的最小依赖：
- 需要 bool 时包含 `stdbool.h`
- 需要 uint8_t 等时包含 `stdint.h`
- 不依赖外部 .h 文件先被包含

## 十、函数分层（hlp / drv / usr / cac）与文件组织

### 分层 → 文件映射

| 层 | 文件 | 说明 |
|----|------|------|
| **drv** | `ela_xxx_drv.c/h` | 独立文件，只碰 HAL |
| **usr** | `ela_xxx.c/h` 或 `ela_xxx_usr.c/h` | 独立文件，业务逻辑 |
| **hlp** | 放在 usr 文件中 | 纯计算，可放在 usr 文件 |
| **cac** | `elaco_main.c` | 仅回调分发 |

**核心规则：drv 和 usr 必须分文件，不允许混在同一文件中。**

### 文件命名规范

```
模块: can
├── ela_can_drv.c/h    ← drv 层（HAL 操作）
├── ela_can_usr.c/h    ← usr 层（协议解析 + 业务）
└── elaco_main.c       ← cac 层（回调分发）

模块: uart
├── ela_uart_drv.c/h   ← drv 层
├── ela_uart.c/h       ← usr 层（或 ela_uart_usr.c/h）
└── elaco_main.c

模块: queue（纯计算，无 drv）
└── ela_queue.c/h      ← 只有 usr + hlp，无需 drv 文件
```

### 分层定义

1. **hlp（Helper / 辅助函数）**：纯计算、数据转换、算法逻辑。零硬件依赖，不调用任何 HAL/寄存器。可独立于 MCU 运行和测试。
   - 判断标准：能否在 PC 上直接编译运行？能 → hlp
   - 示例：cycle_sub()、calc_parity()、map_range()
   - 位置：放在 usr 文件中

2. **drv（Driver / 硬件原语）**：HAL 的薄封装，1对1映射硬件操作，只做一件事。不包含业务逻辑。
   - 判断标准：是否直接调用 HAL/寄存器，且只做单一操作？是 → drv
   - 示例：uart_transmit_byte()、spi_read_write()、gpio_set_pin()
   - 特征：函数体内通常只有 1-3 个 HAL 调用，无条件判断业务逻辑
   - **必须放在独立的 `ela_xxx_drv.c/h` 文件中**

3. **usr（User / 业务接口）**：组合多个 drv 函数实现用户级功能。是"随时可以掏出来用"的对外接口。
   - 判断标准：是否调用了多个 drv 或有业务逻辑？是 → usr
   - 示例：ela_uart_send_string()（循环调用 uart_transmit_byte）、ela_can_usr_process()（解析命令 + 分发）
   - 特征：函数体内有循环/条件/多步操作，面向用户需求
   - **必须放在独立的 `ela_xxx.c/h` 或 `ela_xxx_usr.c/h` 文件中**

4. **cac（Callback / 回调函数）**：HAL 回调函数集中在 elaco_main.c 定义（HAL_TIM_PeriodElapsedCallback、HAL_CAN_RxFifo0MsgPendingCallback 等）。回调内部只做分发，具体业务逻辑移至对应模块的 usr 区段。
   - elaco_main.c: 放 HAL 回调函数本体
   - elaco_main.c: 回调内只调用模块接口（如 ela_can_usr_process()）
   - 各模块 drv .c: 放被回调调用的具体实现函数

### drv 函数暴露策略

drv 层的函数暴露方式有两种，**推荐 Pattern B**：

| 模式 | 做法 | 示例 | 适用场景 |
|------|------|------|----------|
| **A: 全 static + `ela_` 包装** | drv 函数全 `static`，额外写 `ela_xxx_drv_` 包装函数对外暴露 | `ela_can_drv.c` | drv 内部函数较多，需精简对外接口 |
| **B: 非 static 直接暴露** | drv 函数不加 `static`，直接在 `ela_xxx_drv.h` 中声明，usr 层直接调 | `ela_tmc5160_drv.c` | drv 函数少，无需再包一层（**推荐**） |

### 文件段标记约定

每个 `.c` 文件内部用段标记划分功能区，**使用具体模块名**：

```c
/* tmc5160 hlp start */    ← hlp 段（纯计算）
/* tmc5160 hlp end */
//----------------------------------------------------------------------------------
/* tmc5160 drv start */    ← drv 段（HAL 原语）
/* tmc5160 drv end */
//----------------------------------------------------------------------------------
/* tmc5160 usr start */    ← usr 段（业务逻辑）
/* tmc5160 usr end */
```

- **格式**：`/* 模块名_层 start/end */`，连字符 `-` 保留
- **废弃** `/* module xxx start */` 这种通用占位，必须写具体模块名
- **空段允许**：某层没有函数时，段标记可以保留作为占位，也可以删除不写

### 分层判断流程图
`
函数是否直接操作硬件？
├── 否 → hlp（纯计算）
└── 是 → 是否只做单一硬件操作？
    ├── 是 → drv（硬件原语）
    └── 否 → 是否有多步操作/业务逻辑？
        ├── 是 → usr（业务接口）
        └── 否 → drv
`

### 决策：是否需要创建 drv 文件？

开始编码前，AI **必须**先判断模块是否需要独立的 drv 文件：

```
需求描述
  ↓
模块是否涉及硬件操作（GPIO/SPI/UART/I2C/TIM/ADC/DAC/等 HAL 或寄存器）？
├── 否 → 单文件 ela_xxx.c/h（纯 hlp/usr，如 ela_queue、ela_pid）
├── 是 → 需要创建：
│   ├── ela_xxx_drv.c/h  ← 硬件原语层（HAL 封装 + 引脚定义）
│   └── ela_xxx_usr.c/h  ← 业务逻辑层（组合 drv 实现功能）
│
└── ⚠️ 不确定时列硬件依赖清单让用户确认
```

**判断依据**（满足任意一条即需要 drv 文件）：
1. 调用任何 `HAL_` 函数（`HAL_GPIO_`/`HAL_SPI_`/`HAL_UART_` 等）
2. 直接读写 MCU 寄存器
3. 需要操作物理引脚（CS/SCK/MOSI/MISO/INT/EN 等）
4. 需要配置外设时序/波特率/采样率等硬件参数
5. 涉及中断向量或 DMA 通道

**不需要 drv 文件的特例**：
- 纯算法模块（PID、CRC、滤波器、查表）
- 纯数据结构（队列、环形缓冲、链表）
- 纯协议数据转换（不涉及硬件 I/O 的协议）

### 分层示例（UART 模块 — 两文件分拆）

**`ela_uart_drv.h`** — 硬件原语声明（只暴露 drv 层函数给 usr 调用）：

```c
/****
 * @ 文件: ela_uart_drv.h
 * @ 作者: ELACO
 * @ 日期: YYYY-MM-DD
 * @ 版本: 1.0.0
 * @ 说明: UART 硬件驱动层，HAL 原语
 ********/
#ifndef ELA_UART_DRV_H
#define ELA_UART_DRV_H

#include <stdint.h>

/* 引脚/外设宏定义 */
#define UART_BAUDRATE       115200
#define UART_TX_PORT        GPIOA
#define UART_TX_PIN         GPIO_PIN_9

/* ---- HAL 原语 ---- */
void uart_drv_transmit_byte(uint8_t byte);
uint8_t uart_drv_receive_byte(void);

#endif
```

**`ela_uart_drv.c`** — 硬件原语实现（唯一允许碰 HAL 的文件）：

```c
/****
 * @ 文件: ela_uart_drv.c
 * @ 作者: ELACO
 * @ 日期: YYYY-MM-DD
 * @ 版本: 1.0.0
 * @ 说明: UART 硬件驱动层，HAL 原语
 ********/
#include "ela_uart_drv.h"
#include "usart.h"

/* uart drv start */

/****
 * @ 输入: byte: 要发送的字节
 * @ 输出: 无
 * @ 说明: HAL 串口单字节发送，阻塞 100ms
 ********/
void uart_drv_transmit_byte(uint8_t byte)
{
    HAL_UART_Transmit(&huart1, &byte, 1, 100);
}

/****
 * @ 输入: 无
 * @ 输出: 接收到的字节
 * @ 说明: HAL 串口单字节接收，阻塞等待
 ********/
uint8_t uart_drv_receive_byte(void)
{
    uint8_t byte;
    HAL_UART_Receive(&huart1, &byte, 1, HAL_MAX_DELAY);
    return byte;
}

/* uart drv end */
```

**`ela_uart_usr.h`** — 业务接口声明：

```c
/****
 * @ 文件: ela_uart_usr.h
 * @ 作者: ELACO
 * @ 日期: YYYY-MM-DD
 * @ 版本: 1.0.0
 * @ 说明: UART 业务逻辑层
 ********/
#ifndef ELA_UART_USR_H
#define ELA_UART_USR_H

#include <stdint.h>

void ela_uart_send_string(const char *str);
void ela_uart_send_packet(uint8_t *data, uint16_t len);

#endif
```

**`ela_uart_usr.c`** — 业务逻辑实现：

```c
/****
 * @ 文件: ela_uart_usr.c
 * @ 作者: ELACO
 * @ 日期: YYYY-MM-DD
 * @ 版本: 1.0.0
 * @ 说明: UART 业务逻辑层
 ********/
#include "ela_uart_usr.h"
#include "ela_uart_drv.h"

/* uart hlp start */

/****
 * @ 输入: data: 数据指针, len: 数据长度
 * @ 输出: CRC16 校验值
 * @ 说明: 计算 CRC16 校验（纯计算，无硬件依赖）
 ********/
static uint16_t calc_crc16(uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    /* ... CRC 计算逻辑 ... */
    return crc;
}

/* uart hlp end */
//----------------------------------------------------------------------------------
/* uart usr start */

/****
 * @ 输入: str: 以 null 结尾的字符串
 * @ 输出: 无
 * @ 说明: 调用 drv 原语循环发送字符串
 ********/
void ela_uart_send_string(const char *str)
{
    while (*str)
    {
        uart_drv_transmit_byte(*str++);
    }
}

/****
 * @ 输入: data: 数据包指针, len: 数据包长度
 * @ 输出: 无
 * @ 说明: 组帧（含 CRC）后逐字节发送
 ********/
void ela_uart_send_packet(uint8_t *data, uint16_t len)
{
    uint16_t crc = calc_crc16(data, len);
    uart_drv_transmit_byte(0xAA);        // 帧头
    /* ... 更多组帧逻辑 ... */
}

/* uart usr end */
```

### 分层边界速查表
| 场景 | 层级 | 理由 |
|------|------|------|
| 计算CRC校验值 | hlp | 纯算法，无硬件依赖 |
| 调用 HAL_SPI_TransmitReceive() | drv | 1对1 HAL封装 |
| 读取编码器角度（SPI读取+校验+查表） | usr | 组合了drv+hlp |
| HAL_UART_RxCpltCallback() | cac | 中断回调 |
| 查表插值计算 | hlp | 纯数据转换 |
| 设置GPIO引脚电平 | drv | 单一硬件操作 |
| 按键扫描（读GPIO+消抖+状态机） | usr | 多步业务逻辑 |

### 模块独立性约束（强制）

1. **头文件独立**: 每个 ela_xxx.h 只 include 自己需要的头文件
   - 允许: stdint.h, stdbool.h, 本模块 drv 需要的 HAL 头文件
   - 禁止: elaco_main.h, 其他 ela_xxx.h
   - 原因: 避免循环依赖，实现模块独立编译

2. **drv 区段自包含**: drv 区段是唯一接触 HAL 的地方
   - hlp/usr 区段禁止 #include stm32f4xx_hal_xxx.h
   - drv 区段必须明确列出所有 HAL handle 和引脚依赖

3. **移植检查清单**: 复制模块后必须检查
   - [ ] 头文件是否独立（不依赖 elaco_main.h）
   - [ ] drv 区段 HAL handle 是否适配新项目
   - [ ] 引脚定义是否查表填写（参考核心文档引脚映射表）
   - [ ] 编译通过 + 功能正常

## 十一、volatile 使用规则
以下场景必须使用 volatile 关键字：

1. **中断共享变量**：在 ISR 中修改、在主循环或任务中读取的变量必须声明为 volatile
   - 正确：volatile bool g_rx_complete;
   - 错误：~~bool g_rx_complete;~~（编译器可能优化掉读取）
2. **硬件寄存器映射**：直接访问硬件寄存器时必须使用 volatile
   - 正确：*(volatile uint32_t *)0x40021000 |= BIT(0);
   - 错误：~~*(uint32_t *)0x40021000 |= BIT(0);~~
3. **编译器可能优化掉的循环**：如等待硬件就绪的忙等循环
   - 正确：while (REG & READY_BIT) { /* volatile read */ }

## 十二、内存对齐规范
1. **默认自然对齐**：结构体使用编译器默认对齐方式，不强制 #pragma pack
2. **特定场景允许对齐控制**：
   - 协议帧结构（如 CAN 报文、UART 协议包）可使用 #pragma pack(1)
   - Flash/EEPROM 存储结构可使用 #pragma pack(1)
3. 使用 #pragma pack 时必须在结构体定义前后成对出现：
   `c
   #pragma pack(push, 1)
   typedef struct {
       uint8_t header;
       uint16_t data;
       uint8_t crc;
   } PROTOCOL_FRAME;
   #pragma pack(pop)
   `

## 十三、中断安全规范
1. **ISR 中禁止调用**：
   - 阻塞 API（HAL_Delay、vTaskDelay）
   - printf、malloc、free
   - 任何可能导致死锁的函数
2. **ISR 中共享变量**必须声明为 volatile
3. **ISR 应尽量简短**：只做置标志或发信号，复杂逻辑延迟到主循环处理
4. **临界区保护**：访问 ISR 共享变量时需关中断保护（__disable_irq() / __enable_irq() 或 	askENTER_CRITICAL()）

## 十四、错误处理规范
1. **致命错误**（初始化失败、硬件异常）：直接调用 Error_Handler()，系统停止
   `c
   if (HAL_OK != HAL_SPI_Init(&hspi1)) {
       Error_Handler();
   }
   `
2. **非致命错误**（通信超时、数据校验失败）：记录日志，返回错误码，允许继续运行
   `c
   if (HAL_OK != HAL_UART_Receive(&huart1, &data, 1, 100)) {
       ELA_LOGW("UART receive timeout");
       return ELA_ERR_TIMEOUT;
   }
   `
3. **错误码定义**：模块应定义统一的错误码枚举，如 ELA_OK、ELA_ERR_TIMEOUT、ELA_ERR_PARAM
   - 错误码枚举在各模块 `.h` 中自行定义，以 `模块_PREFIX` 格式避免冲突
   - 日志宏（如 `ELA_LOGW`）是可选工具，非强制；无统一日志库时可自行实现或直接用 `printf`
4. **条件判断使用 Yoda 风格**：常量在比较运算符左侧，防止误写赋值
   `c
   if (HAL_OK != status) { ... }  // 正确
   if (status != HAL_OK) { ... }  // 容易误写成 if (status = HAL_OK)
   `

## 十五、栈使用规范
1. **禁止递归函数**：嵌入式系统栈空间有限，递归容易导致栈溢出
2. **大数组使用 static 分配**：局部数组超过 64 字节时，应声明为 static
   `c
   // 错误：可能栈溢出
   void func(void) {
       uint8_t buf[256];  // 256字节在栈上
   }
   // 正确：static分配
   void func(void) {
       static uint8_t buf[256];
   }
   `
3. **栈水位检查**：定期使用调试工具（如 FreeRTOS 的 uxTaskGetStackHighWaterMark()）检查栈使用情况
4. **函数局部变量总和**不应超过栈预算的 80%

## 十六、函数宏 vs 内联函数
1. **简单操作用宏**：如 BIT(n)、MIN(a,b)、MAX(a,b)，性能最稳定
2. **复杂逻辑用 static inline**：多条语句、有条件判断的函数，类型安全
3. **宏必须加括号**：参数和整体都要加括号，防止运算优先级问题
   - 正确：#define MIN(a, b) ((a) < (b) ? (a) : (b))
   - 错误：~~#define MIN(a, b) a < b ? a : b~~

## 十七、配置管理三态模式
模块配置生命周期使用三态状态机管理，避免在业务代码中散落 EEPROM/Flash 操作：

`c
typedef enum {
    CONFIG_RESTORE = 0,  /* 恢复出厂：写入默认值→复位系统 */
    CONFIG_OK,           /* 正常运行：不触发任何存储操作 */
    CONFIG_COMMIT        /* 提交保存：擦除Flash→写入新配置 */
} CONFIG_STATUS;
`

### 使用规范
1. **写入方**（如UART命令、按键）只修改配置字段并设置 CONFIG_COMMIT
2. **执行方**（主循环）检查状态并执行实际的存储操作：
   `c
   if (CONFIG_COMMIT == boardConfig.configStatus) {
       boardConfig.configStatus = CONFIG_OK;
       EEPROM_Write(0, &boardConfig, sizeof(BOARD_CONFIG));
   } else if (CONFIG_RESTORE == boardConfig.configStatus) {
       EEPROM_Write(0, &boardConfig, sizeof(BOARD_CONFIG));
       HAL_NVIC_SystemReset();
   }
   `
3. **好处**：存储操作集中在一处，避免多处调用 EEPROM_Write 导致的竞态和维护困难

## 十八、模块数据交互模式
### SetConfig 整包模式（低频配置）
适用于初始化和参数更新场景，模块通过接收配置结构体指针获得所有参数：

`c
/* 配置结构体 */
typedef struct {
    MOTION_PLANNER_CONFIG motionParams;
    CONTROLLER_CONFIG ctrlParams;
} MOTOR_CONFIG;

/* 模块提供SetConfig接口 */
void motor_set_config(MOTOR_CONFIG *config);

/* 调用方组装配置后一次性传入 */
motor_set_config(&motor_config);
`

**适用场景**：
- 模块初始化配置
- 用户修改参数后更新
- 校准数据写入

**特点**：模块不关心数据来源（EEPROM/CAN/默认值），只管接收使用，实现模块独立化。

### 直接读取模式（高频数据）
适用于传感器实时读取、遥测数据等高频场景：

`c
/* 模块提供直接读取接口 */
uint16_t mt6816_get_raw_angle(void);   /* 20kHz直接调用 */
float motor_get_position(bool is_lap); /* 100Hz直接调用 */
`

### 数据交互模式选择表
| 数据类型 | 访问频率 | 推荐模式 | 理由 |
|----------|----------|----------|------|
| 模块配置（PID参数、限值） | 低频（初始化/用户修改） | SetConfig整包传入 | 一次性传递，无性能开销 |
| 校准数据 | 极低频（校准时一次） | SetConfig指针传入 | 数据量大，指针传递高效 |
| 传感器实时值 | 高频（kHz级） | 直接函数读取 | 中断中不能有额外开销 |
| 遥测/状态回传 | 中频（10-100Hz） | 直接函数读取 | 需要实时性 |
| 控制指令 | 中频（事件触发） | 直接函数设置 | 响应速度要求高 |

### 模块独立化原则
1. 模块通过 SetConfig 接收配置，不直接读取全局变量
2. 模块通过 Get 函数暴露状态，不直接暴露内部变量
3. **模块间引用**：
   - 尽量避免模块间相互引用头文件，优先通过 `elaco_main.c` 中转
   - **允许例外**：模块 A 明确依赖模块 B（如 `ela_can_usr` 依赖 `ela_queue`）→ 可直接 `#include "ela_queue.h"`，但必须在文件头注释中标注依赖关系：
     ```c
     /****
      * @ 依赖: ela_queue  （显式标注依赖模块）
      ********/
     ```
4. 模块内部状态用 static 变量封装，外部不可直接访问

## 十九、测试规范

EM-SKILL 定义两类测试，按场景选用：

### 测试分类总览

| 类型 | 适用场景 | 运行环境 | 是否阻塞主循环 | 文件命名 | 位置 |
|------|----------|----------|---------------|----------|------|
| **弱化测试** | 硬件快速验证（电机转不转、通信通不通）| MCU 真机 | ✅ 自带 while，替代主循环 | `ela_xxx_test.c` | `TEXT/` |
| **单元测试** | hlp 层纯算法逻辑验证 | PC 编译运行 | ❌ 不涉及主循环 | `ela_xxx_utest.c` | `TEXT/` |

### 弱化测试（硬件快速验证）

**定位**：烧录后直接运行，用 while(1) 循环驱动硬件动作，肉眼观察物理现象。
**刚需场景**：首次驱动电机、通信环回验证、传感器数据读取。

#### 规则

1. **文件命名**：`ela_<模块名>_test.c`，统一放在 `TEXT/` 目录
   - 示例：`ela_tmc5160_test.c`、`ela_can_test.c`
2. **函数命名**：`ela_<模块名>_test(void)`，对外函数，声明在对应的头文件或 `elaco_main.h`
3. **函数结构**：必须包含自己的 `while(1)` 循环，运行后不返回
   ```c
   /****
    * @ 说明: TMC5160 弱化测试 — 电机正反转往复
    * @ 注意: 自带 while 循环，调用后不返回，替代 elaco_main 主循环
    ********/
   void ela_tmc5160_test(void)
   {
       ela_tmc5160_drv_init();

       while (1)
       {
           ela_tmc5160_usr_move(1000, 500);
           ela_tmc5160_usr_wait_stop(1000);
           HAL_Delay(2000);

           ela_tmc5160_usr_move(-1000, 500);
           ela_tmc5160_usr_wait_stop(1000);
           HAL_Delay(2000);
       }
   }
   ```
4. **调用方式**：在 `elaco_main()` 的 `while(1)` 之前调用，替代主循环
   ```c
   void elaco_main(void)
   {
       /* 模块初始化 */
       ela_tmc5160_drv_init();

       /* === 弱化测试 === */
       ela_tmc5160_test();   /* 自带 while，不返回 */

       /* 主循环（测试时不执行） */
       while (1)
       {
       }
   }
   ```
5. **启用/禁用**：手动注释/取消注释 `elaco_main()` 中的测试调用；不做条件编译宏（避免代码膨胀）
6. **测试内容**：只驱动硬件动作（转/发/读/闪），不写断言、不输出日志、不校验结果——结果由人眼观察
7. **完成标准**：测试函数正常运行后，删除测试调用，恢复主循环

#### 与 elaco_main.c 的职责关系

```
elaco_main()
├── 模块初始化           ← 正常保留
├── ela_xxx_test();     ← 弱化测试时取消注释，测试完删掉
└── while(1) { ... }    ← 测试时不执行，由测试函数的 while 替代
```

**禁止**：
- ❌ 弱化测试代码常驻 `elaco_main.c`（测试完必须删除测试调用行）
- ❌ 在弱化测试中使用 `printf`、断言、日志输出（物理观察不需要这些）
- ❌ 弱化测试调用 HAL 回调或中断处理（单线程硬件扫荡即可）

### 单元测试

**定位**：脱离 MCU 的纯算法测试，hlp 层函数专用。

#### 规则

1. **文件命名**：`ela_<模块名>_utest.c`，放在 `TEXT/` 目录
2. **测试框架**：推荐 Unity（最轻量，适合嵌入式）
3. **适用范围**：hlp 层函数（纯计算、校验、转换）——零 HAL 依赖，可在 PC 编译运行
4. **drv/usr 层函数**不强制写单元测试（硬件 mock 成本 > 收益）

### TEXT/ 目录结构

```
项目根目录/
├── Core/                   ← CubeMX 文件（禁止修改）
├── Drivers/                ← HAL 库
├── ELA_LIB/                ← 用户模块
│   ├── elaco_main.c/h
│   ├── ela_tmc5160_drv.c/h
│   └── ela_tmc5160_usr.c/h
└── TEXT/                   ← 弱化测试文件（与 ELA_LIB 同级）
    ├── ela_tmc5160_test.c
    ├── ela_can_test.c
    └── ela_mt6816_test.c
```

### Keil 工程同步

- TEXT/ 下的 .c 文件**加入** `.uvprojx` 工程组（不编译就无法运行测试）
- 建议单独建一个 `TEXT` 组存放，与 `ELA_LIB` 组分开，方便识别
- 测试完成后手动从 Keil 工程移除即可

## 二十、elaco_main.c 职责约束

### 与 CubeMX main.c 的关系

```
Core/Src/main.c (CubeMX管理,不可改)    ELA_LIB/elaco_main.c (应用入口)
┌─────────────────────┐           ┌─────────────────────┐
│ int main(void)      │           │ void elaco_main(void)│
│ {                   │           │ {                   │
│   HAL_Init();       │           │   ela_xxx_init();   │
│   SystemClock_Config│           │   while(1) { ... }  │
│   MX_GPIO_Init();   │  调用 →   │ }                   │
│   MX_CAN1_Init();   │ ────────  │                     │
│   ...               │           │ HAL 回调函数本体      │
│   elaco_main(); ←───┼───────────│ (只分发,不实现)       │
│ }                   │           │                     │
└─────────────────────┘           └─────────────────────┘
```

**关键规则**：
- CubeMX `main.c` 保留 `main()` + 所有外设初始化，**不要修改**（除 `USER CODE BEGIN 2` 添加 `elaco_main()` 调用）
- `elaco_main.c/h` 放在 `ELA_LIB/`，不在 `Core/`
- `elaco_main()` 内含主循环 `while(1)` + 模块初始化调用
- HAL 回调函数（如 `HAL_CAN_RxFifo0MsgPendingCallback`）也在 `elaco_main.c` 中定义

### elaco_main.c 允许放的内容

1. **主循环**: elaco_main() — 只做调度，不做业务逻辑
2. **HAL 回调函数本体**: HAL_TIM_PeriodElapsedCallback、HAL_CAN_RxFifo0MsgPendingCallback 等
3. **模块初始化调用**: 在 elaco_main() 开头调用各模块 Init()

### 禁止放的内容

1. **业务逻辑** → 放对应模块的 usr 区段
2. **协议解析** → 放对应通信模块的 usr 区段
3. **测试代码** → 放 `TEXT/` 目录
4. **非 HAL 回调的函数** → 归到各自模块文件

### 回调函数原则

HAL 回调函数留在 elaco_main.c，内部只做分发：

```c
/* elaco_main.c — cac 区段：只分发，不实现 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim5)
    {
        ela_button_scan_proc();     /* 具体实现在 ela_button.c */
    }
    if (htim == &htim9)
    {
        ela_pid_calc();             /* 具体实现在 ela_pid.c */
    }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    ela_can_receive_message();      /* 具体实现在 ela_can.c */
}
```

### 行数约束

elaco_main.c 总行数 ≤ 200 行。超过时检查是否有业务逻辑混入，应移至对应模块。

## 二十一、代码验证规则

模块代码必须通过以下验证才能标记为"可复用"。**逐项检查，任一不通过则不可复用。**

### 1. 文件头注释验证

```
检查项: 每个 .c 和 .h 文件必须有文件头注释
格式:
/****
 * @ 文件: <文件名.c/h>
 * @ 作者: ELACO
 * @ 日期: <YYYY-MM-DD>
 * @ 版本: <X.Y.Z>
 * @ 说明: <功能描述>
 * @ 依赖: <模块名>（可选，跨模块引用时必填）
 ********/

通过条件: 5 个字段全部填写，无空值
         跨模块引用其他 ela_xxx.h 时，必须填写 @ 依赖
```

### 2. 头文件保护名验证

```
检查项: 头文件保护宏必须为 ELA_XXX_H 格式
正确: #ifndef ELA_QUEUE_H / #define ELA_QUEUE_H
错误: #ifndef _ELA_QUEUE_H_ / #ifndef ela_queue_h

通过条件: 全大写，ELA_ 前缀，_H 后缀，无前导/尾随下划线
```

### 3. 头文件独立性验证

```
检查项: .h 文件不能 include elaco_main.h 或其他 ela_*.h
正确: #include <stdint.h>（标准库）
错误: #include "elaco_main.h"

通过条件: 只 include 标准库头文件 + 本模块 drv 需要的 HAL 头文件
验证方法: grep -n "elaco_main\|ela_" *.h 应返回空
```

### 4. 函数命名验证

```
检查项: 对外函数必须为 ela_模块_动作 格式（snake_case）
正确: ela_queue_insert, ela_tmc5160_read_reg
错误: Queue_Insert（PascalCase）, ela_queueinsert（单词未分隔）

通过条件:
  - 以 ela_ 开头
  - 全小写 + 下划线分隔
  - 模块名 + 动作 + 细节（可选）
```

### 5. 静态函数命名验证

```
检查项: static 函数必须为 模块_动作 格式（无 ela_ 前缀）
正确: uart_transmit_byte, tmc5160_write_reg
错误: ela_uart_transmit_byte（static 不加 ela_）

通过条件: static 函数无 ela_ 前缀
```

### 6. 变量命名验证

```
检查项: 全局变量必须为 g_ 前缀
正确: g_queue_st, g_rx1_offset
错误: can_queue（无前缀）, CanQueue（PascalCase）

通过条件:
  - 全局变量: g_ 前缀
  - 结构体全局: g_ + _st 后缀
  - 状态机全局: g_ + _fsm 后缀
  - 静态变量: s_ 前缀
```

### 7. 类型命名验证

```
检查项: 结构体类型必须为全大写
正确: QUEUE_T（短类型别名）、MOTOR_DATA（复杂结构）
错误: Queue（PascalCase）、queue_t（小写）

通过条件: 全大写，以 _T 或 全大写名词 结尾
```

### 8. 宏命名验证

```
检查项: 宏必须为全大写 + 下划线
正确: QUEUE_SIZE, QUEUE_ELEM_SIZE, BIT(n)
错误: queue_size（小写）, QUEUE_SIZE（无分隔）

通过条件: 全大写，单词间用下划线分隔
```

### 9. 函数注释验证

```
检查项: 每个对外函数必须有文档注释
格式:
/****
 * @ 输入: param: 说明（void 时省略）
 * @ 输出: 返回值说明（void 时省略）
 * @ 说明: 功能描述
 * @ 注意: 可重入性/中断安全性（可选）
 ********/

通过条件:
  - 以 /**** 开头，****/ 结尾
  - 包含 @ 输入（有参数时）
  - 包含 @ 输出（非 void 时）
  - 包含 @ 说明
```

### 10. drv/usr 分文件验证

```
检查项: drv 和 usr 必须在独立文件中
正确:
  ela_can_drv.c/h  ← 只含 HAL 操作
  ela_can_usr.c/h  ← 只含业务逻辑
错误:
  ela_can.c  ← 同时包含 drv 和 usr 函数

通过条件: drv 函数只出现在 *_drv.c 中，usr 函数只出现在 *_usr.c 或 *.c 中
```

### 11. drv 文件验证

```
检查项: drv 文件只含 HAL 操作
验证方法: drv 文件内不应出现业务逻辑（循环/条件判断处理业务）
正确: drv 文件只做单一 HAL 封装
错误: drv 文件内有协议解析、状态机等业务逻辑

通过条件: drv 文件函数体只有 1-3 个 HAL 调用，无业务逻辑
```

### 12. 代码格式验证

```
检查项:
  - 缩进: 4 空格（无 Tab）
  - 大括号: Egyptian 风格（左大括号不换行）
  - 行宽: ≤ 84 字符
  - 换行符: CRLF
  - 文件末尾: 空一行
  - 段标记: 使用 /* 模块名_层 start/end */，不用 /* module xxx start */

正确: /* tmc5160 drv start */
错误: /* module drv start */

通过条件: 无 Tab、无超长行、文件末尾有空行、段标记使用具体模块名
```

### 13. Magic Number 验证

```
检查项: 除 NULL/1/-1/sizeof 外，所有数字常量必须宏定义
正确: #define QUEUE_SIZE 48 ... uint8_t buf[QUEUE_SIZE];
错误: uint8_t buf[48];（48 未宏定义）

通过条件: grep -n "[0-9]" *.c | 排除 NULL/1/-1/sizeof 后无裸数字
```

### 14. volatile 验证

```
检查项: ISR 共享变量必须声明 volatile
正确: volatile uint8_t g_flag;
错误: uint8_t g_flag;（在 ISR 中修改）

通过条件: ISR 中修改的变量在声明时有 volatile 关键字
```

### 审核输出格式

```
📋 模块审核: <模块名>

□ 文件头注释 ............. ✅ / ❌   (含 @ 依赖 检查)
□ 头文件保护名 ........... ✅ / ❌
□ 头文件独立性 ........... ✅ / ❌
□ 函数命名 ............... ✅ / ❌
□ 静态函数命名 ........... ✅ / ❌
□ 变量命名 ............... ✅ / ❌
□ 类型命名 ............... ✅ / ❌
□ 宏命名 ................. ✅ / ❌
□ 函数注释 ............... ✅ / ❌
□ 段标记命名 ............. ✅ / ❌   (模块名_层，不用 module)
□ drv 区段验证 ........... ✅ / ❌
□ 代码格式 ............... ✅ / ❌
□ Magic Number .......... ✅ / ❌
□ volatile .............. ✅ / ❌

结果: ✅ 通过（14/14）/ ⚠️ 需修改（X/14）/ ❌ 不通过
```