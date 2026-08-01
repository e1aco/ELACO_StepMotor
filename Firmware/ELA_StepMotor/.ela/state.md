# ELA_StepMotor 项目状态

## 项目
ELACO 步进电机驱动器固件 — STM32F103RET6 + 双 TB67H450FNG H桥 + MT6816 编码器

## 已完成模块 (A~E 分级)
| 模块 | 分级 | 状态 |
|------|------|------|
| ela_uart (drv+usr) | drv | ✅ |
| ela_uart_queue | drv | ✅ |
| ela_can_queue | drv | ✅ |
| ela_mt6816 (drv+usr) | drv | ✅ |
| ela_tb67h450 (drv+usr) | drv | ✅ |
| ela_button (drv+usr) | drv | ✅ |
| ela_stockfile (drv+usr) | drv | ✅ |
| ela_cyclecal | drv | ✅ |
| elaco_calibration_usr | drv | ✅ |
| ela_pow_det (drv+usr) | drv | ✅ |

## 通信协议
- CAN — 已配置 166kbps，队列缓冲已实现
- Modbus RTU/ASCII — FreeModbus 集成，USART1 RS-485
- USART3 — DMA 调试口，printf 重定向

## 测试
- ModTest 模式：test_mt6816 / test_tb67h450 / test_position / test_position_cl
- 位置闭环 test_position_cl 已达标：4 段 90°（4096 计数）全部落在 ±8 计数容差内，
  连续 3 次 PASS（cl33/cl33b/cl33c，见 .ela/logs/position_cl33*.log）

## 进行中
- 位置闭环控制（test_position_cl 已达标，待整合进主流程）
- 校准表生成（elaco_calibration）

## 已知问题
- FreeModbus USART1 IRQHandler 命名冲突
- HSI 8MHz 开发中，生产需切 HSE 72MHz（所有定时器重算）
- 485 DE/RE 引脚控制未实现
- SPI 毛刺：静止读 100 次存在偶发 ±40 尖刺（机械/电气均已排除，见 noise_floor 日志），
  现以 ISR 内 3 次中值 + settle 多采样均值滤除

## 已修复（2026-08-01）
- MT6816 读取恒为 0：`ela_mt6816_usr_read_angle()` 原来发送 0xFFFF 哑命令，芯片返回 0。
  已改为 4 线 SPI 读寄存器命令（0x8300/0x8400）+ 整字偶校验 + 弱磁标志，
  USART3 printf 调试口（COM5 115200）实测 `raw:0x66C0 ang:6576 v:1 m:0`。

## 位置闭环调参历程（2026-08-02，cl6~cl33）
- cl6：唯一历史达标版本，源码丢失（文件未跟踪被覆盖），仅留日志参照
- cl14：增量取中值积分 → 陈旧读拉低增量，欠走 ~11% 废弃
- cl24/25：ISR 每周期 3 读、原始位置取中值、相对段起点回绕差（无累加链）；
  暴露"到位后开环悬停 200ms 漂移 ±25~37"（cl6 是闭环保持下测量）
- cl26：到位后保持 ISR 闭环带内测量，drft 归零 → 确认测量可信
- cl27：HOLD_MA 1000→2000（保持刚度翻倍，防爬出死区）
- cl28：settle 后 5 样本平均（滤边界抖动）
- cl29：DEADBAND 8→4（到位判定收紧）
- cl31：接近目标 |err|<64 时降速至 1 微步/周期（防高速冲入过冲）
- cl32：SETTLE_DELAY 200→500ms、10 样本×30ms 平均 → Seg0 仍间歇 ±40 极限环/超时
- cl33：ISR 加入速度估计 + 阻尼项（cmd -= vel>>1）→ 振荡压至 ±5，连续 3 次 PASS

## 关键参数（达标配置）
- MAX_DELTA=4, KP_SHIFT=6, DEADBAND=4, INBAND_CONFIRM=3, POS_TOL=8
- DRIVE_MA=2000, HOLD_MA=2000, SETTLE_DELAY=500, TIMEOUT=5000ms
- 接近目标降速 + 速度阻尼（cmd -= vel>>1）
- 电机：42 步进，1.8°，0.43Nm，2Ω，3.6mH，2A，285g，50 极对=51200 微步/圈
- 编码器：MT6816 14bit 16384 计数/圈，1 计数=0.022°；±8 计数≈0.18°

## 下一步
- 位置闭环整合进主流程（Modbus/CAN 命令化）
- 校准表生成（elaco_calibration）
- ela_cyclecal 精度验证 → 生产时钟切换（HSI→HSE）
