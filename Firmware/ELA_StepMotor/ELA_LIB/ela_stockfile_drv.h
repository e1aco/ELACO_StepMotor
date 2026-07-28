/********
 * @ 文件: ela_stockfile_drv.h
 * @ 作者: ELACO
 * @ 日期: 2026-07-27
 * @ 版本: 1.0.0
 * @ 说明: Flash 存储硬件驱动层，页擦除与编程原语
 * @ 注意: STM32F103RET6 高密度，页大小 2KB (0x800)
 ********/

#ifndef ELA_STOCKFILE_DRV_H
#define ELA_STOCKFILE_DRV_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

#define STOCKFILE_PAGE_SIZE  0x800U

typedef struct {
    uint32_t start_addr;
    uint32_t end_addr;
    uint32_t page_size;
    uint32_t reserved1;
    uint32_t reserved2;
} STOCKFILE_PART_T;

uint16_t stockfile_drv_read_halfword(uint32_t address);
void ela_stockfile_drv_erase_partition(
    const STOCKFILE_PART_T *part);
void ela_stockfile_drv_write_bulk(
    const STOCKFILE_PART_T *part,
    const uint16_t *data, uint32_t count);

#endif

