/********
 * @ 文件: ela_motor_usr.c
 * @ 作者: ELACO
 * @ 日期: 2026-08-02
 * @ 版本: 1.0.0
 * @ 说明: 电机控制核心（复刻参考 zhjStepMotor motor.c）。
 *         20kHz 中断入口 Motor_Tick20kHz：
 *         读编码器→多圈位置累加→低通速度估计→超前角补偿→
 *         模式分派(DCE 位置双闭环 / PID 速度环 / 电流直通)→
 *         FOC 电流矢量输出，附失步/过载检测与状态机。
 *         位置反馈：校准表查表 g_cali_table[raw]=微步（同参考 rectified）
 * @ 依赖: ela_mt6816_usr, ela_tb67h450_usr/drv, elaco_calibration_usr,
 *         ela_motion_planner_usr
 ********/

#include "ela_motor_usr.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ela_mt6816_usr.h"
#include "ela_tb67h450_usr.h"
#include "ela_tb67h450_drv.h"
#include "elaco_calibration_usr.h"

/* ==================== 运行状态变量 ==================== */
static Motor_Config_t* s_config = NULL;
static Motor_Mode_t s_request_mode = MODE_STOP;
static Motor_Mode_t s_mode_running = MODE_STOP;
static Motor_State_t s_state = STATE_STOP;
static bool s_is_stalled = false;

/* 实际值 */
static int32_t s_real_lap_position = 0;
static int32_t s_real_lap_position_last = 0;
static int32_t s_real_position = 0;
static int32_t s_real_position_last = 0;

/* 估计值 */
static int32_t s_est_velocity = 0;
static int32_t s_est_velocity_integral = 0;
static int32_t s_est_lead_position = 0;
static int32_t s_est_position = 0;

/* 目标值 */
static int32_t s_goal_position = 0;
static int32_t s_goal_velocity = 0;
static int32_t s_goal_current = 0;
static bool s_goal_disable = false;
static bool s_goal_brake = false;

/* 软目标值（经轨迹平滑后） */
static int32_t s_soft_position = 0;
static int32_t s_soft_velocity = 0;
static int32_t s_soft_current = 0;
static bool s_soft_disable = false;
static bool s_soft_brake = false;
static bool s_soft_new_curve = false;

/* FOC 输出 */
static int32_t s_foc_position = 0;
static int32_t s_foc_current = 0;

/* 故障检测 */
static uint32_t s_stalled_time = 0;
static uint32_t s_overload_time = 0;
static bool s_overload_flag = false;

/* 首次调用标志 */
static bool s_first_called = true;

/* ==================== 私有函数 ==================== */

/********
 * @ 输入: vel: 估计速度（微步/s）
 * @ 输出: 补偿角度（微步）
 * @ 说明: 按速度分三段线性补偿磁链超前角，限幅 ±430 微步
 ********/
static int32_t CompensateAdvancedAngle(int32_t vel)
{
    int32_t compensate;

    if (vel < 0) {
        if (vel > -100000) compensate = 0;
        else if (vel > -1300000) compensate = (((vel + 100000) * 262) >> 20) - 0;
        else if (vel > -2200000) compensate = (((vel + 1300000) * 105) >> 20) - 300;
        else compensate = (((vel + 2200000) * 52) >> 20) - 390;

        if (compensate < -430) compensate = -430;
    } else {
        if (vel < 100000) compensate = 0;
        else if (vel < 1300000) compensate = (((vel - 100000) * 262) >> 20) + 0;
        else if (vel < 2200000) compensate = (((vel - 1300000) * 105) >> 20) + 300;
        else compensate = (((vel - 2200000) * 52) >> 20) + 390;

        if (compensate > 430) compensate = 430;
    }

    return compensate;
}

/********
 * @ 输入: current: 电流指令（mA）
 * @ 说明: 电流转 FOC 位置输出。电流>0 时磁链超前估计位置 90°（+SOFT_DIVIDE_NUM），
 *         电流<0 时滞后 90°，电流=0 时磁链停在估计位置
 ********/
