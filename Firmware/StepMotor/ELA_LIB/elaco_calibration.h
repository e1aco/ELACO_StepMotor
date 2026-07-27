#ifndef _ELACO_CALIBRATION_H_
#define _ELACO_CALIBRATION_H_

#include "elaco_main.h" // ELA库

#define ENC_RESOLUTION 16384 // 编码器14位分辨率（2^14=16384）
#define ENC_WHOLESTEP 81.92 // 编码器一圈步数（16384/200）
#define MICROSTEPLAP 51200   // 细分步数，51200对应1圈（电角度0-51200）
#define WHOLESTEPLAP 200   // 整步数，200步对应1圈
#define SOFT_DIVIDE 256      // 电角度除数，51200/256=200步对应1圈

#define SAMPLE_PER_STEP 16   // 每个步采样次数

#define AUTO_SPEED 2 // 自动校准时的移动速度
#define FINE_SPEED 1 // 精调时的移动速度

extern unsigned short* gov_cali_table; // 校准表地址

/* 校准步骤 */
#define CALI_STEP_IDLE      0   // 空闲
#define CALI_STEP_COLLECT   1   // 数据采集（由 Calibration_Proc 在中断中执行）
#define CALI_STEP_CHECK     2   // 数据检查
#define CALI_STEP_GENERATE  3   // 生成校准表
#define CALI_STEP_DONE      4   // 完成

typedef struct 
{
    unsigned char cali_step;    // 校准步骤状态（CALI_STEP_xxx）
    bool calitable_flag;        // True表示校准表已经完成, False表示校准表未完成
    unsigned char data_err;     // 0=数据正常, 2=数据不连贯, 3=方向与实际不符, 4=校准表生成失败
    unsigned short avg_fr_data[WHOLESTEPLAP+1]; // 存储正反向数据的平均值
    unsigned char jump_pot; // 旋转过渡点，0-199（数据由16384回到0的这个点的对应的数组坐标）
    unsigned short jump_pot_data; // 旋转过渡点到最大值的距离
}CALIBRATION_DATA_T;

extern CALIBRATION_DATA_T gov_calibra_st;

void Calibration_Proc(void);
void Cali_Table_Generate_Proc(void);

#endif
