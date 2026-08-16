/*****************************************************************************
 * @文件: motor_usr.c
 * @作者: cl
 * @日期: 2026-08-13
 * @版本: v1.0
 * @说明: 电机用户层（串级闭环：编码器映射 + 位置环→速度环→电流 +
 *   超前角补偿 + planner 软目标 + 位置/速度/电流/轨迹命令 + IIR 速度估计 +
 *   完整状态机 STOP/FINISH/RUNNING/OVERLOAD/STALL/NO_CALIB）
 * @平台: STM32F103RET6
 * @依赖: mt6816_usr, tb67h450_usr, encoder_calibrator_usr,
 *   cycle_usr, motion_planner_usr
 ****************************************************************************/
#include "motor_usr.h"
#include "mt6816_usr.h"
#include "tb67h450_usr.h"
#include "encoder_calibrator_usr.h"
#include "cycle_usr.h"
#include "motion_planner_usr.h"
#include <stddef.h>

/* ==== 常量定义 ==== */
/* 未校准线性映射：raw 14bit(0~16383) ×25/8 → 51200 空间
   依据 .cl/memory/ motor_enc_raw_scale=×25/8（校准后由校准表接管，表输出即 51200 空间） */
#define USR_MOTOR_ENC_SCALE_MUL   25U
#define USR_MOTOR_ENC_SCALE_DIV   8U

/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
static Motor_Config_T *s_config = NULL;      /* 配置指针（main.c 注入） */
static Motor_Mode_T    s_request_mode = MODE_STOP;
static Motor_Mode_T    s_mode_running = MODE_STOP;
static Motor_State_T   s_state = STATE_STOP;

/* 实测位置 */
static int32_t s_real_lap_position = 0;      /* 单圈位置（0~51199） */
static int32_t s_real_lap_position_last = 0;
static int32_t s_real_position = 0;          /* 累计位置（多圈） */
static int32_t s_real_position_last = 0;
static int32_t s_delta_sum = 0;              /* 周期累计位置增量（100ms 遥测清零） */
static int32_t s_vel_last = 0;               /* 上一帧速度估计（速度环阻尼用） */

/* 估计量 */
static int32_t s_est_velocity = 0;           /* 估计速度（细分步/s） */
static int32_t s_est_velocity_integral = 0;
static int32_t s_est_lead_position = 0;      /* 超前角补偿量（细分步，运动随速递减） */
static int32_t s_est_position = 0;           /* 控制用估计位置（=实测+超前角，任务6已加） */

/* 目标值 */
static int32_t s_goal_position = 0;
static int32_t s_goal_velocity = 0;
static int32_t s_goal_current = 0;
static bool    s_goal_disable = false;
static bool    s_goal_brake = false;

/* 软目标（planner 输出，控制环输入） */
static int32_t s_soft_position = 0;          /* 软位置（细分步） */
static int32_t s_soft_velocity = 0;          /* 软速度（细分步/s） */
static int32_t s_soft_current = 0;           /* 软电流（mA） */
static bool    s_soft_disable = false;       /* 上一帧禁用状态（边沿触发新曲线） */
static bool    s_soft_brake = false;         /* 上一帧刹车状态（边沿触发新曲线） */
static bool    s_soft_new_curve = false;     /* 新曲线标志（模式切换/边沿触发） */

/* 故障检测 */
static bool    s_is_stalled = false;         /* 堵转标志 */
static uint32_t s_stalled_time = 0;          /* 堵转计时（µs） */
static uint32_t s_overload_time = 0;         /* 过载计时（µs） */
static bool    s_overload_flag = false;      /* 过载标志 */

/* planner 配置实例（motor 内部持有，SetConfig 时同步注入） */
static MotionPlanner_Config_T s_planner_config;

/* FOC 输出 */
static int32_t s_foc_position = 0;
static int32_t s_foc_current = 0;

/* 首次调用标志 */
static bool s_first_called = true;

/* ==== 内部工具 ==== */
/**
 * @输入 x: 32 位整数
 * @输出 绝对值
 * @说明 取绝对值（避免 INT32_MIN 溢出用分支判断）
 */
static int32_t S_Abs32(int32_t x)
{
    return (x < 0) ? -x : x;
}

/**
 * @输入 vel: 估计速度（细分步/s）
 * @输出 超前角补偿量（细分步，|≤430|）
 * @说明 分段线性超前角补偿：高速时 FOC 电角度超前，补偿电流矢量换相滞后
 *   依据 .cl/memory/ motor_compensate_angle=分段|±430| 步
 *   + 复刻参考 motor.c CompensateAdvancedAngle（阈值/斜率逐值照搬）
 */
static int32_t S_CompensateAdvancedAngle(int32_t vel)
{
    int32_t compensate;

    if (vel < 0)
    {
        if (vel > -(int32_t)USR_MOTOR_LEAD_VEL1)
        {
            compensate = 0;
        }
        else if (vel > -(int32_t)USR_MOTOR_LEAD_VEL2)
        {
            compensate = (((vel + (int32_t)USR_MOTOR_LEAD_VEL1)
                           * (int32_t)USR_MOTOR_LEAD_SLOPE1) >> 20) - 0;
        }
        else if (vel > -(int32_t)USR_MOTOR_LEAD_VEL3)
        {
            compensate = (((vel + (int32_t)USR_MOTOR_LEAD_VEL2)
                           * (int32_t)USR_MOTOR_LEAD_SLOPE2) >> 20) - 300;
        }
        else
        {
            compensate = (((vel + (int32_t)USR_MOTOR_LEAD_VEL3)
                           * (int32_t)USR_MOTOR_LEAD_SLOPE3) >> 20) - 390;
        }
        if (compensate < -(int32_t)USR_MOTOR_LEAD_MAX)
        {
            compensate = -(int32_t)USR_MOTOR_LEAD_MAX;
        }
    }
    else
    {
        if (vel < (int32_t)USR_MOTOR_LEAD_VEL1)
        {
            compensate = 0;
        }
        else if (vel < (int32_t)USR_MOTOR_LEAD_VEL2)
        {
            compensate = (((vel - (int32_t)USR_MOTOR_LEAD_VEL1)
                           * (int32_t)USR_MOTOR_LEAD_SLOPE1) >> 20) + 0;
        }
        else if (vel < (int32_t)USR_MOTOR_LEAD_VEL3)
        {
            compensate = (((vel - (int32_t)USR_MOTOR_LEAD_VEL2)
                           * (int32_t)USR_MOTOR_LEAD_SLOPE2) >> 20) + 300;
        }
        else
        {
            compensate = (((vel - (int32_t)USR_MOTOR_LEAD_VEL3)
                           * (int32_t)USR_MOTOR_LEAD_SLOPE3) >> 20) + 390;
        }
        if (compensate > (int32_t)USR_MOTOR_LEAD_MAX)
        {
            compensate = (int32_t)USR_MOTOR_LEAD_MAX;
        }
    }
    return compensate;
}

