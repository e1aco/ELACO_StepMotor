/********
 * @ 文件: test_pid.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-30
 * @ 版本: 2.0.0
 * @ 说明: 级联 PID 位置闭环（位置环 → 速度环 → FOC 电流）
 *         外环: PID 位置误差 → 目标速度
 *         内环: PID 速度误差 → 目标电流
 *         前级: 梯形加减速规划器
 * @ 注意: 与 test_position.c 冲突（均定义中断回调），编译时只保留一个
 ********/

#include "elaco_main.h"
#include "test_pid.h"
#include "ela_tb67h450_usr.h"
#include "ela_mt6816_usr.h"
#include "ela_uart_usr.h"
#include "tim.h"
#include <stdio.h>
#include <stdlib.h>

/********
 * @ 说明: 电机参数
 ********/
#define POLE_PAIRS         50
#define MICROSTEP_LAP      (POLE_PAIRS * 1024)
#define ENC_RESOLUTION     16384

/********
 * @ 说明: 级联 PID 参数（位置环 → 速度环）
 ********/
#define POS_PID_KP         8
#define POS_PID_KI         1
#define POS_PID_KD         0
#define VEL_PID_KP         2
#define VEL_PID_KI         30
#define VEL_PID_KD         0
#define RATED_CURRENT_MA   2000
#define POSITION_TOLERANCE 100

/********
 * @ 说明: 运动规划器参数
 ********/
#define RATED_VELOCITY     1000000
#define RATED_VEL_ACC      50000
#define CONTROL_FREQ       20000

/* 控制状态 */
static volatile signed long s_goal = 0;
static volatile signed long s_cur_pos = 0;
static volatile unsigned char s_phase = 0;
static volatile unsigned char s_next = 0;
static unsigned long s_hold = 0;

/* 位置环 PID */
static signed long pos_kp = POS_PID_KP;
static signed long pos_ki = POS_PID_KI;
static signed long pos_kd = POS_PID_KD;
static signed long pos_err_last = 0;
static signed long pos_itg_rd = 0;
static signed long pos_itg = 0;

/* 速度环 PID */
static signed long vel_kp = VEL_PID_KP;
static signed long vel_ki = VEL_PID_KI;
static signed long vel_kd = VEL_PID_KD;
static signed long vel_err_last = 0;
static signed long vel_itg_rd = 0;
static signed long vel_itg = 0;

/* 级联中间量 */
static volatile signed long s_target_vel = 0;

/* 增量追踪 */
static signed long s_enc_last = 0;
static unsigned char s_first = 1;

/* 速度估计 */
static signed long s_vel = 0;
static signed long s_vel_itg = 0;

/* 梯形运动规划器 */
static signed long s_plan_pos = 0;
static signed long s_plan_vel = 0;
static signed long s_plan_vel_itg = 0;
static signed long s_plan_pos_itg = 0;

/* test_pid hlp start */

/********
 * @ 说明: 规划器速度积分（梯形加减速用）
 * @ 输入: acc（微步/秒²）
 ********/
static void test_pid_int_vel(signed long acc)
{
    s_plan_vel_itg += acc;
    s_plan_vel += s_plan_vel_itg / CONTROL_FREQ;
    s_plan_vel_itg = s_plan_vel_itg % CONTROL_FREQ;
}

/********
 * @ 说明: 规划器位置积分
 ********/
static void test_pid_int_pos(void)
{
    s_plan_pos_itg += s_plan_vel;
    s_plan_pos += s_plan_pos_itg / CONTROL_FREQ;
    s_plan_pos_itg = s_plan_pos_itg % CONTROL_FREQ;
}

/********
 * @ 说明: 每 tick 调用，从 s_plan_pos 向 goal 做梯形规划
 * @ 输入: goal: 目标微步位置
 ********/
