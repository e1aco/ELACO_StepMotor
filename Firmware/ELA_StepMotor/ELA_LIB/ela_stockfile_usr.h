/********
 * @ 文件: ela_stockfile_usr.h
 * @ 作者: ELACO
 * @ 日期: 2026-07-27
 * @ 版本: 1.0.0
 * @ 说明: Flash 存储管理，分区定义与读写 API
 * @ 依赖: ela_stockfile_drv
 ********/

#ifndef ELA_STOCKFILE_USR_H
#define ELA_STOCKFILE_USR_H

#include <stdbool.h>
#include <stdint.h>
#include "ela_stockfile_drv.h"

/* Flash 分区地址映射 (STM32F103RET6, 512KB Flash) */
#define STOCKFILE_FIRMWARE_ADDR  0x08000000
#define STOCKFILE_FIRMWARE_SIZE  0x00020000

#define STOCKFILE_CALI_ADDR      0x08020000
#define STOCKFILE_CALI_SIZE      0x00008000

#define STOCKFILE_DATA_ADDR      0x08028000
#define STOCKFILE_DATA_SIZE      0x00001000

extern STOCKFILE_PART_T g_stockfile_cali_st;
extern STOCKFILE_PART_T g_stockfile_data_st;

void ela_stockfile_init(void);
void ela_stockfile_usr_erase(STOCKFILE_PART_T *part);
void ela_stockfile_usr_read(
    const STOCKFILE_PART_T *part,
    uint16_t *data, uint32_t count);
void ela_stockfile_usr_write(
    const STOCKFILE_PART_T *part,
    const uint16_t *data, uint32_t count);
bool ela_stockfile_usr_is_valid(
    const STOCKFILE_PART_T *part);
const STOCKFILE_PART_T *ela_stockfile_usr_get_part(
    uint32_t index);

void ela_stockfile_usr_seq_write_begin(
    const STOCKFILE_PART_T *part);
void ela_stockfile_usr_seq_write_next(
    const STOCKFILE_PART_T *part, uint16_t data);
void ela_stockfile_usr_seq_write_end(
    const STOCKFILE_PART_T *part);

#endif

