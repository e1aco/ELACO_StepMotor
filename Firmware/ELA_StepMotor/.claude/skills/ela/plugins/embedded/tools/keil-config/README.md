# keil-config — Keil 工程 ELA_LIB 文件同步工具

自动将 `ELA_LIB/` 下的 `.c` 文件同步到 Keil `.uvprojx` 工程的 ELA_LIB 组。

## 用法

```bash
# 指定工程文件
python keil_config.py --project project.uvprojx

# 扫描目录自动找工程
python keil_config.py --scan .

# 预览模式（不写文件）
python keil_config.py --project project.uvprojx --dry-run

# 不传参数 = 扫描当前目录
python keil_config.py
```

## 功能

1. 扫描 `ELA_LIB/*.c`（相对于 `.uvprojx` 的 `../ELA_LIB/`）
2. 检查 `.uvprojx` 中 `<Group>ELA_LIB</Group>` 是否已有
3. 无则追加 `<File>` 节点（`FileType=1` C源文件）
4. 检查 `<IncludePath>` 是否包含 `../ELA_LIB`，无则追加
5. 所有 Target 都会同步

## 注册到插件

`plugins/embedded/PLUGIN.md` 的 `tools` 段注册：

```yaml
- name: keil-config
  path: tools/keil-config/scripts/keil_config.py
  kind: python
```
