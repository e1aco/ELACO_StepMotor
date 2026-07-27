#include "motion_planner.h"
#include <stdlib.h>
#include <stdio.h>
#include "usart.h"
#include <string.h>
/* 配置指针 */
MotionPlanner_Config_t* g_motion_config = NULL;

/* ==================== CurrentTracker 全局变量 ==================== */
static int32_t s_current_acc = 0;
static int32_t s_current_integral = 0;
static int32_t s_track_current = 0;
int32_t g_go_current = 0;

/* ==================== VelocityTracker 全局变量 ==================== */
static int32_t s_velocity_acc = 0;
static int32_t s_velocity_integral = 0;
static int32_t s_track_velocity = 0;
int32_t g_go_velocity = 0;

/* ==================== PositionTracker 全局变量 ==================== */
static int32_t s_velocity_up_acc = 0;
static int32_t s_velocity_down_acc = 0;
static float s_quick_velocity_down_acc = 0;
static int32_t s_speed_locking_brake = 0;
static int32_t s_velocity_integral_pos = 0;
static int32_t s_track_velocity_pos = 0;
static int32_t s_position_integral = 0;
static int32_t s_track_position = 0;
int32_t g_go_location = 0;
int32_t g_go_location_velocity = 0;

/* ==================== PositionInterpolator 全局变量 ==================== */
static int32_t s_record_position = 0;
static int32_t s_record_position_last = 0;
static int32_t s_est_position = 0;
static int32_t s_est_position_integral = 0;
static int32_t s_est_velocity_interp = 0;
int32_t g_interp_go_position = 0;
int32_t g_interp_go_velocity = 0;

/* ==================== TrajectoryTracker 全局变量 ==================== */
static int32_t s_velocity_down_acc_traj = 0;
static int32_t s_dynamic_velocity_acc = 0;
static int32_t s_update_time = 0;
static int32_t s_update_timeout = 200;
static bool s_overtime_flag = false;
static int32_t s_record_velocity = 0;
static int32_t s_record_position_traj = 0;
static int32_t s_dynamic_vel_acc_remainder = 0;
static int32_t s_velocity_now = 0;
static int32_t s_velocity_now_remainder = 0;
static int32_t s_position_now = 0;
int32_t g_traj_go_position = 0;
int32_t g_traj_go_velocity = 0;

/* ==================== 辅助函数 ==================== */
static void CalcCurrentIntegral(int32_t current)
{
    s_current_integral += current;
    s_track_current += s_current_integral / CONTROL_FREQUENCY;
    s_current_integral = s_current_integral % CONTROL_FREQUENCY;
}

static void CalcVelocityIntegral(int32_t velocity)
{
    s_velocity_integral += velocity;
    s_track_velocity += s_velocity_integral / CONTROL_FREQUENCY;
    s_velocity_integral = s_velocity_integral % CONTROL_FREQUENCY;
}

static void CalcPositionVelocityIntegral(int32_t value)
{
    s_velocity_integral_pos += value;
    s_track_velocity_pos += s_velocity_integral_pos / CONTROL_FREQUENCY;
    s_velocity_integral_pos = s_velocity_integral_pos % CONTROL_FREQUENCY;
}

static void CalcPositionIntegral(int32_t value)
{
    s_position_integral += value;
    s_track_position += s_position_integral / CONTROL_FREQUENCY;
    s_position_integral = s_position_integral % CONTROL_FREQUENCY;
}

static void CalcTrajVelocityIntegral(int32_t value)
{
    s_dynamic_vel_acc_remainder += value;
    s_velocity_now += s_dynamic_vel_acc_remainder / CONTROL_FREQUENCY;
    s_dynamic_vel_acc_remainder = s_dynamic_vel_acc_remainder % CONTROL_FREQUENCY;
}

static void CalcTrajPositionIntegral(int32_t value)
{
    s_velocity_now_remainder += value;
    s_position_now += s_velocity_now_remainder / CONTROL_FREQUENCY;
    s_velocity_now_remainder = s_velocity_now_remainder % CONTROL_FREQUENCY;
}

