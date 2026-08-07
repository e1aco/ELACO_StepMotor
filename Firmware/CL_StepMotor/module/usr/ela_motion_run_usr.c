/*****************************************************************************
 * @文件: ela_motion_run_usr.c
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: 正式运行态实现。以校准表为基准（g_cali_table[enc]=微步），
 ****************************************************************************/

#include "ela_motion_run_usr.h"
#include "ela_mt6816_usr.h"
#include "ela_tb67h450_usr.h"
#include "elaco_calibration_usr.h"
#include "main.h"
#include <stdio.h>

#define MOTION_RUN_CTRL_DIV      1     /* 每个 20kHz tick 控制一次 = 20kHz */
#define MOTION_RUN_KP_SHIFT      6     /* Kp = 1/64 */
#define MOTION_RUN_MAX_DELTA     4     /* 每控制周期最大微步（查表目标逼近），复刻 run_pid 成功配置 */
#define MOTION_RUN_HOLD_MAX_DELTA 2    /* 到位保持最大微步（I+D 同款控制器，限幅调温和） */
#define MOTION_RUN_ERR_ACC_MAX   (MOTION_RUN_MAX_DELTA << MOTION_RUN_KP_SHIFT) /* ±256 防 windup */
#define MOTION_RUN_SLOW_ERR      64    /* 接近目标（编码器计数）时降速至 ±1 */
#define MOTION_RUN_DEADBAND      8     /* 到位死区（编码器计数），兼顾回绕边界摆动与定位精度 */
#define MOTION_RUN_CONFIRM       3     /* 连续带内次数 */
#define MOTION_RUN_ENC_JUMP      256   /* 单 50µs tick 编码器最大合法跳变，超限视为坏值（满速仅 ~2） */
#define MOTION_RUN_HOLD_MA       2000
#define MOTION_RUN_DRIVE_MA      2000

static const unsigned short s_demo_targets[MOTION_RUN_TARGET_NUM] = {
    MOTION_RUN_TGT_0DEG, MOTION_RUN_TGT_90DEG,
    MOTION_RUN_TGT_180DEG, MOTION_RUN_TGT_270DEG
};

/* 运行态状态 */
static volatile unsigned short s_target_enc = 0;
static volatile int s_target_step = 0;      /* 目标微步（table[target]，恒定） */
static volatile int s_cur_step = 0;         /* 当前微步（动态逼近） */
static volatile unsigned short s_last_enc = 0;
static volatile short s_last_err = 0;
static volatile unsigned char s_running = 0;
static volatile unsigned char s_arrived = 0;
static volatile unsigned char s_inband_cnt = 0;

/* 闭环控制器状态（误差累加器 + 速度阻尼） */
static volatile int s_err_acc = 0;
static volatile int s_ctrl_tick = 0;
static volatile unsigned short s_prev_enc = 0;

/* 演示索引 */
static unsigned char s_demo_idx = 0;
static unsigned char s_demo_phase = 0;   /* 0: 前进, 1: 返回 */
static unsigned char s_demo_hold_active = 0;  /* 到位保持计时进行中 */
static unsigned long s_demo_hold_start = 0;   /* 保持起始 tick */
#define MOTION_RUN_DEMO_HOLD_MS  3000  /* 到位保持时间，观察收敛 */

static short motion_run_angle_delta(

/* ==== 全局实例 ==== */
    unsigned short curr, unsigned short prev);

/* ==== 接口实现 ==== */
/********
 * @输出: 编码器角度值（单次读取 + 跳变防护）
 * @说明: 每 20kHz tick 仅调 USR_MT6816_ReadAngle() 一次，
 *         其内部含奇偶校验 + 最多 3 次重试，非法值由 data_valid
 *         标记。另加跳变防护：单 tick（50µs）编码器跳变超
 *         MOTION_RUN_ENC_JUMP 即视为坏值，沿用上次有效值，
 *         防止偶发坏值触发控制器误动（拖拽转子）
 ********/
