#include "ela_cyclecal.h"


/****
 * @ 原型: unsigned int CycleCal_Mod(unsigned int a, unsigned int b)
 * @ 输入: unsigned int a: 被取模数; unsigned int b: 模数
 * @ 输出: a 对 b 取模的结果，范围在 [0, b) 内
 * @ 说明: 循环取模，当数值超出范围时自动回绕，如数组索引超过长度时回到开头
 ********/
unsigned int CycleCal_Mod(unsigned int a, unsigned int b)
{
    return (a + b) % b;
}

/****
 * @ 原型: int CycleCal_Diff(int a, int b, int cyc)
 * @ 输入: int a: 第一个数据; int b: 第二个数据; int cyc: 环绕周期（如编码器的分辨率）
 * @ 输出: a 和 b 的最短循环差值, 范围 (-cyc/2, cyc/2]
 * @ 说明: 带环绕处理的最短差值，如编码器分辨率为360时，359→0的差值为1而非-359
 ********/
int CycleCal_Diff(int a, int b, int cyc)
{
    int sub = a - b;
    if (sub > (cyc >> 1)) sub -= cyc;      /* 超过半圈，减一圈 */
    if (sub < (-(cyc >> 1))) sub += cyc;   /* 低于负半圈，加一圈 */
    return sub;
}

/****
 * @ 原型: int CycleCal_AvgTwo(int a, int b, int cyc)
 * @ 输入: int a: 第一个数据; int b: 第二个数据; int cyc: 环绕周期（如编码器的分辨率）
 * @ 输出: a 和 b 的循环平均值，结果在 [0, cyc) 内
 * @ 说明: 两值循环平均，输入范围须在 [0, cyc) 内；半圈对称时返回两数中间值
 *         如 350° 和 10° 的平均值为 0°，而非 180°
 ********/
int CycleCal_AvgTwo(int a, int b, int cyc)
{
    int sub = a - b;            // 计算差值
    int ave = (a + b) >> 1;     // 计算平均值
    if (abs(sub) > (cyc >> 1)) {    // 如果差值超过半圈，说明跨越了0°点，需要调整平均值
        if (ave >= (cyc >> 1))      // 如果平均值在半圈以上，说明应该减去半圈；否则应该加上半圈
            ave -= (cyc >> 1);
        else
            ave += (cyc >> 1);
    }
    return ave;
}

/****
 * @ 原型: int CycleCal_AvgArray(const unsigned short *data, unsigned short len, int cyc)
 * @ 输入: const unsigned short *data: 待平均的数据数组; unsigned short len: 数组长度; int cyc: 环绕周期（如编码器的分辨率）
 * @ 输出: int: 循环平均值，已归一化到 [0, cyc) 范围
 * @ 说明: 对数组进行循环平均，以第一个数据为基准处理环绕跳变（如编码器 16383→0），结果归一化到指定周期内
 ********/
int CycleCal_AvgArray(const unsigned short *data, unsigned short len, int cyc)
{
    int sum = data[0];
    for (unsigned short i = 1; i < len; i++)
    {
        int diff = data[i];
        int sub = data[i] - data[0];
        /* 以第一个数据为基准，处理其他数据的环绕 */
        if (sub > (cyc >> 1))
            diff = data[i] - cyc;
        if (sub < (-(cyc >> 1)))
            diff = data[i] + cyc;
        sum += diff;
    }
    sum = sum / len;
    /* 将结果归一化到[0, cyc)范围 */
    if (sum < 0)
        sum += cyc;
    if (sum > cyc)
        sum -= cyc;
    return sum;
}
