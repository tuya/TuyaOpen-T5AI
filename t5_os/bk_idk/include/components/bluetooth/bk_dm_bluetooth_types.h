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

#include <stdint.h>
#include <stdbool.h>
#include "common/bk_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BK_BT_ADDR_LEN       (6)


/**
 * @brief Bluetooth status type, to indicate whether the bluetooth is ready.
 */
typedef enum {
    BK_BLUETOOTH_STATUS_UNINITIALIZED   = 0,        /*!< Bluetooth not initialized */
    BK_BLUETOOTH_STATUS_ENABLED                     /*!< Bluetooth initialized and enabled */
} bk_bluetooth_status_t;


typedef enum
{
    BK_BT_CONTROLLER_STACK_TYPE_BTDM_5_2,
    BK_BT_CONTROLLER_STACK_NOT_SUPPORT,
}BK_BT_CONTROLLER_STACK_TYPE;

typedef enum
{
    BK_BT_HOST_STACK_TYPE_ETHERMIND,
    BK_BT_HOST_STACK_TYPE_NOT_SUPPORT,
}BK_BT_HOST_STACK_TYPE;

//typedef bk_err_t ble_err_t;
typedef int ble_err_t;

#ifndef bt_err_t
typedef int bt_err_t;
#endif

#define BK_ERR_BT_SUCCESS               BK_OK                      /**< success */
#define BK_ERR_BT_FAIL                  BK_FAIL                    /**< fail        */
#define BK_ERR_BT_NO_MEM                BK_ERR_NO_MEM              /**<  no mem       */
#define BK_ERR_BT_PROFILE               (BK_ERR_BLE_BASE - 40)      /**<  profile err       */
#define BK_ERR_BT_CMD_NOT_SUPPORT       (BK_ERR_BLE_BASE - 41)      /**< unknow cmd err        */
#define BK_ERR_BT_UNKNOW_IDX            (BK_ERR_BLE_BASE - 42)      /**< index err, suchas conn_ind        */
#define BK_ERR_BT_BT_STATUS             (BK_ERR_BLE_BASE - 43)      /**<  ble status not match       */
#define BK_ERR_BT_CMD_RUN               (BK_ERR_BLE_BASE - 44)      /**< cmd run err        */
#define BK_ERR_BT_INIT_CREATE           (BK_ERR_BLE_BASE - 45)      /**< create init err, such as bk_ble_create_init        */
#define BK_ERR_BT_INIT_STATE            (BK_ERR_BLE_BASE - 46)      /**<  current init status not match       */

