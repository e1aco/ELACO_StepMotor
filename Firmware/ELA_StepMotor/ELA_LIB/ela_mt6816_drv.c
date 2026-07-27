/********
 * @ 文件: ela_mt6816_drv.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-23
 * @ 版本: 1.0.0
 * @ 说明: MT6816 磁编码器硬件驱动层，SPI 原语
 ********/

#include "ela_mt6816_drv.h"
#include "ela_mt6816.h"
#include "spi.h"

/* mt6816 drv start */

/********
 * @ 输入: data_tx: 发送给编码器的 16 位命令
 * @ 输出: 从编码器接收到的 16 位原始数据
 * @ 说明: 单次 CS 周期内完成 16 位 SPI 收发
 ********/
uint16_t mt6816_drv_spi_transfer(uint16_t data_tx)
{
    uint16_t data_rx = 0;

    MT6816_CS_LOW();
    HAL_SPI_TransmitReceive(&hspi1,
                            (unsigned char *)&data_tx,
                            (unsigned char *)&data_rx,
                            1, HAL_MAX_DELAY);
    MT6816_CS_HIGH();

    return data_rx;
}

/********
 * @ 说明: 初始化 MT6816 编码器 CS 引脚初始状态为高电平
 * @ 注意: SPI1 已在 MX_SPI1_Init() 中初始化
 ********/
void ela_mt6816_drv_init(void)
{
    MT6816_CS_HIGH();
}

/* mt6816 drv end */

