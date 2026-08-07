/*****************************************************************************
 * @文件: ela_motion_run_usr.h
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: 正式运行态：按校准表查表 + 编码器闭环驱动。
 ****************************************************************************/

#ifndef ELA_MOTION_RUN_USR_H
#define ELA_MOTION_RUN_USR_H

#include <stdbool.h>
#include <stdint.h>

/* ==== 常量定义 ==== */
#define MOTION_RUN_TARGET_NUM  4

/* 演示/回零目标：编码器绝对位置（0 / 90 / 180 / 270 度） */
#define MOTION_RUN_TGT_0DEG      0
#define MOTION_RUN_TGT_90DEG     4096
#define MOTION_RUN_TGT_180DEG    8192

#define MOTION_RUN_TGT_270DEG    12288

/* ==== 接口 ==== */


void USR_MotionRun_Init(void);
void USR_MotionRun_Proc(void);
void USR_MotionRun_DemoStart(void);
void USR_MotionRun_DemoPoll(void);
void USR_MotionRun_DebugPrint(void);
void USR_MotionRun_GotoTarget(unsigned short target);
bool USR_MotionRun_IsIdle(void);
unsigned short USR_MotionRun_CurrentEnc(void);

#endif






