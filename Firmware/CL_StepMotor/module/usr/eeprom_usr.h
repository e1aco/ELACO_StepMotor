/*****************************************************************************
 * @文件: eeprom_usr.h
 * @作者: cl
 * @日期: 2026-08-17
 * @版本: v1.0
 * @说明: Flash 模拟 EEPROM 用户层（区内偏移读写/惰性擦除/有效判定，
 *   只调 DRV_Flash 原语，无业务语义）
 * @平台: STM32F103RET6
 * @依赖: flash_drv
 ****************************************************************************/
#ifndef EEPROM_USR_H
#define EEPROM_USR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "flash_drv.h"

/* ==== 常量定义 ==== */
/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
/* ==== 接口 ==== */
void    USR_EEPROM_Read(DRV_Flash_Area_T *area, uint32_t addr,
                        void *data, uint32_t size);
void    USR_EEPROM_Write(DRV_Flash_Area_T *area, uint32_t addr,
                         const void *data, uint32_t size);
uint8_t USR_EEPROM_IsValid(DRV_Flash_Area_T *area);
void    USR_EEPROM_Erase(DRV_Flash_Area_T *area);

#ifdef __cplusplus
}
#endif

#endif /* EEPROM_USR_H */
