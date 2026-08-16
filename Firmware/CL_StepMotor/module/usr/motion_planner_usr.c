/*****************************************************************************
 * @文件: motion_planner_usr.c
 * @作者: cl
 * @日期: 2026-08-16
 * @版本: v1.0
 * @说明: 运动规划用户层实现（复刻参考 motion_planner.c 分层：4 tracker
 *   20kHz 软目标生成；输出收敛为内部变量 + getter，无 extern 全局耦合）
 * @平台: STM32F103RET6
 * @依赖: stdlib.h
 ****************************************************************************/
#include "motion_planner_usr.h"

#include <stdbool.h>
#include <stdlib.h>

/* ==== 全局实例 ==== */
static MotionPlanner_Config_T *s_config = NULL;

/* ---- CurrentTracker ---- */
static int32_t s_current_acc = 0;
static int32_t s_current_integral = 0;
static int32_t s_track_current = 0;
static int32_t s_go_current = 0;

/* ---- VelocityTracker ---- */
static int32_t s_velocity_acc = 0;
static int32_t s_velocity_integral = 0;
static int32_t s_track_velocity = 0;
static int32_t s_go_velocity = 0;

/* ---- PositionTracker ---- */
static int32_t s_velocity_up_acc = 0;
static int32_t s_velocity_down_acc = 0;
static float   s_quick_velocity_down_acc = 0.0f;
static int32_t s_speed_locking_brake = 0;
static int32_t s_velocity_integral_pos = 0;
static int32_t s_track_velocity_pos = 0;
static int32_t s_position_integral = 0;
static int32_t s_track_position = 0;
static int32_t s_go_location = 0;
static int32_t s_go_location_velocity = 0;

/* ---- TrajectoryTracker ---- */
static int32_t s_velocity_down_acc_traj = 0;
static int32_t s_dynamic_velocity_acc = 0;
static int32_t s_update_time = 0;
static int32_t s_update_timeout = 200;
static bool     s_overtime_flag = false;
static int32_t s_record_velocity = 0;
static int32_t s_record_position_traj = 0;
static int32_t s_dynamic_vel_acc_remainder = 0;
static int32_t s_velocity_now = 0;
static int32_t s_velocity_now_remainder = 0;
static int32_t s_position_now = 0;
static int32_t s_go_traj_position = 0;
static int32_t s_go_traj_velocity = 0;

/* ==== 内部工具 ==== */
/* 电流积分器：余量保留按 CONTROL_FREQUENCY 拆分整数步进（防逐帧截断损失） */
static void S_CalcCurrentIntegral(int32_t current)
{
    s_current_integral += current;
    s_track_current += s_current_integral / USR_MOTION_PLANNER_CTRL_FREQ;
    s_current_integral = s_current_integral % USR_MOTION_PLANNER_CTRL_FREQ;
}

/* 速度积分器（同电流积分器，梯形规划步进） */
static void S_CalcVelocityIntegral(int32_t velocity)
{
    s_velocity_integral += velocity;
    s_track_velocity += s_velocity_integral / USR_MOTION_PLANNER_CTRL_FREQ;
    s_velocity_integral = s_velocity_integral % USR_MOTION_PLANNER_CTRL_FREQ;
}

/* 位置速度积分器（S 曲线速度轨迹步进） */
static void S_CalcPositionVelocityIntegral(int32_t value)
{
    s_velocity_integral_pos += value;
    s_track_velocity_pos += s_velocity_integral_pos / USR_MOTION_PLANNER_CTRL_FREQ;
    s_velocity_integral_pos = s_velocity_integral_pos % USR_MOTION_PLANNER_CTRL_FREQ;
}

/* 位置积分器（速度→位置） */
static void S_CalcPositionIntegral(int32_t value)
{
    s_position_integral += value;
    s_track_position += s_position_integral / USR_MOTION_PLANNER_CTRL_FREQ;
    s_position_integral = s_position_integral % USR_MOTION_PLANNER_CTRL_FREQ;
}

/* 轨迹速度积分器（动态加速度步进） */
static void S_CalcTrajVelocityIntegral(int32_t value)
{
    s_dynamic_vel_acc_remainder += value;
    s_velocity_now += s_dynamic_vel_acc_remainder / USR_MOTION_PLANNER_CTRL_FREQ;
    s_dynamic_vel_acc_remainder = s_dynamic_vel_acc_remainder % USR_MOTION_PLANNER_CTRL_FREQ;
}

