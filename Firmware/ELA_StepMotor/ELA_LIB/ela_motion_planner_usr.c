/********
 * @ 文件: ela_motion_planner_usr.c
 * @ 作者: ELACO
 * @ 日期: 2026-08-02
 * @ 版本: 1.0.0
 * @ 说明: 运动轨迹规划器（复刻参考 zhjStepMotor motion_planner.c）。
 *         五类软目标规划器，输出全局 g_go_* 软目标供 DCE/PID 闭环使用。
 *         所有积分用 /CONTROL_FREQUENCY 拆商余，避免浮点与溢出
 * @ 依赖: ela_motion_planner_usr.h
 ********/

#include "ela_motion_planner_usr.h"
#include <stdlib.h>
#include <stdio.h>
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

/* ==================== 积分累加器 ==================== */

/********
 * @ 输入: current: 电流变化率（mA/20kHz tick 累加）
 * @ 说明: 电流斜坡积分。s_current_integral 累积商余，
 *         每 CONTROL_FREQUENCY 累加 1 个单位到 s_track_current
 ********/
static void CalcCurrentIntegral(int32_t current)
{
    s_current_integral += current;
    s_track_current += s_current_integral / CONTROL_FREQUENCY;
    s_current_integral = s_current_integral % CONTROL_FREQUENCY;
}

/********
 * @ 输入: velocity: 速度变化率（步/20kHz tick 累加）
 * @ 说明: 速度斜坡积分
 ********/
static void CalcVelocityIntegral(int32_t velocity)
{
    s_velocity_integral += velocity;
    s_track_velocity += s_velocity_integral / CONTROL_FREQUENCY;
    s_velocity_integral = s_velocity_integral % CONTROL_FREQUENCY;
}

/********
 * @ 输入: value: 位置跟踪速度变化率
 * @ 说明: 位置跟踪器速度斜坡积分
 ********/
static void CalcPositionVelocityIntegral(int32_t value)
{
    s_velocity_integral_pos += value;
    s_track_velocity_pos += s_velocity_integral_pos / CONTROL_FREQUENCY;
    s_velocity_integral_pos = s_velocity_integral_pos % CONTROL_FREQUENCY;
}

/********
 * @ 输入: value: 位置变化率
 * @ 说明: 位置跟踪器位置积分
 ********/
static void CalcPositionIntegral(int32_t value)
{
    s_position_integral += value;
    s_track_position += s_position_integral / CONTROL_FREQUENCY;
    s_position_integral = s_position_integral % CONTROL_FREQUENCY;
}

/********
 * @ 输入: value: 轨迹速度变化率
 * @ 说明: 轨迹跟踪器速度积分
 ********/
static void CalcTrajVelocityIntegral(int32_t value)
{
    s_dynamic_vel_acc_remainder += value;
    s_velocity_now += s_dynamic_vel_acc_remainder / CONTROL_FREQUENCY;
    s_dynamic_vel_acc_remainder = s_dynamic_vel_acc_remainder % CONTROL_FREQUENCY;
}

/********
 * @ 输入: value: 轨迹速度（步/s）
 * @ 说明: 轨迹跟踪器位置积分
 ********/
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

/********
 * @ 输入: goalCurrent: 目标电流（mA）
 * @ 说明: 电流斜坡平滑规划器。以固定 s_current_acc 变化率
 *         趋近目标，穿越 0 时按 0 处理
 ********/
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

/********
 * @ 输入: goalVelocity: 目标速度（步/s）
 * @ 说明: 速度斜坡平滑规划器
 ********/
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

/********
 * @ 输入: goalPosition: 目标位置（微步）
 * @ 说明: 位置 S 型平滑规划器。梯形速度曲线：先加速到额定
 *         速度，按 ½v²/a 提前减速，最终锁速刹车归零。
 *         速度用 ratedVelocityAcc 斜坡，位置对速度积分
 ********/
