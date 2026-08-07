/*****************************************************************************
 * @文件: ela_stockfile_drv.h
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: Flash 存储硬件驱动层，页擦除与编程原语
 ****************************************************************************/

#ifndef ELA_STOCKFILE_DRV_H
#define ELA_STOCKFILE_DRV_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

/* ==== 常量定义 ==== */
#define STOCKFILE_PAGE_SIZE  0x800U

/* ==== 类型定义 ==== */
typedef struct {
    uint32_t start_addr;
    uint32_t end_addr;
    uint32_t page_size;
    uint32_t reserved1;
    uint32_t reserved2;
} STOCKFILE_PART_T;

/* ==== 接口 ==== */



uint16_t DRV_Stockfile_ReadHalfword(uint32_t address);
void DRV_Stockfile_ErasePartition(
    const STOCKFILE_PART_T *part);
void DRV_Stockfile_WriteBulk(
    const STOCKFILE_PART_T *part,
    const uint16_t *data, uint32_t count);

#endif






