/**
 * @file tkl_mutex.c
 * @brief the default weak implements of tuya os mutex, this implement only used when OS=linux
 * @version 0.1
 * @date 2019-08-15
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#include "tkl_mutex.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


extern void bk_printf(const char *fmt, ...);
/**
* @brief Create mutex
*
* @param[out] pMutexHandle: mutex handle
*
* @note This API is used to create mutex.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_mutex_create_init(TKL_MUTEX_HANDLE *handle)
{
    if(!handle)
        return OPRT_INVALID_PARM;

#if configUSE_RECURSIVE_MUTEXES
	*handle = (TKL_MUTEX_HANDLE)xSemaphoreCreateRecursiveMutex();
#else
	*handle = (TKL_MUTEX_HANDLE)xSemaphoreCreateMutex();
#endif

	if (NULL == *handle) {
		return OPRT_OS_ADAPTER_MUTEX_CREAT_FAILED;
	}

    return OPRT_OK;
}

/**
* @brief Lock mutex
*
* @param[in] mutexHandle: mutex handle
*
* @note This API is used to lock mutex.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_mutex_lock(const TKL_MUTEX_HANDLE handle)
{
    if(!handle) {
        return OPRT_INVALID_PARM;
    }

    BaseType_t ret;
#if configUSE_RECURSIVE_MUTEXES
    ret = xSemaphoreTakeRecursive(handle, portMAX_DELAY);
#else
    ret = xSemaphoreTake(handle, portMAX_DELAY);
#endif
    if (pdTRUE != ret) {
        return OPRT_OS_ADAPTER_MUTEX_LOCK_FAILED;
    }

    return OPRT_OK;
}

/**
* @brief Unlock mutex
*
* @param[in] mutexHandle: mutex handle
*
* @note This API is used to unlock mutex.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_mutex_unlock(const TKL_MUTEX_HANDLE handle)
{
    BaseType_t ret;

    if(!handle) {
        return OPRT_INVALID_PARM;
    }

#if configUSE_RECURSIVE_MUTEXES
    ret = xSemaphoreGiveRecursive(handle);
#else
    ret = xSemaphoreGive(handle);
#endif
    // 互斥量不能用于中断
    //
    // extern uint32_t bk_wlan_get_INT_status(void);
    // if (0 == bk_wlan_get_INT_status()) {
    //     ret = xSemaphoreGive(handle);
    // } else {
    //     signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    //     ret = xSemaphoreGiveFromISR(handle, &xHigherPriorityTaskWoken);
    //     portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    // }
    if (pdTRUE != ret) {
        return OPRT_OS_ADAPTER_MUTEX_UNLOCK_FAILED;
    }

    return OPRT_OK;
}

/**
* @brief Release mutex
*
* @param[in] mutexHandle: mutex handle
*
* @note This API is used to release mutex.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_mutex_release(const TKL_MUTEX_HANDLE handle)
{
    if(!handle) {
        return OPRT_INVALID_PARM;
    }

    vSemaphoreDelete(handle);

    return OPRT_OK;
}

/**
* @brief Try Lock mutex
*
* @param[in] mutexHandle: mutex handle
*
* @note This API is used to try lock mutex.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_mutex_trylock(const TKL_MUTEX_HANDLE mutexHandle)
{
    return OPRT_NOT_SUPPORTED;
}
