#include <common/sys_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_gap_ble_types.h"
#include "components/bluetooth/bk_dm_gap_ble.h"
#include "components/bluetooth/bk_dm_gatt_types.h"
#include "components/bluetooth/bk_dm_gatts.h"
#include "components/bluetooth/bk_dm_gattc.h"

#include "at_ble_common.h"
#include "ble_at_gatt.h"

#define INVALID_ATTR_HANDLE 0



static ble_at_gatt_ctx_handle_t s_dm_gatt_api;

static uint16_t s_service_attr_handle = INVALID_ATTR_HANDLE;

static uint16_t s_char_attr_handle = INVALID_ATTR_HANDLE;
static uint8_t s_char_buff[4] = {0};

static uint16_t s_char_desc_attr_handle = INVALID_ATTR_HANDLE;
static uint16_t s_char_desc_buff = 0;

static uint16_t s_char2_attr_handle = INVALID_ATTR_HANDLE;
//static uint16_t s_char2_buff = {0};

static uint16_t s_char2_desc_attr_handle = INVALID_ATTR_HANDLE;

static uint16_t s_char3_attr_handle = INVALID_ATTR_HANDLE;
//static uint16_t s_char3_buff = {0};

static uint16_t s_char3_desc_attr_handle = INVALID_ATTR_HANDLE;

static uint8_t s_service_type = DB_TYPE_TIMER_SEND;
static beken_timer_t s_service_timer_send_mode_timer_handle;
static uint16_t s_service_test_send_size = 20;
static uint32_t s_service_test_send_inter = 1000;



static uint32_t s_service_performance_tx_all_bytes = 0;
static uint32_t s_service_performance_rx_all_bytes = 0;

#define BK_GATT_ATTR_TYPE(iuuid) {.len = BK_UUID_LEN_16, .uuid = {.uuid16 = iuuid}}
#define BK_GATT_ATTR_CONTENT(iuuid) {.len = BK_UUID_LEN_16, .uuid = {.uuid16 = iuuid}}
#define BK_GATT_ATTR_VALUE(ilen, ivalue) {.attr_max_len = ilen, .attr_len = ilen, .attr_value = ivalue}


#define BK_GATT_PRIMARY_SERVICE_DECL(iuuid) \
    .att_desc =\
               {\
                .attr_type = BK_GATT_ATTR_TYPE(BK_GATT_UUID_PRI_SERVICE),\
                .attr_content = BK_GATT_ATTR_CONTENT(iuuid),\
               }

#define BK_GATT_CHAR_DECL(iuuid, ilen, ivalue, iprop, iperm, irsp) \
    .att_desc = \
                {\
                 .attr_type = BK_GATT_ATTR_TYPE(BK_GATT_UUID_CHAR_DECLARE),\
                 .attr_content = BK_GATT_ATTR_CONTENT(iuuid),\
                 .value = BK_GATT_ATTR_VALUE(ilen, ivalue),\
                 .prop = iprop,\
                 .perm = iperm,\
                },\
                .attr_control = {.auto_rsp = irsp}

#define BK_GATT_CHAR_DESC_DECL(iuuid, ilen, ivalue, iperm, irsp) \
    .att_desc = \
                {\
                 .attr_type = BK_GATT_ATTR_TYPE(iuuid),\
                 .value = BK_GATT_ATTR_VALUE(ilen, ivalue),\
                 .perm = iperm,\
                },\
                .attr_control = {.auto_rsp = irsp}

