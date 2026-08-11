/********
 * @ 文件: ela_motion_run_usr.c
 * @ 作者: ELACO
 * @ 日期: 2026-08-02
 * @ 版本: 1.0.0
 * @ 说明: 正式运行态实现。以校准表为基准（g_cali_table[enc]=微步），
 *         固定演示动作走到 0/90/180/270°，ISR 中限速逼近目标微步，
 *         到位后用编码器误差做闭环微调
 * @ 依赖: ela_mt6816_usr, ela_tb67h450_usr, elaco_calibration_usr
 ********/

#include "ela_motion_run_usr.h"
#include "ela_mt6816_usr.h"
#include "ela_tb67h450_usr.h"
#include "elaco_calibration_usr.h"
#include "main.h"
#include <stdio.h>

#define MOTION_RUN_CTRL_DIV      1     /* 每个 20kHz tick 控制一次 = 20kHz */
#define MOTION_RUN_KP_SHIFT      6     /* Kp = 1/64 */
#define MOTION_RUN_MAX_DELTA     4     /* 每控制周期最大微步（查表目标逼近），复刻 run_pid 成功配置 */
#define MOTION_RUN_HOLD_MAX_DELTA 32   /* 到位保持最大微步（err 积分补偿静摩擦，磁场可偏 32 微步≈22.5°电角） */
#define MOTION_RUN_ERR_ACC_MAX   (MOTION_RUN_MAX_DELTA << MOTION_RUN_KP_SHIFT) /* ±256 防 windup */
#define MOTION_RUN_SLOW_ERR      128   /* 接近目标（编码器计数）时降速至 ±1 */
#define MOTION_RUN_DEADBAND      8     /* 到位死区（编码器计数），兼顾回绕边界摆动与定位精度 */
#define MOTION_RUN_HOLD_HYST     14    /* 保持态滞回（编码器计数）：|err|<HYST 磁场回中，≥HYST 才积分。滞回>死区防边缘抖动积分饱和 */
#define MOTION_RUN_CONFIRM       3     /* 连续带内次数 */
#define MOTION_RUN_ENC_JUMP      256   /* 单 50µs tick 编码器最大合法跳变，超限视为坏值（满速仅 ~2） */
#define MOTION_RUN_ENC_BAD_MAX   10    /* 编码器连续跳变容忍 tick 数：10 tick(500µs) 内持续跳变才接受为新值 */
#define MOTION_RUN_ERR_DELTA_MAX 60    /* 保持态 err 单 tick 突变上限（编码器计数）：保持态 err 应 ±20 内，突变>60 视为坏读冻结，防触发 REENTER 重入放大成 runaway。回绕边突变(±33)不超 */
#define MOTION_RUN_HOLD_MA       1700
#define MOTION_RUN_DRIVE_MA      1500
#define MOTION_RUN_REENTER_ERR   80    /* 到位保持中 err 超此阈值（编码器计数）→ 撤销到位重入运行态逼近。提高避免 0°/270° 保持力不足的小 err(±20) 触发重入放大成 runaway */
#define MOTION_RUN_ARRIVE_VEL    2     /* 到位判定速度门（编码器计数/50µs tick）：超过即视为惯性冲过死区，不判到位 */

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

/* 触发式诊断缓冲：记录最近 32 tick 的 enc/err/cur，失控(hold→running)瞬间打印 */
#define DBG_RING 32
static unsigned short s_dbg_enc[DBG_RING];
static int s_dbg_err[DBG_RING];
static int s_dbg_cur[DBG_RING];
static unsigned char s_dbg_idx = 0;
static unsigned char s_dbg_cnt = 0;

/* 闭环控制器状态（误差累加器 + 速度阻尼） */
static volatile int s_err_acc = 0;
static volatile int s_ctrl_tick = 0;
static volatile unsigned short s_prev_enc = 0;
static volatile int s_err_prev = 0;      /* 保持态 err 突变抑制基准 */

/* 演示索引 */
static unsigned char s_demo_idx = 0;
static unsigned char s_demo_phase = 0;   /* 0: 前进, 1: 返回 */
static unsigned char s_demo_hold_active = 0;  /* 到位保持计时进行中 */
static unsigned long s_demo_hold_start = 0;   /* 保持起始 tick */
#define MOTION_RUN_DEMO_HOLD_MS  3000  /* 到位保持时间，观察收敛 */

