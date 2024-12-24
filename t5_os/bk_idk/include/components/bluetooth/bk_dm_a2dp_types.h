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


#ifdef __cplusplus
extern "C" {
#endif



/**
 * @brief A2DP callback events
 */
typedef enum
{
    BK_A2DP_CONNECTION_STATE_EVT = 0,           /*!< connection state changed event */
    BK_A2DP_AUDIO_STATE_EVT,                    /*!< audio stream transmission state changed event */
    BK_A2DP_AUDIO_CFG_EVT,                      /*!< audio codec is configured, only used for A2DP SINK */
    BK_A2DP_AUDIO_SOURCE_CFG_EVT,               /*!< audio codec is configured, only used for A2DP SOURCE */
    BK_A2DP_MEDIA_CTRL_ACK_EVT,                /*!< acknowledge event in response to media control commands */
    BK_A2DP_PROF_STATE_EVT,                    /*!< indicate a2dp init&deinit complete */
    BK_A2DP_SNK_SET_DELAY_VALUE_EVT,           /*!< indicate a2dp sink set delay report value complete,  only used for A2DP SINK */
    BK_A2DP_SNK_GET_DELAY_VALUE_EVT,           /*!< indicate a2dp sink get delay report value complete,  only used for A2DP SINK */
} bk_a2dp_cb_event_t;


/**
 * @brief Bluetooth A2DP connection states
 */
typedef enum
{
    BK_A2DP_CONNECTION_STATE_DISCONNECTED = 0, /*!< connection released  */
    BK_A2DP_CONNECTION_STATE_CONNECTING,       /*!< connecting remote device */
    BK_A2DP_CONNECTION_STATE_CONNECTED,        /*!< connection established */
    BK_A2DP_CONNECTION_STATE_DISCONNECTING     /*!< disconnecting remote device */
} bk_a2dp_connection_state_t;

/**
 * @brief Bluetooth A2DP disconnection reason
 */
typedef enum
{
    BK_A2DP_DISC_RSN_NORMAL = 0,               /*!< Finished disconnection that is initiated by local or remote device */
    BK_A2DP_DISC_RSN_ABNORMAL                  /*!< Abnormal disconnection caused by signal loss */
} bk_a2dp_disc_rsn_t;

/**
 * @brief Bluetooth A2DP datapath states
 */
typedef enum
{
    BK_A2DP_AUDIO_STATE_SUSPEND = 0,           /*!< audio stream datapath suspend */
    BK_A2DP_AUDIO_STATE_STARTED,               /*!< audio stream datapath started */
} bk_a2dp_audio_state_t;

/**
 * @brief A2DP media control commands
 */
typedef enum
{
    BK_A2DP_MEDIA_CTRL_START,                  /*!< command to set up media transmission channel */
    BK_A2DP_MEDIA_CTRL_SUSPEND,                /*!< command to suspend media transmission  */
} bk_a2dp_media_ctrl_t;

/**
 * @brief A2DP media control command acknowledgement code
 */
typedef enum
{
    BK_A2DP_MEDIA_CTRL_ACK_SUCCESS = 0,        /*!< media control command is acknowledged with success */
    BK_A2DP_MEDIA_CTRL_ACK_FAILURE,            /*!< media control command is acknowledged with failure */
    BK_A2DP_MEDIA_CTRL_ACK_BUSY,               /*!< media control command is rejected, as previous command is not yet acknowledged */
} bk_a2dp_media_ctrl_ack_t;

/**
 * @brief A2DP media codec capabilities union
 */
typedef struct
{
    uint8_t type;                              /*!< A2DP media codec type */

    union
    {
        /**
         * @brief  SBC codec capabilities
         */
        struct
        {
            uint8_t channels;
            uint8_t channel_mode;
            uint8_t block_len;
            uint8_t subbands;
            uint32_t sample_rate;
            uint8_t bit_pool;
            uint8_t alloc_mode;
        } sbc_codec;

        /**
         * @brief  MPEG-2, 4 AAC codec capabilities
         */
        struct
        {
            uint8_t channels;
            uint32_t sample_rate;
        } aac_codec;
    } cie;                                     /*!< A2DP codec information element */
} __attribute__((packed)) bk_a2dp_mcc_t;

/**
 * @brief Bluetooth A2DP set delay report value states
 */
typedef enum {
    BK_A2DP_SET_SUCCESS = 0,                /*!< A2DP profile set delay report value successful */
    BK_A2DP_SET_FAIL                        /*!< A2DP profile set delay report value failed */
} bk_a2dp_set_delay_value_state_t;

/**
 * @brief A2DP state callback parameters
 */
typedef union
{
    /**
     * @brief  BK_A2DP_CONNECTION_STATE_EVT
     */
    struct a2dp_conn_state_param
    {
        bk_a2dp_connection_state_t state;      /*!< one of values from bk_a2dp_connection_state_t */
        uint8_t remote_bda[6];                 /*!< remote bluetooth device address */
        bk_a2dp_disc_rsn_t disc_rsn;           /*!< reason of disconnection for "DISCONNECTED" */
    } conn_state;                              /*!< A2DP connection status */

    /**
     * @brief BK_A2DP_AUDIO_STATE_EVT
     */
    struct a2dp_audio_state_param
    {
        bk_a2dp_audio_state_t state;           /*!< one of the values from bk_a2dp_audio_state_t */
        uint8_t remote_bda[6];                 /*!< remote bluetooth device address */
    } audio_state;                             /*!< audio stream playing state */

    /**
     * @brief BK_A2DP_AUDIO_CFG_EVT
     */
    struct a2dp_audio_cfg_param
    {
        uint8_t remote_bda[6];                 /*!< remote bluetooth device address */
        bk_a2dp_mcc_t mcc;                     /*!< A2DP media codec capability information */
    } audio_cfg;                               /*!< media codec configuration information */

    /**
     * @brief BK_A2DP_AUDIO_SOURCE_CFG_EVT
     */
    struct a2dp_audio_source_cfg_param
    {
        uint8_t remote_bda[6];                 /*!< remote bluetooth device address */
        uint16_t mtu;                          /** max payload len, only used in source */
        bk_a2dp_mcc_t mcc;                     /*!< A2DP media codec capability information */
    } audio_source_cfg;                               /*!< media codec configuration information */

    /**
     * @brief BK_A2DP_MEDIA_CTRL_ACK_EVT
     */
    struct media_ctrl_stat_param
    {
        bk_a2dp_media_ctrl_t cmd;              /*!< media control commands to acknowledge */
        bk_a2dp_media_ctrl_ack_t status;       /*!< acknowledgement to media control commands */
    } media_ctrl_stat;                         /*!< status in acknowledgement to media control commands */

    /**
     * @brief BK_A2DP_PROF_STATE_EVT
     */
    struct a2dp_prof_stat_param
    {
        uint8_t action;                         /// 0 means init, 1 means deinit
        uint8_t role;                           ///0 means source, 1 means sink
        uint8_t status;                         /// 0 means success, other means fail
        uint8_t reason;                         /// not used now
    } a2dp_prof_stat;                           /*!< status to indicate a2dp prof init or deinit */

    /**
     * @brief BK_A2DP_SNK_SET_DELAY_VALUE_EVT
     */
    struct a2dp_set_stat_param {
        bk_a2dp_set_delay_value_state_t set_state;       /*!< a2dp profile state param */
        uint16_t delay_value;                            /*!< delay report value */
    } a2dp_set_delay_value_stat;                          /*!< A2DP sink set delay report value status */

    /**
     * @brief BK_A2DP_SNK_GET_DELAY_VALUE_EVT
     */
    struct a2dp_get_stat_param {
        uint16_t delay_value;                  /*!< delay report value */
    } a2dp_get_delay_value_stat;                /*!< A2DP sink get delay report value status */

} bk_a2dp_cb_param_t;


/**
 * @brief           A2DP profile callback function type
 *
 * @param           event : Event type
 *
 * @param           param : Pointer to callback parameter
 */
typedef void (* bk_bt_a2dp_cb_t)(bk_a2dp_cb_event_t event, bk_a2dp_cb_param_t *param);

/**
 * @brief           A2DP sink data callback function
 *
 * @param[in]       buf : pointer to the data received from A2DP source device
 *
 * @param[in]       len : size(in bytes) in buf
 */
typedef void (* bk_bt_sink_data_cb_t)(const uint8_t *buf, uint16_t len);


/**
 * @brief           A2DP source data read callback function
 *
 * @param[in]       buf : buffer to be filled with PCM data stream from higher layer
 *
 * @param[in]       len : size(in bytes) of data block to be copied to buf. -1 is an indication to user
 *                  that data buffer shall be flushed
 *
 * @return          size of bytes read successfully, if the argument len is -1, this value is ignored.
 *
 */
typedef int32_t (* bk_a2dp_source_data_cb_t)(uint8_t *buf, int32_t len);

/**
 * @brief           pcm resample callback function
 *
 * @param[in]       in_addr : input buff before resample (in bytes).
 * @param[in|out]   in_len : input buff len (in bytes). When call done, it should change to how many bytes had processed.
 * @param[in]       out_addr : input buff after resample (in bytes).
 * @param[in|out]   out_len : out buff len (in bytes). When call done, it should change to how many bytes had outputed.
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail, bluetooth will ignore the data.
 *
 */
typedef int32_t (* bk_a2dp_source_pcm_resample_cb_t)(uint8_t *in_addr, uint32_t *in_len, uint8_t *out_addr, uint32_t *out_len);

/**
 * @brief           pcm encode callback function
 *
 * @param[in]       type : encode type, 0 means sbc.
 * @param[in]       in_addr : input buff before encode (in bytes).
 * @param[in|out]   in_len : input buff len (in bytes). When call done, it should change to how many bytes had processed.
 * @param[in]       out_addr : input buff after encode (in bytes).
 * @param[in|out]   out_len : out buff len (in bytes). When call done, it should change to how many bytes had outputed.
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  - others: fail, bluetooth will ignore the data.
 *
 */
typedef int32_t (* bk_a2dp_source_pcm_encode_cb_t)(uint8_t type, uint8_t *in_addr, uint32_t *in_len, uint8_t *out_addr, uint32_t *out_len);

#ifdef __cplusplus
}
#endif

