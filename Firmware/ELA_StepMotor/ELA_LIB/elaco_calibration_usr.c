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
    unsigned int result_num = 0;

    ela_stockfile_usr_erase(&g_stockfile_cali_st);
    ela_stockfile_usr_seq_write_begin(&g_stockfile_cali_st);

    /* 逐编码器生成：对每个编码器 e=0..16383，找覆盖它的校准段 i
     * （avg[i]→avg[i+1]，编码器降序），线性插值微步。
     * 段低 avg[i+1] ↔ (i+1)*256，段高 avg[i] ↔ i*256（校准第 i 整步微步）。
     * 跨环绕段（avg[i] 低≈0、avg[i+1] 高≈16384）经 cyclecal_diff 回绕，
     * 使编码器 0 的微步确定（消除旧实现 0° 磁点偏移/随机） */
    for (int e = 0; e < ENC_RESOLUTION; e++)
    {
        unsigned int micro = 0;

        for (int i = 0; i < WHOLESTEPLAP; i++)
        {
            int e_hi = g_calibra_st.avg_fr_data[i];
            int e_lo = g_calibra_st.avg_fr_data[i + 1];
            int span = cyclecal_diff(e_lo, e_hi, ENC_RESOLUTION);
            if (span <= 0) span += ENC_RESOLUTION;

            /* e 距段低 avg[i+1] 的升序偏移（跨环绕回绕到 [0, span]） */
            int d = cyclecal_diff(e_lo, e, ENC_RESOLUTION);
            if (d < 0) d += ENC_RESOLUTION;

            if (d <= span)
            {
                micro = cyclecal_mod(
                    (unsigned int)((i + 1) * SOFT_DIVIDE)
                    - (unsigned int)((d * SOFT_DIVIDE) / span),
                    MICROSTEPLAP);
                break;
            }
        }

        ela_stockfile_usr_seq_write_next(
            &g_stockfile_cali_st, (unsigned short)micro);
        result_num++;
    }

    ela_stockfile_usr_seq_write_end(&g_stockfile_cali_st);

    if (ENC_RESOLUTION == result_num)
        g_calibra_st.calitable_flag = true;
    else
        g_calibra_st.data_err = 4;
}

/********
 * @ 说明: 0° 磁点修正。校准表 table[0] 基于校准环绕段 avg（随校准起点
 *         随机），与真 0° 磁点有偏移（实测磁场停 table[0] 时编码器 ≠0）。
 *         修正方法：磁场停在 table[0]，读编码器 e0，真磁点 m0=table[0]+e0*3
 *         （负斜率 3.125≈3），全表平移 shift 重写 Flash，使 table[0]=真磁点。
 *         平移不改表斜率 → 90/180/270 闭环到位不受影响（编码器目标不变）
 ********/
static void calibration_zero_offset_fix(void)
{
    static uint16_t tmp[ENC_RESOLUTION];
    int e0 = 0, m0, shift, i;

    ela_tb67h450_set_foc_current(g_cali_table[0], 2000);
    HAL_Delay(300);                          /* 磁场停稳，转子到位 */

    for (i = 0; i < 8; i++)                  /* 多次读取取环绕平均 */
    {
        ela_mt6816_usr_read_angle();
        e0 = cyclecal_avg_two(
            (unsigned short)e0, g_mt6816_st.raw_angle,
            ENC_RESOLUTION);
        HAL_Delay(10);
    }
    if (e0 > 8192) e0 -= ENC_RESOLUTION;     /* 0°±两侧 */

    if (e0 < -4000 || e0 > 4000)
    {
        printf("[CALI] zero fix skip: tbl0=%u e0=%d (|e0|>4000)\r\n",
               g_cali_table[0], e0);
        return;                              /* 偏离 0° 过远，不修 */
    }

    m0 = (int)cyclecal_mod((int)g_cali_table[0] + e0 * 3, MICROSTEPLAP);
    shift = cyclecal_diff((int)g_cali_table[0], m0, MICROSTEPLAP);  /* m0 - table[0] */
    if (shift == 0)
    {
        return;
    }

    ela_stockfile_usr_read(&g_stockfile_cali_st, tmp, ENC_RESOLUTION);
    for (i = 0; i < ENC_RESOLUTION; i++)
    {
        tmp[i] = (uint16_t)cyclecal_mod((int)tmp[i] + shift, MICROSTEPLAP);
    }
    ela_stockfile_usr_erase(&g_stockfile_cali_st);
    ela_stockfile_usr_write(&g_stockfile_cali_st, tmp, ENC_RESOLUTION);
    printf("[CALI] zero fix: tbl0=%u e0=%d m0=%d shift=%d\r\n",
           g_cali_table[0], e0, m0, shift);
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
            calibration_zero_offset_fix();
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

