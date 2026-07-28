/********
 * @ 文件: ela_stockfile_usr.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-27
 * @ 版本: 1.0.1
 * @ 说明: Flash 存储管理，分区初始化与读写接口
 * @ 依赖: ela_stockfile_drv
 ********/

#include "ela_stockfile_usr.h"
#include <string.h>

STOCKFILE_PART_T g_stockfile_cali_st;
STOCKFILE_PART_T g_stockfile_data_st;

static uint32_t s_seq_write_addr;

/* stockfile usr start */

/********
 * @ 说明: STOCKFILE 分区表配置
 * @ 注意: 按地址升序排列，分区不可重叠
 ********/
static const STOCKFILE_PART_T s_partitions[] = {
    {STOCKFILE_CALI_ADDR,
     STOCKFILE_CALI_ADDR + STOCKFILE_CALI_SIZE,
     STOCKFILE_PAGE_SIZE, 0, 0},
    {STOCKFILE_DATA_ADDR,
     STOCKFILE_DATA_ADDR + STOCKFILE_DATA_SIZE,
     STOCKFILE_PAGE_SIZE, 0, 0},
};

/********
 * @ 说明: 初始化分区全局变量
 ********/
void ela_stockfile_init(void)
{
    g_stockfile_cali_st = s_partitions[0];
    g_stockfile_data_st = s_partitions[1];
}

/********
 * @ 输入: part: 指向要擦除的分区结构体
 * @ 说明: 擦除指定分区的所有页
 ********/
void ela_stockfile_usr_erase(
    STOCKFILE_PART_T *part)
{
    ela_stockfile_drv_erase_partition(part);
}

/********
 * @ 输入: part: 分区指针; data: 数据源; count: 半字数
 * @ 说明: 读取分区数据到缓冲区
 ********/
void ela_stockfile_usr_read(
    const STOCKFILE_PART_T *part,
    uint16_t *data, uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; i++)
    {
        data[i] = stockfile_drv_read_halfword(
                      part->start_addr + i * 2);
    }
}

/********
 * @ 输入: part: 分区指针; data: 数据源; count: 半字数
 * @ 说明: 写入数据到分区（擦除由调用方负责）
 ********/
void ela_stockfile_usr_write(
    const STOCKFILE_PART_T *part,
    const uint16_t *data, uint32_t count)
{
    ela_stockfile_drv_write_bulk(part, data, count);
}

/********
 * @ 输入: part: 分区指针
 * @ 输出: 分区数量
 * @ 说明: 获取分区表大小
 ********/
static uint32_t ela_stockfile_usr_get_part_count(void)
{
    return sizeof(s_partitions) / sizeof(s_partitions[0]);
}

/********
 * @ 输入: index: 分区索引
 * @ 输出: 分区结构体指针，越界返回 NULL
 * @ 说明: 获取指定索引的分区配置
 ********/
const STOCKFILE_PART_T *ela_stockfile_usr_get_part(
    uint32_t index)
{
    if (index >= ela_stockfile_usr_get_part_count())
    {
        return (const STOCKFILE_PART_T *)0;
    }

    return &s_partitions[index];
}

/********
 * @ 输入: part: 分区指针
 * @ 输出: true 表示分区已写入有效数据，false 表示空分区
 * @ 说明: 检查分区第一个字是否为 0xFFFF（空 Flash）
 ********/
bool ela_stockfile_usr_is_valid(
    const STOCKFILE_PART_T *part)
{
    uint16_t val = stockfile_drv_read_halfword(
                       part->start_addr);
    return (0xFFFF != val);
}

/********
 * @ 输入: part: 分区指针
 * @ 说明: 开始顺序写入，解锁 Flash 并设置写地址为分区起始
 ********/
void ela_stockfile_usr_seq_write_begin(
    const STOCKFILE_PART_T *part)
{
    s_seq_write_addr = part->start_addr;
    HAL_FLASH_Unlock();
}

/********
 * @ 输入: part: 分区指针; data: 待写入的 16 位数据
 * @ 说明: 在当前写地址写入一个半字，地址自动前进 2 字节
 ********/
void ela_stockfile_usr_seq_write_next(
    const STOCKFILE_PART_T *part, uint16_t data)
{
    (void)part;
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                      s_seq_write_addr, data);
    s_seq_write_addr += 2;
}

/********
 * @ 输入: part: 分区指针
 * @ 说明: 结束顺序写入，锁定 Flash
 ********/
void ela_stockfile_usr_seq_write_end(
    const STOCKFILE_PART_T *part)
{
    (void)part;
    HAL_FLASH_Lock();
}

/* stockfile usr end */

