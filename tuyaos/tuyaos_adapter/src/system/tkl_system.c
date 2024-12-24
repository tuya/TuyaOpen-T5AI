/**
 * @file tkl_system.c
 * @brief the default weak implements of tuya os system api, this implement only used when OS=linux
 * @version 0.1
 * @date 2019-08-15
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#include "tkl_system.h"
#include "FreeRTOS.h"
#include "task.h"
#include <os/os.h>
#include <components/system.h>
#include "reset_reason.h"
#include <driver/trng.h>
#include "tkl_memory.h"
#include <driver/otp.h>

extern void bk_printf(const char *fmt, ...);

/**
* @brief Get system ticket count
*
* @param void
*
* @note This API is used to get system ticket count.
*
* @return system ticket count
*/
SYS_TICK_T tkl_system_get_tick_count(void)
{
    return (SYS_TICK_T)xTaskGetTickCount();
}

/**
* @brief Get system millisecond
*
* @param none
*
* @return system millisecond
*/
SYS_TIME_T tkl_system_get_millisecond(void)
{
    return (SYS_TIME_T)(tkl_system_get_tick_count() * portTICK_RATE_MS);
}

/**
* @brief System sleep
*
* @param[in] msTime: time in MS
*
* @note This API is used for system sleep.
*
* @return void
*/
void tkl_system_sleep(const uint32_t num_ms)
{
    uint32_t ticks = num_ms / portTICK_RATE_MS;

    if (ticks == 0) {
        ticks = 1;
    }

    vTaskDelay(ticks);
}


/**
* @brief System reset
*
* @param void
*
* @note This API is used for system reset.
*
* @return void
*/
void tkl_system_reset(void)
{
    bk_reboot();
	return;
}

/**
* @brief Get free heap size
*
* @param void
*
* @note This API is used for getting free heap size.
*
* @return size of free heap
*/
int tkl_system_get_free_heap_size(void)
{
    return (int)xPortGetFreeHeapSize();
}

/**
* @brief Get system reset reason
*
* @param void
*
* @note This API is used for getting system reset reason.
*
* @return reset reason of system
*/
TUYA_RESET_REASON_E tkl_system_get_reset_reason(char** describe)
{
    unsigned char value = bk_misc_get_reset_reason() & 0xFF;
    TUYA_RESET_REASON_E ty_value;

    switch (value) {
        case RESET_SOURCE_POWERON:
            ty_value = TUYA_RESET_REASON_POWERON;
            break;

        case RESET_SOURCE_REBOOT:
            ty_value = TUYA_RESET_REASON_SOFTWARE;
            break;

        case RESET_SOURCE_WATCHDOG:
        case RESET_SOURCE_NMI_WDT:
            ty_value = TUYA_RESET_REASON_HW_WDOG;
            break;

        case RESET_SOURCE_DEEPPS_GPIO:
        case RESET_SOURCE_DEEPPS_RTC:
        case RESET_SOURCE_DEEPPS_USB:
        case RESET_SOURCE_DEEPPS_TOUCH:
        case RESET_SOURCE_SUPER_DEEP:
            ty_value = TUYA_RESET_REASON_DEEPSLEEP;
            break;

        case RESET_SOURCE_CRASH_ILLEGAL_JUMP:
        case RESET_SOURCE_CRASH_UNDEFINED:
        case RESET_SOURCE_CRASH_PREFETCH_ABORT:
        case RESET_SOURCE_CRASH_DATA_ABORT:
        case RESET_SOURCE_CRASH_UNUSED:
        case RESET_SOURCE_CRASH_ILLEGAL_INSTRUCTION:
        case RESET_SOURCE_CRASH_MISALIGNED:
        case RESET_SOURCE_CRASH_ASSERT:
            ty_value = TUYA_RESET_REASON_CRASH;
            break;

        case RESET_SOURCE_HARD_FAULT:
        case RESET_SOURCE_MPU_FAULT:
        case RESET_SOURCE_BUS_FAULT:
        case RESET_SOURCE_USAGE_FAULT:
        case RESET_SOURCE_SECURE_FAULT:
        case RESET_SOURCE_DEFAULT_EXCEPTION:
            ty_value = TUYA_RESET_REASON_FAULT;
            break;

        default:
            ty_value = TUYA_RESET_REASON_UNKNOWN;
            break;
    }

    bk_printf("bk_value:%x, ty_value:%x\r\n", value, ty_value);
    return ty_value;

}

/**
* @brief Get a random number in the specified range
*
* @param[in] range: range
*
* @note This API is used for getting a random number in the specified range
*
* @return a random number in the specified range
*/
int tkl_system_get_random(const uint32_t range)
{
    unsigned int trange = range;

    if (range == 0) {
        trange = 0xFF;
    }

    static char exec_flag = FALSE;

    if (!exec_flag) {
        exec_flag = TRUE;
    }

    return (bk_rand() % trange);
}

#define EFUSE_DEVICE_ID_BYTE_NUM 5
#define OTP_DEVICE_ID 30

OPERATE_RET tkl_system_get_cpu_info(TUYA_CPU_INFO_T **cpu_ary, int *cpu_cnt)
{
    TUYA_CPU_INFO_T *cpu = tkl_system_malloc(sizeof(TUYA_CPU_INFO_T));
    if (NULL == cpu) {
        return OPRT_MALLOC_FAILED;
    }
    memset(cpu, 0, sizeof(TUYA_CPU_INFO_T));
    bk_otp_apb_read(OTP_DEVICE_ID, cpu->chipid, EFUSE_DEVICE_ID_BYTE_NUM);
    cpu->chipidlen = EFUSE_DEVICE_ID_BYTE_NUM;
    if (cpu_cnt) {
        *cpu_cnt = 1;
    }

    *cpu_ary = cpu;

    return OPRT_OK;
}