/* motion_run hlp start */

static short motion_run_angle_delta(
    unsigned short curr, unsigned short prev);

/********
 * @ 输出: 编码器角度值（单次读取 + 跳变防护）
 * @ 说明: 每 20kHz tick 仅调 ela_mt6816_usr_read_angle() 一次，
 *         其内部含奇偶校验 + 最多 3 次重试，非法值由 data_valid
 *         标记。另加跳变防护：单 tick（50µs）编码器跳变超
 *         MOTION_RUN_ENC_JUMP 即视为坏值，沿用上次有效值，
 *         防止偶发坏值触发控制器误动（拖拽转子）
 ********/
static unsigned short motion_run_read_median(void)
{
    static unsigned short last_valid = 0;
    static unsigned char have_valid = 0;
    static unsigned char bad_cnt = 0;
    unsigned short raw;
    int jump;

    ela_mt6816_usr_read_angle();
    raw = g_mt6816_st.raw_angle;

    if (have_valid)
    {
        jump = motion_run_angle_delta(raw, last_valid);
        if (jump > MOTION_RUN_ENC_JUMP
            || jump < -MOTION_RUN_ENC_JUMP)
        {
            /* 偶发跳变（MT6816 磁场读数瞬时错，parity 拦不住）：
             * 连续 MOTION_RUN_ENC_BAD_MAX tick 跳变才接受新值，
             * 否则沿用上次有效值，过滤单次/连续几次坏读 */
            if (bad_cnt < MOTION_RUN_ENC_BAD_MAX)
            {
                bad_cnt++;
                return last_valid;
            }
            /* 持续跳变 → 可能是真实快速运动，接受 */
        }
    }

    bad_cnt = 0;
    last_valid = raw;
    have_valid = 1;
    return raw;
}

/********
 * @ 输入: curr: 当前编码器值; prev: 前一次编码器值
 * @ 输出: 差值（考虑 14-bit 回绕）
 * @ 说明: 计算编码器角度差，自动处理 0->16383 的回绕
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

/* motion_run hlp end */
//----------------------------------------------------------------------------------
/* motion_run usr start */

/********
 * @ 说明: 初始化运行态
 ********/
void ela_motion_run_init(void)
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
 * @ 说明: 启动固定演示动作（0/90/180/270° 往返）
 ********/
void ela_motion_run_demo_start(void)
{
    s_demo_idx = 0;   /* 从 0° 开始（0° 磁点修正后验证） */
    s_demo_phase = 0;
    ela_motion_run_goto_target(s_demo_targets[s_demo_idx]);
}

/********
 * @ 输出: true=电机空闲（未运行且已到位）
 * @ 说明: 供主流程判断"非运行状态"，此时才允许双键触发校准
 ********/
bool ela_motion_run_is_idle(void)
{
    return (!s_running) && s_arrived;
}

/********
 * @ 输出: 当前编码器值
 * @ 说明: 读取当前编码器（3 次取中值），供主流程/演示调度使用
 ********/
unsigned short ela_motion_run_current_enc(void)
{
    return g_mt6816_st.raw_angle;
}

/********
 * @ 输入: target: 目标编码器绝对位置 (0~16383)
 * @ 说明: 设置运行目标。目标微步 = 校准表直接查表（table[target]），
 *         当前微步 = 表查当前编码器。ISR 中把当前微步以限速
 *         逼近目标微步（跨 0 回绕走短路径），到位后按编码器
 *         误差做闭环微调。若电机已在运行则忽略
 ********/
void ela_motion_run_goto_target(unsigned short target)
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
    s_err_prev = 0;
    s_arrived = 0;
    s_running = 1;
}

/********
 * @ 说明: 运行态控制进程，在 TIM4 20kHz ISR 中调用（由主流程分派）。
 *         每 5 tick（4kHz）读一次编码器（3 次取中值滤 SPI 毛刺），
 *         以绝对目标编码器为基准做误差闭环：误差累加器（I 主导）
 *         + 速度阻尼（D），逼近目标时降速 ±1 微步/周期防过冲，
 *         收敛于死区即判定到位。结构同达标基准 test_position_cl.c
 ********/
