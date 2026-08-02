/********
 * @ 文件: ela_motion_planner_usr.h
 * @ 作者: ELACO
 * @ 日期: 2026-08-02
 * @ 版本: 1.0.0
 * @ 说明: 运动轨迹规划器（复刻参考 zhjStepMotor motion_planner.h）。
 *         含 CurrentTracker / VelocityTracker / PositionTracker /
 *         PositionInterpolator / TrajectoryTracker 五类软目标规划
 * @ 依赖: 无（纯算法，配置经 g_motion_config 注入）
 ********/

#ifndef ELA_MOTION_PLANNER_USR_H
#define ELA_MOTION_PLANNER_USR_H

#include <stdint.h>
#include <stdbool.h>

/* 控制频率 */
#define CONTROL_FREQUENCY  20000   /* Hz */
#define CONTROL_PERIOD_US  50      /* 微秒 */

/* ==================== 配置 ==================== */
typedef struct {
    int32_t encoderHomeOffset;
    int32_t caliCurrent;
    int32_t ratedCurrent;
    int32_t ratedVelocity;
    int32_t ratedVelocityAcc;
    int32_t ratedCurrentAcc;
} MotionPlanner_Config_t;

/* 全局配置指针（需在 c 文件中赋值） */
extern MotionPlanner_Config_t* g_motion_config;

/* ==================== CurrentTracker ==================== */
extern int32_t g_go_current;

void CurrentTracker_Init(void);
void CurrentTracker_SetCurrentAcc(int32_t currentAcc);
void CurrentTracker_NewTask(int32_t realCurrent);
void CurrentTracker_CalcSoftGoal(int32_t goalCurrent);

/* ==================== VelocityTracker ==================== */
extern int32_t g_go_velocity;

void VelocityTracker_Init(void);
void VelocityTracker_SetVelocityAcc(int32_t velocityAcc);
void VelocityTracker_NewTask(int32_t realVelocity);
void VelocityTracker_CalcSoftGoal(int32_t goalVelocity);

/* ==================== PositionTracker ==================== */
extern int32_t g_go_location;
extern int32_t g_go_location_velocity;

void PositionTracker_Init(void);
void PositionTracker_SetVelocityAcc(int32_t value);
void PositionTracker_NewTask(int32_t realLocation, int32_t realSpeed);
void PositionTracker_CalcSoftGoal(int32_t goalPosition);

/* ==================== PositionInterpolator ==================== */
extern int32_t g_interp_go_position;
extern int32_t g_interp_go_velocity;

void PositionInterpolator_Init(void);
void PositionInterpolator_NewTask(int32_t realPosition, int32_t realVelocity);
void PositionInterpolator_CalcSoftGoal(int32_t goalPosition);

/* ==================== TrajectoryTracker ==================== */
extern int32_t g_traj_go_position;
extern int32_t g_traj_go_velocity;

void TrajectoryTracker_Init(int32_t updateTimeout);
void TrajectoryTracker_SetSlowDownVelocityAcc(int32_t value);
void TrajectoryTracker_NewTask(int32_t realLocation, int32_t realSpeed);
void TrajectoryTracker_CalcSoftGoal(int32_t goalPosition, int32_t goalVelocity);

#endif
