#ifndef _ELA_MT6816_H_
#define _ELA_MT6816_H_

#include "elaco_main.h"// HAL库


/* -1- 角度获取结构体 */
typedef struct MT6816_ANGLE
{
    unsigned short raw_data; // 原始数据 16位，包含14位角度数据、1位无磁场标志和1位奇偶校验位
    unsigned short raw_angle; // 解析后的角度值 (0 ~ 16383)
    unsigned int micro_angle; // 角度值 (0 ~ 51200),每一个编码器值对应的微步的值。
    bool data_valid; // 数据有效标志，true表示数据正确，false表示数据错误（如校验失败）
    bool magnet_valid; // 磁铁有效标志，true表示磁铁存在，false表示磁铁不存在
    int direciton; // 方向标志，1表示正向，-1表示反向
}MT6816_ANGLE_T;

extern MT6816_ANGLE_T gov_mt6816_st; // 角度获取结构体

/* -2- 读取命令值 */
#define MT6816_CMD_ANGLE      0x03  /* 读取角度 */
#define MT6816_CMD_RAW_ANGLE  0x04  /* 读取原始角度 */

/* -3- SPI片选引脚电平转换 */
#define MT6816_CS_HIGH()    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET)
#define MT6816_CS_LOW()     HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET)

/* -4- 函数声明 */
void MT6816_Init(void);
unsigned short MT6816_ReadData(unsigned short tx_data);
static unsigned char MT6816_CalcParity(unsigned short data);
void MT6816_GetAngle(void);



#endif