static const bk_gatts_attr_db_t s_gatts_attr_db_service_performance_test[] =
{
    //service
    {
        BK_GATT_PRIMARY_SERVICE_DECL(0x1234),
    },

    //char 1
    {
        BK_GATT_CHAR_DECL(0x5678,
                          sizeof(s_char_buff), s_char_buff,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR | BK_GATT_CHAR_PROP_BIT_WRITE | BK_GATT_CHAR_PROP_BIT_NOTIFY,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          //BK_GATT_PERM_READ_ENCRYPTED | BK_GATT_PERM_WRITE_ENCRYPTED,
                          //BK_GATT_PERM_READ_ENC_MITM | BK_GATT_PERM_WRITE_ENC_MITM, //gap iocap must not be BK_IO_CAP_NONE !!!
                          BK_GATT_AUTO_RSP),
    },
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_CLIENT_CONFIG,
                               sizeof(s_char_desc_buff), (uint8_t *)&s_char_desc_buff,
                               BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                               BK_GATT_AUTO_RSP),
    },

    //char 2
    {
        BK_GATT_CHAR_DECL(0x9abc,
                          sizeof(s_service_test_send_size), (void *)&s_service_test_send_size,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR | BK_GATT_CHAR_PROP_BIT_WRITE,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_AUTO_RSP),
    },
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_USER_DESCRIPTION,
                               sizeof("send size"), (uint8_t *)"send size",
                               BK_GATT_PERM_READ,
                               BK_GATT_AUTO_RSP),
    },

    //char 3
    {
        BK_GATT_CHAR_DECL(0xdef0,
                          sizeof(s_service_test_send_inter), (void *)&s_service_test_send_inter,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE_NR | BK_GATT_CHAR_PROP_BIT_WRITE,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_AUTO_RSP),
    },
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_USER_DESCRIPTION,
                               sizeof("send interval"), (uint8_t *)"send interval",
                               BK_GATT_PERM_READ,
                               BK_GATT_AUTO_RSP),
    },
};

static uint16_t *const s_attr_handle_list[sizeof(s_gatts_attr_db_service_performance_test) / sizeof(s_gatts_attr_db_service_performance_test[0])] =
{
    &s_service_attr_handle,
    &s_char_attr_handle,
    &s_char_desc_attr_handle,
    &s_char2_attr_handle,
    &s_char2_desc_attr_handle,
    &s_char3_attr_handle,
    &s_char3_desc_attr_handle,
};

static void ble_at_timer_send_mode_timer_callback(void *param)
{
    static uint8_t tmp_payload = 0;
    int32_t retval = 0;
    uint8_t *tmp_buff = NULL;

    tmp_buff = os_malloc(s_service_test_send_size);

    if (!tmp_buff)
    {
        bt_at_loge("alloc send failed");
        return;
    }

    os_memset(tmp_buff, 0, s_service_test_send_size);
    tmp_buff[0] = tmp_payload++;

    retval = bk_ble_gatts_send_indicate(s_dm_gatt_api.gatts_if, s_dm_gatt_api.conn_handle, s_char_attr_handle, s_service_test_send_size, tmp_buff, 1);

    os_free(tmp_buff);

    if (retval != 0)
    {
        bt_at_loge("notify err %d", retval);
    }
    else
    {
        s_service_performance_tx_all_bytes += s_service_test_send_size;
    }
}