/* Definitions for bt error constants. */
#define BT_OK                           BK_OK                           /**< success */
#define BT_FAIL                         BK_FAIL                         /**< failure */
#define BT_INTERFACE_NOT_SUPPORT        BK_ERR_BLE_BASE - 100           /**< interface not support */
/// Status Return Value
typedef enum {
    BK_BT_STATUS_SUCCESS                         = 0,
    BK_BT_STATUS_UNKNOWN_HCI_COMMAND             = 0x01,
    BK_BT_STATUS_UNKNOWN_CONNECTION_ID           = 0x02,
    BK_BT_STATUS_HARDWARE_FAILURE                = 0x03,
    BK_BT_STATUS_PAGE_TIMEOUT                    = 0x04,
    BK_BT_STATUS_AUTH_FAILURE                    = 0x05,
    BK_BT_STATUS_PIN_MISSING                     = 0x06,
    BK_BT_STATUS_MEMORY_CAPA_EXCEED              = 0x07,
    BK_BT_STATUS_CON_TIMEOUT                     = 0x08,
    BK_BT_STATUS_CON_LIMIT_EXCEED                = 0x09,
    BK_BT_STATUS_SYNC_CON_LIMIT_DEV_EXCEED       = 0x0A,
    BK_BT_STATUS_CON_ALREADY_EXISTS              = 0x0B,
    BK_BT_STATUS_COMMAND_DISALLOWED              = 0x0C,
    BK_BT_STATUS_CONN_REJ_LIMITED_RESOURCES      = 0x0D,
    BK_BT_STATUS_CONN_REJ_SECURITY_REASONS       = 0x0E,
    BK_BT_STATUS_CONN_REJ_UNACCEPTABLE_BDADDR    = 0x0F,
    BK_BT_STATUS_CONN_ACCEPT_TIMEOUT_EXCEED      = 0x10,
    BK_BT_STATUS_UNSUPPORTED                     = 0x11,
    BK_BT_STATUS_INVALID_HCI_PARAM               = 0x12,
    BK_BT_STATUS_REMOTE_USER_TERM_CON            = 0x13,
    BK_BT_STATUS_REMOTE_DEV_TERM_LOW_RESOURCES   = 0x14,
    BK_BT_STATUS_REMOTE_DEV_POWER_OFF            = 0x15,
    BK_BT_STATUS_CON_TERM_BY_LOCAL_HOST          = 0x16,
    BK_BT_STATUS_REPEATED_ATTEMPTS               = 0x17,
    BK_BT_STATUS_PAIRING_NOT_ALLOWED             = 0x18,
    BK_BT_STATUS_UNKNOWN_LMP_PDU                 = 0x19,
    BK_BT_STATUS_UNSUPPORTED_REMOTE_FEATURE      = 0x1A,
    BK_BT_STATUS_SCO_OFFSET_REJECTED             = 0x1B,
    BK_BT_STATUS_SCO_INTERVAL_REJECTED           = 0x1C,
    BK_BT_STATUS_SCO_AIR_MODE_REJECTED           = 0x1D,
    BK_BT_STATUS_INVALID_LMP_PARAM               = 0x1E,
    BK_BT_STATUS_UNSPECIFIED_ERROR               = 0x1F,
    BK_BT_STATUS_UNSUPPORTED_LMP_PARAM_VALUE     = 0x20,
    BK_BT_STATUS_ROLE_CHANGE_NOT_ALLOWED         = 0x21,
    BK_BT_STATUS_LMP_RSP_TIMEOUT                 = 0x22,
    BK_BT_STATUS_LMP_COLLISION                   = 0x23,
    BK_BT_STATUS_LMP_PDU_NOT_ALLOWED             = 0x24,
    BK_BT_STATUS_ENC_MODE_NOT_ACCEPT             = 0x25,
    BK_BT_STATUS_LINK_KEY_CANT_CHANGE            = 0x26,
    BK_BT_STATUS_QOS_NOT_SUPPORTED               = 0x27,
    BK_BT_STATUS_INSTANT_PASSED                  = 0x28,
    BK_BT_STATUS_PAIRING_WITH_UNIT_KEY_NOT_SUP   = 0x29,
    BK_BT_STATUS_DIFF_TRANSACTION_COLLISION      = 0x2A,
    BK_BT_STATUS_QOS_UNACCEPTABLE_PARAM          = 0x2C,
    BK_BT_STATUS_QOS_REJECTED                    = 0x2D,
    BK_BT_STATUS_CHANNEL_CLASS_NOT_SUP           = 0x2E,
    BK_BT_STATUS_INSUFFICIENT_SECURITY           = 0x2F,
    BK_BT_STATUS_PARAM_OUT_OF_MAND_RANGE         = 0x30,
    BK_BT_STATUS_ROLE_SWITCH_PEND                = 0x32, /* LM_ROLE_SWITCH_PENDING               */
    BK_BT_STATUS_RESERVED_SLOT_VIOLATION         = 0x34, /* LM_RESERVED_SLOT_VIOLATION           */
    BK_BT_STATUS_ROLE_SWITCH_FAIL                = 0x35, /* LM_ROLE_SWITCH_FAILED                */
    BK_BT_STATUS_EIR_TOO_LARGE                   = 0x36, /* LM_EXTENDED_INQUIRY_RESPONSE_TOO_LARGE */
    BK_BT_STATUS_SP_NOT_SUPPORTED_HOST           = 0x37,
    BK_BT_STATUS_HOST_BUSY_PAIRING               = 0x38,
    BK_BT_STATUS_CONTROLLER_BUSY                 = 0x3A,
    BK_BT_STATUS_UNACCEPTABLE_CONN_PARAM         = 0x3B,
    BK_BT_STATUS_ADV_TO                          = 0x3C,
    BK_BT_STATUS_TERMINATED_MIC_FAILURE          = 0x3D,
    BK_BT_STATUS_CONN_FAILED_TO_BE_EST           = 0x3E,
    BK_BT_STATUS_CCA_REJ_USE_CLOCK_DRAG          = 0x40,
    BK_BT_STATUS_TYPE0_SUBMAP_NOT_DEFINED        = 0x41,
    BK_BT_STATUS_UNKNOWN_ADVERTISING_ID          = 0x42,
    BK_BT_STATUS_LIMIT_REACHED                   = 0x43,
    BK_BT_STATUS_OPERATION_CANCELED_BY_HOST      = 0x44,
    BK_BT_STATUS_PKT_TOO_LONG                    = 0x45,

    BK_BT_STATUS_UNDEFINED                       = 0xFF,
} bk_bt_status_t;

