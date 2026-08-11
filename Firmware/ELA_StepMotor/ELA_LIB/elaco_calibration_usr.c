/********
 * @ 文件: elaco_calibration_usr.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-27
 * @ 版本: 1.0.0
 * @ 说明: 编码器校准模块，数据采集与校准表生成
 * @ 依赖: ela_mt6816_usr, ela_tb67h450_usr, ela_cyclecal,
 *         ela_stockfile_usr, ela_stockfile_drv
 ********/

#include "elaco_calibration_usr.h"
#include "ela_mt6816_usr.h"
#include "ela_tb67h450_usr.h"
#include "ela_cyclecal.h"
#include "ela_stockfile_usr.h"
#include "ela_stockfile_drv.h"
#include "tim.h"

#include <stdio.h>

static unsigned short s_forward_data[WHOLESTEPLAP + 1];
static unsigned short s_reverse_data[WHOLESTEPLAP + 1];

CALIBRATION_DATA_T g_calibra_st = {CALI_STEP_IDLE, false, 0};
unsigned short *g_cali_table =
    (unsigned short *)STOCKFILE_CALI_ADDR;

/* elaco_calibration usr start */

/********
 * @ 说明: 校准数据采集进程，在 20kHz 定时器中断中调用。
 *         纯开环状态机（参考 EncoderCalibrator_Tick20kHz）：
 *         1 预转一圈建立运动 → 2 正转采样一圈 → 3 越界 20 整步
 *         → 4 回退消除间隙 → 5 反转采样一圈 → 6 停止进 CHECK。
 *         采样点每 SOFT_DIVIDE 停住采 SAMPLE_PER_STEP 次取循环平均。
 *         绝对微步以 MICROSTEPLAP(51200) 为测量基准，正向到
 *         2*MICROSTEPLAP 结束，故 pos_set 全程无符号不下溢。
 ********/