static void CalcCurrentToOutput(int32_t current)
{
    s_foc_current = current;

    if (s_foc_current > 0) {
        s_foc_position = s_est_position + SOFT_DIVIDE_NUM;  /* 超前90度 */
    } else if (s_foc_current < 0) {
        s_foc_position = s_est_position - SOFT_DIVIDE_NUM;  /* 滞后90度 */
    } else {
        s_foc_position = s_est_position;
    }

    ela_tb67h450_set_foc_current((unsigned short)s_foc_position, (short)s_foc_current);
}

/********
 * @ 输入: speed: 目标速度（微步/s）
 * @ 说明: PID 速度环。误差限幅 ±1M，I 用 /1024 拆商余，
 *         Ki 项限幅 ±ratedCurrent<<10，输出限幅 ±ratedCurrent
 ********/
static void CalcPidToOutput(int32_t speed)
{
    /* PID 速度环 */
    s_config->ctrlParams.pid.vErrorLast = s_config->ctrlParams.pid.vError;
    s_config->ctrlParams.pid.vError = speed - s_est_velocity;

    /* 限幅 */
    if (s_config->ctrlParams.pid.vError > (1024 * 1024))
        s_config->ctrlParams.pid.vError = (1024 * 1024);
    if (s_config->ctrlParams.pid.vError < (-1024 * 1024))
        s_config->ctrlParams.pid.vError = (-1024 * 1024);

    s_config->ctrlParams.pid.outputKp = s_config->ctrlParams.pid.kp * s_config->ctrlParams.pid.vError;

    /* 积分器 */
    s_config->ctrlParams.pid.integralRound += (s_config->ctrlParams.pid.ki * s_config->ctrlParams.pid.vError);
    s_config->ctrlParams.pid.integralRemainder = s_config->ctrlParams.pid.integralRound >> 10;
    s_config->ctrlParams.pid.integralRound -= (s_config->ctrlParams.pid.integralRemainder << 10);
    s_config->ctrlParams.pid.outputKi += s_config->ctrlParams.pid.integralRemainder;

    /* 积分限幅 */
    if (s_config->ctrlParams.pid.outputKi > (s_config->motionParams.ratedCurrent << 10))
        s_config->ctrlParams.pid.outputKi = (s_config->motionParams.ratedCurrent << 10);
    else if (s_config->ctrlParams.pid.outputKi < -(s_config->motionParams.ratedCurrent << 10))
        s_config->ctrlParams.pid.outputKi = -(s_config->motionParams.ratedCurrent << 10);

    /* 微分项 */
    s_config->ctrlParams.pid.outputKd = s_config->ctrlParams.pid.kd *
                                        (s_config->ctrlParams.pid.vError - s_config->ctrlParams.pid.vErrorLast);

    /* 合成输出 */
    s_config->ctrlParams.pid.output = (s_config->ctrlParams.pid.outputKp +
                                       s_config->ctrlParams.pid.outputKi +
                                       s_config->ctrlParams.pid.outputKd) >> 10;

    /* 输出限幅 */
    if (s_config->ctrlParams.pid.output > s_config->motionParams.ratedCurrent)
        s_config->ctrlParams.pid.output = s_config->motionParams.ratedCurrent;
    else if (s_config->ctrlParams.pid.output < -s_config->motionParams.ratedCurrent)
        s_config->ctrlParams.pid.output = -s_config->motionParams.ratedCurrent;

    CalcCurrentToOutput(s_config->ctrlParams.pid.output);
}

/********
 * @ 输入: location: 目标位置（微步）
 *         speed: 目标速度（微步/s）
 * @ 说明: DCE 双闭环（位置闭环 + 速度前馈）。pError 限幅 ±3200，
 *         vError 限幅 ±4000，Ki/Kv 积分 /128 拆商余，
 *         Ki 限幅 ±ratedCurrent<<10，输出限幅 ±ratedCurrent
 ********/
