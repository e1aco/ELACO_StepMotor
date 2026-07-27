#ifndef _ELA_UART_H_
#define _ELA_UART_H_

#include "elaco_main.h"


#define DMA_BUF_SIZE 256 // DMA 缓冲区大小，根据实际需求调整

extern unsigned char gov_rx1_offset;      // 串口接收偏移量，用于计算接收数据长度
extern unsigned char gov_rx1_buf[];        // 串口接收缓冲区


#endif