void elaco_calibration_proc(void)
{
    static unsigned char cali_state = 0;
    static unsigned int pos_set = 0;
    static unsigned short sample_raw[SAMPLE_PER_STEP];
    static unsigned char sample_cnt = 0;

    ela_mt6816_usr_read_angle();

    switch (g_calibra_st.cali_step)
    {
        case CALI_STEP_COLLECT:
            /* 触发入口：空闲态初始化为测量基准位置 */
            if (0 == cali_state)
            {
                g_calibra_st.reset_microstep = MICROSTEPLAP;
                pos_set = MICROSTEPLAP;
                sample_cnt = 0;
                cali_state = 1;
            }
            break;

        case CALI_STEP_CHECK:
            ela_tb67h450_set_foc_current(0, 0);
            return;

        default:
            return;
    }

    switch (cali_state)
    {
        /* 1 正向准备：从测量基准快跑一整圈（AUTO_SPEED），
         * 建立运动与齿轮啮合方向，再回基准开始测量 */
        case 1:
            pos_set += AUTO_SPEED;
            ela_tb67h450_set_foc_current(pos_set, 2000);
            if (pos_set == (2 * MICROSTEPLAP))
            {
                pos_set = MICROSTEPLAP;
                cali_state = 2;
            }
            break;

        /* 2 正向测量：正转一整圈，每 SOFT_DIVIDE 停住采
         * SAMPLE_PER_STEP 次取循环平均，存入正向数据数组 */
        case 2:
            if (0 == (pos_set % SOFT_DIVIDE))
            {
                sample_raw[sample_cnt++] =
                    g_mt6816_st.raw_angle;
                if (SAMPLE_PER_STEP == sample_cnt)
                {
                    unsigned int idx =
                        (pos_set - MICROSTEPLAP)
                        / SOFT_DIVIDE;
                    s_forward_data[idx] =
                        (unsigned short)cyclecal_avg_array(
                            sample_raw, SAMPLE_PER_STEP,
                            ENC_RESOLUTION);
                    sample_cnt = 0;
                    pos_set += FINE_SPEED;
                }
            }
            else
            {
                pos_set += FINE_SPEED;
            }
            ela_tb67h450_set_foc_current(pos_set, 2000);
            if (pos_set > (2 * MICROSTEPLAP))
            {
                cali_state = 3;
            }
            break;

        /* 3 越界：继续前进 20 整步（消除齿轮间隙量程） */
        case 3:
            pos_set += FINE_SPEED;
            ela_tb67h450_set_foc_current(pos_set, 2000);
            if (pos_set == (2 * MICROSTEPLAP + SOFT_DIVIDE * 20))
            {
                cali_state = 4;
            }
            break;

        /* 4 回差消除：反向退回终点，保证反转测量从同一方向切入 */
        case 4:
            pos_set -= FINE_SPEED;
            ela_tb67h450_set_foc_current(pos_set, 2000);
            if (pos_set == (2 * MICROSTEPLAP))
            {
                cali_state = 5;
            }
            break;

        /* 5 反向测量：反转一整圈，每 SOFT_DIVIDE 停住采
         * SAMPLE_PER_STEP 次取循环平均，存入反向数据数组 */
        case 5:
            if (0 == (pos_set % SOFT_DIVIDE))
            {
                sample_raw[sample_cnt++] =
                    g_mt6816_st.raw_angle;
                if (SAMPLE_PER_STEP == sample_cnt)
                {
                    unsigned int idx =
                        (pos_set - MICROSTEPLAP)
                        / SOFT_DIVIDE;
                    s_reverse_data[idx] =
                        (unsigned short)cyclecal_avg_array(
                            sample_raw, SAMPLE_PER_STEP,
                            ENC_RESOLUTION);
                    sample_cnt = 0;
                    pos_set -= FINE_SPEED;
                }
            }
            else
            {
                pos_set -= FINE_SPEED;
            }
            ela_tb67h450_set_foc_current(pos_set, 2000);
            if (pos_set < MICROSTEPLAP)
            {
                cali_state = 6;
            }
            break;

        /* 6 停止电机，复位采集状态，进入 CHECK */
        case 6:
            ela_tb67h450_set_foc_current(0, 0);
            cali_state = 0;
            g_calibra_st.cali_step = CALI_STEP_CHECK;
            break;
    }
}

/* elaco_calibration hlp start */

/********
 * @ 输入: data: 数据数组; len: 数据长度; direction: 方向
 *         (1: 正向, -1: 反向)
 * @ 输出: true 表示数据连续，false 表示数据异常
 * @ 说明: 检查校准数据的连续性，确保相邻点差值在合理范围
 ********/
static bool calibration_check_continuity(
    const unsigned short *data, unsigned int len,
    int direction)
{
    for (unsigned int i = 1; i < len; i++)
    {
        /* 与 direction 同向：取前向差 data[i] - data[i-1]
         * （硬件约定编码器随 pos_set 递增而递减，故前向差为负） */
        int cyc_diff = cyclecal_diff(
            data[i - 1], data[i], ENC_RESOLUTION);
        int abs_diff = (cyc_diff > 0) ? cyc_diff : -cyc_diff;

        if (abs_diff > (ENC_WHOLESTEP * 3 / 2))
        {
            g_calibra_st.data_err = 2;
            return false;
        }
        if (abs_diff < (ENC_WHOLESTEP * 1 / 2))
        {
            g_calibra_st.data_err = 2;
            return false;
        }
        if (0 == cyc_diff)
        {
            g_calibra_st.data_err = 2;
            return false;
        }
        if ((cyc_diff > 0) != (direction > 0))
        {
            g_calibra_st.data_err = 3;
            return false;
        }
    }
    return true;
}

/********
 * @ 输入: data: 数据数组; len: 数据长度
 * @ 输出: true 表示找到跳跃点
 * @ 说明: 查找编码器旋转过程中 0→16383 的跳跃点位置。
 *         硬件约定：编码器值随微步递增而递减（见复位闭环注释），
 *         故环绕为低侧(≈0)跳到高侧(≈16383)
 ********/
