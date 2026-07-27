#include "elaco_calibration.h"

unsigned short forward_data[WHOLESTEPLAP+1]; // 存储正向测量数据
unsigned short reverse_data[WHOLESTEPLAP+1]; // 存储反向测量数据

CALIBRATION_DATA_T gov_calibra_st={CALI_STEP_IDLE, FALSE, 0}; // 校准数据
unsigned short* gov_cali_table = (unsigned short*)STOCKFILE_CALI_ADDR; // 校准表地址


/****
 * @ 原型: void Calibration_Proc(void)
 * @ 输入: void
 * @ 输出: void
 * @ 说明: 对电机进行转动并读取编码器值以获得正反转的编码器值
 ********/
void Calibration_Proc(void)
{
    static unsigned char CALI_STATE = 0;
    static unsigned int pos_set = 0; // 电角度位置设置
    static unsigned short sample_raw[SAMPLE_PER_STEP]; // 16次取平均
    static unsigned char sample_cnt = 0; // 采样计数器
    MT6816_GetAngle();
    switch (CALI_STATE)
    {
        case 0: // 空闲状态
            if(gov_calibra_st.cali_step == CALI_STEP_COLLECT) // 如果开始校准
            {
                TB67H450_SetFocCurrentVector(pos_set, 2000); // 将电机标定到电角度为0的位置，范围0-51200是一圈，对应0-360度，电角度=机械角度*1024/360
                pos_set = MICROSTEPLAP; // 设置电角度位置为一圈的位置
                CALI_STATE = 1; // 进入下一状态
            }
            break;
        case 1: // 正向准备：移动到起始位置
            pos_set += AUTO_SPEED; // 每次增加2个电角度单位，慢速移动
            TB67H450_SetFocCurrentVector(pos_set, 2000); 
            if (pos_set >= 2*MICROSTEPLAP) // 如果达到一圈的位置
            {
                pos_set = MICROSTEPLAP; // 重置位置设置
                CALI_STATE = 2; // 进入下一状态
            }
            break;
        case 2: // 正向测量：正转一圈，采集数据
            if(pos_set % SOFT_DIVIDE == 0) // 每256个电角度单位（对应90度）采集一次数据，采集一圈也就是200个数
            {
                sample_raw[sample_cnt++] = gov_mt6816_st.raw_data;
                if (sample_cnt == SAMPLE_PER_STEP)
                {
                    /* 16次采样取平均，存入正向数据数组 */
                    int idx = (pos_set - MICROSTEPLAP) / SOFT_DIVIDE;
                    forward_data[idx] = CycleCal_AvgArray(sample_raw, SAMPLE_PER_STEP, MICROSTEPLAP);
                    sample_cnt = 0;
                    pos_set += FINE_SPEED;
                }
            }
            else
            {
                pos_set += FINE_SPEED; // 每次增加2个电角度单位，慢速移动
            }
            TB67H450_SetFocCurrentVector(pos_set, 2000);
            if(pos_set >= 2*MICROSTEPLAP) // 如果达到一圈的位置
            {
                pos_set = MICROSTEPLAP; // 重置位置设置
                CALI_STATE = 3; // 进入下一状态
            }
            break;
        case 3:
            pos_set += FINE_SPEED;
            TB67H450_SetFocCurrentVector(pos_set, 2000);
            if (pos_set == 2 * MICROSTEPLAP + SOFT_DIVIDE * 20) // 正向走20步
            {
                CALI_STATE = 4;
            }
            break;
        case 4: /* 消除间隙：反向移动消除齿轮间隙 */
            pos_set -= FINE_SPEED;
            TB67H450_SetFocCurrentVector(pos_set, 2000);
            if (pos_set == 2 * MICROSTEPLAP)
            {
                CALI_STATE = 5;
            }
            break;
        case 5: /* 反向测量：反转一圈，采集数据 */
            if(pos_set % SOFT_DIVIDE == 0) // 每256个电角度单位（对应90度）采集一次数据，采集一圈也就是200个数
            {
                sample_raw[sample_cnt++] = gov_mt6816_st.raw_data;
                if (sample_cnt == SAMPLE_PER_STEP)
                {
                    /* 16次采样取平均，存入反向数据数组 */
                    int idx = (pos_set - MICROSTEPLAP) / SOFT_DIVIDE;
                    reverse_data[idx] = CycleCal_AvgArray(sample_raw, SAMPLE_PER_STEP, MICROSTEPLAP);
                    sample_cnt = 0;
                    pos_set -= FINE_SPEED;
                }
            }
            else
            {
                pos_set -= FINE_SPEED; // 每次增加2个电角度单位，慢速移动
            }
            TB67H450_SetFocCurrentVector(pos_set, 2000);
            if(pos_set < MICROSTEPLAP) // 如果达到一圈的位置
            {
                pos_set = MICROSTEPLAP; // 重置位置设置
                CALI_STATE = 6; // 进入下一状态
            }
            break;
        case 6: // 计算状态：停止电机输出
            TB67H450_SetFocCurrentVector(0, 0);
            gov_calibra_st.cali_step = CALI_STEP_CHECK; // 数据采集完成，进入检查阶段
            break;
    }
}

