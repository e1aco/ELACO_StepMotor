/*****************************************************************************
 * @文件: motion_planner_usr.h
 * @作者: cl
 * @日期: 2026-08-16
 * @版本: v1.0
 * @说明: 运动规划用户层（4 tracker 20kHz 软目标生成：电流/速度梯形平滑、
 *   位置 S 曲线、轨迹动态加速度；PositionInterpolator 不移植）
 * @平台: STM32F103RET6
 * @依赖: stdint.h
 ****************************************************************************/
#ifndef MOTION_PLANNER_USR_H
#define MOTION_PLANNER_USR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ==== 常量定义 ==== */
/* 控制 tick 频率/周期（积分器分母）
 * 依据 .cl/memory/ control_frequency = 20000: TIM4 PSC=71/Period=49
 *   → 72MHz/(72×50) = 20kHz */
#define USR_MOTION_PLANNER_CTRL_FREQ 20000
#define USR_MOTION_PLANNER_CTRL_US   50

/* ==== 类型定义 ==== */
/* 规划配置（精简自参考 MotionPlanner_Config_t：encoderHomeOffset/caliCurrent
 *   属电机配置不属 planner，留 motor 侧。变更点 2026-08-16） */
typedef struct {
    int32_t ratedCurrent;      /* 额定电流限幅（mA） */
    int32_t ratedVelocity;     /* 额定速度限幅（细分步/s） */
    int32_t ratedVelocityAcc;  /* 速度加速度限幅（细分步/s²） */
    int32_t ratedCurrentAcc;   /* 电流加速度限幅（mA/s） */
} MotionPlanner_Config_T;

/* ==== 全局实例 ==== */
/* ==== 接口 ==== */
/**
 * @ 输入 config: 规划配置指针（仅存指针，Init 前必须调用）
 * @ 输出 无
 * @ 说明 注入规划配置（ratedVelocity/ratedVelocityAcc/ratedCurrentAcc）
 */
void    USR_MotionPlanner_SetConfig(MotionPlanner_Config_T *config);

/* ---- CurrentTracker：电流梯形平滑规划 ---- */
/**
 * @ 输入 无
 * @ 输出 无
 * @ 说明 初始化电流规划器（加速度取配置 ratedCurrentAcc）
 */
void    USR_MotionPlanner_CurrentTracker_Init(void);
void    USR_MotionPlanner_CurrentTracker_SetCurrentAcc(int32_t current_acc);
void    USR_MotionPlanner_CurrentTracker_NewTask(int32_t real_current);
void    USR_MotionPlanner_CurrentTracker_CalcSoftGoal(int32_t goal_current);
int32_t USR_MotionPlanner_CurrentTracker_GetGoCurrent(void);

/* ---- VelocityTracker：速度梯形平滑规划 ---- */
void    USR_MotionPlanner_VelocityTracker_Init(void);
void    USR_MotionPlanner_VelocityTracker_SetVelocityAcc(int32_t velocity_acc);
void    USR_MotionPlanner_VelocityTracker_NewTask(int32_t real_velocity);
void    USR_MotionPlanner_VelocityTracker_CalcSoftGoal(int32_t goal_velocity);
int32_t USR_MotionPlanner_VelocityTracker_GetGoVelocity(void);

/* ---- PositionTracker：位置 S 曲线规划（梯形 + v²/2a 提前减速） ---- */
void    USR_MotionPlanner_PositionTracker_Init(void);
void    USR_MotionPlanner_PositionTracker_SetVelocityAcc(int32_t value);
void    USR_MotionPlanner_PositionTracker_NewTask(int32_t real_location,
                                                  int32_t real_speed);
void    USR_MotionPlanner_PositionTracker_CalcSoftGoal(int32_t goal_position);
int32_t USR_MotionPlanner_PositionTracker_GetGoLocation(void);
int32_t USR_MotionPlanner_PositionTracker_GetGoLocationVelocity(void);

/* ---- TrajectoryTracker：轨迹动态加速度规划（含超时安全停车） ---- */
void    USR_MotionPlanner_TrajectoryTracker_Init(int32_t update_timeout);
void    USR_MotionPlanner_TrajectoryTracker_SetSlowDownVelocityAcc(int32_t value);
void    USR_MotionPlanner_TrajectoryTracker_NewTask(int32_t real_location,
                                                    int32_t real_speed);
void    USR_MotionPlanner_TrajectoryTracker_CalcSoftGoal(int32_t goal_position,
                                                         int32_t goal_velocity);
int32_t USR_MotionPlanner_TrajectoryTracker_GetGoTrajPosition(void);
int32_t USR_MotionPlanner_TrajectoryTracker_GetGoTrajVelocity(void);

#ifdef __cplusplus
}
#endif

#endif /* MOTION_PLANNER_USR_H */
