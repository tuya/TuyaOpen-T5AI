/*
 * tkl_wakeup.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "tkl_wakeup.h"
#include "tkl_memory.h"
#include "sdkconfig.h"
#include <modules/pm.h>
#include <driver/gpio.h>
#if CONFIG_AON_RTC
#include "aon_pmu_driver.h"
#include <driver/aon_rtc_types.h>
#include <driver/hal/hal_aon_rtc_types.h>
#include <driver/aon_rtc.h>
#endif

#define WAKEUP_SOURCE_CNT   6

static TUYA_WAKEUP_SOURCE_BASE_CFG_T  *wakeup_source = NULL;
static uint32_t wakeup_source_init_status = 0;
extern void bk_printf(const char *fmt, ...);

alarm_info_t rtc_alarm;

static int __save_wakeup_node(const TUYA_WAKEUP_SOURCE_BASE_CFG_T  *param)
{
    int i = 0;

    if (wakeup_source == NULL) {
        bk_printf("wakeup source not init, %d\r\n", __LINE__);
        return -1;
    }

    for (i = 0; i < WAKEUP_SOURCE_CNT; i++) {
        if ((param->source == TUYA_WAKEUP_SOURCE_RTC) &&
                (wakeup_source[i].source == TUYA_WAKEUP_SOURCE_RTC)) {
            // rtc
            memcpy(&wakeup_source[i], param, sizeof(TUYA_WAKEUP_SOURCE_BASE_CFG_T));
            return i;
        }
        if ((param->source == TUYA_WAKEUP_SOURCE_GPIO) &&
                (wakeup_source[i].source == TUYA_WAKEUP_SOURCE_GPIO)) {
            // gpio
            if (param->wakeup_para.gpio_param.gpio_num ==
                    wakeup_source[i].wakeup_para.gpio_param.gpio_num) {
                memcpy(&wakeup_source[i], param, sizeof(TUYA_WAKEUP_SOURCE_BASE_CFG_T));
                return i;
            }
        }
    }

    for (i = 0; i < WAKEUP_SOURCE_CNT; i++) {
        if (wakeup_source[i].source == 0xff) {
            memcpy(&wakeup_source[i], param, sizeof(TUYA_WAKEUP_SOURCE_BASE_CFG_T));
            return i;
        }
    }

    bk_printf("no free wakeup space\r\n");
    return -1;
}

#if CONFIG_GPIO_WAKEUP_SUPPORT
static void __gpio_wakeup_source_set(gpio_id_t id, TUYA_GPIO_WAKE_TYPE_E level)
{
    gpio_int_type_t int_type = GPIO_INT_TYPE_FALLING_EDGE;

    // TODO irq type
    switch (level) {
        case TUYA_GPIO_WAKEUP_LOW:
            int_type = GPIO_INT_TYPE_LOW_LEVEL;
            break;
        case TUYA_GPIO_WAKEUP_HIGH:
            int_type = GPIO_INT_TYPE_HIGH_LEVEL;
            break;
        case TUYA_GPIO_WAKEUP_RISE:
            int_type = GPIO_INT_TYPE_RISING_EDGE;
            break;
        case TUYA_GPIO_WAKEUP_FALL:
            int_type = GPIO_INT_TYPE_FALLING_EDGE;
            break;
        default:
            //rising
            break;
    }

    bk_gpio_register_wakeup_source(id, int_type);
    bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);
}
#endif //CONFIG_GPIO_WAKEUP_SUPPORT

#if CONFIG_AON_RTC
static void __rtc_wakeup_source_set(uint32_t ms)
{
    uint32_t rtc_ms_tick = bk_rtc_get_clock_freq()/1000;
    aon_pmu_drv_lpo_src_set(PM_LPO_SRC_ROSC);

    memcpy(rtc_alarm.name, "low_vol",sizeof("low_vol"));
    rtc_alarm.period_tick = ms * rtc_ms_tick;
    rtc_alarm.period_cnt = 1;
    rtc_alarm.callback = NULL;
    rtc_alarm.param_p = NULL;

    if(ms < 500) {
        bk_printf("param %d invalid ! must > %dms.\r\n", ms, 500);
        return ;
    }
    //force unregister previous if doesn't finish.
    bk_alarm_unregister(AON_RTC_ID_1, rtc_alarm.name);
    bk_alarm_register(AON_RTC_ID_1, &rtc_alarm);
    bk_printf("rtc wakeup: %d, %d\r\n", rtc_ms_tick, ms);
    bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
}
#endif // CONFIG_AON_RTC

// set wakeup source to low level logic
static void tkl_set_ll_wakeup_source(void)
{
    int i = 0;

    if (wakeup_source == NULL) {
        bk_printf("wakeup source not init, %d\r\n", __LINE__);
        return;
    }

    for (i = 0; i < WAKEUP_SOURCE_CNT; i++) {
        if (wakeup_source[i].source == TUYA_WAKEUP_SOURCE_GPIO) {
#if CONFIG_GPIO_WAKEUP_SUPPORT
            bk_printf("set gpio wakeup source %d\r\n",
                    wakeup_source[i].wakeup_para.gpio_param.gpio_num);
            __gpio_wakeup_source_set(wakeup_source[i].wakeup_para.gpio_param.gpio_num,
                    wakeup_source[i].wakeup_para.gpio_param.level);
#endif // CONFIG_GPIO_WAKEUP_SUPPORT
        } else if (wakeup_source[i].source == TUYA_WAKEUP_SOURCE_RTC) {
#if CONFIG_AON_RTC
            bk_printf("set rtc wakeup source %d\r\n",
                    wakeup_source[i].wakeup_para.timer_param.ms);
            __rtc_wakeup_source_set(wakeup_source[i].wakeup_para.timer_param.ms);
#endif // CONFIG_AON_RTC
        }
    }
}

/**
 * @brief wake up source set
 *
 * @param[in] param: wake up source set,
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wakeup_source_set(const TUYA_WAKEUP_SOURCE_BASE_CFG_T  *param)
{
    if (param == NULL)
        return OPRT_INVALID_PARM;

    if (wakeup_source == NULL) {
        wakeup_source = (TUYA_WAKEUP_SOURCE_BASE_CFG_T  *)tkl_system_malloc(WAKEUP_SOURCE_CNT * sizeof(TUYA_WAKEUP_SOURCE_BASE_CFG_T));
        if (wakeup_source == NULL) {
            bk_printf("wakeup source malloc failed\r\n");
            return OPRT_INVALID_PARM;
        }
        memset(wakeup_source, 0xff, WAKEUP_SOURCE_CNT * sizeof(TUYA_WAKEUP_SOURCE_BASE_CFG_T));
    }

    if ((param->source == TUYA_WAKEUP_SOURCE_GPIO) ||
        (param->source == TUYA_WAKEUP_SOURCE_RTC)) {
        int ret = __save_wakeup_node(param);
        if (ret < 0)
            return OPRT_COM_ERROR;
    } else {
        bk_printf("unsupport source: %d\r\n", param->source);
        return OPRT_INVALID_PARM;
    }
    //wakeup_source_init_status |= (1 << param->source);

    tkl_set_ll_wakeup_source();
    bk_printf("wakeup source: %d %x\r\n", param->source, wakeup_source_init_status);

    return OPRT_OK;
}

/**
 * @brief wake up source get
 *
 * @param[in] param:  wake up source get
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wakeup_source_get(TUYA_WAKEUP_SOURCE_BASE_CFG_T *param, uint32_t num, uint32_t *status)
{
    if (param == NULL || num > WAKEUP_SOURCE_CNT) {
        bk_printf("parm is NULL, or num > %d \r\n", WAKEUP_SOURCE_CNT);
        return OPRT_INVALID_PARM;
    }

    if (wakeup_source == NULL) {
        bk_printf("wakeup source not set\r\n");
        return OPRT_NOT_FOUND;
    }

    memcpy(param, wakeup_source, num * sizeof(TUYA_WAKEUP_SOURCE_BASE_CFG_T));
    *status = wakeup_source_init_status;

    return OPRT_OK;
}

/**
 * @brief wake up source clear
 *
 * @param[in] param:  wake up source clear,
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wakeup_source_clear(const TUYA_WAKEUP_SOURCE_BASE_CFG_T *param)
{
    if (wakeup_source == NULL) {
        bk_printf("wakeup source not init, %d\r\n", __LINE__);
        return OPRT_COM_ERROR;
    }

    static char clear_flag = 0;

    for (int i = 0; i < WAKEUP_SOURCE_CNT; i++) {
        if(wakeup_source[i].source == TUYA_WAKEUP_SOURCE_RTC) {
            if(param->source == TUYA_WAKEUP_SOURCE_RTC) {
                bk_alarm_unregister(AON_RTC_ID_1, rtc_alarm.name);
                memset(&wakeup_source[i], 0xFF, sizeof(TUYA_WAKEUP_SOURCE_BASE_CFG_T));
                clear_flag = 1;
            }
        } else if(wakeup_source[i].source == TUYA_WAKEUP_SOURCE_GPIO) {
            if((param->source == TUYA_WAKEUP_SOURCE_GPIO) && (wakeup_source[i].wakeup_para.gpio_param.gpio_num == param->wakeup_para.gpio_param.gpio_num)) {
                bk_gpio_unregister_wakeup_source(param->wakeup_para.gpio_param.gpio_num);
                memset(&wakeup_source[i], 0xFF, sizeof(TUYA_WAKEUP_SOURCE_BASE_CFG_T));
                clear_flag = 1;
            }
        } else if(wakeup_source[i].source == TUYA_WAKEUP_SOURCE_TIMER) {

        }

        if(clear_flag && wakeup_source[i + 1].source != 0xFF) {
            memcpy(&wakeup_source[i], &wakeup_source[i + 1], sizeof(TUYA_WAKEUP_SOURCE_BASE_CFG_T));
        }
    }

    return OPRT_OK;
}



