/*****************************************************************************
 * @文件: flash_drv.h
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v1.0
 * @说明: STM32F103RE 内部 Flash 存储原语（页擦除/半字写，无业务逻辑）
 * @平台: STM32F103RET6 (512K, 页 2K=0x800)
 * @依赖: stm32f1xx_hal_flash_ex.h
 ****************************************************************************/
#ifndef FLASH_DRV_H
#define FLASH_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ==== 常量定义 ==== */
#define DRV_FLASH_PAGE_SIZE      0x800U    /* F103RE 大容量 512K 页大小 2K（依据 stm32f1xx_hal_flash_ex.h:246） */
#define DRV_FLASH_CALI_ADDR      0x08077800U   /* 校准表区起始（依据 .cl/memory/ Flash 分区推导，512K 尾部按 2K 页对齐） */
#define DRV_FLASH_CALI_SIZE      0x8000U   /* 校准表区 32K = 16 页（容纳 16384×uint16 校准表） */
#define DRV_FLASH_DATA_ADDR      0x0807F800U   /* 配置数据区起始（CALI 顶上一页，2K 页对齐） */
#define DRV_FLASH_DATA_SIZE      0x0800U   /* 配置数据区 2K = 1 页 */
typedef struct
{
    uint32_t begin_add;        /* 起始地址 */
    uint32_t area_size;        /* 区域大小 */
    uint32_t page_num;         /* 页数量 */
    uint32_t asce_write_add;   /* 写地址（过程量） */
} DRV_Flash_Area_T;

/* ==== 全局实例 ==== */
extern DRV_Flash_Area_T g_flash_quick_cali;   /* 校准表区 0x08077800 起 32K(16页) */
extern DRV_Flash_Area_T g_flash_data;         /* 配置数据区 0x0807F800 起 2K(1页) */

/* ==== 接口 ==== */
void DRV_Flash_AreaEmpty(DRV_Flash_Area_T *area);
void DRV_Flash_AreaBegin(DRV_Flash_Area_T *area);
void DRV_Flash_AreaEnd(DRV_Flash_Area_T *area);
void DRV_Flash_AreaWrite16(DRV_Flash_Area_T *area, uint16_t *data, uint32_t num);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_DRV_H */
