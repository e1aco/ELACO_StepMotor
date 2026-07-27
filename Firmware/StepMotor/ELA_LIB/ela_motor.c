#include "ela_motor.h"

//----------------------------------------------------------
/* -1- 全局变量 */
unsigned char gov_motor_brake_flag = 0; // 1表示刹车
unsigned char gov_motor_sleep_flag = 0; // 1表示休眠
unsigned char gov_motor_mode_fsm = 0;   // 电机模式状态机
//----------------------------------------------------------
/* -2- 辅助函数 */

// 超前角获取
int advance_position(int speed)
{
    int compensate;

    if (speed < 0)
    {
        if (speed > -100000)
            compensate = 0;
        else if (speed > -1300000)
            compensate = (((speed + 100000) * 262) >> 20) - 0;
        else if (speed > -2200000)
            compensate = (((speed + 1300000) * 105) >> 20) - 300;
        else
            compensate = (((speed + 2200000) * 52) >> 20) - 390;

        if (compensate < -430)
            compensate = -430;
    }
    else // 正的速度
    {
        if (speed < 100000)
            compensate = 0;
        else if (speed < 1300000)
            compensate = (((speed - 100000) * 262) >> 20) + 0;
        else if (speed < 2200000)
            compensate = (((speed - 1300000) * 105) >> 20) + 300;
        else
            compensate = (((speed - 2200000) * 52) >> 20) + 390;

        if (compensate > 430)
            compensate = 430;
    }

    return compensate;
}

/****
 * @ 原型: int Position_PID_Control(int position,int speed)
 * @ 输入: int est_position：当前位置（估计值，为了预测控制时的位置）,int position：目标位置;int speed：目标速度
 * @ 输出: int: 控制输出值
 * @ 说明: 通过目标位置和目标速度计算需要施加给电机的电磁转矩所对应的相电流大小
 *         PI2D,本质是一个双积分结构
 ********/
int Position_PID_Control(int est_position, int est_speed, int soft_position, int soft_speed)
{
    // 计算距离和速度的误差
    position_pid_st.pErr = soft_position - est_position; // 位置误差
    position_pid_st.vErr = (soft_speed - est_speed) >> 7; // 速度误差(除以128，匹配位置误差的值)
    // 误差限幅
    if (position_pid_st.pErr > 3200)
        position_pid_st.pErr = 3200;
    if (position_pid_st.pErr < -3200)
        position_pid_st.pErr = -3200;
    if (position_pid_st.vErr > 4000)
        position_pid_st.vErr = 4000;
    if (position_pid_st.vErr < -4000)
        position_pid_st.vErr = -4000;
    // 比例运算
    position_pid_st.outputKp = position_pid_st.pErr * position_pid_st.kp; 
    // 积分运算
    position_pid_st.integralRound += (position_pid_st.ki * position_pid_st.pErr + 
                                      position_pid_st.kv * position_pid_st.vErr);// ()
    position_pid_st.integralRemainder = position_pid_st.integralRound >> 7;
    position_pid_st.integralRound -= position_pid_st.integralRemainder << 7;
    position_pid_st.outputKi += position_pid_st.integralRemainder; // 积分输出
    // 积分限幅
    

}

//----------------------------------------------------------
/* -3- 用户调用函数 */

// 放在 FREQUENCY_Motor Khz的定时器中断中调用
void ela_motor_proc(void)
{
    static unsigned int last_micro_angle = 0; // 上一次的微步角度值
    static unsigned int micro_angle = 0;      // 当前微步角度值
    static short angle_diff = 0;              // 当前微步角度值与上一次微步角度值的差值
    static int acc_position = 0;              // 多圈积累的微步值

    static int est_speed = 0;           // 估计速度值
    static int est_speed_intergral = 0; // 速度积分值，用于一阶滤波计算估计速度
    static int est_position = 0;        // 估计位置值

    static int soft_position = 0; // 位置模式目标位置
    static int soft_speed = 0;    // 速度模式目标速度

    MT6816_GetAngle(); // 获取编码器角度
    micro_angle = gov_mt6816_st.micro_angle;
    // 第一次调用，直接退出
    if (last_micro_angle == 0)
    {
        last_micro_angle = micro_angle;
        return;
    }
    // 更新电机的目标角度值
    angle_diff = CycleCal_Diff(micro_angle, last_micro_angle, 51200); // 计算当前微步角度值与上一次微步角度值的差值
    last_micro_angle = micro_angle;                                   // 更新上一次的微步角度值
    // 计算步进的总距离（多圈累计值）
    acc_position += angle_diff;
    // 计算速度然后进行一阶滤波，获取估计速度（预判速度）
    est_speed_intergral = angle_diff * FREQUENCY_Motor + (est_speed << 5) - est_speed;
    est_speed = est_speed_intergral >> 5;    // 更新估计速度值
    est_speed_intergral -= (est_speed << 5); // 更新积分值
    // 获取估计位置
    est_position += advance_position(est_speed); // 获取超前角度补偿值（本质是分段的线性处理）
    // 电机控制
    if (gov_motor_brake_flag)
    {
        // 刹车
        TB67H450_Brake();
    }
    else if (gov_motor_sleep_flag)
    {
        // 休眠
        TB67H450_Sleep();
    }
    else
    {
        // 运动模式选择
        switch (gov_motor_mode_fsm)
        {
        // 电机停止
        case MODE_STOP:
            TB67H450_Sleep();
            break;
        // 位置模式
        case MODE_COMMAND_POSITION:
        case MODE_COMMAND_TRAJECTORY:
        case MODE_PWM_POSITION:
            Position_PID_Control(est_position, est_speed, soft_position, soft_speed);
            break;
        // 速度模式
        case MODE_COMMAND_VELOCITY:
        case MODE_PWM_VELOCITY:
            // CalcPidToOutput(s_soft_velocity);
            break;
        // 电流模式
        case MODE_COMMAND_CURRENT:
        case MODE_PWM_CURRENT:
            // CalcCurrentToOutput(s_soft_current);
            break;
        default:
            break;
        }
    }
}
