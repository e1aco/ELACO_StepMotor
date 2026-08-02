/********
 * @ 文件: ela_motor_usr.h
 * @ 作者: ELACO
 * @ 日期: 2026-08-02
 * @ 版本: 1.0.0
 * @ 说明: 电机控制核心（复刻参考 zhjStepMotor motor.h）。
 *         含位置/速度/电流三种指令模式，DCE 双闭环 + PID 速度环，
 *         FOC 电流矢量输出，失步/过载检测与多档状态机
 * @ 依赖: ela_motion_planner_usr.h
 ********/

#ifndef ELA_MOTOR_USR_H
#define ELA_MOTOR_USR_H

#include <stdint.h>
#include <stdbool.h>
#include "ela_motion_planner_usr.h"

/* 步进参数 */
#define MOTOR_HARD_STEPS          200
#define SOFT_DIVIDE_NUM           256
#define MOTOR_SUBDIVIDE_STEPS     (MOTOR_HARD_STEPS * SOFT_DIVIDE_NUM)  /* 51200 */

/* 控制模式 */
typedef enum {
    MODE_STOP,
    MODE_COMMAND_POSITION,
    MODE_COMMAND_VELOCITY,
    MODE_COMMAND_CURRENT,
    MODE_COMMAND_TRAJECTORY,
    MODE_PWM_POSITION,
    MODE_PWM_VELOCITY,
    MODE_PWM_CURRENT,
    MODE_STEP_DIR
} Motor_Mode_t;

/* 运行状态 */
typedef enum {
    STATE_STOP,
    STATE_FINISH,
    STATE_RUNNING,
    STATE_OVERLOAD,
    STATE_STALL,
    STATE_NO_CALIB
} Motor_State_t;

/* PID 结构体 */
typedef struct {
    int32_t kp, ki, kd;
    int32_t vError, vErrorLast;
    int32_t outputKp, outputKi, outputKd;
    int32_t integralRound;
    int32_t integralRemainder;
    int32_t output;
} PID_t;

/* DCE 结构体 */
typedef struct {
    int32_t kp, kv, ki, kd;
    int32_t pError, vError;
    int32_t outputKp, outputKi, outputKd;
    int32_t integralRound;
    int32_t integralRemainder;
    int32_t output;
} DCE_t;

/* 控制器配置 */
typedef struct {
    PID_t pid;
    DCE_t dce;
    bool stallProtectSwitch;
} Controller_Config_t;

/* 电机配置 */
typedef struct {
    MotionPlanner_Config_t motionParams;
    Controller_Config_t ctrlParams;
} Motor_Config_t;

/* 初始化整个系统 */
void Motor_Init(void);

/* 设置电机配置 */
void Motor_SetConfig(Motor_Config_t* config);

/* 20kHz 中断中调用 */
void Motor_Tick20kHz(void);

/* 控制接口 */
void Motor_SetMode(Motor_Mode_t mode);
void Motor_SetPosition(int32_t pos);
void Motor_SetVelocity(int32_t vel);
void Motor_SetCurrent(int32_t cur);
void Motor_SetDisable(bool disable);
void Motor_SetBrake(bool brake);
void Motor_ClearStallFlag(void);

/* 状态读取 */
Motor_State_t Motor_GetState(void);
float Motor_GetPosition(bool isLap);
float Motor_GetVelocity(void);
float Motor_GetCurrent(void);
bool Motor_IsCalibrated(void);
uint8_t Motor_GetMode(void);

/* 位置校准接口 */
void Motor_TriggerCalibration(void);

void Motor_GetTelemetry(float *pos, float *vel, float *cur, uint8_t *mode, uint8_t *state);

/* 调试诊断：暴露内部状态，仅供串口打印 */
void Motor_GetFocDiag(int32_t *est, int32_t *foc, int32_t *goal, int32_t *soft,
                      int32_t *p_err, int32_t *v_err, int32_t *dce_out);

void Motor_ZeroPosition(void);

#endif