/* ==================== CurrentTracker 实现 ==================== */
void CurrentTracker_Init(void)
{
    CurrentTracker_SetCurrentAcc(g_motion_config->ratedCurrentAcc);
}

void CurrentTracker_SetCurrentAcc(int32_t currentAcc)
{
    s_current_acc = currentAcc;
}

void CurrentTracker_NewTask(int32_t realCurrent)
{
    s_current_integral = 0;
    s_track_current = realCurrent;
}
//电流梯形平滑规划控制
void CurrentTracker_CalcSoftGoal(int32_t goalCurrent)
{
    int32_t delta = goalCurrent - s_track_current;
    
    if (delta == 0) {
        s_track_current = goalCurrent;
    } else if (delta > 0) {
        if (s_track_current >= 0) {
            CalcCurrentIntegral(s_current_acc);
            if (s_track_current >= goalCurrent) {
                s_current_integral = 0;
                s_track_current = goalCurrent;
            }
        } else {
            CalcCurrentIntegral(s_current_acc);
            if (s_track_current >= 0) {
                s_current_integral = 0;
                s_track_current = 0;
            }
        }
    } else {
        if (s_track_current <= 0) {
            CalcCurrentIntegral(-s_current_acc);
            if (s_track_current <= goalCurrent) {
                s_current_integral = 0;
                s_track_current = goalCurrent;
            }
        } else {
            CalcCurrentIntegral(-s_current_acc);
            if (s_track_current <= 0) {
                s_current_integral = 0;
                s_track_current = 0;
            }
        }
    }
    
    g_go_current = s_track_current;
}

/* ==================== VelocityTracker 实现 ==================== */
void VelocityTracker_Init(void)
{
    VelocityTracker_SetVelocityAcc(g_motion_config->ratedVelocityAcc);
}

void VelocityTracker_SetVelocityAcc(int32_t velocityAcc)
{
    s_velocity_acc = velocityAcc;
}

void VelocityTracker_NewTask(int32_t realVelocity)
{
    s_velocity_integral = 0;
    s_track_velocity = realVelocity;
}

//速度梯形平滑规划控制
void VelocityTracker_CalcSoftGoal(int32_t goalVelocity)
{
    int32_t delta = goalVelocity - s_track_velocity;
    
    if (delta == 0) {
        s_track_velocity = goalVelocity;
    } else if (delta > 0) {
        if (s_track_velocity >= 0) {
            CalcVelocityIntegral(s_velocity_acc);
            if (s_track_velocity >= goalVelocity) {
                s_velocity_integral = 0;
                s_track_velocity = goalVelocity;
            }
        } else {
            CalcVelocityIntegral(s_velocity_acc);
            if (s_track_velocity >= 0) {
                s_velocity_integral = 0;
                s_track_velocity = 0;
            }
        }
    } else {
        if (s_track_velocity <= 0) {
            CalcVelocityIntegral(-s_velocity_acc);
            if (s_track_velocity <= goalVelocity) {
                s_velocity_integral = 0;
                s_track_velocity = goalVelocity;
            }
        } else {
            CalcVelocityIntegral(-s_velocity_acc);
            if (s_track_velocity <= 0) {
                s_velocity_integral = 0;
                s_track_velocity = 0;
            }
        }
    }
    
    g_go_velocity = s_track_velocity;
}

/* ==================== PositionTracker 实现 ==================== */
void PositionTracker_Init(void)
{
    PositionTracker_SetVelocityAcc(g_motion_config->ratedVelocityAcc);
    s_speed_locking_brake = g_motion_config->ratedVelocityAcc / 1000;
}

void PositionTracker_SetVelocityAcc(int32_t value)
{
    s_velocity_up_acc = value;
    s_velocity_down_acc = value;
    s_quick_velocity_down_acc = 0.5f / (float)s_velocity_down_acc;
}

