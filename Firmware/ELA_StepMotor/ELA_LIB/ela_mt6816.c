/********
 * @ 文件: ela_mt6816.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-17
 * @ 版本: 1.0.1
 * @ 说明: MT6816 磁编码器驱动，SPI 读取角度数据
 ********/

#include "ela_mt6816.h"
#include "ela_mt6816_drv.h"
#include "spi.h"

/********
 * @ 说明: MT6816 重试次数
 ********/
#define MT6816_RETRY_COUNT  3

MT6816_ANGLE_T g_mt6816_st = {0};

/* ela_mt6816 hlp start */

/********
 * @ 输入: data: 需要计算奇偶校验的 16 位数据
 * @ 输出: 0 表示偶数个 1（校验通过），1 表示奇数个 1
 * @ 说明: MT6816 偶校验：整个 16 位中 1 的个数应为偶数
 ********/
static unsigned char mt6816_calc_parity(unsigned short data)
{
    unsigned char count = 0;
    unsigned char i;

    for (i = 0; i < 16; i++)
    {
        if (data & (0x0001 << i))
        {
            count++;
        }
    }
    return count & 0x01;
}

/* ela_mt6816 hlp end */
//----------------------------------------------------------------------------------
/* ela_mt6816 usr start */

/********
 * @ 说明: 获取 MT6816 编码器角度数据，更新全局结构体
 *         g_mt6816_st
 * @ 注意: MT6816 协议：两次独立 16 位传输，各自 CS
 *         周期，ANGLE 返回高字节，RAW_ANGLE 返回低字节，
 *         拼装后校验
 ********/
void ela_mt6816_get_angle(void)
{
    unsigned short tx_angle;
    unsigned short tx_raw;
    unsigned short rx_angle = 0;
    unsigned short rx_raw = 0;
    unsigned char i;

    tx_angle = (MT6816_CMD_READ_BIT | MT6816_CMD_ANGLE) << 8;
    tx_raw = (MT6816_CMD_READ_BIT | MT6816_CMD_RAW_ANGLE) << 8;

    for (i = 0; i < MT6816_RETRY_COUNT; i++)
    {
        rx_angle = mt6816_drv_spi_transfer(tx_angle);
        rx_raw = mt6816_drv_spi_transfer(tx_raw);

        g_mt6816_st.raw_data =
            ((rx_angle & 0x00FF) << 8) | (rx_raw & 0x00FF);

        if (0 == mt6816_calc_parity(g_mt6816_st.raw_data))
        {
            g_mt6816_st.data_valid = true;
            break;
        }
        else
        {
            g_mt6816_st.data_valid = false;
        }
    }

    if (g_mt6816_st.data_valid)
    {
        g_mt6816_st.raw_angle = g_mt6816_st.raw_data >> 2;
        g_mt6816_st.magnet_valid =
            (bool)(g_mt6816_st.raw_data & (0x0001 << 1));
    }
}

/* ela_mt6816 usr end */