static unsigned short motion_run_read_median(void)
{
    static unsigned short last_valid = 0;
    static unsigned char have_valid = 0;
    unsigned short raw;
    int jump;

    USR_MT6816_ReadAngle();
    raw = g_mt6816_st.raw_angle;

    if (have_valid)
    {
        jump = motion_run_angle_delta(raw, last_valid);
        if (jump > MOTION_RUN_ENC_JUMP
            || jump < -MOTION_RUN_ENC_JUMP)
        {
            return last_valid;
        }
    }

    last_valid = raw;
    have_valid = 1;
    return raw;
}

/********
 * @输入: curr: 当前编码器值; prev: 前一次编码器值
 * @输出: 差值（考虑 14-bit 回绕）
 * @说明: 计算编码器角度差，自动处理 0->16383 的回绕
 ********/
static short motion_run_angle_delta(
    unsigned short curr, unsigned short prev)
{
    int diff = (int)curr - (int)prev;

    if (diff < -8192)
    {
        diff += ENC_RESOLUTION;
    }
    else if (diff > 8192)
    {
        diff -= ENC_RESOLUTION;
    }

    return (short)diff;
}

/********
 * @说明: 初始化运行态
 ********/
void USR_MotionRun_Init(void)
{
    s_target_enc = 0;
    s_target_step = 0;
    s_cur_step = 0;
    s_last_enc = 0;
    s_last_err = 0;
    s_running = 0;
    s_arrived = 0;
    s_inband_cnt = 0;
    s_err_acc = 0;
    s_ctrl_tick = 0;
    s_prev_enc = 0;
    s_demo_idx = 0;
    s_demo_phase = 0;
}

/********
 * @说明: 启动固定演示动作（0/90/180/270° 往返）
 ********/
void USR_MotionRun_DemoStart(void)
{
    s_demo_idx = 0;
    s_demo_phase = 0;
    USR_MotionRun_GotoTarget(s_demo_targets[s_demo_idx]);
}

/********
 * @输出: true=电机空闲（未运行且已到位）
 * @说明: 供主流程判断"非运行状态"，此时才允许双键触发校准
 ********/
bool USR_MotionRun_IsIdle(void)
{
    return (!s_running) && s_arrived;
}

/********
 * @输出: 当前编码器值
 * @说明: 读取当前编码器（3 次取中值），供主流程/演示调度使用
 ********/
unsigned short USR_MotionRun_CurrentEnc(void)
{
    return g_mt6816_st.raw_angle;
}

/********
 * @输入: target: 目标编码器绝对位置 (0~16383)
 * @说明: 设置运行目标。目标微步 = 校准表直接查表（table[target]），
 *         当前微步 = 表查当前编码器。ISR 中把当前微步以限速
 *         逼近目标微步（跨 0 回绕走短路径），到位后按编码器
 *         误差做闭环微调。若电机已在运行则忽略
 ********/
void USR_MotionRun_GotoTarget(unsigned short target)
{
    unsigned short enc0;

    if (s_running)
    {
        return;
    }

    /* 保持当前磁场不动（FOC 停在到位时的 s_cur_step），
     * 直接读当前位置。切勿先命令到微步 0 再读——那会把
     * 转子拽离平衡点，读到的 enc0 被扰动（经验包：到位后保持闭环） */
    enc0 = motion_run_read_median();
    s_target_enc = target;
    s_target_step = (int)g_cali_table[target];
    s_cur_step = (int)g_cali_table[enc0];
    printf("[GOTO] enc0=%u step0=%d tgt=%u stepT=%d\r\n",
           enc0, s_cur_step, target, s_target_step);
    s_last_enc = 0;
    s_last_err = 0;
    s_inband_cnt = 0;
    s_err_acc = 0;
    s_prev_enc = enc0;
    s_arrived = 0;
    s_running = 1;
}