static int32_t bk_gatts_at_cb (bk_gatts_cb_event_t event, bk_gatt_if_t gatts_if, bk_ble_gatts_cb_param_t *comm_param)
{
    switch (event)
    {
    case BK_GATTS_REG_EVT:
    {
        struct gatts_reg_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTS_REG_EVT %d %d", param->status, param->gatt_if);
        s_dm_gatt_api.gatts_if = param->gatt_if;

        if (*ble_at_get_sema_handle() != NULL)
        {
            rtos_set_semaphore( ble_at_get_sema_handle() );
        }
    }
    break;

    case BK_GATTS_CREAT_ATTR_TAB_EVT:
    {
        struct gatts_add_attr_tab_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTS_CREAT_ATTR_TAB_EVT %d %d", param->status, param->num_handle);

        for (int i = 0; i < param->num_handle; ++i)
        {
            *s_attr_handle_list[i] = param->handles[i];

            if (i) //service handle cant get buff
            {
                uint16_t tmp_len = 0;
                uint8_t *tmp_buff = NULL;
                ble_err_t g_status = 0;

                g_status = bk_ble_gatts_get_attr_value(param->handles[i], &tmp_len, &tmp_buff);

                if (g_status)
                {
                    bt_at_loge("get attr value err %d", g_status);
                }


                if (tmp_len != s_gatts_attr_db_service_performance_test[i].att_desc.value.attr_len ||
                        tmp_buff != s_gatts_attr_db_service_performance_test[i].att_desc.value.attr_value)
                {
                    bt_at_loge("get attr value not match create attr handle %d i %d %d %d %p %p!!!!",
                              param->handles[i], i,
                              tmp_len, s_gatts_attr_db_service_performance_test[i].att_desc.value.attr_len,
                              tmp_buff, s_gatts_attr_db_service_performance_test[i].att_desc.value.attr_value);
                }
//                else
//                {
//                    bt_at_logi("handle %d i %d len %d value %p", param->handles[i], i, tmp_len, tmp_buff);
//                }
//
//                if(i == 6)
//                {
//                    memcpy(tmp_buff, "asdasd", 6);
//                    bt_at_logi("write after handle %d i %d len %d value %s", param->handles[i], i, tmp_len, tmp_buff);
//                }
            }
        }

        if (*ble_at_get_sema_handle() != NULL)
        {
            rtos_set_semaphore( ble_at_get_sema_handle() );
        }
    }
    break;

    case BK_GATTS_WRITE_EVT:
    {
        struct gatts_write_evt_param *param = (typeof(param))comm_param;

        if (param->handle == s_char_desc_attr_handle)
        {
            uint16_t cli_config = 0;
            os_memcpy(&cli_config, param->value, sizeof(cli_config));

            if ((cli_config & 1) || (cli_config & 2))
            {
                if (s_service_type == DB_TYPE_TIMER_SEND)
                {
                    rtos_init_timer(&s_service_timer_send_mode_timer_handle, s_service_test_send_inter, ble_at_timer_send_mode_timer_callback, NULL);
                    rtos_start_timer(&s_service_timer_send_mode_timer_handle);
                }
                else if (s_service_type == DB_TYPE_PERFORMANCE)
                {

                }
            }
            else if (!cli_config)
            {
                if (s_service_type == DB_TYPE_TIMER_SEND && rtos_is_timer_init(&s_service_timer_send_mode_timer_handle))
                {
                    if (rtos_is_timer_running(&s_service_timer_send_mode_timer_handle))
                    {
                        rtos_stop_timer(&s_service_timer_send_mode_timer_handle);
                    }

                    rtos_deinit_timer(&s_service_timer_send_mode_timer_handle);
                }
            }
        }
        else if(param->handle == s_char2_attr_handle)
        {

        }
        else if(param->handle == s_char3_attr_handle)
        {

        }
    }
    break;

    case BK_GATTS_RESPONSE_EVT:
    {
        struct gatts_rsp_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTS_RESPONSE_EVT %d %d", param->status, param->handle);

        if (s_service_type == DB_TYPE_PERFORMANCE)
        {
            ble_at_timer_send_mode_timer_callback(NULL);
        }
    }
    break;

    case BK_GATTS_CONNECT_EVT:
    {
        struct gatts_connect_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTS_CONNECT_EVT %d %02X:%02X:%02X:%02X:%02X:%02X", param->link_role,
                   param->remote_bda[5],
                   param->remote_bda[4],
                   param->remote_bda[3],
                   param->remote_bda[2],
                   param->remote_bda[1],
                   param->remote_bda[0]);

        s_dm_gatt_api.conn_handle = param->conn_id;
    }
    break;

    case BK_GATTS_DISCONNECT_EVT:
    {
        struct gatts_disconnect_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTS_DISCONNECT_EVT %02X:%02X:%02X:%02X:%02X:%02X",
                   param->remote_bda[5],
                   param->remote_bda[4],
                   param->remote_bda[3],
                   param->remote_bda[2],
                   param->remote_bda[1],
                   param->remote_bda[0]);


        s_dm_gatt_api.conn_handle = 0xff;

        //        if (rtos_is_timer_init(&s_char_notify_timer))
        //        {
        //            if (rtos_is_timer_running(&s_char_notify_timer))
        //            {
        //                rtos_stop_timer(&s_char_notify_timer);
        //            }
        //
        //            rtos_deinit_timer(&s_char_notify_timer);
        //        }

        //        bk_bd_addr_t addr;
        //        uint8_t addr_type = 0;
        //        const bk_ble_gap_ext_adv_t ext_adv =
        //        {
        //            .instance = 0,
        //            .duration = 0,
        //            .max_events = 0,
        //        };
        //
        //        if (0)//dm_gat_get_authen_status(addr, &addr_type))
        //        {
        //            bk_ble_gap_ext_adv_params_t adv_param =
        //            {
        //                .type = BK_BLE
    }
    break;



    case BK_GATTS_MTU_EVT:
    {
        struct gatts_mtu_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTS_MTU_EVT %d %d", param->conn_id, param->mtu);
    }
    break;

    default:
        break;
    }

    return 0;
}