static void CalcDceToOutput(int32_t location, int32_t speed)
{
    /* DCE 双闭环位置控制 */
    s_config->ctrlParams.dce.pError = location - s_est_position;
    s_config->ctrlParams.dce.vError = (speed - s_est_velocity) >> 7;

    /* 限幅 */
    if (s_config->ctrlParams.dce.pError > 3200) s_config->ctrlParams.dce.pError = 3200;
    if (s_config->ctrlParams.dce.pError < -3200) s_config->ctrlParams.dce.pError = -3200;
    if (s_config->ctrlParams.dce.vError > 4000) s_config->ctrlParams.dce.vError = 4000;
    if (s_config->ctrlParams.dce.vError < -4000) s_config->ctrlParams.dce.vError = -4000;

    /* 比例项 */
    s_config->ctrlParams.dce.outputKp = s_config->ctrlParams.dce.kp * s_config->ctrlParams.dce.pError;

    /* 积分项（位置+速度共同积分） */
    s_config->ctrlParams.dce.integralRound += (s_config->ctrlParams.dce.ki * s_config->ctrlParams.dce.pError +
                                               s_config->ctrlParams.dce.kv * s_config->ctrlParams.dce.vError);
    s_config->ctrlParams.dce.integralRemainder = s_config->ctrlParams.dce.integralRound >> 7;
    s_config->ctrlParams.dce.integralRound -= (s_config->ctrlParams.dce.integralRemainder << 7);
    s_config->ctrlParams.dce.outputKi += s_config->ctrlParams.dce.integralRemainder;

    /* 积分限幅 */
    if (s_config->ctrlParams.dce.outputKi > (s_config->motionParams.ratedCurrent << 10))
        s_config->ctrlParams.dce.outputKi = (s_config->motionParams.ratedCurrent << 10);
    else if (s_config->ctrlParams.dce.outputKi < -(s_config->motionParams.ratedCurrent << 10))
        s_config->ctrlParams.dce.outputKi = -(s_config->motionParams.ratedCurrent << 10);

    /* 微分项 */
    s_config->ctrlParams.dce.outputKd = s_config->ctrlParams.dce.kd * s_config->ctrlParams.dce.vError;

    /* 合成输出 */
    s_config->ctrlParams.dce.output = (s_config->ctrlParams.dce.outputKp +
                                       s_config->ctrlParams.dce.outputKi +
                                       s_config->ctrlParams.dce.outputKd) >> 10;

    /* 输出限幅 */
    if (s_config->ctrlParams.dce.output > s_config->motionParams.ratedCurrent)
        s_config->ctrlParams.dce.output = s_config->motionParams.ratedCurrent;
    else if (s_config->ctrlParams.dce.output < -s_config->motionParams.ratedCurrent)
        s_config->ctrlParams.dce.output = -s_config->motionParams.ratedCurrent;

    CalcCurrentToOutput(s_config->ctrlParams.dce.output);
}

/********
 * @ 说明: 清零 PID 与 DCE 积分器
 ********/
static void ClearIntegral(void)
{
    s_config->ctrlParams.pid.integralRound = 0;
    s_config->ctrlParams.pid.integralRemainder = 0;
    s_config->ctrlParams.pid.outputKi = 0;

    s_config->ctrlParams.dce.integralRound = 0;
    s_config->ctrlParams.dce.integralRemainder = 0;
    s_config->ctrlParams.dce.outputKi = 0;
}

/* ==================== 公开接口 ==================== */
void Motor_Init(void)
{
    s_first_called = true;
    s_is_stalled = false;
    s_overload_flag = false;
    s_stalled_time = 0;
    s_overload_time = 0;

    /* 初始化运动规划器 */
    if (s_config) {
        g_motion_config = &s_config->motionParams;
        CurrentTracker_Init();
        VelocityTracker_Init();
        PositionTracker_Init();
        PositionInterpolator_Init();
        TrajectoryTracker_Init(200);
    }
}

void Motor_SetConfig(Motor_Config_t* config)
{
    s_config = config;
}