static bool calibration_find_jump_point(
    const unsigned short *data, unsigned int len)
{
    for (unsigned int i = 0; i < len - 1; i++)
    {
        unsigned int curr = data[i];
        unsigned int next = data[i + 1];

        if (curr < (ENC_RESOLUTION / 4)
            && next > (ENC_RESOLUTION * 3 / 4))
        {
            g_calibra_st.jump_pot = (unsigned char)i;
            g_calibra_st.jump_pot_data = curr;
            return true;
        }
    }
    g_calibra_st.data_err = 3;
    return false;
}

/********
 * @ 说明: 检查校准数据，计算正反向平均值、确定旋转方向、
 *         验证数据连续性、查找跳跃点
 ********/
static void calibration_check_data(void)
{
    unsigned int point_count = WHOLESTEPLAP + 1;
    g_calibra_st.data_err = 0;

    for (unsigned int i = 0; i < point_count; i++)
    {
        g_calibra_st.avg_fr_data[i] = cyclecal_avg_two(
            s_forward_data[i], s_reverse_data[i],
            ENC_RESOLUTION);
    }

    /* 方向由相邻采样点判定：avg[1] 相对 avg[0] 的环绕差。
     * 不能用 avg[0] vs avg[WHOLESTEPLAP]，二者是同一物理位置
     * （差一整圈），差值恒 0 */
    int cyc_diff = cyclecal_diff(
        g_calibra_st.avg_fr_data[0],
        g_calibra_st.avg_fr_data[1],
        ENC_RESOLUTION);

    if (0 == cyc_diff)
    {
        g_calibra_st.data_err = 3;
        return;
    }

    int direction = (cyc_diff > 0) ? 1 : -1;

    if (!calibration_check_continuity(
            g_calibra_st.avg_fr_data, point_count,
            direction))
    {
        return;
    }

    if (!calibration_find_jump_point(
            g_calibra_st.avg_fr_data, point_count))
    {
        return;
    }
}

/********
 * @ 说明: 串口打印采集到的正反编码器数据，用于上位机验证
 *         格式: F=正转采样值 R=反转采样值，一行一个采样点
 ********/
static void calibration_print_data(void)
{
    printf("--- Calibration Collect Done ---\r\n");
    printf("reset_microstep=%u\r\n", g_calibra_st.reset_microstep);
    for (unsigned int i = 0; i <= WHOLESTEPLAP; i++)
    {
        printf("P[%3u] F=%5u R=%5u\r\n",
               i, s_forward_data[i], s_reverse_data[i]);
    }
}

/********
 * @ 说明: 根据校准数据生成校准表，线性插值后写入 Flash。
 *         校准表以编码器值为索引，微步值为内容，共 16384 个
 *         halfword，写入 STOCKFILE_CALI_ADDR 分区。
 *         硬件约定编码器随微步递减，环绕为 0→16383，故从环绕段
 *         起点开始按编码器升序扫描各整步段并线性插值写入
 ********/