/* 轨迹位置积分器（速度→位置） */
static void S_CalcTrajPositionIntegral(int32_t value)
{
    s_velocity_now_remainder += value;
    s_position_now += s_velocity_now_remainder / USR_MOTION_PLANNER_CTRL_FREQ;
    s_velocity_now_remainder = s_velocity_now_remainder % USR_MOTION_PLANNER_CTRL_FREQ;
}

/* ==== 接口实现 ==== */

/**
 * @ 输入 config: 规划配置指针
 * @ 输出 无
 * @ 说明 注入规划配置（仅存指针；Init 前必须调用）
 */
void USR_MotionPlanner_SetConfig(MotionPlanner_Config_T *config)
{
    s_config = config;
}

/* ---- CurrentTracker ---- */

/**
 * @ 输入 无
 * @ 输出 无
 * @ 说明 初始化电流规划器：加速度取配置 ratedCurrentAcc
 */
void USR_MotionPlanner_CurrentTracker_Init(void)
{
    if (NULL == s_config)
    {
        return;
    }
    USR_MotionPlanner_CurrentTracker_SetCurrentAcc(s_config->ratedCurrentAcc);
}

/**
 * @ 输入 current_acc: 电流加速度（mA/s）
 * @ 输出 无
 * @ 说明 设置电流梯形斜坡加速度
 */
void USR_MotionPlanner_CurrentTracker_SetCurrentAcc(int32_t current_acc)
{
    s_current_acc = current_acc;
}

/**
 * @ 输入 real_current: 当前实际电流（mA）
 * @ 输出 无
 * @ 说明 新任务：清积分，软目标从当前电流出发
 */
void USR_MotionPlanner_CurrentTracker_NewTask(int32_t real_current)
{
    s_current_integral = 0;
    s_track_current = real_current;
}

/**
 * @ 输入 goal_current: 目标电流（mA，外部已限幅）
 * @ 输出 无
 * @ 说明 电流梯形平滑规划：以 s_current_acc 斜率爬升/下降，穿越 0 点归零
 */
void USR_MotionPlanner_CurrentTracker_CalcSoftGoal(int32_t goal_current)
{
    int32_t delta = goal_current - s_track_current;

    if (delta == 0)
    {
        s_track_current = goal_current;
    }
    else if (delta > 0)
    {
        if (s_track_current >= 0)
        {
            S_CalcCurrentIntegral(s_current_acc);
            if (s_track_current >= goal_current)
            {
                s_current_integral = 0;
                s_track_current = goal_current;
            }
        }
        else
        {
            S_CalcCurrentIntegral(s_current_acc);
            if (s_track_current >= 0)
            {
                s_current_integral = 0;
                s_track_current = 0;
            }
        }
    }
    else
    {
        if (s_track_current <= 0)
        {
            S_CalcCurrentIntegral(-s_current_acc);
            if (s_track_current <= goal_current)
            {
                s_current_integral = 0;
                s_track_current = goal_current;
            }
        }
        else
        {
            S_CalcCurrentIntegral(-s_current_acc);
            if (s_track_current <= 0)
            {
                s_current_integral = 0;
                s_track_current = 0;
            }
        }
    }

    s_go_current = s_track_current;
}

/**
 * @ 输入 无
 * @ 输出 规划后电流软目标（mA）
 * @ 说明 电流规划输出 getter
 */
int32_t USR_MotionPlanner_CurrentTracker_GetGoCurrent(void)
{
    return s_go_current;
}

/* ---- VelocityTracker ---- */

/**
 * @ 输入 无
 * @ 输出 无
 * @ 说明 初始化速度规划器：加速度取配置 ratedVelocityAcc
 */
void USR_MotionPlanner_VelocityTracker_Init(void)
{
    if (NULL == s_config)
    {
        return;
    }
    USR_MotionPlanner_VelocityTracker_SetVelocityAcc(s_config->ratedVelocityAcc);
}

/**
 * @ 输入 velocity_acc: 速度加速度（细分步/s²）
 * @ 输出 无
 * @ 说明 设置速度梯形斜坡加速度
 */
