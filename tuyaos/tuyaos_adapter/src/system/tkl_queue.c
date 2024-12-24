/**
 * @file tkl_queue.c
 * @brief the default weak implements of tuya os queue, this implement only used when OS=linux
 * @version 0.1
 * @date 2019-08-15
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#include "tkl_queue.h"
#include "tkl_system.h"
#include "tkl_output.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

typedef struct {
    xQueueHandle queue;
} QUEUE_MANAGE, *P_QUEUE_MANAGE;

/**
 * @brief Create message queue
 *
 * @param[in] msgsize message size
 * @param[in] msgcount message number
 * @param[out] queue the queue handle created
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_queue_create_init(TKL_QUEUE_HANDLE *queue, int msgsize, int msgcount)
{
    if (!queue) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    *queue = NULL;

    *queue = (TKL_QUEUE_HANDLE)xQueueCreate(msgcount, msgsize);
    if (*queue == NULL) {
        return OPRT_OS_ADAPTER_QUEUE_CREAT_FAILED;
    }

	return OPRT_OK;
}

/**
 * @brief post a message to the message queue
 *
 * @param[in] queue the handle of the queue
 * @param[in] data the data of the message
 * @param[in] timeout timeout time
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_queue_post(const TKL_QUEUE_HANDLE queue, void *data, uint32_t timeout)
{
    int ret = pdPASS;

    if (!queue) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

extern uint32_t platform_is_in_interrupt_context( void );
    if (platform_is_in_interrupt_context() == 0) {
        if (timeout == TKL_QUEUE_WAIT_FROEVER) {
            ret = xQueueSend(queue, data, portMAX_DELAY);
        } else {
            uint32_t ticks = timeout / portTICK_RATE_MS;

            if (ticks == 0) {
                ticks = 1;
            }
            ret = xQueueSend(queue, data, ticks);
        }
    } else {
        signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
        ret = xQueueSendFromISR(queue, data, &xHigherPriorityTaskWoken);
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    }

    if (pdPASS != ret) {
        return OPRT_OS_ADAPTER_QUEUE_SEND_FAIL;
    }

	return OPRT_OK;
}

/**
 * @brief fetch message from the message queue
 *
 * @param[in] queue the message queue handle
 * @param[inout] msg the message fetch form the message queue
 * @param[in] timeout timeout time
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_queue_fetch(const TKL_QUEUE_HANDLE queue, void *msg, uint32_t timeout)
{
    void *dummyptr;
    int ret = pdPASS;

    if (!queue) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    if (msg == NULL) {
        msg = &dummyptr;
    }

    if (timeout == TKL_QUEUE_WAIT_FROEVER) {
        ret = xQueueReceive(queue, msg, portMAX_DELAY);
    } else {
        uint32_t ticks = timeout / portTICK_RATE_MS;

        if (ticks == 0) {
            ticks = 1;
        }
        ret = xQueueReceive(queue, msg, ticks);
    }

    if (pdPASS != ret) {
        return OPRT_OS_ADAPTER_QUEUE_SEND_FAIL;
    }

	return OPRT_OK;
}


/**
 * @brief free the message queue
 *
 * @param[in] queue the message queue handle
 *
 * @return void
 */
void tkl_queue_free(const TKL_QUEUE_HANDLE queue)
{
    if (!queue) {
        return ;
    }

    if (uxQueueMessagesWaiting((QueueHandle_t)queue)) {
        /* Line for breakpoint.  Should never break here! */
        portNOP();
        // TODO notify the user of failure.
    }

    vQueueDelete((QueueHandle_t)queue);
}