static void calibration_generate_table(void)
{
    int jump = g_calibra_st.jump_pot;
    unsigned int result_num = 0;

    ela_stockfile_usr_erase(&g_stockfile_cali_st);
    ela_stockfile_usr_seq_write_begin(&g_stockfile_cali_st);

    /* 预计算环绕段（jump 点）：环绕低侧值 avg[jump]（≈0），环绕高侧
     * 值 avg[jump+1]（≈16384）。环绕段高侧终点（enc 16383）的微步，
     * 用于低侧起点连续化（消除回绕处表跳变，0° runaway 根因） */
    int jp = (int)g_calibra_st.jump_pot;
    int e_hi_wrap = g_calibra_st.avg_fr_data[jp];
    int e_lo_wrap = g_calibra_st.avg_fr_data[
        cyclecal_mod(jp + 1, WHOLESTEPLAP)];
    int dec_wrap = cyclecal_diff(e_lo_wrap, e_hi_wrap, ENC_RESOLUTION);
    if (dec_wrap <= 0) dec_wrap += ENC_RESOLUTION;
    unsigned int m_wrap = cyclecal_mod(
        g_calibra_st.reset_microstep
        + (unsigned int)(jp * SOFT_DIVIDE), MICROSTEPLAP);
    int d_wrap_end = cyclecal_diff(
        ENC_RESOLUTION - 1, e_lo_wrap, ENC_RESOLUTION);
    unsigned int m_wrap_end = cyclecal_mod(
        m_wrap + (unsigned int)((d_wrap_end * SOFT_DIVIDE) / dec_wrap),
        MICROSTEPLAP);

    /* 从环绕段 jump 起，按编码器升序遍历 200+1 个整步段。
     * 环绕段拆两半：先写起点 e=e_hi，其余高侧留到最后补写，
     * 保证整体严格按 encoder 值 0..16383 递增写表 */
    for (int off = 0; off <= WHOLESTEPLAP; off++)
    {
        /* jump - off 可为负，须先加回一圈再取模，否则负值经
         * cyclecal_mod(unsigned) 隐式转换得到巨大数，k 错位
         * （如 -1 % 200 → 95 而非 199），导致表后半圈偏移 */
        int k = jump - off;

        if (k < 0)
        {
            k += WHOLESTEPLAP;
        }
        int e_hi = g_calibra_st.avg_fr_data[k];
        int e_lo = g_calibra_st.avg_fr_data[
            cyclecal_mod(k + 1, WHOLESTEPLAP)];
        int dec = cyclecal_diff(e_lo, e_hi, ENC_RESOLUTION);
        unsigned int m_hi;
        int e_start, e_end;

        if (dec <= 0)
        {
            dec += ENC_RESOLUTION;
        }

        /* 段起点微步 = 环绕段基准 + 整步号 * 每整步微步 */
        m_hi = cyclecal_mod(
            g_calibra_st.reset_microstep
            + (unsigned int)(k * SOFT_DIVIDE),
            MICROSTEPLAP);

        if (0 == off)
        {
            /* 环绕段低侧：编码器 0..e_hi（约 0），微步从段起点内插 */
            e_start = 0;
            e_end = e_hi;
        }
        else if (WHOLESTEPLAP == off)
        {
            /* 环绕段高侧：编码器 e_lo+1..16383（约 16383 侧）。
             * 该段 e_hi 是环绕低侧值（≈0），不能作高侧插值基准。
             * d 须相对 e_lo（高侧本段编码器低界）计算，否则斜率
             * 错位（实测 enc 16350..16383 斜率仅 -1.06 vs 正常 -3.125） */
            e_start = e_lo + 1;
            e_end = ENC_RESOLUTION - 1;
        }
        else
        {
            /* 整段：编码器 e_lo+1..e_hi，升序 */
            e_start = e_lo + 1;
            e_end = e_hi;
        }

        /* 按编码器升序写入，保证表索引 == 编码器值 */
        for (int e = e_start; e <= e_end; e++)
        {
            unsigned int val;

            if (0 == off && e_end > 0)
            {
                /* 环绕段低侧连续化：enc 0 起点 = 环绕高侧终点
                 * (enc 16383) 微步 + 1 计数微步（消除回绕处表跳变，
                 * 0° runaway 根因），线性过渡到环绕点微步 m_hi */
                int val0 = (int)cyclecal_mod(
                    m_wrap_end + 3, MICROSTEPLAP);  /* 1 计数≈3 微步 */
                int delta = ((int)m_hi - val0) * e / e_end;
                val = cyclecal_mod((unsigned int)(val0 + delta),
                                   MICROSTEPLAP);
            }
            else
            {
                /* 段内距段起点沿递减方向的计数。
                 * 环绕段高侧（off==WHOLESTEPLAP）以 e_lo 为基准（该段
                 * 编码器低界），其余段以 e_hi 为基准（段内编码器高界） */
                int base = (WHOLESTEPLAP == off) ? e_lo : e_hi;
                int d = cyclecal_diff(e, base, ENC_RESOLUTION);
                val = cyclecal_mod(
                    m_hi
                    + (unsigned int)((d * SOFT_DIVIDE) / dec),
                    MICROSTEPLAP);
            }

            ela_stockfile_usr_seq_write_next(
                &g_stockfile_cali_st, (unsigned short)val);
            result_num++;
        }
    }

    ela_stockfile_usr_seq_write_end(&g_stockfile_cali_st);

    if (ENC_RESOLUTION == result_num)
        g_calibra_st.calitable_flag = true;
    else
        g_calibra_st.data_err = 4;
}