void ela_motion_run_proc(void)
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

    /* 触发式诊断缓冲：记录 enc/err/cur 环形缓冲 */
    s_dbg_enc[s_dbg_idx] = enc;
    s_dbg_err[s_dbg_idx] = err;
    s_dbg_cur[s_dbg_idx] = s_cur_step;
    s_dbg_idx = (s_dbg_idx + 1) % DBG_RING;
    if (s_dbg_cnt < DBG_RING) s_dbg_cnt++;

    if (s_arrived)
    {
        /* 保持态 err 突变抑制：保持态 err 应极小（±20 内），若某 tick 突变
         * 超限（编码器坏读漏过 read_median），冻结 err 沿用上次值，防止
         * 积分器放大成 runaway（0°/270° 偶发失控根因）。运行态 GOTO 逼近
         * err 大是正常的，不做此抑制 */
        if (err - s_err_prev > MOTION_RUN_ERR_DELTA_MAX
            || err - s_err_prev < -MOTION_RUN_ERR_DELTA_MAX)
        {
            err = s_err_prev;
        }
    }
    s_err_prev = err;

    if (s_arrived)
    {
        /* 0° 回绕边特判：校准表 table[0] 基准与真 0° 磁点有偏移（实测
         * stepT 26 vs 真磁点 220），保持态回中 stepT 会把转子拉偏 62 计数
         * → 反复重入 runaway。改为保留到位时磁场 s_cur_step（err 微调），
         * 不重置到错误的 stepT */
        if (s_target_enc == 0 || s_target_enc > (ENC_RESOLUTION - 200))
        {
            if (err > MOTION_RUN_REENTER_ERR || err < -MOTION_RUN_REENTER_ERR)
            {
                s_arrived = 0;
                s_running = 1;
                s_err_acc = 0;
                s_inband_cnt = 0;
            }
            else if (err < MOTION_RUN_DEADBAND && err > -MOTION_RUN_DEADBAND)
            {
                s_err_acc = 0;
                ela_tb67h450_set_foc_current(
                    (unsigned int)s_cur_step, MOTION_RUN_HOLD_MA);
                return;
            }
            else
            {
                s_err_acc += err;
                if (s_err_acc > MOTION_RUN_ERR_ACC_MAX)
                    s_err_acc = MOTION_RUN_ERR_ACC_MAX;
                else if (s_err_acc < -MOTION_RUN_ERR_ACC_MAX)
                    s_err_acc = -MOTION_RUN_ERR_ACC_MAX;

                int c = (s_err_acc >> MOTION_RUN_KP_SHIFT) * 2;
                if (c > MOTION_RUN_HOLD_MAX_DELTA) c = MOTION_RUN_HOLD_MAX_DELTA;
                else if (c < -MOTION_RUN_HOLD_MAX_DELTA) c = -MOTION_RUN_HOLD_MAX_DELTA;
                s_cur_step -= c;
                if (s_cur_step < 0) s_cur_step += MICROSTEPLAP;
                else if (s_cur_step >= MICROSTEPLAP) s_cur_step -= MICROSTEPLAP;
                ela_tb67h450_set_foc_current(
                    (unsigned int)s_cur_step, MOTION_RUN_HOLD_MA);
                return;
            }
        }

        /* 到位保持：磁场以目标磁点 stepT 为基准，err 积分补偿静摩擦。
         * 旧实现锚定 table[enc]（当前编码器零转矩点）+ clamp ±8 微步 →
         * 磁场停在不平衡位置，稳态 err 残留。新实现：
         *  - 死区内（|err|<±8）：磁场回中 stepT（零转矩），达标保持；
         *  - 死区外：err 积分（I 项补偿静摩擦）+ 限速逼近，把编码器拉回目标。
         * 大漂移重入：回绕边漂移（0° 跨 16384 等，err 可达数百计数）远超保持
         * clamp 能力，撤销到位回运行态用完整积分器逼近（无 ±24 限制） */
        if (err > MOTION_RUN_REENTER_ERR || err < -MOTION_RUN_REENTER_ERR)
        {
            /* 触发诊断：打印最近 32 tick 的 enc/err/cur，定位失控源头 */
            unsigned char i = (s_dbg_idx + DBG_RING - s_dbg_cnt) % DBG_RING;
            for (unsigned char k = 0; k < s_dbg_cnt; k++)
            {
                printf("[DBG] %u: enc=%u err=%d cur=%d\r\n",
                       k, s_dbg_enc[i], s_dbg_err[i], s_dbg_cur[i]);
                i = (i + 1) % DBG_RING;
            }
            s_arrived = 0;
            s_running = 1;
            s_err_acc = 0;
            s_inband_cnt = 0;
        }
        else if (err < MOTION_RUN_HOLD_HYST && err > -MOTION_RUN_HOLD_HYST)
        {
            /* 死区（含滞回）：|err|<HYST 磁场回中 stepT（零转矩保持）。
             * 用 HYST(14) > DEADBAND(8) 的滞回避免死区边缘 err 抖动反复
             * 触发积分分支 → 积分器饱和 → runaway */
            s_err_acc = 0;
            s_cur_step = (int)s_target_step;
            ela_tb67h450_set_foc_current(
                (unsigned int)s_cur_step, MOTION_RUN_HOLD_MA);
            return;
        }
        else
        {
            s_err_acc += err;
            if (s_err_acc > MOTION_RUN_ERR_ACC_MAX / 2)
            {
                s_err_acc = MOTION_RUN_ERR_ACC_MAX / 2;
            }
            else if (s_err_acc < -(MOTION_RUN_ERR_ACC_MAX / 2))
            {
                s_err_acc = -(MOTION_RUN_ERR_ACC_MAX / 2);
            }

            cmd = (s_err_acc >> MOTION_RUN_KP_SHIFT) * 2;
            if (cmd > MOTION_RUN_HOLD_MAX_DELTA)
            {
                cmd = MOTION_RUN_HOLD_MAX_DELTA;
            }
            else if (cmd < -MOTION_RUN_HOLD_MAX_DELTA)
            {
                cmd = -MOTION_RUN_HOLD_MAX_DELTA;
            }
            s_cur_step = (int)s_target_step - cmd;
            if (s_cur_step < 0)
            {
                s_cur_step += MICROSTEPLAP;
            }
            else if (s_cur_step >= MICROSTEPLAP)
            {
                s_cur_step -= MICROSTEPLAP;
            }
            ela_tb67h450_set_foc_current(
                (unsigned int)s_cur_step, MOTION_RUN_HOLD_MA);
            return;
        }
    }
    else
    {
        /* 到位判定：带内（|err|<±8）且速度足够低（|vel|≤2 计数/tick）连续 CONFIRM 次。
         * 加 vel 速度门：编码器惯性冲过死区时 err 短暂带内但 vel 大，不判到位，
         * 待转子真正停住（vel≈0）再切保持，避免到位后惯性漂移触发重入振荡 */
        if (err < MOTION_RUN_DEADBAND && err > -MOTION_RUN_DEADBAND
            && vel <= MOTION_RUN_ARRIVE_VEL && vel >= -MOTION_RUN_ARRIVE_VEL)
        {
            if (s_inband_cnt < MOTION_RUN_CONFIRM)
            {
                s_inband_cnt++;
                ela_tb67h450_set_foc_current(
                    (unsigned int)s_cur_step, MOTION_RUN_HOLD_MA);
                return;
            }

            s_arrived = 1;
            s_running = 0;
            s_err_acc = 0;
            ela_tb67h450_set_foc_current(
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
    ela_tb67h450_set_foc_current((unsigned int)s_cur_step, cur_ma);
}

/********
 * @ 说明: 演示调度，在主循环中调用。
 *         到位后保持 MOTION_RUN_DEMO_HOLD_MS（非阻塞，期间
 *         主循环继续输出 [RUN]/[POW_DET] 观察收敛），
 *         然后前进到下一个目标，走完一圈返回起点
 ********/
void ela_motion_run_demo_poll(void)
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

    ela_motion_run_goto_target(s_demo_targets[s_demo_idx]);
}

/********
 * @ 说明: 运行态诊断打印，在主循环中定期调用。
 *         输出当前编码器/目标/误差/微步/到位状态
 ********/
void ela_motion_run_debug_print(void)
{
    short err = motion_run_angle_delta(
        g_mt6816_st.raw_angle, s_target_enc);

    printf("[RUN] enc=%u tgt=%u err=%d cur=%d stepT=%d valid=%d running=%d hold=%d\r\n",
           g_mt6816_st.raw_angle, s_target_enc, err,
           s_cur_step, s_target_step,
           g_mt6816_st.data_valid, s_running, s_arrived);
}

/* motion_run usr end */
