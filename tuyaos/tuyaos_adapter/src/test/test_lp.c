/*
 * test_lp.c
 * Copyright (C) 2025 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "cli.h"
#include "cli_tuya_test.h"
#include "tuya_cloud_types.h"
#include "tkl_wifi.h"


#if CONFIG_SYS_CPU0 && CONFIG_SOC_BK7258
#include "modules/pm.h"
#include "tkl_wakeup.h"
#include "tkl_semaphore.h"
#include "tkl_audio.h"
#include "FreeRTOS.h"
#include "task.h"
TaskHandle_t __thread_handle = NULL;
TKL_SEM_HANDLE __test_sem;
static void __test_wakeup_func(void *arg)
{
    int cnt = 0;
    tkl_semaphore_create_init(&__test_sem, 0, 1);
    while (1) {
        tkl_semaphore_wait(__test_sem, 0xffffffff);
        bk_printf("ooooooooooooooooo\r\n");
        bk_printf("eeeeeeeeeeeeeeeee\r\n");
        bk_printf("ccccccccccccccccc\r\n");

        tkl_system_sleep(1000);
    }
}

static void __pm_gpio_callback(void *arg)
{
    // bk_pm_module_vote_sleep_ctrl(12, 0, 0);
    bk_printf("__pm_gpio_callback[%d]: %d\r\n",
            bk_pm_exit_low_vol_wakeup_source_get(), (uint32_t)arg);
    tkl_semaphore_post(__test_sem);
}

static void __set_test_wakeup_source(uint32_t type, uint32_t param)
{
    TUYA_WAKEUP_SOURCE_BASE_CFG_T cfg;
    if (type == 2) {
        bk_printf("wakeup gpio mode\r\n");
        TUYA_GPIO_IRQ_T irq_cfg;
        irq_cfg.mode = TUYA_GPIO_IRQ_RISE;
        irq_cfg.cb = __pm_gpio_callback;
        irq_cfg.arg = (void *)12;
        tkl_gpio_irq_init(12, &irq_cfg);

        if (__thread_handle == NULL)
            xTaskCreate(__test_wakeup_func, "test_func", 1024, NULL, 6, (TaskHandle_t * const )&__thread_handle);

        cfg.source = TUYA_WAKEUP_SOURCE_GPIO;
        cfg.wakeup_para.gpio_param.gpio_num = 12;
        cfg.wakeup_para.gpio_param.level = TUYA_GPIO_WAKEUP_RISE;
    } else if (type == 1) {
        bk_printf("wakeup rtc mode\r\n");
        uint32_t t = (param == 0)? 5: param;
        cfg.source = TUYA_WAKEUP_SOURCE_RTC;
        cfg.wakeup_para.timer_param.timer_num = TUYA_TIMER_NUM_5;
        cfg.wakeup_para.timer_param.mode = TUYA_TIMER_MODE_ONCE;
        cfg.wakeup_para.timer_param.ms = t * 1000;
    }

    tkl_wakeup_source_set(&cfg);
    tkl_system_sleep(200);
}

void cli_lp_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint8_t interval = 0;
    int ret = 0;
    char *msg = NULL;

    if (argc < 2) {
        bk_printf("invalid argc num\r\n");
        goto error;
    }

    if (!os_strcmp(argv[2], "rtc")) {
        uint32_t t = 12;
        if (argv[3] != NULL) {
            uint32_t t = os_strtoul(argv[3], NULL, 10);
        }
        __set_test_wakeup_source(1, t);
    } else if (!os_strcmp(argv[2], "gpio")) {
        TUYA_GPIO_NUM_E pin_id = 12;
        if (argv[3] != NULL) {
            pin_id = os_strtoul(argv[3], NULL, 10);
        }
        __set_test_wakeup_source(2, pin_id);
    } else {
        goto error;
    }

    tkl_wifi_set_lp_mode(1, 10);
    tkl_hci_deinit();

    if (!os_strcmp(argv[1], "deepsleep")) {
        tkl_cpu_sleep_mode_set(1, TUYA_CPU_DEEP_SLEEP);
    } else if (!os_strcmp(argv[1], "sleep")) {
        tkl_cpu_sleep_mode_set(1, TUYA_CPU_SLEEP);
    }

    return;

error:
    bk_printf("Usage: lp deepsleep [rtc|gpio] [rtc time<seconds>|gpio num]");
    msg = WIFI_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return;
}
#endif // CONFIG_SYS_CPU0 && CONFIG_SOC_BK7258


