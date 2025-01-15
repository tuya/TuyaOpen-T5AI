// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "bk_dm_bluetooth_types.h"
#include "bk_dm_a2dp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief           Register application callback function to A2DP module. used by both A2DP source
 *                  and sink.
 *
 * @param[in]       callback: A2DP event callback function
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  -  others: fail
 */
bt_err_t bk_bt_a2dp_register_callback(bk_bt_a2dp_cb_t callback);

/**
 *
 * @brief           Initialize the bluetooth A2DP source module. A2DP can work independently.
 *
 * @attention       sink/source must init only one.
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: if the initialization request is sent to lower layer successfully
 *                  - others: fail
 *
 */
bt_err_t bk_bt_a2dp_source_init(void);

/**
 *
 * @brief           De-initialize for A2DP source module. This function
 *                  should be called only after bk_bt_a2dp_source_init() completes successfully,
 *                  and BK_A2DP_PROF_STATE_EVT will reported to APP layer.
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: if the initialization request is sent to lower layer successfully
 *                  - others: fail
 *
 */
bt_err_t bk_bt_a2dp_source_deinit(void);

/**
 * @brief           Register A2DP source data input function. For now, the input should be PCM data stream.

 *
 * @param[in]       callback: A2DP source data callback function
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 *
 */
bt_err_t bk_a2dp_source_register_data_callback(bk_a2dp_source_data_cb_t callback);

/**
 * @brief           set a2dp source data format
 *
 * @param[in]       sample_rate: pcm data sample rate, such as 8000 16000 32000 44100 48000.
 *
 * @param[in]       bit_depth : pcm data bit depth, now support 16 bits only.
 *
 * @param[in]       channel_count : pcm data channel count, support 1 or 2 now.
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_a2dp_source_set_pcm_data_format(uint32_t sample_rate, uint8_t bit_depth, uint8_t channel_count);

/**
 * @brief           register pcm encode callback.
 *
 * @param[in]       cb: callback.
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_a2dp_source_register_pcm_encode_callback(bk_a2dp_source_pcm_encode_cb_t cb);

/**
 * @brief           register pcm resample callback, will be used if input pcm is not match a2dp negotiation, see BK_A2DP_AUDIO_SOURCE_CFG_EVT.
 *
 * @param[in]       cb: callback.
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail
 */
bt_err_t bk_a2dp_source_register_pcm_resample_callback(bk_a2dp_source_pcm_resample_cb_t cb);

/**
 *
 * @brief           Connect to remote A2DP sink device. This API must be called
 *                  after bk_bt_a2dp_source_init()
 *
 * @param[in]       addr: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: connect request is sent to lower layer successfully
 *                  - others: fail
 *
 */
bt_err_t bk_bt_a2dp_source_connect(uint8_t *addr);

/**
 *
 * @brief           Disconnect from the remote A2DP sink device. This API must be called
 *                  after bk_bt_a2dp_source_init()
 *
 * @param[in]       addr: remote bluetooth device address
 * @return
 *
 *                  - BK_ERR_BT_SUCCESS: connect request is sent to lower layer successfully
 *                  - others: fail
 *
 */
bt_err_t bk_bt_a2dp_source_disconnect(uint8_t *addr);

/**
 *
 * @brief           Media control commands. This API can be used for both A2DP sink
 *                  and must be called after bk_bt_a2dp_source_init()
 *
 * @param[in]       ctrl: control commands for A2DP action
 *
 * @attention       this function only used for a2dp source
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: control command is sent to lower layer successfully
 *                  - others: fail
 *
 */
bt_err_t bk_a2dp_media_ctrl(bk_a2dp_media_ctrl_t ctrl);



/**
 *
 * @brief     Initialize the bluetooth A2DP sink module.
 *
 * @param[in]   aac_supported: 1 means aac is supported, 0 means aac is not supported.
 *
 * @return
 *            - BK_ERR_BT_SUCCESS: the initialization request is successfully
 *            -  others: fail
 */
bt_err_t bk_bt_a2dp_sink_init(uint8_t aac_supported);

/**
 *
 * @brief           De-initialize for A2DP sink module. This function
 *                  should be called only after bk_bt_a2dp_sink_init() completes successfully,
 *                  and BK_A2DP_PROF_STATE_EVT will reported to APP layer.
 *
 * @return
 *            - BK_ERR_BT_SUCCESS: the deinitialization request is successfully
 *            -  others: fail
 *
 */
bt_err_t bk_bt_a2dp_sink_deinit(void);

/**
 * @brief           Register A2DP sink data output function;
 *
 * @param[in]       callback: A2DP sink data callback function
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  -  others: fail
 */
bt_err_t bk_bt_a2dp_sink_register_data_callback(bk_bt_sink_data_cb_t callback);

/**
 *
 * @brief           Connect to remote bluetooth A2DP source device. This API must be called after
 *                  bk_bt_a2dp_sink_init().
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  -  others: fail
 *
 */
bt_err_t bk_bt_a2dp_sink_connect(uint8_t *remote_bda);

/**
 *
 * @brief           Disconnect from the remote A2DP source device. This API must be called after
 *                  bk_bt_a2dp_sink_init().
 *
 * @param[in]       remote_bda: remote bluetooth device address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  -  others: fail
 *
 */
bt_err_t bk_bt_a2dp_sink_disconnect(uint8_t *remote_bda);

/**
 *
 * @brief           Set delay reporting value.This API must be called after
 *                  bk_bt_a2dp_sink_init().
 *
 * @param[in]       delay_value: reporting value is in 1/10 millisecond
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: delay value is sent to lower layer successfully
 *                  - others: fail
 *
 */
bt_err_t bk_bt_a2dp_sink_set_delay_value(uint16_t delay_value);

/**
 *
 * @brief           Get delay reporting value. This API must be called after
 *                  bk_bt_a2dp_sink_init().
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: if the request is sent successfully
 *                  - others: fail
 *
 */
bt_err_t bk_bt_a2dp_sink_get_delay_value(void);

#ifdef __cplusplus
}
#endif

