/**
 * @file tkl_thread.c
 * @brief the default weak implements of tuya os thread
 * @version 0.1
 * @date 2019-08-15
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#include "tkl_thread.h"
#include "FreeRTOS.h"
#include "task.h"
#include "mpu_wrappers.h"
#include "portmacro.h"
#include "projdefs.h"

/**
* @brief Create thread
*
* @param[out] thread: thread handle
* @param[in] name: thread name
* @param[in] stack_size: stack size of thread
* @param[in] priority: priority of thread
* @param[in] func: the main thread process function
* @param[in] arg: the args of the func, can be null
*
* @note This API is used for creating thread.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_thread_create(TKL_THREAD_HANDLE* thread,
                           const char* name,
                           uint32_t stack_size,
                           uint32_t priority,
                           THREAD_FUNC_T func,
                           void* const arg)
{
    if (!thread) {
        return OPRT_INVALID_PARM;
    }

    BaseType_t ret = 0;
    // thread stack default in sram
    ret = xTaskCreate(func, name, stack_size / sizeof(portSTACK_TYPE), (void *const)arg, priority, (TaskHandle_t * const )thread);
    if (ret != pdPASS) {
        return OPRT_OS_ADAPTER_THRD_CREAT_FAILED;
    }

    return OPRT_OK;
}

/**
* @brief Terminal thread and release thread resources
*
* @param[in] thread: thread handle
*
* @note This API is used to terminal thread and release thread resources.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_thread_release(TKL_THREAD_HANDLE thread)
{
    if (!thread) {
        return OPRT_INVALID_PARM;
    }

    vTaskDelete(thread);

    return OPRT_OK;
}

/**
* @brief Get the thread stack's watermark
*
* @param[in] thread: thread handle
* @param[out] watermark: watermark in Bytes
*
* @note This API is used to get the thread stack's watermark.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_thread_get_watermark(TKL_THREAD_HANDLE thread, uint32_t* watermark)
{
    // *watermark = uxTaskGetStackHighWaterMark(thread) * sizeof( StackType_t );
    return OPRT_OK;
}

/**
* @brief Get the thread thread handle
*
* @param[out] thread: thread handle
*
* @note This API is used to get the thread handle.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_thread_get_id(TKL_THREAD_HANDLE *thread)
{
    *thread = xTaskGetCurrentTaskHandle();
    return OPRT_OK;
}

/**
* @brief Set name of self thread
*
* @param[in] name: thread name
*
* @note This API is used to set name of self thread.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_thread_set_self_name(const char* name)
{
    if (!name) {
        return OPRT_INVALID_PARM;
    }

    return OPRT_OK;
}



OPERATE_RET tkl_thread_is_self(TKL_THREAD_HANDLE thread, BOOL_T* is_self)
{
    if (NULL == thread || NULL == is_self) {
        return OPRT_INVALID_PARM;
    }

    TKL_THREAD_HANDLE self_handle = xTaskGetCurrentTaskHandle();
    if (NULL == self_handle) {
        return OPRT_OS_ADAPTER_THRD_JUDGE_SELF_FAILED;
    }

    *is_self = (thread == self_handle);

    return OPRT_OK;
}

/**
* @brief Diagnose the thread(dump task stack, etc.)
*
* @param[in] thread: thread handle
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_thread_diagnose(TKL_THREAD_HANDLE thread)
{
    return OPRT_NOT_SUPPORTED;
}

/**
* @brief Create thread in PSRAM
*
* @param[out] thread: thread handle
* @param[in] name: thread name
* @param[in] stack_size: stack size of thread
* @param[in] priority: priority of thread
* @param[in] func: the main thread process function
* @param[in] arg: the args of the func, can be null
*
* @note This API is used for creating thread.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_thread_create_in_psram(TKL_THREAD_HANDLE* thread,
                           const char* name,
                           uint32_t stack_size,
                           uint32_t priority,
                           THREAD_FUNC_T func,
                           void* const arg)
{
    if (!thread) {
        return OPRT_INVALID_PARM;
    }

    BaseType_t ret = 0;
    ret = xTaskCreateInPsram(func, name, stack_size / sizeof(portSTACK_TYPE), (void *const)arg, priority, (TaskHandle_t * const )thread);
    if (ret != pdPASS) {
        return OPRT_OS_ADAPTER_THRD_CREAT_FAILED;
    }

    return OPRT_OK;
}