static int32_t bk_gattc_at_cb (bk_gattc_cb_event_t event, bk_gatt_if_t gattc_if, bk_ble_gattc_cb_param_t *comm_param)
{
    ble_err_t ret = 0;
    //bk_gatt_auth_req_t auth_req = BK_GATT_AUTH_REQ_NONE;

    //const uint16_t client_config_noti_enable = 1, client_config_indic_enable = 2, client_config_all_disable = 0;

    switch (event)
    {
    case BK_GATTC_REG_EVT:
    {
        struct gattc_reg_evt_param *param = (typeof(param))comm_param;

        s_dm_gatt_api.gattc_if = param->gatt_if;
        bt_at_logi("reg ret gatt_if %d", s_dm_gatt_api.gattc_if);

        if (*ble_at_get_sema_handle() != NULL)
        {
            rtos_set_semaphore( ble_at_get_sema_handle() );
        }
    }

    break;

    case BK_GATTC_DIS_SRVC_CMPL_EVT:
    {
        struct gattc_dis_srvc_cmpl_evt_param *param = (typeof(param))comm_param;

        //bk_bt_uuid_t uuid = {BK_UUID_LEN_16, {BK_GATT_UUID_CHAR_DECLARE}};

        bt_at_logi("BK_GATTC_DIS_SRVC_CMPL_EVT %d %d", param->status, param->conn_id);
    }
    break;

    case BK_GATTC_DIS_RES_SERVICE_EVT:
    {
        struct gattc_dis_res_service_evt_param *param = (typeof(param))comm_param;
        bt_at_logi("BK_GATTC_DIS_RES_SERVICE_EVT count %d", param->count);

        for (int i = 0; i < param->count; ++i)
        {
            switch (param->array[i].srvc_id.uuid.len)
            {
            case BK_UUID_LEN_16:
            {
                bt_at_logi("0x%04x %d~%d", param->array[i].srvc_id.uuid.uuid.uuid16, param->array[i].start_handle, param->array[i].end_handle);
            }
            break;

            case BK_UUID_LEN_128:
            {
                uint16_t short_uuid = 0;

                memcpy(&short_uuid, &param->array[i].srvc_id.uuid.uuid.uuid128[BK_UUID_LEN_128 - 4], sizeof(short_uuid));
                bt_at_logi("0x%04x %d~%d", short_uuid, param->array[i].start_handle, param->array[i].end_handle);
            }
            break;
            }
        }
    }
    break;

    case BK_GATTC_DIS_RES_CHAR_EVT:
    {
        struct gattc_dis_res_char_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTC_DIS_RES_CHAR_EVT count %d", param->count);

        for (int i = 0; i < param->count; ++i)
        {
            switch (param->array[i].uuid.uuid.len)
            {
            case BK_UUID_LEN_16:
            {
                bt_at_logi("0x%04x %d~%d %d", param->array[i].uuid.uuid.uuid.uuid16,
                           param->array[i].start_handle, param->array[i].end_handle, param->array[i].char_value_handle);
            }
            break;

            case BK_UUID_LEN_128:
            {
                uint16_t short_uuid = 0;

                memcpy(&short_uuid, &param->array[i].uuid.uuid.uuid.uuid128[BK_UUID_LEN_128 - 4], sizeof(short_uuid));
                bt_at_logi("0x%04x %d~%d", short_uuid, param->array[i].start_handle, param->array[i].end_handle);
            }
            break;
            }
        }
    }
    break;

    case BK_GATTC_DIS_RES_CHAR_DESC_EVT:
    {
        struct gattc_dis_res_char_desc_evt_param *param = (typeof(param))comm_param;
        bt_at_logi("BK_GATTC_DIS_RES_CHAR_DESC_EVT count %d", param->count);

        for (int i = 0; i < param->count; ++i)
        {
            switch (param->array[i].uuid.uuid.len)
            {
            case BK_UUID_LEN_16:
            {
                bt_at_logi("0x%04x char_handle %d desc_handle %d", param->array[i].uuid.uuid.uuid.uuid16, param->array[i].char_handle, param->array[i].desc_handle);
            }
            break;

            case BK_UUID_LEN_128:
            {
                uint16_t short_uuid = 0;

                memcpy(&short_uuid, &param->array[i].uuid.uuid.uuid.uuid128[BK_UUID_LEN_128 - 4], sizeof(short_uuid));
                bt_at_logi("0x%04x char_handle %d desc_handle %d", short_uuid, param->array[i].char_handle, param->array[i].desc_handle);
            }
            break;

            default:
                bt_at_loge("unknow uuid len %d", param->array[i].uuid.uuid.len);
                break;
            }
        }
    }
    break;

    case BK_GATTC_READ_CHAR_EVT:
    {
        struct gattc_read_char_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTC_READ_CHAR_EVT 0x%x %d %d", param->status, param->handle, param->value_len);
    }
    break;

    case BK_GATTC_READ_DESCR_EVT:
    {
        struct gattc_read_char_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTC_READ_DESCR_EVT %x %d %d", param->status, param->handle, param->value_len);
    }
    break;

    case BK_GATTC_READ_BY_TYPE_EVT:
    {
        struct gattc_read_by_type_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTC_READ_BY_TYPE_EVT %d %d %d", param->status, param->conn_id, param->elem_count);
    }
    break;

    case BK_GATTC_READ_MULTIPLE_EVT:
    {
        struct gattc_read_char_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTC_READ_MULTIPLE_EVT %x %d %d", param->status, param->handle, param->value_len);
    }
    break;

    case BK_GATTC_WRITE_CHAR_EVT:
    {
        struct gattc_write_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTC_WRITE_CHAR_EVT %d %d %d %d", param->status, param->conn_id, param->handle, param->offset);
    }
    break;

    case BK_GATTC_WRITE_DESCR_EVT:
    {
        struct gattc_write_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTC_WRITE_DESCR_EVT %d %d %d %d", param->status, param->conn_id, param->handle, param->offset);
    }
    break;

    case BK_GATTC_PREP_WRITE_EVT:
    {
        struct gattc_write_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTC_PREP_WRITE_EVT %d %d %d %d", param->status, param->conn_id, param->handle, param->offset);
    }
    break;

    case BK_GATTC_EXEC_EVT:
    {
        struct gattc_exec_cmpl_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTC_EXEC_EVT %d %d", param->status, param->conn_id);
    }
    break;

    case BK_GATTC_NOTIFY_EVT:
    {
        struct gattc_notify_evt_param *param = (typeof(param))comm_param;
        //static uint8_t recv_count = 0;

        bt_at_logi("BK_GATTC_NOTIFY_EVT %d %d handle %d vallen %d %02X:%02X:%02X:%02X:%02X:%02X", param->conn_id,
                   param->is_notify,
                   param->handle,
                   param->value_len,
                   param->remote_bda[5],
                   param->remote_bda[4],
                   param->remote_bda[3],
                   param->remote_bda[2],
                   param->remote_bda[1],
                   param->remote_bda[0]);

        s_service_performance_rx_all_bytes += param->value_len;

        if (s_service_type == DB_TYPE_PERFORMANCE)
        {

        }
    }
    break;

    case BK_GATTC_CONNECT_EVT:
    {
        struct gattc_connect_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTC_CONNECT_EVT %d %02X:%02X:%02X:%02X:%02X:%02X", param->link_role,
                   param->remote_bda[5],
                   param->remote_bda[4],
                   param->remote_bda[3],
                   param->remote_bda[2],
                   param->remote_bda[1],
                   param->remote_bda[0]);

        //s_conn_id = param->conn_id;
    }
    break;

    case BK_GATTC_DISCONNECT_EVT:
    {
        struct gattc_disconnect_evt_param *param = (typeof(param))comm_param;

        bt_at_logi("BK_GATTC_DISCONNECT_EVT %02X:%02X:%02X:%02X:%02X:%02X",
                   param->remote_bda[5],
                   param->remote_bda[4],
                   param->remote_bda[3],
                   param->remote_bda[2],
                   param->remote_bda[1],
                   param->remote_bda[0]);

        //        s_conn_id = 0xff;
    }
    break;

    default:
        break;
    }

    return ret;
}