void Motor_Tick20kHz(void)
{
    uint16_t rectified_angle = 0;

    /* 读取编码器角度并查表得到微步位置（同参考 MT6816_GetRectifiedAngle） */
    ela_mt6816_usr_read_angle();
    rectified_angle = (uint16_t)g_cali_table[g_mt6816_st.raw_angle];

    /* 首次调用，初始化位置 */
    if (s_first_called) {
        int32_t angle;
        if (s_config->motionParams.encoderHomeOffset < MOTOR_SUBDIVIDE_STEPS / 2) {
            angle = (rectified_angle > s_config->motionParams.encoderHomeOffset + MOTOR_SUBDIVIDE_STEPS / 2) ?
                    rectified_angle - MOTOR_SUBDIVIDE_STEPS : rectified_angle;
        } else {
            angle = (rectified_angle < s_config->motionParams.encoderHomeOffset - MOTOR_SUBDIVIDE_STEPS / 2) ?
                    rectified_angle + MOTOR_SUBDIVIDE_STEPS : rectified_angle;
        }

        s_real_lap_position = angle;
        s_real_lap_position_last = angle;
        s_real_position = angle;
        s_real_position_last = angle;
        s_first_called = false;
        return;
    }

    /* 计算位置 */
    s_real_lap_position_last = s_real_lap_position;
    s_real_lap_position = rectified_angle;

    int32_t delta = s_real_lap_position - s_real_lap_position_last;
    if (delta > (MOTOR_SUBDIVIDE_STEPS >> 1))
        delta -= MOTOR_SUBDIVIDE_STEPS;
    else if (delta < -(MOTOR_SUBDIVIDE_STEPS >> 1))
        delta += MOTOR_SUBDIVIDE_STEPS;

    s_real_position_last = s_real_position;
    s_real_position += delta;

    /* 估计速度（低通：63/64 旧值 + 1/64 新值，等效 1 阶低通） */
    s_est_velocity_integral += ((s_real_position - s_real_position_last) * CONTROL_FREQUENCY +
                                ((s_est_velocity << 5) - s_est_velocity));
    s_est_velocity = s_est_velocity_integral >> 5;
    s_est_velocity_integral -= (s_est_velocity << 5);

    /* 估计位置：考虑超前角补偿 */
    s_est_lead_position = CompensateAdvancedAngle(s_est_velocity);
    s_est_position = s_real_position + s_est_lead_position;

    /* 控制循环 */
    if (s_is_stalled || s_soft_disable || !Motor_IsCalibrated()) {/* 停止 */
        ClearIntegral();
        s_foc_position = 0;
        s_foc_current = 0;
        ela_tb67h450_sleep();
    } else if (s_soft_brake) { /* 刹车 */
        ClearIntegral();
        s_foc_position = 0;
        s_foc_current = 0;
        ela_tb67h450_brake();
    } else {
        switch (s_mode_running) {
            case MODE_STOP:
                ela_tb67h450_sleep();
                break;
            case MODE_COMMAND_POSITION:
            case MODE_COMMAND_TRAJECTORY:
            case MODE_PWM_POSITION:
                CalcDceToOutput(s_soft_position, s_soft_velocity);
                break;
            case MODE_COMMAND_VELOCITY:
            case MODE_PWM_VELOCITY:
                CalcPidToOutput(s_soft_velocity);
                break;
            case MODE_COMMAND_CURRENT:
            case MODE_PWM_CURRENT:
                CalcCurrentToOutput(s_soft_current);
                break;
            default:
                break;
        }
    }

    /* 模式切换 */
    if (s_mode_running != s_request_mode) {
        s_mode_running = s_request_mode;
        s_soft_new_curve = true;
    }

    /* 限幅 */
    if (s_goal_velocity > s_config->motionParams.ratedVelocity)
        s_goal_velocity = s_config->motionParams.ratedVelocity;
    else if (s_goal_velocity < -s_config->motionParams.ratedVelocity)
        s_goal_velocity = -s_config->motionParams.ratedVelocity;
    if (s_goal_current > s_config->motionParams.ratedCurrent)
        s_goal_current = s_config->motionParams.ratedCurrent;
    else if (s_goal_current < -s_config->motionParams.ratedCurrent)
        s_goal_current = -s_config->motionParams.ratedCurrent;

    /* 运动规划 */
    if ((s_soft_disable && !s_goal_disable) || (s_soft_brake && !s_goal_brake)) {
        s_soft_new_curve = true;
    }

    if (s_soft_new_curve) {
        s_soft_new_curve = false;
        ClearIntegral();
        Motor_ClearStallFlag();

        switch (s_mode_running) {
            case MODE_COMMAND_POSITION:
            case MODE_PWM_POSITION:
                PositionTracker_NewTask(s_est_position, s_est_velocity);
                break;
            case MODE_COMMAND_VELOCITY:
            case MODE_PWM_VELOCITY:
                VelocityTracker_NewTask(s_est_velocity);
                break;
            case MODE_COMMAND_CURRENT:
            case MODE_PWM_CURRENT:
                CurrentTracker_NewTask(s_foc_current);
                break;
            case MODE_COMMAND_TRAJECTORY:
                TrajectoryTracker_NewTask(s_est_position, s_est_velocity);
                break;
            default:
                break;
        }
    }

    /* 更新软目标 */
    switch (s_mode_running) {
        case MODE_COMMAND_POSITION:
        case MODE_PWM_POSITION:
            PositionTracker_CalcSoftGoal(s_goal_position);
            s_soft_position = g_go_location;
            s_soft_velocity = g_go_location_velocity;
            break;
        case MODE_COMMAND_VELOCITY:
        case MODE_PWM_VELOCITY:
            VelocityTracker_CalcSoftGoal(s_goal_velocity);
            s_soft_velocity = g_go_velocity;
            break;
        case MODE_COMMAND_CURRENT:
        case MODE_PWM_CURRENT:
            CurrentTracker_CalcSoftGoal(s_goal_current);
            s_soft_current = g_go_current;
            break;
        case MODE_COMMAND_TRAJECTORY:
            TrajectoryTracker_CalcSoftGoal(s_goal_position, s_goal_velocity);
            s_soft_position = g_traj_go_position;
            s_soft_velocity = g_traj_go_velocity;
            break;
        default:
            break;
    }

    s_soft_disable = s_goal_disable;
    s_soft_brake = s_goal_brake;

    /* 失步检测 */
    int32_t current_abs = abs(s_foc_current);

    if (s_config->ctrlParams.stallProtectSwitch) {
        if (((s_mode_running == MODE_COMMAND_CURRENT || s_mode_running == MODE_PWM_CURRENT) && current_abs != 0) ||
            current_abs == s_config->motionParams.ratedCurrent) {
            if (abs(s_est_velocity) < MOTOR_SUBDIVIDE_STEPS / 5) {
                if (s_stalled_time >= 1000 * 1000) {
                    s_is_stalled = true;
                } else {
                    s_stalled_time += CONTROL_PERIOD_US;
                }
            }
        } else {
            s_stalled_time = 0;
        }
    }

    /* 过载检测 */
    if ((s_mode_running != MODE_COMMAND_CURRENT) && (s_mode_running != MODE_PWM_CURRENT) &&
        current_abs == s_config->motionParams.ratedCurrent) {
        if (s_overload_time >= 1000 * 1000) {
            s_overload_flag = true;
        } else {
            s_overload_time += CONTROL_PERIOD_US;
        }
    } else {
        s_overload_time = 0;
        s_overload_flag = false;
    }

    /* 状态判定 */
    if (!Motor_IsCalibrated()) {
        s_state = STATE_NO_CALIB;
    } else if (s_mode_running == MODE_STOP) {
        s_state = STATE_STOP;
    } else if (s_is_stalled) {
        s_state = STATE_STALL;
    } else if (s_overload_flag) {
        s_state = STATE_OVERLOAD;
    } else {
        /* 根据模式判定 */
        if (s_mode_running == MODE_COMMAND_POSITION) {
            if ((s_soft_position == s_goal_position) && (s_soft_velocity == 0))
                s_state = STATE_FINISH;
            else
                s_state = STATE_RUNNING;
        } else if (s_mode_running == MODE_COMMAND_VELOCITY) {
            if (s_soft_velocity == s_goal_velocity)
                s_state = STATE_FINISH;
            else
                s_state = STATE_RUNNING;
        } else if (s_mode_running == MODE_COMMAND_CURRENT) {
            if (s_soft_current == s_goal_current)
                s_state = STATE_FINISH;
            else
                s_state = STATE_RUNNING;
        } else {
            s_state = STATE_FINISH;
        }
    }
}