/* elaco_calibration hlp end */
//----------------------------------------------------------------------------------
/* elaco_calibration usr start */

/********
 * @ 说明: 校准模块初始化，复位状态为 IDLE。
 *         校准表有效性由上电检查 (elaco_calibration_table_data_valid)
 *         决定，有效则直接进入运行态，无效则等待按键触发
 ********/
void elaco_calibration_init(void)
{
    g_calibra_st.cali_step = CALI_STEP_IDLE;
    g_calibra_st.calitable_flag = false;
    g_calibra_st.data_err = 0;
}

/********
 * @ 说明: 手动触发校准（双键长按等入口调用）。
 *         先擦除校准分区防止旧表进入运行态，再置复位对齐状态
 ********/
void elaco_calibration_start(void)
{
    g_calibra_st.calitable_flag = false;
    g_calibra_st.cali_step = CALI_STEP_COLLECT;
    ela_stockfile_usr_erase(&g_stockfile_cali_st);

    /* 校准由 20kHz TIM4 中断驱动，需在此启动 PWM 与中断 */
    ela_mt6816_usr_init();
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
    HAL_TIM_Base_Start_IT(&htim4);
}

/********
 * @ 说明: 上电检查 Flash 中校准表是否有效
 ********/
void elaco_calibration_table_data_valid(void)
{
    uint16_t first = g_cali_table[0];
    if ((0xFFFF != first) && (0 != first))
    {
        g_calibra_st.calitable_flag = true;
    }
}

/********
 * @ 说明: 校准表生成主进程，在主循环中调用。
 *         状态机：IDLE → COLLECT(由中断处理) → CHECK
 *         → GENERATE → DONE
 *         IDLE 不自动开始，须由 elaco_calibration_start()
 *         （双键长按等）触发进入 COLLECT
 ********/
void elaco_calibration_table_generate_proc(void)
{
    switch (g_calibra_st.cali_step)
    {
        case CALI_STEP_IDLE:
            break;

        case CALI_STEP_CHECK:
            calibration_print_data();
            calibration_check_data();
            if (0 == g_calibra_st.data_err)
            {
                g_calibra_st.cali_step = CALI_STEP_GENERATE;
            }
            else
            {
                g_calibra_st.cali_step = CALI_STEP_DONE;
            }
            break;

        case CALI_STEP_GENERATE:
            calibration_generate_table();
            g_calibra_st.cali_step = CALI_STEP_DONE;
            break;

        case CALI_STEP_COLLECT:
            break;

        case CALI_STEP_DONE:
            /* 校准结束：回到 IDLE，供上电/主循环状态机进入正式运行态。
             * 此前 DONE 分支直接 break 导致 cali_step 永久卡 DONE，
             * demo/正式模式永不启动（表现为 tgt=0 stepT=0 全默认值） */
            g_calibra_st.cali_step = CALI_STEP_IDLE;
            break;

        default:
            g_calibra_st.cali_step = CALI_STEP_IDLE;
            break;
    }
}

/* elaco_calibration usr end */

