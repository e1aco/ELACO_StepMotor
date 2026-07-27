#include "ela_mt6816.h"

MT6816_ANGLE_T gov_mt6816_st = {0}; // 角度获取结构体

//----------------------------------------------------------------------------------
/* -1- 辅助函数 */

/****
 * @ 原型: uint16_t MT6816_ReadData(uint16_t tx_data)
 * @ 输入: uint16_t tx_data: 发送给编码器的读取命令
 * @ 输出: uint16_t: 返回从编码器接收到的 16 位原始数据
 * @ 说明: 通过 SPI 读取 MT6816 的原始 16 位数据
 * @ 注意: SPI 通信格式为 16 位数据帧，Mode 3 (CPOL=1, CPHA=1)
 ********/
unsigned short MT6816_ReadData(unsigned short tx_data)
{
    unsigned short rx_data = 0x0000;

    /* CS 拉低，使能编码器 */
    MT6816_CS_LOW();
    /* SPI 收发：发送 16 位空数据，同时接收 16 位编码器数据 */
    HAL_SPI_TransmitReceive(&hspi1, (unsigned char *)&tx_data, (unsigned char *)&rx_data, 1, HAL_MAX_DELAY);
    /* CS 拉高，禁用编码器 */
    MT6816_CS_HIGH();

    return rx_data;
}

/****
 * @ 原型: static uint8_t MT6816_CalcParity(uint16_t data)
 * @ 输入: uint16_t data: 需要计算奇偶校验的数据
 * @ 输出: uint8_t: 返回计算得到的奇偶校验位，1表示奇数个1，0表示偶数个1
 * @ 说明: MT6816 的数据帧中包含一个奇偶校验位，需根据接收到的数据计算校验位以验证数据正确性,偶数个1表示数据正确
 ********/
static unsigned char MT6816_CalcParity(unsigned short data)
{
    unsigned char count = 0;
    for (unsigned char i = 0; i < 16; i++)
    {
        if (data & (0x0001 << i))
            count++;
    }
    return count & 0x01; /* 返回1表示奇数个1，0表示偶数个1 */
}

//----------------------------------------------------------------------------------
/* -2- 用户调用函数 */

/****
 * @ 原型: void MT6816_Init(void)
 * @ 输入: void
 * @ 输出: void
 * @ 说明: 初始化 MT6816 编码器相关引脚和 SPI 接口
 * @ 注意: SPI1 已在 MX_SPI1_Init() 中初始化，本函数仅确保 CS 引脚初始状态为高电平
 ********/
void MT6816_Init(void)
{
    /* 确保 CS 引脚初始为高电平（片选无效状态） */
    MT6816_CS_HIGH();
}
/****
 * @ 原型: void MT6816_GetAngle(void)
 * @ 输入: void
 * @ 输出: void
 * @ 说明: 获取 MT6816 编码器的角度数据，通过发送读取命令并解析返回的数据更新全局角度结构体 gov_mt6816_st
 * @ 注意: 该函数会尝试多次读取直到数据校验通过，成功获取角度数据后会更新 gov_mt6816_st 结构体中的字段
 ********/
void MT6816_GetAngle(void)
{
    unsigned short tx_data[2];
    unsigned short rx_data[2];

    tx_data[0] = (0x80 | MT6816_CMD_ANGLE) << 8;     // 0x8300
    tx_data[1] = (0x80 | MT6816_CMD_RAW_ANGLE) << 8; // 0x8400

    /* 尝试3次读取，直到校验通过 */
    for (unsigned char i = 0; i < 3; i++)
    {
        rx_data[0] = MT6816_ReadData(tx_data[0]); // 读取8位角度数据
        rx_data[1] = MT6816_ReadData(tx_data[1]); // 读取8位角度数据

        /* 组合数据 */
        gov_mt6816_st.raw_data = ((rx_data[0] & 0x00FF) << 8) | (rx_data[1] & 0x00FF); // 两个命令的返回值组合成一个16位数据

        /* 奇偶校验 */
        if (MT6816_CalcParity(gov_mt6816_st.raw_data) == 0)
        {
            /* 校验通过（偶数个1） */
            gov_mt6816_st.data_valid = true;
            break;
        }
        else
        {
            gov_mt6816_st.data_valid = false;
        }
    }

    if (gov_mt6816_st.data_valid)
    {
        /* 提取14位角度值（bit2-bit15） */
        gov_mt6816_st.raw_angle = gov_mt6816_st.raw_data >> 2;
        /* 提取无磁场标志（bit1） */
        gov_mt6816_st.magnet_valid = (bool)(gov_mt6816_st.raw_data & (0x0001 << 1));

        if (gov_calibra_st.calitable_flag == true) 
        {
            gov_mt6816_st.raw_angle = gov_cali_table[gov_mt6816_st.raw_angle];
        } else {
            gov_mt6816_st.micro_angle = gov_mt6816_st.raw_angle;
        }
    }

}