/* ==================== 控制接口 ==================== */
void Motor_SetMode(Motor_Mode_t mode)
{
    s_request_mode = mode;
}

void Motor_SetPosition(int32_t pos)
{
    s_goal_position = pos + s_config->motionParams.encoderHomeOffset;
}

void Motor_SetVelocity(int32_t vel)
{
    if (vel >= -s_config->motionParams.ratedVelocity &&
        vel <= s_config->motionParams.ratedVelocity) {
        s_goal_velocity = vel;
    }
}

void Motor_SetCurrent(int32_t cur)
{
    if (cur > s_config->motionParams.ratedCurrent)
        s_goal_current = s_config->motionParams.ratedCurrent;
    else if (cur < -s_config->motionParams.ratedCurrent)
        s_goal_current = -s_config->motionParams.ratedCurrent;
    else
        s_goal_current = cur;
}

void Motor_SetDisable(bool disable)
{
    s_goal_disable = disable;
}

void Motor_SetBrake(bool brake)
{
    s_goal_brake = brake;
}

void Motor_ClearStallFlag(void)
{
    s_stalled_time = 0;
    s_is_stalled = false;
}

/* ==================== 状态读取 ==================== */
Motor_State_t Motor_GetState(void)
{
    return s_state;
}

float Motor_GetPosition(bool isLap)
{
    if (isLap) {
        return (float)(s_real_lap_position - s_config->motionParams.encoderHomeOffset) /
               (float)MOTOR_SUBDIVIDE_STEPS;
    } else {
        return (float)(s_real_position - s_config->motionParams.encoderHomeOffset) /
               (float)MOTOR_SUBDIVIDE_STEPS;
    }
}