int32_t ble_at_reg_gatts_db(uint8_t db_type)
{
    int32_t ret = 0;

    if (*ble_at_get_sema_handle() == NULL)
    {
        ret = rtos_init_semaphore(ble_at_get_sema_handle(), 1);

        if (ret != 0)
        {
            bt_at_loge("rtos_init_semaphore err %d", ret);
            return -1;
        }
    }

    ret = bk_ble_gatts_create_attr_tab(s_gatts_attr_db_service_performance_test, s_dm_gatt_api.gatts_if,
                                       sizeof(s_gatts_attr_db_service_performance_test) / sizeof(s_gatts_attr_db_service_performance_test[0]), 30);

    if (ret != 0)
    {
        bt_at_loge("bk_ble_gatts_create_attr_tab err %d", ret);
        return -1;
    }

    ret = rtos_get_semaphore(ble_at_get_sema_handle(), AT_SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        bt_at_loge("rtos_get_semaphore err %d", ret);
        return -1;
    }

    s_service_type = db_type;
    bk_ble_gatts_start_service(s_service_attr_handle);

    ret = rtos_get_semaphore(ble_at_get_sema_handle(), AT_SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        bt_at_loge("rtos_get_semaphore err %d", ret);
        return -1;
    }

    return 0;
}


ble_at_gatt_ctx_handle_t *ble_at_get_gatt_ctx_handle(void)
{
    return &s_dm_gatt_api;
}