/********
 * @说明: 运行态控制进程，在 TIM4 20kHz ISR 中调用（由主流程分派）。
 *         每 5 tick（4kHz）读一次编码器（3 次取中值滤 SPI 毛刺），
 *         以绝对目标编码器为基准做误差闭环：误差累加器（I 主导）
 *         + 速度阻尼（D），逼近目标时降速 ±1 微步/周期防过冲，
 *         收敛于死区即判定到位。结构同达标基准 test_position_cl.c
 ********/
void USR_MotionRun_Proc(void)
{
    int err;
    int cmd;
    int vel;
    unsigned short enc;
    int max_delta;
    unsigned int cur_ma;

    /* 从未启动过目标（上电未触发）：不开环 */
    if (!s_running && !s_arrived)
    {
        return;
    }

    enc = motion_run_read_median();
    s_last_enc = enc;

    /* 绝对目标误差：跨 0 回绕取短路径（±8192 内） */
    err = motion_run_angle_delta(s_target_enc, enc);
    s_last_err = (short)err;

    /* 速度：相邻控制周期编码器位移（计数/周期） */
    vel = motion_run_angle_delta(enc, s_prev_enc);
    s_prev_enc = enc;

    if (s_arrived)
    {
        /* 到位保持：磁场基准锚定到校准表磁点 table[enc]（当前编码器
         * 位置对应的零转矩微步）。s_cur_step 原为自由积分器，在 0°/270°
         * 会把磁场停在磁点之间 → 误差持续饱和 → 20kHz 满圈扫掠极限环
         * （enc ±1500 横跳、cur 满圈横扫）；锚定后磁场始终贴住磁点，
         * 仅叠加 ±2 微步比例校正（增益 1/4），带外立即生效温和拉回 */
        if (err < MOTION_RUN_DEADBAND && err > -MOTION_RUN_DEADBAND)
        {
            s_err_acc = 0;
            s_cur_step = (int)g_cali_table[enc];
            USR_TB67H450_SetFocCurrent(
                (unsigned int)s_cur_step, MOTION_RUN_HOLD_MA);
            return;
        }

        cmd = err >> 2;
        if (cmd > MOTION_RUN_HOLD_MAX_DELTA)
        {
            cmd = MOTION_RUN_HOLD_MAX_DELTA;
        }
        else if (cmd < -MOTION_RUN_HOLD_MAX_DELTA)
        {
            cmd = -MOTION_RUN_HOLD_MAX_DELTA;
        }
        s_cur_step = (int)g_cali_table[enc] - cmd;
        if (s_cur_step < 0)
        {
            s_cur_step += MICROSTEPLAP;
        }
        else if (s_cur_step >= MICROSTEPLAP)
        {
            s_cur_step -= MICROSTEPLAP;
        }
        USR_TB67H450_SetFocCurrent(
            (unsigned int)s_cur_step, MOTION_RUN_HOLD_MA);
        return;
    }
    else
    {
        /* 到位判定：带内（|err|<±4）连续 CONFIRM 次。
         * 接近目标已降速至 ±1，扫过死区速度极低，无需速度门 */
        if (err < MOTION_RUN_DEADBAND && err > -MOTION_RUN_DEADBAND)
        {
            if (s_inband_cnt < MOTION_RUN_CONFIRM)
            {
                s_inband_cnt++;
                USR_TB67H450_SetFocCurrent(
                    (unsigned int)s_cur_step, MOTION_RUN_HOLD_MA);
                return;
            }

            s_arrived = 1;
            s_running = 0;
            s_err_acc = 0;
            USR_TB67H450_SetFocCurrent(
                (unsigned int)s_cur_step, MOTION_RUN_HOLD_MA);
            return;
        }

        s_inband_cnt = 0;
        max_delta = MOTION_RUN_MAX_DELTA;
        cur_ma = MOTION_RUN_DRIVE_MA;
    }

    /* 误差累加器（I 主导），带 ±256 上限防 windup */
    s_err_acc += err;
    if (s_err_acc > MOTION_RUN_ERR_ACC_MAX)
    {
        s_err_acc = MOTION_RUN_ERR_ACC_MAX;
    }
    else if (s_err_acc < -MOTION_RUN_ERR_ACC_MAX)
    {
        s_err_acc = -MOTION_RUN_ERR_ACC_MAX;
    }

    cmd = s_err_acc >> MOTION_RUN_KP_SHIFT;

    /* 速度阻尼（D）：抵消转子欠阻尼振荡（极限环） */
    cmd -= (vel >> 1);

    /* 接近目标（|err|<64 计数）时降至 1 微步/周期，
     * 避免高速冲入死区造成过冲-反弹 */
    if (err < MOTION_RUN_SLOW_ERR && err > -MOTION_RUN_SLOW_ERR)
    {
        if (cmd > 1)
        {
            cmd = 1;
        }
        else if (cmd < -1)
        {
            cmd = -1;
        }
    }
    else if (cmd > max_delta)
    {
        cmd = max_delta;
    }
    else if (cmd < -max_delta)
    {
        cmd = -max_delta;
    }

    /* 速度式积分回馈：应用了多少就从累加器减多少（防 windup） */
    s_err_acc -= cmd << MOTION_RUN_KP_SHIFT;

    /* 校准表单调：微步↑ → 编码器↓，故 err>0（需 enc 增）→ cmd>0 → step 减 */
    s_cur_step -= cmd;
    if (s_cur_step < 0)
    {
        s_cur_step += MICROSTEPLAP;
    }
    else if (s_cur_step >= MICROSTEPLAP)
    {
        s_cur_step -= MICROSTEPLAP;
    }
    USR_TB67H450_SetFocCurrent((unsigned int)s_cur_step, cur_ma);
}