/**
 * @输入 cur: 电流指针（输出限幅结果）
 * @输出 无
 * @说明 电流限幅到 ±ratedCurrent
 * 依据 .cl/memory/ motor_minloop_control: P 环输出限 ±ratedCurrent
 */
static void S_ClampCurrent(int32_t *cur)
{
    if (*cur > s_config->ratedCurrent)
    {
        *cur = s_config->ratedCurrent;
    }
    else if (*cur < -s_config->ratedCurrent)
    {
        *cur = -s_config->ratedCurrent;
    }
}

/**
 * @输入 无
 * @输出 无
 * @说明 清零 FOC 输出并断电/制动由调用方决定
 */
static void S_ZeroOutput(void)
{
    s_foc_position = 0;
    s_foc_current = 0;
}

/**
 * @输入 current: 电流输出（mA）
 * @输出 无
 * @说明 FOC 电流矢量输出：正电流电角度超前 90°、负电流滞后 90°（细分步=256），
 *   再交给 tb67h450_usr 查正弦表驱动两相 PWM
 * 依据 .cl/memory/ motor_foc_lead_90=SOFT_DIVIDE_NUM=256（A 相领先 B 相 90°，细分步域）
 */
static void S_CalcCurrentToOutput(int32_t current)
{
    s_foc_current = current;

    if (s_foc_current > 0)
    {
        s_foc_position = s_est_position + (int32_t)USR_MOTOR_SOFT_DIVIDE;
    }
    else if (s_foc_current < 0)
    {
        s_foc_position = s_est_position - (int32_t)USR_MOTOR_SOFT_DIVIDE;
    }
    else
    {
        s_foc_position = s_est_position;
    }

    USR_TB67H450_SetFocCurrentVector((uint32_t)s_foc_position, s_foc_current);
}

/**
 * @输入 goal_pos: 目标位置（细分步）
 * @输出 无
 * @说明 位置环（串级外环）：|err|≤死区 → 速度目标 0（速度环刹停残余速度，静止时
 *   输出≈0 等价归零，避免 FOC 电流引起编码器微抖被速度估计放大成极限环）；
 *   死区外 → P 项死区映射 → 速度目标=(posKp×err)>>10，限 ±ratedVelocity（串级限速，
 *   位置环不再混合 Kd——阻尼职责移交速度环）→ 调 S_CalcVelocityP 内环输出电流
 * 依据 .cl/memory/ motor_pos_deadband=128 + motor_cascade_poskp=32768
 *   + config_default_pid pid_kp=5（待实测整定）+ 参考 motor.c CalcPidToOutput 量纲
 */
static void S_CalcVelocityP(int32_t goal_vel);   /* 前向声明（内环定义在外环之后） */

/* vel_goal 上一帧值（限斜率用，外环静态状态） */
static int32_t s_vel_goal_last = 0;
/* 死区状态（滞回）：1=死区外驱动中，0=死区内归零。
   出界触发驱动，回中心（|err|<HYST）才归零 → 防边缘漂移极限环 */
static uint8_t s_db_active = 0U;
/* 死区制动倒计时（帧）：进入死区后先刹停残余速度（过冲小）再 0 电流。
   8/15 实测：保持力电流 → 编码器磁干扰 → 假速度 ±100000 → 位置环误判
   → 摆动 ±260（180° 段）；0 电流无磁干扰但残余速度滑行过冲；
   一次性制动（有限帧）→ 无持续速度环 → 无假速度自激 */
static uint16_t s_db_brake_cnt = 0U;

/**
 * @输入 无
 * @输出 无
 * @说明 清控制环状态（新曲线/断电/刹车时调用）：
 *   速度环阻尼基准对齐当前速度（防阻尼阶跃反冲）+
 *   位置环限斜率/死区状态复位（重新爬坡/重新判定）
 *   依据 .cl/memory/ motor_arrival_brake_ms
 *   + 复刻参考 motor.c ClearIntegral（本项目无积分器）
 */
static void S_ClearIntegral(void)
{
    s_vel_last = s_est_velocity;
    s_vel_goal_last = 0;
    s_db_active = 0U;
    s_db_brake_cnt = 0U;
}

