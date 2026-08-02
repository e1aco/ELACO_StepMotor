/********
 * @ 文件: ela_mt6816_usr.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-23
 * @ 版本: 1.0.1
 * @ 说明: MT6816 磁编码器应用层接口，含奇偶校验和重试逻辑
 ********/

#include "ela_mt6816_usr.h"
#include "ela_mt6816_drv.h"
#include <string.h>

MT6816_ANGLE_T g_mt6816_st;

/* mt6816 usr start */

/********
 * @ 输入: data: 需要计算奇偶校验的 16 位数据
 * @ 输出: 0 表示偶数个 1（校验通过），1 表示奇数个 1（校验失败）
 * @ 说明: 计算 16 位数据的偶校验，返回奇偶校验结果
 ********/
static unsigned char mt6816_usr_even_parity(uint16_t data)
{
    unsigned char count_bits = 0;
    uint16_t temp = data;

    while (temp)
    {
        if (temp & 0x01)
        {
            count_bits++;
        }
        temp >>= 1;
    }

    return count_bits & 1;
}

/********
 * @ 说明: 初始化 MT6816 编码器，清除角度结构体
 ********/
void ela_mt6816_usr_init(void)
{
    memset(&g_mt6816_st, 0, sizeof(MT6816_ANGLE_T));
    ela_mt6816_drv_init();
}

/********
 * @ 输出: 角度结构体指针
 * @ 说明: 读取编码器角度，含重试和奇偶校验
 * @ 注意: 帧 1 发送 0x8300 读 ANGLE 寄存器（角度高 8 位），
 *         帧 2 发送 0x8400 读 RAW_ANGLE 寄存器（角度低 6 位 +
 *         bit1 弱磁报警 + bit0 偶校验位），拼装后整字偶校验
 ********/
MT6816_ANGLE_T *ela_mt6816_usr_read_angle(void)
{
    unsigned char retry = 0;
    uint16_t tx_angle;
    uint16_t tx_raw;
    uint16_t rx_angle = 0;
    uint16_t rx_raw = 0;

    tx_angle = (MT6816_CMD_READ_BIT | MT6816_CMD_ANGLE) << 8;
    tx_raw = (MT6816_CMD_READ_BIT | MT6816_CMD_RAW_ANGLE) << 8;

    for (retry = 0; retry < 3; retry++)
    {
        /* ANGLE 寄存器通过帧 1 读取 */
        rx_angle = mt6816_drv_spi_transfer(tx_angle);
        rx_raw = mt6816_drv_spi_transfer(tx_raw);

        g_mt6816_st.raw_data =
            ((rx_angle & 0x00FF) << 8) | (rx_raw & 0x00FF);

        if (0 == mt6816_usr_even_parity(g_mt6816_st.raw_data))
        {
            g_mt6816_st.data_valid = true;
            g_mt6816_st.raw_angle = g_mt6816_st.raw_data >> 2;
            g_mt6816_st.magnet_valid =
                (bool)(g_mt6816_st.raw_data & (0x0001 << 1));
            break;
        }

        g_mt6816_st.data_valid = false;
    }

    return &g_mt6816_st;
}

/* mt6816 usr end */

