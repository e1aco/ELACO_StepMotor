#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include <stdbool.h>

/* 读取数据（从Flash到内存） */
void EEPROM_Read(uint32_t addr, void* data, uint32_t size);

/* 写入数据（内存到Flash） */
void EEPROM_Write(uint32_t addr, void* data, uint32_t size);

/* 检查EEPROM是否有有效数据 */
bool EEPROM_IsValid(void);

#endif