void USR_MotionPlanner_VelocityTracker_SetVelocityAcc(int32_t velocity_acc)
{
    s_velocity_acc = velocity_acc;
}

/**
 * @ 输入 real_velocity: 当前实际速度（细分步/s）
 * @ 输出 无
 * @ 说明 新任务：清积分，软目标从当前速度出发
 */
void USR_MotionPlanner_VelocityTracker_NewTask(int32_t real_velocity)
{
    s_velocity_integral = 0;
    s_track_velocity = real_velocity;
}

/**
 * @ 输入 goal_velocity: 目标速度（细分步/s，外部已限幅）
 * @ 输出 无
 * @ 说明 速度梯形平滑规划：以 s_velocity_acc 斜率爬升/下降，穿越 0 点归零
 */
void USR_MotionPlanner_VelocityTracker_CalcSoftGoal(int32_t goal_velocity)
{
    int32_t delta = goal_velocity - s_track_velocity;

    if (delta == 0)
    {
        s_track_velocity = goal_velocity;
    }
    else if (delta > 0)
    {
        if (s_track_velocity >= 0)
        {
            S_CalcVelocityIntegral(s_velocity_acc);
            if (s_track_velocity >= goal_velocity)
            {
                s_velocity_integral = 0;
                s_track_velocity = goal_velocity;
            }
        }
        else
        {
            S_CalcVelocityIntegral(s_velocity_acc);
            if (s_track_velocity >= 0)
            {
                s_velocity_integral = 0;
                s_track_velocity = 0;
            }
        }
    }
    else
    {
        if (s_track_velocity <= 0)
        {
            S_CalcVelocityIntegral(-s_velocity_acc);
            if (s_track_velocity <= goal_velocity)
            {
                s_velocity_integral = 0;
                s_track_velocity = goal_velocity;
            }
        }
        else
        {
            S_CalcVelocityIntegral(-s_velocity_acc);
            if (s_track_velocity <= 0)
            {
                s_velocity_integral = 0;
                s_track_velocity = 0;
            }
        }
    }

    s_go_velocity = s_track_velocity;
}

/**
 * @ 输入 无
 * @ 输出 规划后速度软目标（细分步/s）
 * @ 说明 速度规划输出 getter
 */
int32_t USR_MotionPlanner_VelocityTracker_GetGoVelocity(void)
{
    return s_go_velocity;
}

/* ---- PositionTracker ---- */

/**
 * @ 输入 无
 * @ 输出 无
 * @ 说明 初始化位置规划器：加速度取配置 ratedVelocityAcc；
 *   刹车锁定阈值 = 加速度/1000（参考推导，速度小于该值直接锁定 0）
 */
void USR_MotionPlanner_PositionTracker_Init(void)
{
    if (NULL == s_config)
    {
        return;
    }
    USR_MotionPlanner_PositionTracker_SetVelocityAcc(s_config->ratedVelocityAcc);
    s_speed_locking_brake = s_config->ratedVelocityAcc / 1000;
}

/**
 * @ 输入 value: 位置规划加速度（细分步/s²）
 * @ 输出 无
 * @ 说明 设置 S 曲线加速/减速斜率；quick_velocity_down_acc = 0.5/a
 *   （need_down = v²/(2a) 的预计算系数，避免每帧除法）
 */
void USR_MotionPlanner_PositionTracker_SetVelocityAcc(int32_t value)
{
    s_velocity_up_acc = value;
    s_velocity_down_acc = value;
    s_quick_velocity_down_acc = 0.5f / (float)s_velocity_down_acc;
}

/**
 * @ 输入 real_location: 当前实际位置（细分步）
 *        real_speed:    当前实际速度（细分步/s）
 * @ 输出 无
 * @ 说明 新任务：清积分，软目标从当前位置/速度出发
 *   【变更点 2026-08-16】参考源码 NewTask 参数交叉赋值
 *   （s_track_velocity_pos=realLocation / s_track_position=realSpeed ，
 *   初"速度"=位置值、初"位置"=速度值 → 输出相对位移轨迹 + 位置环首帧
 *   反冲），本复刻修正为语义正确：初速度=real_speed、初位置=real_location
 */
