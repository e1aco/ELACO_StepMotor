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

## 通信协议
- CAN — 已配置 166kbps，队列缓冲已实现
- Modbus RTU/ASCII — FreeModbus 集成，USART1 RS-485
- USART3 — DMA 调试口，printf 重定向

## 测试
- ModTest 模式：test_mt6816 / test_tb67h450 / test_position

## 进行中
- 位置闭环控制（test_position 测试中）
- 校准表生成（elaco_calibration）
- 电源电压检测（ela_pow_det_drv/usr — 🔄 验证中）

## 已知问题
- FreeModbus USART1 IRQHandler 命名冲突
- HSI 8MHz 开发中，生产需切 HSE 72MHz（所有定时器重算）
- 485 DE/RE 引脚控制未实现

## 下一步
- ela_cyclecal 精度验证 → 位置闭环调参 → 生产时钟切换