static void S_CalcPositionCascade(int32_t goal_pos)
{
    int32_t err = goal_pos - s_est_position;
    int32_t vel_goal;
    int32_t deadband = (int32_t)USR_MOTOR_POS_DEADBAND;

    /* planner 未完成（软目标≠最终目标）：轨迹速度主导 + P 修正（8/16）
       根因：planner 减速段软目标爬升慢 → 位置环 err 长期滞留低速区
       （vel_goal < 假速度 ±25000~40000）→ 速度环被编码器磁干扰假速度
       主导 → ±2A 猛摆极限环（8/16 实测 POSITION 长行程 vel ±1~3.8 圈/s，
       pos 微摆不收敛）。改为 vel_goal=planner 轨迹速度+位置修正 →
       速度环跟踪连续轨迹（无假速度主导机会）；planner 完成时 real 贴
       soft → 小 err 交棒下方 8/15 整形 → 死区收敛 */
    if (goal_pos != s_goal_position)
    {
        vel_goal = s_soft_velocity + ((s_config->posKp * err) >> 10);
        if (vel_goal > s_config->ratedVelocity)
        {
            vel_goal = s_config->ratedVelocity;
        }
        else if (vel_goal < -s_config->ratedVelocity)
        {
            vel_goal = -s_config->ratedVelocity;
        }
        /* 低速直驱（8/16）：vel_goal 落假速度区（≤25000 步/s，planner 减速
           尾段 soft_vel→0 时）→ 速度环被假速度主导摆荡 → 改位置环误差直驱
           电流（用 s_real_position 免超前角污染，磁干扰只污染速度估计） */
        if ((vel_goal <= (int32_t)USR_MOTOR_FAKE_VEL_MAX)
                && (vel_goal >= -(int32_t)USR_MOTOR_FAKE_VEL_MAX))
        {
            int32_t cur = goal_pos - s_real_position;
            if (cur > s_config->ratedCurrent)
            {
                cur = s_config->ratedCurrent;
            }
            else if (cur < -s_config->ratedCurrent)
            {
                cur = -s_config->ratedCurrent;
            }
            s_vel_goal_last = 0;
            S_CalcCurrentToOutput(cur);
            return;
        }
        int32_t dv = vel_goal - s_vel_goal_last;
        if (dv > (int32_t)USR_MOTOR_VEL_GOAL_ACC)
        {
            vel_goal = s_vel_goal_last + (int32_t)USR_MOTOR_VEL_GOAL_ACC;
        }
        else if (dv < -(int32_t)USR_MOTOR_VEL_GOAL_ACC)
        {
            vel_goal = s_vel_goal_last - (int32_t)USR_MOTOR_VEL_GOAL_ACC;
        }
        s_vel_goal_last = vel_goal;
        s_db_active = 0U;   /* 交棒：planner 完成走归零段（制动+0 电流） */
        S_CalcVelocityP(vel_goal);
        return;
    }

    /* 绕回窗口判定用固定目标 s_goal_position（goal_pos=planner 软目标逼近时边界抖动） */
    int32_t goal_fix = s_goal_position;

    /* 绕回窗口内死区加大（128→256）：0° 处弹性/毛刺摆动 ±250 > 128 →
       频繁出界驱动 → 摆动自激（8/15 MODE_POSITION 实测 33 行）；
       256 吞掉摆动 → 不出界不驱动 → 自由衰减静止 */
    if ((goal_fix > ((int32_t)USR_MOTOR_SUBDIVIDE_STEPS - (int32_t)USR_MOTOR_POS_WRAP_WIN))
            && (goal_fix < ((int32_t)USR_MOTOR_SUBDIVIDE_STEPS + (int32_t)USR_MOTOR_POS_WRAP_WIN)))
    {
        deadband = (int32_t)USR_MOTOR_POS_DEADBAND_WRAP;
    }

    if (s_db_active != 0U)
    {
        /* 归零窗口：err ∈ [+HYST, +DEADBAND]（目标下方 16~死区 步）。
           BUG 记录（8/15）：原对称归零 |err|<16 → 归零后机械正向回弹
           （齿隙弹性恒 +123 步）恒出界 → 推回量≡回弹量恒等平衡 → 极限环
           （180° 实测 26 行 25609↔25766）；目标下方归零 → 正向回弹把
           位置带回死区中心 → 一次收敛 */
        if ((err >= (int32_t)USR_MOTOR_POS_DB_HYST)
                && (err <= deadband))
        {
            s_db_active = 0U;
            s_db_brake_cnt = USR_MOTOR_POS_DB_BRAKE_MS;   /* 进入死区：先刹停残余速度 */
        }
    }
    else if ((err > deadband)
            || (err < -deadband))
    {
        s_db_active = 1U;
    }

    if (s_db_active != 0U)
    {
        /* 死区映射：出界 → 削死区（保持符号）；滞回残留（|err|≤死区）→ 0。
           BUG 记录（8/15）：残留区 err+128 翻转符号（err=-18→+110）被 MIN_VEL
           钳位放大成反向猛推 → 0°/90° 到位摆动极限环（实测 17~23 行） */
        if (err > deadband)
        {
            err -= deadband;
        }
        else if (err < -deadband)
        {
            err += deadband;
        }
        else
        {
            err = 0;
        }

        if (err > (int32_t)USR_MOTOR_POS_ERR_MAX)
        {
            err = (int32_t)USR_MOTOR_POS_ERR_MAX;
        }
        else if (err < -(int32_t)USR_MOTOR_POS_ERR_MAX)
        {
            err = -(int32_t)USR_MOTOR_POS_ERR_MAX;
        }

        /* 位置环：误差→速度目标（串级外环输出限幅=内环额定速度） */
        vel_goal = (s_config->posKp * err) >> 10;
        if (vel_goal > s_config->ratedVelocity)
        {
            vel_goal = s_config->ratedVelocity;
        }
        else if (vel_goal < -s_config->ratedVelocity)
        {
            vel_goal = -s_config->ratedVelocity;
        }
        /* 死区外最小推进速度钳位（8/15 整形，planner 完成段）：仅 |err|>减速窗口
           强制 MIN_VEL 推进（推得动 > 假速度）；减速窗口内 32×err 线性下坡低速
           进死区 → 落点无过冲。8/16 注：此块仅在 planner 完成（软目标=最终目标）
           后生效——planner 段已由轨迹速度主导（见函数头），无 MIN_VEL 冲过问题 */
        if ((err != 0)
                && ((err > (int32_t)USR_MOTOR_POS_MIN_VEL_DS)
                    || (err < -(int32_t)USR_MOTOR_POS_MIN_VEL_DS)))
        {
            int32_t min_vel = (int32_t)USR_MOTOR_POS_MIN_VEL;
            if ((goal_fix > ((int32_t)USR_MOTOR_SUBDIVIDE_STEPS - (int32_t)USR_MOTOR_POS_WRAP_WIN))
                    && (goal_fix < ((int32_t)USR_MOTOR_SUBDIVIDE_STEPS + (int32_t)USR_MOTOR_POS_WRAP_WIN)))
            {
                min_vel = (int32_t)USR_MOTOR_POS_MIN_VEL_WRAP;
            }

            if ((vel_goal >= 0) && (vel_goal < min_vel))
            {
                vel_goal = min_vel;
            }
            else if ((vel_goal < 0) && (vel_goal > -min_vel))
            {
                vel_goal = -min_vel;
            }
        }

        /* 低速直驱（8/16）：MIN_VEL 未钳位的 vel_goal 落假速度区（≤25000 步/s）
           → 速度环被假速度主导摆荡 → 改位置环误差直驱电流（用 s_real_position
           免超前角污染）→ err 单调衰减 → 死区 0 电流收敛 */
        if ((vel_goal <= (int32_t)USR_MOTOR_FAKE_VEL_MAX)
                && (vel_goal >= -(int32_t)USR_MOTOR_FAKE_VEL_MAX))
        {
            int32_t cur = goal_fix - s_real_position;
            if (cur > s_config->ratedCurrent)
            {
                cur = s_config->ratedCurrent;
            }
            else if (cur < -s_config->ratedCurrent)
            {
                cur = -s_config->ratedCurrent;
            }
            s_vel_goal_last = 0;
            S_CalcCurrentToOutput(cur);
            return;
        }

        /* 输出限斜率（简易轨迹整形）：vel_goal 每帧变化 ≤ ACC，
           长行程满速冲入改为受控加减速 → 到位速度≈0 → 无过冲振荡 */
        int32_t dv = vel_goal - s_vel_goal_last;
        if (dv > (int32_t)USR_MOTOR_VEL_GOAL_ACC)
        {
            vel_goal = s_vel_goal_last + (int32_t)USR_MOTOR_VEL_GOAL_ACC;
        }
        else if (dv < -(int32_t)USR_MOTOR_VEL_GOAL_ACC)
        {
            vel_goal = s_vel_goal_last - (int32_t)USR_MOTOR_VEL_GOAL_ACC;
        }
        s_vel_goal_last = vel_goal;
    }
    else
    {
        /* 归零：先一次性制动（刹停残余速度 → 过冲小），后 0 电流（衰减微振）。
           BUG 记录（8/15）：保持力电流 → 编码器磁干扰 → 假速度 ±100000 →
           位置环误判 → 摆动 ±260（180° 段）→ 删除保持力；
           0 电流无磁干扰，但残余速度滑行过冲 → 制动 10ms 刹停；
           绕回窗口（目标≈编码器 0 点）直接 0 电流（0° 毛刺假速度 ±40000
           会让制动输出波动推位置 → 摆动，实测 0 电流衰减收敛） */
        if ((goal_fix > ((int32_t)USR_MOTOR_SUBDIVIDE_STEPS - (int32_t)USR_MOTOR_POS_WRAP_WIN))
                && (goal_fix < ((int32_t)USR_MOTOR_SUBDIVIDE_STEPS + (int32_t)USR_MOTOR_POS_WRAP_WIN)))
        {
            s_vel_goal_last = 0;
            S_CalcCurrentToOutput(0);
        }
        else if (s_db_brake_cnt > 0U)
        {
            s_db_brake_cnt--;
            s_vel_goal_last = 0;
            S_CalcVelocityP(0);
        }
        else
        {
            s_vel_goal_last = 0;
            S_CalcCurrentToOutput(0);
        }
        return;
    }

    S_CalcVelocityP(vel_goal);
}

