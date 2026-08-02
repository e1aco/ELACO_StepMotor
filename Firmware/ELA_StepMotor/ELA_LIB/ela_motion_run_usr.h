/********
 * @ 文件: ela_motion_run_usr.h
 * @ 作者: ELACO
 * @ 日期: 2026-08-02
 * @ 版本: 1.0.0
 * @ 说明: 正式运行态：按校准表查表 + 编码器闭环驱动。
 *         固定演示动作（0/90/180/270° 往返），供主流程调用
 * @ 依赖: ela_mt6816_usr, ela_tb67h450_usr, elaco_calibration_usr
 ********/

#ifndef ELA_MOTION_RUN_USR_H
#define ELA_MOTION_RUN_USR_H

#include <stdbool.h>
#include <stdint.h>

#define MOTION_RUN_TARGET_NUM  4

/* 演示/回零目标：编码器绝对位置（0 / 90 / 180 / 270 度） */
#define MOTION_RUN_TGT_0DEG      0
#define MOTION_RUN_TGT_90DEG     4096
#define MOTION_RUN_TGT_180DEG    8192
#define MOTION_RUN_TGT_270DEG    12288

void ela_motion_run_init(void);
void ela_motion_run_proc(void);
void ela_motion_run_demo_start(void);
void ela_motion_run_demo_poll(void);
void ela_motion_run_debug_print(void);
void ela_motion_run_goto_target(unsigned short target);
bool ela_motion_run_is_idle(void);
unsigned short ela_motion_run_current_enc(void);

#endif