void PositionTracker_CalcSoftGoal(int32_t goalPosition)
{
    int32_t delta = goalPosition - s_track_position;  /* 剩余距离 */

    /* ==================== 情形1：已到目标位置 ==================== */
    if (delta == 0) {
        /* 速度很小时在锁速制动值内，直接归零停止 */
        if ((s_track_velocity_pos >= -s_speed_locking_brake) &&
            (s_track_velocity_pos <= s_speed_locking_brake)) {
            s_velocity_integral_pos = 0;
            s_track_velocity_pos = 0;
            s_position_integral = 0;
        }
        /* 速度为正在正向运动，需减速到0 */
        else if (s_track_velocity_pos > 0) {
            CalcPositionVelocityIntegral(-s_velocity_down_acc);  /* 减速 */
            if (s_track_velocity_pos <= 0) {  /* 已经减到0或越过 */
                s_velocity_integral_pos = 0;
                s_track_velocity_pos = 0;
            }
        }
        /* 速度为正在反向运动，需减速到0 */
        else if (s_track_velocity_pos < 0) {
            CalcPositionVelocityIntegral(s_velocity_down_acc);   /* 减速，反向 */
            if (s_track_velocity_pos >= 0) {
                s_velocity_integral_pos = 0;
                s_track_velocity_pos = 0;
            }
        }
    }

    /* ==================== 情形2：还需要移动 ==================== */
    else {
        /* ---------- 情形2.1：当前速度为0，从静止开始加速 ---------- */
        if (s_track_velocity_pos == 0) {
            if (delta > 0) {
                CalcPositionVelocityIntegral(s_velocity_up_acc);   /* 正向加速 */
            } else {
                CalcPositionVelocityIntegral(-s_velocity_up_acc);  /* 反向加速 */
            }
        }

        /* ---------- 情形2.2：正向移动中，且目标一致 ---------- */
        else if ((delta > 0) && (s_track_velocity_pos > 0)) {
            /* 检查当前速度是否在加速范围内 */
            if (s_track_velocity_pos <= g_motion_config->ratedVelocity) {
                /* 减速公式：need_down = v2 / (2a) */
                int32_t need_down = (int32_t)((float)s_track_velocity_pos *
                                               (float)s_track_velocity_pos *
                                               s_quick_velocity_down_acc);

                /* 判断：剩余距离是否足够减速 */
                if (abs(delta) > need_down) {
                    /* 距离足够，可以继续加速或保持速度 */
                    if (s_track_velocity_pos < g_motion_config->ratedVelocity) {
                        CalcPositionVelocityIntegral(s_velocity_up_acc);  /* 继续加速 */
                        /* 限幅不能超过额定速度 */
                        if (s_track_velocity_pos >= g_motion_config->ratedVelocity) {
                            s_velocity_integral_pos = 0;
                            s_track_velocity_pos = g_motion_config->ratedVelocity;
                        }
                    } else if (s_track_velocity_pos > g_motion_config->ratedVelocity) {
                        CalcPositionVelocityIntegral(-s_velocity_down_acc); /* 减速到额定 */
                    }
                } else {
                    /* 距离不够了，必须开始减速 */
                    CalcPositionVelocityIntegral(-s_velocity_down_acc);
                    if (s_track_velocity_pos <= 0) {
                        s_velocity_integral_pos = 0;
                        s_track_velocity_pos = 0;
                    }
                }
            } else {
                /* 速度超限，强制减速 */
                CalcPositionVelocityIntegral(-s_velocity_down_acc);
                if (s_track_velocity_pos <= 0) {
                    s_velocity_integral_pos = 0;
                    s_track_velocity_pos = 0;
                }
            }
        }

        /* ---------- 情形2.3：反向移动中，且目标一致 ---------- */
        else if ((delta < 0) && (s_track_velocity_pos < 0)) {
            /* 逻辑与正向对称，方向相反 */
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

        /* ---------- 情形2.4：速度方向与目标方向相反 ---------- */
        else if ((delta < 0) && (s_track_velocity_pos > 0)) {
            /* 需要反向，但当前在正向运动 → 先减速到0 */
            CalcPositionVelocityIntegral(-s_velocity_down_acc);
            if (s_track_velocity_pos <= 0) {
                s_velocity_integral_pos = 0;
                s_track_velocity_pos = 0;
            }
        }

        /* ---------- 情形2.5：速度方向与目标方向相反 ---------- */
        else if ((delta > 0) && (s_track_velocity_pos < 0)) {
            /* 需要正向，但当前在反向运动 → 先减速到0 */
            CalcPositionVelocityIntegral(s_velocity_down_acc);
            if (s_track_velocity_pos >= 0) {
                s_velocity_integral_pos = 0;
                s_track_velocity_pos = 0;
            }
        }
    }

    /* 根据当前速度，累加位置 */
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

/********
 * @ 输入: goalPosition: 目标位置（微步）
 * @ 说明: Step/Dir 模式位置插值。记录目标位置变化，一阶低通
 *         估计速度，输出平滑的目标位置与速度
 ********/
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

/********
 * @ 输入: goalPosition: 目标位置（微步）
 *         goalVelocity: 目标速度（步/s）
 * @ 说明: 轨迹跟踪器。目标变化时用运动学公式 v2²-v1²=2as 反推
 *         加速度；目标长时间未变则视为超时，减速到 0
 ********/
void TrajectoryTracker_CalcSoftGoal(int32_t goalPosition, int32_t goalVelocity)
{
    /* ==================== 第1步：判断目标是否变化 ==================== */
    if (goalVelocity != s_record_velocity || goalPosition != s_record_position_traj) {
        /* 目标有变化（收到了新的轨迹指令） */
        s_update_time = 0;                      /* 重置超时计时 */
        s_record_velocity = goalVelocity;       /* 记录目标速度 */
        s_record_position_traj = goalPosition;  /* 记录目标位置 */

        /* 运动学公式 v2²-v1²=2·a·s，反推需要的加速度 */
        s_dynamic_velocity_acc = (int32_t)((float)(goalVelocity + s_velocity_now) *
                                           (float)(goalVelocity - s_velocity_now) /
                                           (float)(2 * (goalPosition - s_position_now)));
        s_overtime_flag = false;                /* 清除超时标志 */
    }
    /* ==================== 第2步：目标未变化则检查超时 ==================== */
    else {
        /* 超时还没有收到指令，累积超时时间 */
        if (s_update_time >= (s_update_timeout * 1000)) {
            s_overtime_flag = true;             /* 超时，强制全速停车 */
        } else {
            s_update_time += CONTROL_PERIOD_US; /* 累计时间（单位微秒） */
        }
    }

    /* ==================== 第3步：按不同模式执行运动 ==================== */
    if (s_overtime_flag) {
        /* 超时模式：通过减速，强制停车 */
        if (s_velocity_now == 0) {
            /* 已经停止，允许重新开始 */
            s_dynamic_vel_acc_remainder = 0;
        } else if (s_velocity_now > 0) {
            /* 正向运动 → 减速（减小速度） */
            CalcTrajVelocityIntegral(-s_velocity_down_acc_traj);
            if (s_velocity_now <= 0) {
                /* 已经减到 0 或越过 */
                s_dynamic_vel_acc_remainder = 0;
                s_velocity_now = 0;
            }
        } else {
            /* 反向运动 → 减速（增加速度，因为速度是负的） */
            CalcTrajVelocityIntegral(s_velocity_down_acc_traj);
            if (s_velocity_now >= 0) {
                s_dynamic_vel_acc_remainder = 0;
                s_velocity_now = 0;
            }
        }
    } else {
        /* 正常模式：按计算的加速度运动 */
        CalcTrajVelocityIntegral(s_dynamic_velocity_acc);
    }

    /* ==================== 第4步：根据速度更新位置 ==================== */
    CalcTrajPositionIntegral(s_velocity_now);

    /* ==================== 第5步：输出结果 ==================== */
    g_traj_go_position = s_position_now;   /* 规划后的位置 */
    g_traj_go_velocity = s_velocity_now;   /* 规划后的速度 */
}