float Motor_GetVelocity(void)
{
    return (float)s_est_velocity / (float)MOTOR_SUBDIVIDE_STEPS;
}

float Motor_GetCurrent(void)
{
    return (float)s_foc_current / 1000.0f;
}

bool Motor_IsCalibrated(void)
{
    return g_calibra_st.calitable_flag;
}

void Motor_TriggerCalibration(void)
{
    /* 校准触发统一由主流程 elaco_calibration_usr_reset 完成 */
}

uint8_t Motor_GetMode(void)
{
    return s_mode_running;
}

void Motor_GetTelemetry(float *pos, float *vel, float *cur, uint8_t *mode, uint8_t *state)
{
    *pos = Motor_GetPosition(false);
    *vel = Motor_GetVelocity();
    *cur = Motor_GetCurrent();
    *mode = s_mode_running;
    *state = s_state;
}

/* 调试诊断：暴露内部状态（est/foc/goal/soft），仅供串口打印 */
void Motor_GetFocDiag(int32_t *est, int32_t *foc, int32_t *goal, int32_t *soft,
                      int32_t *p_err, int32_t *v_err, int32_t *dce_out)
{
    if (est)  *est  = s_est_position;
    if (foc)  *foc  = s_foc_position;
    if (goal) *goal = s_goal_position;
    if (soft) *soft = s_soft_position;
    if (p_err) *p_err = s_config->ctrlParams.dce.pError;
    if (v_err) *v_err = s_config->ctrlParams.dce.vError;
    if (dce_out) *dce_out = s_config->ctrlParams.dce.output;
}

void Motor_ZeroPosition(void)
{
    /* 把当前位置设为新的 HomeOffset */
    s_config->motionParams.encoderHomeOffset = s_real_position % MOTOR_SUBDIVIDE_STEPS;
}
