/*****************************************************************************
 * @文件: flash_drv.c
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v1.0
 * @说明: STM32F103RE 内部 Flash 存储原语实现（分区表 + 页擦除 + 半字写入）
 * @平台: STM32F103RET6 (512K, 页 2K=0x800)
 * @依赖: stm32f1xx_hal_flash_ex.h
 ****************************************************************************/
#include "flash_drv.h"
#include "stm32f1xx_hal.h"

/* ==== 常量定义 ==== */
/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
DRV_Flash_Area_T g_flash_quick_cali = {
    DRV_FLASH_CALI_ADDR,
    DRV_FLASH_CALI_SIZE,
    (DRV_FLASH_CALI_SIZE / DRV_FLASH_PAGE_SIZE),
    0U
};
DRV_Flash_Area_T g_flash_data = {
    DRV_FLASH_DATA_ADDR,
    DRV_FLASH_DATA_SIZE,
    (DRV_FLASH_DATA_SIZE / DRV_FLASH_PAGE_SIZE),
    0U
};

/* ==== 内部工具 ==== */
/* ==== 接口实现 ==== */
/**
 * @输入 area: Flash 分区表实例
 * @输出 无
 * @说明 擦除分区内全部页（逐页擦除）
 * 依据 .cl/memory/ flash_page_size=0x800 + stm32f1xx_hal_flash_ex.c 页擦除流程
 */
void DRV_Flash_AreaEmpty(DRV_Flash_Area_T *area)
{
    uint32_t count;
    HAL_FLASH_Unlock();

    for (count = 0U; count < area->page_num; count++)
    {
        FLASH_EraseInitTypeDef erase_config;
        uint32_t page_error = 0U;

        erase_config.TypeErase   = FLASH_TYPEERASE_PAGES;
        erase_config.PageAddress = area->begin_add + (count * DRV_FLASH_PAGE_SIZE);
        erase_config.NbPages     = 1U;

        HAL_FLASHEx_Erase(&erase_config, &page_error);
        FLASH_WaitForLastOperation(HAL_MAX_DELAY);
        CLEAR_BIT(FLASH->CR, FLASH_CR_PER);
    }

    HAL_FLASH_Lock();
}

/**
 * @输入 area: Flash 分区表实例
 * @输出 无
 * @说明 开始写入：解锁 Flash 并复位写地址到分区起始
 */
void DRV_Flash_AreaBegin(DRV_Flash_Area_T *area)
{
    HAL_FLASH_Unlock();
    area->asce_write_add = area->begin_add;
}

/**
 * @输入 area: Flash 分区表实例
 * @输出 无
 * @说明 结束写入：重新上锁 Flash
 */
void DRV_Flash_AreaEnd(DRV_Flash_Area_T *area)
{
    (void)area;
    HAL_FLASH_Lock();
}

/**
 * @输入 area: Flash 分区表实例; addr: 写目标绝对地址（须在分区范围内）
 * @输出 无
 * @说明 设置写地址（对齐参考 stockpile_f103cb.c Set_Write_Add 原语，
 *   越界地址直接忽略保持原值）
 */
void DRV_Flash_AreaSetAddr(DRV_Flash_Area_T *area, uint32_t addr)
{
    if (addr < area->begin_add)
    {
        return;
    }
    if (addr > (area->begin_add + area->area_size))
    {
        return;
    }
    area->asce_write_add = addr;
}

/**
 * @输入 area: Flash 分区表实例; data: 半字数据缓冲区; num: 半字数量
 * @输出 无
 * @说明 自 asce_write_add 起连续写入 num 个 16bit 半字，成功后写地址自增
 */
void DRV_Flash_AreaWrite16(DRV_Flash_Area_T *area, uint16_t *data, uint32_t num)
{
    uint32_t i;

    if (area->asce_write_add < area->begin_add)
    {
        return;
    }
    if ((area->asce_write_add + (num * 2U)) > (area->begin_add + area->area_size))
    {
        return;
    }

    for (i = 0U; i < num; i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, area->asce_write_add, (uint64_t)data[i]) == HAL_OK)
        {
            area->asce_write_add += 2U;
        }
    }
}
