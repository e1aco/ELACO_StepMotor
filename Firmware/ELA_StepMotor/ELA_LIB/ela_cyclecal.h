/********
 * @ 文件: ela_cyclecal.h
 * @ 作者: ELACO
 * @ 日期: 2026-07-27
 * @ 版本: 1.0.0
 * @ 说明: 循环计算函数库，提供环绕取模/差值/平均等操作
 ********/

#ifndef ELA_CYCLECAL_H
#define ELA_CYCLECAL_H

unsigned int cyclecal_mod(unsigned int a, unsigned int b);
int cyclecal_diff(int a, int b, int cyc);
int cyclecal_avg_two(int a, int b, int cyc);
int cyclecal_avg_array(const unsigned short *data,
                       unsigned short len, int cyc);

#endif

