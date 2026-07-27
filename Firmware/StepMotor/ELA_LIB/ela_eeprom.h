#ifndef _ELA_EEPROM_H_
#define _ELA_EEPROM_H_

#include "elaco_main.h"

void EEPROM_Read(STOCKFILE_FLASH_T *stockfile, unsigned int addr, void* data, unsigned int size);
void EEPROM_Write(unsigned int addr,STOCKFILE_FLASH_T *stockfile, void* data, unsigned int size);
bool EEPROM_Valid(STOCKFILE_FLASH_T *stockfile);
void EEPROM_Erase(STOCKFILE_FLASH_T *stockfile);

#endif