int32_t ble_at_gattc_reg(void)
{
    ble_err_t ret = 0;

    if(s_dm_gatt_api.gattc_if)
    {
        return 0;
    }

    if (*ble_at_get_sema_handle() == NULL)
    {
        ret = rtos_init_semaphore(ble_at_get_sema_handle(), 1);

        if (ret != 0)
        {
            bt_at_loge("rtos_init_semaphore err %d", ret);
            return -1;
        }
    }

    bk_ble_gattc_register_callback(bk_gattc_at_cb);

    ret = bk_ble_gattc_app_register(0);

    if (ret)
    {
        bt_at_loge("reg err %d", ret);
        return -1;
    }

    ret = rtos_get_semaphore(ble_at_get_sema_handle(), AT_SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        bt_at_loge("rtos_get_semaphore reg err %d", ret);
        return -1;
    }

    return 0;
}

int32_t ble_at_gatts_reg(void)
{
    ble_err_t ret = 0;

    if(s_dm_gatt_api.gatts_if)
    {
        return 0;
    }

    if (*ble_at_get_sema_handle() == NULL)
    {
        ret = rtos_init_semaphore(ble_at_get_sema_handle(), 1);

        if (ret != 0)
        {
            bt_at_loge("rtos_init_semaphore err %d", ret);
            return -1;
        }
    }

    bk_ble_gatts_register_callback(bk_gatts_at_cb);

    ret = bk_ble_gatts_app_register(0);

    if (ret)
    {
        bt_at_loge("reg err %d", ret);
        return -1;
    }

    ret = rtos_get_semaphore(ble_at_get_sema_handle(), AT_SYNC_CMD_TIMEOUT_MS);

    if (ret != kNoErr)
    {
        bt_at_loge("rtos_get_semaphore reg err %d", ret);
        return -1;
    }

    return 0;
}

