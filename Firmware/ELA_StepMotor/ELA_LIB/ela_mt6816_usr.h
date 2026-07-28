/********
 * @ 文件: ela_mt6816_usr.h
 * @ 作者: ELACO
 * @ 日期: 2026-07-27
 * @ 版本: 1.0.1
 * @ 说明: MT6816 磁编码器应用层头文件，提供角度读取接口
 ********/

#ifndef ELA_MT6816_USR_H
#define ELA_MT6816_USR_H

#include <stdbool.h>
#include "ela_mt6816_drv.h"

/********
 * @ 说明: 角度获取结构体
 ********/
typedef struct MT6816_ANGLE
{
    unsigned short raw_data;    /* 原始16位数据 */
    unsigned short raw_angle;   /* 解析后角度值 (0 ~ 16383) */
    unsigned int micro_angle;   /* 微步角度值 (0 ~ 51200) */
    bool data_valid;            /* 数据有效标志 */
    bool magnet_valid;          /* 磁铁有效标志 */
    bool direction;             /* 方向标志，0正向，1反向 */
} MT6816_ANGLE_T;

extern MT6816_ANGLE_T g_mt6816_st;

/* 读取命令值 */
#define MT6816_CMD_ANGLE      0x03
#define MT6816_CMD_RAW_ANGLE  0x04
#define MT6816_CMD_READ_BIT   0x80

/* 函数声明 */
void ela_mt6816_usr_init(void);
MT6816_ANGLE_T *ela_mt6816_usr_read_angle(void);

#endif

