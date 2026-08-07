/*****************************************************************************
 * @文件: ela_stockfile_drv.c
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: Flash 存储驱动层，页擦除与半字/字/双字编程原语
 ****************************************************************************/

#include "ela_stockfile_drv.h"

/********
 * @输入: address: 目标页起始地址
 * @说明: 擦除指定 Flash 页（2KB）
 * @注意: 调用前必须在 Flash 解锁状态
 ********/
static void Stockfile_ErasePage(uint32_t address)

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

/* ==== 全局实例 ==== */
/********
 * @输入: address: 编程地址; data: 16 位数据
 * @说明: 写入 16 位半字到指定 Flash 地址
 ********/
static void Stockfile_ProgramHalfword(uint32_t address,
                                           uint16_t data)
{
    if (HAL_OK != HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                                     address, data))
    {
        Error_Handler();
    }
}

/* ==== 接口实现 ==== */
/********
 * @输入: address: 目标地址
 * @输出: Flash 中存储的 16 位半字值
 * @说明: 直接从 Flash 地址读取 16 位半字
 ********/
uint16_t DRV_Stockfile_ReadHalfword(uint32_t address)
{
    return *((__IO uint16_t *)address);
}

/********
 * @输入: part: 分区配置指针
 * @说明: 擦除分区所有页，解锁 Flash 后调用擦除原语
 ********/
void DRV_Stockfile_ErasePartition(
    const STOCKFILE_PART_T *part)
{
    uint32_t addr;

    HAL_FLASH_Unlock();
    addr = part->start_addr;

    while (addr < part->end_addr)
    {
        Stockfile_ErasePage(addr);
        addr += part->page_size;
    }

    HAL_FLASH_Lock();
}

/********
 * @输入: part: 分区配置; data: 数据源; count: 半字数
 * @说明: 批量写入 16 位半字到指定分区
 * @注意: 调用前必须已擦除目标分区
 ********/
void DRV_Stockfile_WriteBulk(
    const STOCKFILE_PART_T *part,
    const uint16_t *data, uint32_t count)
{
    uint32_t i;
    uint32_t addr = part->start_addr;

    HAL_FLASH_Unlock();

    for (i = 0; i < count; i++)
    {
        Stockfile_ProgramHalfword(addr, data[i]);
        addr += 2;
    }

    HAL_FLASH_Lock();
}






