/*****************************************************************************
 * @文件: elaco_calibration_usr.h
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: 编码器校准模块，生成角度校准表并写入 Flash
 ****************************************************************************/

#ifndef ELACO_CALIBRATION_USR_H
#define ELACO_CALIBRATION_USR_H

#include <stdbool.h>
#include <stdint.h>

/* ==== 常量定义 ==== */
#define ENC_RESOLUTION  16384
#define ENC_WHOLESTEP   81.92
#define MICROSTEPLAP    51200
#define WHOLESTEPLAP    200
#define SOFT_DIVIDE     256

#define SAMPLE_PER_STEP  16

#define AUTO_SPEED  2
#define FINE_SPEED  1

/* 复位对齐参数（闭环转到编码器目标值） */
#define CALI_RESET_ENC_TARGET  0      /* 复位目标编码器值 */
#define CALI_RESET_CTRL_DIV    5      /* 每 5 个 20kHz tick 控制一次 = 4kHz */
#define CALI_RESET_KP_SHIFT    6      /* Kp = 1/64 */
#define CALI_RESET_MAX_DELTA   4      /* 每控制周期最大微步 */
#define CALI_RESET_DEADBAND    4      /* 收敛死区（编码器计数） */
#define CALI_RESET_CONFIRM     3      /* 连续带内次数 */
#define CALI_RESET_DRIVE_MA    2000
#define CALI_RESET_HOLD_MA     2000

#define CALI_STEP_IDLE      0
#define CALI_STEP_RESET     1
#define CALI_STEP_COLLECT   2
#define CALI_STEP_CHECK     3
#define CALI_STEP_GENERATE  4
#define CALI_STEP_DONE      5

/* ==== 类型定义 ==== */
typedef struct {
    unsigned char  cali_step;
    bool           calitable_flag;
    unsigned char  data_err;
    unsigned short avg_fr_data[WHOLESTEPLAP + 1];
    unsigned char  jump_pot;
    unsigned short jump_pot_data;
    unsigned int   reset_microstep;
} CALIBRATION_DATA_T;

/* ==== 全局实例 ==== */
extern CALIBRATION_DATA_T g_calibra_st;
extern unsigned short *g_cali_table;

/* ==== 接口 ==== */




void USR_Calibration_Init(void);
void USR_Calibration_Start(void);
void USR_Calibration_Proc(void);
void USR_Calibration_TableGenerateProc(void);
void USR_Calibration_TableDataValid(void);

#endif






