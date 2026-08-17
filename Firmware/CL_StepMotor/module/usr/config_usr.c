/*****************************************************************************
 * @文件: config_usr.c
 * @作者: cl
 * @日期: 2026-08-17
 * @版本: v1.0
 * @说明: 配置持久化用户层实现（复刻参考 configurations.h + main.c 加载
 *   逻辑：读 EEPROM → magic/configStatus 校验 → 无效填默认并回写 →
 *   主循环处理 CONFIG_COMMIT 落盘 / CONFIG_RESTORE 恢复复位）
 * @平台: STM32F103RET6
 * @依赖: eeprom_usr, motor_usr
 ****************************************************************************/
#include "config_usr.h"
#include "eeprom_usr.h"
#include "flash_drv.h"
#include "stm32f1xx_hal.h"

/* ==== 常量定义 ==== */
/* ==== 类型定义 ==== */
/* ==== 全局实例 ==== */
BoardConfig_T g_board_config;

/* ==== 内部工具 ==== */
/**
 * @输入 无
 * @输出 无
 * @说明 填默认配置（configStatus=OK + magic，直接可用）：
 *   参考 main.c 默认值分支 + .cl/memory/ 实测值覆盖
 * 依据 .cl/memory/ config_default_* + planner_* + motor_* 各组推导值
 */
static void S_SetDefaults(void)
{
    g_board_config.magic              = USR_CONFIG_MAGIC;
    g_board_config.configStatus       = CONFIG_OK;
    g_board_config.canNodeId          = USR_CONFIG_DEF_NODE_ID;
    g_board_config.encoderHomeOffset  = 0;
    g_board_config.defaultMode        = USR_CONFIG_DEF_MODE;
    g_board_config.currentLimit       = USR_CONFIG_DEF_CUR_LIMIT;
    g_board_config.velocityLimit      = USR_CONFIG_DEF_VEL_LIMIT;
    g_board_config.velocityAcc        = USR_CONFIG_DEF_VEL_ACC;
    g_board_config.calibrationCurrent = USR_CONFIG_DEF_CALI_CUR;
    g_board_config.posKp              = USR_CONFIG_DEF_POS_KP;
    g_board_config.pidKp              = USR_CONFIG_DEF_PID_KP;
    g_board_config.pidKd              = USR_CONFIG_DEF_PID_KD;
    g_board_config.enableMotorOnBoot  = false;  /* 安全优先：上电不使能 */
    g_board_config.enableStallProtect = true;   /* 8/15 后确认默认开 */
}

/* ==== 接口实现 ==== */
/**
 * @输入 无
 * @输出 1=读到有效配置；0=首次/损坏，已填默认并回写 Flash
 * @说明 加载配置：magic 不符（掉电半写/未初始化）或状态非 OK（恢复出厂
 *   残留）→ 填默认并落盘（复刻参考 main.c 步骤 4/5）
 * 依据 .cl/memory/ config_default_* 组 + stm32_flash.md magic 校验建议
 */
uint8_t USR_Config_Init(void)
{
    USR_EEPROM_Read(&g_flash_data, 0U, &g_board_config,
                    sizeof(BoardConfig_T));

    if ((g_board_config.magic != USR_CONFIG_MAGIC)
            || (g_board_config.configStatus != CONFIG_OK))
    {
        S_SetDefaults();
        USR_EEPROM_Write(&g_flash_data, 0U, &g_board_config,
                         sizeof(BoardConfig_T));
        return 0U;
    }

    return 1U;
}

/**
 * @输入 无
 * @输出 板级配置指针（全局 g_board_config 地址）
 * @说明 配置读取入口（命令层改字段用指针直改，改完调 Commit）
 */
BoardConfig_T *USR_Config_Get(void)
{
    return &g_board_config;
}

/**
 * @输入 无
 * @输出 无
 * @说明 置待提交标志：命令层修改配置字段后调用，主循环落盘
 *   （复刻参考 uart_cmd 改字段 + configStatus=CONFIG_COMMIT 语义）
 */
void USR_Config_Commit(void)
{
    g_board_config.configStatus = CONFIG_COMMIT;
}

/**
 * @输入 无
 * @输出 无
 * @说明 置恢复出厂标志：主循环写回 + 系统复位（复位后 Init 判
 *   configStatus != OK → 填默认，等效出厂恢复）
 */
void USR_Config_Restore(void)
{
    g_board_config.configStatus = CONFIG_RESTORE;
}

/**
 * @输入 无
 * @输出 无
 * @说明 主循环周期调用：处理待提交/恢复出厂（复刻参考 main.c 主循环
 *   CONFIG_COMMIT/CONFIG_RESTORE 分支；写回由 EEPROM 惰性擦除完成）
 */
void USR_Config_TickMainLoop(void)
{
    if (g_board_config.configStatus == CONFIG_COMMIT)
    {
        g_board_config.configStatus = CONFIG_OK;
        USR_EEPROM_Write(&g_flash_data, 0U, &g_board_config,
                         sizeof(BoardConfig_T));
    }
    else if (g_board_config.configStatus == CONFIG_RESTORE)
    {
        /* 保持 RESTORE 状态写入：复位后 Init 判非 OK → 填默认 */
        USR_EEPROM_Write(&g_flash_data, 0U, &g_board_config,
                         sizeof(BoardConfig_T));
        HAL_NVIC_SystemReset();
    }
}