void PositionTracker_NewTask(int32_t realLocation, int32_t realSpeed)
{
    s_velocity_integral_pos = 0;
    s_track_velocity_pos = realLocation;
    s_position_integral = 0;
    s_track_position = realSpeed;
}
//位置S形平滑规划控制
void PositionTracker_CalcSoftGoal(int32_t goalPosition)
{
    int32_t delta = goalPosition - s_track_position;  // 剩余距离
    
    /* ==================== 情况1：已到达目标位置 ==================== */
    if (delta == 0) {
        // 速度很小时（在刹车阈值内），直接锁定停止
        if ((s_track_velocity_pos >= -s_speed_locking_brake) && 
            (s_track_velocity_pos <= s_speed_locking_brake)) {
            s_velocity_integral_pos = 0;
            s_track_velocity_pos = 0;
            s_position_integral = 0;
        } 
        // 速度为正，需要减速到0
        else if (s_track_velocity_pos > 0) {
            CalcPositionVelocityIntegral(-s_velocity_down_acc);  // 减速
            if (s_track_velocity_pos <= 0) {  // 已经减到0或以下
                s_velocity_integral_pos = 0;
                s_track_velocity_pos = 0;
            }
        } 
        // 速度为负，需要减速到0
        else if (s_track_velocity_pos < 0) {
            CalcPositionVelocityIntegral(s_velocity_down_acc);   // 减速（反向）
            if (s_track_velocity_pos >= 0) {
                s_velocity_integral_pos = 0;
                s_track_velocity_pos = 0;
            }
        }
    }
    
    /* ==================== 情况2：还需要移动 ==================== */
    else {
        /* ---------- 子情况2.1：当前速度为0（从静止开始加速）---------- */
        if (s_track_velocity_pos == 0) {
            if (delta > 0) {
                CalcPositionVelocityIntegral(s_velocity_up_acc);   // 正向加速
            } else {
                CalcPositionVelocityIntegral(-s_velocity_up_acc);  // 反向加速
            }
        }
        
        /* ---------- 子情况2.2：正向移动中（方向和目标一致）---------- */
        else if ((delta > 0) && (s_track_velocity_pos > 0)) {
            // 检查当前速度是否在限速范围内
            if (s_track_velocity_pos <= g_motion_config->ratedVelocity) {
                // 核心公式：计算从当前速度减到0需要的距离
                // need_down = v2 / (2a)
                int32_t need_down = (int32_t)((float)s_track_velocity_pos * 
                                               (float)s_track_velocity_pos * 
                                               s_quick_velocity_down_acc);
                
                // 判断：剩余距离是否足够减速？
                if (abs(delta) > need_down) {
                    // 距离足够，可以继续加速或保持匀速
                    if (s_track_velocity_pos < g_motion_config->ratedVelocity) {
                        CalcPositionVelocityIntegral(s_velocity_up_acc);  // 继续加速
                        // 限幅：不超过最大速度
                        if (s_track_velocity_pos >= g_motion_config->ratedVelocity) {
                            s_velocity_integral_pos = 0;
                            s_track_velocity_pos = g_motion_config->ratedVelocity;
                        }
                    } else if (s_track_velocity_pos > g_motion_config->ratedVelocity) {
                        CalcPositionVelocityIntegral(-s_velocity_down_acc); // 减速到限速
                    }
                } else {
                    // 距离不够了，必须开始减速！
                    CalcPositionVelocityIntegral(-s_velocity_down_acc);
                    if (s_track_velocity_pos <= 0) {
                        s_velocity_integral_pos = 0;
                        s_track_velocity_pos = 0;
                    }
                }
            } else {
                // 速度超限，强制减速
                CalcPositionVelocityIntegral(-s_velocity_down_acc);
                if (s_track_velocity_pos <= 0) {
                    s_velocity_integral_pos = 0;
                    s_track_velocity_pos = 0;
                }
            }
        }
        
        /* ---------- 子情况2.3：反向移动中（方向和目标一致）---------- */
        else if ((delta < 0) && (s_track_velocity_pos < 0)) {
            // 逻辑与正向对称，方向相反
            if (s_track_velocity_pos >= -g_motion_config->ratedVelocity) {
                int32_t need_down = (int32_t)((float)s_track_velocity_pos * 
                                               (float)s_track_velocity_pos * 
                                               s_quick_velocity_down_acc);
                if (abs(delta) > need_down) {
                    if (s_track_velocity_pos > -g_motion_config->ratedVelocity) {
                        CalcPositionVelocityIntegral(-s_velocity_up_acc);
                        if (s_track_velocity_pos <= -g_motion_config->ratedVelocity) {
                            s_velocity_integral_pos = 0;
                            s_track_velocity_pos = -g_motion_config->ratedVelocity;
                        }
                    } else if (s_track_velocity_pos < -g_motion_config->ratedVelocity) {
                        CalcPositionVelocityIntegral(s_velocity_down_acc);
                    }
                } else {
                    CalcPositionVelocityIntegral(s_velocity_down_acc);
                    if (s_track_velocity_pos >= 0) {
                        s_velocity_integral_pos = 0;
                        s_track_velocity_pos = 0;
                    }
                }
            } else {
                CalcPositionVelocityIntegral(s_velocity_down_acc);
                if (s_track_velocity_pos >= 0) {
                    s_velocity_integral_pos = 0;
                    s_track_velocity_pos = 0;
                }
            }
        }
        
        /* ---------- 子情况2.4：速度方向与目标方向相反 ---------- */
        else if ((delta < 0) && (s_track_velocity_pos > 0)) {
            // 需要反向，但当前正在正向运动 → 先减速到0
            CalcPositionVelocityIntegral(-s_velocity_down_acc);
            if (s_track_velocity_pos <= 0) {
                s_velocity_integral_pos = 0;
                s_track_velocity_pos = 0;
            }
        }
        
        /* ---------- 子情况2.5：速度方向与目标方向相反 ---------- */
        else if ((delta > 0) && (s_track_velocity_pos < 0)) {
            // 需要正向，但当前正在反向运动 → 先减速到0
            CalcPositionVelocityIntegral(s_velocity_down_acc);
            if (s_track_velocity_pos >= 0) {
                s_velocity_integral_pos = 0;
                s_track_velocity_pos = 0;
            }
        }
    }
    
    /* 根据当前速度，更新位置 */
    CalcPositionIntegral(s_track_velocity_pos);
    
    /* 输出规划后的位置和速度 */
    g_go_location = s_track_position;
    g_go_location_velocity = s_track_velocity_pos;
}

