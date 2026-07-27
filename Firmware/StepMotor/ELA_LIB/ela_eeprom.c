#include "ela_eeprom.h"


#define PAGE_MAX_SIZE 0x400U  // 103的1页是1KB(1024字节)
#define WRITE_DATA_ADDR  STOCKFILE_DATA_ADDR // 写入数据地址

static bool erased_flag = false; // 是否已经擦除过,false表示未擦除过

/****
 * @ 原型: void EEPROM_Read(unsigned int stockaddr, unsigned int startaddr, void* data, unsigned int size)
 * @ 输入: unsigned int stockaddr: 存储块起始地址; unsigned int startaddr: 起始读取地址; void* data: 读取数据存放地址; unsigned int size: 读取数据大小
 * @ 输出: void
 * @ 说明: 读取指定存储块中从startaddr开始的size字节数据到data指向的内存区域
 ********/
void EEPROM_Read(STOCKFILE_FLASH_T *stockfile, unsigned int addr, void* data, unsigned int size)
{
    if (addr + size > PAGE_MAX_SIZE) return;
    
    unsigned char* dst = (unsigned char*)data;
    unsigned int flash_addr = stockfile->begin_add + addr;
    
    for (unsigned int i = 0; i < size; i++) {
        dst[i] = *((unsigned char*)flash_addr + i);
    }
}

/****
 * @ 原型: void EEPROM_Write(unsigned int addr,STOCKFILE_FLASH_T *stockfile, void* data, unsigned int size)
 * @ 输入: unsigned int addr: 写入地址; STOCKFILE_FLASH_T *stockfile: 指向存储块结构体的指针; void* data: 写入数据存放地址; unsigned int size: 写入数据大小
 * @ 输出: void
 * @ 说明: 将data指向的size字节数据写入指定存储块中从addr开始的位置，写入前会检查是否已经擦除过，如果未擦除过则先擦除整个分区，写入完成后锁定 Flash
 ********/
void EEPROM_Write(unsigned int addr,STOCKFILE_FLASH_T *stockfile, void* data, unsigned int size)
{
    if (addr + size > PAGE_MAX_SIZE) return;
    
    /* 第一次写入时，先擦除整个分区 */
    if (!erased_flag) {
        Stockfile_FlashData_Empty(stockfile);
        erased_flag = true;
    }
    
    /* 开始写入 */
    Stockfile_FlashData_Begin(stockfile); //解锁并设置写地址为起始地址
    
    /* 设置写地址 */
    Stockfile_FlashData_Set_Write_Add(stockfile, stockfile->begin_add + addr);// 设置写地址为起始地址
    
    /* 写入数据（16位对齐） */
    unsigned int halfword_count = (size + 1) / 2; // 计算需要写入的半字数量，向上取整
    Stockfile_FlashData_Write_Data16(stockfile, (unsigned short*)data, halfword_count);
    
    /* 结束写入 */
    Stockfile_FlashData_End(stockfile); // 锁定 Flash，防止进一步写入
}

/****
 * @ 原型: bool EEPROM_Valid(STOCKFILE_FLASH_T *stockfile)
 * @ 输入: STOCKFILE_FLASH_T *stockfile: 指向要检查的存储块结构体的指针
 * @ 输出: bool
 * @ 说明: 检查指定存储块是否有效，即是否已经写入过数据，通过检查第一个字是否全为0xFF来判断
 ********/
bool EEPROM_Valid(STOCKFILE_FLASH_T *stockfile)
{
    unsigned int* pFirst = (unsigned int*)stockfile->begin_add; // 数据区起始地址
    
    /* 检查第一个字是否全0xFF（空Flash） */
    return (*pFirst != 0xFFFFFFFF);
}

/****
 * @ 原型: void EEPROM_Erase(STOCKFILE_FLASH_T *stockfile)
 * @ 输入: STOCKFILE_FLASH_T *stockfile: 指向要擦除的存储块结构体的指针
 * @ 输出: void
 * @ 说明: 擦除EEPROM中指定区域的数据，调用Stockfile_FlashData_Empty函数进行擦除，并设置擦除标志
 ********/
void EEPROM_Erase(STOCKFILE_FLASH_T *stockfile)
{
    Stockfile_FlashData_Empty(stockfile);
    erased_flag = true;
}
