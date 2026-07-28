/********
 * @ 文件: test_mt6816.c
 * @ 作者: ELACO
 * @ 日期: 2026-07-17
 * @ 版本: 1.0.0
 * @ 说明: MT6816 编码器测试，循环读取角度并打印
 ********/

#include "test_mt6816.h"
#include "ela_mt6816_usr.h"
#include "ela_mt6816_drv.h"
#include "ela_uart_usr.h"

/********
 * @ 说明: MT6816 编码器测试函数，初始化后循环读取
 *         并打印角度
 ********/
void test_mt6816(void)
{
    ela_mt6816_usr_init();
    while (1)
    {
        ela_mt6816_usr_read_angle();
        HAL_Delay(200);
        printf("raw:0x%04X ang:%d v:%d m:%d\r\n",
               g_mt6816_st.raw_data,
               g_mt6816_st.raw_angle,
               g_mt6816_st.data_valid,
               g_mt6816_st.magnet_valid);
    }
}
