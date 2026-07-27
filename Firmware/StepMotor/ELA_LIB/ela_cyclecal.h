#ifndef _ELA_CYCLECAL_H_
#define _ELA_CYCLECAL_H_

#include "elaco_main.h"


unsigned int CycleCal_Mod(unsigned int a, unsigned int b);
int CycleCal_Diff(int a, int b, int cyc);
int CycleCal_AvgTwo(int a, int b, int cyc);
int CycleCal_AvgArray(const unsigned short *data, unsigned short len, int cyc);


#endif
/* @ elaco: 用于存储循环计算相关的函数 */
