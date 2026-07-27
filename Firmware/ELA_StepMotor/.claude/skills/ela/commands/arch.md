# /ela arch — 归档已完成步骤

## 功能
将已完成步骤归档到 `history/`。自动 git tag + CHANGELOG 更新。

## 触发
```
/ela arch
```

## 流程

1. 归档 sessions/ 到 history/
2. 归档 project-spec.md → history/（保留当前 skeleton）
3. 归档 problem-log.md / decisions.md → history/
4. git tag 当前状态
5. 更新 CHANGELOG