static void test_pid_track_goal(signed long goal)
{
    signed long delta = goal - s_plan_pos;
    signed long accel = RATED_VEL_ACC;

    if (delta == 0)
    {
        if (s_plan_vel > 0)
        {
            test_pid_int_vel(-accel);
            if (s_plan_vel < 0)
            {
                s_plan_vel_itg = 0;
                s_plan_vel = 0;
            }
        }
        else if (s_plan_vel < 0)
        {
            test_pid_int_vel(accel);
            if (s_plan_vel > 0)
            {
                s_plan_vel_itg = 0;
                s_plan_vel = 0;
            }
        }
    }
    else
    {
        if (s_plan_vel == 0)
        {
            if (delta > 0) test_pid_int_vel(accel);
            else test_pid_int_vel(-accel);
        }
        else if ((delta > 0 && s_plan_vel > 0) ||
                 (delta < 0 && s_plan_vel < 0))
        {
            signed long need_down = (signed long)(
                (float)s_plan_vel * (float)s_plan_vel
                / (2.0f * (float)accel));

            if (abs(delta) > need_down)
            {
                if (abs(s_plan_vel) < RATED_VELOCITY)
                {
                    if (delta > 0) test_pid_int_vel(accel);
                    else test_pid_int_vel(-accel);
                }
            }
            else
            {
                if (delta > 0) test_pid_int_vel(-accel);
                else test_pid_int_vel(accel);
            }
        }
        else
        {
            if (s_plan_vel > 0) test_pid_int_vel(-accel);
            else test_pid_int_vel(accel);

            if (s_plan_vel == 0)
            {
                s_plan_vel_itg = 0;
            }
        }
    }

    test_pid_int_pos();
}

/********
 * @ 说明: 速度越快，FOC 电角度越要提前，补偿电感滞后
 * @ 输入: vel: 当前速度（微步/秒）
 * @ 输出: 超前补偿量（微步）
 * @ 注意: 标定数据来自实测
 *         0~100k    → 0
 *         100k~1300k → 0→300
 *         1300k~2200k → 300→390
 *         >2200k    → 390→430(极限)
 ********/
static signed long test_pid_comp_angle(signed long vel)
{
    signed long compensate;

    if (vel < 0)
    {
        if (vel > -100000) compensate = 0;
        else if (vel > -1300000) compensate = (((vel + 100000) * 262) >> 20) - 0;
        else if (vel > -2200000) compensate = (((vel + 1300000) * 105) >> 20) - 300;
        else compensate = (((vel + 2200000) * 52) >> 20) - 390;
        if (compensate < -430) compensate = -430;
    }
    else
    {
        if (vel < 100000) compensate = 0;
        else if (vel < 1300000) compensate = (((vel - 100000) * 262) >> 20) + 0;
        else if (vel < 2200000) compensate = (((vel - 1300000) * 105) >> 20) + 300;
        else compensate = (((vel - 2200000) * 52) >> 20) + 390;
        if (compensate > 430) compensate = 430;
    }

    return compensate;
}

/* test_pid hlp end */
//----------------------------------------------------------------------------------
/* test_pid cac start */

/********
 * @ 说明: TIM4 20kHz 周期中断回调
 *         编码器 → 速度估计 → 运动规划
 *         → 位置环 PID（输出目标速度）
 *         → 速度环 PID（输出目标电流）
 *         → 超前角 + FOC
 ********/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    signed long raw, delta, pos_error, pos_int_step, pos_d;
    signed long vel_error, vel_int_step, vel_d, current;
    signed long lead, elec_angle, goal_error;

    if (htim->Instance != TIM4)
        return;

    ela_mt6816_usr_read_angle();
    raw = (signed long)g_mt6816_st.raw_angle;

    if (s_first)
    {
        s_enc_last = raw;
        s_cur_pos = raw * MICROSTEP_LAP / ENC_RESOLUTION;
        s_first = 0;
        return;
    }

    delta = raw - s_enc_last;
    if (delta > (ENC_RESOLUTION >> 1))
        delta -= ENC_RESOLUTION;
    else if (delta < -(ENC_RESOLUTION >> 1))
        delta += ENC_RESOLUTION;
    s_enc_last = raw;

    delta = delta * MICROSTEP_LAP / ENC_RESOLUTION;

    /* 低通滤波速度估计：新值×1/32 + 旧值×31/32 */
    s_vel_itg += (delta * 20000) + ((s_vel << 5) - s_vel);
    s_vel = s_vel_itg >> 5;
    s_vel_itg -= (s_vel << 5);

    s_cur_pos += delta;

    test_pid_track_goal(s_goal);

    /* ===== 位置环 PID：误差 → 目标速度 =====
       pos_error 单位: 微步
       输出 s_target_vel 单位: 微步/秒
       Ki 用 >> 10 缩放 */
    pos_error = s_plan_pos - s_cur_pos;

    pos_itg_rd += pos_ki * pos_error;
    pos_int_step = pos_itg_rd >> 10;
    pos_itg_rd -= pos_int_step << 10;
    pos_itg += pos_int_step;

    if (pos_itg > RATED_VELOCITY)
        pos_itg = RATED_VELOCITY;
    else if (pos_itg < -RATED_VELOCITY)
        pos_itg = -RATED_VELOCITY;

    pos_d = pos_kd * (pos_error - pos_err_last);
    pos_err_last = pos_error;

    s_target_vel = pos_kp * pos_error + pos_itg + pos_d;

    if (s_target_vel > RATED_VELOCITY)
        s_target_vel = RATED_VELOCITY;
    else if (s_target_vel < -RATED_VELOCITY)
        s_target_vel = -RATED_VELOCITY;

    /* ===== 速度环 PID：速度误差 → 目标电流 =====
       vel_error 单位: 微步/秒
       输出 current 单位: mA
       Ki 用 >> 10 缩放 */
    vel_error = s_target_vel - s_vel;

    vel_itg_rd += vel_ki * vel_error;
    vel_int_step = vel_itg_rd >> 10;
    vel_itg_rd -= vel_int_step << 10;
    vel_itg += vel_int_step;

    if (vel_itg > RATED_CURRENT_MA)
        vel_itg = RATED_CURRENT_MA;
    else if (vel_itg < -RATED_CURRENT_MA)
        vel_itg = -RATED_CURRENT_MA;

    vel_d = vel_kd * (vel_error - vel_err_last);
    vel_err_last = vel_error;

    current = vel_kp * vel_error + vel_itg + vel_d;

    if (current > RATED_CURRENT_MA)
        current = RATED_CURRENT_MA;
    else if (current < -RATED_CURRENT_MA)
        current = -RATED_CURRENT_MA;

    lead = test_pid_comp_angle(s_vel);
    elec_angle = (s_cur_pos + lead + 256) % 1024;
    if (elec_angle < 0) elec_angle += 1024;
    ela_tb67h450_set_foc_current((unsigned long)elec_angle, (short)current);

    goal_error = s_goal - s_cur_pos;
    if (abs(goal_error) < POSITION_TOLERANCE)
    {
        s_hold++;
        if (s_hold > 20000)
        {
            s_hold = 0;
            s_next = 1;
        }
    }
    else
    {
        s_hold = 0;
    }
}