/**
 * @输入 goal_vel: 目标速度（细分步/s）
 * @输出 无
 * @说明 速度 P 环（串级内环）：current = pidKp×err>>10，err 限 ±1024×1024，
 *   输出限 ±ratedCurrent；位置模式经外环输入目标，速度模式直接输入
 * 依据 .cl/memory/ config_default_pid pid_kp=5（待实测整定）+ motor_minloop_control P 环量纲
 */
static void S_CalcVelocityP(int32_t goal_vel)
{
    int32_t err = goal_vel - s_est_velocity;
    int32_t out;
    int32_t dvel;

    if (err > (int32_t)USR_MOTOR_VEL_ERR_MAX)
    {
        err = (int32_t)USR_MOTOR_VEL_ERR_MAX;
    }
    else if (err < -(int32_t)USR_MOTOR_VEL_ERR_MAX)
    {
        err = -(int32_t)USR_MOTOR_VEL_ERR_MAX;
    }

    /* 速度阻尼（8/13 实测 Kd=400 压住编码器微抖假速度极限环；串级重构后恢复）
       微振时 v 每帧大幅波动 → 阻尼反向电流抵消；匀速时 dv≈0 不干预。
       8/16 VELOCITY 持续运行实测：微振 dvel 被 Kd×dvel>>10 放大 → 电流饱和
       ±2000mA 方波 bang-bang（0.5~2.5 圈/s 摆动极限环）→ Kd 项限 ±256mA：
       阻尼电流 < 静摩擦（250mA）推不动真实电机 → 不激发运动 → 震荡消除；
       真实加减速由 Kp 项主导；POSITION 死区制动（10ms）Kp=10×err>>10 仍有效 */
    dvel = s_est_velocity - s_vel_last;
    s_vel_last = s_est_velocity;

    out = (s_config->pidKp * err) >> 10;
    /* 8/16 VELOCITY 持续运行实测：Kp=10 → 500mA 推空载电机几十 ms 冲过目标 →
       err 反号 → 反向满推 → 极限环（±1~3 圈/s 往返，周期 ~200ms）。
       速度模式（无外环整形）降 Kp=3 减增益破环；POSITION 外环（8/15 已收敛
       标定）保持 Kp=10 */
    if ((s_mode_running == MODE_COMMAND_VELOCITY)
            || (s_mode_running == MODE_PWM_VELOCITY))
    {
        out = (3 * err) >> 10;
    }
    {
        int32_t kd = (s_config->pidKd * dvel) >> 10;
        if (kd > (int32_t)256)
        {
            kd = (int32_t)256;
        }
        else if (kd < (int32_t)-256)
        {
            kd = (int32_t)-256;
        }
        out -= kd;
    }
    S_ClampCurrent(&out);
    S_CalcCurrentToOutput(out);
}