/* ==================== PositionInterpolator 实现 ==================== */
void PositionInterpolator_Init(void)
{
    /* Nothing to init */
}

void PositionInterpolator_NewTask(int32_t realPosition, int32_t realVelocity)
{
    s_record_position = realPosition;
    s_record_position_last = realPosition;
    s_est_position = realPosition;
    s_est_velocity_interp = realVelocity;
}
//Step/Dir模式
void PositionInterpolator_CalcSoftGoal(int32_t goalPosition)
{
    s_record_position_last = s_record_position;
    s_record_position = goalPosition;
    
    s_est_position_integral += ((s_record_position - s_record_position_last) * CONTROL_FREQUENCY)
                                + ((s_est_velocity_interp << 6) - s_est_velocity_interp);
    s_est_velocity_interp = s_est_position_integral >> 6;
    s_est_position_integral -= (s_est_velocity_interp << 6);
    
    s_est_position = s_record_position;
    
    g_interp_go_position = s_est_position;
    g_interp_go_velocity = s_est_velocity_interp;
}

/* ==================== TrajectoryTracker 实现 ==================== */
void TrajectoryTracker_Init(int32_t updateTimeout)
{
    TrajectoryTracker_SetSlowDownVelocityAcc(g_motion_config->ratedVelocityAcc);
    s_update_timeout = updateTimeout;
}

