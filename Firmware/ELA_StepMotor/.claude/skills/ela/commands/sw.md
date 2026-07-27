# /ela sw — 切换项目

## 功能
切换到另一个项目。自动 CodeGraph 增量同步。

## 触发
```
/ela sw <项目名称>
```

## 流程

1. 保存当前项目状态
2. 从全局索引查找目标项目
3. 切换目录
4. CodeGraph：按 `workflows/codegraph.md` 场景 C 处理同步
5. 加载目标项目状态
