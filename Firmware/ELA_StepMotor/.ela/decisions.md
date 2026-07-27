# 关键决策记录

## 2026-07-26 - 项目初始化

### 决策内容
ELA_StepMotor 项目初始化为 embedded 类型，基于 STM32F103RET6 + Keil MDK-ARM + CubeMX 工具链。

### 理由
项目使用 CubeMX 生成 HAL 初始化代码，Keil 作为编译环境，属于典型嵌入式开发流程。

### 影响
启用 build/flash/serial 扩展命令，代码生成遵循 ELA 编码规范。

---

## 2026-07-26 - 代码格式修复

### 决策内容
修复 2 个格式问题：
1. ela_can_queue.h 中 TRUE/FALSE 宏定义反转
2. 全部 20 个 ELA_LIB 文件缺少末尾空行

### 理由
TRUE/FALSE 宏定义反了会导致逻辑错误；文件末尾无空行会导致 Keil 编译 Warning。

### 影响
消除 Keil 编译警告，确保逻辑正确性。

### 参考
- ela_rules.md 第八条第6款：文件末尾必须空一行
- ela_rules.md 第五条第4款：宏命名全大写

---

## 历史归档

<!-- 已归档决策索引 -->
