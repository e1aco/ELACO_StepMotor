/*****************************************************************************
 * @文件: config_usr.h
 * @作者: cl
 * @日期: 2026-08-17
 * @版本: v1.0
 * @说明: 配置持久化用户层（BoardConfig_T 定义/默认值/加载校验/提交恢复
 *   状态机，存储介质为 Flash 模拟 EEPROM）
 * @平台: STM32F103RET6
 * @依赖: eeprom_usr, motor_usr
 ****************************************************************************/
#ifndef CONFIG_USR_H
#define CONFIG_USR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "motor_usr.h"

/* ==== 常量定义 ==== */
/* 配置区有效校验字（掉电半写检测；知识库 stm32_flash.md 建议 magic+校验） */
#define USR_CONFIG_MAGIC         0x5A5AA5A5U

/* 默认配置值（依据 .cl/memory/ config_default_* 组，实测值优先） */
#define USR_CONFIG_DEF_NODE_ID   2U      /* config_default_can=2 */
#define USR_CONFIG_DEF_MODE      MODE_COMMAND_POSITION /* config_default_mode */
#define USR_CONFIG_DEF_CUR_LIMIT 2000    /* motor_test_limits=2000mA 额定 2A */
#define USR_CONFIG_DEF_VEL_LIMIT (2 * USR_MOTOR_SUBDIVIDE_STEPS)
                                         /* planner_rated_velocity=102400 2圈/s */
#define USR_CONFIG_DEF_VEL_ACC   (100 * USR_MOTOR_SUBDIVIDE_STEPS)
                                         /* planner_rated_velocity_acc=5120000 */
#define USR_CONFIG_DEF_CALI_CUR  2000    /* config_default_calib_current */
#define USR_CONFIG_DEF_POS_KP    32768   /* motor_cascade_poskp 标定 */
#define USR_CONFIG_DEF_PID_KP    10      /* motor_minloop_control 实测 10 */
#define USR_CONFIG_DEF_PID_KD    400     /* motor_loop_damping 实测 400 */

/* ==== 类型定义 ==== */
/* 配置状态（复刻参考 configurations.h configStatus_t 三态语义） */
typedef enum {
    CONFIG_RESTORE = 0,   /* 恢复出厂（写回后复位） */
    CONFIG_OK,            /* 有效配置 */
    CONFIG_COMMIT         /* 待提交（改字段后置位，主循环落盘） */
} Config_Status_T;

/* 板级配置（参考 BoardConfig_t 裁剪：dce_kp/kv/ki/kd 已退役 → posKp，
   pid_ki 未用裁剪；新增 magic 校验字；字段顺序对齐参考 configurations.h） */
typedef struct {
    uint32_t         magic;              /* 校验字（掉电半写检测） */
    Config_Status_T  configStatus;       /* 配置状态 */
    uint32_t         canNodeId;          /* CAN 节点 ID */
    int32_t          encoderHomeOffset;  /* 编码器零位偏移（细分步） */
    uint32_t         defaultMode;        /* 默认运行模式 */
    int32_t          currentLimit;       /* 电流限幅（mA）→ ratedCurrent */
    int32_t          velocityLimit;      /* 速度限幅（细分步/s）→ ratedVelocity */
    int32_t          velocityAcc;        /* 加速度（细分步/s²）→ ratedVelocityAcc */
    int32_t          calibrationCurrent; /* 校准电流（mA） */
    int32_t          posKp;              /* 位置环增益（标定 32768） */
    int32_t          pidKp;              /* 速度环增益 */
    int32_t          pidKd;              /* 速度环阻尼 */
    bool             enableMotorOnBoot;  /* 上电自动使能 */
    bool             enableStallProtect; /* 堵转保护开关 → stallProtectSwitch */
} BoardConfig_T;

/* ==== 全局实例 ==== */
extern BoardConfig_T g_board_config;

/* ==== 接口 ==== */
uint8_t        USR_Config_Init(void);
BoardConfig_T *USR_Config_Get(void);
void           USR_Config_Commit(void);
void           USR_Config_Restore(void);
void           USR_Config_TickMainLoop(void);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_USR_H */
