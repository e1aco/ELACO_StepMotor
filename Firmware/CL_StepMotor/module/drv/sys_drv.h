/*****************************************************************************
 * @文件: sys_drv.h
 * @作者: cl
 * @日期: 2026-08-12
 * @版本: v1.0
 * @说明: 系统驱动层（复位/延时等原语，无业务逻辑）
 * @平台: STM32F103RET6
 * @依赖: stm32f1xx_hal.h
 ****************************************************************************/
#ifndef SYS_DRV_H
#define SYS_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ==== 常量定义 ==== */
/* ==== 接口 ==== */
void     DRV_Sys_SystemReset(void);
uint32_t DRV_Sys_GetTickMs(void);

#ifdef __cplusplus
}
#endif

#endif /* SYS_DRV_H */
