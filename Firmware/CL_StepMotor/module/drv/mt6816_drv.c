/*****************************************************************************
 * @文件: mt6816_drv.c
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v2.0
 * @说明: MT6816 磁性编码器硬件驱动层（SPI 读角度原语 + 偶校验协议，无业务逻辑）
 * @平台: STM32F103RET6 (SPI1, CS=PA4)
 * @依赖: HAL_SPI, HAL_GPIO
 ****************************************************************************/
#include "mt6816_drv.h"
#include "spi.h"
#include "main.h"

/* ==== 常量定义 ==== */
#define MT6816_CMD_ANGLE       0x03U   /* 读角度(0x03=Angle<13:6>) */
#define MT6816_CMD_RAW_ANGLE   0x04U   /* 读原始角度(0x04=Angle<5:0>|NoMag|PC) */
#define MT6816_READ_BIT        0x80U   /* bit7=1 表示读 */

/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
/* ==== 内部工具 ==== */
/**
 * @输入 data_tx: 待发送 16 位数据
 * @输出 接收到的 16 位数据
 * @说明 SPI 发送并读取 16 位数据，CS 低电平选通、高电平释放
 * 依据 .cl/datasheet/pages/MT6816CT-ACD.ch00.p020.md: 模式3(CPOL=1,CPHA=1)
 *   数据传输开始于 CSN 下降沿，结束于 CSN 上升沿
 */
static uint16_t S_SpiXfer16(uint16_t data_tx)
{
    uint16_t data_rx = 0;

    HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_RESET);
    /* 寄存器级轮询：写 DR 后等 RXNE，规避 HAL_SPI_TransmitReceive 逐调用状态检查开销
     * （T3 超预算调参 2026-08-15：HAL 路径 28µs → 寄存器路径预期 ~5µs）
     * 依据 .cl/datasheet/pages/MT6816CT-ACD.ch00.p020.md: 模式3(CPOL=1,CPHA=1)
     *   数据传输开始于 CSN 下降沿，结束于 CSN 上升沿；全双工写 DR 即触发移位接收
     * 注意：必须先置 SPE 使能外设（HAL_SPI_TransmitReceive 内部每次调用前也会
     *   __HAL_SPI_ENABLE，2026-08-15 实测缺此行导致 SCK 不产生、RXNE 永不置位死循环）
     */
    if (0U == (SPI1->CR1 & SPI_CR1_SPE))
    {
        SPI1->CR1 |= SPI_CR1_SPE;
    }
    SPI1->DR = data_tx;
    while (0U == (SPI1->SR & SPI_SR_RXNE))
    {
    }
    data_rx = (uint16_t)SPI1->DR;
    HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_SET);

    return data_rx;
}

/**
 * @输入 data: 16 位数据
 * @输出 1=奇数个1(奇校验) 0=偶数个1(偶校验)
 * @说明 计算偶校验位
 */
static uint8_t S_CalcParity(uint16_t data)
{
    uint8_t count = 0;
    uint8_t i;

    for (i = 0; i < 16; i++)
    {
        if (data & (0x0001U << i))
        {
            count++;
        }
    }
    return (count & 0x01U);
}

/* ==== 接口实现 ==== */
/**
 * @输入 raw_angle: 输出14位原始角度(0~16383); no_mag_flag: 输出无磁场标志(可传NULL)
 * @输出 true=偶校验通过(raw_angle/no_mag_flag 有效) false=偶校验失败
 * @说明 单帧读角度：0x83/0x84 双寄存器拼接 16 位数据，偶校验通过后解出角度
 * 依据 .cl/datasheet/pages/MT6816CT-ACD.ch00.p023.md: 角度寄存器
 *   0x03=Angle<13:6>, 0x04=Angle<5:0>|No_Mag_Warning|PC(偶校验)
 *   raw_data=(0x03<<8)|0x04; 14bit = raw_data>>2 (bit2~15)
 *   偶校验失败返回 false，重试策略由用户层决定（本层无业务）
 */
bool DRV_MT6816_ReadAngle(uint16_t *raw_angle, bool *no_mag_flag)
{
    uint16_t tx_buf[2];
    uint16_t rx_buf[2];
    uint16_t raw_data;

    tx_buf[0] = (uint16_t)((MT6816_READ_BIT | MT6816_CMD_ANGLE) << 8);
    tx_buf[1] = (uint16_t)((MT6816_READ_BIT | MT6816_CMD_RAW_ANGLE) << 8);

    rx_buf[0] = S_SpiXfer16(tx_buf[0]);
    rx_buf[1] = S_SpiXfer16(tx_buf[1]);

    raw_data = (uint16_t)(((rx_buf[0] & 0x00FFU) << 8) | (rx_buf[1] & 0x00FFU));

    if (0U != S_CalcParity(raw_data))
    {
        return false;
    }

    if (NULL != raw_angle)
    {
        *raw_angle = (uint16_t)(raw_data >> 2);
    }
    if (NULL != no_mag_flag)
    {
        *no_mag_flag = (bool)(raw_data & (0x0001U << 1));
    }
    return true;
}

/**
 * @输入 无
 * @输出 读回的原始 16 位数据
 * @说明 单次读角度命令(测试用)
 */
uint16_t DRV_MT6816_TestRead(void)
{
    return S_SpiXfer16((uint16_t)((MT6816_READ_BIT | MT6816_CMD_ANGLE) << 8));
}
