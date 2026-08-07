/*****************************************************************************
 * @文件: ela_mt6816_drv.h
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: MT6816 磁编码器硬件驱动层，SPI 原语
 ****************************************************************************/

#ifndef ELA_MT6816_DRV_H
#define ELA_MT6816_DRV_H

#include <stdint.h>
#include "main.h"

/* ==== 常量定义 ==== */
/* SPI 片选引脚电平转换 */
#define MT6816_CS_HIGH() \
    HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, \
                      GPIO_PIN_SET)
#define MT6816_CS_LOW() \
    HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, \
                      GPIO_PIN_RESET)

/* ==== 接口 ==== */
/********
 * @输入: data_tx: 发送给编码器的 16 位命令
 * @输出: 从编码器接收到的 16 位原始数据
 * @说明: 单次 CS 周期内完成 16 位 SPI 收发
 ********/


uint16_t DRV_MT6816_SpiTransfer(uint16_t data_tx);

/********
 * @说明: 初始化 MT6816 编码器 CS 引脚初始状态为高电平
 * @注意: SPI1 已在 MX_SPI1_Init() 中初始化
 ********/
void DRV_MT6816_Init(void);

#endif