/* ==== 接口实现 ==== */
/**
 * @输入 无
 * @输出 无
 * @说明 初始化：复位状态/目标/输出，等待 SetConfig 注入配置
 */
void USR_Motor_Init(void)
{
    s_first_called = true;          /* 首帧标志：下一次 Tick 做零位初始化 */
    s_request_mode = MODE_STOP;     /* 请求模式复位为 STOP */
    s_mode_running = MODE_STOP;     /* 运行中模式同步复位 */
    s_state = STATE_STOP;           /* 状态机回到 STOP */
    s_goal_position = 0;            /* 位置目标清零（细分步） */
    s_goal_velocity = 0;            /* 速度目标清零（细分步/s） */
    s_goal_current = 0;             /* 电流目标清零（mA） */
    s_goal_disable = false;         /* 撤销禁用请求 */
    s_goal_brake = false;           /* 撤销刹车请求 */
    s_real_lap_position = 0;        /* 实测单圈位置清零 */
    s_real_lap_position_last = 0;   /* 上一帧单圈位置清零（算增量用） */
    s_real_position = 0;            /* 实测累计位置清零（多圈） */
    s_real_position_last = 0;       /* 上一帧累计位置清零（算速度用） */
    s_est_velocity = 0;             /* 估计速度清零（细分步/s） */
    s_est_velocity_integral = 0;    /* IIR 速度积分器清零 */
    s_est_lead_position = 0;        /* 超前角补偿清零 */
    s_est_position = 0;             /* 控制用估计位置清零 */
    s_soft_position = 0;            /* 软目标清零 */
    s_soft_velocity = 0;
    s_soft_current = 0;
    s_soft_disable = false;
    s_soft_brake = false;
    s_soft_new_curve = false;
    s_is_stalled = false;           /* 堵转标志清零 */
    s_stalled_time = 0;
    s_overload_time = 0;
    s_overload_flag = false;
    s_foc_position = 0;             /* FOC 输出电角度清零（细分步） */
    s_foc_current = 0;              /* FOC 输出电流清零（mA） */
    s_vel_last = 0;
    s_vel_goal_last = 0;
    s_db_active = 0U;
    s_db_brake_cnt = 0U;

    /* 装配 planner（SetConfig 已注入时；复刻参考 Motor_Init 挂 g_motion_config） */
    if (NULL != s_config)
    {
        s_planner_config.ratedCurrent = s_config->ratedCurrent;
        s_planner_config.ratedVelocity = s_config->ratedVelocity;
        s_planner_config.ratedVelocityAcc = s_config->ratedVelocityAcc;
        s_planner_config.ratedCurrentAcc = s_config->ratedCurrentAcc;
        USR_MotionPlanner_SetConfig(&s_planner_config);
        USR_MotionPlanner_CurrentTracker_Init();
        USR_MotionPlanner_VelocityTracker_Init();
        USR_MotionPlanner_PositionTracker_Init();
        /* 轨迹更新超时 200ms（依据 .cl/memory/ planner_trajectory_update_timeout=200） */
        USR_MotionPlanner_TrajectoryTracker_Init(200);
    }
}

/**
 * @输入 config: 电机配置指针（encoderHomeOffset/ratedCurrent/ratedVelocity/增益）
 * @输出 无
 * @说明 注入配置（在 Init 之后、Tick20kHz 之前调用）
 */
void USR_Motor_SetConfig(Motor_Config_T *config)
{
    s_config = config;
}

/**
 * @输入 无
 * @输出 无
 * @说明 20kHz 控制 tick：读角度→位置增量→IIR 速度→P 环/电流→FOC 输出→状态机
 * 依据 .cl/memory/ control_frequency=20000 / control_period_us=50
 */
