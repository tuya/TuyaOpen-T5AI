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
#include "tkl_ipc.h"
#include "sdkconfig.h"

extern void bk_printf(const char *fmt, ...);

void tkl_sys_ipc_func(struct ipc_msg_s *msg)
{
    switch(msg->subtype) {
        case TKL_IPC_TYPE_SYS_REBOOT:
        {
            tkl_system_reset();
        }
            break;

        default:
            break;
    }


    return;
}

/**
 * @brief system enter critical
 *
 * @param[in]   none
 * @return  irq mask
 */
uint32_t tkl_system_enter_critical(void)
{
    return rtos_disable_int();
}

/**
 * @brief system exit critical
 *
 * @param[in]   irq_mask: irq mask 
 * @return  none
 */
void tkl_system_exit_critical(uint32_t irq_mask)
{
    rtos_enable_int(irq_mask);
}

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

extern OPERATE_RET tkl_ipc_send_no_sync(const uint8_t *buf, uint32_t buf_len);

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
#if CONFIG_SYS_CPU0
    bk_reboot();
#else
    struct ipc_msg_s msg = {0};
    msg.type = TKL_IPC_TYPE_SYS;
    msg.subtype = TKL_IPC_TYPE_SYS_REBOOT;

    tkl_ipc_send_no_sync((const uint8_t *)&msg, sizeof(struct ipc_msg_s));
    // TODO ret
    while(1) {
        tkl_system_sleep(100);
    }
#endif
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
TUYA_RESET_REASON_E tkl_system_get_reset_reason(CHAR_T** describe)
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
            // ty_value = TUYA_RESET_REASON_UNKNOWN;
            ty_value = TUYA_RESET_REASON_POWERON;
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
    // TODO
#if 0
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
#endif
    return OPRT_NOT_SUPPORTED;
}