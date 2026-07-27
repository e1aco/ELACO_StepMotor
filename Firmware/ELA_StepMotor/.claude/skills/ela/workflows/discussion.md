# 工作流: 深度讨论（deep）

> 被 `new.md` 在 deep 档时引用。

**适用**：系统级、新外设、协议栈、状态机重构
**产出**：5 个文件（split / req / hardware / brainstorm / milestones）

---

## 阶段 1：拆分（split.md）

将大功能拆为独立子系统。>3 子系统时启动并行子 Agent：

```bash
Agent-1: 子系统 A 详细设计
Agent-2: 子系统 B 详细设计
Agent-3: 子系统 C 详细设计
```

## 阶段 2：需求对齐（req.md）

- 调用 `codegraph_explore` 查看现有接口
- 罗列依赖关系
- 对齐通信契约（数据结构、接口签名）

## 阶段 3：硬件对齐（hardware.md）

嵌入式项目强制（通用项目跳过）：
- 引脚分配检查
- 外设冲突检查（DMA、定时器、中断优先级）
- 资源预算（RAM/Flash/堆栈）

## 阶段 4：头脑风暴（brainstorm.md）

同 standard 档方案对比，但 level 更深：
- 增加实时性分析
- 增加错误处理策略

## 阶段 5：里程碑（milestones.md）

拆分实现步骤，每步可独立验证。

**禁止在讨论完成前编码**。
