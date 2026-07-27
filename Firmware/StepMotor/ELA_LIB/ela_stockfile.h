#ifndef _ELA_STOCKFILE_H_
#define _ELA_STOCKFILE_H_

#include "elaco_main.h" 
/*********************STM32F103xx*************************/
//主储存块容量
//Flash Size(bytes)/RAM size(bytes)
// 大容量   1M / 96K                                     RG               VG           ZG
// 大容量 768K / 96K                                     RF               VF           ZF
// 大容量 512K / 64K                                     RE               VE           ZE
// 大容量 384K / 64K                                     RD               VD           ZD
// 大容量 256K / 48K                                     RC               VC           ZC
// 中容量 128K / 20K      TB           CB                RB               VB
// 中容量  64K / 20K      T8           C8                R8               V8
// 小容量  32K / 10K      T6           C6                R6
// 小容量  16K /  6K      T4           C4                R4

/* -1- 存放变量 */
typedef struct{
	//配置
	unsigned int	begin_add;			//起始地址
	unsigned int	area_size;			//区域大小
	unsigned int	page_num;			//芯片实体页数量
	//过程量
	unsigned int	asce_write_add;	//写地址
}STOCKFILE_FLASH_T;

//extern StockFile_FLASH_T stockpile_app_firmware; // Keil会帮我们搞好
extern STOCKFILE_FLASH_T gov_stockfile_cail_st; // 校准值 32K
extern STOCKFILE_FLASH_T gov_stockfile_data_st; // 数据 1K

/* -2- 存放地址位置宏定义 */
// 存放程序
#define STOCKFILE_FIRMWARE_ADDR (0x08000000) // 起始地址
#define STOCKFILE_FIRMWARE_SIZE (0x00017C00) // Flash容量    95K 这里的容量在Keil里面修改
// 存放校准值
#define STOCKFILE_CALI_ADDR (0x08017C00) // 起始地址
#define STOCKFILE_CALI_SIZE (0x00008000) // Flash容量    32K
// 存放数据
#define STOCKFILE_DATA_ADDR (0x0801FC00) // 起始地址
#define STOCKFILE_DATA_SIZE (0x00000400) // Flash容量     1K

/* -3- 函数声明 */
void Stockfile_FlashData_Empty(STOCKFILE_FLASH_T *stockfile);
void Stockfile_FlashData_Begin(STOCKFILE_FLASH_T *stockfile);
void Stockfile_FlashData_End(STOCKFILE_FLASH_T *stockfile);
void Stockfile_FlashData_Set_Write_Add(STOCKFILE_FLASH_T *stockfile, uint32_t write_add);
void Stockfile_FlashData_Write_Data16(STOCKFILE_FLASH_T *stockfile, unsigned short *data, unsigned int num);
void Stockfile_FlashData_Write_Data32(STOCKFILE_FLASH_T *stockfile, unsigned int *data, unsigned int num);
void Stockfile_FlashData_Write_Data64(STOCKFILE_FLASH_T *stockfile, unsigned long long *data, unsigned int num);


#endif
