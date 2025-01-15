#pragma once

#include "bk_dm_bluetooth_types.h"
#include "bk_dm_gatt_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup dm_gattc_v1 GATT CLIENT API
 * @{
 */


/**
 * @defgroup dm_gattc_types_v1 Defines
 * @{
 */

/// GATT Client callback function events
typedef enum
{
    BK_GATTC_REG_EVT                 = 0,        /*!< When GATT client is registered, the event comes */
    BK_GATTC_UNREG_EVT               = 1,        /*!< When GATT client is unregistered, the event comes */
    BK_GATTC_READ_CHAR_EVT           = 3,        /*!< When GATT characteristic is read, the event comes */
    BK_GATTC_WRITE_CHAR_EVT          = 4,        /*!< When GATT characteristic write operation completes, the event comes */
    BK_GATTC_READ_DESCR_EVT          = 8,        /*!< When GATT characteristic descriptor read completes, the event comes */
    BK_GATTC_WRITE_DESCR_EVT         = 9,        /*!< When GATT characteristic descriptor write completes, the event comes */
    BK_GATTC_NOTIFY_EVT              = 10,       /*!< When GATT notification or indication arrives, the event comes */
    BK_GATTC_PREP_WRITE_EVT          = 11,       /*!< When GATT prepare-write operation completes, the event comes */
    BK_GATTC_EXEC_EVT                = 12,       /*!< When write execution completes, the event comes */
    BK_GATTC_CFG_MTU_EVT             = 18,       /*!< When configuration of MTU completes, the event comes */
    BK_GATTC_CONNECT_EVT             = 40,       /*!< When the ble physical connection is set up, the event comes */
    BK_GATTC_DISCONNECT_EVT          = 41,       /*!< When the ble physical connection disconnected, the event comes */
    BK_GATTC_READ_MULTIPLE_EVT       = 42,       /*!< When the ble characteristic or descriptor multiple complete, the event comes */
    BK_GATTC_DIS_SRVC_CMPL_EVT       = 46,       /*!< When the ble discover service complete, the event comes */

    BK_GATTC_DIS_RES_SERVICE_EVT,
    BK_GATTC_DIS_RES_CHAR_EVT,
    BK_GATTC_DIS_RES_CHAR_DESC_EVT,
    BK_GATTC_READ_BY_TYPE_EVT,
} bk_gattc_cb_event_t;


/**
 * @brief Gatt client callback parameters union
 */
typedef union
{
    /**
     * @brief BK_GATTC_REG_EVT
     */
    struct gattc_reg_evt_param
    {
        bk_gatt_status_t status;       /*!< Operation status */
        uint16_t app_id;                /*!< Application id which input in register API */
        bk_gatt_if_t gatt_if;
    } reg;                              /*!< Gatt client callback param of BK_GATTC_REG_EVT */

    /**
     * @brief BK_GATTC_CFG_MTU_EVT
     */
    struct gattc_cfg_mtu_evt_param
    {
        bk_gatt_status_t status;       /*!< Operation status */
        uint16_t conn_id;               /*!< Connection id */
        uint16_t mtu;                   /*!< MTU size */
    } cfg_mtu;                          /*!< Gatt client callback param of BK_GATTC_CFG_MTU_EVT */

    /**
     * @brief BK_GATTC_READ_CHAR_EVT,  BK_GATTC_READ_DESCR_EVT, BK_GATTC_READ_MULTIPLE_EVT
     */
    struct gattc_read_char_evt_param
    {
        bk_gatt_status_t status;       /*!< Operation status */
        uint16_t conn_id;               /*!< Connection id */
        uint16_t handle;                /*!< Characteristic handle, Not used when BK_GATTC_READ_MULTIPLE_EVT */
        uint8_t *value;                 /*!< Characteristic value */
        uint16_t value_len;             /*!< Characteristic value length */
    } read;                             /*!< Gatt client callback param of BK_GATTC_READ_CHAR_EVT */

    /**
     * @brief BK_GATTC_WRITE_CHAR_EVT, BK_GATTC_PREP_WRITE_EVT, BK_GATTC_WRITE_DESCR_EVT
     */
    struct gattc_write_evt_param
    {
        bk_gatt_status_t status;       /*!< Operation status */
        uint16_t conn_id;               /*!< Connection id */
        uint16_t handle;                /*!< The Characteristic or descriptor handle */
        uint16_t offset;                /*!< The prepare write offset, this value is valid only when prepare write */
    } write;                            /*!< Gatt client callback param of BK_GATTC_WRITE_DESCR_EVT */

    /**
     * @brief BK_GATTC_EXEC_EVT
     */
    struct gattc_exec_cmpl_evt_param
    {
        bk_gatt_status_t status;       /*!< Operation status */
        uint16_t conn_id;               /*!< Connection id */
    } exec_cmpl;                        /*!< Gatt client callback param of BK_GATTC_EXEC_EVT */

    /**
     * @brief BK_GATTC_NOTIFY_EVT
     */
    struct gattc_notify_evt_param
    {
        uint16_t conn_id;               /*!< Connection id */
        bk_bd_addr_t remote_bda;       /*!< Remote bluetooth device address */
        uint16_t handle;                /*!< The Characteristic or descriptor handle */
        uint16_t value_len;             /*!< Notify attribute value */
        uint8_t *value;                 /*!< Notify attribute value */
        bool is_notify;                 /*!< True means notify, false means indicate */
    } notify;                           /*!< Gatt client callback param of BK_GATTC_NOTIFY_EVT */

    /**
     * @brief BK_GATTC_SRVC_CHG_EVT
     */
    struct gattc_srvc_chg_evt_param
    {
        bk_bd_addr_t remote_bda;       /*!< Remote bluetooth device address */
    } srvc_chg;                         /*!< Gatt client callback param of BK_GATTC_SRVC_CHG_EVT */

    /**
     * @brief BK_GATTC_CONNECT_EVT
     */
    struct gattc_connect_evt_param
    {
        uint16_t conn_id;               /*!< Connection id */
        uint8_t link_role;              /*!< Link role : master role = 0  ; slave role = 1*/
        bk_bd_addr_t remote_bda;       /*!< Remote bluetooth device address */
        bk_gatt_conn_params_t conn_params; /*!< current connection parameters */
    } connect;                          /*!< Gatt client callback param of BK_GATTC_CONNECT_EVT */

    /**
     * @brief BK_GATTC_DISCONNECT_EVT
     */
    struct gattc_disconnect_evt_param
    {
        bk_gatt_conn_reason_t reason;  /*!< The reason of gatt connection close */
        uint16_t conn_id;               /*!< Connection id */
        bk_bd_addr_t remote_bda;       /*!< Remote bluetooth device address */
    } disconnect;                       /*!< Gatt client callback param of BK_GATTC_DISCONNECT_EVT */

    /**
     * @brief BK_GATTC_DIS_SRVC_CMPL_EVT
     */
    struct gattc_dis_srvc_cmpl_evt_param
    {
        bk_gatt_status_t status;      /*!< Operation status */
        uint16_t conn_id;              /*!< Connection id */
    } dis_srvc_cmpl;                   /*!< Gatt client callback param of BK_GATTC_DIS_SRVC_CMPL_EVT */


    /**
     * @brief BK_GATTC_DIS_RES_SERVICE_EVT
     */
    struct gattc_dis_res_service_evt_param
    {
        uint16_t conn_id;               /*!< Connection id */

        struct
        {
            uint16_t start_handle;          /*!< Service start handle */
            uint16_t end_handle;            /*!< Service end handle */
            bk_gatt_id_t srvc_id;          /*!< Service id, include service uuid and other information */
            bool      is_primary;           /*!< True if this is the primary service */
        }*array;
        uint32_t count;

    } dis_res_service;                       /*!< Gatt client callback param of BK_GATTC_DIS_RES_SERVICE_EVT */


    /**
     * @brief BK_GATTC_DIS_RES_CHAR_EVT
     */
    struct gattc_dis_res_char_evt_param
    {
        uint16_t conn_id;               /*!< Connection id */

        struct
        {
            uint16_t start_handle;          /*!< char start handle */
            uint16_t end_handle;            /*!< char end handle */
            uint16_t char_value_handle;          /*!< char declar value handle */
            bk_gatt_id_t uuid;          /*!< uuid */
        }*array;
        uint32_t count;
    } dis_res_char;                       /*!< Gatt client callback param of BK_GATTC_DIS_RES_CHAR_EVT */

    /**
     * @brief BK_GATTC_DIS_RES_CHAR_DESC_EVT
     */
    struct gattc_dis_res_char_desc_evt_param
    {
        uint16_t conn_id;               /*!< Connection id */

        struct
        {
            uint16_t char_handle;          /*!< which char handle own this desc */
            uint16_t desc_handle;          /*!< desc handle */
            bk_gatt_id_t uuid;
        }*array;

        uint32_t count;
    } dis_res_char_desc;                       /*!< Gatt client callback param of BK_GATTC_DIS_RES_CHAR_DESC_EVT */



    /**
     * @brief BK_GATTC_READ_BY_TYPE_EVT
     */
    struct gattc_read_by_type_evt_param
    {
        bk_gatt_status_t status;       /*!< Operation status */
        uint16_t conn_id;               /*!< Connection id */
        uint16_t elem_count;             /// elem_count

        struct
        {
            uint16_t handle;     /// attr handle
            uint8_t *value;      /// attr value
            uint16_t len;        /// attr value len
        } *elem;

    } read_by_type;

} bk_ble_gattc_cb_param_t;             /*!< GATT client callback parameter union type */

/**
 * @brief GATT Client callback function type
 * @param event : Event type
 * @param gattc_if : GATT client access interface, normally
 *                   different gattc_if correspond to different profile
 * @param param : Point to callback parameter, currently is union type
 */
typedef int32_t (* bk_gattc_cb_t)(bk_gattc_cb_event_t event, bk_gatt_if_t gattc_if, bk_ble_gattc_cb_param_t *param);


///@}




/**
 * @defgroup dm_gattc_functions_v1 Functions
 * @{
 */




/**
 * @brief           This function is called to register application callbacks
 *                  with GATTC module.
 *
 * @param[in]       callback : pointer to the application callback function.
 *
 * @return
 *                  - BK_ERR_BLE_SUCCESS: success
 *                  - other: failed
 *
 */
ble_err_t bk_ble_gattc_register_callback(bk_gattc_cb_t callback);


/**
 * @brief           This function is called to register application callbacks
 *                  with GATTC module.
 *
 * @param[in]       app_id : Application Identify (UUID), for different application
 *
 * @return
 *                  - BK_ERR_BLE_SUCCESS: success
 *                  - other: failed
 *
 */
ble_err_t bk_ble_gattc_app_register(uint16_t app_id);


/**
 * @brief           This function is called to unregister an application
 *                  from GATTC module.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 *
 * @return
 *                  - BK_ERR_BLE_SUCCESS: success
 *                  - other: failed
 *
 */
ble_err_t bk_ble_gattc_app_unregister(bk_gatt_if_t gattc_if);


/**
 * @brief           Configure the MTU size in the GATT channel. This can be done
 *                  only once per connection. Before using, use bk_ble_gatt_set_local_mtu()
 *                  to configure the local MTU size.
 *
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id: connection ID.
 *
 * @return
 *                  - BK_ERR_BLE_SUCCESS: success
 *                  - other: failed
 *
 */
ble_err_t bk_ble_gattc_send_mtu_req (bk_gatt_if_t gattc_if, uint16_t conn_id);

/**
 * @brief           This function is called to read a service's characteristics of
 *                  the given characteristic handle
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID.
 * @param[in]       handle : characteritic handle to read.
 * @param[in]       auth_req : authenticate request type
 *
 * @return
 *                  - BK_ERR_BLE_SUCCESS: success
 *                  - other: failed
 *
 */
ble_err_t bk_ble_gattc_read_char (bk_gatt_if_t gattc_if,
                                   uint16_t conn_id,
                                   uint16_t handle,
                                   bk_gatt_auth_req_t auth_req);

/**
 * @brief           This function is called to read a service's characteristics of
 *                  the given characteristic UUID
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID.
 * @param[in]       start_handle : the attribute start handle.
 * @param[in]       end_handle : the attribute end handle
 * @param[in]       uuid : The UUID of attribute which will be read.
 * @param[in]       auth_req : authenticate request type
 *
 * @return
 *                  - BK_ERR_BLE_SUCCESS: success
 *                  - other: failed
 *
 */
ble_err_t bk_ble_gattc_read_by_type (bk_gatt_if_t gattc_if,
                                      uint16_t conn_id,
                                      uint16_t start_handle,
                                      uint16_t end_handle,
                                      bk_bt_uuid_t *uuid,
                                      bk_gatt_auth_req_t auth_req);

/**
 * @brief           This function is called to read multiple characteristic or
 *                  characteristic descriptors.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID.
 * @param[in]       read_multi : pointer to the read multiple parameter.
 * @param[in]       auth_req : authenticate request type
 *
 * @return
 *                  - BK_ERR_BLE_SUCCESS: success
 *                  - other: failed
 *
 */
ble_err_t bk_ble_gattc_read_multiple(bk_gatt_if_t gattc_if,
                                      uint16_t conn_id, bk_gattc_multi_t *read_multi,
                                      bk_gatt_auth_req_t auth_req);


/**
 * @brief           This function is called to read a characteristics descriptor.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID.
 * @param[in]       handle : descriptor handle to read.
 * @param[in]       auth_req : authenticate request type
 *
 * @return
 *                  - BK_ERR_BLE_SUCCESS: success
 *                  - other: failed
 *
 */
ble_err_t bk_ble_gattc_read_char_descr (bk_gatt_if_t gattc_if,
        uint16_t conn_id,
        uint16_t handle,
        bk_gatt_auth_req_t auth_req);


/**
 * @brief           This function is called to write characteristic value.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID.
 * @param[in]       handle : characteristic handle to write.
 * @param[in]       value_len: length of the value to be written.
 * @param[in]       value : the value to be written.
 * @param[in]       write_type : the type of attribute write operation.
 * @param[in]       auth_req : authentication request.
 *
 * @return
 *                  - BK_ERR_BLE_SUCCESS: success
 *                  - other: failed
 *
 */
ble_err_t bk_ble_gattc_write_char( bk_gatt_if_t gattc_if,
                                    uint16_t conn_id,
                                    uint16_t handle,
                                    uint16_t value_len,
                                    uint8_t *value,
                                    bk_gatt_write_type_t write_type,
                                    bk_gatt_auth_req_t auth_req);


/**
 * @brief           This function is called to write characteristic descriptor value.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID
 * @param[in]       handle : descriptor handle to write.
 * @param[in]       value_len: length of the value to be written.
 * @param[in]       value : the value to be written.
 * @param[in]       write_type : the type of attribute write operation.
 * @param[in]       auth_req : authentication request.
 *
 * @return
 *                  - BK_ERR_BLE_SUCCESS: success
 *                  - other: failed
 *
 */
ble_err_t bk_ble_gattc_write_char_descr (bk_gatt_if_t gattc_if,
        uint16_t conn_id,
        uint16_t handle,
        uint16_t value_len,
        uint8_t *value,
        bk_gatt_write_type_t write_type,
        bk_gatt_auth_req_t auth_req);


/**
 * @brief           This function is called to prepare write a characteristic value.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID.
 * @param[in]       handle : characteristic handle to prepare write.
 * @param[in]       offset : offset of the write value.
 * @param[in]       value_len: length of the value to be written.
 * @param[in]       value : the value to be written.
 * @param[in]       auth_req : authentication request.
 *
 * @return
 *                  - BK_ERR_BLE_SUCCESS: success
 *                  - other: failed
 *
 */
ble_err_t bk_ble_gattc_prepare_write(bk_gatt_if_t gattc_if,
                                      uint16_t conn_id,
                                      uint16_t handle,
                                      uint16_t offset,
                                      uint16_t value_len,
                                      uint8_t *value,
                                      bk_gatt_auth_req_t auth_req);


/**
 * @brief           This function is called to prepare write a characteristic descriptor value.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID.
 * @param[in]       handle : characteristic descriptor handle to prepare write.
 * @param[in]       offset : offset of the write value.
 * @param[in]       value_len: length of the value to be written.
 * @param[in]       value : the value to be written.
 * @param[in]       auth_req : authentication request.
 *
 * @return
 *                  - BK_ERR_BLE_SUCCESS: success
 *                  - other: failed
 *
 */
ble_err_t bk_ble_gattc_prepare_write_char_descr(bk_gatt_if_t gattc_if,
        uint16_t conn_id,
        uint16_t handle,
        uint16_t offset,
        uint16_t value_len,
        uint8_t *value,
        bk_gatt_auth_req_t auth_req);


/**
 * @brief           This function is called to execute write a prepare write sequence.
 *
 * @param[in]       gattc_if: Gatt client access interface.
 * @param[in]       conn_id : connection ID.
 * @param[in]       is_execute : execute or cancel.
 *
 * @return
 *                  - BK_ERR_BLE_SUCCESS: success
 *                  - other: failed
 *
 */
ble_err_t bk_ble_gattc_execute_write (bk_gatt_if_t gattc_if, uint16_t conn_id, bool is_execute);


/**
* @brief           Triggle a discovery of the remote device.
*
* @param[in]       gattc_if: Gatt client access interface.
* @param[in]       handle : GATT characteristic handle.
* @param[in]       auth_req : authentication request.
* @return
*                  - BK_ERR_BLE_SUCCESS: success
*                  - other: failed
*
*/
ble_err_t bk_ble_gattc_discover(bk_gatt_if_t gattc_if, uint16_t conn_id, bk_gatt_auth_req_t auth_req);

///@}

///@}
#ifdef __cplusplus
}
#endif

