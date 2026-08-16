/*****************************************************************************
 * @文件: motor_usr.h
 * @作者: cl
 * @日期: 2026-08-13
 * @版本: v1.0
 * @说明: 电机用户层（最小闭环：编码器 raw×25/8 映射 + FOC 电流输出 +
 *   位置/速度/电流命令 + IIR 速度估计 + 简单 P 环状态机）
 * @平台: STM32F103RET6
 * @依赖: mt6816_usr, tb67h450_usr, encoder_calibrator_usr
 ****************************************************************************/
#ifndef MOTOR_USR_H
#define MOTOR_USR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ==== 常量定义 ==== */
#define USR_MOTOR_HARD_STEPS      200U    /* 步进电机每圈 200 整步 */
#define USR_MOTOR_SOFT_DIVIDE     256U    /* 每整步细分 256 微步（memory） */
/* 每圈细分步 = 200 整步 × 256 微步 = 51200 */
#define USR_MOTOR_SUBDIVIDE_STEPS (USR_MOTOR_HARD_STEPS * USR_MOTOR_SOFT_DIVIDE)
#define USR_MOTOR_CONTROL_FREQ    20000U  /* 控制 tick 20kHz（memory） */
#define USR_MOTOR_CONTROL_US      50U     /* 控制周期 50µs（memory） */

/* 简单 P 环限幅（依据 .cl/memory/ motor_minloop_control: err 限 ±3200） */
#define USR_MOTOR_POS_ERR_MAX     3200U
/* vel_goal 每帧变化限幅（步/s）：位置环输出限斜率=简易轨迹整形，
   防长行程满速冲入过冲振荡（8/15 实测 330° 回 90° 过冲 1230 步 + 回摆） */
#define USR_MOTOR_VEL_GOAL_ACC    500U
/* 死区外最小推进速度（步/s）：必须 > 编码器微振假速度才推得动，否则速度环
   被假速度主导 → 死区边缘拉锯不收敛（8/15 实测 90° 处假速度 ±25000）
   30000：常规档（90°/180°/270° 实测：形变小→回弹小→收敛）
   60000 全档实测：推回形变 247 步→回弹→出界→极限环 17 行 ✗ */
#define USR_MOTOR_POS_MIN_VEL     30000U
/* 减速窗口（出界深度，细分步）：|err|≤窗口 → MIN_VEL 不钳位 → vel_goal=32×err
   线性下坡（限斜率整形）→ 低速进入死区（8192@死区边缘）→ 落点无过冲无摆动；
   |err|>窗口 → MIN_VEL 强制推进（推得动 > 假速度）。8/15 实测 30000 冲入死区
   → 过冲 ±150~250 → 弹性摆动自持（90° 24 行 / 0° 33 行，到位抖动大） */
#define USR_MOTOR_POS_MIN_VEL_DS  2048U
/* 假速度上限（步/s）：编码器磁干扰读数抖动经 IIR 后仍达 ±25000~40000（8/15
   实测）→ vel_goal 落入此区间时速度环被假速度主导（err=vel_goal-假速度 →
   ±2A 猛摆极限环，8/16 实测）→ 该区间改位置环误差直驱电流（不经速度环） */
#define USR_MOTOR_FAKE_VEL_MAX    25000U
/* 死区制动时长（ms）：进入死区后先速度环刹停残余速度（过冲小）再 0 电流。
   一次性制动（有限时长）→ 无持续速度环 → 无假速度自激（8/15 实测持续
   死区速度环 → 自激 30 行） */
#define USR_MOTOR_POS_DB_BRAKE_MS 10U
/* 绕回点档（目标≈编码器 0 点 51200±1024）：MT6816 读数在 0 附近跳变毛刺
   （raw 0~243 摆动）→ 假速度 ±40000 > 30000 推不回卡死（实测 3s）
   → 该目标区间用 60000 猛推冲破（仅在绕回窗口生效，不影响常规收敛） */