void USR_Motor_Tick20kHz(void)
{
    uint16_t rectified;
    int32_t delta;
    int32_t angle;

    if (NULL == s_config)
    {
        return;   /* 未配置：不驱动 */
    }

    /* 1. 读角度（校准表已输出 51200 空间；未校准 raw×25/8 线性缩放） */
    USR_MT6816_UpdateAngle();
    rectified = USR_MT6816_GetRectifiedAngle();
    /* 未校准的话：raw×25/8 映射到 51200 空间 */
    if (!USR_EncoderCalibrator_IsCalibrated())
    {
        rectified = (uint16_t)(((uint32_t)USR_MT6816_GetRawAngle()
                                * USR_MOTOR_ENC_SCALE_MUL)
                               / USR_MOTOR_ENC_SCALE_DIV);
    }

    /* 2. 首次调用：按零位偏移初始化位置 */
    if (s_first_called)
    {
        if (s_config->encoderHomeOffset
                < (int32_t)(USR_MOTOR_SUBDIVIDE_STEPS / 2))
        {
            if (rectified
                    > (uint16_t)(s_config->encoderHomeOffset
                                 + (int32_t)(USR_MOTOR_SUBDIVIDE_STEPS / 2)))
            {
                angle = (int32_t)rectified - (int32_t)USR_MOTOR_SUBDIVIDE_STEPS;
            }
            else
            {
                angle = (int32_t)rectified;
            }
        }
        else
        {
            if (rectified
                    < (uint16_t)(s_config->encoderHomeOffset
                                 - (int32_t)(USR_MOTOR_SUBDIVIDE_STEPS / 2)))
            {
                angle = (int32_t)rectified + (int32_t)USR_MOTOR_SUBDIVIDE_STEPS;
            }
            else
            {
                angle = (int32_t)rectified;
            }
        }
        s_real_lap_position = angle;
        s_real_lap_position_last = angle;
        s_real_position = angle;
        s_real_position_last = angle;
        s_first_called = false;
        return;
    }

    /* 3. 位置增量（回绕安全，复用 cycle_usr 最短循环差） */
    s_real_lap_position_last = s_real_lap_position;
    s_real_lap_position = (int32_t)rectified;
    delta = USR_Cycle_Sub(s_real_lap_position, s_real_lap_position_last,
                          (int32_t)USR_MOTOR_SUBDIVIDE_STEPS);
    s_delta_sum += delta;
    s_real_position_last = s_real_position;
    s_real_position += delta;

    /* 4. 速度估计 IIR（低通系数 1/32）
       依据 .cl/memory/ motor_est_vel_filter:
       integral += Δpos×20kHz + (v<<5 - v), v=integral>>5 */
    s_est_velocity_integral += ((s_real_position - s_real_position_last)
                                * (int32_t)USR_MOTOR_CONTROL_FREQ
                                + ((s_est_velocity << 5) - s_est_velocity));
    s_est_velocity = s_est_velocity_integral >> 5;
    s_est_velocity_integral -= (s_est_velocity << 5);

    /* 超前角补偿：控制位置 = 实测 + 补偿（速度越高超前越多，到位减速至 0 补偿归零）
       依据 .cl/memory/ motor_compensate_angle + 复刻参考 motor.c（补偿量仅影响控制，
       不影响 s_real_position 遥测） */
    s_est_lead_position = S_CompensateAdvancedAngle(s_est_velocity);
    s_est_position = s_real_position + s_est_lead_position;

    /* 5. 模式切换（请求→运行，切换置新曲线标志） */
    if (s_mode_running != s_request_mode)
    {
        s_mode_running = s_request_mode;
        S_ZeroOutput();
        s_soft_new_curve = true;
    }

    /* 6. 目标限幅 */
    if (s_goal_velocity > s_config->ratedVelocity)
    {
        s_goal_velocity = s_config->ratedVelocity;
    }
    else if (s_goal_velocity < -s_config->ratedVelocity)
    {
        s_goal_velocity = -s_config->ratedVelocity;
    }
    if (s_goal_current > s_config->ratedCurrent)
    {
        s_goal_current = s_config->ratedCurrent;
    }
    else if (s_goal_current < -s_config->ratedCurrent)
    {
        s_goal_current = -s_config->ratedCurrent;
    }

    /* 7. 控制分派（堵转/断电/未校准 → 睡眠；刹车 → 制动；否则按模式）
       依据 .cl/memory/ motor_minloop_control 串级 + 复刻参考 motor.c 分派顺序 */
    if (s_is_stalled || s_soft_disable || !USR_EncoderCalibrator_IsCalibrated())
    {
        S_ZeroOutput();
        S_ClearIntegral();
        USR_TB67H450_Sleep();
    }
    else if (s_soft_brake)
    {
        S_ZeroOutput();
        S_ClearIntegral();
        USR_TB67H450_Brake();
    }
    else
    {
        switch (s_mode_running)
        {
            case MODE_STOP:
                S_ZeroOutput();
                USR_TB67H450_Sleep();
                break;
            case MODE_COMMAND_POSITION:
            case MODE_COMMAND_TRAJECTORY:
            case MODE_PWM_POSITION:
                S_CalcPositionCascade(s_soft_position);
                break;
            case MODE_COMMAND_VELOCITY:
            case MODE_PWM_VELOCITY:
                S_CalcVelocityP(s_soft_velocity);
                break;
            case MODE_COMMAND_CURRENT:
            case MODE_PWM_CURRENT:
                S_CalcCurrentToOutput(s_soft_current);
                break;
            default:
                S_ZeroOutput();
                USR_TB67H450_Sleep();
                break;
        }
    }

    /* 8. 新曲线触发（模式切换或断电/刹车边沿）：重起 planner 软目标 */
    if (s_soft_new_curve
            || (s_soft_disable && !s_goal_disable)
            || (s_soft_brake && !s_goal_brake))
    {
        s_soft_new_curve = false;
        S_ClearIntegral();
        s_is_stalled = false;
        s_stalled_time = 0;
        s_overload_time = 0;
        s_overload_flag = false;

        switch (s_mode_running)
        {
            case MODE_COMMAND_POSITION:
            case MODE_PWM_POSITION:
                USR_MotionPlanner_PositionTracker_NewTask(s_est_position,
                                                          s_est_velocity);
                break;
            case MODE_COMMAND_TRAJECTORY:
                USR_MotionPlanner_TrajectoryTracker_NewTask(s_est_position,
                                                            s_est_velocity);
                break;
            case MODE_COMMAND_VELOCITY:
            case MODE_PWM_VELOCITY:
                USR_MotionPlanner_VelocityTracker_NewTask(s_est_velocity);
                break;
            case MODE_COMMAND_CURRENT:
            case MODE_PWM_CURRENT:
                USR_MotionPlanner_CurrentTracker_NewTask(s_foc_current);
                break;
            default:
                break;
        }
    }

    /* 9. planner 软目标生成（复刻参考 motor.c Tracker_CalcSoftGoal 分派） */
    switch (s_mode_running)
    {
        case MODE_COMMAND_POSITION:
        case MODE_PWM_POSITION:
            USR_MotionPlanner_PositionTracker_CalcSoftGoal(s_goal_position);
            s_soft_position = USR_MotionPlanner_PositionTracker_GetGoLocation();
            s_soft_velocity =
                USR_MotionPlanner_PositionTracker_GetGoLocationVelocity();
            break;
        case MODE_COMMAND_TRAJECTORY:
            USR_MotionPlanner_TrajectoryTracker_CalcSoftGoal(s_goal_position,
                                                             s_goal_velocity);
            s_soft_position =
                USR_MotionPlanner_TrajectoryTracker_GetGoTrajPosition();
            s_soft_velocity =
                USR_MotionPlanner_TrajectoryTracker_GetGoTrajVelocity();
            break;
        case MODE_COMMAND_VELOCITY:
        case MODE_PWM_VELOCITY:
            USR_MotionPlanner_VelocityTracker_CalcSoftGoal(s_goal_velocity);
            s_soft_velocity = USR_MotionPlanner_VelocityTracker_GetGoVelocity();
            break;
        case MODE_COMMAND_CURRENT:
        case MODE_PWM_CURRENT:
            USR_MotionPlanner_CurrentTracker_CalcSoftGoal(s_goal_current);
            s_soft_current = USR_MotionPlanner_CurrentTracker_GetGoCurrent();
            break;
        default:
            break;
    }

    s_soft_disable = s_goal_disable;
    s_soft_brake = s_goal_brake;

    /* 10. 堵转检测（复刻参考 motor.c）：保护开关开启时，
        电流模式非零电流 或 电流顶格(≤额定限幅即推不动) + 速度 < 1/5 圈/s 持续 1s */
    if (s_config->stallProtectSwitch)
    {
        if ((((s_mode_running == MODE_COMMAND_CURRENT)
                || (s_mode_running == MODE_PWM_CURRENT))
                && (S_Abs32(s_foc_current) != 0))
                || (S_Abs32(s_foc_current) == s_config->ratedCurrent))
        {
            if (S_Abs32(s_est_velocity) < (int32_t)USR_MOTOR_STALL_VEL_MAX)
            {
                if (s_stalled_time >= (uint32_t)USR_MOTOR_STALL_TIME_US)
                {
                    s_is_stalled = true;
                }
                else
                {
                    s_stalled_time += (uint32_t)USR_MOTOR_CONTROL_US;
                }
            }
        }
        else
        {
            s_stalled_time = 0;
        }
    }

    /* 11. 过载检测（复刻参考 motor.c）：非电流模式 电流顶格 持续 1s */
    if ((s_mode_running != MODE_COMMAND_CURRENT)
            && (s_mode_running != MODE_PWM_CURRENT)
            && (S_Abs32(s_foc_current) == s_config->ratedCurrent))
    {
        if (s_overload_time >= (uint32_t)USR_MOTOR_STALL_TIME_US)
        {
            s_overload_flag = true;
        }
        else
        {
            s_overload_time += (uint32_t)USR_MOTOR_CONTROL_US;
        }
    }
    else
    {
        s_overload_time = 0;
        s_overload_flag = false;
    }

    /* 12. 状态机（完整：未校准 > 停止 > 堵转 > 过载 > 模式判定） */
    if (!USR_EncoderCalibrator_IsCalibrated())
    {
        s_state = STATE_NO_CALIB;
    }
    else if (s_mode_running == MODE_STOP)
    {
        s_state = STATE_STOP;
    }
    else if (s_is_stalled)
    {
        s_state = STATE_STALL;
    }
    else if (s_overload_flag)
    {
        s_state = STATE_OVERLOAD;
    }
    else
    {
        switch (s_mode_running)
        {
            case MODE_COMMAND_POSITION:
            case MODE_COMMAND_TRAJECTORY:
            case MODE_PWM_POSITION:
            {
                /* 到位判定：绕回窗口内死区同步加大（128→256，与位置环一致） */
                int32_t db = (int32_t)USR_MOTOR_POS_DEADBAND;
                if ((s_goal_position > ((int32_t)USR_MOTOR_SUBDIVIDE_STEPS - (int32_t)USR_MOTOR_POS_WRAP_WIN))
                        && (s_goal_position < ((int32_t)USR_MOTOR_SUBDIVIDE_STEPS + (int32_t)USR_MOTOR_POS_WRAP_WIN)))
                {
                    db = (int32_t)USR_MOTOR_POS_DEADBAND_WRAP;
                }
                if ((S_Abs32(s_goal_position - s_est_position) <= db)
                    && (S_Abs32(s_est_velocity)
                        <= (int32_t)USR_MOTOR_VEL_DEADBAND))
                {
                    s_state = STATE_FINISH;
                }
                else
                {
                    s_state = STATE_RUNNING;
                }
                break;
            }
            case MODE_COMMAND_VELOCITY:
            case MODE_PWM_VELOCITY:
                if (S_Abs32(s_goal_velocity - s_est_velocity)
                    <= (int32_t)USR_MOTOR_VEL_DEADBAND)
                {
                    s_state = STATE_FINISH;
                }
                else
                {
                    s_state = STATE_RUNNING;
                }
                break;
            case MODE_COMMAND_CURRENT:
            case MODE_PWM_CURRENT:
                if (S_Abs32(s_goal_current - s_foc_current)
                    <= (int32_t)USR_MOTOR_CUR_DEADBAND)
                {
                    s_state = STATE_FINISH;
                }
                else
                {
                    s_state = STATE_RUNNING;
                }
                break;
            default:
                s_state = STATE_STOP;
                break;
        }
    }
}

