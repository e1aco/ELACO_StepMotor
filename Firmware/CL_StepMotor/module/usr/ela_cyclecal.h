/*****************************************************************************
 * @文件: ela_cyclecal.h
 * @作者: cl
 * @日期: 2026-08-06
 * @版本: v1.0.0
 * @说明: 循环计算函数库，提供环绕取模/差值/平均等操作
 ****************************************************************************/

#ifndef ELA_CYCLECAL_H

/* ==== 常量定义 ==== */
#define ELA_CYCLECAL_H

/* ==== 接口 ==== */


unsigned int USR_CycleCal_Mod(unsigned int a, unsigned int b);
int USR_CycleCal_Diff(int a, int b, int cyc);
int USR_CycleCal_AvgTwo(int a, int b, int cyc);
int USR_CycleCal_AvgArray(const unsigned short *data,
                       unsigned short len, int cyc);

#endif