#define USR_MOTOR_POS_MIN_VEL_WRAP 60000U
#define USR_MOTOR_POS_WRAP_WIN    1024U
/* 死区入界滞回（步）：出界（|err|>DEADBAND）驱动推回至 |err|<HYST 才归零。
   8/15 实测过冲落点 err 偏负 ~-125±12 → 一半概率落死区外几步 → MIN_VEL 推回量与
   微振漂移量（±38 步/100ms）平衡 → 归零-出界极限环 4s；HYST=16 → 推回 105 步
   > 漂移周期 76 步 → 破平衡收敛 */
#define USR_MOTOR_POS_DB_HYST     16U
/* 绕回窗口内死区（细分步）：0° 处弹性/毛刺摆动 ±250 > 常规死区 128 → 频繁
   出界 → MIN_VEL 驱动 → 摆动自激（8/15 MODE_POSITION 实测 33 行不收敛）；
   256 吞掉摆动 → 不出界不驱动 → 自由衰减静止（静止位置=摆动中心 err≤128，
   精度仍 ≤0.9° 需求）。仅绕回窗口（51200±1024）内生效 */
#define USR_MOTOR_POS_DEADBAND_WRAP 256U
/* 死区保持力系数（mA/步×1024）：到位死区内输出静态保持电流把位置缓慢推回
   死区中心 → 抵抗齿隙弹性漂移（8/15 实测 0 电流归零 → 弹性把位置弹出死区
   ±50 步 → 出界→推回→归零→再弹 26 行极限环）。
   1024 = 1mA/步 → err=128 → 112mA（≈3.4% 额定），不经速度环 → 无微振自激 */
#define USR_MOTOR_POS_HOLD_MA_PER_STEP 1024U
/* 速度 P 环误差限幅（复刻参考 motor.c CalcPidToOutput pid.vError 限 ±1024×1024） */
#define USR_MOTOR_VEL_ERR_MAX     1048576U
/* 到位死区（最小闭环无 planner，软目标=目标，需自判到位；待实测确认整定）
   8/15 实测：到位后齿隙弹性/残余摆动 ±150~250 步 > 128 → 频繁出界 → MIN_VEL
   驱动 → 摆动自持（90° 24 行 / 0° 33 行）；256 吞掉摆动 → 不出界不驱动 →
   摩擦衰减静止，静止位置=摆动中心（err≤50，精度 0.35° < 0.9° 需求） */
#define USR_MOTOR_POS_DEADBAND    256U   /* 位置到位死区（细分步，≈1.8° 判定/0.35° 静止） */
#define USR_MOTOR_VEL_DEADBAND    512U   /* 速度到位死区（细分步/s，≈0.01 圈/s；待实测确认） */
#define USR_MOTOR_CUR_DEADBAND    10U    /* 电流到位死区（mA；待实测确认） */

/* 超前角补偿分段（依据 .cl/memory/ motor_compensate_angle=分段|±430| 步，
   vel 阈值 100k/1.3M/2.2M，斜率 262/105/52>>20，复刻参考 motor.c） */
#define USR_MOTOR_LEAD_VEL1       100000   /* 低于此速无补偿（细分步/s） */
#define USR_MOTOR_LEAD_VEL2       1300000  /* 第一段上界 */
#define USR_MOTOR_LEAD_VEL3       2200000  /* 第二段上界 */
#define USR_MOTOR_LEAD_MAX        430      /* 补偿封顶（细分步，≈3°） */
#define USR_MOTOR_LEAD_SLOPE1     262      /* 段 1 斜率（>>20） */
#define USR_MOTOR_LEAD_SLOPE2     105      /* 段 2 斜率（>>20） */
#define USR_MOTOR_LEAD_SLOPE3     52       /* 段 3 斜率（>>20） */

/* 堵转/过载检测（复刻参考 motor.c）：电流顶格(或电流模式非零)且 |速度|<1/5 圈/s
   持续 1s → 堵转；非电流模式电流顶格持续 1s → 过载 */
