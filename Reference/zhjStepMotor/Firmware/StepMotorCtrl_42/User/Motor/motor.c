#include "motor.h"
#include <math.h>
#include <stdlib.h>
#include "mt6816.h"    
#include "tb67h450.h"   
#include "encoder_calibrator.h"
#include <stdio.h>
#include "usart.h"
#include <string.h>
#include "eeprom.h"
#include "configurations.h"
/* ==================== 电机状态变量 ==================== */
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

/* 软目标值（平滑后的）*/
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

/* 外部引用 */
extern BoardConfig_t boardConfig;

/* ==================== 辅助函数 ==================== */
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

static void CalcCurrentToOutput(int32_t current)
{
    s_foc_current = current;
    
    if (s_foc_current > 0) {
        s_foc_position = s_est_position + SOFT_DIVIDE_NUM;  /* 超前90° */
    } else if (s_foc_current < 0) {
        s_foc_position = s_est_position - SOFT_DIVIDE_NUM;  /* 滞后90° */
    } else {
        s_foc_position = s_est_position;
    }
	
		TB67H450_SetFocCurrentVector(s_foc_position, s_foc_current);
}

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
    
    /* 积分项 */
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
    
    /* 总输出 */
    s_config->ctrlParams.pid.output = (s_config->ctrlParams.pid.outputKp + 
                                       s_config->ctrlParams.pid.outputKi + 
                                       s_config->ctrlParams.pid.outputKd) >> 10;
    
    /* 输出限幅 */
    if (s_config->ctrlParams.pid.output > s_config->motionParams.ratedCurrent)
        s_config->ctrlParams.pid.output = s_config->motionParams.ratedCurrent;
    else if (s_config->ctrlParams.pid.output < -s_config->motionParams.ratedCurrent)
        s_config->ctrlParams.pid.output = -s_config->motionParams.ratedCurrent;
		
    CalcCurrentToOutput(s_config->ctrlParams.pid.output);
		
//		printf("target=%ld, est=%ld, err=%ld, out=%ld\r\n",
//       speed, s_est_velocity, 
//       s_config->ctrlParams.pid.vError,
//       s_config->ctrlParams.pid.output);
}

static void CalcDceToOutput(int32_t location, int32_t speed)
{
    /* DCE 双闭环控制器 */
    s_config->ctrlParams.dce.pError = location - s_est_position;
    s_config->ctrlParams.dce.vError = (speed - s_est_velocity) >> 7;
    
    /* 限幅 */
    if (s_config->ctrlParams.dce.pError > 3200) s_config->ctrlParams.dce.pError = 3200;
    if (s_config->ctrlParams.dce.pError < -3200) s_config->ctrlParams.dce.pError = -3200;
    if (s_config->ctrlParams.dce.vError > 4000) s_config->ctrlParams.dce.vError = 4000;
    if (s_config->ctrlParams.dce.vError < -4000) s_config->ctrlParams.dce.vError = -4000;
    
    /* 比例项 */
    s_config->ctrlParams.dce.outputKp = s_config->ctrlParams.dce.kp * s_config->ctrlParams.dce.pError;
    
    /* 积分项 */
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
    
    /* 总输出 */
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

static void ClearIntegral(void)
{
    s_config->ctrlParams.pid.integralRound = 0;
    s_config->ctrlParams.pid.integralRemainder = 0;
    s_config->ctrlParams.pid.outputKi = 0;
    
    s_config->ctrlParams.dce.integralRound = 0;
    s_config->ctrlParams.dce.integralRemainder = 0;
    s_config->ctrlParams.dce.outputKi = 0;
}

/* ==================== 公共函数 ==================== */
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
	
		MT6816_UpdateAngle();
    /* 读取编码器角度*/
    uint16_t rectified_angle = 0;
    rectified_angle = MT6816_GetRectifiedAngle();
	
	
    /* 首次调用：初始化位置 */
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
    
    /* 更新位置 */
    s_real_lap_position_last = s_real_lap_position;
    s_real_lap_position = rectified_angle;
    
    int32_t delta = s_real_lap_position - s_real_lap_position_last;
    if (delta > (MOTOR_SUBDIVIDE_STEPS >> 1))
        delta -= MOTOR_SUBDIVIDE_STEPS;
    else if (delta < -(MOTOR_SUBDIVIDE_STEPS >> 1))
        delta += MOTOR_SUBDIVIDE_STEPS;
		
    s_real_position_last = s_real_position;
    s_real_position += delta;
		
    /* 估计速度 */
    s_est_velocity_integral += ((s_real_position - s_real_position_last) * CONTROL_FREQUENCY +
                                ((s_est_velocity << 5) - s_est_velocity));
    s_est_velocity = s_est_velocity_integral >> 5;
    s_est_velocity_integral -= (s_est_velocity << 5);
	
    /* 估计位置（带超前角补偿）*/
    s_est_lead_position = CompensateAdvancedAngle(s_est_velocity);
    s_est_position = s_real_position + s_est_lead_position;
		
    /* 控制循环 */
    if (s_is_stalled || s_soft_disable || !EncoderCalibrator_IsCalibrated()) {/* 休眠 */
        ClearIntegral();
        s_foc_position = 0;
        s_foc_current = 0;
				TB67H450_Sleep();
    } else if (s_soft_brake) { /* 刹车 */
        ClearIntegral();
        s_foc_position = 0;
        s_foc_current = 0;
			  TB67H450_Brake();
    } else {
        switch (s_mode_running) {
            case MODE_STOP:
								TB67H450_Sleep();
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
    
    /* 计算软目标 */
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
    
    /* 故障检测 */
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
    
    /* 状态机 */
    if (!EncoderCalibrator_IsCalibrated()) {
        s_state = STATE_NO_CALIB;
    } else if (s_mode_running == MODE_STOP) {
        s_state = STATE_STOP;
    } else if (s_is_stalled) {
        s_state = STATE_STALL;
    } else if (s_overload_flag) {
        s_state = STATE_OVERLOAD;
    } else {
        // 加上模式判断
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
    // 直接从编码器校准模块获取状态
    return EncoderCalibrator_IsCalibrated();
}

void Motor_TriggerCalibration(void)
{
    // 触发编码器校准
    EncoderCalibrator_Trigger();
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

void Motor_ZeroPosition(void)
{
    // 把当前位置设为新的 HomeOffset
    s_config->motionParams.encoderHomeOffset = s_real_position % MOTOR_SUBDIVIDE_STEPS;
    
    // 保存到 EEPROM
    boardConfig.encoderHomeOffset = s_config->motionParams.encoderHomeOffset;
    EEPROM_Write(0, &boardConfig, sizeof(BoardConfig_t));
}