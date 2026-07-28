/********
 * @ 文件: ela_cyclecal.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-27
 * @ 版本: 1.0.0
 * @ 说明: 循环计算工具，环绕差值/平均值 (hlp 层)
 ********/

#include "ela_cyclecal.h"

/* cyclecal hlp start */

/********
 * @ 输入: a: 输入值; b: 输入值; cyc: 循环周期
 * @ 输出: a 和 b 的最短循环差值，范围 (-cyc/2, cyc/2]
 * @ 说明: 带环绕处理的最短差值，如编码器分辨率为 16384 时
 *        16383→0 的差值为 1 而非 -16383
 ********/
int cyclecal_diff(int a, int b, int cyc)
{
    int d = b - a;

    if (d > cyc / 2)
    {
        d -= cyc;
    }
    else if (d <= -cyc / 2)
    {
        d += cyc;
    }

    return d;
}

/********
 * @ 输入: a: 输入值; b: 输入值; cyc: 循环周期
 * @ 输出: a 和 b 的循环平均值，结果在 [0, cyc) 内
 * @ 说明: 两值循环平均，输入范围须在 [0, cyc) 内；
 *        结果范围也在 [0, cyc) 内
 ********/
int cyclecal_avg_two(int a, int b, int cyc)
{
    int d = cyclecal_diff(a, b, cyc);
    int avg = (a + (d / 2)) % cyc;

    if (avg < 0)
    {
        avg += cyc;
    }

    return avg;
}

/********
 * @ 输入: a: 被取模数; b: 模数
 * @ 输出: a % b 循环取模结果
 * @ 说明: 无符号循环取模，结果在 [0, b) 内
 ********/
unsigned int cyclecal_mod(unsigned int a, unsigned int b)
{
    return a % b;
}

/********
 * @ 输入: data: 数据数组; len: 数组长度; cyc: 循环周期
 * @ 输出: 循环平均值
 * @ 说明: 基于循环差值的多点环绕平均，以首个元素为参考
 ********/
int cyclecal_avg_array(const unsigned short *data,
                       unsigned short len, int cyc)
{
    unsigned short i;
    long sum_diff = 0;
    int ref = data[0];

    for (i = 0; i < len; i++)
    {
        sum_diff += cyclecal_diff(ref, data[i], cyc);
    }

    return cyclecal_avg_two(ref, ref + (int)(sum_diff / len),
                            cyc);
}

/* cyclecal hlp end */