#define USR_MOTOR_STALL_TIME_US   1000000U /* 堵转判定时长（µs） */
#define USR_MOTOR_STALL_VEL_MAX   (USR_MOTOR_SUBDIVIDE_STEPS / 5)  /* 堵转速度上限 */

/* ==== 类型定义 ==== */
/* 电机模式（对齐参考 motor.h，最小闭环仅实现 STOP/POSITION/VELOCITY/CURRENT，其余回 STOP） */
typedef enum {
    MODE_STOP = 0,
    MODE_COMMAND_POSITION,
    MODE_COMMAND_VELOCITY,
    MODE_COMMAND_CURRENT,
    MODE_COMMAND_TRAJECTORY,
    MODE_PWM_POSITION,
    MODE_PWM_VELOCITY,
    MODE_PWM_CURRENT,
    MODE_STEP_DIR
} Motor_Mode_T;

/* 电机状态（对齐参考 motor.h；最小闭环仅用 STOP/FINISH/RUNNING，任务6后补完整） */
typedef enum {
    STATE_STOP = 0,
    STATE_FINISH,
    STATE_RUNNING,
    STATE_OVERLOAD,
    STATE_STALL,
    STATE_NO_CALIB
} Motor_State_T;

/* 电机配置（本任务扩展：planner 参数 + 保护开关，config_usr 任务再统一 BoardConfig_t） */
typedef struct {
    int32_t encoderHomeOffset;  /* 编码器零位偏移（细分步 0~51199） */
    int32_t ratedCurrent;       /* 额定电流限幅（mA） */
    int32_t ratedVelocity;      /* 额定速度限幅（细分步/s） */
    int32_t ratedVelocityAcc;   /* 速度加速度限幅（细分步/s²，planner 用） */
    int32_t ratedCurrentAcc;    /* 电流加速度限幅（mA/s，planner 用） */
    int32_t posKp;              /* 位置环增益（误差→速度目标，依据 .cl/memory/ motor_cascade_poskp=32768） */
    int32_t pidKp;              /* 速度环增益（速度误差→电流，依据 .cl/memory/ pid_kp=5 待实测整定） */
    int32_t pidKd;              /* 速度环阻尼（速度变化→反向电流，8/13 实测 Kd=400 压微振极限环） */
    bool    stallProtectSwitch; /* 堵转保护开关（8/15 后确认默认开） */
} Motor_Config_T;

/* ==== 全局实例 ==== */
/* ==== 接口 ==== */
void    USR_Motor_Init(void);
void    USR_Motor_SetConfig(Motor_Config_T *config);
void    USR_Motor_Tick20kHz(void);
void    USR_Motor_SetMode(Motor_Mode_T mode);
void    USR_Motor_SetPosition(int32_t pos);
void    USR_Motor_SetVelocity(int32_t vel);
void    USR_Motor_SetCurrent(int32_t cur);
void    USR_Motor_SetDisable(bool disable);
void    USR_Motor_SetBrake(bool brake);
void    USR_Motor_ClearStallFlag(void);
Motor_State_T USR_Motor_GetState(void);
float   USR_Motor_GetPosition(bool is_lap);
float   USR_Motor_GetVelocity(void);
float   USR_Motor_GetCurrent(void);
uint8_t USR_Motor_GetMode(void);
void    USR_Motor_GetTelemetry(float *pos, float *vel, float *cur,
                               uint8_t *mode, uint8_t *state);
int32_t USR_Motor_GetRawPosition(void);
int32_t USR_Motor_GetRawVelocity(void);
int32_t USR_Motor_GetRawDelta(void);
bool    USR_Motor_IsCalibrated(void);
void    USR_Motor_TriggerCalibration(void);
void    USR_Motor_ZeroPosition(void);

#ifdef __cplusplus
}
#endif

#endif /* MOTOR_USR_H */
