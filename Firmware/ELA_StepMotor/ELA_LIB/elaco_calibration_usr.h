/********
 * @ 文件: elaco_calibration_usr.h
 * @ 作者: ELACO
 * @ 日期: 2026-07-27
 * @ 版本: 1.0.0
 * @ 说明: 编码器校准模块，生成角度校准表并写入 Flash
 * @ 依赖: ela_mt6816, ela_tb67h450, ela_cyclecal, ela_stockfile
 ********/

#ifndef ELACO_CALIBRATION_USR_H
#define ELACO_CALIBRATION_USR_H

#include <stdbool.h>
#include <stdint.h>

#define ENC_RESOLUTION  16384
#define ENC_WHOLESTEP   81.92
#define MICROSTEPLAP    51200
#define WHOLESTEPLAP    200
#define SOFT_DIVIDE     256

#define SAMPLE_PER_STEP  16

#define AUTO_SPEED  2
#define FINE_SPEED  1

#define CALI_STEP_IDLE      0
#define CALI_STEP_COLLECT   1
#define CALI_STEP_CHECK     2
#define CALI_STEP_GENERATE  3
#define CALI_STEP_DONE      4

typedef struct {
    unsigned char  cali_step;
    bool           calitable_flag;
    unsigned char  data_err;
    unsigned short avg_fr_data[WHOLESTEPLAP + 1];
    unsigned char  jump_pot;
    unsigned short jump_pot_data;
} CALIBRATION_DATA_T;

extern CALIBRATION_DATA_T g_calibra_st;
extern unsigned short *g_cali_table;

void elaco_calibration_proc(void);
void elaco_calibration_table_generate_proc(void);
void elaco_calibration_table_data_valid(void);

#endif

