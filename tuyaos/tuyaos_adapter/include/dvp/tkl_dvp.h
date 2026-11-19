/**
* @file tkl_dvp.h
* @brief Common process - camera dvp process
* @version 0.1
* @date 2025-06-09
*
* @copyright Copyright 2021-2025 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_DVP_H__
#define __TKL_DVP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"

/**
 * @brief tkl_dvp_init
 * 
 * @param[in] cfg: camera device cfg
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_dvp_init(TUYA_DVP_CFG_T *dvp_cfg);

/**
 * @brief tkl_dvp_deinit
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_dvp_deinit(void);

/**
 * @brief tkl_dvp_start
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_dvp_start(void);

/**
 * @brief tkl_dvp_stop
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_dvp_stop(void);

/**
 * @brief Adjust the H.264 encoder target bitrate.
 *
 * The BK H.264 encoder has no absolute bitrate register. This selects the
 * closest calibrated quality preset at or below @p kbps and asks the driver
 * to apply it at the end of the next encoded frame.
 *
 * @param[in] kbps target bitrate in kbps
 *
 * @return OPRT_OK when the request was accepted;
 *         OPRT_RESOURCE_NOT_READY when a previous request is still pending;
 *         OPRT_NOT_SUPPORTED when the DVP is not in H.264 mode or @p kbps is
 *         below the lowest calibrated preset;
 *         OPRT_COM_ERROR when the DVP is not running.
 */
OPERATE_RET tkl_dvp_set_h264_bitrate(uint32_t kbps);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif