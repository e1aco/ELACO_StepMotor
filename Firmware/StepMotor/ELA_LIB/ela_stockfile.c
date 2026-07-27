#include "ela_stockfile.h"

#define Stockfile_Page_Size		0x400U		//扇区大小(默认1024字节)

/* 存储块结构体 */
STOCKFILE_FLASH_T gov_stockfile_cail_st={STOCKFILE_CALI_ADDR, STOCKFILE_CALI_SIZE,(STOCKFILE_CALI_SIZE / Stockfile_Page_Size),0}; // 校准值 32K
STOCKFILE_FLASH_T gov_stockfile_data_st={STOCKFILE_DATA_ADDR, STOCKFILE_DATA_SIZE,(STOCKFILE_DATA_SIZE / Stockfile_Page_Size),0}; // 数据 1K


/****
 * @ 原型: void Stockfile_FlashData_Empty(STOCKFILE_FLASH_T *stockfile)
 * @ 输入: STOCKFILE_FLASH_T *stockfile: 指向要擦除的存储块结构体的指针
 * @ 输出: void
 * @ 说明: 擦除指定存储块中的所有页，使用 HAL 库函数进行 Flash 擦除操作，擦除完成后锁定 Flash
 ********/
void Stockfile_FlashData_Empty(STOCKFILE_FLASH_T *stockfile)
{
	unsigned int count;
	HAL_FLASH_Unlock();	
	for(count = 0; count < stockfile->page_num; count++)
	{
		FLASH_EraseInitTypeDef erase_config;
		unsigned int page_error;
		erase_config.TypeErase   = FLASH_TYPEERASE_PAGES;									//页擦除		
		erase_config.PageAddress = stockfile->begin_add + (count * Stockfile_Page_Size);	//页起始地址
		erase_config.NbPages     = 1;																											//擦除页数量
		HAL_FLASHEx_Erase(&erase_config, &page_error);
		FLASH_WaitForLastOperation(HAL_MAX_DELAY);
		CLEAR_BIT(FLASH->CR, FLASH_CR_PER);
	}
	HAL_FLASH_Lock();	
}

/****
 * @ 原型: void Stockfile_FlashData_Begin(STOCKFILE_FLASH_T *stockfile)
 * @ 输入: STOCKFILE_FLASH_T *stockfile: 指向要擦除的存储块结构体的指针
 * @ 输出: void
 * @ 说明: 开始写入数据，将结构体的写地址初始化为存储块的起始地址，并解锁 Flash 以允许写入操作
 ********/
void Stockfile_FlashData_Begin(STOCKFILE_FLASH_T *stockfile)
{
	HAL_FLASH_Unlock();	
	stockfile->asce_write_add = stockfile->begin_add;
}

/****
 * @ 原型: void Stockfile_FlashData_End(STOCKFILE_FLASH_T *stockfile)
 * @ 输入: STOCKFILE_FLASH_T *stockfile: 指向要擦除的存储块结构体的指针
 * @ 输出: void
 * @ 说明: 结束写入数据，锁定 Flash 以防止进一步的写入操作
 ********/
void Stockfile_FlashData_End(STOCKFILE_FLASH_T *stockfile)
{
	HAL_FLASH_Lock();	
}

/****
 * @ 原型: void Stockfile_FlashData_Set_Write_Add(STOCKFILE_FLASH_T *stockfile, uint32_t write_add)
 * @ 输入: STOCKFILE_FLASH_T *stockfile: 指向要擦除的存储块结构体的指针; uint32_t write_add: 要设置的写入地址
 * @ 输出: void
 * @ 说明: 设置写入地址，检查写入地址是否在存储块的有效范围内，如果有效则更新结构体中的写地址
 ********/
void Stockfile_FlashData_Set_Write_Add(STOCKFILE_FLASH_T *stockfile, uint32_t write_add)
{
	if(write_add < stockfile->begin_add)						return;
	if(write_add > stockfile->begin_add + stockfile->area_size)	return;
	stockfile->asce_write_add = write_add;
}

/****
 * @ 原型: void Stockfile_FlashData_Write_Data16(STOCKFILE_FLASH_T *stockfile, unsigned short *data, unsigned int num)
 * @ 输入: STOCKFILE_FLASH_T *stockfile: 指向要擦除的存储块结构体的指针; unsigned short *data: 要写入的数据指针; unsigned int num: 要写入的数据数量
 * @ 输出: void
 * @ 说明: 写入16位数据，检查写入地址是否在存储块的有效范围内，如果有效则更新结构体中的写地址
 ********/
void Stockfile_FlashData_Write_Data16(STOCKFILE_FLASH_T *stockfile, unsigned short *data, unsigned int num)
{
	if(stockfile->asce_write_add < stockfile->begin_add)									return;
	if((stockfile->asce_write_add + num * 2) > stockfile->begin_add + stockfile->area_size)	return;
	
	for(unsigned int i=0; i<num; i++)
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, stockfile->asce_write_add, (uint64_t)data[i]) == HAL_OK)
			stockfile->asce_write_add += 2;
	}
}

/****
 * @ 原型: void Stockfile_FlashData_Write_Data32(STOCKFILE_FLASH_T *stockfile, unsigned int *data, unsigned int num)
 * @ 输入: STOCKFILE_FLASH_T *stockfile: 指向要擦除的存储块结构体的指针; unsigned int *data: 要写入的数据指针; unsigned int num: 要写入的数据数量
 * @ 输出: void
 * @ 说明: 写入32位数据，检查写入地址是否在存储块的有效范围内，如果有效则更新结构体中的写地址
 ********/
void Stockfile_FlashData_Write_Data32(STOCKFILE_FLASH_T *stockfile, unsigned int *data, unsigned int num)
{
	if(stockfile->asce_write_add < stockfile->begin_add)									return;
	if((stockfile->asce_write_add + num * 4) > stockfile->begin_add + stockfile->area_size)	return;
	
	for(unsigned int i=0; i<num; i++)
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, stockfile->asce_write_add, (unsigned long long)data[i]) == HAL_OK)
			stockfile->asce_write_add += 4;
	}
}

/****
 * @ 原型: void Stockfile_FlashData_Write_Data64(STOCKFILE_FLASH_T *stockfile, unsigned long long *data, unsigned int num)
 * @ 输入: STOCKFILE_FLASH_T *stockfile: 指向要擦除的存储块结构体的指针; unsigned long long *data: 要写入的数据指针; unsigned int num: 要写入的数据数量
 * @ 输出: void
 * @ 说明: 写入64位数据，检查写入地址是否在存储块的有效范围内，如果有效则更新结构体中的写地址
 ********/
void Stockfile_FlashData_Write_Data64(STOCKFILE_FLASH_T *stockfile, unsigned long long *data, unsigned int num)
{
	if(stockfile->asce_write_add < stockfile->begin_add)									return;
	if((stockfile->asce_write_add + num * 8) > stockfile->begin_add + stockfile->area_size)	return;
	
	for(unsigned int i=0; i<num; i++)
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, stockfile->asce_write_add, (unsigned long long)data[i]) == HAL_OK)
			stockfile->asce_write_add += 8;
	}
}
