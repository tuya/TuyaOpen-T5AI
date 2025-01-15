/**
* @file tkl_watchdog.c
* @brief Common process - adapter the watchdog api
* @version 0.1
* @date 2021-08-06
*
* @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
*
*/
#include "tkl_watchdog.h"

#include "drv_model_pub.h"
#include <driver/wdt.h>

#define WDT_RST_MASK             (0x1FF)
#define PMU_WDT_RST_REG           (0x44000000+0x2*4)

#define SYS_CPU_26M_WDT_CLK_DIV_ADDR      (0x44010000+0xa*4)

extern void bk_printf(const char *fmt, ...);

static void bk_wdt_rst_enable(void) {
    int level = rtos_disable_int();

    REG_WRITE(SYS_CPU_26M_WDT_CLK_DIV_ADDR, 0xc);
    REG_WRITE(PMU_WDT_RST_REG, WDT_RST_MASK);

    rtos_enable_int(level);
}

uint32_t tkl_watchdog_init(TUYA_WDOG_BASE_CFG_T *cfg)
{
    if (cfg->interval_ms > 30000) {
        cfg->interval_ms = 30000;
    }
    bk_printf("tkl_watchdog_init interval_ms(%d)!\r\n", cfg->interval_ms);

    bk_wdt_rst_enable();

    bk_wdt_start((uint32_t)cfg->interval_ms);
    bk_wdt_feed();
    return cfg->interval_ms;
}

OPERATE_RET tkl_watchdog_deinit(void)
{
    bk_wdt_stop();
    return OPRT_OK;
}

OPERATE_RET tkl_watchdog_refresh(void)
{
    bk_wdt_feed();
    return OPRT_OK;
}
