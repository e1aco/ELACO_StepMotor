# /ela learn — 学习模式

## 功能
交互式教学 + 知识库沉淀检索。

## 触发
```
/ela learn teach <主题>   # 交互式教学
/ela learn kb            # 知识库管理
/ela learn status        # 学习进度
```

## 教学循环
讲解 → 示例 → 练习 → 反馈 → 复习

## 知识库结构
```
.ela/learning/kb/
├── debug/       # 调试经验（问题+方案）
├── concepts/    # 技术要点
└── snippets/    # 代码片段
```

## 自动沉淀
每次 `/ela result` 后询问是否沉淀到知识库。

## 相关
- `plugins/learning/PLUGIN.md` — 完整文档
- `commands/result.md` — 结果后自动触发沉淀询问