/****
 * @ 原型: static bool Calib_CheckContinuity(const unsigned short *data, unsigned int len, int direction)
 * @ 输入: data: 数据数组, len: 数据长度, direction: 方向(1:正向, -1:反向)
 * @ 输出: static bool
 * @ 说明: 检查数据连续性
 ********/
static bool Calib_CheckContinuity(const unsigned short *data, unsigned int len, int direction)
{
    for (unsigned int i = 1; i < len; i++)
    {
        int cyc_diff = CycleCal_Diff(data[i], data[i - 1], ENC_RESOLUTION);
        int abs_diff = abs(cyc_diff);

        if (abs_diff > (ENC_WHOLESTEP * 3 / 2))
        {
            gov_calibra_st.data_err = 2; // 连续性差值过大
            return false;
        }
        if (abs_diff < (ENC_WHOLESTEP * 1 / 2))
        {
            gov_calibra_st.data_err = 2; // 连续性差值过小
            return false;
        }
        if (cyc_diff == 0)
        {
            gov_calibra_st.data_err = 2; // 数据不连续
            return false;
        }
        if ((cyc_diff > 0) != direction)
        {
            gov_calibra_st.data_err = 3; // 方向不一致
            return false;
        }
    }

    return true;
}

/****
 * @ 原型: static bool Calib_FindJumpPoint(const unsigned short *data, unsigned int len, int direction)
 * @ 输入: data: 数据数组, len: 数据长度, direction: 方向(1:正向, -1:反向)
 * @ 输出: static bool
 * @ 说明: 查找旋转跳跃点
 ********/
static bool Calib_FindJumpPoint(const unsigned short *data, unsigned int len, int direction)
{
    for (unsigned int i = 0; i < len - 1; i++)
    {
        int curr = data[i];
        int next = data[i + 1];

        if (direction == 1)
        {
            if (curr > (ENC_RESOLUTION * 3 / 4) && next < (ENC_RESOLUTION / 4))
            {
                gov_calibra_st.jump_pot = i;
                gov_calibra_st.jump_pot_data = (ENC_RESOLUTION - 1) - curr;
                return true;
            }
        }
        else
        {
            if (curr < (ENC_RESOLUTION / 4) && next > (ENC_RESOLUTION * 3 / 4))
            {
                gov_calibra_st.jump_pot = i;
                gov_calibra_st.jump_pot_data = curr; // 获取旋转跳跃点到结束时的数据值
                return true;
            }
        }
    }

    gov_calibra_st.data_err = 3; // 未找到旋转跳跃点
    return false;
}

/****
 * @ 原型: void Check_CaliData(void)
 * @ 输入: void
 * @ 输出: void
 * @ 说明: 校准数据检查
 ********/
void Check_CaliData(void)
{
    const unsigned int point_count = WHOLESTEPLAP + 1;
    gov_calibra_st.data_err = 0;

    /* 1. 计算正向和反向数据的平均值 */
    for (unsigned int i = 0; i < point_count; i++)
    {
        gov_calibra_st.avg_fr_data[i] = CycleCal_AvgTwo(forward_data[i], reverse_data[i], ENC_RESOLUTION); // 值是11对应的
    }

    /* 2. 确定磁编码器的旋转方向 */
    int cyc_diff = CycleCal_Diff(gov_calibra_st.avg_fr_data[0], gov_calibra_st.avg_fr_data[WHOLESTEPLAP], ENC_RESOLUTION);
    if (cyc_diff == 0)
    {
        gov_calibra_st.data_err = 3; // 数据方向无法判断
        return;
    }
    const int direction = (cyc_diff > 0) ? 1 : -1; //从大到小，表示反向，从小到大，表示正向。
    gov_mt6816_st.direciton = direction; // 1表示正向，-1表示反向

    /* 3. 检查相邻点连续性 */
    if (!Calib_CheckContinuity(gov_calibra_st.avg_fr_data, point_count, direction))
    {
        return;
    }

    /* 4. 查找跳跃点 */
    if (!Calib_FindJumpPoint(gov_calibra_st.avg_fr_data, point_count, direction))
    {
        return;
    }
}

/****
 * @ 原型: void Generate_CaliTable(void)
 * @ 输入: void
 * @ 输出: void
 * @ 说明: 用于根据校准数据生成校准表,以编码器值为索引，以微步值为内容设立一个表
 ********/
