/********
 * @ 文件: port.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-17
 * @ 版本: 1.0.0
 * @ 说明: FreeModbus 寄存器回调函数（线圈/保持/输入/离散）
 ********/

#include "mb.h"
#include "mbport.h"

// 10路线圈可读可写的位控制
#define REG_COILS_SIZE 10
uint8_t REG_COILS_BUF[REG_COILS_SIZE] = {1, 1, 1, 1, 0, 0, 0, 0, 1, 1};
// 低位在前，高位在后
// 1 1 1 1 0 0 0 0 | 1 1
// 0 1 2 3 4 5 6 7 | 8 9
// 0f              | 03
// 对应的16进制数为： 0x0F 0x03

// 10路离散量输入寄存器
#define REG_DISC_SIZE 10
uint8_t REG_DISC_BUF[REG_DISC_SIZE] = {1, 1, 1, 1, 0, 0, 0, 0, 1, 1};
// 只读的线圈

// 10路保持寄存器 可读可写的2字节寄存器,相当于20个字节
#define REG_HOLD_SIZE 10
uint16_t REG_HOLD_BUF[REG_HOLD_SIZE];

// 10路输入寄存器 只读的2字节寄存器
#define REG_INPUT_SIZE 10
uint16_t REG_INPUT_BUF[REG_INPUT_SIZE];
// 只读保持寄存器

eMBErrorCode eMBRegInputCB(UCHAR *pucRegBuffer,
                           USHORT usAddress,
                           USHORT usNRegs)
{
    USHORT usRegIndex = usAddress - 1; // 寄存器地址从1开始，数组索引从0开始
    if ((usRegIndex + usNRegs) > REG_INPUT_SIZE)
    {
        return MB_ENOREG; // 超出寄存器范围
    }
    while (usNRegs > 0)
    {
        // 高字节
        *pucRegBuffer++ = (unsigned char)(REG_INPUT_BUF[usRegIndex] >> 8);
        // 低字节
        *pucRegBuffer++ = (unsigned char)(REG_INPUT_BUF[usRegIndex] & 0xFF);
        usRegIndex++;
        usNRegs--;
    }
    for (usRegIndex = 0; usRegIndex < REG_INPUT_SIZE; usRegIndex++)
    {
        REG_INPUT_BUF[usRegIndex]++; // 模拟输入寄存器的变化
    }
    return MB_ENOERR;
}

// CMD6、3、16命令处理回调函数，这个是我们主要使用的回调函数
eMBErrorCode eMBRegHoldingCB(UCHAR *pucRegBuffer,
                             USHORT usAddress,
                             USHORT usNRegs,
                             eMBRegisterMode eMode)
{
    USHORT usRegIndex = usAddress - 1; // 寄存器地址从1开始，数组索引从0开始

    // 非法检测
    if ((usRegIndex + usNRegs) > REG_HOLD_SIZE)
    {
        return MB_ENOREG; // 超出寄存器范围
    }
    // 读取保持寄存器的值
    if (eMode == MB_REG_WRITE)
    {
        while (usNRegs > 0)
        {
            // 高字节在前，低字节在后
            REG_HOLD_BUF[usRegIndex] = (pucRegBuffer[0] << 8) | pucRegBuffer[1];
            pucRegBuffer += 2;
            usRegIndex++;
            usNRegs--;
        }
    }
    // 读取保持寄存器的值
    else
    {
        while (usNRegs > 0)
        {
            // 高字节
            *pucRegBuffer++ = (unsigned char)(REG_HOLD_BUF[usRegIndex] >> 8);
            // 低字节
            *pucRegBuffer++ = (unsigned char)(REG_HOLD_BUF[usRegIndex] & 0xFF);
            usRegIndex++;
            usNRegs--;
        }
    }
    return MB_ENOERR;
}

// CMD1、5、15命令处理回调函数，读取线圈寄存器
eMBErrorCode eMBRegCoilsCB(UCHAR *pucRegBuffer,
                           USHORT usAddress,
                           USHORT usNCoils,
                           eMBRegisterMode eMode)
{
    USHORT usRegIndex = usAddress - 1;
    UCHAR ucBits = 0;
    UCHAR ucState = 0;
    UCHAR ucLoops = 0;
    // 非法检测
    if ((usRegIndex + usNCoils) > REG_COILS_SIZE)
    {
        return MB_ENOREG; // 超出寄存器范围
    }

    if (eMode == MB_REG_WRITE)
    {
        ucLoops = (usNCoils - 1) / 8 + 1; // 计算需要处理的字节数
        while (ucLoops != 0)
        {
            ucState = *pucRegBuffer++; // 获取当前字节的值
            ucBits = 0;
            while (usNCoils != 0 && ucBits < 8)
            {
                // 更新线圈状态
                REG_COILS_BUF[usRegIndex++] = (ucState >> ucBits) & 0x01;
                usNCoils--;
                ucBits++;
            }
            ucLoops--;
        }
    }
    else
    {
        ucLoops = (usNCoils - 1) / 8 + 1; // 计算需要处理的字节数
        while (ucLoops != 0)
        {
            ucState = 0;
            ucBits = 0;
            while (usNCoils != 0 && ucBits < 8)
            {
                if (REG_COILS_BUF[usRegIndex])
                {
                    ucState |= (1 << ucBits); // 设置对应位为1
                }
                usNCoils--;
                usRegIndex++;
                ucBits++;
            }
            *pucRegBuffer++ = ucState; // 写入当前字节的值
            ucLoops--;
        }
    }
    return MB_ENOERR;
}

// CMD2命令处理回调函数，读取离散量输入寄存器
eMBErrorCode eMBRegDiscreteCB(UCHAR *pucRegBuffer,
                              USHORT usAddress,
                              USHORT usNDiscrete)
{
    USHORT usRegIndex = usAddress - 1;
    UCHAR ucBits = 0;
    UCHAR ucState = 0;
    UCHAR ucLoops = 0;
    // 非法检测
    if ((usRegIndex + usNDiscrete) > REG_DISC_SIZE)
    {
        return MB_ENOREG; // 超出寄存器范围
    }
    ucLoops = (usNDiscrete - 1) / 8 + 1; // 计算需要处理的字节数
    while (ucLoops != 0)
    {
        ucState = 0;
        ucBits = 0;
        while (usNDiscrete != 0 && ucBits < 8)
        {
            if (REG_DISC_BUF[usRegIndex])
            {
                ucState |= (1 << ucBits); // 设置对应位为1
            }
            usNDiscrete--;
            usRegIndex++;
            ucBits++;
        }
        *pucRegBuffer++ = ucState; // 写入当前字节的值
        ucLoops--;
    }
    for (usRegIndex = 0; usRegIndex < REG_DISC_SIZE; usRegIndex++)
    {
        REG_DISC_BUF[usRegIndex] = !REG_DISC_BUF[usRegIndex]; // 模拟离散量输入寄存器的变化
    }
    return MB_ENOERR;
}
