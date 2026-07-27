# 工作流: 嵌入式验证子流程

> 被 `verify.md` 在嵌入式项目时引用。

---

## 执行流程

在通用 verify 的并行执行通道中，嵌入以下三步：

```
Agent-A: 编译
  → 调用 build-dispatcher（自动检测 Keil/CMake/GCC）
  → 收集编译结果（0 error, 0 warning？）

Agent-B: 静态分析
  → 14 项编码规范检查（references/ela-rules-quick.md）
  → MISRA-C 子集检查
  → 编译警告检查

Agent-C: 回顾性代码审查
  → 逻辑正确性
  → 边界条件
  → 资源竞争
```

## 中断安全检查

- ISR 中无 `HAL_Delay`、`printf`、`malloc`、`free`
- 共享变量访问有关中断保护

## Keil 工程同步

检测到新 `.c` 文件 → 提示运行 keil-config 工作流：
```bash
python <BASE_DIR>/plugins/embedded/tools/keil-config/scripts/keil_config.py --scan <项目目录>
```