/**
 * @输入 mode: 请求模式（MODE_*）
 * @输出 无
 * @说明 设置运行模式（下次 tick 生效）
 */
void USR_Motor_SetMode(Motor_Mode_T mode)
{
    s_request_mode = mode;
}

/**
 * @输入 pos: 目标位置（相对 HomeOffset 的细分步）
 * @输出 无
 * @说明 位置命令：加 HomeOffset 存入目标
 */
void USR_Motor_SetPosition(int32_t pos)
{
    s_goal_position = pos + s_config->encoderHomeOffset;
}

/**
 * @输入 vel: 目标速度（细分步/s）
 * @输出 无
 * @说明 速度命令：限幅后存入目标
 */
void USR_Motor_SetVelocity(int32_t vel)
{
    if (vel >= -s_config->ratedVelocity && vel <= s_config->ratedVelocity)
    {
        s_goal_velocity = vel;
    }
}

/**
 * @输入 cur: 目标电流（mA）
 * @输出 无
 * @说明 电流命令：限幅后存入目标
 */
void USR_Motor_SetCurrent(int32_t cur)
{
    if (cur > s_config->ratedCurrent)
    {
        s_goal_current = s_config->ratedCurrent;
    }
    else if (cur < -s_config->ratedCurrent)
    {
        s_goal_current = -s_config->ratedCurrent;
    }
    else
    {
        s_goal_current = cur;
    }
}