void USR_MotionPlanner_PositionTracker_NewTask(int32_t real_location,
                                               int32_t real_speed)
{
    s_velocity_integral_pos = 0;
    s_track_velocity_pos = real_speed;
    s_position_integral = 0;
    s_track_position = real_location;
}

/**
 * @ 输入 goal_position: 目标位置（细分步）
 * @ 输出 无
 * @ 说明 位置 S 曲线规划：梯形（加速→匀速→减速）+ v²/(2a) 提前减速判据 +
 *   反向先归零 + 到位刹车锁定。输出位置/速度软目标
 */
void USR_MotionPlanner_PositionTracker_CalcSoftGoal(int32_t goal_position)
{
    int32_t delta = goal_position - s_track_position;  /* 剩余距离 */

    /* ============ 情况1：已到达目标位置 ============ */
    if (delta == 0)
    {
        /* 速度在刹车阈值内 → 直接锁定停止 */
        if ((s_track_velocity_pos >= -s_speed_locking_brake) &&
            (s_track_velocity_pos <= s_speed_locking_brake))
        {
            s_velocity_integral_pos = 0;
            s_track_velocity_pos = 0;
            s_position_integral = 0;
        }
        /* 速度为正 → 减速到 0 */
        else if (s_track_velocity_pos > 0)
        {
            S_CalcPositionVelocityIntegral(-s_velocity_down_acc);
            if (s_track_velocity_pos <= 0)
            {
                s_velocity_integral_pos = 0;
                s_track_velocity_pos = 0;
            }
        }
        /* 速度为负 → 减速到 0 */
        else if (s_track_velocity_pos < 0)
        {
            S_CalcPositionVelocityIntegral(s_velocity_down_acc);
            if (s_track_velocity_pos >= 0)
            {
                s_velocity_integral_pos = 0;
                s_track_velocity_pos = 0;
            }
        }
    }

    /* ============ 情况2：还需要移动 ============ */
    else
    {
        /* ---- 子情况2.1：当前速度为 0（从静止开始加速） ---- */
        if (s_track_velocity_pos == 0)
        {
            if (delta > 0)
            {
                S_CalcPositionVelocityIntegral(s_velocity_up_acc);
            }
            else
            {
                S_CalcPositionVelocityIntegral(-s_velocity_up_acc);
            }
        }

        /* ---- 子情况2.2：正向移动中（方向和目标一致） ---- */
        else if ((delta > 0) && (s_track_velocity_pos > 0))
        {
            if (s_track_velocity_pos <= s_config->ratedVelocity)
            {
                /* 核心公式：从当前速度减到 0 需要的距离 need_down = v²/(2a) */
                int32_t need_down = (int32_t)((float)s_track_velocity_pos *
                                              (float)s_track_velocity_pos *
                                              s_quick_velocity_down_acc);
                if (abs(delta) > need_down)
                {
                    /* 距离足够 → 继续加速或保持匀速 */
                    if (s_track_velocity_pos < s_config->ratedVelocity)
                    {
                        S_CalcPositionVelocityIntegral(s_velocity_up_acc);
                        if (s_track_velocity_pos >= s_config->ratedVelocity)
                        {
                            s_velocity_integral_pos = 0;
                            s_track_velocity_pos = s_config->ratedVelocity;
                        }
                    }
                    else if (s_track_velocity_pos > s_config->ratedVelocity)
                    {
                        S_CalcPositionVelocityIntegral(-s_velocity_down_acc);
                    }
                }
                else
                {
                    /* 距离不够 → 必须开始减速 */
                    S_CalcPositionVelocityIntegral(-s_velocity_down_acc);
                    if (s_track_velocity_pos <= 0)
                    {
                        s_velocity_integral_pos = 0;
                        s_track_velocity_pos = 0;
                    }
                }
            }
            else
            {
                /* 速度超限 → 强制减速 */
                S_CalcPositionVelocityIntegral(-s_velocity_down_acc);
                if (s_track_velocity_pos <= 0)
                {
                    s_velocity_integral_pos = 0;
                    s_track_velocity_pos = 0;
                }
            }
        }

        /* ---- 子情况2.3：反向移动中（方向和目标一致） ---- */
        else if ((delta < 0) && (s_track_velocity_pos < 0))
        {
            /* 逻辑与正向对称，方向相反 */
            if (s_track_velocity_pos >= -s_config->ratedVelocity)
            {
                int32_t need_down = (int32_t)((float)s_track_velocity_pos *
                                              (float)s_track_velocity_pos *
                                              s_quick_velocity_down_acc);
                if (abs(delta) > need_down)
                {
                    if (s_track_velocity_pos > -s_config->ratedVelocity)
                    {
                        S_CalcPositionVelocityIntegral(-s_velocity_up_acc);
                        if (s_track_velocity_pos <= -s_config->ratedVelocity)
                        {
                            s_velocity_integral_pos = 0;
                            s_track_velocity_pos = -s_config->ratedVelocity;
                        }
                    }
                    else if (s_track_velocity_pos < -s_config->ratedVelocity)
                    {
                        S_CalcPositionVelocityIntegral(s_velocity_down_acc);
                    }
                }
                else
                {
                    S_CalcPositionVelocityIntegral(s_velocity_down_acc);
                    if (s_track_velocity_pos >= 0)
                    {
                        s_velocity_integral_pos = 0;
                        s_track_velocity_pos = 0;
                    }
                }
            }
            else
            {
                S_CalcPositionVelocityIntegral(s_velocity_down_acc);
                if (s_track_velocity_pos >= 0)
                {
                    s_velocity_integral_pos = 0;
                    s_track_velocity_pos = 0;
                }
            }
        }

        /* ---- 子情况2.4：需要反向，但当前正在正向运动 → 先减速到 0 ---- */
        else if ((delta < 0) && (s_track_velocity_pos > 0))
        {
            S_CalcPositionVelocityIntegral(-s_velocity_down_acc);
            if (s_track_velocity_pos <= 0)
            {
                s_velocity_integral_pos = 0;
                s_track_velocity_pos = 0;
            }
        }

        /* ---- 子情况2.5：需要正向，但当前正在反向运动 → 先减速到 0 ---- */
        else if ((delta > 0) && (s_track_velocity_pos < 0))
        {
            S_CalcPositionVelocityIntegral(s_velocity_down_acc);
            if (s_track_velocity_pos >= 0)
            {
                s_velocity_integral_pos = 0;
                s_track_velocity_pos = 0;
            }
        }
    }

    /* 根据当前速度更新位置 */
    S_CalcPositionIntegral(s_track_velocity_pos);

    /* 输出规划后的位置和速度 */
    s_go_location = s_track_position;
    s_go_location_velocity = s_track_velocity_pos;
}