void Generate_CaliTable(void)
{
    int data;
    unsigned short val;
    
    unsigned char result_num = 0;
    
    /* 擦除并开始写入Flash */
    Stockfile_FlashData_Empty(&gov_stockfile_cail_st); // 擦除flash
    Stockfile_FlashData_Begin(&gov_stockfile_cail_st); // 开始写入flash
    
    if (gov_mt6816_st.direciton) { // 相对于编码器来说是正转方向的
        /* 正转方向：线性插值生成校准表 */
        for (int x = gov_calibra_st.jump_pot ; x < gov_calibra_st.jump_pot + WHOLESTEPLAP + 1; x++) {
            /* 计算相邻两个采样点之间的角度差 */
            data = CycleCal_Diff(gov_calibra_st.avg_fr_data[CycleCal_Mod(x+1, WHOLESTEPLAP)], 
                            gov_calibra_st.avg_fr_data[CycleCal_Mod(x, WHOLESTEPLAP)], ENC_RESOLUTION); // 编码器的差值
            
            /* 确定插值范围 */
            int wholestep_start = (x == gov_calibra_st.jump_pot) ? gov_calibra_st.jump_pot_data + 1 : 0; // 处理开始值（+1使Flash[0]对应编码器0）
            int wholestep_end = (x == gov_calibra_st.jump_pot + WHOLESTEPLAP) ? gov_calibra_st.jump_pot_data + 1 : data; // 处理结束值（+1将编码器16383放到表尾）
            
            /* 在两点之间线性插值 */
            for (int y = wholestep_start; y < wholestep_end; y++) {
                val = CycleCal_Mod(SOFT_DIVIDE * x + SOFT_DIVIDE * y / data, MICROSTEPLAP);
                Stockfile_FlashData_Write_Data16(&gov_stockfile_cail_st, &val, 1);
                result_num++;
            }
        }
    } else {
        /* 反转方向：线性插值生成校准表 */
        for (int x = gov_calibra_st.jump_pot + WHOLESTEPLAP; x > gov_calibra_st.jump_pot - 1; x--) {
            data = CycleCal_Diff(gov_calibra_st.avg_fr_data[CycleCal_Mod(x, WHOLESTEPLAP)], 
                            gov_calibra_st.avg_fr_data[CycleCal_Mod(x+1, WHOLESTEPLAP)], ENC_RESOLUTION);
            
            int wholestep_start = (x == gov_calibra_st.jump_pot + WHOLESTEPLAP) ? gov_calibra_st.jump_pot_data + 1 : 0;
            int wholestep_end = (x == gov_calibra_st.jump_pot) ? gov_calibra_st.jump_pot_data + 1 : data;
            
            for (int y = wholestep_start; y < wholestep_end; y++) {
                val = CycleCal_Mod(SOFT_DIVIDE * (x+1) - SOFT_DIVIDE * y / data, MICROSTEPLAP);
                Stockfile_FlashData_Write_Data16(&gov_stockfile_cail_st, &val, 1);
                result_num++;
            }
        }
    }
    
    Stockfile_FlashData_End(&gov_stockfile_cail_st);
	//printf("GenerateTable done, result_num=%lu, cali_table[0]=%u\r\n", result_num, cali_table[0]);
    
    /* 检查生成的点数是否正确 */
    if (result_num == ENC_RESOLUTION) gov_calibra_st.calitable_flag = true;
    else gov_calibra_st.data_err = 4;    
}

/****
 * @ 原型: void Cali_Table_DataValid(void)
 * @ 输入: void
 * @ 输出: void
 * @ 说明: 用于上电检测表内数据是否有效
 ********/
void Cali_Table_DataValid(void)
{
    /* 检查Flash中是否有有效校准数据（第一个字不是0xFFFF也不是0）*/
    uint16_t first = gov_cali_table[0];
    if (first != 0xFFFF && first != 0) {
        gov_calibra_st.calitable_flag = true;
    }
}

/****
 * @ 原型: void Cali_Table_Generate_Proc(void)
 * @ 输入: void
 * @ 输出: void
 * @ 说明: 校准表生成进程函数，在主循环中调用。
 *        通过 cali_step 统一管理校准全流程状态机：
 *        IDLE → COLLECT(中断处理) → CHECK → GENERATE → DONE
 ********/
void Cali_Table_Generate_Proc(void)
{
    switch (gov_calibra_st.cali_step)
    {
        case CALI_STEP_IDLE: /* 空闲：校准表无效则启动采集 */
            if (gov_calibra_st.calitable_flag == false)
            {
                gov_calibra_st.cali_step = CALI_STEP_COLLECT;
            }
            break;

        case CALI_STEP_CHECK: /* 数据采集完成，检查连续性/方向/跳跃点 */
            Check_CaliData();
            if (gov_calibra_st.data_err == 0)
            {
                gov_calibra_st.cali_step = CALI_STEP_GENERATE;
            }
            else
            {
                gov_calibra_st.cali_step = CALI_STEP_DONE;
            }
            break;

        case CALI_STEP_GENERATE: /* 生成校准表并写入Flash */
            Generate_CaliTable();
            if (gov_calibra_st.calitable_flag == true)
            {
                gov_calibra_st.cali_step = CALI_STEP_DONE;
            }
            else
            {
                gov_calibra_st.cali_step = CALI_STEP_DONE;
            }
            break;

        case CALI_STEP_COLLECT: /* 由 Calibration_Proc 在20kHz中断中处理 */
        case CALI_STEP_DONE:    /* 校准完成 */
            break;

        default: /* 异常复位 */
            gov_calibra_st.cali_step = CALI_STEP_IDLE;
            break;
    }
}
