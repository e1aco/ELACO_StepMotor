/********
 * @ 文件: elaco_main.h
 * @ 作者: ELACO
 * @ 日期: 2026-07-17
 * @ 版本: 1.0.0
 * @ 说明: 应用主入口头文件，中央包含头
 ********/

#ifndef ELACO_MAIN_H
#define ELACO_MAIN_H

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "stm32f1xx_hal.h"
#include "main.h"

#include "ela_cyclecal.h"
#include "ela_stockfile_usr.h"
#include "ela_stockfile_drv.h"
#include "elaco_calibration_usr.h"

void elaco_main(void);

#define ModTest

#endif