/**
 * @ 输入 无
 * @ 输出 规划后位置软目标（细分步）
 * @ 说明 位置规划输出 getter
 */
int32_t USR_MotionPlanner_PositionTracker_GetGoLocation(void)
{
    return s_go_location;
}

/**
 * @ 输入 无
 * @ 输出 规划后速度软目标（细分步/s）
 * @ 说明 位置规划速度输出 getter
 */
int32_t USR_MotionPlanner_PositionTracker_GetGoLocationVelocity(void)
{
    return s_go_location_velocity;
}

/* ---- TrajectoryTracker ---- */

/**
 * @ 输入 update_timeout: 指令超时（ms），超时触发安全停车
 * @ 输出 无
 * @ 说明 初始化轨迹规划器：减速加速度取配置 ratedVelocityAcc，超时取入参
 */
void USR_MotionPlanner_TrajectoryTracker_Init(int32_t update_timeout)
{
    if (NULL == s_config)
    {
        return;
    }
    USR_MotionPlanner_TrajectoryTracker_SetSlowDownVelocityAcc(
        s_config->ratedVelocityAcc);
    s_update_timeout = update_timeout;
}

/**
 * @ 输入 value: 超时停车减速加速度（细分步/s²）
 * @ 输出 无
 * @ 说明 设置超时安全停车用的减速加速度
 */
void USR_MotionPlanner_TrajectoryTracker_SetSlowDownVelocityAcc(int32_t value)
{
    s_velocity_down_acc_traj = value;
}

/**
 * @ 输入 real_location: 当前实际位置（细分步）
 *        real_speed:    当前实际速度（细分步/s）
 * @ 输出 无
 * @ 说明 新任务：清超时/积分，从当前位置/速度出发
 */
