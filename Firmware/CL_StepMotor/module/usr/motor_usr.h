/*****************************************************************************
 * @文件: motor_usr.h
 * @作者: cl
 * @日期: 2026-08-13
 * @版本: v1.0
 * @说明: 电机用户层（最小闭环：编码器 raw×25/8 映射 + FOC 电流输出 +
 *   位置/速度/电流命令 + IIR 速度估计 + 简单 P 环状态机）
 * @平台: STM32F103RET6
 * @依赖: mt6816_usr, tb67h450_usr, encoder_calibrator_usr
 ****************************************************************************/
#ifndef MOTOR_USR_H
#define MOTOR_USR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ==== 常量定义 ==== */
#define USR_MOTOR_HARD_STEPS      200U    /* 步进电机每圈 200 整步 */
#define USR_MOTOR_SOFT_DIVIDE     256U    /* 每整步细分 256 微步（memory） */
/* 每圈细分步 = 200 整步 × 256 微步 = 51200 */
#define USR_MOTOR_SUBDIVIDE_STEPS (USR_MOTOR_HARD_STEPS * USR_MOTOR_SOFT_DIVIDE)
#define USR_MOTOR_CONTROL_FREQ    20000U  /* 控制 tick 20kHz（memory） */
#define USR_MOTOR_CONTROL_US      50U     /* 控制周期 50µs（memory） */

/* 简单 P 环限幅（依据 .cl/memory/ motor_minloop_control: err 限 ±3200，Kp=dce_kp=200） */
#define USR_MOTOR_POS_ERR_MAX     3200U
/* 速度 P 环误差限幅（复刻参考 motor.c CalcPidToOutput pid.vError 限 ±1024×1024） */
#define USR_MOTOR_VEL_ERR_MAX     1048576U
/* 位置环速度阻尼误差限幅（复刻参考 motor.c CalcDceToOutput dce.vError 限 ±4000） */
#define USR_MOTOR_VEL_ERR_DAMP_MAX 4000U
/* 到位死区（最小闭环无 planner，软目标=目标，需自判到位；待实测确认整定） */
#define USR_MOTOR_POS_DEADBAND    128U   /* 位置到位死区（细分步，≈0.9°；待实测确认） */
#define USR_MOTOR_VEL_DEADBAND    512U   /* 速度到位死区（细分步/s，≈0.01 圈/s；待实测确认） */
#define USR_MOTOR_CUR_DEADBAND    10U    /* 电流到位死区（mA；待实测确认） */

/* ==== 类型定义 ==== */
/* 电机模式（对齐参考 motor.h，最小闭环仅实现 STOP/POSITION/VELOCITY/CURRENT，其余回 STOP） */
typedef enum {
    MODE_STOP = 0,
    MODE_COMMAND_POSITION,
    MODE_COMMAND_VELOCITY,
    MODE_COMMAND_CURRENT,
    MODE_COMMAND_TRAJECTORY,
    MODE_PWM_POSITION,
    MODE_PWM_VELOCITY,
    MODE_PWM_CURRENT,
    MODE_STEP_DIR
} Motor_Mode_T;

/* 电机状态（对齐参考 motor.h；最小闭环仅用 STOP/FINISH/RUNNING，任务6后补完整） */
typedef enum {
    STATE_STOP = 0,
    STATE_FINISH,
    STATE_RUNNING,
    STATE_OVERLOAD,
    STATE_STALL,
    STATE_NO_CALIB
} Motor_State_T;

/* 电机配置（精简自建：config_usr/planner 未建，先按最小闭环所需字段；
   后续 config_usr/PID-DCE 落地后扩为参考 motor.h 全结构） */
typedef struct {
    int32_t encoderHomeOffset;  /* 编码器零位偏移（细分步 0~51199） */
    int32_t ratedCurrent;       /* 额定电流限幅（mA） */
    int32_t ratedVelocity;      /* 额定速度限幅（细分步/s） */
    int32_t dceKp;              /* 位置 P 环增益（依据 .cl/memory/ dce_kp=200） */
    int32_t dceKd;              /* 位置环速度阻尼增益（依据 .cl/memory/ dce_kd=250） */
    int32_t pidKp;              /* 速度 P 环增益（依据 .cl/memory/ pid_kp=5） */
} Motor_Config_T;

/* ==== 全局实例 ==== */
/* ==== 接口 ==== */
void    USR_Motor_Init(void);
void    USR_Motor_SetConfig(Motor_Config_T *config);
void    USR_Motor_Tick20kHz(void);
void    USR_Motor_SetMode(Motor_Mode_T mode);
void    USR_Motor_SetPosition(int32_t pos);
void    USR_Motor_SetVelocity(int32_t vel);
void    USR_Motor_SetCurrent(int32_t cur);
void    USR_Motor_SetDisable(bool disable);
void    USR_Motor_SetBrake(bool brake);
void    USR_Motor_ClearStallFlag(void);
Motor_State_T USR_Motor_GetState(void);
float   USR_Motor_GetPosition(bool is_lap);
float   USR_Motor_GetVelocity(void);
float   USR_Motor_GetCurrent(void);
uint8_t USR_Motor_GetMode(void);
void    USR_Motor_GetTelemetry(float *pos, float *vel, float *cur,
                               uint8_t *mode, uint8_t *state);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_USR_H */
