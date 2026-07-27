# /ela result — 记录验证结果

## 功能
提交验证结果。通过则留痕推进，失败则进入诊断循环。

## 触发
```
/ela result <结果>
```

## 通过路径

```
更新 HVR → 更新 state.md（S<N> ✅）→ 自动推进步骤编号
  → 核心文档留痕（追加开发进度到 .lnk 目标文件）
  → 更新 require-index.md（行号偏移重算）
  → 询问是否沉淀到学习知识库
```

## 失败路径

```
记录 problem-log（状态: open）
  → 进入诊断流程
  → 修复后重新 /ela verify
```

## 相关
- `commands/verify.md` — 步骤验证
- `commands/reindex.md` — 索引更新
