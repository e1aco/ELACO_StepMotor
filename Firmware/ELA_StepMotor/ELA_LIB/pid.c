#include "pid.h"
#include "delay.h"

//
// @简介：对PID控制器进行初始化
// @参数 Kp - 比例系数
// @参数 Ki - 积分系数
// @参数 Kd - 微分系数
//
void PID_Init(PID_TypeDef *PID, float Kp, float Ki, float Kd)
{
	PID->Kp = Kp;
	PID->Ki = Ki;
	PID->Kd = Kd;
	PID->SP = 0.0f;
	
	PID->t_k_1 = 0;
	PID->err_k_1 = 0.0f;
	PID->err_int_k_1 = 0.0f;
	
	PID->UpperLimit = +3.4e+38f;
	PID->LowerLimit = -3.4e+38f;
}

//
// @简介：设置PID控制器门限
// @参数 Upper - 上限值
// @参数 Lower - 下限值
//
void PID_LimitConfig(PID_TypeDef *PID, float Upper, float Lower)
{
	PID->UpperLimit = Upper;
	PID->LowerLimit = Lower;
}

//
// @简介：改变设定值SP
// @参数 SP - 新的设定值
//
void PID_ChangeSP(PID_TypeDef *PID, float SP)
{
	PID->SP = SP;
}

//
// @简介：执行一次PID运算
// @参数 FB - 反馈的值，也就是传感器采回的值
// @返回值：PID控制器计算的结果
//
float PID_Compute(PID_TypeDef *PID, float FB)
{
	float err = PID->SP - FB;
	
	uint64_t t_k = GetUs();
	
	float deltaT = (t_k - PID->t_k_1)* 1.0e-6f;
	
	float err_dev = 0.0f;
	float err_int = 0.0f;
	
	if(PID->t_k_1 != 0)
	{
		err_dev = (err - PID->err_k_1) / deltaT;
		err_int = PID->err_int_k_1 + (err + PID->err_k_1) * deltaT * 0.5f;
	}
	
	float COp = PID->Kp * err;
	float COi = PID->Ki * err_int;
	float COd = PID->Kd * err_dev;
	float CO = COp + COi + COd;
	
	// 更新
	PID->t_k_1 = t_k;
	PID->err_k_1 = err;
	PID->err_int_k_1 = err_int;
	
	// 输出限幅
	if(CO > PID->UpperLimit) CO = PID->UpperLimit;
	if(CO < PID->LowerLimit) CO = PID->LowerLimit;
	
	// 积分限幅
	if(PID->err_int_k_1 > PID->UpperLimit) PID->err_int_k_1 = PID->UpperLimit;
	if(PID->err_int_k_1 < PID->LowerLimit) PID->err_int_k_1 = PID->LowerLimit;
	
	return CO;
}

//
// @简介：对PID控制器进行复位
//
void PID_Reset(PID_TypeDef *PID)
{
	PID->t_k_1 = 0;
	PID->err_k_1 = 0.0f;
	PID->err_int_k_1 = 0.0f;
}
