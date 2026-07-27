#ifndef _ELA_MOTOR_H_
#define _ELA_MOTOR_H_

#include "elaco_main.h"

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

typedef struct 
{
    int kp,ki,kd,kv;
    int pErr,vErr;
    int outputKp, outputKi, outputKd;
    int integralRound;
    int integralRemainder;
    int output;
}POSITION_PID_T;

/* 全局变量 */
extern unsigned char gov_motor_brake_flag; // 1表示刹车
extern unsigned char gov_motor_sleep_flag; // 1表示休眠
extern unsigned char gov_motor_mode_fsm; // 电机运行模式
POSITION_PID_T position_pid_st;


#define FREQUENCY_Motor 20000 // 电机控制频率，单位Hz

#endif