void USR_MotionPlanner_TrajectoryTracker_NewTask(int32_t real_location,
                                                 int32_t real_speed)
{
    s_update_time = 0;
    s_overtime_flag = false;
    s_dynamic_vel_acc_remainder = 0;
    s_velocity_now = real_speed;
    s_velocity_now_remainder = 0;
    s_position_now = real_location;
}

/**
 * @ 输入 goal_position: 目标位置（细分步）
 *        goal_velocity: 目标速度（细分步/s）
 * @ 输出 无
 * @ 说明 轨迹动态加速度规划：目标变化时按 v2²-v1²=2as 求恒定加速度插值；
 *   目标长时间不变 → 判定通信中断 → 安全减速停车
 */
void USR_MotionPlanner_TrajectoryTracker_CalcSoftGoal(int32_t goal_position,
                                                      int32_t goal_velocity)
{
    /* ============ 第1步：检查目标是否变化 ============ */
    if (goal_velocity != s_record_velocity ||
        goal_position != s_record_position_traj)
    {
        /* 目标变化（收到新轨迹指令） */
        s_update_time = 0;
        s_record_velocity = goal_velocity;
        s_record_position_traj = goal_position;

        /* 核心公式：a = (v2²-v1²)/(2s)，用 (v2+v1)(v2-v1) 避免大数平方溢出
         *   goalVelocity = v2，s_velocity_now = v1，
         *   goalPosition - s_position_now = s */
        if (goal_position != s_position_now)
        {
            s_dynamic_velocity_acc =
                (int32_t)((float)(goal_velocity + s_velocity_now) *
                          (float)(goal_velocity - s_velocity_now) /
                          (float)(2 * (goal_position - s_position_now)));
        }
        else
        {
            /* 【变更点 2026-08-16】目标=当前位置时参考源码除零 UB，
             * 本复刻加保护：位移为 0 → 无加速需求 */
            s_dynamic_velocity_acc = 0;
        }
        s_overtime_flag = false;
    }
    /* ============ 第2步：目标未变化，检查超时 ============ */
    else
    {
        if (s_update_time >= (s_update_timeout * 1000))
        {
            s_overtime_flag = true;   /* 超时 → 触发安全停车 */
        }
        else
        {
            s_update_time += USR_MOTION_PLANNER_CTRL_US;
        }
    }

    /* ============ 第3步：根据模式执行运动 ============ */
    if (s_overtime_flag)
    {
        /* 超时模式：通信中断 → 安全减速到 0 */
        if (s_velocity_now == 0)
        {
            s_dynamic_vel_acc_remainder = 0;
        }
        else if (s_velocity_now > 0)
        {
            S_CalcTrajVelocityIntegral(-s_velocity_down_acc_traj);
            if (s_velocity_now <= 0)
            {
                s_dynamic_vel_acc_remainder = 0;
                s_velocity_now = 0;
            }
        }
        else
        {
            S_CalcTrajVelocityIntegral(s_velocity_down_acc_traj);
            if (s_velocity_now >= 0)
            {
                s_dynamic_vel_acc_remainder = 0;
                s_velocity_now = 0;
            }
        }
    }
    else
    {
        /* 正常模式：按计算出的加速度运动 */
        S_CalcTrajVelocityIntegral(s_dynamic_velocity_acc);
    }

    /* ============ 第4步：根据速度更新位置 ============ */
    S_CalcTrajPositionIntegral(s_velocity_now);

    /* ============ 第5步：输出结果 ============ */
    s_go_traj_position = s_position_now;
    s_go_traj_velocity = s_velocity_now;
}

/**
 * @ 输入 无
 * @ 输出 规划后轨迹位置（细分步）
 * @ 说明 轨迹规划位置输出 getter
 */
int32_t USR_MotionPlanner_TrajectoryTracker_GetGoTrajPosition(void)
{
    return s_go_traj_position;
}

/**
 * @ 输入 无
 * @ 输出 规划后轨迹速度（细分步/s）
 * @ 说明 轨迹规划速度输出 getter
 */
int32_t USR_MotionPlanner_TrajectoryTracker_GetGoTrajVelocity(void)
{
    return s_go_traj_velocity;
}
