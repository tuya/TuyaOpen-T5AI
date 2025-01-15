/**
 * @file tkl_timer.h
 * @brief Common process - adapter the hardware timer api
 * @version 0.1
 * @date 2021-08-06
 *
 * @copyright Copyright 2018-2021 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TKL_LVGL_H__
#define __TKL_LVGL_H__

#include "tuya_cloud_types.h"
#include "tkl_display.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*TKL_LVGL_EVENT_CB)(void *buf, uint32_t len, void *args);

typedef struct {
    TKL_LVGL_EVENT_CB   recv_cb;
    void                *recv_arg;
} TKL_LVGL_CFG_T;


/**
 * @brief lvgl init
 *
 * @param[in] cfg configure
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_lvgl_init(TKL_DISP_INFO_S *disp_info, TKL_LVGL_CFG_T *cfg);

/**
 * @brief lvgl message sync
 *
 * @param[in] msg message
 * @param[in] length
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_lvgl_msg_sync(uint8_t *msg, uint32_t length);

/**
 * @brief lvgl start
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_lvgl_start(void);

/**
 * @brief lvgl stop
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_lvgl_stop(void);

/**
 * @brief lvgl uninit
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_lvgl_uninit(void);



#ifdef __cplusplus
} // extern "C"
#endif

#endif /* !__TKL_LVGL_H__ */