/* test_pid cac end */
//----------------------------------------------------------------------------------
/* test_pid usr start */

/********
 * @ 说明: 级联 PID 定位测试主函数
 *         梯形规划 → 位置环 PID → 速度环 PID → FOC
 *         依次走 10000 → -5000 → -5000 → 0 微步
 ********/
void test_pid(void)
{
    signed long targets[] = { 10000, -5000, -5000, 0 };
    unsigned char num_targets = sizeof(targets) / sizeof(targets[0]);

    printf("--- Cascaded PID + Trapezoidal Planner + Advanced Angle ---\r\n");
    printf("Pos PID: Kp=%ld, Ki=%ld(x1/1024), Kd=%ld\r\n",
           pos_kp, pos_ki, pos_kd);
    printf("Vel PID: Kp=%ld, Ki=%ld(x1/1024), Kd=%ld\r\n",
           vel_kp, vel_ki, vel_kd);
    printf("RatedCurrent=%ldmA, RatedVel=%ld\r\n",
           RATED_CURRENT_MA, RATED_VELOCITY);

    ela_mt6816_usr_init();
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);

    ela_mt6816_usr_read_angle();
    s_first = 1;

    HAL_TIM_Base_Start_IT(&htim4);

    while (s_first);

    printf("Start position: %ld\r\n", s_cur_pos);

    s_plan_pos = s_cur_pos;
    s_plan_vel = 0;
    s_plan_vel_itg = 0;
    s_plan_pos_itg = 0;

    s_goal = s_cur_pos + targets[0];
    s_phase = 0;
    printf("Phase 0: target = %ld\r\n", s_goal);

    while (1)
    {
        if (s_next)
        {
            s_next = 0;
            s_phase++;

            /* 清空两环 PID 状态，防止旧误差残留 */
            pos_itg_rd = 0;
            pos_itg = 0;
            pos_err_last = 0;
            vel_itg_rd = 0;
            vel_itg = 0;
            vel_err_last = 0;
            s_plan_vel_itg = 0;
            s_plan_pos_itg = 0;

            if (s_phase >= num_targets)
            {
                printf("--- Test Complete ---\r\n");
                ela_tb67h450_brake();
                break;
            }

            s_plan_vel = 0;
            s_goal += targets[s_phase];
            printf("Phase %d: target = %ld\r\n", s_phase, s_goal);
        }
    }

    while (1) {}
}

/* test_pid usr end */
