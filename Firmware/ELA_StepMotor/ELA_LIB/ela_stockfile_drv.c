/********
 * @ 文件: ela_stockfile_drv.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-27
 * @ 版本: 1.0.1
 * @ 说明: Flash 存储驱动层，页擦除与半字/字/双字编程原语
 ********/

#include "ela_stockfile_drv.h"

/* stockfile drv start */

/********
 * @ 输入: address: 目标页起始地址
 * @ 说明: 擦除指定 Flash 页（2KB）
 * @ 注意: 调用前必须在 Flash 解锁状态
 ********/
static void stockfile_drv_erase_page(uint32_t address)
{
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error;

    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.PageAddress = address;
    erase_init.NbPages = 1;

    if (HAL_OK != HAL_FLASHEx_Erase(&erase_init, &page_error))
    {
        Error_Handler();
    }
}

/********
 * @ 输入: address: 编程地址; data: 16 位数据
 * @ 说明: 写入 16 位半字到指定 Flash 地址
 ********/
static void stockfile_drv_program_halfword(uint32_t address,
                                           uint16_t data)
{
    if (HAL_OK != HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                                     address, data))
    {
        Error_Handler();
    }
}

/********
 * @ 输入: address: 编程地址; data: 32 位数据
 * @ 说明: 写入 32 位字到指定 Flash 地址
 ********/
static void stockfile_drv_program_word(uint32_t address,
                                       uint32_t data)
{
    if (HAL_OK != HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                                     address, data))
    {
        Error_Handler();
    }
}

/********
 * @ 输入: address: 编程地址; data: 64 位数据
 * @ 说明: 写入 64 位双字到指定 Flash 地址
 ********/
static void stockfile_drv_program_dword(uint32_t address,
                                        uint64_t data)
{
    if (HAL_OK != HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                                     address, data))
    {
        Error_Handler();
    }
}

/********
 * @ 输入: address: 目标地址
 * @ 输出: Flash 中存储的 16 位半字值
 * @ 说明: 直接从 Flash 地址读取 16 位半字
 ********/
uint16_t stockfile_drv_read_halfword(uint32_t address)
{
    return *((__IO uint16_t *)address);
}

/********
 * @ 输入: part: 分区配置指针
 * @ 说明: 擦除分区所有页，解锁 Flash 后调用擦除原语
 ********/
void ela_stockfile_drv_erase_partition(
    const STOCKFILE_PART_T *part)
{
    uint32_t addr;

    HAL_FLASH_Unlock();
    addr = part->start_addr;

    while (addr < part->end_addr)
    {
        stockfile_drv_erase_page(addr);
        addr += part->page_size;
    }

    HAL_FLASH_Lock();
}

/********
 * @ 输入: part: 分区配置; data: 数据源; count: 半字数
 * @ 说明: 批量写入 16 位半字到指定分区
 * @ 注意: 调用前必须已擦除目标分区
 ********/
void ela_stockfile_drv_write_bulk(
    const STOCKFILE_PART_T *part,
    const uint16_t *data, uint32_t count)
{
    uint32_t i;
    uint32_t addr = part->start_addr;

    HAL_FLASH_Unlock();

    for (i = 0; i < count; i++)
    {
        stockfile_drv_program_halfword(addr, data[i]);
        addr += 2;
    }

    HAL_FLASH_Lock();
}

/* stockfile drv end */

