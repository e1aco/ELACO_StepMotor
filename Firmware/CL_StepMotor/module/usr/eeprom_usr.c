/*****************************************************************************
 * @文件: eeprom_usr.c
 * @作者: cl
 * @日期: 2026-08-17
 * @版本: v1.0
 * @说明: Flash 模拟 EEPROM 用户层实现（复刻参考 eeprom.c：直接内存读、
 *   首次写前整区擦除、16bit 半字写入；参考全局惰性擦除标志改进为按区记录）
 * @平台: STM32F103RET6
 * @依赖: flash_drv
 ****************************************************************************/
#include "eeprom_usr.h"
#include <string.h>

/* ==== 常量定义 ==== */
/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
static uint32_t s_erased_area_begin = 0U;  /* 已擦除分区起始地址（按区惰性擦除标志） */

/* ==== 内部工具 ==== */
/* ==== 接口实现 ==== */
/**
 * @输入 area: Flash 分区表实例; addr: 区内偏移; data: 读出缓冲; size: 字节数
 * @输出 无
 * @说明 自 (area->begin_add + addr) 直接内存读取（Flash 读等效普通内存，
 *   对齐参考 eeprom.c EEPROM_Read 逐字节拷贝）
 */
void USR_EEPROM_Read(DRV_Flash_Area_T *area, uint32_t addr,
                     void *data, uint32_t size)
{
    uint8_t *dst    = (uint8_t *)data;
    uint32_t flash_addr;
    uint32_t i;

    if ((addr + size) > area->area_size)
    {
        return;
    }

    flash_addr = area->begin_add + addr;
    for (i = 0U; i < size; i++)
    {
        dst[i] = *((uint8_t *)flash_addr + i);
    }
}

/**
 * @输入 area: Flash 分区表实例; addr: 区内偏移; data: 写入源; size: 字节数
 * @输出 无
 * @说明 首次写某分区前先整区擦除（Flash 只能 1→0，写前必擦），随后
 *   16bit 半字写入。参考 eeprom.c 全局单标志 s_is_erased 两分区互坑
 *   （先擦 cali 再擦 data 时第二区不擦）→ 按区记录擦除标志（变更点）
 * 依据 .cl/memory/ flash_page_size=0x800 + stm32f1xx_hal_flash_ex.c 半字编程
 */
void USR_EEPROM_Write(DRV_Flash_Area_T *area, uint32_t addr,
                      const void *data, uint32_t size)
{
    uint32_t halfword_count;

    if ((addr + size) > area->area_size)
    {
        return;
    }

    /* 惰性擦除：该分区首次写入前整区擦除 */
    if (s_erased_area_begin != area->begin_add)
    {
        DRV_Flash_AreaEmpty(area);
        s_erased_area_begin = area->begin_add;
    }

    DRV_Flash_AreaBegin(area);
    DRV_Flash_AreaSetAddr(area, area->begin_add + addr);

    /* 16bit 半字写入（F103 编程粒度，末字节不足半字按半字补） */
    halfword_count = (size + 1U) / 2U;
    DRV_Flash_AreaWrite16(area, (uint16_t *)data, halfword_count);
    DRV_Flash_AreaEnd(area);
}

/**
 * @输入 area: Flash 分区表实例
 * @输出 1=区首 32bit 非 0xFFFFFFFF（已写入数据），0=擦除态
 * @说明 有效判定：擦除态全 0xFF → 无数据；首字非 0xFF → 有写入痕迹。
 *   具体配置合法性（magic/configStatus）由 config_usr 校验
 * 依据 .cl/memory/ stm32_flash.md 节点「配置丢失/magic 校验」故障视角
 */
uint8_t USR_EEPROM_IsValid(DRV_Flash_Area_T *area)
{
    uint32_t *p_first = (uint32_t *)area->begin_add;

    return (*p_first != 0xFFFFFFFFU) ? 1U : 0U;
}

/**
 * @输入 area: Flash 分区表实例
 * @输出 无
 * @说明 显式整区擦除并复位惰性擦除标志（对齐参考 eeprom.c EEPROM_Erase）
 */
void USR_EEPROM_Erase(DRV_Flash_Area_T *area)
{
    DRV_Flash_AreaEmpty(area);
    s_erased_area_begin = area->begin_add;
}
