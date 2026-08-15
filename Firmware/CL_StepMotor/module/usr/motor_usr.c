/*****************************************************************************
 * @文件: motor_usr.c
 * @作者: cl
 * @日期: 2026-08-13
 * @版本: v1.0
 * @说明: 电机用户层（串级闭环：编码器 raw×25/8 映射 + 位置环→速度环→电流 +
 *   位置/速度/电流命令 + IIR 速度估计 + 状态机 STOP/FINISH/RUNNING）
 * @平台: STM32F103RET6
 * @依赖: mt6816_usr, tb67h450_usr, encoder_calibrator_usr, cycle_usr
 ****************************************************************************/
#include "motor_usr.h"
#include "mt6816_usr.h"
#include "tb67h450_usr.h"
#include "encoder_calibrator_usr.h"
#include "cycle_usr.h"
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
static int32_t s_est_position = 0;           /* 控制用估计位置（=实测，无超前角补偿，任务6加） */

/* 目标值 */
static int32_t s_goal_position = 0;
static int32_t s_goal_velocity = 0;
static int32_t s_goal_current = 0;
static bool    s_goal_disable = false;
static bool    s_goal_brake = false;

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

static void S_CalcPositionCascade(int32_t goal_pos)
{
    int32_t err = goal_pos - s_est_position;
    int32_t vel_goal;
    int32_t deadband = (int32_t)USR_MOTOR_POS_DEADBAND;

    /* 绕回窗口内死区加大（128→256）：0° 处弹性/毛刺摆动 ±250 > 128 →
       频繁出界驱动 → 摆动自激（8/15 MODE_POSITION 实测 33 行）；
       256 吞掉摆动 → 不出界不驱动 → 自由衰减静止 */
    if ((goal_pos > ((int32_t)USR_MOTOR_SUBDIVIDE_STEPS - (int32_t)USR_MOTOR_POS_WRAP_WIN))
            && (goal_pos < ((int32_t)USR_MOTOR_SUBDIVIDE_STEPS + (int32_t)USR_MOTOR_POS_WRAP_WIN)))
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

        /* 死区外最小推进速度钳位：仅远离死区（|err|>减速窗口）强制 MIN_VEL 推进；
           减速窗口内不钳 → vel_goal=32×err 线性下坡（限斜率整形）→ 低速进入
           死区（~8192@死区边缘）→ 落点无过冲无摆动。
           BUG 记录（8/15）：原 err≠0 全区间钳位 30000 → 30000 冲入死区 → 过冲
           ±150~250 → 弹性摆动自持（90° 24 行 / 0° 33 行，到位抖动大）；
           vel_goal < 假速度（±25000）会被速度环假速度主导 → 但减速窗口内
           位置带动能低速接近（动摩擦小）→ 实测无卡死。
           目标在编码器 0 点绕回窗口内 → 读数毛刺假速度 ±40000 → 用 WRAP 档 */
        if ((err != 0)
                && ((err > (int32_t)USR_MOTOR_POS_MIN_VEL_DS)
                    || (err < -(int32_t)USR_MOTOR_POS_MIN_VEL_DS)))
        {
            int32_t min_vel = (int32_t)USR_MOTOR_POS_MIN_VEL;
            if ((goal_pos > ((int32_t)USR_MOTOR_SUBDIVIDE_STEPS - (int32_t)USR_MOTOR_POS_WRAP_WIN))
                    && (goal_pos < ((int32_t)USR_MOTOR_SUBDIVIDE_STEPS + (int32_t)USR_MOTOR_POS_WRAP_WIN)))
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
        if ((goal_pos > ((int32_t)USR_MOTOR_SUBDIVIDE_STEPS - (int32_t)USR_MOTOR_POS_WRAP_WIN))
                && (goal_pos < ((int32_t)USR_MOTOR_SUBDIVIDE_STEPS + (int32_t)USR_MOTOR_POS_WRAP_WIN)))
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
       微振时 v 每帧大幅波动 → 阻尼反向电流抵消；匀速时 dv≈0 不干预 */
    dvel = s_est_velocity - s_vel_last;
    s_vel_last = s_est_velocity;

    out = (s_config->pidKp * err) >> 10;
    out -= (s_config->pidKd * dvel) >> 10;
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
    s_est_position = 0;             /* 控制用估计位置清零（当前=实测） */
    s_foc_position = 0;             /* FOC 输出电角度清零（细分步） */
    s_foc_current = 0;              /* FOC 输出电流清零（mA） */
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

    s_est_position = s_real_position;   /* 最小闭环无超前角补偿（任务6加） */

    /* 5. 模式切换（请求→运行） */
    if (s_mode_running != s_request_mode)
    {
        s_mode_running = s_request_mode;
        S_ZeroOutput();
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

    /* 7. 控制分派 */
    if (s_goal_disable)
    {
        S_ZeroOutput();
        USR_TB67H450_Sleep();
    }
    else if (s_goal_brake)
    {
        S_ZeroOutput();
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
                S_CalcPositionCascade(s_goal_position);
                break;
            case MODE_COMMAND_VELOCITY:
                S_CalcVelocityP(s_goal_velocity);
                break;
            case MODE_COMMAND_CURRENT:
                S_CalcCurrentToOutput(s_goal_current);
                break;
            default:
                S_ZeroOutput();
                USR_TB67H450_Sleep();
                break;
        }
    }

    /* 8. 状态机（最小闭环：STOP/FINISH/RUNNING，任务6后补过载/堵转/未校准） */
    if (s_mode_running == MODE_STOP)
    {
        s_state = STATE_STOP;
    }
    else if (s_goal_disable || s_goal_brake)
    {
        s_state = STATE_STOP;
    }
    else
    {
        switch (s_mode_running)
        {
            case MODE_COMMAND_POSITION:
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
 * @说明 清堵转标志（最小闭环无堵转检测，预留接口）
 */
void USR_Motor_ClearStallFlag(void)
{
    /* 最小闭环无堵转检测：无实现（任务6后补过载/堵转检测时填充） */
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
