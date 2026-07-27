#include "mt6816.h"
#include "spi.h"
#include "main.h"

/* SPI CS引脚 */
#define MT6816_CS_GPIO_Port   SPI1_CS_GPIO_Port
#define MT6816_CS_Pin         SPI1_CS_Pin

/* 命令定义 */
#define MT6816_CMD_ANGLE      0x03  /* 读取角度 */
#define MT6816_CMD_RAW_ANGLE  0x04  /* 读取原始角度 */

/* SPI数据结构 */
typedef struct {
    uint16_t raw_data;      /* SPI原始16位数据 */
    uint16_t raw_angle;     /* 14位原始角度 */
    bool no_mag_flag;       /* 无磁场标志 */
    bool checksum_flag;     /* 校验标志 */
} MT6816_SpiData_t;

/* 静态变量 */
static uint16_t* s_quick_cali_data_ptr = NULL;  /* 校准数据指针 */
static MT6816_SpiData_t s_spi_raw_data;
static uint16_t s_data_tx[2];
static uint16_t s_data_rx[2];
static uint8_t s_hcount;
static uint16_t s_rectified_angle;  /* 校准后的角度 */
static uint16_t s_raw_angle;        /* 原始角度 */

/* SPI传输并读取16位数据 */
static uint16_t MT6816_SpiTransmitAndRead16Bits(uint16_t data_tx)
{
    uint16_t data_rx;
    
    /* CS低电平选中 */
    HAL_GPIO_WritePin(MT6816_CS_GPIO_Port, MT6816_CS_Pin, GPIO_PIN_RESET);
    
    /* SPI收发 */
    HAL_SPI_TransmitReceive(&hspi1, (uint8_t*)&data_tx, (uint8_t*)&data_rx, 1, HAL_MAX_DELAY);
    
    /* CS高电平释放 */
    HAL_GPIO_WritePin(MT6816_CS_GPIO_Port, MT6816_CS_Pin, GPIO_PIN_SET);
    
    return data_rx;
}

/* 奇偶校验计算 */
static uint8_t MT6816_CalcParity(uint16_t data)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < 16; i++) {
        if (data & (0x0001 << i))
            count++;
    }
    return count & 0x01;  /* 返回1表示奇数个1，0表示偶数个1 */
}

bool MT6816_Init(void)
{
    MT6816_UpdateAngle();
    
    /* 检查校准数据是否有效 */
    if (s_quick_cali_data_ptr == NULL) {
        return false;
    }
    
    for (uint32_t i = 0; i < MT6816_RESOLUTION; i++) {
        if (s_quick_cali_data_ptr[i] == 0xFFFF) {
            return false;
        }
    }
    
    return true;
}

uint16_t MT6816_UpdateAngle(void)
{
    /* 构造命令：最高位为1表示读 */
    s_data_tx[0] = (0x80 | MT6816_CMD_ANGLE) << 8;
    s_data_tx[1] = (0x80 | MT6816_CMD_RAW_ANGLE) << 8;
    
    /* 尝试3次读取，直到校验通过 */
    for (uint8_t i = 0; i < 3; i++) {
        s_data_rx[0] = MT6816_SpiTransmitAndRead16Bits(s_data_tx[0]);
        s_data_rx[1] = MT6816_SpiTransmitAndRead16Bits(s_data_tx[1]);
        
        /* 组合数据 */
        s_spi_raw_data.raw_data = ((s_data_rx[0] & 0x00FF) << 8) | (s_data_rx[1] & 0x00FF);
        
        /* 奇偶校验 */
        if (MT6816_CalcParity(s_spi_raw_data.raw_data) == 0) {
            /* 校验通过（偶数个1） */
            s_spi_raw_data.checksum_flag = true;
            break;
        } else {
            s_spi_raw_data.checksum_flag = false;
        }
    }
    
    if (s_spi_raw_data.checksum_flag) {
        /* 提取14位角度值（bit2-bit15） */
        s_spi_raw_data.raw_angle = s_spi_raw_data.raw_data >> 2;
        /* 提取无磁场标志（bit1） */
        s_spi_raw_data.no_mag_flag = (bool)(s_spi_raw_data.raw_data & (0x0001 << 1));
        
        s_raw_angle = s_spi_raw_data.raw_angle;
        
        /* 查表校准 */
        if (s_quick_cali_data_ptr != NULL) {
            s_rectified_angle = s_quick_cali_data_ptr[s_raw_angle];
        } else {
            s_rectified_angle = s_raw_angle;
        }
    }
    
    return s_rectified_angle;
}

uint16_t MT6816_GetRawAngle(void)
{
    return s_raw_angle;
}

uint16_t MT6816_GetRectifiedAngle(void)
{
    return s_rectified_angle;
}

bool MT6816_IsCalibrated(void)
{
    if (s_quick_cali_data_ptr == NULL) {
        return false;
    }
    
    for (uint32_t i = 0; i < MT6816_RESOLUTION; i++) {
        if (s_quick_cali_data_ptr[i] == 0xFFFF) {
            return false;
        }
    }
    return true;
}

void MT6816_SetCalibrationData(uint16_t* cali_data_ptr)
{
    s_quick_cali_data_ptr = cali_data_ptr;
}

uint16_t MT6816_TestRead(void)
{
    uint16_t data_tx = (0x80 | MT6816_CMD_ANGLE) << 8;
    uint16_t data_rx;
    
    /* CS低电平选中 */
    HAL_GPIO_WritePin(MT6816_CS_GPIO_Port, MT6816_CS_Pin, GPIO_PIN_RESET);
    
    /* SPI收发 */
    HAL_SPI_TransmitReceive(&hspi1, (uint8_t*)&data_tx, (uint8_t*)&data_rx, 1, HAL_MAX_DELAY);
    
    /* CS高电平释放 */
    HAL_GPIO_WritePin(MT6816_CS_GPIO_Port, MT6816_CS_Pin, GPIO_PIN_SET);
    
    return data_rx;
}