/**
 * @输入 disable: true=断电（线圈不励磁）
 * @输出 无
 * @说明 断电控制
 */
void USR_Motor_SetDisable(bool disable)
{
    s_goal_disable = disable;
}

/**
 * @输入 brake: true=刹车（H 桥制动）
 * @输出 无
 * @说明 刹车控制
 */
void USR_Motor_SetBrake(bool brake)
{
    s_goal_brake = brake;
}

/**
 * @输入 无
 * @输出 无
 * @说明 清堵转标志（新曲线自动清；外部命令也可主动清）
 *   复刻参考 motor.c Motor_ClearStallFlag（清计时 + 标志）
 */
void USR_Motor_ClearStallFlag(void)
{
    s_stalled_time = 0;
    s_is_stalled = false;
}

/**
 * @输入 无
 * @输出 电机状态（STATE_*）
 * @说明 查询状态
 */
Motor_State_T USR_Motor_GetState(void)
{
    return s_state;
}

/**
 * @输入 is_lap: true=单圈位置(圈) false=累计位置(圈)
 * @输出 位置（圈，相对 HomeOffset）
 * @说明 查询位置
 */
float USR_Motor_GetPosition(bool is_lap)
{
    if (is_lap)
    {
        return (float)(s_real_lap_position - s_config->encoderHomeOffset)
               / (float)USR_MOTOR_SUBDIVIDE_STEPS;
    }
    return (float)(s_real_position - s_config->encoderHomeOffset)
           / (float)USR_MOTOR_SUBDIVIDE_STEPS;
}

/**
 * @输入 无
 * @输出 速度（圈/s）
 * @说明 查询估计速度
 */
float USR_Motor_GetVelocity(void)
{
    return (float)s_est_velocity / (float)USR_MOTOR_SUBDIVIDE_STEPS;
}

/**
 * @输入 无
 * @输出 电流（A）
 * @说明 查询 FOC 输出电流
 */
float USR_Motor_GetCurrent(void)
{
    return (float)s_foc_current / 1000.0f;
}

/**
 * @输入 无
 * @输出 运行模式（MODE_*）
 * @说明 查询模式
 */
uint8_t USR_Motor_GetMode(void)
{
    return (uint8_t)s_mode_running;
}

/**
 * @输入 pos/vel/cur: 输出指针（位置圈/速度圈每秒/电流A）
 * @输入 mode/state: 输出指针（模式/状态）
 * @输出 无
 * @说明 遥测打包（100Hz 周期调用）
 */
void USR_Motor_GetTelemetry(float *pos, float *vel, float *cur,
                            uint8_t *mode, uint8_t *state)
{
    *pos = USR_Motor_GetPosition(false);
    *vel = USR_Motor_GetVelocity();
    *cur = USR_Motor_GetCurrent();
    *mode = (uint8_t)s_mode_running;
    *state = (uint8_t)s_state;
}

/**
 * @输入 无
 * @输出 累计位置原始值（细分步）
 * @说明 查询实测累计位置（遥测诊断用，非业务接口）
 */
int32_t USR_Motor_GetRawPosition(void)
{
    return s_real_position;
}

/**
 * @输入 无
 * @输出 估计速度原始值（细分步/s）
 * @说明 查询速度估计原始值（遥测诊断用，非业务接口）
 */
int32_t USR_Motor_GetRawVelocity(void)
{
    return s_est_velocity;
}

/**
 * @输入 无
 * @输出 最近一帧位置增量（细分步）
 * @说明 查询最近一帧 Δpos（遥测诊断用，非业务接口）
 */
int32_t USR_Motor_GetRawDelta(void)
{
    int32_t sum = s_delta_sum;
    s_delta_sum = 0;
    return sum;
}

/**
 * @输入 无
 * @输出 true=已校准
 * @说明 查询编码器校准状态（复刻参考 motor.c Motor_IsCalibrated）
 */
bool USR_Motor_IsCalibrated(void)
{
    return USR_EncoderCalibrator_IsCalibrated();
}

/**
 * @输入 无
 * @输出 无
 * @说明 触发编码器校准（复刻参考 motor.c Motor_TriggerCalibration）
 */
void USR_Motor_TriggerCalibration(void)
{
    USR_EncoderCalibrator_Trigger();
}

/**
 * @输入 无
 * @输出 无
 * @说明 把当前位置设为新的零位（更新内存 offset；
 *   落盘待 config_usr/eeprom_usr 落地后接 EEPROM_Write，任务4）
 *   复刻参考 motor.c Motor_ZeroPosition（EEPROM 部分未移植）
 */
void USR_Motor_ZeroPosition(void)
{
    s_config->encoderHomeOffset =
        s_real_position % (int32_t)USR_MOTOR_SUBDIVIDE_STEPS;
}
