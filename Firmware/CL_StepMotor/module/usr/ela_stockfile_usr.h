/*****************************************************************************
 * @文件: ela_stockfile_usr.h
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: Flash 存储管理，分区定义与读写 API
 ****************************************************************************/

#ifndef ELA_STOCKFILE_USR_H
#define ELA_STOCKFILE_USR_H

#include <stdbool.h>
#include <stdint.h>
#include "ela_stockfile_drv.h"

/* ==== 常量定义 ==== */
/* Flash 分区地址映射 (STM32F103RET6, 512KB Flash) */
#define STOCKFILE_FIRMWARE_ADDR  0x08000000
#define STOCKFILE_FIRMWARE_SIZE  0x00020000

#define STOCKFILE_CALI_ADDR      0x08020000

#define STOCKFILE_CALI_SIZE      0x00008000

#define STOCKFILE_DATA_ADDR      0x08028000
#define STOCKFILE_DATA_SIZE      0x00001000

/* ==== 全局实例 ==== */
extern STOCKFILE_PART_T g_stockfile_cali_st;
extern STOCKFILE_PART_T g_stockfile_data_st;

/* ==== 接口 ==== */



void USR_Stockfile_Init(void);
void USR_Stockfile_Erase(STOCKFILE_PART_T *part);
void USR_Stockfile_Read(
    const STOCKFILE_PART_T *part,
    uint16_t *data, uint32_t count);
void USR_Stockfile_Write(
    const STOCKFILE_PART_T *part,
    const uint16_t *data, uint32_t count);
bool USR_Stockfile_IsValid(
    const STOCKFILE_PART_T *part);
const STOCKFILE_PART_T *USR_Stockfile_GetPart(
    uint32_t index);

void USR_Stockfile_SeqWriteBegin(
    const STOCKFILE_PART_T *part);
void USR_Stockfile_SeqWriteNext(
    const STOCKFILE_PART_T *part, uint16_t data);
void USR_Stockfile_SeqWriteEnd(
    const STOCKFILE_PART_T *part);

#endif