/********
 * @说明: 演示调度，在主循环中调用。
 *         到位后保持 MOTION_RUN_DEMO_HOLD_MS（非阻塞，期间
 *         主循环继续输出 [RUN]/[POW_DET] 观察收敛），
 *         然后前进到下一个目标，走完一圈返回起点
 ********/
void USR_MotionRun_DemoPoll(void)
{
    if (!s_arrived)
    {
        return;
    }

    /* 到位时刻：启动保持计时（仅一次） */
    if (!s_demo_hold_active)
    {
        s_demo_hold_active = 1;
        s_demo_hold_start = HAL_GetTick();
        return;
    }

    /* 保持未满：继续观察 */
    if ((unsigned long)(HAL_GetTick() - s_demo_hold_start)
        < MOTION_RUN_DEMO_HOLD_MS)
    {
        return;
    }

    /* 保持结束：前进到下一目标 */
    s_demo_hold_active = 0;

    if (0 == s_demo_phase)
    {
        if (s_demo_idx + 1 < MOTION_RUN_TARGET_NUM)
        {
            s_demo_idx++;
        }
        else
        {
            s_demo_phase = 1;
            s_demo_idx--;
        }
    }
    else
    {
        if (0 == s_demo_idx)
        {
            s_demo_phase = 0;
            s_demo_idx = 0;
        }
        else
        {
            s_demo_idx--;
        }
    }

    USR_MotionRun_GotoTarget(s_demo_targets[s_demo_idx]);
}

/********
 * @说明: 运行态诊断打印，在主循环中定期调用。
 *         输出当前编码器/目标/误差/微步/到位状态
 ********/
void USR_MotionRun_DebugPrint(void)
{
    short err = motion_run_angle_delta(
        g_mt6816_st.raw_angle, s_target_enc);

    printf("[RUN] enc=%u tgt=%u err=%d cur=%d stepT=%d valid=%d running=%d hold=%d\r\n",
           g_mt6816_st.raw_angle, s_target_enc, err,
           s_cur_step, s_target_step,
           g_mt6816_st.data_valid, s_running, s_arrived);
}