/*
 * @brief for bt api async call result in bt_at_cmd_cb's cmd
 */
typedef enum
{
    BT_CMD_INQUIRY,
    BT_CMD_CREATE_CONNECT,
    BT_CMD_DISCONNECT,
    BT_CMD_SDP,
    BT_CMD_READ_SCAN_ENABLE,
    BT_CMD_WRITE_SCAN_ENABLE,
    BT_CMD_MAX,
} bt_cmd_t;

/**
 * @defgroup bk_bt_api_v1_typedef struct
 * @brief bt struct type
 * @ingroup bk_bt_api_v1_typedef struct
 * @{
 */
typedef struct
{
    uint8_t cmd_idx;      /**< actv_idx */
    bt_err_t status;     /**< The status for this command */
    void *param;
} bt_cmd_param_t;

/*Define the bt octet 16 bit size*/
#define BK_BT_OCTET16_LEN    16
typedef uint8_t bk_bt_octet16_t[BK_BT_OCTET16_LEN];   /* octet array: size 16 */

#define BK_BT_OCTET8_LEN    8
typedef uint8_t bk_bt_octet8_t[BK_BT_OCTET8_LEN];   /* octet array: size 8 */

typedef uint8_t bk_link_key_t[BK_BT_OCTET16_LEN];      /* Link Key */

/// UUID type

#if 1

typedef struct {
#define BK_UUID_LEN_16     2
#define BK_UUID_LEN_32     4
#define BK_UUID_LEN_128    16
    uint16_t len;							/*!< UUID length, 16bit, 32bit or 128bit */
    union {
        uint16_t    uuid16;                 /*!< 16bit UUID */
        uint32_t    uuid32;                 /*!< 32bit UUID */
        uint8_t     uuid128[BK_UUID_LEN_128]; /*!< 128bit UUID */
    } uuid;									/*!< UUID */
} __attribute__((packed)) bk_bt_uuid_t;

/// Bluetooth device type
typedef enum {
    BK_BT_DEVICE_TYPE_BREDR   = 0x01,
    BK_BT_DEVICE_TYPE_BLE     = 0x02,
    BK_BT_DEVICE_TYPE_DUMO    = 0x03,
} bk_bt_dev_type_t;
#else

typedef struct {
#define BK_UUID_LEN_16     2
#define BK_UUID_LEN_32     4
#define BK_UUID_LEN_128    16
    uint16_t len;
    union {
        struct {
           uint32_t :32;
           uint32_t :32;
           uint32_t :32;
           uint16_t uuid16;
        };

        struct {
           uint32_t :32;
           uint32_t :32;
           uint32_t :32;
           uint32_t uuid32;
        };

        uint8_t     uuid128[BK_UUID_LEN_128];
    } uuid;
} __attribute__((packed)) bk_bt_uuid_t;

#endif



/// Bluetooth address length
#define BK_BD_ADDR_LEN     6

/// Bluetooth device address
typedef uint8_t bk_bd_addr_t[BK_BD_ADDR_LEN];

/// BLE device address type
typedef enum {
    BLE_ADDR_TYPE_PUBLIC        = 0x00,
    BLE_ADDR_TYPE_RANDOM        = 0x01,

    /// when used with own addr, below means rpa effort, otherwise public
    /// when used with peer addr, below doesn't used
    BLE_ADDR_TYPE_RPA_PUBLIC    = 0x02,

    /// when used with own addr, below means rpa effort, otherwise random
    /// when used with peer addr, below doesn't used
    BLE_ADDR_TYPE_RPA_RANDOM    = 0x03,
} bk_ble_addr_type_t;

/// white list address type
typedef enum {
    BLE_WL_ADDR_TYPE_PUBLIC        = 0x00,
    BLE_WL_ADDR_TYPE_RANDOM        = 0x01,
} bk_ble_wl_addr_type_t;



/**
 * @brief for sync bt api call return
 *
 * most bt api have bt_cmd_cb_t param, you must wait is callback.
 *
 * @param
 * - cmd: cmd id.
 * - param: param
 *
**/
typedef void (*bt_cmd_cb_t)(bt_cmd_t cmd, bt_cmd_param_t *param);

#ifdef __cplusplus
}
#endif