void TrajectoryTracker_SetSlowDownVelocityAcc(int32_t value)
{
    s_velocity_down_acc_traj = value;
}

void TrajectoryTracker_NewTask(int32_t realLocation, int32_t realSpeed)
{
    s_update_time = 0;
    s_overtime_flag = false;
    s_dynamic_vel_acc_remainder = 0;
    s_velocity_now = realSpeed;
    s_velocity_now_remainder = 0;
    s_position_now = realLocation;
}

void TrajectoryTracker_CalcSoftGoal(int32_t goalPosition, int32_t goalVelocity)
{
    /* ==================== 第1步：检查目标是否变化 ==================== */
    if (goalVelocity != s_record_velocity || goalPosition != s_record_position_traj) {
        // 目标有变化（收到了新的轨迹指令）
        s_update_time = 0;                      // 重置超时计时器
        s_record_velocity = goalVelocity;       // 记录新目标速度
        s_record_position_traj = goalPosition;  // 记录新目标位置
        
        /**
         * 核心公式：计算需要的加速度
         * 
         * 由运动学公式：v22 - v12 = 2 × a × s
         * 推导出：a = (v22 - v12) / (2 × s)
         * 
         * 代码中用 (v2 + v1)(v2 - v1) 代替 v22 - v12，
         * 避免大数平方导致溢出。
         * 
         * 参数说明：
         *   goalVelocity   = v2（目标速度）
         *   s_velocity_now = v1（当前速度）
         *   goalPosition - s_position_now = s（位移）
         */
        s_dynamic_velocity_acc = (int32_t)((float)(goalVelocity + s_velocity_now) *
                                           (float)(goalVelocity - s_velocity_now) /
                                           (float)(2 * (goalPosition - s_position_now)));
        s_overtime_flag = false;                // 清除超时标志
    } 
    /* ==================== 第2步：目标未变化，检查超时 ==================== */
    else {
        // 长时间没收到新指令，累积超时时间
        if (s_update_time >= (s_update_timeout * 1000)) {
            s_overtime_flag = true;             // 超时！触发安全停车
        } else {
            s_update_time += CONTROL_PERIOD_US; // 累加时间（单位：微秒）
        }
    }
    
    /* ==================== 第3步：根据模式执行运动 ==================== */
    if (s_overtime_flag) {
        /**
         * 超时模式：通信中断，安全停车
         * 
         * 作用：如果上位机长时间没有发送新的轨迹指令，
         *       认为通信可能中断，主动减速到 0。
         */
        if (s_velocity_now == 0) {
            // 已经停止，无事可做
            s_dynamic_vel_acc_remainder = 0;
        } else if (s_velocity_now > 0) {
            // 正向运动 → 减速（负加速度）
            CalcTrajVelocityIntegral(-s_velocity_down_acc_traj);
            if (s_velocity_now <= 0) {
                // 已经减到 0 或以下
                s_dynamic_vel_acc_remainder = 0;
                s_velocity_now = 0;
            }
        } else {
            // 反向运动 → 减速（正加速度，因为速度是负的）
            CalcTrajVelocityIntegral(s_velocity_down_acc_traj);
            if (s_velocity_now >= 0) {
                s_dynamic_vel_acc_remainder = 0;
                s_velocity_now = 0;
            }
        }
    } else {
        /**
         * 正常模式：按计算出的加速度运动
         * 
         * 加速度可能是正（加速）、负（减速）或 0（匀速）
         */
        CalcTrajVelocityIntegral(s_dynamic_velocity_acc);
    }
    
    /* ==================== 第4步：根据速度更新位置 ==================== */
    CalcTrajPositionIntegral(s_velocity_now);
    
    /* ==================== 第5步：输出结果 ==================== */
    g_traj_go_position = s_position_now;   // 规划后的位置
    g_traj_go_velocity = s_velocity_now;   // 规划后的速度
}