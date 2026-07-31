# Quick-Plan: 电源电压检测

## 目标
POW_MT6816 (PA0, ADC1_IN0) 电压采样 → 串口打印

## 改动清单
- 新增 `ELA_LIB/ela_pow_det_drv.c/h` — ADC 读取封装
- 新增 `ELA_LIB/ela_pow_det_usr.c/h` — 电压计算 + printf
- 修改 `ELA_LIB/elaco_main.c` — 初始化 + 循环调用
- 修改 `MDK-ARM/ELA_StepMotor.uvprojx` — Keil 工程同步

## 注意事项
- ADC1 已由 CubeMX 初始化，驱动层仅做转换启动 + 轮询读取
- 电压公式: V = adc_value / 4095 * 3.3V
- 每秒输出一次，避免串口刷屏
- 新增 .c 已加入 .uvprojx ELA_LIB Group，无需手动同步
