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

static unsigned short s_forward_data[WHOLESTEPLAP + 1];
static unsigned short s_reverse_data[WHOLESTEPLAP + 1];

CALIBRATION_DATA_T g_calibra_st = {CALI_STEP_IDLE, false, 0};
unsigned short *g_cali_table =
    (unsigned short *)STOCKFILE_CALI_ADDR;

/* elaco_calibration usr start */

/********
 * @ 说明: 校准数据采集进程，在 20kHz 定时器中断中调用。
 *         控制电机正反转并采集编码器原始数据
 ********/
void elaco_calibration_proc(void)
{
    static unsigned char cali_state = 0;
    static unsigned int pos_set = 0;
    static unsigned short sample_raw[SAMPLE_PER_STEP];
    static unsigned char sample_cnt = 0;

    ela_mt6816_usr_read_angle();

    switch (cali_state)
    {
        case 0:
            if (CALI_STEP_COLLECT == g_calibra_st.cali_step)
            {
                ela_tb67h450_set_foc_current(pos_set, 2000);
                pos_set = MICROSTEPLAP;
                cali_state = 1;
            }
            break;

        case 1:
            pos_set += AUTO_SPEED;
            ela_tb67h450_set_foc_current(pos_set, 2000);
            if (pos_set >= (2 * MICROSTEPLAP))
            {
                pos_set = MICROSTEPLAP;
                cali_state = 2;
            }
            break;

        case 2:
            if (0 == (pos_set % SOFT_DIVIDE))
            {
                sample_raw[sample_cnt++] =
                    g_mt6816_st.raw_data;
                if (SAMPLE_PER_STEP == sample_cnt)
                {
                    unsigned int idx =
                        (pos_set - MICROSTEPLAP)
                        / SOFT_DIVIDE;
                    s_forward_data[idx] =
                        cyclecal_avg_array(
                            sample_raw,
                            SAMPLE_PER_STEP,
                            MICROSTEPLAP);
                    sample_cnt = 0;
                    pos_set += FINE_SPEED;
                }
            }
            else
            {
                pos_set += FINE_SPEED;
            }
            ela_tb67h450_set_foc_current(pos_set, 2000);
            if (pos_set >= (2 * MICROSTEPLAP))
            {
                pos_set = MICROSTEPLAP;
                cali_state = 3;
            }
            break;

        case 3:
            pos_set += FINE_SPEED;
            ela_tb67h450_set_foc_current(pos_set, 2000);
            if (pos_set == (2 * MICROSTEPLAP
                            + SOFT_DIVIDE * 20))
            {
                cali_state = 4;
            }
            break;

        case 4:
            pos_set -= FINE_SPEED;
            ela_tb67h450_set_foc_current(pos_set, 2000);
            if (pos_set == (2 * MICROSTEPLAP))
            {
                cali_state = 5;
            }
            break;

        case 5:
            if (0 == (pos_set % SOFT_DIVIDE))
            {
                sample_raw[sample_cnt++] =
                    g_mt6816_st.raw_data;
                if (SAMPLE_PER_STEP == sample_cnt)
                {
                    unsigned int idx =
                        (pos_set - MICROSTEPLAP)
                        / SOFT_DIVIDE;
                    s_reverse_data[idx] =
                        cyclecal_avg_array(
                            sample_raw,
                            SAMPLE_PER_STEP,
                            MICROSTEPLAP);
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
                pos_set = MICROSTEPLAP;
                cali_state = 6;
            }
            break;

        case 6:
            ela_tb67h450_set_foc_current(0, 0);
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
        int cyc_diff = cyclecal_diff(data[i], data[i - 1],
                                     ENC_RESOLUTION);
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
 * @ 输入: data: 数据数组; len: 数据长度; direction: 方向
 *         (1: 正向, -1: 反向)
 * @ 输出: true 表示找到跳跃点
 * @ 说明: 查找编码器旋转过程中 16383→0 的跳跃点位置
 ********/
static bool calibration_find_jump_point(
    const unsigned short *data, unsigned int len,
    int direction)
{
    for (unsigned int i = 0; i < len - 1; i++)
    {
        unsigned int curr = data[i];
        unsigned int next = data[i + 1];

        if (direction > 0)
        {
            if (curr > (ENC_RESOLUTION * 3 / 4)
                && next < (ENC_RESOLUTION / 4))
            {
                g_calibra_st.jump_pot = (unsigned char)i;
                g_calibra_st.jump_pot_data =
                    (ENC_RESOLUTION - 1) - curr;
                return true;
            }
        }
        else
        {
            if (curr < (ENC_RESOLUTION / 4)
                && next > (ENC_RESOLUTION * 3 / 4))
            {
                g_calibra_st.jump_pot = (unsigned char)i;
                g_calibra_st.jump_pot_data = curr;
                return true;
            }
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

    int cyc_diff = cyclecal_diff(
        g_calibra_st.avg_fr_data[0],
        g_calibra_st.avg_fr_data[WHOLESTEPLAP],
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
            g_calibra_st.avg_fr_data, point_count,
            direction))
    {
        return;
    }
}

/********
 * @ 说明: 根据校准数据生成校准表，线性插值后写入 Flash。
 *         校准表以编码器值为索引，微步值为内容，共 16384 个
 *         halfword，写入 STOCKFILE_CALI_ADDR 分区
 ********/
static void calibration_generate_table(void)
{
    int data;
    unsigned short val;
    unsigned int result_num = 0;
    int direction = 1;

    unsigned int first = g_calibra_st.avg_fr_data[0];
    unsigned int last =
        g_calibra_st.avg_fr_data[WHOLESTEPLAP];
    int cyc_diff = cyclecal_diff(first, last,
                                 ENC_RESOLUTION);
    if (cyc_diff > 0)
        direction = 1;
    else
        direction = -1;

    ela_stockfile_usr_erase(&g_stockfile_cali_st);
    ela_stockfile_usr_seq_write_begin(&g_stockfile_cali_st);

    if (direction > 0)
    {
        for (int x = g_calibra_st.jump_pot;
             x < g_calibra_st.jump_pot + WHOLESTEPLAP + 1;
             x++)
        {
            data = cyclecal_diff(
                g_calibra_st.avg_fr_data[
                    cyclecal_mod(x + 1, WHOLESTEPLAP)],
                g_calibra_st.avg_fr_data[
                    cyclecal_mod(x, WHOLESTEPLAP)],
                ENC_RESOLUTION);

            int wholestep_start =
                (x == g_calibra_st.jump_pot)
                ? g_calibra_st.jump_pot_data + 1 : 0;
            int wholestep_end =
                (x == g_calibra_st.jump_pot + WHOLESTEPLAP)
                ? g_calibra_st.jump_pot_data + 1 : data;

            for (int y = wholestep_start;
                 y < wholestep_end; y++)
            {
                val = cyclecal_mod(
                    SOFT_DIVIDE * x
                    + SOFT_DIVIDE * y / data,
                    MICROSTEPLAP);
                ela_stockfile_usr_seq_write_next(
                    &g_stockfile_cali_st, val);
                result_num++;
            }
        }
    }
    else
    {
        for (int x = g_calibra_st.jump_pot + WHOLESTEPLAP;
             x > g_calibra_st.jump_pot - 1; x--)
        {
            data = cyclecal_diff(
                g_calibra_st.avg_fr_data[
                    cyclecal_mod(x, WHOLESTEPLAP)],
                g_calibra_st.avg_fr_data[
                    cyclecal_mod(x + 1, WHOLESTEPLAP)],
                ENC_RESOLUTION);

            int wholestep_start =
                (x == g_calibra_st.jump_pot + WHOLESTEPLAP)
                ? g_calibra_st.jump_pot_data + 1 : 0;
            int wholestep_end =
                (x == g_calibra_st.jump_pot)
                ? g_calibra_st.jump_pot_data + 1 : data;

            for (int y = wholestep_start;
                 y < wholestep_end; y++)
            {
                val = cyclecal_mod(
                    SOFT_DIVIDE * (x + 1)
                    - SOFT_DIVIDE * y / data,
                    MICROSTEPLAP);
                ela_stockfile_usr_seq_write_next(
                    &g_stockfile_cali_st, val);
                result_num++;
            }
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
 ********/
void elaco_calibration_table_generate_proc(void)
{
    switch (g_calibra_st.cali_step)
    {
        case CALI_STEP_IDLE:
            if (false == g_calibra_st.calitable_flag)
            {
                g_calibra_st.cali_step = CALI_STEP_COLLECT;
            }
            break;

        case CALI_STEP_CHECK:
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
        case CALI_STEP_DONE:
            break;

        default:
            g_calibra_st.cali_step = CALI_STEP_IDLE;
            break;
    }
}

/* elaco_calibration usr end */

