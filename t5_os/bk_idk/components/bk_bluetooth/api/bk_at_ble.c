//#include <stdio.h>
//#include <stdlib.h>
#include <common/bk_typedef.h>
//#include "common.h"
#include <os.h>
#include <str.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <components/log.h>
#include <mem.h>



#include "atsvr_unite.h"
#include "bk_at_ble.h"

#include "modules/wifi.h"
#include <driver/gpio.h>
#include "components/bluetooth/bk_dm_bluetooth.h"

#include "ble_api_5_x.h"

#if CONFIG_BLE//(CONFIG_BLE_5_X || CONFIG_BTDM_5_2)
//#include "at_ble_common.h"

#include "bluetooth_legacy_include.h"

#ifdef CONFIG_ALI_MQTT
#include "iot_import.h"
#include "iot_export.h"
#include "iot_export_mqtt.h"
#endif

#include "components/bluetooth/bk_dm_gatt_types.h"
#include "components/bluetooth/bk_dm_gatts.h"
#include "components/bluetooth/bk_dm_gattc.h"
#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_gap_ble_types.h"
#include "components/bluetooth/bk_dm_gap_ble.h"
#include "../bt/ble_boarding/ble_boarding.h"
#include "ble_at_gatt.h"

#if CONFIG_MBEDTLS
#include "mbedtls/aes.h"
#endif

enum
{
    TEST_IDX_SVC,
    TEST_IDX_CHAR_DECL,
    TEST_IDX_CHAR_VALUE,
    TEST_IDX_CHAR_DESC,

    TEST_IDX_CHAR_DATALEN_DECL,
    TEST_IDX_CHAR_DATALEN_VALUE,

    TEST_IDX_CHAR_INTER_DECL,
    TEST_IDX_CHAR_INTER_VALUE,

    TEST_IDX_CHAR_WRITE_TEST_DECL,
    TEST_IDX_CHAR_WRITE_TEST_VALUE,
    TEST_IDX_CHAR_WRITE_TEST_DESC,

    TEST_IDX_NB,
};



static beken_semaphore_t ble_at_cmd_sema = NULL;
ble_err_t at_cmd_status = BK_ERR_BLE_SUCCESS;
uint8 g_test_prf_task_id = 0;
uint8 g_test_noti_att_id = 0;

//#if CONFIG_BTDM_5_2
static beken_timer_t ble_noti_tmr;
static beken_timer_t ble_performance_tx_statistics_tmr;
static beken_timer_t ble_performance_rx_statistics_tmr;
static uint32 s_test_noti_count = 0;
//#endif
static uint16_t s_test_data_len = 20;
static uint32_t s_test_send_inter = 500;

uint8_t s_test_conn_ind = ~0;


static uint8_t s_service_type = 0;
static uint8_t s_device_name_hdl = 0;

static uint32_t s_performance_tx_bytes = 0;
static uint32_t s_performance_rx_bytes = 0;
static uint8_t s_performance_tx_enable = 0;


//for ethermind
static beken_timer_t s_ethermind_ble_send_test_timer;
static uint16_t s_ethermind_send_test_service_handle = 0;;
static uint16_t s_ethermind_send_char_test_handle = 0;
static uint16_t s_ethermind_send_size_handle = 0;
static uint16_t s_ethermind_send_intv_handle = 0;


static uint8_t s_ethermind_send_test_value = 0xa;
static uint16_t s_ethermind_current_mtu = 23;
static uint16_t s_ethermind_send_size_value = 23 - 3;
static uint16_t s_ethermind_send_intv_value = 1000;

static uint8_t s_ethermind_att_handle; //ATT_CON_ID


static uint8_t s_ethermind_service_type = 0;

static uint32_t s_ethermind_performance_tx_bytes = 0;
static uint32_t s_ethermind_performance_rx_bytes = 0;
static uint8_t s_ethermind_auto_tx_enable = 0;

static beken_timer_t ble_ethermind_performance_tx_statistics_tmr;
static beken_timer_t ble_ethermind_performance_rx_statistics_tmr;


static ATT_ATTR_HANDLE s_ethermind_nordic_write_attr_handle[8];
static ATT_ATTR_HANDLE s_ethermind_nordic_write_notify_handle[8];
//static ATT_HANDLE s_ethermind_nordic_att_info[8];
static uint8_t s_ethermind_nordic_att_info[8];
static uint8_t s_ethermind_nordic_used[8] = {0};

const uint8_t nus_tx_uuid[] = {0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x03, 0x00, 0x40, 0x6e};
const uint8_t nus_rx_uuid[] = {0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x02, 0x00, 0x40, 0x6e};
const uint8_t device_name_uuid[] = {0x00, 0x2A, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static uint16_t plc_ccc_handle[10] = {0};
static uint16_t plc_rx_handle[10] = {0};

static uint8_t send_value[32] = {0};
//static ATT_HANDLE att_handle;
static ATT_ATTR_HANDLE s_read_tmp_attr_handle = 0;

static ble_read_phy_t s_ble_phy = {0, 0};

#define TAG  "AT_BLE"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct
{
    uint8_t state;
    uint8_t peer_addr_type;
    bd_addr_t peer_addr;
    uint16_t stability_wt_handle;
    uint32_t s_recv_count;
    beken2_timer_t m_tmr;
} ble_st_conn_env_t;

ble_st_conn_env_t ble_st_conn_env[10];

static uint8 stability_test_m_enabled = 0;
static uint8 stability_test_s_enabled = 0;

static uint8_t s_is_adv_set_periodic;
static beken_timer_t s_gap_create_connection_timeout_tmr;

extern void at_set_data_handle(uint8_t *out, char *buff, uint16_t len);

int get_addr_from_param(bd_addr_t *bdaddr, char *input_param);

static int ble_convert_128b_2_16b_uuid(uint8_t *uuid128, uint16_t *uuid16);
static void ble_att_sdp_charac_callback(CHAR_TYPE type, uint8 conidx, uint16_t hdl, uint16_t len, uint8 *data);

static int ble_at_findcmd_is_sync(char *name);

int set_ble_name_handle(int sync, int argc, char **argv);

int get_ble_name_handle(int sync, int argc, char **argv);

int ble_set_adv_param_handle(int sync, int argc, char **argv);

int ble_set_adv_data_handle(int sync, int argc, char **argv);

int ble_set_per_adv_data_handle(int sync, int argc, char **argv);

int ble_set_scan_rsp_data_handle(int sync, int argc, char **argv);

int ble_set_adv_enable_handle(int sync, int argc, char **argv);

int ble_set_scan_param_handle(int sync, int argc, char **argv);

int ble_set_scan_enable_handle(int sync, int argc, char **argv);

int ble_create_connect_handle(int sync, int argc, char **argv);

int ble_cancel_create_connect_handle(int sync, int argc, char **argv);

int ble_disconnect_handle(int sync, int argc, char **argv);

int ble_update_conn_param_handle(int sync, int argc, char **argv);

int ble_get_conn_state_handle(int sync, int argc, char **argv);

int ble_get_local_addr_handle(int sync, int argc, char **argv);

int ble_set_local_addr_handle(int sync, int argc, char **argv);




static int ble_register_service_handle(int sync, int argc, char **argv);
int ble_register_noti_service_handle(int sync, int argc, char **argv);
int ble_register_performance_service_handle(int sync, int argc, char **argv);

int ble_enable_performance_statistic_handle(int sync, int argc, char **argv);

int ble_tx_test_enable_handle(int sync, int argc, char **argv);
int ble_tx_test_param_handle(int sync, int argc, char **argv);
int ble_update_mtu_2_max_handle(int sync, int argc, char **argv);
void ble_test_service_write_handle(uint8 val, uint8 con_idx);
int ble_read_phy_handle(int sync, int argc, char **argv);
int ble_set_phy_handle(int sync, int argc, char **argv);
int ble_set_max_mtu_handle(int sync, int argc, char **argv);
//#endif
int ble_delete_adv_activity_handle(int sync, int argc, char **argv);
int ble_delete_scan_activity_handle(int sync, int argc, char **argv);
extern int ble_boarding_handle(int sync, int argc, char **argv);
void ble_test_noti_hdl(void *param);
static void ble_performance_tx_timer_hdl(void *param);
static void ble_performance_rx_timer_hdl(void *param);
int ble_connect_by_name_handle(int sync, int argc, char **argv);
int ble_disconnect_by_name_handle(int sync, int argc, char **argv);

int ble_create_periodic_sync_handle(int sync, int argc, char **argv);
int ble_start_periodic_sync_handle(int sync, int argc, char **argv);
int ble_stop_periodic_sync_handle(int sync, int argc, char **argv);
int ble_att_write_handle(int sync, int argc, char **argv);

static void ble_tx_test_active_timer_callback(void *param);
static void ble_ethermind_performance_tx_timer_hdl(void *param);
static void ble_ethermind_performance_rx_timer_hdl(void *param);
int ble_enable_packet_loss_ratio_test_handle(int sync, int argc, char **argv);
#ifdef CONFIG_ALI_MQTT
int ble_mqtt_loop_handle(int sync, int argc, char **argv);
#endif
int ble_power_handle(int sync, int argc, char **argv);

static int ble_att_read_handle(int sync, int argc, char **argv);

int ble_stability_test_handle(int sync, int argc, char **argv);
static void ble_stability_show_recv_info(uint8_t *value, uint16_t len, uint8 con_idx);
void ble_stability_test_master_reconnect_timer_hdl(void *param, unsigned int ulparam);

int ble_set_atcmd_version_handle(int sync, int argc, char **argv);
int ble_unregister_service_handle(int sync, int argc, char **argv);
int ble_create_bond_handle(int sync, int argc, char **argv);

#if 1
int set_ble_device_name_handle_gap(int sync, int argc, char **argv);
int get_ble_device_name_handle_gap(int sync, int argc, char **argv);
int ble_set_adv_param_handle_gap(int sync, int argc, char **argv);
int ble_set_adv_data_raw_handle_gap(int sync, int argc, char **argv);
int ble_set_adv_data_handle_gap(int sync, int argc, char **argv);
int ble_set_scan_rsp_data_handle_gap(int sync, int argc, char **argv);
int ble_set_scan_rsp_data_raw_handle_gap(int sync, int argc, char **argv);
int ble_set_adv_enable_handle_gap(int sync, int argc, char **argv);
int ble_set_scan_param_handle_gap(int sync, int argc, char **argv);
int ble_set_scan_enable_handle_gap(int sync, int argc, char **argv);
int ble_set_per_adv_param_handle_gap(int sync, int argc, char **argv);
int ble_set_per_adv_data_handle_gap(int sync, int argc, char **argv);
int ble_set_per_adv_enable_handle_gap(int sync, int argc, char **argv);
#endif

#ifdef CONFIG_ALI_MQTT
static void ble_send_data_2_mqtt(uint8 con_idx, uint16_t len, uint8 *data);
uint8_t loop_type = 0;


enum
{
    LT_NONE,
    LT_WIFI_ONLY,
    LT_COEX,
};
#endif
#if 0
int ble_set_atcmd_version_handle(int argc, char **argv)
{
    int err = kNoErr;


    if (argc != 1)
    {
        BK_LOGE(TAG, "input param error\r\n");
        err = kParamErr;
        goto error;
    }

    atcmd_updated = os_strtoul(argv[0], NULL, 10) & 0xFF;
    atsvr_cmd_rsp_ok();

error:
    atsvr_cmd_rsp_error();
    return err;
}
#endif
void at_set_data_handle(uint8_t *out, char *buff, uint16_t len)
{
    char temp[3];

    int i = 0, j = 0;
    for (i = 0; i < len;)
    {
        os_memcpy(temp, buff + i, 2);
        temp[2] = '\0';
        i = i + 2;
        out[j++] = os_strtoul(temp, NULL, 16) & 0xFF;
    }
}

int get_addr_from_param(bd_addr_t *bdaddr, char *input_param)
{
    int err = kNoErr;
    uint8_t addr_len = os_strlen(input_param);
    char temp[3];
    uint8_t j = 1;
    uint8_t k = 0;

    if (addr_len != (BK_BLE_GAP_BD_ADDR_LEN * 2 + 5))
    {
        err = kParamErr;
        return err;
    }

    for (uint8_t i = 0; i < addr_len; i++)
    {
        if (input_param[i] >= '0' && input_param[i] <= '9')
        {
            temp[k] = input_param[i];
            k += 1;
        }
        else if (input_param[i] >= 'a' && input_param[i] <= 'f')
        {
            temp[k] = input_param[i];
            k += 1;
        }
        else if (input_param[i] >= 'A' && input_param[i] <= 'F')
        {
            temp[k] = input_param[i];
            k += 1;
        }
        else if (input_param[i] == ':')
        {
            temp[k] = '\0';
            bdaddr->addr[BK_BLE_GAP_BD_ADDR_LEN - j] = os_strtoul(temp, NULL, 16) & 0xFF;
            k = 0;
            j++;
        }
        else
        {
            err = kParamErr;
            return err;
        }

        if (i == (addr_len - 1))
        {
            temp[k] = '\0';
            bdaddr->addr[BK_BLE_GAP_BD_ADDR_LEN - j] = os_strtoul(temp, NULL, 16) & 0xFF;
            k = 0;
        }
    }

    return kNoErr;
}


void ble_at_cmd_cb(ble_cmd_t cmd, ble_cmd_param_t *param)
{
    at_cmd_status = param->status;
    switch (cmd)
    {
        case BLE_CREATE_ADV:
        case BLE_SET_ADV_DATA:
        case BLE_SET_RSP_DATA:
        case BLE_START_ADV:
        case BLE_STOP_ADV:
        case BLE_CREATE_SCAN:
        case BLE_START_SCAN:
        case BLE_STOP_SCAN:
        case BLE_INIT_CREATE:
        case BLE_INIT_START_CONN:
        case BLE_INIT_STOP_CONN:
        case BLE_CONN_DIS_CONN:
        case BLE_CONN_UPDATE_PARAM:
        case BLE_DELETE_ADV:
        case BLE_DELETE_SCAN:
        case BLE_CONN_READ_PHY:
        case BLE_CONN_SET_PHY:
        case BLE_CONN_UPDATE_MTU:
        case BLE_CREATE_PERIODIC:
        case BLE_START_PERIODIC:
        case BLE_STOP_PERIODIC:
        case BLE_DELETE_PERIODIC:
        case BLE_SET_LOCAL_NAME:
        case BLE_GET_LOCAL_NAME:
        case BLE_READ_LOCAL_ADDR:
        case BLE_SET_RANDOM_ADDR:
            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
            break;
        default:
            break;
    }

}

enum
{
    STATE_IDLE,
    STATE_DISCOVERING,
    STATE_DISCOVERED,
    STATE_CONNECTINIG,
    STATE_CONNECTED,
    STATE_DISCONNECTINIG,
};

typedef struct
{
    uint8_t len;
    uint8_t name[50];
} ble_device_name_t;

typedef struct
{
    uint8_t state;
    uint8_t addr_type;
    bd_addr_t bdaddr;
    ble_device_name_t dev;
} ble_device_info_t;

ble_device_info_t g_peer_dev;

static uint8_t ble_check_device_name(uint8_t *p_buf, uint8_t data_len, ble_device_name_t *dev_name)
{
    uint8_t *p_data = p_buf;
    uint8_t *p_data_end = p_buf + data_len;
    uint8_t name_len = 0;

    uint8_t is_found = 0;

    while (p_data < p_data_end)
    {
        if (*(p_data + 1) == 0x09)//GAP_AD_TYPE_COMPLETE_NAME
        {
            name_len = *p_data - 1;

            if ((name_len == dev_name->len) && (!os_memcmp(dev_name->name, p_data + 2, name_len)))
            {
                is_found = 1;
            }
            break;
        }

        // Go to next advertising info
        p_data += (*p_data + 1);
    }

    return (is_found);
}

static uint8_t ble_get_device_name_from_adv(uint8_t *p_buf, uint8_t data_len, uint8_t *dev_name, uint8_t *in_out_len)
{
    uint8_t *p_data = p_buf;
    uint8_t *p_data_end = p_buf + data_len;

    uint8_t is_found = 0;

    while (p_data < p_data_end)
    {
        if (*(p_data + 1) == 0x09)//GAP_AD_TYPE_COMPLETE_NAME
        {
            is_found = 1;

            os_memcpy(dev_name, p_data + 2, ((p_data[0] - 1 > *in_out_len) ? *in_out_len : p_data[0] - 1));
            break;
        }

        // Go to next advertising info
        p_data += (*p_data + 1);
    }

    return (is_found);
}

static int8_t ethermind_find_idle_info_index(uint8_t *index)
{
    for (uint8_t i = 0; i < sizeof(s_ethermind_nordic_used) / sizeof(s_ethermind_nordic_used[0]); ++i)
    {
        if (s_ethermind_nordic_used[i] == 0)
        {
            *index = i;
            return 0;
        }
    }

    return -1;
}

static int8_t ethermind_clean_info_by_index(uint8_t index)
{
    s_ethermind_nordic_used[index] = 0;
    os_memset(s_ethermind_nordic_write_attr_handle + index, 0, sizeof(*s_ethermind_nordic_write_attr_handle));
    os_memset(s_ethermind_nordic_att_info + index, 0, sizeof(*s_ethermind_nordic_att_info));

    return 0;
}

static int8_t ethermind_find_index_by_info(uint8_t *index, uint8_t att_con_id)
{

    for (uint8_t i = 0; i < sizeof(s_ethermind_nordic_used) / sizeof(s_ethermind_nordic_used[0]); ++i)
    {
        if (s_ethermind_nordic_used[i] &&
            s_ethermind_nordic_att_info[i] == att_con_id)
        {
            *index = i;
            return 0;
        }
    }

    return -1;
}

int8_t ethermind_find_index_by_att_con_id(uint8_t *index, ATT_CON_ID att_id)
{

    for (uint8_t i = 0; i < sizeof(s_ethermind_nordic_used) / sizeof(s_ethermind_nordic_used[0]); ++i)
    {
        if (s_ethermind_nordic_used[i] &&
            s_ethermind_nordic_att_info[i] == att_id)
        {
            *index = i;
            return 0;
        }
    }

    return -1;
}

static uint16_t s_write_test_ccc_val = 0;
static bk_ble_key_t s_ble_enc_key;
static uint8_t s_ble_auth_req = 0;

static uint8_t ble_check_random_bda_is_resolvable(uint8_t *addr, uint8_t addr_type)
{
    uint8_t ret = 0;
#if CONFIG_MBEDTLS
    uint8_t key[16];
    uint8_t plain_text[16];
    uint8_t encrypt_data[16];
    mbedtls_aes_context aes;

    if ((0 == addr_type) || (0x40 != (addr[5] & 0xc0))) //most significant bit, bit7, bit6 is 01 to be resolvable random
    {
        return ret;
    }

    mbedtls_aes_init(&aes);
    for (uint8_t i = 0; i < 16; i++)
    {
        key[15 -i] = s_ble_enc_key.pirk.irk[i];
    }
    mbedtls_aes_setkey_enc(&aes, key, 128);

    os_memset(plain_text, 0, sizeof(plain_text));
    plain_text[13] = addr[5];
    plain_text[14] = addr[4];
    plain_text[15] = addr[3];

    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, plain_text, encrypt_data);

    if ((encrypt_data[13] == addr[2]) && (encrypt_data[14] == addr[1]) && (encrypt_data[15] == addr[0]))
    {
        ret = 1;
        LOGI("the random bda is resolvable!\r\n");
    }

    mbedtls_aes_free(&aes);
#endif
    return ret;
}

static void ble_at_notice_cb(ble_notice_t notice, void *param)
{
    switch (notice)
    {
        case BLE_5_WRITE_EVENT:
        {
            ble_write_req_t *w_req = (ble_write_req_t *)param;
            LOGI("write_cb:conn_idx:%d, prf_id:%d, att_idx:%d, len:%d, data[0]:0x%02x\r\n",
                 w_req->conn_idx, w_req->prf_id, w_req->att_idx, w_req->len, w_req->value[0]);
            //#if (CONFIG_BTDM_5_2)
            if (bk_ble_get_controller_stack_type() == BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2
                && w_req->prf_id == g_test_prf_task_id)
            {
                switch (w_req->att_idx)
                {
                    case TEST_IDX_CHAR_DECL:
                        break;
                    case TEST_IDX_CHAR_VALUE:
                        break;
                    case TEST_IDX_CHAR_DESC:
                        g_test_noti_att_id = TEST_IDX_CHAR_VALUE;
                        ble_test_service_write_handle(w_req->value[0], w_req->conn_idx);
                        break;

                    case TEST_IDX_CHAR_DATALEN_DECL:
                        break;
                    case TEST_IDX_CHAR_DATALEN_VALUE:
                        os_memcpy(&s_test_data_len, w_req->value, sizeof(s_test_data_len));
                        break;

                    case TEST_IDX_CHAR_INTER_DECL:
                        break;
                    case TEST_IDX_CHAR_INTER_VALUE:
                        os_memcpy(&s_test_send_inter, w_req->value, sizeof(s_test_send_inter));
                        break;

                    case TEST_IDX_CHAR_WRITE_TEST_VALUE:
                        ble_stability_show_recv_info(w_req->value, w_req->len, w_req->conn_idx);
                        break;

                    case TEST_IDX_CHAR_WRITE_TEST_DESC:
                        os_memcpy(&s_write_test_ccc_val, &w_req->value[0], sizeof(s_write_test_ccc_val));
                        break;

                    default:
                        break;
                }
            }
            //#endif
            break;
        }
        case BLE_5_READ_EVENT:
        {
            ble_read_req_t *r_req = (ble_read_req_t *)param;
            LOGI("read_cb:conn_idx:%d, prf_id:%d, att_idx:%d\r\n",
                 r_req->conn_idx, r_req->prf_id, r_req->att_idx);

            if (r_req->prf_id == g_test_prf_task_id)
            {
                switch (r_req->att_idx)
                {
                    case TEST_IDX_CHAR_DECL:
                        break;
                    case TEST_IDX_CHAR_VALUE:
                        break;
                    case TEST_IDX_CHAR_DESC:
                    {
                        uint8_t value[3] = {0};
                        uint32_t length = 3;
                        value[0] = 0x11;
                        value[1] = 0x22;
                        value[2] = 0x33;
                        length = 3;
                        bk_ble_read_response_value(r_req->conn_idx, length, value, r_req->prf_id, r_req->att_idx);
                        break;
                    }
                    case TEST_IDX_CHAR_DATALEN_DECL:

                        break;
                    case TEST_IDX_CHAR_DATALEN_VALUE:
                    {
                        uint8_t *value = (uint8_t *)&s_test_data_len;
                        bk_ble_read_response_value(r_req->conn_idx, sizeof(s_test_data_len), value, r_req->prf_id, r_req->att_idx);
                        break;
                    }

                    case TEST_IDX_CHAR_INTER_DECL:

                        break;

                    case TEST_IDX_CHAR_INTER_VALUE:
                    {
                        uint8_t *value = (uint8_t *)&s_test_send_inter;
                        bk_ble_read_response_value(r_req->conn_idx, sizeof(s_test_send_inter), value, r_req->prf_id, r_req->att_idx);
                        break;
                    }

                    case TEST_IDX_CHAR_WRITE_TEST_DESC:
                    {
                        uint8_t *value = (uint8_t *)&s_write_test_ccc_val;
                        bk_ble_read_response_value(r_req->conn_idx, sizeof(s_write_test_ccc_val), value, r_req->prf_id, r_req->att_idx);
                        break;
                    }

                    default:
                        break;
                }
            }
            break;
        }
        case BLE_5_REPORT_ADV:
        {
            ble_recv_adv_t *r_ind = (ble_recv_adv_t *)param;
            uint8_t adv_type = r_ind->evt_type & REPORT_INFO_REPORT_TYPE_MASK;
            //LOGI("r_ind:actv_idx:%d,", r_ind->actv_idx);
            char *type_log = NULL;
            switch (adv_type)
            {
                case REPORT_TYPE_ADV_EXT:
                    type_log = "EXT_ADV,";
                    break;
                case REPORT_TYPE_ADV_LEG:
                {
                    switch (r_ind->evt_type)
                    {
                        case (REPORT_INFO_SCAN_ADV_BIT | REPORT_INFO_CONN_ADV_BIT | REPORT_INFO_COMPLETE_BIT | REPORT_TYPE_ADV_LEG):
                            type_log = "ADV_IND,";
                            break;
                        case (REPORT_INFO_DIR_ADV_BIT | REPORT_INFO_CONN_ADV_BIT | REPORT_INFO_COMPLETE_BIT | REPORT_TYPE_ADV_LEG):
                            type_log = "ADV_DIRECT_IND,";
                            break;
                        case (REPORT_INFO_SCAN_ADV_BIT | REPORT_INFO_COMPLETE_BIT | REPORT_TYPE_ADV_LEG):
                            type_log = "ADV_SCAN_IND,";
                            break;
                        case (REPORT_INFO_COMPLETE_BIT | REPORT_TYPE_ADV_LEG):
                            type_log = "ADV_NONCONN_IND,";
                            break;
                        default:
                            type_log = "ERR_LEG_ADV,";
                            break;
                    }
                }
                break;
                case REPORT_TYPE_SCAN_RSP_LEG:
                    type_log = "SCAN_RSP,";
                    break;
                case REPORT_TYPE_SCAN_RSP_EXT:
                    type_log = "AUX_SCAN_RSP,";
                    break;
                case REPORT_TYPE_PER_ADV:
                    type_log = "PER_ADV,";
                    break;
                default:
                    type_log = "ERR_ADV,";
                    break;
            }
            LOGI("%s addr_type:%d, adv_addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n", type_log,
                 r_ind->adv_addr_type, r_ind->adv_addr[0], r_ind->adv_addr[1], r_ind->adv_addr[2],
                 r_ind->adv_addr[3], r_ind->adv_addr[4], r_ind->adv_addr[5]);
            if ((STATE_DISCOVERING == g_peer_dev.state) && (ble_check_device_name(r_ind->data, r_ind->data_len, &(g_peer_dev.dev))))
            {
                LOGI("dev : %s is discovered\r\n", g_peer_dev.dev.name);
                os_memcpy(g_peer_dev.bdaddr.addr, r_ind->adv_addr, BK_BLE_GAP_BD_ADDR_LEN);
                g_peer_dev.addr_type = r_ind->adv_addr_type;
                g_peer_dev.state = STATE_DISCOVERED;
                if (ble_at_cmd_sema != NULL)
                {
                    rtos_set_semaphore(&ble_at_cmd_sema);
                }
            }
        }
        break;
        case BLE_5_MTU_CHANGE:
        {
            ble_mtu_change_t *m_ind = (ble_mtu_change_t *)param;
            LOGI("%s m_ind:conn_idx:%d, mtu_size:%d\r\n", __func__, m_ind->conn_idx, m_ind->mtu_size);
            s_ethermind_current_mtu = m_ind->mtu_size;
            break;
        }
        case BLE_5_CONNECT_EVENT:
        {
            ble_conn_ind_t *c_ind = (ble_conn_ind_t *)param;
            LOGI("c_ind:conn_idx:%d, addr_type:%d, peer_addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                 c_ind->conn_idx, c_ind->peer_addr_type, c_ind->peer_addr[0], c_ind->peer_addr[1],
                 c_ind->peer_addr[2], c_ind->peer_addr[3], c_ind->peer_addr[4], c_ind->peer_addr[5]);
            s_test_conn_ind = c_ind->conn_idx;


            break;
        }
        case BLE_5_DISCONNECT_EVENT:
        {

            ble_discon_ind_t *d_ind = (ble_discon_ind_t *)param;
            LOGI("d_ind:conn_idx:%d,reason:%d\r\n", d_ind->conn_idx, d_ind->reason);
            s_test_conn_ind = ~0;

            if (stability_test_s_enabled)
            {

                uint8_t actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_CREATED);

                if (actv_idx == AT_BLE_MAX_ACTV)
                {
                    LOGI("ble adv not created!\n");
                }
                else
                {
                    bk_ble_start_advertising(actv_idx, 0, NULL);
                }

            }


            break;
        }
        case BLE_5_ATT_INFO_REQ:
        {
            ble_att_info_req_t *a_ind = (ble_att_info_req_t *)param;
            LOGI("a_ind:conn_idx:%d\r\n", a_ind->conn_idx);
            a_ind->length = 128;
            a_ind->status = BK_ERR_BLE_SUCCESS;
            break;
        }
        case BLE_5_CREATE_DB:
        {

            //#if (CONFIG_BTDM_5_2)
            if (bk_ble_get_controller_stack_type() == BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2)
            {
                ble_create_db_t *cd_ind = (ble_create_db_t *)param;
                LOGI("cd_ind:prf_id:%d, status:%d\r\n", cd_ind->prf_id, cd_ind->status);

                at_cmd_status = cd_ind->status;
                if (ble_at_cmd_sema != NULL)
                {
                    rtos_set_semaphore(&ble_at_cmd_sema);
                }
            }
            //#endif

            break;
        }
        case BLE_5_INIT_CONNECT_EVENT:
        {
            ble_conn_ind_t *c_ind = (ble_conn_ind_t *)param;
            LOGI("BLE_5_INIT_CONNECT_EVENT:conn_idx:%d, addr_type:%d, peer_addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n",
                 c_ind->conn_idx, c_ind->peer_addr_type, c_ind->peer_addr[0], c_ind->peer_addr[1],
                 c_ind->peer_addr[2], c_ind->peer_addr[3], c_ind->peer_addr[4], c_ind->peer_addr[5]);

            if ((STATE_CONNECTINIG == g_peer_dev.state) && (!os_memcmp(g_peer_dev.bdaddr.addr, c_ind->peer_addr, BK_BLE_GAP_BD_ADDR_LEN)))
            {
                g_peer_dev.state = STATE_CONNECTED;
            }

            if (stability_test_m_enabled)
            {
                ble_st_conn_env[c_ind->conn_idx].state = 1;
                ble_st_conn_env[c_ind->conn_idx].peer_addr_type = c_ind->peer_addr_type;
                os_memcpy(ble_st_conn_env[c_ind->conn_idx].peer_addr.addr, c_ind->peer_addr, 6);
            }

            break;
        }
        case BLE_5_INIT_DISCONNECT_EVENT:
        {
            ble_discon_ind_t *d_ind = (ble_discon_ind_t *)param;
            LOGI("BLE_5_INIT_DISCONNECT_EVENT:conn_idx:%d,reason:%d\r\n", d_ind->conn_idx, d_ind->reason);

            if (stability_test_m_enabled)
            {
                ble_st_conn_env[d_ind->conn_idx].state = 0;
                ble_st_conn_env[d_ind->conn_idx].stability_wt_handle = 0;

                if (!rtos_is_oneshot_timer_init(&ble_st_conn_env[d_ind->conn_idx].m_tmr))
                {
                    rtos_init_oneshot_timer(&ble_st_conn_env[d_ind->conn_idx].m_tmr, 100, (timer_2handler_t)ble_stability_test_master_reconnect_timer_hdl, (void *)((uint32)d_ind->conn_idx), 0);
                    rtos_start_oneshot_timer(&ble_st_conn_env[d_ind->conn_idx].m_tmr);
                }
            }

            break;
        }

        case BLE_5_INIT_CONNECT_FAILED_EVENT:
        {
            ble_discon_ind_t *d_ind = (ble_discon_ind_t *)param;
            LOGI("BLE_5_INIT_CONNECT_FAILED_EVENT:conn_idx:%d,reason:%d\r\n", d_ind->conn_idx, d_ind->reason);

            if (stability_test_m_enabled)
            {
                if (!rtos_is_oneshot_timer_init(&ble_st_conn_env[d_ind->conn_idx].m_tmr))
                {
                    rtos_init_oneshot_timer(&ble_st_conn_env[d_ind->conn_idx].m_tmr, 100, (timer_2handler_t)ble_stability_test_master_reconnect_timer_hdl, (void *)((uint32)d_ind->conn_idx), 0);
                    rtos_start_oneshot_timer(&ble_st_conn_env[d_ind->conn_idx].m_tmr);
                }
            }

            break;
        }

        case BLE_5_SDP_REGISTER_FAILED:
            LOGI("BLE_5_SDP_REGISTER_FAILED\r\n");
            break;
        case BLE_5_READ_PHY_EVENT:
        {
            s_ble_phy = *(ble_read_phy_t *)param;
            LOGI("BLE_5_READ_PHY_EVENT:tx_phy:0x%02x, rx_phy:0x%02x\r\n", s_ble_phy.tx_phy, s_ble_phy.rx_phy);
            break;
        }
        case BLE_5_TX_DONE:
        {
            bk_ble_gatt_cmp_evt_t *evt = (bk_ble_gatt_cmp_evt_t *)param;
            uint8_t conn_idx = evt->conn_idx;
            if (s_service_type && s_performance_tx_enable == 1)
            {
                s_performance_tx_bytes += s_test_data_len;
                ble_test_noti_hdl((void *)((uint32)conn_idx));
            }
            else
            {
                BK_LOGI("ble_at", "BLE_5_TX_DONE\r\n");
            }
        }

        break;

        case BLE_5_CONN_UPDATA_EVENT:
        {
            ble_conn_param_t *updata_param = (ble_conn_param_t *)param;
            LOGI("BLE_5_CONN_UPDATA_EVENT:conn_interval:0x%04x, con_latency:0x%04x, sup_to:0x%04x\r\n", updata_param->intv_max,
                 updata_param->con_latency, updata_param->sup_to);


            break;
        }

        case BLE_5_PERIODIC_SYNC_CMPL_EVENT:
            LOGI("BLE_5_PERIODIC_SYNC_CMPL_EVENT \n");
            break;

        case BLE_5_RECV_NOTIFY_EVENT:
        {
            //ethermind

            ble_att_notify_t *tmp = (typeof(tmp))param;


            s_ethermind_performance_rx_bytes += tmp->len;

#ifdef CONFIG_ALI_MQTT
            if (loop_type == LT_COEX)
            {
                LOGI("ATT_CON_ID %d data %s len %d sendto mqtt\n",
                     tmp->conn_handle,
                     tmp->data, tmp->len);

                ble_send_data_2_mqtt(tmp->conn_handle, tmp->len, tmp->data);
            }
            else
#endif
            {
                uint8_t index = 0;
                if (0 == ethermind_find_index_by_info(&index, tmp->conn_handle))
                {
                    LOGI("%s ATT_CON_ID %d, send data to nordic len %d %s\n", __func__,
                         tmp->conn_handle, tmp->len, tmp->data);

                    bk_ble_att_write(tmp->conn_handle, s_ethermind_nordic_write_attr_handle[index], tmp->data, tmp->len);
                }
                else
                {
                    LOGI("%s cant find info ATT_CON_ID %d\n", __func__, tmp->conn_handle);
                }
            }
        }
        break;

        case BLE_5_DISCOVERY_PRIMARY_SERVICE_EVENT:
        {
            ble_discovery_primary_service_t *tmp = (typeof(tmp))param;
            LOGI("%s BLE_5_DISCOVERY_PRIMARY_SERVICE_EVENT count %d\n", __func__, tmp->count);
            for (uint8_t i = 0; i < tmp->count; i++)
            {
                //service's char must in this range
                LOGI("%s service uuid 0x%X start %d end %d\n", __func__, tmp->service[i].uuid.uuid_16,
                     tmp->service[i].range.start_handle, tmp->service[i].range.end_handle);

            }
        }
        break;

        case BLE_5_DISCOVERY_CHAR_EVENT:
        {
            ble_discovery_char_t *tmp = (typeof(tmp))param;
            uint8_t index = 0;
            uint8_t found = 0;
            uint8_t i = 0;

            LOGI("%s BLE_5_DISCOVERY_CHAR_EVENT count %d ATT_CON_ID %d\n", __func__, tmp->count,
                 tmp->conn_handle);

            //

            for (i = 0; i < tmp->count; ++i)
            {
                //LOGI("%s type %d %p\n", __func__, tmp->character[7].uuid_type, &tmp->character[7].uuid_type);

                switch (tmp->character[i].uuid_type)
                {
                    case ATT_16_BIT_UUID_FORMAT:
                        LOGI("%s char 16bit uuid 0x%04X, attr_handle %d\n", __func__, tmp->character[i].uuid.uuid_16, tmp->character[i].value_handle);
                        break;

                    case ATT_128_BIT_UUID_FORMAT:
                    {
                        //LOGI("%s char 128bit uuid 0x%08X\n", __func__, (uint32_t *)(tmp->character[i].uuid.uuid_128.value + 11));

                        if (!os_memcmp(&tmp->character[i].uuid.uuid_128.value, nus_rx_uuid, sizeof(nus_rx_uuid)))
                        {
                            found = 1;
                            i = tmp->count - 1;
                            break;
                        }
                    }
                    break;
                }
            }

            if (!found)
            {
                break;
            }


            if (0 == ethermind_find_index_by_info(&index, tmp->conn_handle))
            {
                LOGI("%s nordic info already exist ! att_id %d\n", __func__, tmp->conn_handle);
            }
            else if (0 == ethermind_find_idle_info_index(&index))
            {
            }
            else
            {
                LOGE("%s nordic info list is full !\n", __func__);
                break;
            }


            for (i = 0; i < tmp->count; ++i)
            {
                //LOGI("%s type %d %p\n", __func__, tmp->character[7].uuid_type, &tmp->character[7].uuid_type);

                switch (tmp->character[i].uuid_type)
                {
                    case ATT_16_BIT_UUID_FORMAT:
                        break;

                    case ATT_128_BIT_UUID_FORMAT:
                    {
                        LOGI("%s char 128bit uuid 0x%08X attr_hdl:%d\n", __func__, (uint32_t *)(tmp->character[i].uuid.uuid_128.value + 11), tmp->character[i].value_handle);

                        if (!os_memcmp(&tmp->character[i].uuid.uuid_128.value, nus_rx_uuid, sizeof(nus_rx_uuid)))
                        {
                            LOGI("%s add to nordic info list %d\n", __func__, index);
                            s_ethermind_nordic_used[index] = 1;
                            s_ethermind_nordic_write_attr_handle[index] = tmp->character[i].value_handle;
                            s_ethermind_nordic_att_info[index] = tmp->conn_handle;
                        }
                        else if (!os_memcmp(&tmp->character[i].uuid.uuid_128.value, nus_tx_uuid, sizeof(nus_tx_uuid)))
                        {
                            if (tmp->character[i].desc_index)
                            {
                                for (uint8_t j = 0; j < tmp->character[i].desc_index; ++j)
                                {
                                    if (ATT_16_BIT_UUID_FORMAT == tmp->character[i].descriptor[j].uuid_type && GATT_CLIENT_CONFIG == tmp->character[i].descriptor[j].uuid.uuid_16)
                                    {
                                        s_ethermind_nordic_write_notify_handle[index] = tmp->character[i].descriptor[j].handle;
                                        //                                const uint16_t noti_enable = 0x0001;
                                        //                                LOGI("%s write nordic enable notify\n", __func__);
                                        //                                bk_ble_att_write(&tmp->att_handle, tmp->character[i].descriptor[j].handle, (uint8_t *)&noti_enable, sizeof(noti_enable));
                                    }
                                }
                            }
                        }
                    }
                    break;

                    default:
                        LOGE("%s unknow uuid type %d %d %p\n", __func__, tmp->character[i].uuid_type, i, &tmp->character[i].uuid_type);
                        break;
                }
            }


        }
        break;
        case BLE_5_CONN_UPD_PAR_ASK:
        {

            ble_conn_update_para_ind_t *tmp = (typeof(tmp))param;
            LOGI("%s BLE_5_CONN_UPD_PAR_ASK accept\n", __func__);
            tmp->is_agree = 1;

        }
        break;
        case BLE_5_ATT_READ_RESPONSE:
        {
            ble_att_rw_t *tmp = (typeof(tmp))param;
            if (!tmp->event_result)
            {
                LOGI("%s att_id:%d, attr_hand:0x%x, result:%d\r\n", __func__,
                     tmp->conn_handle,
                     s_read_tmp_attr_handle,
                     tmp->event_result
                    );
                LOGI("data-> ");
                for (int i = 0; i < tmp->len; i++)
                {
                    LOGI("0x%x ", *tmp->data);
                }
                LOGI("\r\n");
                if (ble_at_cmd_sema != NULL)
                {
                    rtos_set_semaphore(&ble_at_cmd_sema);
                }
            }
            else
            {
                LOGE("%s: Recevied Error Response!!! att_id:%d, attr_hand:%d, resp code:%d", __func__,
                     tmp->conn_handle,
                     ((uint8_t *)(&(tmp->event_result)))[1],
                     ((uint8_t *)(&(tmp->event_result)))[0]
                    );
            }
        }
        break;

        case BLE_5_PAIRING_REQ:
        {
            LOGI("BLE_5_PAIRING_REQ\r\n");
            ble_smp_ind_t *s_ind = (ble_smp_ind_t *)param;
            bk_ble_sec_send_auth_mode(s_ind->conn_idx, GAP_AUTH_REQ_NO_MITM_BOND, BK_BLE_GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
                                      GAP_SEC1_NOAUTH_PAIR_ENC, GAP_OOB_AUTH_DATA_NOT_PRESENT);
            break;
        }

        case BLE_5_PARING_PASSKEY_REQ:
            LOGI("BLE_5_PARING_PASSKEY_REQ\r\n");
            break;

        case BLE_5_ENCRYPT_EVENT:
            LOGI("BLE_5_ENCRYPT_EVENT\r\n");
            at_cmd_status = 0;
            if ((ble_at_cmd_sema != NULL) && (s_ble_auth_req))
            {
                rtos_set_semaphore( &ble_at_cmd_sema);
            }
            break;

        case BLE_5_PAIRING_SUCCEED:
            LOGI("BLE_5_PAIRING_SUCCEED\r\n");
            at_cmd_status = 0;
            if ((ble_at_cmd_sema != NULL) && (s_ble_auth_req))
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
            break;

        case BLE_5_PAIRING_FAILED:
        {
            ble_smp_ind_t *s_ind = (ble_smp_ind_t *)param;
            at_cmd_status = s_ind->status;
            os_memset(&s_ble_enc_key, 0 ,sizeof(s_ble_enc_key));
            LOGI("BLE_5_PAIRING_FAILED 0x%x\r\n", s_ind->status);
            if ((ble_at_cmd_sema != NULL) && (s_ble_auth_req))
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        case BLE_5_DELETE_SERVICE_DONE:
        {
            if (bk_ble_get_controller_stack_type() == BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2)
            {
                ble_create_db_t *cd_ind = (ble_create_db_t *)param;
                LOGI("delete dervice done, prf_id:%d, status:%d\r\n", cd_ind->prf_id, cd_ind->status);

                at_cmd_status = cd_ind->status;
                if (ble_at_cmd_sema != NULL)
                {
                    rtos_set_semaphore(&ble_at_cmd_sema);
                }
            }
            break;
        }
        case BLE_5_GAP_CMD_CMP_EVENT:
        {
            ble_cmd_cmp_evt_t *event = (ble_cmd_cmp_evt_t *)param;

            switch (event->cmd)
            {
                case BLE_CONN_DIS_CONN:
				{
                	if (STATE_DISCONNECTINIG != g_peer_dev.state)
                	{
						break;
            		}
        		}
                case BLE_CONN_UPDATE_PARAM:
                case BLE_CONN_SET_PHY:
                case BLE_CONN_READ_PHY:
                case BLE_CONN_UPDATE_MTU:
                case BLE_SET_MAX_MTU:
                {
                    at_cmd_status = event->status;
                    if (ble_at_cmd_sema != NULL)
                    {
                        rtos_set_semaphore(&ble_at_cmd_sema);
                    }
                    break;
                }
                case BLE_CONN_ENCRYPT:
		        {
		            LOGI("BLE_5_GAP_CMD_CMP_EVENT(BLE_CONN_ENCRYPT) , status %x\r\n",event->status);
		            if (event->status)
		            {
		                os_memset(&s_ble_enc_key, 0 ,sizeof(s_ble_enc_key));
		                if ((ble_at_cmd_sema != NULL) && (s_ble_auth_req))
		                {
		                    at_cmd_status = event->status;
		                    rtos_set_semaphore( &ble_at_cmd_sema);
		                }
		                bk_ble_disconnect(event->conn_idx);
		            }
		        }
				break;
                case BLE_ADD_WHITE_LIST:
                case BLE_RMV_WHITE_LIST:
                case BLE_CLE_WHITE_LIST:
                    LOGI("BLE_WHITE_LIST_DONE, status 0x%x\r\n", event->status);
                    at_cmd_status = event->status;
                    if (ble_at_cmd_sema != NULL)
                    {
                        rtos_set_semaphore(&ble_at_cmd_sema);
                    }
                    break;
                default:
                    break;
            }

            break;
        }
        case BLE_5_KEY_EVENT:
        {
            bk_printf("BLE_5_KEY_EVENT\r\n");
            s_ble_enc_key = *((bk_ble_key_t *)param);
            break;
        }

        case BLE_5_BOND_INFO_REQ_EVENT:
        {
            bk_printf("BLE_5_BOND_INFO_REQ_EVENT\r\n");
            bk_ble_bond_info_req_t *bond_info_req = (bk_ble_bond_info_req_t *)param;
            if (((bond_info_req->key.peer_addr_type == s_ble_enc_key.peer_addr_type)
                && (!os_memcmp(bond_info_req->key.peer_addr, s_ble_enc_key.peer_addr, 6)))
                || (ble_check_random_bda_is_resolvable(bond_info_req->key.peer_addr, bond_info_req->key.peer_addr_type)))
            {
                bond_info_req->key_found = 1;
                bond_info_req->key = s_ble_enc_key;
            }
            break;
        }

        default:
            break;
    }
}

static uint32_t dm_ble_at_event_cb(ble_event_enum_t event, void *param)
{
    switch (event)
    {
#if 0
        case BK_DM_BLE_EVENT_REPORT_ADV:
        {
            ble_adv_report_t *r_ind = (typeof(r_ind)) param;
            char dname[64] = { 0 };
            uint8_t dname_len = sizeof(dname);

            memset(&dname, 0, sizeof(dname));

            ble_get_device_name_from_adv(r_ind->data, r_ind->data_len,
                                         (uint8_t *) dname, &dname_len);
            dname[dname_len - 1] = 0;

            if (r_ind->evt_type & (1 << 4))
            {
                LOGI("evt_type:LEG_ADV,");
            }
            else
            {
                LOGI("evt_type:EXT_ADV,");
            }

            LOGI(
                " adv_addr_type:%d, adv_addr:%02x:%02x:%02x:%02x:%02x:%02x name %s\r\n",
                r_ind->peer_address_type, r_ind->peer_address.addr[5],
                r_ind->peer_address.addr[4], r_ind->peer_address.addr[3],
                r_ind->peer_address.addr[2], r_ind->peer_address.addr[1],
                r_ind->peer_address.addr[0], dname);

            if ((STATE_DISCOVERING == g_peer_dev.state)
                && (ble_check_device_name(r_ind->data, r_ind->data_len,
                                          &(g_peer_dev.dev))))
            {
                LOGI("dev : %s is discovered\r\n", g_peer_dev.dev.name);
                os_memcpy(g_peer_dev.bdaddr.addr, r_ind->peer_address.addr,
                          BK_BLE_GAP_BD_ADDR_LEN);
                g_peer_dev.addr_type = r_ind->peer_address_type;
                g_peer_dev.state = STATE_DISCOVERED;

                if (ble_at_cmd_sema != NULL)
                {
                    rtos_set_semaphore(&ble_at_cmd_sema);
                }
            }

        }
        break;
#endif
        case BK_DM_BLE_EVENT_MTU_CHANGE:
        {
            ble_mtu_change_t *m_ind = (ble_mtu_change_t *) param;
            LOGI("%s m_ind:conn_idx:%d, mtu_size:%d\r\n", __func__, m_ind->conn_idx, m_ind->mtu_size);
            s_ethermind_current_mtu = m_ind->mtu_size;
            break;
        }

        case BK_DM_BLE_EVENT_CONNECT:
        {

            ble_conn_att_t *conn_att = (typeof(conn_att)) param;
            LOGI("%s ethermind connected, ATT_CON_ID %d atype %d addr 0x%02X:%02X:%02X:%02X:%02X:%02X\n",
                 __func__, conn_att->conn_handle, conn_att->peer_addr_type,
                 conn_att->peer_addr[5], conn_att->peer_addr[4],
                 conn_att->peer_addr[3], conn_att->peer_addr[2],
                 conn_att->peer_addr[1], conn_att->peer_addr[0]);
            s_ethermind_att_handle = conn_att->conn_handle;

            if (rtos_is_timer_init(&s_gap_create_connection_timeout_tmr))
            {
                if (rtos_is_timer_running(&s_gap_create_connection_timeout_tmr))
                {
                    rtos_stop_timer(&s_gap_create_connection_timeout_tmr);
                }

                rtos_deinit_timer(&s_gap_create_connection_timeout_tmr);
            }

            if ((STATE_CONNECTINIG == g_peer_dev.state)
                && (!os_memcmp(g_peer_dev.bdaddr.addr, conn_att->peer_addr,
                               BK_BLE_GAP_BD_ADDR_LEN)))
            {
                g_peer_dev.state = STATE_CONNECTED;
            }


            break;
        }

        case BK_DM_BLE_EVENT_DISCONNECT:
        {
            {
                ble_conn_att_t *att_handle = (typeof(att_handle)) param;
                uint8_t index = 0;
                LOGI("%s ethermind disconnect reason %d attid %d atype %d addr 0x%02X:%02X:%02X:%02X:%02X:%02X\n",
                     __func__, att_handle->event_result, att_handle->conn_handle,
                     att_handle->peer_addr_type, att_handle->peer_addr[5],
                     att_handle->peer_addr[4], att_handle->peer_addr[3],
                     att_handle->peer_addr[2], att_handle->peer_addr[1],
                     att_handle->peer_addr[0]);

                os_memset(&s_ethermind_att_handle, 0, sizeof(s_ethermind_att_handle));

                if (0 == ethermind_find_index_by_info(&index, att_handle->conn_handle))
                {
                    ethermind_clean_info_by_index(index);
                }
            }

            break;
        }

#if 0
        case BK_DM_BLE_EVENT_CREATE_DB:
        {

            LOGI("%s ethermind BK_DM_BLE_EVENT_CREATE_DB ok\n", __func__);

            break;
        }

        case BK_DM_BLE_EVENT_READ_PHY:
        {
            s_ble_phy = *(ble_read_phy_t *) param;
            LOGI("BK_DM_BLE_EVENT_READ_PHY:tx_phy:0x%02x, rx_phy:0x%02x\r\n", s_ble_phy.tx_phy, s_ble_phy.rx_phy);
            break;
        }

        case BK_DM_BLE_EVENT_TX_DONE:
        {
            ble_att_tx_compl_t *tx_comp = (typeof(tx_comp)) param;
            //        LOGI("%s BK_DM_BLE_EVENT_TX_DONE res %d ATT_CON_ID %d ATT_ATTR_HANDLE %d\n",
            //                  __func__, tx_comp->event_result, tx_comp->conn_handle,
            //                  tx_comp->attr_handle);

            if (s_ethermind_service_type == 1 && s_ethermind_auto_tx_enable)
            {
                //                for (uint8_t i = 0; i < a_wr_c->number_package_completed; ++i)
                //                {
                //                    ble_tx_test_active_timer_callback(NULL);
                //                }
                ble_tx_test_active_timer_callback(NULL);
            }
        }

        break;

        case BK_DM_BLE_EVENT_CONN_UPDATA:
        {
            {
                ble_conn_update_param_compl_ind_t *ind = (typeof(ind)) param;
                LOGI("%s BK_DM_BLE_EVENT_CONN_UPDATA mac 0x%02X:%02X:%02X:%02X:%02X:%02X status 0x%X interval 0x%X lantency 0x%X tout 0x%X\n",
                     __func__,
                     ind->peer_address.addr[5],
                     ind->peer_address.addr[4], ind->peer_address.addr[3],
                     ind->peer_address.addr[2], ind->peer_address.addr[1],
                     ind->peer_address.addr[0],
                     ind->status, ind->conn_interval,
                     ind->conn_latency, ind->supervision_timeout);

                if (ble_at_cmd_sema != NULL)
                {
                    rtos_set_semaphore(&ble_at_cmd_sema);
                }
            }

            break;
        }
#endif
        case BK_DM_BLE_EVENT_RECV_NOTIFY:
        {
            ble_att_notify_t *tmp = (typeof(tmp)) param;

            s_ethermind_performance_rx_bytes += tmp->len;

#ifdef CONFIG_ALI_MQTT

            if (loop_type == LT_COEX)
            {
                LOGI("ATT_CON_ID %d data %s len %d sendto mqtt\n",
                     tmp->conn_handle, tmp->data, tmp->len);

                ble_send_data_2_mqtt(tmp->conn_handle, tmp->len, tmp->data);
            }
            else
#endif
            {
                uint8_t index = 0;

                if (0 == ethermind_find_index_by_info(&index, tmp->conn_handle))
                {
                    LOGI("%s ATT_CON_ID %d, send data to nordic len %d %s\n",
                         __func__, tmp->conn_handle, tmp->len, tmp->data);

                    bk_ble_att_write(tmp->conn_handle,
                                     s_ethermind_nordic_write_attr_handle[index], tmp->data,
                                     tmp->len);
                }
                else
                {
                    //                LOGI("%s cant find info ATT_CON_ID %d\n", __func__, tmp->conn_handle);
                }
            }
        }
        break;
#if 0
        case BK_DM_BLE_EVENT_DISCOVERY_PRIMARY_SERVICE:
        {
            ble_discovery_primary_service_t *tmp = (typeof(tmp)) param;
            LOGI("%s BK_DM_BLE_EVENT_DISCOVERY_PRIMARY_SERVICE count %d\n",
                 __func__, tmp->count);

            for (uint8_t i = 0; i < tmp->count; i++)
            {
                //service's char must in this range
                LOGI("%s service uuid 0x%X start %d end %d\n", __func__,
                     tmp->service[i].uuid.uuid_16,
                     tmp->service[i].range.start_handle,
                     tmp->service[i].range.end_handle);

            }
        }
        break;

        case BK_DM_BLE_EVENT_DISCOVERY_CHAR:
        {
            ble_discovery_char_t *tmp = (typeof(tmp)) param;
            uint8_t index = 0;
            uint8_t found = 0;
            uint8_t i = 0;

            LOGI("%s BK_DM_BLE_EVENT_DISCOVERY_CHAR count %d ATT_CON_ID %d\n",
                 __func__, tmp->count, tmp->conn_handle);

            //

            for (i = 0; i < tmp->count; ++i)
            {
                //LOGI("%s type %d %p\n", __func__, tmp->character[7].uuid_type, &tmp->character[7].uuid_type);

                switch (tmp->character[i].uuid_type)
                {
                    case ATT_16_BIT_UUID_FORMAT:
                        LOGI("%s char 16bit uuid 0x%04X, attr_handle %d\n",
                             __func__, tmp->character[i].uuid.uuid_16,
                             tmp->character[i].value_handle);
                        break;

                    case ATT_128_BIT_UUID_FORMAT:
                    {
                        //LOGI("%s char 128bit uuid 0x%08X\n", __func__, (uint32_t *)(tmp->character[i].uuid.uuid_128.value + 11));

                        if (!os_memcmp(&tmp->character[i].uuid.uuid_128.value,
                                       nus_rx_uuid, sizeof(nus_rx_uuid)))
                        {
                            found = 1;
                            i = tmp->count - 1;
                            break;
                        }
                    }
                    break;
                }
            }

            if (!found)
            {
                break;
            }

            if (0 == ethermind_find_index_by_info(&index, tmp->conn_handle))
            {
                LOGI("%s nordic info already exist ! att_id %d\n", __func__,
                     tmp->conn_handle);
            }
            else if (0 == ethermind_find_idle_info_index(&index))
            {
            }
            else
            {
                LOGI("%s nordic info list is full !\n", __func__);
                break;
            }

            for (i = 0; i < tmp->count; ++i)
            {
                //LOGI("%s type %d %p\n", __func__, tmp->character[7].uuid_type, &tmp->character[7].uuid_type);

                switch (tmp->character[i].uuid_type)
                {
                    case ATT_16_BIT_UUID_FORMAT:
                        break;

                    case ATT_128_BIT_UUID_FORMAT:
                    {
                        LOGI("%s char 128bit uuid 0x%08X attr_hdl:%d\n", __func__,
                             (uint32_t *)(tmp->character[i].uuid.uuid_128.value + 11),
                             tmp->character[i].value_handle);

                        if (!os_memcmp(&tmp->character[i].uuid.uuid_128.value,
                                       nus_rx_uuid, sizeof(nus_rx_uuid)))
                        {
                            LOGI("%s add to nordic info list %d\n", __func__,
                                 index);
                            s_ethermind_nordic_used[index] = 1;
                            s_ethermind_nordic_write_attr_handle[index] =
                                tmp->character[i].value_handle;
                            s_ethermind_nordic_att_info[index] = tmp->conn_handle;
                        }
                        else if (!os_memcmp(&tmp->character[i].uuid.uuid_128.value,
                                            nus_tx_uuid, sizeof(nus_tx_uuid)))
                        {
                            if (tmp->character[i].desc_index)
                            {
                                for (uint8_t j = 0; j < tmp->character[i].desc_index;
                                     ++j)
                                {
                                    if (ATT_16_BIT_UUID_FORMAT
                                        == tmp->character[i].descriptor[j].uuid_type
                                        && GATT_CLIENT_CONFIG
                                        == tmp->character[i].descriptor[j].uuid.uuid_16)
                                    {
                                        s_ethermind_nordic_write_notify_handle[index] =
                                            tmp->character[i].descriptor[j].handle;
                                        //                                const uint16_t noti_enable = 0x0001;
                                        //                                LOGI("%s write nordic enable notify\n", __func__);
                                        //                                bk_ble_att_write(&tmp->att_handle, tmp->character[i].descriptor[j].handle, (uint8_t *)&noti_enable, sizeof(noti_enable));
                                    }
                                }
                            }
                        }
                    }
                    break;

                    default:
                        LOGI("%s unknow uuid type %d %d %p\n", __func__,
                             tmp->character[i].uuid_type, i,
                             &tmp->character[i].uuid_type);
                        break;
                }
            }

        }
        break;

        case BK_DM_BLE_EVENT_CONN_UPD_PAR_ASK:
        {
            {
                ble_conn_update_param_ind_t *ind = (typeof(ind)) param;
                LOGI(
                    "%s BK_DM_BLE_EVENT_CONN_UPD_PAR_ASK 0x%02X:%02X:%02X:%02X:%02X:%02X imin 0x%X imax 0x%x latency 0x%X tout 0x%X\n",
                    __func__, ind->peer_address.addr[5],
                    ind->peer_address.addr[4], ind->peer_address.addr[3],
                    ind->peer_address.addr[2], ind->peer_address.addr[1],
                    ind->peer_address.addr[0], ind->conn_interval_min,
                    ind->conn_interval_max, ind->conn_latency,
                    ind->supervision_timeout);

                //            ind->conn_interval_min++;
                //            ind->conn_interval_max++;
                ind->is_agree = 1;
            }
        }
        break;

        case BK_DM_BLE_EVENT_ATT_READ_RESPONSE:
        {
            ble_att_rw_t *tmp = (typeof(tmp)) param;

            if (!tmp->event_result)
            {
                LOGI("%s att_id:%d, attr_hand:0x%x, result:%d\r\n", __func__,
                     tmp->conn_handle, s_read_tmp_attr_handle,
                     tmp->event_result);
                LOGI("data-> ");

                for (int i = 0; i < tmp->len; i++)
                {
                    LOGI("0x%x ", *tmp->data);
                }

                LOGI("\r\n");

                if (ble_at_cmd_sema != NULL)
                {
                    rtos_set_semaphore(&ble_at_cmd_sema);
                }
            }
            else
            {
                LOGI(
                    "%s: Recevied Error Response!!! att_id:%d, attr_hand:%d, resp code:%d",
                    __func__, tmp->conn_handle,
                    ((uint8_t *)(&(tmp->event_result)))[1],
                    ((uint8_t *)(&(tmp->event_result)))[0]);
            }
        }
        break;
#endif
        default:
            break;
    }

    return 0;
}


static void dm_ble_gap_at_cb(bk_ble_gap_cb_event_t event, bk_ble_gap_cb_param_t *param)
{
    switch (event)
    {
        case BK_BLE_GAP_EXT_ADV_REPORT_EVT:
        {
            struct ble_ext_adv_report_param *pm = (typeof(pm))param;
            char printf_buff[128] = {0};
            uint32_t index = 0;
            char dname[64] = { 0 };
            uint8_t dname_len = sizeof(dname);

            ble_get_device_name_from_adv(pm->params.adv_data, pm->params.adv_data_len, (uint8_t *) dname, &dname_len);

            index += os_snprintf(printf_buff + index, sizeof(printf_buff) - index,
                                 pm->params.event_type & (1 << 4) ? "evt_type:LEG_ADV, 0x%x" : "evt_type:EXT_ADV, 0x%x", pm->params.event_type);

            index += os_snprintf(printf_buff + index, sizeof(printf_buff) - index,
                                 " adv_addr_type:%d, adv_addr:%02x:%02x:%02x:%02x:%02x:%02x name %s",
                                 pm->params.addr_type,
                                 pm->params.addr[0],
                                 pm->params.addr[1],
                                 pm->params.addr[2],
                                 pm->params.addr[3],
                                 pm->params.addr[4],
                                 pm->params.addr[5],
                                 dname);

            printf_buff[sizeof(printf_buff) - 1] = 0;
            LOGI("%s\n", printf_buff);
        }
        break;

        case BK_BLE_GAP_SET_STATIC_RAND_ADDR_EVT:
        {
            struct ble_set_rand_cmpl_evt_param *pm = (typeof(pm))param;

            if (pm->status)
            {
                bt_at_loge("set rand addr err 0x%x", pm->status);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        case BK_BLE_GAP_EXT_SCAN_PARAMS_SET_COMPLETE_EVT:
        {
            struct ble_scan_params_set_cmpl_param *pm = (typeof(pm))param;

            at_cmd_status = pm->status;
            if (pm->status)
            {
                bt_at_loge("set scan param err 0x%x", pm->status);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        case BK_BLE_GAP_EXT_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        {
            struct ble_adv_scan_rsp_set_cmpl_evt_param *pm = (typeof(pm))param;

            at_cmd_status = pm->status;
            if (pm->status)
            {
                bt_at_loge("set scan rsp data err 0x%x", pm->status);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        case BK_BLE_GAP_EXT_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        {
            struct ble_scan_rsp_data_raw_set_cmpl_evt_param *pm = (typeof(pm))param;

            at_cmd_status = pm->status;
            if (pm->status)
            {
                bt_at_loge("set scan rsp raw data err 0x%x", pm->status);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        case BK_BLE_GAP_EXT_SCAN_START_COMPLETE_EVT:
        {
            struct ble_scan_start_cmpl_param *pm = (typeof(pm))param;

            at_cmd_status = pm->status;
            if (pm->status)
            {
                bt_at_loge("set scan enable err 0x%x", pm->status);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        case BK_BLE_GAP_EXT_SCAN_STOP_COMPLETE_EVT:
        {
            struct ble_scan_stop_cmpl_param *pm = (typeof(pm))param;

            at_cmd_status = pm->status;
            if (pm->status)
            {
                bt_at_loge("set scan disable err 0x%x", pm->status);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        case BK_BLE_GAP_EXT_ADV_DATA_SET_COMPLETE_EVT:
        {
            struct ble_adv_data_set_cmpl_evt_param *pm = (typeof(pm))param;

            at_cmd_status = pm->status;
            if (pm->status)
            {
                bt_at_loge("set adv data err 0x%x", pm->status);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }

        }
        break;

        case BK_BLE_GAP_EXT_ADV_DATA_RAW_SET_COMPLETE_EVT:
        {
            struct ble_adv_data_raw_set_cmpl_evt_param *pm = (typeof(pm))param;

            at_cmd_status = pm->status;
            if (pm->status)
            {
                bt_at_loge("set adv raw data err 0x%x", pm->status);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        case BK_BLE_GAP_EXT_ADV_PARAMS_SET_COMPLETE_EVT:
        {
            struct ble_adv_params_set_cmpl_evt_param *pm = (typeof(pm))param;

            at_cmd_status = pm->status;
            if (pm->status)
            {
                bt_at_loge("set adv param err 0x%x", pm->status);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        case BK_BLE_GAP_EXT_ADV_START_COMPLETE_EVT:
        {
            struct ble_adv_start_cmpl_evt_param *pm = (typeof(pm))param;

            at_cmd_status = pm->status;
            if (pm->status)
            {
                bt_at_loge("set adv enable err 0x%x", pm->status);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        case BK_BLE_GAP_EXT_ADV_STOP_COMPLETE_EVT:
        {
            struct ble_adv_stop_cmpl_evt_param *pm = (typeof(pm))param;

            at_cmd_status = pm->status;
            if (pm->status)
            {
                bt_at_loge("set adv disable err 0x%x", pm->status);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        case BK_BLE_GAP_PERIODIC_ADV_SET_PARAMS_COMPLETE_EVT:
        {
            struct ble_periodic_adv_set_params_cmpl_param *pm = (typeof(pm))param;

            at_cmd_status = pm->status;
            if (pm->status)
            {
                bt_at_loge("set periodic adv param err 0x%x", pm->status);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        case BK_BLE_GAP_PERIODIC_ADV_DATA_SET_COMPLETE_EVT:
        {
            struct ble_periodic_adv_data_set_cmpl_param *pm = (typeof(pm))param;

            at_cmd_status = pm->status;
            if (pm->status)
            {
                bt_at_loge("set periodic adv data err 0x%x", pm->status);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        case BK_BLE_GAP_PERIODIC_ADV_START_COMPLETE_EVT:
        {
            struct ble_periodic_adv_start_cmpl_param *pm = (typeof(pm))param;

            at_cmd_status = pm->status;
            if (pm->status)
            {
                bt_at_loge("set periodic adv enable err 0x%x", pm->status);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        case BK_BLE_GAP_PERIODIC_ADV_STOP_COMPLETE_EVT:
        {
            struct ble_periodic_adv_stop_cmpl_param *pm = (typeof(pm))param;

            at_cmd_status = pm->status;
            if (pm->status)
            {
                bt_at_loge("set periodic adv disable err 0x%x", pm->status);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        case BK_BLE_GAP_UPDATE_CONN_PARAMS_EVT:
        {
            struct ble_update_conn_params_evt_param *pm = (typeof(pm))param;

            at_cmd_status = pm->status;
            if (pm->status)
            {
                bt_at_loge("update conn param err 0x%x", pm->status);
            }
            else
            {
                bt_at_logi("BK_BLE_GAP_UPDATE_CONN_PARAMS_EVT mac 0x%02X:%02X:%02X:%02X:%02X:%02X status 0x%X interval 0x%X lantency 0x%X tout 0x%X",
                           pm->bda[5],
                           pm->bda[4],
                           pm->bda[3],
                           pm->bda[2],
                           pm->bda[1],
                           pm->bda[0],
                           pm->status,
                           pm->conn_int,
                           pm->latency,
                           pm->timeout);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        case BK_BLE_GAP_READ_PHY_COMPLETE_EVT:
        {
            struct ble_read_phy_cmpl_evt_param *pm = (typeof(pm))param;

            at_cmd_status = pm->status;
            if (pm->status)
            {
                bt_at_loge("read phy err 0x%x", pm->status);
            }
            else
            {
                bt_at_logi("read phy 0x%x 0x%x", pm->rx_phy, pm->tx_phy);
            }

            if (ble_at_cmd_sema != NULL)
            {
                rtos_set_semaphore(&ble_at_cmd_sema);
            }
        }
        break;

        default:
            break;
    }
}


//AT+BLE_BLENAME=? //AT+BLE_BLENAME=<param1>
int set_ble_name_handle(int sync, int argc, char **argv)
{
    //uint8_t name[] = "BK_BLE_7231n";
    uint8_t name_len = 0;/*os_strlen((const char *)name);*/
    //#ifdef CONFIG_BT  /*unused*/
    //    uint8_t name[BK_BLE_APP_DEVICE_NAME_MAX_LEN] = {0};
    //#endif
    int err = kNoErr;
    if (argc != 1)
    {
        err = kParamErr;
        goto error;
    }
#ifdef CONFIG_BT
    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
#endif
        name_len = bk_ble_appm_set_dev_name(os_strlen(argv[0]), (uint8_t *)argv[0]);
        if (name_len == 0)
        {
            LOGE("\nname is empty!!!\n");
            goto error;
        }
        atsvr_cmd_rsp_ok();
        return kNoErr;
#ifdef CONFIG_BT
    }
    else
    {
        return set_ble_device_name_handle_gap(sync, argc, argv);
    }
#endif

error:
    atsvr_cmd_rsp_error();
    return err;
}

int get_ble_name_handle(int sync, int argc, char **argv)
{
    uint8_t name[BK_BLE_APP_DEVICE_NAME_MAX_LEN] = {0};
    uint8_t name_len = 0;
    char resultbuf[200];
    int err = kNoErr;
    if (argc != 0)
    {
        err = kParamErr;
        goto error;
    }
#ifdef CONFIG_BT
    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
#endif
        name_len = bk_ble_appm_get_dev_name(name, BK_BLE_APP_DEVICE_NAME_MAX_LEN);
        if (name_len == 0)
        {
            LOGE("\nname is empty!!!\n");
            goto error;
        }

        os_snprintf(resultbuf, sizeof(resultbuf), "%s:%s\r\n", "BLENAME", name);
        atsvr_output_msg(resultbuf);
        atsvr_cmd_rsp_ok();

        return kNoErr;
#ifdef CONFIG_BT
    }
    else
    {
        return get_ble_device_name_handle_gap(sync, argc, argv);

    }
#endif

error:
    atsvr_cmd_rsp_error();
    return err;
}

//AT+BLECMD=SETADVPARAM, map, min_intval, max_intval, local_addr_type, adv_type, adv_properties, prim_phy, second_phy
int ble_set_adv_param_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    int actv_idx = 0;
    ble_adv_param_t adv_param;

    if (argc != 8)
    {
        LOGE("\nThe count of param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    os_memset(&adv_param, 0, sizeof(ble_adv_param_t));
    adv_param.chnl_map = os_strtoul(argv[0], NULL, 16);
    if (adv_param.chnl_map > 7)
    {
        LOGE("\nThe first(channel_map) param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    adv_param.adv_intv_min = os_strtoul(argv[1], NULL, 16) & 0xFFFFFF;
    adv_param.adv_intv_max = os_strtoul(argv[2], NULL, 16) & 0xFFFFFF;
    if ((adv_param.adv_intv_min > ADV_INTERVAL_MAX || adv_param.adv_intv_min < ADV_INTERVAL_MIN)
        || (adv_param.adv_intv_max > ADV_INTERVAL_MAX || adv_param.adv_intv_max < ADV_INTERVAL_MIN)
        || (adv_param.adv_intv_min > adv_param.adv_intv_max))
    {
        LOGE("input param interval is error\n");
        err = kParamErr;
        goto error;
    }

    adv_param.own_addr_type = os_strtoul(argv[3], NULL, 16) & 0xFF;
    adv_param.adv_type = os_strtoul(argv[4], NULL, 16) & 0xFF;

    if (adv_param.adv_type > 2)
    {
        LOGE("\nThe forth(adv_type) param is wrong!\n");
        err = kParamErr;
        goto error;
    }
    adv_param.adv_prop = os_strtoul(argv[5], NULL, 16) & 0xFFFF;
    adv_param.prim_phy = os_strtoul(argv[6], NULL, 16) & 0xFF;
    if (!(adv_param.prim_phy == 1 || adv_param.prim_phy == 3))
    {
        LOGE("input param prim_phy is error\n");
        err = kParamErr;
        goto error;
    }

    adv_param.second_phy = os_strtoul(argv[7], NULL, 16) & 0xFF;
    if (adv_param.second_phy < 1 || adv_param.second_phy > 3)
    {
        LOGE("input param second_phy is error\n");
        err = kParamErr;
        goto error;
    }

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        switch (adv_param.own_addr_type)
        {
            case 0:
            case 1:
                adv_param.own_addr_type = OWN_ADDR_TYPE_PUBLIC_ADDR;
                break;
            case 2:
                adv_param.own_addr_type = OWN_ADDR_TYPE_GEN_RSLV_OR_RANDOM_ADDR;
                break;
            case 3:
                adv_param.own_addr_type = OWN_ADDR_TYPE_GEN_NON_RSLV_OR_RANDOM_ADDR;
                break;
            default:
                LOGE("\nThe third(own_addr_type) param is wrong!\n");
                err = kParamErr;
                break;
        }

        if (err != kNoErr)
        {
            goto error;
        }

        actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_CREATED);
        if (actv_idx == AT_BLE_MAX_ACTV)
        {
            actv_idx = bk_ble_get_idle_actv_idx_handle();
            if (actv_idx == UNKNOW_ACT_IDX)
            {
                err = kNoResourcesErr;
                goto error;
            }
        }

        err = bk_ble_create_advertising(actv_idx, &adv_param, ble_at_cmd_cb);
        if (err != BK_ERR_BLE_SUCCESS)
        {
            goto error;
        }
        if (ble_at_cmd_sema != NULL)
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                goto error;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    atsvr_cmd_rsp_ok();
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return 0;
                }
                else
                {
                    err = at_cmd_status;
                    goto error;
                }
            }
        }
    }
    else
    {
        if (ADV_TYPE_PERIODIC == adv_param.adv_type)
        {
            return ble_set_per_adv_param_handle_gap(sync, argc, argv);
        }
        else
        {
            return ble_set_adv_param_handle_gap(sync, argc, argv);
        }

    }
error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}


//AT+BLE_SETADVDATA=<data>,<length>
int ble_set_adv_data_handle(int sync, int argc, char **argv)
{
    uint8_t adv_data[255];
    uint8_t adv_len = 0;
    int actv_idx = -1;
    int err = kNoErr;

    if (argc != 2)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    adv_len = os_strtoul(argv[1], NULL, 16) & 0xFF;
    if (adv_len > 255 || adv_len != os_strlen(argv[0]) / 2)
    {
        LOGE("input adv len over limited\n");
        err = kParamErr;
        goto error;
    }

    at_set_data_handle(adv_data, argv[0],  os_strlen(argv[0]));

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_CREATED);

        if (actv_idx == AT_BLE_MAX_ACTV)
        {
            actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_STARTED);
            if (actv_idx == AT_BLE_MAX_ACTV)
            {
                LOGE("ble adv not set params before\n");
                err = kNoResourcesErr;
                goto error;
            }
        }

        err = bk_ble_set_adv_data(actv_idx, adv_data, adv_len, ble_at_cmd_cb);
        if (err != BK_ERR_BLE_SUCCESS)
        {
            goto error;
        }
        if (ble_at_cmd_sema != NULL)
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                goto error;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    atsvr_cmd_rsp_ok();
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return 0;
                }
                else
                {
                    err = at_cmd_status;
                    goto error;
                }
            }
        }
    }
    else
    {
        return ble_set_adv_data_raw_handle_gap(sync, argc, argv);
    }
error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

//AT+BLECMD=SETPERADVDATA,data,length
int ble_set_per_adv_data_handle(int sync, int argc, char **argv)
{
    uint8_t adv_data[255];
    uint8_t adv_len = 0;
    int actv_idx = -1;
    int err = kNoErr;

    if (argc != 2)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    adv_len = os_strtoul(argv[1], NULL, 16) & 0xFF;
    if (adv_len > 255 || adv_len != os_strlen(argv[0]) / 2)
    {
        LOGE("input adv len over limited\n");
        err = kParamErr;
        goto error;
    }

    at_set_data_handle(adv_data, argv[0],  os_strlen(argv[0]));

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_CREATED);
        if (actv_idx == AT_BLE_MAX_ACTV)
        {
            actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_STARTED);
            if (actv_idx == AT_BLE_MAX_ACTV)
            {
                BK_LOGE(TAG, "ble adv not set params before\n");
                err = kNoResourcesErr;
                goto error;
            }
        }

        err = bk_ble_set_per_adv_data(actv_idx, adv_data, adv_len, ble_at_cmd_cb);
        if (err != BK_ERR_BLE_SUCCESS)
        {
            goto error;
        }
    }
    else
    {
        return ble_set_per_adv_data_handle_gap(sync, argc, argv);
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

int ble_set_scan_rsp_data_handle(int sync, int argc, char **argv)
{
    uint8_t scan_rsp_data[255];
    uint8_t data_len = 0;
    int actv_idx = -1;
    int err = kNoErr;

    if (argc != 2)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    data_len = os_strtoul(argv[1], NULL, 16) & 0xFF;
    if (data_len > 255 || data_len != os_strlen(argv[0]) / 2)
    {
        LOGE("input adv len over limited\n");
        err = kParamErr;
        goto error;
    }

    at_set_data_handle(scan_rsp_data, argv[0], os_strlen(argv[0]));

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_CREATED);
        if (actv_idx == AT_BLE_MAX_ACTV)
        {
            actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_STARTED);
            if (actv_idx == AT_BLE_MAX_ACTV)
            {
                LOGE("ble adv not set params before\n");
                err = kNoResourcesErr;
                goto error;
            }
        }

        err = bk_ble_set_scan_rsp_data(actv_idx, scan_rsp_data, data_len, ble_at_cmd_cb);
        if (err != BK_ERR_BLE_SUCCESS)
        {
            goto error;
        }
        if (ble_at_cmd_sema != NULL)
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                goto error;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    atsvr_cmd_rsp_ok();
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return 0;
                }
                else
                {
                    err = at_cmd_status;
                    goto error;
                }
            }
        }
    }
    else
    {
        return ble_set_scan_rsp_data_raw_handle_gap(sync, argc, argv);
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;

}

int ble_set_adv_enable_handle(int sync, int argc, char **argv)
{
    int actv_idx = -1;
    int enable = 0;
    int err = kNoErr;

    if (argc != 1)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    if (os_strcmp(argv[0], "1") == 0)
    {
        enable = 1;
    }
    else if (os_strcmp(argv[0], "0") == 0)
    {
        enable = 0;
    }
    else
    {
        LOGE("the input param is error\n");
        err = kParamErr;
        goto error;
    }

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }


    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_ble_set_notice_cb(ble_at_notice_cb);
        if (enable == 1)
        {
            actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_CREATED);
        }
        else
        {
            actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_STARTED);
        }

        if (actv_idx == AT_BLE_MAX_ACTV)
        {
            LOGE("ble adv not set params before\n");
            err = kNoResourcesErr;
            goto error;
        }

        if (enable == 1)
        {
            err = bk_ble_start_advertising(actv_idx, 0, ble_at_cmd_cb);
        }
        else
        {
            err = bk_ble_stop_advertising(actv_idx, ble_at_cmd_cb);
        }

        if (err != BK_ERR_BLE_SUCCESS)
        {
            goto error;
        }
        if (ble_at_cmd_sema != NULL)
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                goto error;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    atsvr_cmd_rsp_ok();
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return 0;
                }
                else
                {
                    err = at_cmd_status;
                    goto error;
                }
            }
        }
    }
    else
    {
        if (s_is_adv_set_periodic)
        {
            return ble_set_per_adv_enable_handle_gap(sync, argc, argv);
        }
        else
        {
            return ble_set_adv_enable_handle_gap(sync, argc, argv);
        }

    }
error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

int ble_set_scan_param_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    uint8_t actv_idx = 0;
    ble_scan_param_t scan_param;

    if (argc < 4)
    {
        LOGE("\nThe number of param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    os_memset(&scan_param, 0, sizeof(ble_scan_param_t));
    scan_param.own_addr_type = os_strtoul(argv[0], NULL, 16) & 0xFF;
    scan_param.scan_phy = os_strtoul(argv[1], NULL, 16) & 0xFF;

    if (!(scan_param.scan_phy & (PHY_1MBPS_BIT | PHY_CODED_BIT)))
    {
        LOGE("\nThe scan phy param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    scan_param.scan_intv = os_strtoul(argv[2], NULL, 16) & 0xFFFF;
    scan_param.scan_wd = os_strtoul(argv[3], NULL, 16) & 0xFFFF;

    if (argc >= 5)
    {
        scan_param.scan_type = os_strtoul(argv[4], NULL, 16) & 0xFF;
    }
    if(argc >= 6)
    {
        uint8_t filter_policy = os_strtoul(argv[5], NULL, 10);
        LOGI("set scan filter %d \n", filter_policy);
        if(filter_policy == 0)
        {
            scan_param.scan_filter = BASIC_UNFILTER_SCAN_POLICY;
        }else
        {
            scan_param.scan_filter = BASIC_FILTER_SCAN_POLICY;
        }
    }
    if (scan_param.scan_intv < SCAN_INTERVAL_MIN || scan_param.scan_intv > SCAN_INTERVAL_MAX ||
        scan_param.scan_wd < SCAN_WINDOW_MIN || scan_param.scan_wd > SCAN_WINDOW_MAX ||
        scan_param.scan_intv < scan_param.scan_wd)
    {
        LOGE("\nThe second/third param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        switch (scan_param.own_addr_type)
        {
            case 0:
            case 1:
                scan_param.own_addr_type = OWN_ADDR_TYPE_PUBLIC_ADDR;
                break;
            case 2:
                scan_param.own_addr_type = OWN_ADDR_TYPE_GEN_RSLV_OR_RANDOM_ADDR;
                break;
            case 3:
                scan_param.own_addr_type = OWN_ADDR_TYPE_GEN_NON_RSLV_OR_RANDOM_ADDR;
                break;
            default:
                LOGE("\nThe fourth param is wrong!\n");
                err = kParamErr;
                break;
        }

        if (err != kNoErr)
        {
            goto error;
        }

        bk_ble_set_notice_cb(ble_at_notice_cb);
        actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_SCAN_CREATED);
        if (actv_idx == AT_BLE_MAX_ACTV)
        {
            actv_idx = bk_ble_get_idle_actv_idx_handle();
            if (actv_idx == UNKNOW_ACT_IDX)
            {
                err = kNoResourcesErr;
                goto error;
            }
        }

        err = bk_ble_create_scaning(actv_idx, &scan_param, ble_at_cmd_cb);

        if (err != BK_ERR_BLE_SUCCESS)
        {
            goto error;
        }
        if (ble_at_cmd_sema != NULL)
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                goto error;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    atsvr_cmd_rsp_ok();
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return err;
                }
                else
                {
                    err = at_cmd_status;
                    goto error;
                }
            }
        }
    }
    else
    {
        return ble_set_scan_param_handle_gap(sync, argc, argv);
    }
error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

int ble_set_scan_enable_handle(int sync, int argc, char **argv)
{
    uint8_t actv_idx = 0;
    int enable = 0;
    int err = kNoErr;

    uint8_t filt_duplicate = 0;
    uint16_t duration = 0;
    uint16_t period = 10;

    if (argc < 1)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    if (os_strcmp(argv[0], "1") == 0)
    {
        enable = 1;
    }
    else if (os_strcmp(argv[0], "0") == 0)
    {
        enable = 0;
    }
    else
    {
        LOGE("the input param is error\n");
        err = kParamErr;
        goto error;
    }

    if (enable)
    {
        if (argc == 1)
        {

        }
        else if (argc >= 1 + 3)
        {
            filt_duplicate = os_strtoul(argv[1], NULL, 16) & 0xFF;
            duration = os_strtoul(argv[2], NULL, 16) & 0xFFFF;
            period = os_strtoul(argv[3], NULL, 16) & 0xFFFF;
        }
        else
        {
            LOGE("the input param is error\n");
            err = kParamErr;
            goto error;
        }
    }


    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }


    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_ble_set_notice_cb(ble_at_notice_cb);
        if (enable == 1)
        {
            actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_SCAN_CREATED);
        }
        else
        {
            actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_SCAN_STARTED);
        }

        if (actv_idx == AT_BLE_MAX_ACTV)
        {
            LOGE("scan actv not start before\n");
            err = kNoResourcesErr;
            goto error;
        }

        if (enable == 1)
        {
            err = bk_ble_start_scaning_ex(actv_idx, filt_duplicate, duration, period, ble_at_cmd_cb);
        }
        else
        {
            err = bk_ble_stop_scaning(actv_idx, ble_at_cmd_cb);
        }

        if (err != BK_ERR_BLE_SUCCESS)
        {
            goto error;
        }
        if (ble_at_cmd_sema != NULL)
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                goto error;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    atsvr_cmd_rsp_ok();
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return 0;
                }
                else
                {
                    err = at_cmd_status;
                    goto error;
                }
            }
        }
    }
    else
    {
        return ble_set_scan_enable_handle_gap(sync, argc, argv);
    }
error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

static void ble_connect_sdp_comm_callback(MASTER_COMMON_TYPE type, uint8 conidx, void *param)
{
    if (MST_TYPE_SVR_UUID == type)
    {
        struct ble_sdp_svc_ind *svc_inf = (struct ble_sdp_svc_ind *)param;
        LOGI("Service UUID: 0x%02x%02x\r\n", svc_inf->uuid[1], svc_inf->uuid[0]);
    }

    if (MST_TYPE_ATT_UUID == type)
    {
        struct ble_sdp_char_inf *char_inf = (struct ble_sdp_char_inf *)param;
        if (!os_memcmp(char_inf->uuid, device_name_uuid, 2))
        {
            s_device_name_hdl = char_inf->val_hdl;
        }
        LOGI("Charac UUID: 0x%02x%02x, val_hdl: %d\r\n", char_inf->uuid[1], char_inf->uuid[0], char_inf->val_hdl);
    }

    switch (type)
    {
        case MST_TYPE_DISCOVER_PRI_SERVICE_RSP:
        case MST_TYPE_DISCOVER_PRI_SERVICE_BY_UUID_RSP:
        case MST_TYPE_DISCOVER_PRI_SERVICE_BY_128_UUID_RSP:
        {
            struct ble_sdp_svc_ind *srv_ind = (typeof(srv_ind))param;
            uint16_t uuid = 0;

            if (srv_ind->uuid_len == 16)
            {
                ble_convert_128b_2_16b_uuid(srv_ind->uuid, &uuid);
            }
            else
            {
                os_memcpy(&uuid, srv_ind->uuid, sizeof(uuid));
            }

            LOGI("service rsp uuid 0x%04x sh 0x%04x eh 0x%04x len %d\n", uuid, srv_ind->start_hdl, srv_ind->end_hdl, srv_ind->uuid_len);
        }
        break;

        case MST_TYPE_DISCOVER_CHAR_RSP:
        case MST_TYPE_DISCOVER_CHAR_BY_UUID_RSP:
        case MST_TYPE_DISCOVER_CHAR_BY_128_UUID_RSP:
        {
            struct ble_sdp_char_inf *char_inf = (typeof(char_inf))param;
            uint16_t uuid = 0;

            if (char_inf->uuid_len == 16)
            {
                ble_convert_128b_2_16b_uuid(char_inf->uuid, &uuid);
            }
            else
            {
                os_memcpy(&uuid, char_inf->uuid, sizeof(uuid));
            }

            LOGI("char rsp uuid 0x%04x decl handle 0x%04x value handle 0x%04x prop 0x%x len %d\n", uuid, char_inf->char_hdl, char_inf->val_hdl, char_inf->prop, char_inf->uuid_len);
        }
        break;

        case MST_TYPE_DISCOVER_CHAR_DESC:
        {
            struct ble_sdp_char_desc_inf *desc_inf = (typeof(desc_inf))param;
            uint16_t uuid = 0;

            if (desc_inf->uuid_len == 16)
            {
                ble_convert_128b_2_16b_uuid(desc_inf->uuid, &uuid);
            }
            else
            {
                os_memcpy(&uuid, desc_inf->uuid, sizeof(uuid));
            }

            LOGI("char desc rsp uuid 0x%04x handle 0x%04x len %d\n", uuid, desc_inf->desc_hdl, desc_inf->uuid_len);
        }
        break;

        case MST_TYPE_DISCOVER_COMPLETED:
        {
            struct ble_descover_complete_inf *desc_inf = (typeof(desc_inf))param;
            LOGI("discover complete action 0x%x con_id 0x%x\n", desc_inf->action_id, conidx);
        }
        break;
        default:
            break;
    }
}

int ble_start_connect_handle(uint8_t actv_idx, uint8_t peer_addr_type, bd_addr_t *bdaddr, ble_cmd_cb_t cb)
{
    int err = kNoErr;
    err = bk_ble_init_set_connect_dev_addr(actv_idx, bdaddr, peer_addr_type);
    if (err != 0)
    {
        return err;
    }
    err = bk_ble_init_start_conn(actv_idx, cb);
    if (err != 0)
    {
        return err;
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err == kNoErr)
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                return kNoErr;
            }
            else
            {
                err = at_cmd_status;
            }
        }
    }
    else
    {
        err = kNotFoundErr;
    }

    return err;
}

static void ble_gap_create_connection_timerout_timer_hdl(void *param)
{
    int32_t err = 0;

    LOGE("%s connect time out, try cancel\n", __func__, err);

    err = bk_ble_cancel_connect(ble_at_cmd_cb);

    if (err != BK_ERR_BLE_SUCCESS)
    {
        LOGE("%s cancel connect fail %d\n", __func__, err);
    }

    if (rtos_is_timer_init(&s_gap_create_connection_timeout_tmr))
    {
        if (rtos_is_timer_running(&s_gap_create_connection_timeout_tmr))
        {
            rtos_stop_timer(&s_gap_create_connection_timeout_tmr);
        }

        rtos_deinit_timer(&s_gap_create_connection_timeout_tmr);
    }
}


//AT+BLE_CREATECONNECT,<Param1>,...,<Param6>
int ble_create_connect_handle(int sync, int argc, char **argv)
{
    uint8_t actv_idx = 0;
    ble_conn_param_t conn_param;
    uint8_t peer_addr_type = 0;
    bd_addr_t bdaddr;
    int err = kNoErr;
    uint8_t central_count = 0;

    if (!bk_ble_if_support_central(&central_count) || central_count == 0)
    {
        LOGE("not support central\n");
        err = kParamErr;
        goto error;
    }


    if (argc != 6)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    conn_param.intv_max = conn_param.intv_min = os_strtoul(argv[0], NULL, 16) & 0xFFFF;
    conn_param.con_latency = os_strtoul(argv[1], NULL, 16) & 0xFFFF;
    if (conn_param.intv_min < CON_INTERVAL_MIN || conn_param.intv_min > CON_INTERVAL_MAX
        || conn_param.con_latency > CON_LATENCY_MAX)
    {
        err = kParamErr;
        goto error;
    }

    conn_param.sup_to = os_strtoul(argv[2], NULL, 16) & 0xFFFF;
    if (conn_param.sup_to < CON_SUP_TO_MIN || conn_param.sup_to > CON_SUP_TO_MAX)
    {
        err = kParamErr;
        goto error;
    }

    conn_param.init_phys = os_strtoul(argv[3], NULL, 16) & 0xFF;
    if (conn_param.init_phys > (PHY_1MBPS_BIT | PHY_2MBPS_BIT | PHY_CODED_BIT))
    {
        err = kParamErr;
        goto error;
    }

    peer_addr_type = os_strtoul(argv[4], NULL, 16) & 0xFF;
    if (peer_addr_type > OWN_ADDR_TYPE_RANDOM_ADDR)
    {
        err = kParamErr;
        goto error;
    }

    err = get_addr_from_param(&bdaddr, argv[5]);
    if (err != kNoErr)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    if ((10 * conn_param.sup_to) < (((1 + conn_param.con_latency) * conn_param.intv_min * 5 + 1) >> 1))
    {
        LOGE("input param not suitable, maybe you can set con_latency to 0\n");
        err = kParamErr;
        goto error;
    }

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }



    /*actv_idx = bk_ble_find_master_state_idx_handle(AT_INIT_STATE_CREATED);
    if (actv_idx == AT_BLE_MAX_CONN)*/
    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_ble_set_notice_cb(ble_at_notice_cb);
        /// Do not create actv

        actv_idx = bk_ble_get_idle_conn_idx_handle();
        if (actv_idx == UNKNOW_ACT_IDX)
        {
            err = kNoResourcesErr;
            goto error;
        }

        err = bk_ble_create_init(actv_idx, &conn_param, ble_at_cmd_cb);

        if (err != BK_ERR_BLE_SUCCESS)
        {
            goto error;
        }
        if (ble_at_cmd_sema != NULL)
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                goto error;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    //bk_ble_register_app_sdp_common_callback(ble_connect_sdp_comm_callback);
                    err = ble_start_connect_handle(actv_idx, peer_addr_type, &bdaddr, ble_at_cmd_cb);
                    if (err != kNoErr)
                    {
                        goto error;
                    }
                    atsvr_cmd_rsp_ok();
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return err;
                }
                else
                {
                    err = at_cmd_status;
                    goto error;
                }
            }
        }
    }
    else
    {
        bk_ble_gap_register_callback(dm_ble_gap_at_cb);
        ble_at_gattc_reg();
        ble_conn_param_normal_t tmp;

        tmp.conn_interval_min = conn_param.intv_min;
        tmp.conn_interval_max = conn_param.intv_max;
        tmp.conn_latency = conn_param.con_latency;
        tmp.supervision_timeout = conn_param.sup_to;
        tmp.initiating_phys = conn_param.init_phys;

        tmp.peer_address_type = peer_addr_type;
        os_memcpy(tmp.peer_address.addr, bdaddr.addr, sizeof(bdaddr.addr));

        //todo: not impl bk_ble_create_connection in ethermind
        bk_ble_set_event_callback(dm_ble_at_event_cb);
        err = bk_ble_create_connection(&tmp, ble_at_cmd_cb);

        if (err != BK_ERR_BLE_SUCCESS)
        {
            goto error;
        }

        if (ble_at_cmd_sema != NULL)
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                goto error;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    if (!rtos_is_timer_init(&s_gap_create_connection_timeout_tmr))
                    {
                        rtos_init_timer(&s_gap_create_connection_timeout_tmr, 10 * 1000, ble_gap_create_connection_timerout_timer_hdl, (void *)0);
                        rtos_start_timer(&s_gap_create_connection_timeout_tmr);
                    }
                    atsvr_cmd_rsp_ok();
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return err;
                }
                else
                {
                    err = at_cmd_status;
                    goto error;
                }
            }
        }

    }


error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

int ble_cancel_create_connect_handle(int sync, int argc, char **argv)
{
    uint8_t actv_idx = 0;
    int err = kNoErr;
    uint8_t central_count = 0;

    if (!bk_ble_if_support_central(&central_count) || central_count == 0)
    {
        LOGE("not support central\n");
        err = kParamErr;
        goto error;
    }

    if (argc != 0)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        actv_idx = bk_ble_find_master_state_idx_handle(AT_INIT_STATE_CONNECTTING);
        if (actv_idx == AT_BLE_MAX_ACTV)
        {
            LOGE("ble adv not set params before\n");
            err = kNoResourcesErr;
            goto error;
        }

        err = bk_ble_init_stop_conn(actv_idx, ble_at_cmd_cb);
    }
    else
    {
        if (rtos_is_timer_init(&s_gap_create_connection_timeout_tmr))
        {
            if (rtos_is_timer_running(&s_gap_create_connection_timeout_tmr))
            {
                rtos_stop_timer(&s_gap_create_connection_timeout_tmr);
            }

            rtos_deinit_timer(&s_gap_create_connection_timeout_tmr);
        }
        err = bk_ble_cancel_connect(ble_at_cmd_cb);
    }
    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }
    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

int ble_disconnect_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    bd_addr_t connect_addr;
    uint8_t addr_type = 1;

    uint8_t conn_idx;
    uint8_t central_count = 0;

    if (!bk_ble_if_support_central(&central_count) || central_count == 0)
    {
        LOGE("not support central\n");
        err = kParamErr;
        goto error;
    }

    if (argc > 2)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    err = get_addr_from_param(&connect_addr, argv[0]);
    if (err != kNoErr)
    {
        LOGE("input addr param error\n");
        err = kParamErr;
        goto error;
    }

    if (argc < 2)
    {
        LOGW("%s warning, you need input addr type\n", __func__);
    }
    else
    {
        addr_type = os_strtoul(argv[1], NULL, 10) & 0xFF;
    }

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }

    /// get connect_idx from connect_addr
    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        conn_idx = bk_ble_find_conn_idx_from_addr(&connect_addr);
        if (conn_idx == AT_BLE_MAX_CONN)
        {
            LOGE("ble not connection\n");
            err = kNoResourcesErr;
            goto error;
        }
        bk_ble_set_notice_cb(ble_at_notice_cb);
        err = bk_ble_disconnect(conn_idx);
        g_peer_dev.state = STATE_DISCONNECTINIG;
    }
    else
    {
        bk_ble_gap_register_callback(dm_ble_gap_at_cb);
        ble_at_gattc_reg();
        err = bk_ble_disconnect_connection(&connect_addr, ble_at_cmd_cb);
    }

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }
    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_DISCON_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                g_peer_dev.state = STATE_IDLE;
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

    (void)addr_type;

error:
    atsvr_cmd_rsp_error();
    g_peer_dev.state = STATE_IDLE;
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

int ble_update_conn_param_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    bd_addr_t connect_addr;
    ble_conn_param_t conn_param;
    uint8_t conn_idx;
    uint8_t central_count = 0;
    uint8_t addr_type = 1;

    if (!bk_ble_if_support_central(&central_count) || central_count == 0)
    {
        LOGE("not support central\n");
        err = kParamErr;
        goto error;
    }

    if (argc > 6)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    err = get_addr_from_param(&connect_addr, argv[0]);
    if (err != kNoErr)
    {
        LOGE("input addr param error\n");
        err = kParamErr;
        goto error;
    }

    conn_param.intv_min = os_strtoul(argv[1], NULL, 16) & 0xFFFF;
    conn_param.intv_max = os_strtoul(argv[2], NULL, 16) & 0xFFFF;
    conn_param.con_latency = os_strtoul(argv[3], NULL, 16) & 0xFFFF;
    conn_param.sup_to = os_strtoul(argv[4], NULL, 16) & 0xFFFF;

    if (argc < 6)
    {
        LOGW("%s warning, you need input addr type\n", __func__);
    }
    else
    {
        addr_type = os_strtoul(argv[5], NULL, 10) & 0xFF;
    }

    if ((conn_param.intv_min < CON_INTERVAL_MIN || conn_param.intv_min > CON_INTERVAL_MAX) ||
        (conn_param.intv_max < CON_INTERVAL_MIN || conn_param.intv_max > CON_INTERVAL_MAX) ||
        (conn_param.intv_min > conn_param.intv_max) || (conn_param.con_latency > CON_LATENCY_MAX) ||
        (conn_param.sup_to < CON_SUP_TO_MIN || conn_param.sup_to > CON_SUP_TO_MAX))
    {
        LOGE("input update param not suitable\n");
        err = kParamErr;
        goto error;
    }

    if ((10 * conn_param.sup_to) < (((1 + conn_param.con_latency) * conn_param.intv_max * 5 + 1) >> 1))
    {
        LOGE("input param not suitable, maybe you can set con_latency to 0\n");
        err = kParamErr;
        goto error;
    }

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            LOGE("rtos_init_semaphore error\n");
            goto error;
        }
    }

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        /// get connect_idx from connect_addr
        conn_idx = bk_ble_find_conn_idx_from_addr(&connect_addr);
        if (conn_idx == AT_BLE_MAX_CONN)
        {
            LOGE("ble not connection\n");
            err = kNoResourcesErr;
            goto error;
        }
        bk_ble_set_notice_cb(ble_at_notice_cb);
        err = bk_ble_update_param(conn_idx, &conn_param);

    }
    else
    {
        //        ble_update_conn_param_t tmp;
        //        tmp.peer_address_type = addr_type;
        //
        //        tmp.conn_interval_min = conn_param.intv_min;
        //        tmp.conn_interval_max = conn_param.intv_max;
        //        tmp.conn_latency = conn_param.con_latency;
        //        tmp.supervision_timeout = conn_param.sup_to;
        //
        //        memcpy(tmp.peer_address.addr, connect_addr.addr, sizeof(connect_addr.addr));

        //        bk_ble_set_event_callback(dm_ble_at_event_cb);
        //        err = bk_ble_update_connection_params(&tmp);

        (void)addr_type;
        bk_ble_conn_update_params_t bk_param =
        {
            .min_int = conn_param.intv_min,
            .max_int = conn_param.intv_max,
            .latency = conn_param.con_latency,
            .timeout = conn_param.sup_to,
        };

        memcpy(&bk_param.bda, connect_addr.addr, sizeof(connect_addr.addr));

        bk_ble_gap_register_callback(dm_ble_gap_at_cb);
        err = bk_ble_gap_update_conn_params(&bk_param);
    }

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return err;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

int ble_get_conn_state_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    uint8_t conn_state = 0;
    char resultbuf[200];
    bd_addr_t peer_addr;
    if (argc != 1)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    err = get_addr_from_param(&peer_addr, argv[0]);
    if (err != kNoErr)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    conn_state = bk_ble_get_connect_state(&peer_addr);
    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        if (conn_state == 1)
        {
            os_snprintf(resultbuf, sizeof(resultbuf), "%s%s\r\n", "ATRSP:", "BLE_CONNECT");
            atsvr_output_msg(resultbuf);
            atsvr_cmd_rsp_ok();
        }
        else
        {
            //sprintf(pcWriteBuffer, "%s:%s\r\n%s", AT_CMDRSP_HEAD, "BLE_DISCONNECT", AT_CMD_RSP_SUCCEED);
            os_snprintf(resultbuf, sizeof(resultbuf), "%s%s\r\n", "ATRSP:", "BLE_DISCONNECT");
            atsvr_output_msg(resultbuf);
            atsvr_cmd_rsp_ok();
        }
        return err;
    }
    else
    {
        if (conn_state == AT_INIT_STATE_CONNECTTED)
        {
            os_snprintf(resultbuf, sizeof(resultbuf), "%s%s\r\n", "ATRSP:", "BLE_CONNECTED");
            atsvr_output_msg(resultbuf);
        }
        else if (conn_state == AT_INIT_STATE_IDLE)
        {
            os_snprintf(resultbuf, sizeof(resultbuf), "%s%s\r\n", "ATRSP:", "BLE_DISCONNECTED");
            atsvr_output_msg(resultbuf);
        }
        else if (conn_state == AT_INIT_STATE_CONNECTTING)
        {
            os_snprintf(resultbuf, sizeof(resultbuf), "%s%s\r\n", "ATRSP:", "BLE_CONNECTTING");
            atsvr_output_msg(resultbuf);
        }
        else if (conn_state == AT_INIT_STATE_STOPPING)
        {
            os_snprintf(resultbuf, sizeof(resultbuf), "%s%s\r\n", "ATRSP:", "BLE_DISCONNECTTING");
            atsvr_output_msg(resultbuf);
        }
        else
        {
            os_snprintf(resultbuf, sizeof(resultbuf), "%s%s\r\n", "ATRSP:", "BLE_DISCONNECTED");
            atsvr_output_msg(resultbuf);
        }

        return err;
    }

error:
    atsvr_cmd_rsp_error();
    return err;
}

int ble_get_local_addr_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    uint8_t local_addr[6];
    char resultbuf[200];

    if (argc != 0)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    err = bk_bluetooth_get_address(local_addr);
    if (err != kNoErr)
    {
        LOGE("bk_bluetooth_get_address error\n");
        err = kParamErr;
        goto error;
    }
    os_snprintf(resultbuf, sizeof(resultbuf), "%s%s%02x:%02x:%02x:%02x:%02x:%02x\r\n", "ATRSP:", "BLE_ADDR:",
                local_addr[5], local_addr[4], local_addr[3], local_addr[2], local_addr[1], local_addr[0]);
    atsvr_output_msg(resultbuf);


    snprintf(resultbuf, sizeof(resultbuf), "%s%s%02x:%02x:%02x:%02x:%02x:%02x\r\n", "ATRSP:", "BLE_ADDR:",
             local_addr[5], local_addr[4], local_addr[3], local_addr[2], local_addr[1], local_addr[0]);
    atsvr_output_msg(resultbuf);
    atsvr_cmd_rsp_ok();

    return err;

error:
    atsvr_cmd_rsp_error();
    return err;
}

int ble_set_local_addr_handle(int sync, int argc, char **argv)
{
    // not defined
    atsvr_output_msg("not defined\r\n");
    atsvr_cmd_rsp_error();
    return 0;
}

//#if (CONFIG_BTDM_5_2)


#define DECL_PRIMARY_SERVICE_128     {0x00,0x28,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
#define DECL_CHARACTERISTIC_128      {0x03,0x28,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
#define DESC_CLIENT_CHAR_CFG_128     {0x02,0x29,0,0,0,0,0,0,0,0,0,0,0,0,0,0}



ble_attm_desc_t test_service_db[TEST_IDX_NB] =
{
    //  Service Declaration
    [TEST_IDX_SVC]              = {DECL_PRIMARY_SERVICE_128, BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
    [TEST_IDX_CHAR_DECL]    = {DECL_CHARACTERISTIC_128,  BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
    // Characteristic Value
    [TEST_IDX_CHAR_VALUE]   = {{0x34, 0x12, 0},     BK_BLE_PERM_SET(NTF, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},
    //Client Characteristic Configuration Descriptor
    [TEST_IDX_CHAR_DESC] = {DESC_CLIENT_CHAR_CFG_128, BK_BLE_PERM_SET(RD, ENABLE) | BK_BLE_PERM_SET(WRITE_REQ, ENABLE), 0, 0},

    //data len
    [TEST_IDX_CHAR_DATALEN_DECL]    = {DECL_CHARACTERISTIC_128,  BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
    [TEST_IDX_CHAR_DATALEN_VALUE]   = {{0xbc, 0x9a, 0}, BK_BLE_PERM_SET(WRITE_REQ, ENABLE) | BK_BLE_PERM_SET(RD, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},

    //send inter
    [TEST_IDX_CHAR_INTER_DECL]    = {DECL_CHARACTERISTIC_128,  BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
    [TEST_IDX_CHAR_INTER_VALUE]   = {{0xf0, 0xde, 0}, BK_BLE_PERM_SET(WRITE_REQ, ENABLE) | BK_BLE_PERM_SET(RD, ENABLE), BK_BLE_PERM_SET(RI, ENABLE) | BK_BLE_PERM_SET(UUID_LEN, UUID_16), 128},

    //write test
    [TEST_IDX_CHAR_WRITE_TEST_DECL]    = {DECL_CHARACTERISTIC_128,  BK_BLE_PERM_SET(RD, ENABLE), 0, 0},
    [TEST_IDX_CHAR_WRITE_TEST_VALUE]   = {{0xf0, 0x12, 0}, BK_BLE_PERM_SET(NTF, ENABLE) | BK_BLE_PERM_SET(WRITE_COMMAND, ENABLE), BK_BLE_PERM_SET(UUID_LEN, UUID_16), 255},
    [TEST_IDX_CHAR_WRITE_TEST_DESC] = {DESC_CLIENT_CHAR_CFG_128, BK_BLE_PERM_SET(RD, ENABLE) | BK_BLE_PERM_SET(WRITE_REQ, ENABLE), 0, 0},
};

#define PERFORMANCE_STATISTIC_TX "tx"
#define PERFORMANCE_STATISTIC_RX "rx"

//AT+BLE_UPDATAMTU2MAX=<param1>
int ble_update_mtu_2_max_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;

    if (bk_ble_get_controller_stack_type() != BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2)
    {
        err = kParamErr;
        goto error;
    }

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        uint8 conn_idx = 0;
        bd_addr_t connect_addr;

        if (argc < 1)
        {
            LOGE("\nThe number of param is wrong!\n");
            err = kParamErr;
            goto error;
        }

        err = get_addr_from_param(&connect_addr, argv[0]);
        if (err != kNoErr)
        {
            LOGE("input addr param error\r\n");
            err = kParamErr;
            goto error;
        }

        conn_idx = bk_ble_find_conn_idx_from_addr(&connect_addr);
        if (conn_idx == AT_BLE_MAX_CONN)
        {
            LOGE("ble not connection\r\n");
            err = kNoResourcesErr;
            goto error;
        }


        if (sync)
        {
            err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
            if (err != kNoErr)
            {
                goto error;
            }
        }


        bk_ble_set_notice_cb(ble_at_notice_cb);

        err = bk_ble_gatt_mtu_change(conn_idx);

        if (err != BK_ERR_BLE_SUCCESS)
        {
            goto error;
        }
        else
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                goto error;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    atsvr_cmd_rsp_ok();
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return err;
                }
                else
                {
                    err = at_cmd_status;
                    at_cmd_status = BK_ERR_BLE_SUCCESS;
                    goto error;
                }
            }
        }
    }
    else
    {
        uint8_t conn_handle;
        uint16_t mtu = 251;

        bk_ble_set_event_callback(dm_ble_at_event_cb);
        conn_handle = os_strtoul(argv[0], NULL, 10) & 0xFF;

        if (argc >= 2)
        {
            mtu = os_strtoul(argv[1], NULL, 10);
        }

        err = bk_ble_set_gatt_mtu(conn_handle, mtu);

        if (err == 0)
        {
            atsvr_cmd_rsp_ok();
            return err;
        }
        else
        {
            LOGE("%s set mtu err %d\n", __func__, err);
        }
    }


error:
    g_test_prf_task_id = 0;
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

int ble_tx_test_param_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    uint16_t len = 0;
    uint32_t inter = 0;

    if (bk_ble_get_controller_stack_type() != BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2)
    {
        err = kParamErr;
        goto error;
    }

    if (argc < 2)
    {
        LOGE("\nThe number of param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    if (g_test_prf_task_id == 0)
    {
        LOGE("%s not reg profile, exit\n", __func__);
        err = kParamErr;
        goto error;
    }

    len = os_strtoul(argv[0], NULL, 10) & 0xFFFF;
    inter = os_strtoul(argv[1], NULL, 10) & 0xFFFFFFFF;


    if (inter == 0 || len == 0)
    {
        LOGE("%s param err\n", __func__);
        err = kParamErr;
        goto error;
    }

    if (len > s_ethermind_current_mtu - 3)
    {
        LOGE("%s len %d must less than s_ethermind_current_mtu - 3 %d!!!\n", __func__, len, s_ethermind_current_mtu - 3);
        err = kParamErr;
        goto error;
    }

    s_ethermind_send_size_value = s_test_data_len = len;
    s_ethermind_send_intv_value = s_test_send_inter = inter;
    LOGI("%s len %d inter %d\n", __func__, s_test_data_len, s_test_send_inter);
    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }
    else
    {
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                return err;
            }
            else
            {
                err = at_cmd_status;
                at_cmd_status = BK_ERR_BLE_SUCCESS;
                goto error;
            }
        }
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

int ble_tx_test_enable_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    uint8_t enable = 0;
    uint8 con_idx = 0;

    if (bk_ble_get_controller_stack_type() != BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2)
    {
        err = kParamErr;
        goto error;
    }

    if (argc < 1)
    {
        LOGE("\nThe number of param is wrong!\n");
        err = kParamErr;
        goto error;
    }


    if (g_test_prf_task_id == 0)
    {
        LOGE("%s not reg profile, exit\n", __func__);
        err = kParamErr;
        goto error;
    }

    enable = os_strtoul(argv[0], NULL, 10) & 0xFF;
    con_idx = os_strtoul(argv[1], NULL, 10) & 0xFF;
    LOGI("%s enable %d, con_idx %d\n", __func__, enable, con_idx);

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        ble_test_service_write_handle(enable, con_idx);
    }
    else
    {
        s_ethermind_auto_tx_enable = enable;

        if (enable)
        {
            if (s_ethermind_service_type == 1)
            {
                ble_tx_test_active_timer_callback(NULL);
            }
            else
            {
                if (!rtos_is_timer_init(&s_ethermind_ble_send_test_timer))
                {
                    rtos_init_timer(&s_ethermind_ble_send_test_timer, 1000, ble_tx_test_active_timer_callback, NULL);
                    rtos_start_timer(&s_ethermind_ble_send_test_timer);
                }
            }
        }
        else
        {
            if (rtos_is_timer_init(&s_ethermind_ble_send_test_timer))
            {
                if (rtos_is_timer_running(&s_ethermind_ble_send_test_timer))
                {
                    rtos_stop_timer(&s_ethermind_ble_send_test_timer);
                }

                rtos_deinit_timer(&s_ethermind_ble_send_test_timer);
            }
        }

    }

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }
    else
    {
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                return err;
            }
            else
            {
                err = at_cmd_status;
                at_cmd_status = BK_ERR_BLE_SUCCESS;
                goto error;
            }
        }
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}


static void ble_sdp_charac_callback(CHAR_TYPE type, uint8 conidx, uint16_t hdl, uint16_t len, uint8 *data)
{
    //LOGI("%s recv type %d len %d data[0] 0x%02X\n", __func__, type, len, data[0]);
    s_performance_rx_bytes += len;
}

int ble_register_noti_service_handle(int sync, int argc, char **argv)
{
    s_ethermind_service_type = s_service_type = 0;

    return ble_register_service_handle(sync, argc, argv);
}

int ble_register_performance_service_handle(int sync, int argc, char **argv)
{
    s_ethermind_service_type = s_service_type = 1;

    return ble_register_service_handle(sync, argc, argv);
}


int ble_enable_performance_statistic_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;

    uint8_t enable = 0;
    uint8_t type = 0;

    if (bk_ble_get_controller_stack_type() != BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2)
    {
        err = kParamErr;
        goto error;
    }

    if (argc < 2)
    {
        LOGE("\nThe number of param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    if (strlen(argv[0]) > 3)
    {
        LOGE("%s param err, exit\n", __func__);
        err = kParamErr;
        goto error;
    }

    if (!strncmp(PERFORMANCE_STATISTIC_TX, argv[0], strlen(PERFORMANCE_STATISTIC_TX)))
    {
        type = 1;
    }
    else if (!strncmp(PERFORMANCE_STATISTIC_RX, argv[0], strlen(PERFORMANCE_STATISTIC_RX)))
    {
        type = 2;
    }
    else
    {
        LOGE("%s param err, exit\n", __func__);
        err = kParamErr;
        goto error;
    }


    enable = os_strtoul(argv[1], NULL, 10) & 0xFFFFFFFF;

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        if (g_test_prf_task_id == 0)
        {
            LOGE("%s not reg profile, exit\n", __func__);
            err = kParamErr;
            goto error;
        }

        if (type == 1) //tx
        {
            if (enable)
            {
                if (!rtos_is_timer_init(&ble_performance_tx_statistics_tmr))
                {
                    s_performance_tx_bytes = 0;

                    rtos_init_timer(&ble_performance_tx_statistics_tmr, 1000, ble_performance_tx_timer_hdl, (void *)0);
                    rtos_start_timer(&ble_performance_tx_statistics_tmr);
                }
            }
            else
            {
                if (rtos_is_timer_init(&ble_performance_tx_statistics_tmr))
                {
                    s_performance_tx_bytes = 0;
                    if (rtos_is_timer_running(&ble_performance_tx_statistics_tmr))
                    {
                        rtos_stop_timer(&ble_performance_tx_statistics_tmr);
                    }
                    rtos_deinit_timer(&ble_performance_tx_statistics_tmr);
                }
            }
        }
        else if (type == 2) //rx
        {
            if (enable)
            {
                if (!rtos_is_timer_init(&ble_performance_rx_statistics_tmr))
                {
                    s_performance_rx_bytes = 0;

                    rtos_init_timer(&ble_performance_rx_statistics_tmr, 1000, ble_performance_rx_timer_hdl, (void *)0);
                    rtos_start_timer(&ble_performance_rx_statistics_tmr);
                }
            }
            else
            {
                if (rtos_is_timer_init(&ble_performance_rx_statistics_tmr))
                {
                    s_performance_rx_bytes = 0;

                    if (rtos_is_timer_running(&ble_performance_rx_statistics_tmr))
                    {
                        rtos_stop_timer(&ble_performance_rx_statistics_tmr);
                    }
                    rtos_deinit_timer(&ble_performance_rx_statistics_tmr);
                }
            }
        }
    }
    else
    {
        if (type == 1) //tx
        {
            if (enable)
            {
                if (!rtos_is_timer_init(&ble_ethermind_performance_tx_statistics_tmr))
                {
                    s_ethermind_performance_tx_bytes = 0;

                    rtos_init_timer(&ble_ethermind_performance_tx_statistics_tmr, 1000, ble_ethermind_performance_tx_timer_hdl, (void *)0);
                    rtos_start_timer(&ble_ethermind_performance_tx_statistics_tmr);
                }
            }
            else
            {
                if (rtos_is_timer_init(&ble_ethermind_performance_tx_statistics_tmr))
                {
                    s_performance_tx_bytes = 0;
                    if (rtos_is_timer_running(&ble_ethermind_performance_tx_statistics_tmr))
                    {
                        rtos_stop_timer(&ble_ethermind_performance_tx_statistics_tmr);
                    }
                    rtos_deinit_timer(&ble_ethermind_performance_tx_statistics_tmr);
                }
            }
        }
        else if (type == 2) //rx
        {
            if (enable)
            {
                if (!rtos_is_timer_init(&ble_ethermind_performance_rx_statistics_tmr))
                {
                    s_ethermind_performance_rx_bytes = 0;

                    rtos_init_timer(&ble_ethermind_performance_rx_statistics_tmr, 1000, ble_ethermind_performance_rx_timer_hdl, (void *)0);
                    rtos_start_timer(&ble_ethermind_performance_rx_statistics_tmr);
                }
            }
            else
            {
                if (rtos_is_timer_init(&ble_ethermind_performance_rx_statistics_tmr))
                {
                    s_ethermind_performance_rx_bytes = 0;

                    if (rtos_is_timer_running(&ble_ethermind_performance_rx_statistics_tmr))
                    {
                        rtos_stop_timer(&ble_ethermind_performance_rx_statistics_tmr);
                    }

                    rtos_deinit_timer(&ble_ethermind_performance_rx_statistics_tmr);
                }
            }
        }
    }

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }
    else
    {
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                return err;
            }
            else
            {
                err = at_cmd_status;
                at_cmd_status = BK_ERR_BLE_SUCCESS;
                goto error;
            }
        }
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

static void ble_convert_2_128b_uuid(uint8_t *uuid128, const uint8_t *uuid, uint8_t uuid_len)
{
    uint8_t auc_128UUIDBase[16] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t cursor = 0;

    if ((uuid_len == 4) || (uuid_len == 2))
    {
        cursor = 12;
    }
    else
    {
        uuid_len  = 16;
    }

    os_memcpy(&(auc_128UUIDBase[cursor]), uuid, uuid_len);
    os_memcpy(&uuid128[0], &auc_128UUIDBase[0], 16);
}


static int ble_discover_service_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
#if 0
    // int scanf_ret = 0;
    uint16_t uuid = 0;//0x1801;//BK_GATT_UUID_GAP_SVC;
    uint8_t att_req_uuid_len = 2;
    uint16_t sh = 1;
    uint16_t eh = 0xffff;
    uint8_t conn_handle = 0;

    if (bk_ble_get_host_stack_type() == BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        err = kParamErr;
        goto error;
    }

    if (argc < 3)
    {
        LOGE("\nThe number of param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    conn_handle = os_strtoul(argv[0], NULL, 10) & 0xFF;
    sh = os_strtoul(argv[1], NULL, 16) & 0xFF;
    eh = os_strtoul(argv[2], NULL, 16) & 0xFFFF;

    if (argc >= 4)
    {
        uuid = os_strtoul(argv[3], NULL, 16) & 0xffff;

        if (argc >= 5)
        {
            att_req_uuid_len = os_strtoul(argv[4], NULL, 10) & 0xFF;
        }
    }


    if (!uuid)
    {
        err = bk_ble_discover_primary_service(conn_handle, sh, eh);
    }
    else if (att_req_uuid_len == 2)
    {
        err = bk_ble_discover_primary_service_by_uuid(conn_handle, sh, eh, uuid);
    }
    else if (att_req_uuid_len == 16)
    {
        uint8_t uuid_128[16] = {0};
        ble_convert_2_128b_uuid(uuid_128, (uint8_t *)&uuid, sizeof(uuid));
        err = bk_ble_discover_primary_service_by_128uuid(conn_handle, sh, eh, uuid_128);
    }
    else
    {
        err = -1;
    }

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }
    else

    {
        atsvr_cmd_rsp_ok();
        return err;
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
#endif
    return err;
}

static void ble_tx_test_active_timer_callback(void *param)
{
    API_RESULT retval = 0;
    uint8_t *tmp_buff = NULL;

    tmp_buff = os_malloc(s_ethermind_send_size_value);

    if (!tmp_buff)
    {
        LOGE("%s alloc send failed\n", __func__);
        return;
    }

    os_memset(tmp_buff, 0, s_ethermind_send_size_value);
    tmp_buff[0] = s_ethermind_send_test_value++;


    retval = bk_ble_send_notify(s_ethermind_att_handle, s_ethermind_send_test_service_handle,
                                s_ethermind_send_char_test_handle, tmp_buff, s_ethermind_send_size_value);

    os_free(tmp_buff);

    if (retval != 0)
    {
        LOGE("%s notify err %d\n", __func__, retval);
    }
    else
    {
        s_ethermind_performance_tx_bytes += s_ethermind_send_size_value;
    }
}

static void ble_tx_test_passive_timer_callback(void *param)
{
    API_RESULT retval = 0;
    uint8_t *tmp_buff = NULL;
    GATT_DB_HANDLE *gdbh = (GATT_DB_HANDLE *)param;
    uint8_t conn_handle = s_ethermind_att_handle;

    tmp_buff = os_malloc(s_ethermind_send_size_value);

    if (!tmp_buff)
    {
        LOGE("%s alloc send failed\n", __func__);
        return;
    }

    os_memset(tmp_buff, 0, s_ethermind_send_size_value);
    tmp_buff[0] = s_ethermind_send_test_value++;

    retval = bk_ble_send_notify(conn_handle, gdbh->service_id, gdbh->char_id,
                                tmp_buff, s_ethermind_send_size_value);

    os_free(tmp_buff);

    if (retval != 0)
    {
        LOGE("%s notify err %d\n", __func__, retval);
    }
    else
    {
        s_ethermind_performance_tx_bytes += s_ethermind_send_size_value;
    }

}

static bk_err_t ethermind_test_gatt_char_handler
(
    uint8_t conn_handle,
    GATT_DB_HANDLE     *handle,
    GATT_DB_PARAMS     *params
)
{
    bk_err_t retval = 0;
    uint16_t value = 0;

    static GATT_DB_HANDLE *gdbh = NULL;

    LOGI("%s device_id %d service %d char %d attr_handle %d op 0x%02X\n", __func__,
         handle->device_id, handle->service_id, handle->char_id, params->handle, params->db_op);

    if (handle->service_id == s_ethermind_send_test_service_handle)
    {
        switch (params->db_op)
        {
            case GATT_DB_CHAR_PEER_CLI_CNFG_WRITE_REQ:
            {
                //BT_UNPACK_LE_2_BYTE (&value, params->value.val);
                memcpy(&value, params->value.val, 2);
                if (handle->char_id == s_ethermind_send_char_test_handle)
                {
                    if (GATT_CLI_CNFG_NOTIFICATION == value)
                    {
                        LOGI("%s enable notify\n", __func__);

                        if (!gdbh)
                        {
                            gdbh = (GATT_DB_HANDLE *)os_malloc(sizeof(*handle));

                            if (!gdbh)
                            {
                                LOGE("%s cant alloc GATT_DB_HANDLE\n", __func__);
                                retval = BK_FAIL;
                                break;
                            }
                        }

                        memcpy(gdbh, handle, sizeof(*handle));

                        if (s_ethermind_service_type == 1)
                        {
                            s_ethermind_auto_tx_enable = 1;
                            ble_tx_test_passive_timer_callback((void *)(size_t)gdbh);

                        }
                        else if (!rtos_is_timer_init(&s_ethermind_ble_send_test_timer))
                        {
                            rtos_init_timer(&s_ethermind_ble_send_test_timer, 1000, ble_tx_test_passive_timer_callback, (void *)(size_t)gdbh);
                            rtos_start_timer(&s_ethermind_ble_send_test_timer);
                        }

                    }
                    else
                    {
                        LOGI("%s disable notify\n", __func__);
                        s_ethermind_auto_tx_enable = 0;

                        if (rtos_is_timer_init(&s_ethermind_ble_send_test_timer))
                        {
                            if (rtos_is_timer_running(&s_ethermind_ble_send_test_timer))
                            {
                                rtos_stop_timer(&s_ethermind_ble_send_test_timer);
                            }

                            rtos_deinit_timer(&s_ethermind_ble_send_test_timer);
                        }

                        if (gdbh)
                        {
                            os_free(gdbh);
                            gdbh = NULL;
                        }
                    }
                }
                else
                {
                    retval = BK_FAIL;
                }
            }
            break;

            case GATT_DB_CHAR_PEER_WRITE_REQ:
            {
                if (handle->char_id == s_ethermind_send_size_handle)
                {
                    //BT_UNPACK_LE_2_BYTE(&value, k);
                    memcpy(&value, params->value.val, 2);
                    LOGI("%s write s_ethermind_send_size_handle %d\n", __func__, value);
                }
                else if (handle->char_id == s_ethermind_send_intv_handle)
                {
                    //BT_UNPACK_LE_2_BYTE(&value, params->value.val);
                    memcpy(&value, params->value.val, 2);
                    LOGI("%s write s_ethermind_send_intv_handle %d len %d %d\n", __func__, value, params->value.len, params->value.actual_len);
                }
                else
                {
                    retval = BK_FAIL;
                }
            }
            break;

            case GATT_DB_CHAR_PEER_READ_REQ:
                if (handle->char_id == s_ethermind_send_intv_handle)
                {
#if 0
                    //when s_ethermind_send_intv_handle buff is null
                    uint8_t conn_handle = 0;
                    const uint32_t test_resp = 0xabcdef01;

                    if (0 != bk_ble_get_conn_handle_from_device_handle(&conn_handle, &handle->device_id))
                    {
                        break;
                    }

                    //respone here
                    bk_ble_gatt_read_resp(conn_handle, (uint8_t *)&test_resp, sizeof(test_resp));

                    //must return GATT_DB_DELAYED_RESPONSE
                    retval = GATT_DB_DELAYED_RESPONSE;
#endif
                }

                break;
        }
    }
    else
    {
        //        retval = BK_FAIL;
    }

    return retval;
}



static int ble_register_service_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    uint16 my_service_uuid = 0;
    uint16 my_char_uuid = 0x1234;
    struct bk_ble_db_cfg ble_db_cfg;
    uint8 cur_test_prf_task_id = 0;

    if (bk_ble_get_controller_stack_type() != BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2)
    {
        err = kParamErr;
        goto error;
    }

    if (argc < 2)
    {
        LOGE("\nThe number of param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    cur_test_prf_task_id = g_test_prf_task_id;
    g_test_prf_task_id = os_strtoul(argv[0], NULL, 16) & 0xFF;
    my_service_uuid = os_strtoul(argv[1], NULL, 16) & 0xFFFF;
    if (argc >= 3)
    {
        my_char_uuid = os_strtoul(argv[2], NULL, 16) & 0xFFFF;
    }

    if (argc >= 5)
    {
        s_test_data_len = os_strtoul(argv[3], NULL, 10) & 0xFFFF;
        s_test_send_inter = os_strtoul(argv[4], NULL, 10) & 0xFFFF;

        //only for Midea case 19(add wrong attribute)
        if ((argc >= 6) && (os_strtoul(argv[5], NULL, 10) & 0xFFFF))
        {
            test_service_db[TEST_IDX_CHAR_DECL].uuid[0] = 0xfe;
        }
    }

    os_memcpy(&(test_service_db[TEST_IDX_CHAR_VALUE].uuid[0]), &my_char_uuid, 2);

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {

        os_memset(&ble_db_cfg, 0, sizeof(ble_db_cfg));

        ble_db_cfg.att_db = (ble_attm_desc_t *)test_service_db;
        ble_db_cfg.att_db_nb = TEST_IDX_NB;
        ble_db_cfg.prf_task_id = g_test_prf_task_id;
        ble_db_cfg.start_hdl = 0;
        ble_db_cfg.svc_perm = BK_BLE_PERM_SET(SVC_UUID_LEN, UUID_16);
        os_memcpy(&(ble_db_cfg.uuid[0]), &my_service_uuid, 2);

        if (1)//ble_at_cmd_table[19].is_sync_cmd)
        {
            err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
            if (err != kNoErr)
            {
                goto error;
            }
        }
        bk_ble_set_notice_cb(ble_at_notice_cb);
        //extern void register_app_sdp_charac_callback(void (*)(uint32_t type,uint8 conidx,uint16_t hdl,uint16_t len,uint8 *data));
        //register_app_sdp_charac_callback(ble_sdp_charac_callback);
        bk_ble_register_app_sdp_charac_callback(ble_sdp_charac_callback);
        err = bk_ble_create_db(&ble_db_cfg);


        if (err != BK_ERR_BLE_SUCCESS)
        {
            goto error;
        }
        else
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                goto error;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    atsvr_cmd_rsp_ok();
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return err;
                }
                else
                {
                    err = at_cmd_status;
                    at_cmd_status = BK_ERR_BLE_SUCCESS;
                    goto error;
                }
            }
        }
    }
    else
    {
        API_RESULT           retval;

        bk_ble_gap_register_callback(dm_ble_gap_at_cb);
        ble_at_gatts_reg();

        retval = ble_at_reg_gatts_db(s_service_type);

        if (retval)
        {
            bt_at_loge("ble_at_reg_gatts_db err %d", retval);
            return -1;
        }
        err = 0;

        atsvr_cmd_rsp_ok();
        return err;
    }




error:
    atsvr_cmd_rsp_error();
    g_test_prf_task_id = cur_test_prf_task_id;
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

void ble_test_noti_hdl(void *param)
{
    uint8 *write_buffer;
    uint8 con_idx = (uint32)param;

    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    write_buffer = (uint8_t *)os_malloc(s_test_data_len);

    if (!write_buffer)
    {
        LOGE("%s alloc err\n", __func__);
        return;
    }

    os_memset(write_buffer, 0, s_test_data_len);
    os_memcpy(write_buffer, &s_test_noti_count, sizeof(s_test_noti_count));
    s_test_noti_count++;

    LOGI("ble_at", "%s send_noti_value\r\n", __func__);

    ret = bk_ble_send_noti_value(con_idx, s_test_data_len, write_buffer, g_test_prf_task_id, TEST_IDX_CHAR_VALUE);
    if (ret != BK_ERR_BLE_SUCCESS)
    {
        LOGE("%s ret err %d\n", __func__, ret);
    }

    os_free(write_buffer);
}

static void ble_performance_tx_timer_hdl(void *param)
{
    uint32_t tmp = s_performance_tx_bytes;
    s_performance_tx_bytes = 0;

    LOGI("%s current tx %d bytes/sec\n", __func__, tmp);
}

static void ble_performance_rx_timer_hdl(void *param)
{
    uint32_t tmp = s_performance_rx_bytes;
    s_performance_rx_bytes = 0;

    LOGI("%s current rx %d bytes/sec\n", __func__, tmp);
}

static void ble_ethermind_performance_tx_timer_hdl(void *param)
{
    uint32_t tmp = s_ethermind_performance_tx_bytes;
    s_ethermind_performance_tx_bytes = 0;

    LOGI("%s current tx %d bytes/sec\n", __func__, tmp);
}

static void ble_ethermind_performance_rx_timer_hdl(void *param)
{
    uint32_t tmp = s_ethermind_performance_rx_bytes;
    s_ethermind_performance_rx_bytes = 0;

    LOGI("%s current rx %d bytes/sec\n", __func__, tmp);
}


void ble_test_service_write_handle(uint8 val, uint8 con_idx)
{
    if (val)
    {
        if (s_service_type)
        {
            if (!s_performance_tx_enable)
            {
                s_performance_tx_enable = 1;
                ble_test_noti_hdl((void *)((uint32)con_idx));
            }

        }
        else if (!rtos_is_timer_init(&ble_noti_tmr))
        {
            s_test_noti_count = 0;
            rtos_init_timer(&ble_noti_tmr, s_test_send_inter, ble_test_noti_hdl, (void *)((uint32)con_idx));
            rtos_start_timer(&ble_noti_tmr);
        }
    }
    else
    {
        s_performance_tx_enable = 0;
        if (rtos_is_timer_init(&ble_noti_tmr))
        {
            if (rtos_is_timer_running(&ble_noti_tmr))
            {
                rtos_stop_timer(&ble_noti_tmr);
            }
            rtos_deinit_timer(&ble_noti_tmr);
        }
    }
}

int ble_read_phy_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    bd_addr_t connect_addr;
    uint8_t conn_idx = 0;
    uint8_t addr_type = 1;

    if (bk_ble_get_controller_stack_type() != BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2)
    {
        err = kParamErr;
        goto error;
    }

    if (argc > 2)
    {
        LOGE("input param error\r\n");
        err = kParamErr;
        goto error;
    }


    err = get_addr_from_param(&connect_addr, argv[0]);
    if (err != kNoErr)
    {
        LOGE("input addr param error\r\n");
        err = kParamErr;
        goto error;
    }

    if (argc < 2)
    {
        LOGW("%s warning, you need input addr type\n", __func__);
    }
    else
    {
        addr_type = os_strtoul(argv[1], NULL, 10) & 0xFF;
    }

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_ble_set_notice_cb(ble_at_notice_cb);
        /// get connect_idx from connect_addr
        conn_idx = bk_ble_find_conn_idx_from_addr(&connect_addr);
        if (conn_idx == AT_BLE_MAX_CONN)
        {
            LOGE("ble not connection\r\n");
            err = kNoResourcesErr;
            goto error;
        }

        err = bk_ble_read_phy(conn_idx);
    }
    else
    {
        //        bk_ble_set_event_callback(dm_ble_at_event_cb);
        //        err = bk_ble_hci_read_phy(&connect_addr, addr_type, ble_at_cmd_cb);
        bk_bd_addr_t addr_new;

        os_memcpy(addr_new, g_peer_dev.bdaddr.addr, sizeof(g_peer_dev.bdaddr.addr));

        (void)addr_type;
        bk_ble_gap_register_callback(dm_ble_gap_at_cb);
        //ble_at_gattc_reg();
        err = bk_ble_gap_read_phy(addr_new);

    }

    if (err != kNoErr)
    {
        LOGE("read connect tx/rx phy failed\r\n");
        goto error;
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            LOGE("get sema fail\r\n");
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                //  if (atcmd_updated)
                // {
                //     sprintf(pcWriteBuffer, "%s:%d,%d\r\n%s", "PHY", s_ble_phy.tx_phy, s_ble_phy.rx_phy, AT_CMD_RSP_SUCCEED);
                //  }
                //  else
                //  {
                atsvr_cmd_rsp_ok();
                //   }
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return err;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }
error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

int ble_set_phy_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    bd_addr_t connect_addr;
    uint8_t conn_idx = 0;
    ble_set_phy_t le_set_phy;
    uint8_t addr_type = 1;

    if (bk_ble_get_controller_stack_type() != BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2)
    {
        err = kParamErr;
        goto error;
    }

    if (argc > 5)
    {
        LOGE("input param error\r\n");
        err = kParamErr;
        goto error;
    }

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }


    err = get_addr_from_param(&connect_addr, argv[0]);
    if (err != kNoErr)
    {
        LOGE("input addr param error\r\n");
        err = kParamErr;
        goto error;
    }

    le_set_phy.tx_phy = os_strtoul(argv[1], NULL, 10) & 0xFF;
    le_set_phy.rx_phy = os_strtoul(argv[2], NULL, 10) & 0xFF;
    le_set_phy.phy_opt = os_strtoul(argv[3], NULL, 10) & 0xFF;
    switch (le_set_phy.phy_opt)
    {
        case 0:
            le_set_phy.phy_opt = CODED_NO_PREFEER;
            break;
        case 1:
            le_set_phy.phy_opt = CODED_S2_PREFEER;
            break;
        case 2:
            le_set_phy.phy_opt = CODED_S8_PREFEER;
            break;
        default:
            LOGE("input phy_opt param error\r\n");
            err = kParamErr;
            goto error;
    }

    if (argc < 5)
    {
        LOGW("%s warning, you need input addr type\n", __func__);
    }
    else
    {
        addr_type = os_strtoul(argv[4], NULL, 10) & 0xFF;
    }

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_ble_set_notice_cb(ble_at_notice_cb);
        /// get connect_idx from connect_addr
        conn_idx = bk_ble_find_conn_idx_from_addr(&connect_addr);
        if (conn_idx == AT_BLE_MAX_CONN)
        {
            LOGE("ble not connection\r\n");
            err = kNoResourcesErr;
            goto error;
        }

        err = bk_ble_set_phy(conn_idx, &le_set_phy);
    }
    else
    {
        //        bk_ble_set_event_callback(dm_ble_at_event_cb);
        //        err = bk_ble_hci_set_phy(&connect_addr, addr_type, &le_set_phy, ble_at_cmd_cb);
        bk_bd_addr_t addr_new;
        bk_ble_gap_all_phys_t all_phys_mask = 0;
        bk_ble_gap_phy_mask_t tx_phy_mask = le_set_phy.tx_phy;
        bk_ble_gap_phy_mask_t rx_phy_mask = le_set_phy.rx_phy;
        bk_ble_gap_prefer_phy_options_t phy_options;
        (void)addr_type;
        switch (le_set_phy.phy_opt)
        {
            case 0:
                phy_options = BK_BLE_GAP_PHY_OPTIONS_NO_PREF;
                break;
            case 1:
                phy_options = BK_BLE_GAP_PHY_OPTIONS_PREF_S2_CODING;
                break;
            case 2:
                phy_options = BK_BLE_GAP_PHY_OPTIONS_PREF_S8_CODING;
                break;
            default:
                LOGE("input phy_opt param error\r\n");
                err = kParamErr;
                goto error;
        }

        os_memcpy(addr_new, g_peer_dev.bdaddr.addr, sizeof(g_peer_dev.bdaddr.addr));

        bk_ble_gap_register_callback(dm_ble_gap_at_cb);
        err = bk_ble_gap_set_preferred_phy(addr_new, all_phys_mask, tx_phy_mask, rx_phy_mask, phy_options);
    }

    if (err != kNoErr)
    {
        LOGE("set connect tx/rx phy failed\r\n");
        goto error;
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            LOGE("get sema fail\r\n");
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return err;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}
//#endif

int ble_set_max_mtu_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    uint16_t att_max_mtu = 0;

    if ((bk_ble_get_controller_stack_type() != BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2)
        || (bk_ble_get_host_stack_type() == BK_BLE_HOST_STACK_TYPE_ETHERMIND))
    {
        err = kParamErr;
        goto error;
    }

    if (argc != 1)
    {
        LOGE("input param error\r\n");
        err = kParamErr;
        goto error;
    }

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }

    bk_ble_set_notice_cb(ble_at_notice_cb);

    att_max_mtu = os_strtoul(argv[0], NULL, 10) & 0xFF;
    err = bk_ble_set_max_mtu(att_max_mtu);
    if (err != kNoErr)
    {
        LOGE("set att maximal MTU failed\r\n");
        goto error;
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            LOGE("get sema fail\r\n");
            goto error;
        }
        else
        {
            if (at_cmd_status != BK_ERR_BLE_SUCCESS)
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

    atsvr_cmd_rsp_ok();
    rtos_deinit_semaphore(&ble_at_cmd_sema);
    return err;

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}


int ble_delete_adv_activity_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    int actv_idx = 0;

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_ADV_CREATED);
        if (actv_idx == AT_BLE_MAX_ACTV)
        {
            BK_LOGE(TAG, "\n adv activity has not been created!\n");
            err = kNotFoundErr;
            goto error;
        }

        err = bk_ble_delete_advertising(actv_idx, ble_at_cmd_cb);
        if (err != BK_ERR_BLE_SUCCESS)
        {
            goto error;
        }
        if (ble_at_cmd_sema != NULL)
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                goto error;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    atsvr_cmd_rsp_ok();
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return 0;
                }
                else
                {
                    err = at_cmd_status;
                    goto error;
                }
            }
        }
    }
    else
    {
        if (ble_at_cmd_sema != NULL)
        {
            atsvr_cmd_rsp_ok();
            rtos_deinit_semaphore(&ble_at_cmd_sema);
            return 0;
        }
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

int ble_delete_scan_activity_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    int actv_idx = 0;

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_SCAN_CREATED);
        if (actv_idx == AT_BLE_MAX_ACTV)
        {
            BK_LOGE(TAG, "\n scan activity has not been created!\n");
            err = kNotFoundErr;
            goto error;
        }

        err = bk_ble_delete_scaning(actv_idx, ble_at_cmd_cb);
        if (err != BK_ERR_BLE_SUCCESS)
        {
            goto error;
        }
        if (ble_at_cmd_sema != NULL)
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                goto error;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    atsvr_cmd_rsp_ok();
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return 0;
                }
                else
                {
                    err = at_cmd_status;
                    goto error;
                }
            }
        }
    }
    else
    {
        if (ble_at_cmd_sema != NULL)
        {
            atsvr_cmd_rsp_ok();
            rtos_deinit_semaphore(&ble_at_cmd_sema);
            return 0;
        }
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

int ble_connect_by_name_handle(int sync, int argc, char **argv)
{
    uint8_t actv_idx = 0;
    ble_conn_param_t conn_param;
    int err = kNoErr;
    uint8_t central_count = 0;

    if (!bk_ble_if_support_central(&central_count) || central_count == 0)
    {
        LOGE("not support central\n");
        err = kParamErr;
        goto error;
    }


    if (argc != 5)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    conn_param.intv_min = os_strtoul(argv[0], NULL, 16) & 0xFFFF;
    conn_param.con_latency = os_strtoul(argv[1], NULL, 16) & 0xFFFF;
    if (conn_param.intv_min < CON_INTERVAL_MIN || conn_param.intv_min > CON_INTERVAL_MAX
        || conn_param.con_latency > CON_LATENCY_MAX)
    {
        err = kParamErr;
        goto error;
    }
    conn_param.intv_max = conn_param.intv_min;

    conn_param.sup_to = os_strtoul(argv[2], NULL, 16) & 0xFFFF;
    if (conn_param.sup_to < CON_SUP_TO_MIN || conn_param.sup_to > CON_SUP_TO_MAX)
    {
        err = kParamErr;
        goto error;
    }

    conn_param.init_phys = os_strtoul(argv[3], NULL, 16) & 0xFF;
    if (conn_param.init_phys > (PHY_1MBPS_BIT | PHY_2MBPS_BIT | PHY_CODED_BIT))
    {
        err = kParamErr;
        goto error;
    }

    os_memset(&g_peer_dev, 0, sizeof(g_peer_dev));

    g_peer_dev.dev.len = os_strlen(argv[4]);
    os_memcpy(g_peer_dev.dev.name, argv[4], g_peer_dev.dev.len);
    LOGI("%s %s\r\n", __func__, g_peer_dev.dev.name);
    g_peer_dev.state = STATE_DISCOVERING;

    if (err != kNoErr)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    if ((10 * conn_param.sup_to) < (((1 + conn_param.con_latency) * conn_param.intv_min * 5 + 1) >> 1))
    {
        LOGE("input param not suitable, maybe you can set con_latency to 0\n");
        err = kParamErr;
        goto error;
    }

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }



    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_ble_set_notice_cb(ble_at_notice_cb);
        if (ble_at_cmd_sema != NULL)
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, 10000);
            if (err != kNoErr)
            {
                goto error;
            }
            else
            {
                if (g_peer_dev.state == STATE_DISCOVERED)
                {
                    g_peer_dev.state = STATE_CONNECTINIG;
                }
                else
                {
                    err = BK_ERR_BLE_FAIL;
                    goto error;
                }
            }
        }
        /*actv_idx = bk_ble_find_master_state_idx_handle(AT_INIT_STATE_CREATED);
        if (actv_idx == AT_BLE_MAX_CONN)*/
        {
            /// Do not create actv
            actv_idx = bk_ble_get_idle_conn_idx_handle();
            if (actv_idx == UNKNOW_ACT_IDX)
            {
                err = kNoResourcesErr;
                goto error;
            }

            err = bk_ble_create_init(actv_idx, &conn_param, ble_at_cmd_cb);
            if (err != BK_ERR_BLE_SUCCESS)
            {
                goto error;
            }
            if (ble_at_cmd_sema != NULL)
            {
                err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
                if (err != kNoErr)
                {
                    goto error;
                }
                else
                {
                    if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                    {
                        bk_ble_register_app_sdp_common_callback(ble_connect_sdp_comm_callback);
                        err = ble_start_connect_handle(actv_idx, g_peer_dev.addr_type, &(g_peer_dev.bdaddr), ble_at_cmd_cb);
                        if (err != kNoErr)
                        {
                            goto error;
                        }
                        atsvr_cmd_rsp_ok();
                        rtos_deinit_semaphore(&ble_at_cmd_sema);
                        return err;
                    }
                    else
                    {
                        err = at_cmd_status;
                        goto error;
                    }
                }
            }
        }
        /*    else
            {
                /// have created actv, this happend in which connection have been disconnected
                err = ble_start_connect_handle(actv_idx, g_peer_dev.addr_type, &(g_peer_dev.bdaddr), ble_at_cmd_cb);
                if (err != kNoErr)
                    goto error;
                msg = AT_CMD_RSP_SUCCEED;
                os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return err;
            }
        */
    }
    else
    {
        ble_scan_param_t scan_param;
        scan_param.own_addr_type = 0; // ethermind does not have OWN_ADDR_TYPE_PUBLIC_OR_STATIC_ADDR
        scan_param.scan_phy = PHY_1MBPS_BIT;
        scan_param.scan_intv = 0x64;
        scan_param.scan_wd = 0x1e;

        uint8_t filt_duplicate = 0;
        uint16_t duration = 0;
        uint16_t period = 10;

        bk_ble_set_event_callback(dm_ble_at_event_cb);

        if (ble_at_cmd_sema != NULL)
        {
            err = bk_ble_set_scan_parameters(scan_param.own_addr_type, 0x00, scan_param.scan_phy, scan_param.scan_intv, scan_param.scan_wd, ble_at_cmd_cb);
            LOGI("%s set scan param\r\n", __func__);
            if (err != BK_ERR_BLE_SUCCESS)
            {
                goto error;
            }
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                goto error;
            }
            err = bk_ble_set_scan_enable_extended(1, filt_duplicate, duration, period, ble_at_cmd_cb);
            if (err != BK_ERR_BLE_SUCCESS)
            {
                goto error;
            }
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);

            if (err != kNoErr)
            {
                goto error;
            }
            LOGI("%s start scan\r\n", __func__);

            rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);

            if (g_peer_dev.state == STATE_DISCOVERED)
            {
                bk_ble_set_scan_enable_extended(0, filt_duplicate, duration, period, ble_at_cmd_cb);
                err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
                if (err != kNoErr)
                {
                    goto error;
                }
                LOGI("%s stop scan\r\n", __func__);

                ble_conn_param_normal_t tmp;

                tmp.conn_interval_min = conn_param.intv_min;
                tmp.conn_interval_max = conn_param.intv_max;
                tmp.conn_latency = conn_param.con_latency;
                tmp.supervision_timeout = conn_param.sup_to;
                tmp.initiating_phys = conn_param.init_phys;

                tmp.peer_address_type = g_peer_dev.addr_type;
                os_memcpy(tmp.peer_address.addr, &(g_peer_dev.bdaddr.addr[0]), BK_BLE_GAP_BD_ADDR_LEN);
                LOGI("%s start conn, peer addr: %x %x %x %x %x %x\r\n", __func__, tmp.peer_address.addr[0], tmp.peer_address.addr[1],
                     tmp.peer_address.addr[2], tmp.peer_address.addr[3], tmp.peer_address.addr[4], tmp.peer_address.addr[5]);
                g_peer_dev.state = STATE_CONNECTINIG;

                err = bk_ble_create_connection(&tmp, ble_at_cmd_cb);

                if (err != BK_ERR_BLE_SUCCESS)
                {
                    goto error;
                }

                err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
                if (err != kNoErr)
                {
                    goto error;
                }
                else
                {
                    if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                    {
                        atsvr_cmd_rsp_ok();
                        rtos_deinit_semaphore(&ble_at_cmd_sema);
                        return err;
                    }
                    else
                    {
                        err = at_cmd_status;
                        goto error;
                    }
                }
            }
            else
            {
                LOGI("%s SCAN Timeout %d\r\n", __func__, g_peer_dev.state);
                bk_ble_set_scan_enable_extended(0, filt_duplicate, duration, period, ble_at_cmd_cb);
                goto error;
            }
        }

    }

error:
    g_peer_dev.state = STATE_IDLE;
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

int ble_disconnect_by_name_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    uint8_t conn_idx;
    uint8_t central_count = 0;
    ble_device_name_t dev;

    if (!bk_ble_if_support_central(&central_count) || central_count == 0)
    {
        LOGE("not support central\n");
        err = kParamErr;
        goto error;
    }

    if (argc != 1)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    os_memset(&dev, 0, sizeof(dev));

    dev.len = os_strlen(argv[0]);
    os_memcpy(dev.name, argv[0], dev.len);

    if (!((STATE_CONNECTED == g_peer_dev.state) && (dev.len == g_peer_dev.dev.len) && (!os_memcmp(g_peer_dev.dev.name, dev.name, dev.len))))
    {
        err = kParamErr;
        goto error;
    }

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }
    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        /// get connect_idx from connect_addr
        conn_idx = bk_ble_find_conn_idx_from_addr(&g_peer_dev.bdaddr);
        if (conn_idx == AT_BLE_MAX_CONN)
        {
            LOGE("ble not connection\n");
            err = kNoResourcesErr;
            goto error;
        }

        g_peer_dev.state = STATE_DISCONNECTINIG;

        err = bk_ble_disconnect(conn_idx);
        bk_ble_set_notice_cb(ble_at_notice_cb);
        err = bk_ble_disconnect(conn_idx);
    }
    else
    {
        bk_ble_gap_register_callback(dm_ble_gap_at_cb);
        ble_at_gattc_reg();

        g_peer_dev.state = STATE_DISCONNECTINIG;
        err = bk_ble_disconnect_connection(&g_peer_dev.bdaddr, ble_at_cmd_cb);
    }

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }
    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                g_peer_dev.state = STATE_IDLE;
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    atsvr_cmd_rsp_error();
    g_peer_dev.state = STATE_IDLE;
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}



int ble_create_periodic_sync_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    uint8_t actv_idx = 0;

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }

    bk_ble_set_notice_cb(ble_at_notice_cb);
    actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_PER_SYNC_CREATED);
    if (actv_idx == AT_BLE_MAX_ACTV)
    {
        actv_idx = bk_ble_get_idle_actv_idx_handle();
        if (actv_idx == UNKNOW_ACT_IDX)
        {
            err = kNoResourcesErr;
            goto error;
        }
    }

    err = bk_ble_create_periodic_sync(actv_idx, ble_at_cmd_cb);

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }
    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return err;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

int ble_start_periodic_sync_handle(int sync, int argc, char **argv)
{
    uint8_t actv_idx = 0;

    int err = kNoErr;

    ble_periodic_param_t periodic_param;

    if (argc < 7)
    {
        LOGE("\nThe number of param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    os_memset(&periodic_param, 0, sizeof(periodic_param));

    periodic_param.report_disable = os_strtoul(argv[0], NULL, 16) & 0xFF;
    periodic_param.adv_sid = os_strtoul(argv[1], NULL, 16) & 0xFF;

    err = get_addr_from_param(&periodic_param.adv_addr, argv[2]);
    if (err != kNoErr)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    periodic_param.adv_addr_type = os_strtoul(argv[3], NULL, 16) & 0xFF;
    periodic_param.skip = os_strtoul(argv[4], NULL, 16) & 0xFF;
    periodic_param.sync_to = os_strtoul(argv[5], NULL, 16) & 0xFF;
    periodic_param.cte_type = os_strtoul(argv[6], NULL, 16) & 0xFF;



    if (1)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }
    bk_ble_set_notice_cb(ble_at_notice_cb);


    actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_PER_SYNC_CREATED);


    if (actv_idx == AT_BLE_MAX_ACTV)
    {
        LOGE("periodic not create before\n");
        err = kNoResourcesErr;
        goto error;
    }

    err = bk_ble_start_periodic_sync(actv_idx, &periodic_param, ble_at_cmd_cb);

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }
    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}


int ble_stop_periodic_sync_handle(int sync, int argc, char **argv)
{
    uint8_t actv_idx = 0;

    int err = kNoErr;

    bd_addr_t addr;

    if (argc < 1)
    {
        LOGE("\nThe number of param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    err = get_addr_from_param(&addr, argv[0]);
    if (err != kNoErr)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    if (1)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }

    bk_ble_set_notice_cb(ble_at_notice_cb);

    actv_idx = bk_ble_find_actv_state_idx_handle(AT_ACTV_PER_SYNC_STARTED);

    if (actv_idx == AT_BLE_MAX_ACTV)
    {
        LOGE("periodic not start before\n");
        err = kNoResourcesErr;
        goto error;
    }

    err = bk_ble_stop_periodic_sync(actv_idx, ble_at_cmd_cb);

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }
    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}


#if 0
void s_ble_update_param_callback(MASTER_COMMON_TYPE type, uint8 conidx, void *param)
{
    switch (type)
    {
        case MST_TYPE_UPP_ASK:
        {
            struct mst_comm_updata_para *tmp = (typeof(tmp))param;
            LOGI("%s MST_TYPE_UPP_ASK accept\n", __func__);
            tmp->is_agree = 0;
        }
        break;

        default:
            break;
    }
}

int ble_update_param_reply_handle(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{

    int err = kNoErr;
#if 0
    uint8_t con_idx = 0;
    uint8_t accept = 0;

    if (bk_ble_get_controller_stack_type() != BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2)
    {
        err = kParamErr;
        goto error;
    }

    if (argc != 2)
    {
        LOGI("input param error\r\n");
        err = kParamErr;
        goto error;
    }

    bk_ble_set_notice_cb(ble_at_notice_cb);

    bk_ble_register_app_sdp_common_callback(s_ble_update_param_callback);

    con_idx = os_strtoul(argv[0], NULL, 10) & 0xFF;
    accept = os_strtoul(argv[0], NULL, 10) & 0xFF;

    //err = ble_param_update_reply(con_idx, accept);
    if (err != kNoErr)
    {
        LOGI("set update param reply failed\r\n");
        goto error;
    }
    atsvr_cmd_rsp_ok();


error:
#endif
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}
#endif

int ble_att_write_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;

    char tmp_buf[3] = {0};
    // int scanf_ret = 0;
    uint8_t data_count = 0;
    int8_t i = 0;

    uint8_t conn_handle = 0;
    ATT_ATTR_HANDLE attr_handle = 0;

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        err = kParamErr;
        goto error;
    }

    if (argc < 3)
    {
        LOGE("\nThe number of param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    conn_handle = os_strtoul(argv[0], NULL, 10) & 0xFF;
    attr_handle = os_strtoul(argv[1], NULL, 10) & 0xFFFF;

    if (strlen(argv[2]) > sizeof(send_value) * 2)
    {
        LOGE("\nThe number of data is wrong!\n");
        err = kParamErr;
        goto error;
    }

    for (i = strlen(argv[2]) - 2, data_count = 0; i >= 0; i -= 2, data_count++)
    {
        os_memset(tmp_buf, 0, sizeof(tmp_buf));
        os_memcpy(tmp_buf, argv[2] + i, 2);

        send_value[data_count] = os_strtoul(tmp_buf, NULL, 10) & 0xFF;
    }

    LOGI("%s %d %d %d\n", __func__, conn_handle, attr_handle, data_count);

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_ble_set_notice_cb(ble_at_notice_cb);
        bk_ble_register_app_sdp_charac_callback(ble_att_sdp_charac_callback);
        err = bk_ble_att_write(conn_handle, attr_handle, send_value, data_count);
    }
    else
    {
        bk_ble_gap_register_callback(dm_ble_gap_at_cb);
        ble_at_gattc_reg();
        err = bk_ble_gattc_write_char(ble_at_get_gatt_ctx_handle()->gattc_if, conn_handle, attr_handle, data_count, send_value,
                                      BK_GATT_WRITE_TYPE_RSP, BK_GATT_AUTH_REQ_NONE);
    }

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }
    else
    {
        atsvr_cmd_rsp_ok();
        return err;
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

static void ble_att_sdp_charac_callback(CHAR_TYPE type, uint8 conidx, uint16_t hdl, uint16_t len, uint8 *data)
{
    if (CHARAC_READ == type)
    {
        LOGI("%s, read hdl %d, len %d\r\n", __func__, hdl, len);

        if (len)
        {
            LOGI("\n==================\n");
            char s[100] = {0};

            os_memcpy(s, data, len < sizeof(s) - 1 ? len : sizeof(s) - 1);

            if (s_device_name_hdl == hdl)
            {
                LOGI("%s\n", s);
            }

            LOGI("\n==================\n");

            os_memset(s, 0, sizeof(s));
            for (int i = 0; i < 5 && i < len; ++i)
            {
                sprintf(s + i * 2, "%02x", data[i]);
            }

            LOGI("0x%s\n", s);
            LOGI("\n==================\n");

            if (s_read_tmp_attr_handle == hdl)
            {
                if (ble_at_cmd_sema != NULL)
                {
                    rtos_set_semaphore(&ble_at_cmd_sema);
                }
            }
        }
    }
}

static int ble_att_read_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    uint16_t conn_handle = 0;

    if (argc < 2)
    {
        LOGE("\nThe number of param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    conn_handle = os_strtoul(argv[0], NULL, 10) & 0xFF;
    s_read_tmp_attr_handle = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
    //    LOGI("%s %d %d %d\n", __func__, att_handle.device_id, att_handle.att_id, attr_handle);

    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_ble_set_notice_cb(ble_at_notice_cb);
        bk_ble_register_app_sdp_charac_callback(ble_att_sdp_charac_callback);
        err = bk_ble_gattc_read((uint8_t)conn_handle, s_read_tmp_attr_handle, 0);
    }
    else
    {
        bk_ble_gap_register_callback(dm_ble_gap_at_cb);
        ble_at_gattc_reg();
        err = bk_ble_gattc_read_char(ble_at_get_gatt_ctx_handle()->gattc_if, conn_handle, s_read_tmp_attr_handle, BK_GATT_AUTH_REQ_NONE);
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }
error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

static void ble_plr_sdp_comm_callback(MASTER_COMMON_TYPE type, uint8 conidx, void *param)
{
    if (MST_TYPE_ATT_UUID == type)
    {
        struct ble_sdp_char_inf *char_inf = (struct ble_sdp_char_inf *)param;

        if ((!os_memcmp(char_inf->uuid, nus_tx_uuid, 16)) && (conidx < AT_BLE_MAX_CONN))
        {
            plc_ccc_handle[conidx] = char_inf->val_hdl + 1;
        }

        if ((!os_memcmp(char_inf->uuid, nus_rx_uuid, 16)) && (conidx < AT_BLE_MAX_CONN))
        {
            plc_rx_handle[conidx] = char_inf->val_hdl;
        }
    }

    if (MST_TYPE_SDP_END == type)
    {
        bk_ble_gatt_mtu_change(conidx);
    }

    if (type == MST_TYPE_UPP_ASK)
    {
        struct mst_comm_updata_para *tmp = (typeof(tmp))param;
        LOGI("%s MST_TYPE_UPP_ASK accept\n", __func__);
        tmp->is_agree = 1;
    }

}

static void ble_plr_sdp_charac_callback(CHAR_TYPE type, uint8 conidx, uint16_t hdl, uint16_t len, uint8 *data)
{
    if (CHARAC_NOTIFY == type)
    {
        //LOGI("con %d , len : %d, recv data %s\n", conidx, len, data);
        //bk_ble_gatt_write_value(conidx, plc_rx_handle[conidx], len, data);
#ifdef CONFIG_ALI_MQTT
        if (loop_type == LT_COEX)
        {
            ble_send_data_2_mqtt(conidx, len, data);
        }
        else
        {
            bk_ble_gatt_write_value(conidx, plc_rx_handle[conidx], len, data);
        }
#else
        bk_ble_gatt_write_value(conidx, plc_rx_handle[conidx], len, data);
#endif

    }
    else if (CHARAC_WRITE_DONE == type)
    {
        //LOGI("con %d\n", conidx);
    }
}

int ble_enable_packet_loss_ratio_test_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    uint8_t cmd = 0;

    if (bk_ble_get_controller_stack_type() != BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2)
    {
        err = kParamErr;
        goto error;
    }

    if (argc < 1)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    cmd = os_strtoul(argv[0], NULL, 10) & 0xFF;

    if (cmd == 0)
    {
        if (BK_BLE_HOST_STACK_TYPE_ETHERMIND != bk_ble_get_host_stack_type())
        {
            for (uint8_t j = 0; j < AT_BLE_MAX_CONN; j++)
            {
                if (plc_ccc_handle[j])
                {
                    bk_ble_gatt_write_ccc(j, plc_ccc_handle[j], 0x00);
                }
                else
                {
                    LOGE("the %dth conn is not connected\n", j);
                }
            }
        }
        else
        {
            const uint16_t noti_enable = 0;
            for (uint8_t i = 0; i < sizeof(s_ethermind_nordic_att_info) / sizeof(s_ethermind_nordic_att_info[0]); i++)
            {
                if (s_ethermind_nordic_used[i])
                {
                    bk_ble_att_write(s_ethermind_nordic_att_info[i], s_ethermind_nordic_write_notify_handle[i], (uint8_t *)&noti_enable, sizeof(noti_enable));
                }

            }
        }
    }
    else if (cmd == 1)
    {
        if (BK_BLE_HOST_STACK_TYPE_ETHERMIND != bk_ble_get_host_stack_type())
        {
            os_memset(plc_ccc_handle, 0, sizeof(plc_ccc_handle));
            os_memset(plc_rx_handle, 0, sizeof(plc_ccc_handle));
            bk_ble_register_app_sdp_common_callback(ble_plr_sdp_comm_callback);
            bk_ble_register_app_sdp_charac_callback(ble_plr_sdp_charac_callback);
        }
    }
    else if (cmd == 2)
    {
        if (argc < 4)
        {
            LOGE("input param error\n");
            err = kParamErr;
            goto error;
        }

        uint16_t data_cnt = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
        uint32_t interval = os_strtoul(argv[2], NULL, 10);
        uint16_t data_len = os_strtoul(argv[3], NULL, 10) & 0xFFFF;
        uint8_t cmd[10] = {0x5a, 0xa5, 0, 0};
        os_memcpy(&cmd[2], &data_cnt, sizeof(data_cnt));
        os_memcpy(&cmd[4], &interval, sizeof(interval));
        os_memcpy(&cmd[8], &data_len, sizeof(data_len));
        if (BK_BLE_HOST_STACK_TYPE_ETHERMIND != bk_ble_get_host_stack_type())
        {
            for (uint8_t j = 0; j < AT_BLE_MAX_CONN; j++)
            {
                if (plc_rx_handle[j])
                {
                    bk_ble_gatt_write_value(j, plc_rx_handle[j], sizeof(cmd), cmd);
                }
                else
                {
                    LOGE("the %dth conn is not connected\n", j);
                }
            }
        }
        else
        {
            for (uint8_t i = 0; i < sizeof(s_ethermind_nordic_att_info) / sizeof(s_ethermind_nordic_att_info[0]); i++)
            {
                if (s_ethermind_nordic_used[i])
                {
                    LOGI("%s ATT_CON_ID %d set notify param\n", __func__, s_ethermind_nordic_att_info[i]);
                    bk_ble_att_write(s_ethermind_nordic_att_info[i], s_ethermind_nordic_write_attr_handle[i], (uint8_t *)&cmd, sizeof(cmd));
                }
            }
        }
    }
    else if (cmd == 3)
    {
        if (BK_BLE_HOST_STACK_TYPE_ETHERMIND != bk_ble_get_host_stack_type())
        {
            for (uint8_t j = 0; j < AT_BLE_MAX_CONN; j++)
            {
                if (plc_ccc_handle[j])
                {
                    bk_ble_gatt_write_ccc(j, plc_ccc_handle[j], 0x01);
                }
                else
                {
                    LOGE("the %dth conn is not connected\n", j);
                }
            }
        }
        else
        {
            const uint16_t noti_enable = 0x0001;
            for (uint8_t i = 0; i < sizeof(s_ethermind_nordic_att_info) / sizeof(s_ethermind_nordic_att_info[0]); i++)
            {
                if (s_ethermind_nordic_used[i])
                {
                    LOGI("%s ATT_CON_ID %d enable notify\n", __func__, s_ethermind_nordic_att_info[i]);
                    bk_ble_att_write(s_ethermind_nordic_att_info[i], s_ethermind_nordic_write_notify_handle[i], (uint8_t *)&noti_enable, sizeof(noti_enable));
                }

            }
        }
    }

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }
    else
    {
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                return err;
            }
            else
            {
                err = at_cmd_status;
                at_cmd_status = BK_ERR_BLE_SUCCESS;
                goto error;
            }
        }
    }

error:
    atsvr_cmd_rsp_error();
    return err;
}


#ifdef CONFIG_ALI_MQTT
/*
* AT+BLE=MQTTLOOPBACK, connect, 222.71.10.2, aclsemi, xxxxxxxxx
* AT+BLE=MQTTLOOPBACK, wifi, /ble/lpsr/xxx, /ble/lpci/xxx, 285, 7000
*/
static beken_timer_t *mqtt_loop_timer = NULL;
uint32_t loop_count = 0;
uint32_t loop_interval = 0;
char *loop_topic_ci = NULL;
char *loop_topic_sr = NULL;
uint32_t loop_ci_index = 0;
uint32_t loop_sr_index = 0;

char *ble_mqtt_loop_tx_buffer = NULL;
char *ble_mqtt_loop_rx_buffer = NULL;
void *ble_mqtt_loop_client = NULL;


#define PRODUCT_KEY             "aclsemi"
#define DEVICE_NAME             "bk7256"
#define DEVICE_SECRET           "nMwWRZrjupURGSByK7qu3uCwzEYUHORu"


static void mqtt_loop_timer_callback(void *param)
{
    if (loop_type == LT_WIFI_ONLY)
    {
        iotx_mqtt_topic_info_t topic_msg;
        char msg_pub[350] = {0};
        int rc = -1;
        char rev_data[250] = {0};

        /* Initialize topic information */
        memset(&topic_msg, 0x0, sizeof(iotx_mqtt_topic_info_t));

        topic_msg.qos = IOTX_MQTT_QOS0;
        topic_msg.retain = 0;
        topic_msg.dup = 0;

        /* Generate topic message */
        int msg_len = snprintf(msg_pub, sizeof(msg_pub), "{\"link_id\":\"%d\", \"rev_len\":\"%d\", \"rev_data\":\"", 0, 250);

        if (msg_len < 0)
        {
            LOGE("Error occur! Exit program.\n");
        }

        sprintf(rev_data, "%d-%d", loop_ci_index++, loop_sr_index);
        os_memcpy(&msg_pub[msg_len], rev_data, 250);
        msg_len += 250;

        msg_pub[msg_len++] = '"';
        msg_pub[msg_len++] = '}';

        topic_msg.payload = (void *)msg_pub;
        topic_msg.payload_len = msg_len;

        rc = IOT_MQTT_Publish(ble_mqtt_loop_client, loop_topic_ci, &topic_msg);
        if (rc < 0)
        {
            LOGE("error occur when publish.\n");
        }
        LOGI("packet-id=%u, publish topic msg=%s.\n", (uint32_t)rc, msg_pub);
    }


    if (loop_ci_index == loop_count)
    {
        rtos_stop_timer(mqtt_loop_timer);
        os_free(mqtt_loop_timer);
        mqtt_loop_timer = NULL;
        LOGI("stop\n");
    }

}

void ble_mqtt_loop_event_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    uintptr_t packet_id = (uintptr_t)msg->msg;

    switch (msg->event_type)
    {
        case IOTX_MQTT_EVENT_UNDEF:
            LOGI("undefined event occur.\n");
            break;

        case IOTX_MQTT_EVENT_DISCONNECT:
            LOGI("MQTT disconnect.\n");
            break;

        case IOTX_MQTT_EVENT_RECONNECT:
            LOGI("MQTT reconnect.\n");
            break;

        case IOTX_MQTT_EVENT_SUBCRIBE_SUCCESS:
            LOGI("subscribe success, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_SUBCRIBE_TIMEOUT:
            LOGI("subscribe wait ack timeout, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_SUBCRIBE_NACK:
            LOGI("subscribe nack, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_UNSUBCRIBE_SUCCESS:
            LOGI("unsubscribe success, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_UNSUBCRIBE_TIMEOUT:
            LOGI("unsubscribe timeout, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_UNSUBCRIBE_NACK:
            LOGI("unsubscribe nack, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_PUBLISH_SUCCESS:
            LOGI("publish success, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_PUBLISH_TIMEOUT:
            LOGI("publish timeout, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_PUBLISH_NACK:
            LOGI("publish nack, packet-id=%u.\n", (unsigned int)packet_id);
            break;

        case IOTX_MQTT_EVENT_PUBLISH_RECVEIVED:
#if 0
            iotx_mqtt_topic_info_pt topic_info = (iotx_mqtt_topic_info_pt)msg->msg;
            LOGI("topic message arrived but without any related handle:\n");
            LOGI("    topic=%.*s,\n",
                 topic_info->topic_len,
                 topic_info->ptopic);
            LOGI("    topic_msg=%.*s.\n",
                 topic_info->payload_len,
                 topic_info->payload);
#endif
            break;

        default:
            LOGE("Should NOT arrive here.\n");
            break;
    }
}

#define REQ_TIMEOUT_MS (5000)
#define TRX_BUF_SIZE (7*1024)

void *ble_mqtt_open(const char *host_name, const char *username,
                    const char *password)
{
    iotx_conn_info_pt pconn_info;
    iotx_mqtt_param_t mqtt_params;

    if (ble_mqtt_loop_tx_buffer == NULL)
    {
        ble_mqtt_loop_tx_buffer = (char *)os_malloc(TRX_BUF_SIZE);

        if (ble_mqtt_loop_tx_buffer == NULL)
        {
            LOGE("not enough memory for ble_mqtt_loop_tx_buffer\n");
            goto error;
        }
    }

    if (ble_mqtt_loop_rx_buffer == NULL)
    {
        ble_mqtt_loop_rx_buffer = (char *)os_malloc(TRX_BUF_SIZE);

        if (ble_mqtt_loop_rx_buffer == NULL)
        {
            LOGE("not enough memory for ble_mqtt_loop_rx_buffer\n");
            goto error;
        }
    }

    /* Device AUTH */
    if (0 != IOT_SetupConnInfo(PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET, (void **)&pconn_info))
    {
        LOGE("AUTH request failed!\n");
        goto error;
    }

    os_snprintf(pconn_info->host_name, sizeof(pconn_info->host_name) - 1, "%s", host_name);
    os_snprintf(pconn_info->username, sizeof(pconn_info->username) - 1, "%s", username);
    os_snprintf(pconn_info->password, sizeof(pconn_info->password) - 1, "%s", password);

    /* Initialize MQTT parameter */
    memset(&mqtt_params, 0x0, sizeof(mqtt_params));

    mqtt_params.port = pconn_info->port;
    mqtt_params.host = pconn_info->host_name;
    mqtt_params.client_id = pconn_info->client_id;
    mqtt_params.username = pconn_info->username;
    mqtt_params.password = pconn_info->password;
    mqtt_params.pub_key = pconn_info->pub_key;

    mqtt_params.request_timeout_ms = REQ_TIMEOUT_MS;
    mqtt_params.clean_session = 0;
    mqtt_params.keepalive_interval_ms = 60000;
    mqtt_params.pread_buf = ble_mqtt_loop_rx_buffer;
    mqtt_params.read_buf_size = TRX_BUF_SIZE;
    mqtt_params.pwrite_buf = ble_mqtt_loop_tx_buffer;
    mqtt_params.write_buf_size = TRX_BUF_SIZE;

    mqtt_params.handle_event.h_fp = ble_mqtt_loop_event_handle;
    mqtt_params.handle_event.pcontext = NULL;


    /* Construct a MQTT client with specify parameter */
    return IOT_MQTT_Construct(&mqtt_params);


error:

    if (ble_mqtt_loop_rx_buffer)
    {
        os_free(ble_mqtt_loop_rx_buffer);
        ble_mqtt_loop_rx_buffer = NULL;
    }

    if (ble_mqtt_loop_tx_buffer)
    {
        os_free(ble_mqtt_loop_tx_buffer);
        ble_mqtt_loop_tx_buffer = NULL;
    }

    return NULL;
}

enum
{
    COEX_DEMO_MSG_NULL = 0,
    COEX_DEMO_MSG_TX = 0,
};

typedef struct
{
    uint8_t type;
    uint16_t len;
    char *data;
} coex_demo_msg_t;

beken_queue_t coex_demo_msg_que = NULL;
beken_thread_t coex_demo_thread_handle = NULL;
#define COEX_DEMO_MSG_COUNT          (30)

static void ble_send_data_2_mqtt(uint8 con_idx, uint16_t len, uint8 *data)
{
    coex_demo_msg_t demo_msg;
    char msg_pub[350] = {0};
    int rc = -1;

    os_memset(&demo_msg, 0x0, sizeof(coex_demo_msg_t));
    //os_memset(msg_pub, 0, sizeof(msg_pub));
    if (coex_demo_msg_que == NULL)
    {
        return;
    }

    /* Generate topic message */
    int msg_len = snprintf(msg_pub, sizeof(msg_pub), "{\"link_id\":\"%d\", \"rev_len\":\"%d\", \"rev_data\":\"", con_idx, len);
    if (msg_len < 0)
    {
        LOGE("Error occur! Exit program.\n");
    }

    //LOGI("%s len %d msg_len1 %d %p %s\n", __func__, len, msg_len, &msg_len, msg_pub);
    os_memcpy(&msg_pub[msg_len], data, len);
    msg_len += len;

    msg_pub[msg_len++] = '"';
    msg_pub[msg_len++] = '}';

    //LOGI("total msg_len = %d\n",msg_len);

    demo_msg.data = (char *) os_malloc(msg_len);
    if (demo_msg.data == NULL)
    {
        LOGE("%s, malloc failed\r\n", __func__);
        return;
    }

    os_memcpy(demo_msg.data, msg_pub, msg_len);
    demo_msg.type = COEX_DEMO_MSG_TX;
    demo_msg.len = msg_len;

    rc = rtos_push_to_queue(&coex_demo_msg_que, &demo_msg, BEKEN_NO_WAIT);
    if (kNoErr != rc)
    {
        LOGE("%s, send queue failed\r\n", __func__);
        if (demo_msg.data)
        {
            os_free(demo_msg.data);
        }
    }
}

static void ble_mqtt_loop_recv_handle(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    //iotx_mqtt_topic_info_pt ptopic_info = (iotx_mqtt_topic_info_pt) msg->msg;
    if (loop_type == LT_WIFI_ONLY)
    {
        LOGI("ble mqtt loop recv %d\n", loop_sr_index++);
    }
    else if (loop_type == LT_COEX)
    {
        iotx_mqtt_topic_info_pt ptopic_info = (iotx_mqtt_topic_info_pt) msg->msg;
        uint16_t vaild_len = 0;
        uint8_t con_idx = 0;
        uint8_t *data = NULL;
        char *pos = NULL;

        pos = os_strstr(ptopic_info->payload, "link_id");
        if (pos)
        {
            con_idx = atoi(pos + os_strlen("link_id") + 3);
        }

        pos = os_strstr(ptopic_info->payload, "rev_len");
        if (pos)
        {
            vaild_len = atoi(pos + os_strlen("rev_len") + 3);
        }

        //LOGI("ble mqtt recv payload_len %d, vaild_len %d, con_idx %d\n", ptopic_info->payload_len, vaild_len, con_idx);

        if (ptopic_info->payload_len > vaild_len)
        {
            pos = os_strstr(ptopic_info->payload, "rev_data");
            if (pos)
            {
                data = (uint8_t *)pos + os_strlen("rev_data") + 3;
                //LOGI("recv data[0] %c\n", data[0]);
                if (BK_BLE_HOST_STACK_TYPE_ETHERMIND != bk_ble_get_host_stack_type())
                {
                    bk_ble_gatt_write_value(con_idx, plc_rx_handle[con_idx], vaild_len, data);
                }
                else
                {
                    uint8_t index = 0;
                    if (0 == ethermind_find_index_by_att_con_id(&index, con_idx))
                    {
                        LOGI("%s ATT_CON_ID %d, send data to nordic %s\n", __func__,
                             s_ethermind_nordic_att_info[index],
                             data);


                        bk_ble_att_write(s_ethermind_nordic_att_info[index], s_ethermind_nordic_write_attr_handle[index], data, vaild_len);
                    }
                    else
                    {
                        LOGE("%s cant find att_id %d\n", __func__, con_idx);
                    }
                }

            }
        }

    }
}

#if CONFIG_ARCH_RISCV
extern u64 riscv_get_mtimer(void);
#endif
void coex_demo_main(void *arg)
{
#if CONFIG_ARCH_RISCV
    uint64_t current_delay_max = 0;
    uint8_t packet_sent_count = 0;
#endif
    while (1)
    {
        bk_err_t err;
        coex_demo_msg_t msg;

        err = rtos_pop_from_queue(&coex_demo_msg_que, &msg, BEKEN_WAIT_FOREVER);
        if (kNoErr == err)
        {
            switch (msg.type)
            {
                case COEX_DEMO_MSG_TX:
                {
                    int rc = -1;
                    iotx_mqtt_topic_info_t topic_msg;
#if CONFIG_ARCH_RISCV
                    uint64_t current_time_begin, current_time_end, current_delay = 0;
#endif
                    /* Initialize topic information */
                    os_memset(&topic_msg, 0x0, sizeof(iotx_mqtt_topic_info_t));

                    topic_msg.qos = IOTX_MQTT_QOS0;
                    topic_msg.retain = 0;
                    topic_msg.dup = 0;
                    topic_msg.payload = (void *)msg.data;
                    topic_msg.payload_len = msg.len;
#if CONFIG_ARCH_RISCV
                    //bk_gpio_pull_up(GPIO_2);
                    current_time_begin = riscv_get_mtimer();
#endif
                    rc = IOT_MQTT_Publish(ble_mqtt_loop_client, loop_topic_ci, &topic_msg);
                    if (rc < 0)
                    {
                        LOGE("error occur when publish.\n");
                    }
                    else
                    {
                        //LOGI("packet-id=%u, publish topic msg=%s.\n", (uint32_t)rc, msg.data);
                    }
                    os_free(msg.data);
#if CONFIG_ARCH_RISCV
                    current_time_end = riscv_get_mtimer();
                    current_delay = current_time_end - current_time_begin;

                    if (current_delay > current_delay_max)
                    {
                        current_delay_max = current_delay;
                    }
                    packet_sent_count++;
                    if (packet_sent_count >= 10)
                    {
                        LOGI("mqtt pulish max delay %ld us.\n", (u32)(current_delay_max / 26));
                        packet_sent_count = 0;
                        current_delay_max = 0;
                    }
#endif
                    //bk_gpio_pull_down(GPIO_2);
                }
                break;
                default:
                    break;
            }
        }
    }

    rtos_deinit_queue(&coex_demo_msg_que);
    coex_demo_msg_que = NULL;
    coex_demo_thread_handle = NULL;
    rtos_delete_thread(NULL);
}

int coex_demo_task_init(void)
{
    bk_err_t ret = BK_OK;
    if ((!coex_demo_thread_handle) && (!coex_demo_msg_que))
    {
        ret = rtos_init_queue(&coex_demo_msg_que,
                              "coex_demo_msg_que",
                              sizeof(coex_demo_msg_t),
                              COEX_DEMO_MSG_COUNT);
        if (ret != kNoErr)
        {
            LOGE("coex demo msg queue failed \r\n");
            return BK_FAIL;
        }

        ret = rtos_create_thread(&coex_demo_thread_handle,
                                 BEKEN_DEFAULT_WORKER_PRIORITY,
                                 "coex_demo",
                                 (beken_thread_function_t)coex_demo_main,
                                 4096,
                                 (beken_thread_arg_t)0);
        if (ret != kNoErr)
        {
            LOGE("coex demo task fail \r\n");
            rtos_deinit_queue(&coex_demo_msg_que);
            coex_demo_msg_que = NULL;
            coex_demo_thread_handle = NULL;
        }

        return kNoErr;
    }
    else
    {
        return kInProgressErr;
    }
}

int ble_mqtt_loop_handle(int sync, int argc, char **argv)
{
    int err = kNoErr, i;

    for (i = 0; i < argc; i++)
    {
        LOGI("arg[%d]: %s\n", i, argv[i]);
    }

    if (loop_topic_ci == NULL)
    {
        loop_topic_ci = os_malloc(25);
        os_memset(loop_topic_ci, 0, 25);
    }

    if (loop_topic_sr == NULL)
    {
        loop_topic_sr = os_malloc(25);
        os_memset(loop_topic_sr, 0, 25);
    }


    if (!strcmp(argv[0], "connect"))
    {
        ble_mqtt_loop_client = ble_mqtt_open(argv[1], argv[2], argv[3]);

        if (ble_mqtt_loop_client == NULL)
        {
            LOGE("MQTT connect failed\n");
            goto error;
        }
        else
        {
            LOGI("MQTT connect success\n");
        }
    }

    if (!strcmp(argv[0], "wifi"))
    {
        if (ble_mqtt_loop_client == NULL)
        {
            LOGE("MQTT client was not init\n");
            goto error;
        }

        if (mqtt_loop_timer == NULL)
        {
            mqtt_loop_timer = (beken_timer_t *)os_malloc(sizeof(beken_timer_t));
        }

        uint8_t bt_mac[6];
        bk_get_mac((uint8_t *)bt_mac, MAC_TYPE_BLUETOOTH);

        snprintf(loop_topic_sr, 25, "/ble/lpsr/%02x%02x%02x", bt_mac[3], bt_mac[4], bt_mac[5]);
        snprintf(loop_topic_ci, 25, "/ble/lpci/%02x%02x%02x", bt_mac[3], bt_mac[4], bt_mac[5]);

        loop_interval = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
        loop_count = os_strtoul(argv[2], NULL, 10) & 0xFFFF;
        loop_type = LT_WIFI_ONLY;

        loop_sr_index = 0;
        loop_ci_index = 0;

        LOGI("WIFI only test, interval: %u, count: %u\n", loop_interval, loop_count);
        LOGI("topic: %s:%s\n", loop_topic_sr, loop_topic_ci);

        if (IOT_MQTT_Subscribe(ble_mqtt_loop_client, loop_topic_sr, IOTX_MQTT_QOS0, ble_mqtt_loop_recv_handle, NULL) < 0)
        {

            LOGE("IOT_MQTT_Subscribe() TOPIC_CMD failed\n");
        }


        if (!rtos_is_timer_init(mqtt_loop_timer))
        {
            rtos_init_timer(mqtt_loop_timer, loop_interval, mqtt_loop_timer_callback, NULL);
            rtos_start_timer(mqtt_loop_timer);
        }
    }

    if (!strcmp(argv[0], "stop"))
    {
        if (loop_topic_sr != NULL && ble_mqtt_loop_client != NULL)
        {
            IOT_MQTT_Unsubscribe(ble_mqtt_loop_client, loop_topic_sr);
            LOGI("UnSubscribe\n");
        }

        if (mqtt_loop_timer != NULL)
        {
            rtos_stop_timer(mqtt_loop_timer);
            os_free(mqtt_loop_timer);
            mqtt_loop_timer = NULL;
        }

    }

    if (!os_strcmp(argv[0], "coex"))
    {
        if (ble_mqtt_loop_client == NULL)
        {
            LOGE("MQTT client was not init\n");
            goto error;
        }

        loop_type = LT_COEX;

        uint8_t bt_mac[6];
        bk_get_mac((uint8_t *)bt_mac, MAC_TYPE_BLUETOOTH);

        snprintf(loop_topic_sr, 25, "/ble/lpsr/%02x%02x%02x", bt_mac[3], bt_mac[4], bt_mac[5]);
        snprintf(loop_topic_ci, 25, "/ble/lpci/%02x%02x%02x", bt_mac[3], bt_mac[4], bt_mac[5]);

        loop_sr_index = 0;
        loop_ci_index = 0;

        LOGI("topic: %s:%s", loop_topic_sr, loop_topic_ci);

        if (IOT_MQTT_Subscribe(ble_mqtt_loop_client, loop_topic_sr, IOTX_MQTT_QOS0, ble_mqtt_loop_recv_handle, NULL) < 0)
        {
            LOGE("IOT_MQTT_Subscribe() TOPIC_CMD failed\n");
        }

        coex_demo_task_init();
    }

    atsvr_cmd_rsp_ok();
    return err;

error:
    atsvr_cmd_rsp_error();
    return err;
}
#endif

int ble_power_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;

    if (argc < 1)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    if (os_strcmp(argv[0], "1") == 0)
    {
        bk_bluetooth_init();
    }
    else if (os_strcmp(argv[0], "0") == 0)
    {
        bk_bluetooth_deinit();
    }

    atsvr_cmd_rsp_ok();
    return err;

error:
    atsvr_cmd_rsp_error();
    return err;
}


static uint32_t stability_sent_interval = 0;
static uint16_t stability_sent_data_len = 0;
static uint32_t stability_sent_max = 0;
static beken_timer_t stability_test_tmr;
static uint32 stability_test_sent_count = 0;
static uint32 stability_test_slave_recv_count = 0;

const uint8_t stability_wt_uuid[] = {0xf0, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void show_stability_test_result(void)
{
    uint32 m_sent_total = 0;
    uint32 s_recv_total = 0;

    for (uint8_t j = 0; j < AT_BLE_MAX_CONN; j++)
    {
        if (ble_st_conn_env[j].state)
        {
            m_sent_total += stability_test_sent_count;
            s_recv_total += ble_st_conn_env[j].s_recv_count;
        }
    }
    if(m_sent_total)
    {
        LOGI("stability test result : %d-%d(%d.%02d%%)>\r\n", s_recv_total, m_sent_total,
            (10000 * s_recv_total / m_sent_total) / 100, (10000 * s_recv_total / m_sent_total) % 100);
    }
}

void ble_stability_test_enable_notification_timer_hdl(void *param, unsigned int ulparam)
{
    uint8 con_idx = (uint32)param;

    rtos_deinit_oneshot_timer(&ble_st_conn_env[con_idx].m_tmr);

    bk_ble_gatt_write_ccc(con_idx, (ble_st_conn_env[con_idx].stability_wt_handle + 1), 0x01);
}

static int ble_convert_128b_2_16b_uuid(uint8_t *uuid128, uint16_t *uuid16)
{
    if (uuid128  == NULL || uuid16 == NULL)
    {
        return 1;
    }

    uint8_t base_uuid[16] = {0};

    base_uuid[0U] = 0xFBU;
    base_uuid[1U] = 0x34U;
    base_uuid[2U] = 0x9BU;
    base_uuid[3U] = 0x5FU;
    base_uuid[4U] = 0x80U;
    base_uuid[5U] = 0x00U;
    base_uuid[6U] = 0x00U;
    base_uuid[7U] = 0x80U;
    base_uuid[8U] = 0x00U;
    base_uuid[9U] = 0x10U;
    base_uuid[10U] = 0x00U;
    base_uuid[11U] = 0x00U;
    base_uuid[12U] = uuid128[12U];
    base_uuid[13U] = uuid128[13U];
    base_uuid[14U] = 0x00U;
    base_uuid[15U] = 0x00U;
    if (0 == os_memcmp(uuid128, base_uuid, 16))
    {
        *uuid16 = (uuid128[13U] << 8) | (uuid128[12U]);
        return 0;
    }
    return 1;
}


static void ble_stability_sdp_comm_callback(MASTER_COMMON_TYPE type, uint8 conidx, void *param)
{
    if (MST_TYPE_ATT_UUID == type)
    {
        struct ble_sdp_char_inf *char_inf = (struct ble_sdp_char_inf *)param;

        if ((!os_memcmp(char_inf->uuid, stability_wt_uuid, char_inf->uuid_len)) && (conidx < AT_BLE_MAX_CONN))
        {
            ble_st_conn_env[conidx].stability_wt_handle = char_inf->val_hdl;
        }
    }

    if (MST_TYPE_SDP_END == type)
    {
        bk_ble_gatt_mtu_change(conidx);
        if (!rtos_is_oneshot_timer_init(&ble_st_conn_env[conidx].m_tmr))
        {
            rtos_init_oneshot_timer(&ble_st_conn_env[conidx].m_tmr, 100, (timer_2handler_t)ble_stability_test_enable_notification_timer_hdl, (void *)((uint32)conidx), 0);
            rtos_start_oneshot_timer(&ble_st_conn_env[conidx].m_tmr);
        }
    }

    if (type == MST_TYPE_UPP_ASK)
    {
        struct mst_comm_updata_para *tmp = (typeof(tmp))param;
        LOGI("%s MST_TYPE_UPP_ASK accept\n", __func__);
        tmp->is_agree = 1;
    }
}

static void ble_stability_sdp_charac_callback(CHAR_TYPE type, uint8 conidx, uint16_t hdl, uint16_t len, uint8 *data)
{
    if (CHARAC_NOTIFY == type)
    {
        uint32 s_recv_count = atoi((char *)data);
        ble_st_conn_env[conidx].s_recv_count = s_recv_count;
        LOGI("stability test(link %d) <date_len : %d, PRR : %d-%d(%d.%02d%%)>\r\n", conidx, len, s_recv_count, stability_test_sent_count,
             (10000 * s_recv_count / stability_test_sent_count) / 100, (10000 * s_recv_count / stability_test_sent_count) % 100);

    }
}

static void ble_stability_show_recv_info(uint8_t *value, uint16_t len, uint8 con_idx)
{
    if ((len >= 2) && (0xba == value[0]))
    {
        if (0 == value[1])
        {
            stability_test_s_enabled = 0;
            stability_test_slave_recv_count = 0;
        }
        else if (1 == value[1])
        {
            stability_test_s_enabled = 1;
            stability_test_slave_recv_count = 0;
        }
        else if (2 == value[1])
        {
            if (stability_test_s_enabled)
            {
                value = value + 2;
                stability_test_slave_recv_count++;
                uint32 m_sent_id = atoi((char *)value);
                LOGI("stability test(link %d) <date_len : %d, PRR : %d-%d(%d.%02d%%)>\r\n", con_idx, len, stability_test_slave_recv_count, m_sent_id,
                     (10000 * stability_test_slave_recv_count / m_sent_id) / 100, (10000 * stability_test_slave_recv_count / m_sent_id) % 100);

                char *write_buffer = (char *)os_malloc(len);
                if (!write_buffer)
                {
                    LOGE("%s alloc err\n", __func__);
                    return;
                }

                os_memset(write_buffer, 0, len);
                os_snprintf(write_buffer, len, "%d", stability_test_slave_recv_count);

                ble_err_t ret = bk_ble_send_noti_value(con_idx, len, (uint8_t *)write_buffer, g_test_prf_task_id, TEST_IDX_CHAR_WRITE_TEST_VALUE);
                if (ret != BK_ERR_BLE_SUCCESS)
                {
                    LOGE("%s ret err %d\n", __func__, ret);
                }
                os_free(write_buffer);
            }
            else
            {
                LOGE("%s, stability test is not enabled!\r\n", __func__);
            }
        }
    }
    else
    {
        LOGE("%s, invalid data \r\n", __func__);
    }

}

void ble_stability_test_timer_hdl(void *param)
{
    uint8 *write_buffer = NULL;

    if (stability_sent_max && (stability_sent_max == stability_test_sent_count))
    {
        if (rtos_is_timer_running(&stability_test_tmr))
        {
            rtos_stop_timer(&stability_test_tmr);
        }
        rtos_deinit_timer(&stability_test_tmr);

        show_stability_test_result();
        return;
    }

    write_buffer = (uint8_t *)os_malloc(stability_sent_data_len);

    if (!write_buffer)
    {
        LOGE("%s alloc err\n", __func__);
        return;
    }

    stability_test_sent_count++;

    os_memset(write_buffer, 0, stability_sent_data_len);
    write_buffer[0] = 0xba;
    write_buffer[1] = 0x2;

    os_snprintf((char *)(&write_buffer[2]), stability_sent_data_len, "%d", stability_test_sent_count);
    //sprintf((char*)(&write_buffer[2]), "%d", stability_test_sent_count);

    for (uint8_t j = 0; j < AT_BLE_MAX_CONN; j++)
    {
        if (ble_st_conn_env[j].state && ble_st_conn_env[j].stability_wt_handle)
        {
            if (BK_BLE_HOST_STACK_TYPE_ETHERMIND != bk_ble_get_host_stack_type())
            {
                bk_ble_gatt_write_value(j, ble_st_conn_env[j].stability_wt_handle, stability_sent_data_len, write_buffer);
            }
        }
        else
        {
            //LOGE("the %dth conn is not connected\n",j);
        }
    }

    os_free(write_buffer);
}

int ble_stability_test_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    uint8_t cmd = 0;

    if (bk_ble_get_controller_stack_type() != BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2)
    {
        err = kParamErr;
        goto error;
    }

    if (argc < 1)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    cmd = os_strtoul(argv[0], NULL, 10) & 0xFF;

    if (cmd == 1)
    {
        if (BK_BLE_HOST_STACK_TYPE_ETHERMIND != bk_ble_get_host_stack_type())
        {
            os_memset(&ble_st_conn_env, 0, sizeof(ble_st_conn_env));
            bk_ble_register_app_sdp_common_callback(ble_stability_sdp_comm_callback);
            bk_ble_register_app_sdp_charac_callback(ble_stability_sdp_charac_callback);
            stability_test_m_enabled = 1;
        }
    }
    else if (cmd == 2)
    {
        if (argc < 3)
        {
            LOGE("input param error\n");
            err = kParamErr;
            goto error;
        }

        stability_sent_max = os_strtoul(argv[1], NULL, 10);
        stability_sent_interval = os_strtoul(argv[2], NULL, 10);
        stability_sent_data_len = os_strtoul(argv[3], NULL, 10) & 0xFFFF;

        if (BK_BLE_HOST_STACK_TYPE_ETHERMIND != bk_ble_get_host_stack_type())
        {
            uint8_t cmd[5] = {0xba, 1};

            if (BK_BLE_HOST_STACK_TYPE_ETHERMIND != bk_ble_get_host_stack_type())
            {
                for (uint8_t j = 0; j < AT_BLE_MAX_CONN; j++)
                {
                    if (ble_st_conn_env[j].state && ble_st_conn_env[j].stability_wt_handle)
                    {
                        rtos_delay_milliseconds(5);
                        bk_ble_gatt_write_value(j, ble_st_conn_env[j].stability_wt_handle, sizeof(cmd), cmd);
                    }
                    else
                    {
                        //LOGE("the %dth conn is not connected\n",j);
                    }
                }
            }

            if (!rtos_is_timer_init(&stability_test_tmr))
            {
                stability_test_sent_count = 0;
                rtos_init_timer(&stability_test_tmr, stability_sent_interval, ble_stability_test_timer_hdl, 0);
                rtos_start_timer(&stability_test_tmr);
            }
        }
    }
    else if (cmd == 0)
    {
        stability_test_m_enabled = 0;

        if (rtos_is_timer_init(&stability_test_tmr))
        {
            if (rtos_is_timer_running(&stability_test_tmr))
            {
                rtos_stop_timer(&stability_test_tmr);
            }
            rtos_deinit_timer(&stability_test_tmr);
        }

        uint8_t cmd[5] = {0xba, 0};

        if (BK_BLE_HOST_STACK_TYPE_ETHERMIND != bk_ble_get_host_stack_type())
        {
            for (uint8_t j = 0; j < AT_BLE_MAX_CONN; j++)
            {
                if (ble_st_conn_env[j].state && ble_st_conn_env[j].stability_wt_handle)
                {
                    bk_ble_gatt_write_value(j, ble_st_conn_env[j].stability_wt_handle, sizeof(cmd), cmd);
                }
                else
                {
                    //LOGE("the %dth conn is not connected\n",j);
                }
            }
        }

        show_stability_test_result();
    }

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }
    else
    {
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                return err;
            }
            else
            {
                err = at_cmd_status;
                at_cmd_status = BK_ERR_BLE_SUCCESS;
                goto error;
            }
        }
    }

error:
    atsvr_cmd_rsp_error();
    return err;
}

void ble_stability_test_master_reconnect_timer_hdl(void *param, unsigned int ulparam)
{
    uint8 con_idx = (uint32)param;

    rtos_deinit_oneshot_timer(&ble_st_conn_env[con_idx].m_tmr);

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {

        int err = kNoErr;
        ble_conn_param_t conn_param = {23, 23, 0, 500, 1};

        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            LOGE("%s, error 01!\r\n", __func__);
            return;
        }

        err = bk_ble_create_init(con_idx, &conn_param, ble_at_cmd_cb);
        if (err != 0)
        {
            LOGE("%s, error 02!\r\n", __func__);
            return;
        }

        if (ble_at_cmd_sema != NULL)
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                LOGE("%s, error 03!\r\n", __func__);
                return;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    err = bk_ble_init_set_connect_dev_addr(con_idx, &ble_st_conn_env[con_idx].peer_addr, ble_st_conn_env[con_idx].peer_addr_type);
                    if (err != 0)
                    {
                        LOGE("%s, error 04!\r\n", __func__);
                        return;
                    }

                    err = bk_ble_init_start_conn(con_idx, ble_at_cmd_cb);
                    if (err != 0)
                    {
                        LOGE("%s, error 05!\r\n", __func__);
                        return;
                    }

                    if (ble_at_cmd_sema != NULL)
                    {
                        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
                        if (err == kNoErr)
                        {
                            if (at_cmd_status != BK_ERR_BLE_SUCCESS)
                            {
                                LOGE("%s, error 06!\r\n", __func__);
                                return;
                            }
                        }
                    }
                    else
                    {
                        LOGE("%s, error 07!\r\n", __func__);
                        return;
                    }
                }
                else
                {
                    LOGE("%s, error 08!\r\n", __func__);
                    return;
                }
            }
        }

        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
}

int ble_unregister_service_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    uint16 my_service_uuid = 0;
    struct bk_ble_db_cfg ble_db_cfg;

    if (bk_ble_get_controller_stack_type() != BK_BLE_CONTROLLER_STACK_TYPE_BTDM_5_2)
    {
        err = kParamErr;
        goto error;
    }

    if (argc < 1)
    {
        LOGE("\nThe number of param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    my_service_uuid = os_strtoul(argv[0], NULL, 16) & 0xFFFF;

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        os_memset(&ble_db_cfg, 0, sizeof(ble_db_cfg));

        ble_db_cfg.svc_perm = BK_BLE_PERM_SET(SVC_UUID_LEN, UUID_16);
        os_memcpy(&(ble_db_cfg.uuid[0]), &my_service_uuid, 2);

        if (1)//ble_at_cmd_table[19].is_sync_cmd)
        {
            err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
            if (err != kNoErr)
            {
                goto error;
            }
        }
        bk_ble_set_notice_cb(ble_at_notice_cb);

        err = bk_ble_delete_service(&ble_db_cfg);

        if (err != BK_ERR_BLE_SUCCESS)
        {
            goto error;
        }
        else
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                goto error;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    atsvr_cmd_rsp_ok();
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return err;
                }
                else
                {
                    err = at_cmd_status;
                    at_cmd_status = BK_ERR_BLE_SUCCESS;
                    goto error;
                }
            }
        }
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

int ble_create_bond_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    bd_addr_t bond_addr;
    uint8_t conn_idx;
    uint8_t central_count = 0;

    if (!bk_ble_if_support_central(&central_count) || central_count == 0)
    {
        LOGE("not support central\n");
        err = kParamErr;
        goto error;
    }

    if (argc != 1)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    err = get_addr_from_param(&bond_addr, argv[0]);
    if (err != kNoErr)
    {
        LOGE("input addr param error\n");
        err = kParamErr;
        goto error;
    }

    err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
    if (err != kNoErr)
    {
        LOGE("rtos_init_semaphore error\n");
        goto error;
    }

    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        conn_idx = bk_ble_find_conn_idx_from_addr(&bond_addr);
        if (conn_idx == AT_BLE_MAX_CONN)
        {
            LOGE("ble not connection\n");
            err = kNoResourcesErr;
            goto error;
        }
        bk_ble_set_notice_cb(ble_at_notice_cb);
        err = bk_ble_create_bond(conn_idx, GAP_AUTH_REQ_NO_MITM_BOND, BK_BLE_GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
                                 GAP_SEC1_NOAUTH_PAIR_ENC, GAP_OOB_AUTH_DATA_NOT_PRESENT);
        s_ble_auth_req = 1;
    }
    else
    {
        err = kParamErr;
        goto error;
    }

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, 15000);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                s_ble_auth_req = 0;
                return err;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    s_ble_auth_req = 0;
    at_cmd_status = 0;
    return err;
}

#if 1
int set_ble_device_name_handle_gap(int sync, int argc, char **argv)
{
    uint8_t ret = 0;
    int err = kNoErr;
    if (argc != 1)
    {
        err = kParamErr;
        goto error;
    }
    bk_ble_gap_register_callback(dm_ble_gap_at_cb);
    ret = bk_ble_gap_set_device_name((char *)argv[0]);
    if (ret != BK_OK)
    {
        LOGE("\n name is empty or name_lens over %d\n", BK_BLE_APP_DEVICE_NAME_MAX_LEN);
        goto error;
    }
    atsvr_cmd_rsp_ok();
    return kNoErr;

error:
    atsvr_cmd_rsp_error();
    return err;
}

int get_ble_device_name_handle_gap(int sync, int argc, char **argv)
{
    uint32_t name_len = 0;
    uint8_t ret = 0;
    char name[BK_BLE_APP_DEVICE_NAME_MAX_LEN] = {0};
    int err = kNoErr;

    if (argc != 1)
    {
        err = kParamErr;
        goto error;
    }
    bk_ble_gap_register_callback(dm_ble_gap_at_cb);
    ret = bk_ble_gap_get_device_name(name, &name_len);
    if (ret != BK_OK)
    {
        LOGE("\n name is empty!!!\n");
        goto error;
    }
    atsvr_cmd_rsp_ok();
    return kNoErr;

error:
    atsvr_cmd_rsp_error();
    return err;
}

//AT+BLE=ADVPARAM,7,120,160,0,0,3,1,1
int ble_set_adv_param_handle_gap(int sync, int argc, char **argv)
{
    int err = kNoErr;
    ble_adv_param_t adv_param;

    if (argc != 8)
    {
        LOGE("\nThe count of param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    bk_ble_gap_register_callback(dm_ble_gap_at_cb);
    os_memset(&adv_param, 0, sizeof(ble_adv_param_t));
    adv_param.chnl_map = os_strtoul(argv[0], NULL, 16);
    if (adv_param.chnl_map > 7)
    {
        LOGE("\nThe first(channel_map) param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    adv_param.adv_intv_min = os_strtoul(argv[1], NULL, 16) & 0xFFFFFF;
    adv_param.adv_intv_max = os_strtoul(argv[2], NULL, 16) & 0xFFFFFF;
    if ((adv_param.adv_intv_min > ADV_INTERVAL_MAX || adv_param.adv_intv_min < ADV_INTERVAL_MIN)
        || (adv_param.adv_intv_max > ADV_INTERVAL_MAX || adv_param.adv_intv_max < ADV_INTERVAL_MIN)
        || (adv_param.adv_intv_min > adv_param.adv_intv_max))
    {
        LOGE("input param interval is error\n");
        err = kParamErr;
        goto error;
    }

    adv_param.own_addr_type = os_strtoul(argv[3], NULL, 16) & 0xFF;
    adv_param.adv_type = os_strtoul(argv[4], NULL, 16) & 0xFF;

    if (adv_param.adv_type > 2)
    {
        LOGE("\nThe forth(adv_type) param is wrong!\n");
        err = kParamErr;
        goto error;
    }
    adv_param.adv_prop = os_strtoul(argv[5], NULL, 16) & 0xFFFF;
    adv_param.prim_phy = os_strtoul(argv[6], NULL, 16) & 0xFF;
    if (!(adv_param.prim_phy == 1 || adv_param.prim_phy == 3))
    {
        LOGE("input param prim_phy is error\n");
        err = kParamErr;
        goto error;
    }

    adv_param.second_phy = os_strtoul(argv[7], NULL, 16) & 0xFF;
    if (adv_param.second_phy < 1 || adv_param.second_phy > 3)
    {
        LOGE("input param second_phy is error\n");
        err = kParamErr;
        goto error;
    }

#if 1
    //ADV
    bk_ble_gap_ext_adv_params_t param = {0};
    param.interval_max = adv_param.adv_intv_max;
    param.interval_min = adv_param.adv_intv_min;

    if (adv_param.adv_type == ADV_TYPE_LEGACY)
    {
        param.type = BK_BLE_GAP_SET_EXT_ADV_PROP_LEGACY_IND;
    }
    else if (adv_param.adv_type == ADV_TYPE_EXTENDED)
    {
        param.type = BK_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE;
    }
    //    param.type = adv_param.adv_type;
    param.channel_map = adv_param.chnl_map;
    param.filter_policy = ADV_FILTER_POLICY_ALLOW_SCAN_ANY_CONNECT_ANY;
    param.own_addr_type = adv_param.own_addr_type;
    param.peer_addr_type = BLE_ADDR_TYPE_PUBLIC;
    param.primary_phy = adv_param.prim_phy;
    param.secondary_phy = adv_param.second_phy;
    param.tx_power = 0x7F;
    err = bk_ble_gap_set_adv_params(0, &param);
#else
    //EX_ADV
    bk_ble_gap_ext_adv_params_t param = {0};
    param.interval_max = adv_param.adv_intv_max;
    param.interval_min = adv_param.adv_intv_min;
    param.type = BK_BLE_GAP_SET_EXT_ADV_PROP_CONNECTABLE;
    param.channel_map = adv_param.chnl_map;
    param.filter_policy = ADV_FILTER_POLICY_ALLOW_SCAN_ANY_CONNECT_ANY;
    param.own_addr_type = adv_param.own_addr_type;
    param.peer_addr_type = 0;
    param.primary_phy = adv_param.prim_phy;
    param.secondary_phy = adv_param.second_phy;
    param.tx_power = 0x7F;
    LOGE("%s %x\n", __func__, param.type);
    err = bk_ble_gap_set_adv_params(0, &param);
#endif

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }

    if (ble_at_cmd_sema)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                s_is_adv_set_periodic = 0;
                atsvr_cmd_rsp_ok();

                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }

    }

    return 0;

error:
    atsvr_cmd_rsp_error();
    return err;
}

//AT+BLE=ADVDATARAW,0201060909373233365F424C45,D
int ble_set_adv_data_raw_handle_gap(int sync, int argc, char **argv)
{
    uint8_t adv_data[255] = {0};
    uint8_t adv_len = 0;
    int err = kNoErr;

    if (argc != 2)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    bk_ble_gap_register_callback(dm_ble_gap_at_cb);
    adv_len = os_strtoul(argv[1], NULL, 16) & 0xFF;
    if (adv_len > 255 || adv_len != os_strlen(argv[0]) / 2)
    {
        LOGE("input adv len over limited\n");
        err = kParamErr;
        goto error;
    }

    at_set_data_handle(adv_data, argv[0],  os_strlen(argv[0]));

    err = bk_ble_gap_set_adv_data_raw(0, adv_len, adv_data);
    if (err != BK_ERR_BLE_SUCCESS)
    {
        LOGE(TAG, "%s set adv data raw err %d\n", __func__, err);
        goto error;
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();

                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }



error:
    atsvr_cmd_rsp_error();
    return err;
}

//AT+BLE=ADVDATA
int ble_set_adv_data_handle_gap(int sync, int argc, char **argv)
{
    int err = kNoErr;

    if (argc != 0)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    bk_ble_gap_register_callback(dm_ble_gap_at_cb);
    bk_ble_adv_data_t ble_adv_data = {0};
    ble_adv_data.set_scan_rsp = 0;
    ble_adv_data.include_name = 1;
    ble_adv_data.include_txpower = 1;
    ble_adv_data.min_interval = 0x0006;
    ble_adv_data.max_interval = 0x0010;
    ble_adv_data.appearance = 0x00;
    ble_adv_data.manufacturer_len = 0;
    ble_adv_data.p_manufacturer_data = NULL;
    ble_adv_data.service_data_len = 0;
    ble_adv_data.p_service_data = NULL;
    ble_adv_data.service_uuid_len = 0;
    ble_adv_data.p_service_uuid = NULL;
    ble_adv_data.flag = 0x06;

    err = bk_ble_gap_set_adv_data(&ble_adv_data);
    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }


    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();

                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }


    return 0;

error:
    atsvr_cmd_rsp_error();
    return err;
}

//AT+BLE=SCANRSPRAW,0201060909373233365F424C45,D
int ble_set_scan_rsp_data_raw_handle_gap(int sync, int argc, char **argv)
{
    uint8_t scan_rsp_data[255] = {0};
    uint8_t data_len = 0;
    int err = kNoErr;

    if (argc != 2)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    bk_ble_gap_register_callback(dm_ble_gap_at_cb);
    data_len = os_strtoul(argv[1], NULL, 16) & 0xFF;
    if (data_len > 255 || data_len != os_strlen(argv[0]) / 2)
    {
        LOGE("input adv len over limited\n");
        err = kParamErr;
        goto error;
    }

    at_set_data_handle(scan_rsp_data, argv[0], os_strlen(argv[0]));

    err = bk_ble_gap_set_scan_rsp_data_raw(0, data_len, scan_rsp_data);
    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();

                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

    return 0;

error:
    atsvr_cmd_rsp_error();
    return err;
}

//AT+BLE=SCANRSP
int ble_set_scan_rsp_data_handle_gap(int sync, int argc, char **argv)
{
    int err = kNoErr;

    if (argc != 0)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    bk_ble_gap_register_callback(dm_ble_gap_at_cb);
    bk_ble_adv_data_t ble_adv_data = {0};
    ble_adv_data.set_scan_rsp = 1;
    ble_adv_data.include_name = 1;
    ble_adv_data.include_txpower = 1;
    ble_adv_data.min_interval = 0x0006;
    ble_adv_data.max_interval = 0x0010;
    ble_adv_data.appearance = 0x00;
    ble_adv_data.manufacturer_len = 0;
    ble_adv_data.p_manufacturer_data = NULL;
    ble_adv_data.service_data_len = 0;
    ble_adv_data.p_service_data = NULL;
    ble_adv_data.service_uuid_len = 0;
    ble_adv_data.p_service_uuid = NULL;
    ble_adv_data.flag = 0x06;

    err = bk_ble_gap_set_adv_data(&ble_adv_data);
    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

    return 0;

error:
    atsvr_cmd_rsp_error();
    return err;
}

//AT+BLE=ADVENABLE,1
int ble_set_adv_enable_handle_gap(int sync, int argc, char **argv)
{
    int enable = 0;
    int err = kNoErr;

    if (argc != 1)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    bk_ble_gap_register_callback(dm_ble_gap_at_cb);

    if (os_strcmp(argv[0], "1") == 0)
    {
        enable = 1;
    }
    else if (os_strcmp(argv[0], "0") == 0)
    {
        enable = 0;
    }
    else
    {
        LOGE("the input param is error\n");
        err = kParamErr;
        goto error;
    }

    bk_ble_gap_ext_adv_t ext_adv_param = {0};

    if (enable)
    {
        ext_adv_param.duration = 0;
        ext_adv_param.instance = 0;
        ext_adv_param.max_events = 0;
        err = bk_ble_gap_adv_start(1, &ext_adv_param);
    }
    else
    {
        uint8_t instance = 0;
        err = bk_ble_gap_adv_stop(1, &instance);
    }

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }
    return 0;

error:
    atsvr_cmd_rsp_error();
    return err;
}

//AT+BLE=SCANPARAM,0,1,64,1E
int ble_set_scan_param_handle_gap(int sync, int argc, char **argv)
{
    int err = kNoErr;
    ble_scan_param_t scan_param;

    if (argc < 4)
    {
        LOGE("\nThe number of param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    bk_ble_gap_register_callback(dm_ble_gap_at_cb);
    os_memset(&scan_param, 0, sizeof(ble_scan_param_t));
    scan_param.own_addr_type = os_strtoul(argv[0], NULL, 16) & 0xFF;
    scan_param.scan_phy = os_strtoul(argv[1], NULL, 16) & 0xFF;

    if (!(scan_param.scan_phy & (PHY_1MBPS_BIT | PHY_CODED_BIT)))
    {
        LOGE("\nThe scan phy param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    scan_param.scan_intv = os_strtoul(argv[2], NULL, 16) & 0xFFFF;
    scan_param.scan_wd = os_strtoul(argv[3], NULL, 16) & 0xFFFF;

    if (argc >= 5)
    {
        scan_param.scan_type = os_strtoul(argv[4], NULL, 16) & 0xFF;
    }

    if (scan_param.scan_intv < SCAN_INTERVAL_MIN || scan_param.scan_intv > SCAN_INTERVAL_MAX ||
        scan_param.scan_wd < SCAN_WINDOW_MIN || scan_param.scan_wd > SCAN_WINDOW_MAX ||
        scan_param.scan_intv < scan_param.scan_wd)
    {
        LOGE("\nThe second/third param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    //bk_ble_set_event_callback(dm_ble_at_event_cb);
    bk_ble_ext_scan_params_t param = {0};
    param.own_addr_type = scan_param.own_addr_type;
    param.cfg_mask = scan_param.scan_phy;
    param.uncoded_cfg.scan_interval = scan_param.scan_intv;
    param.uncoded_cfg.scan_window = scan_param.scan_wd;
    param.uncoded_cfg.scan_type = 0;
    param.coded_cfg.scan_interval = scan_param.scan_intv;
    param.coded_cfg.scan_window = scan_param.scan_wd;
    param.coded_cfg.scan_type = 0;
    param.filter_policy = 0;
    param.scan_duplicate = 0;

    err = bk_ble_gap_set_scan_params(&param);

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }


    return err;

error:
    atsvr_cmd_rsp_error();
    return err;
}

//AT+BLE=SCANENABLE,1,0,0,10
int ble_set_scan_enable_handle_gap(int sync, int argc, char **argv)
{
    int enable = 0;
    int err = kNoErr;

    uint8_t filt_duplicate = 0;
    uint16_t duration = 0;
    uint16_t period = 10;
    (void)(filt_duplicate);

    if (argc < 1)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    bk_ble_gap_register_callback(dm_ble_gap_at_cb);
    if (os_strcmp(argv[0], "1") == 0)
    {
        enable = 1;
    }
    else if (os_strcmp(argv[0], "0") == 0)
    {
        enable = 0;
    }
    else
    {
        LOGE("the input param is error\n");
        err = kParamErr;
        goto error;
    }

    if (enable)
    {
        if (argc == 1)
        {

        }
        else if (argc >= 1 + 3)
        {
            filt_duplicate = os_strtoul(argv[1], NULL, 16) & 0xFF;
            duration = os_strtoul(argv[2], NULL, 16) & 0xFFFF;
            period = os_strtoul(argv[3], NULL, 16) & 0xFFFF;
        }
        else
        {
            LOGE("the input param is error\n");
            err = kParamErr;
            goto error;
        }
    }

    //bk_ble_set_event_callback(dm_ble_at_event_cb);
    if (enable == 1)
    {
        err = bk_ble_gap_start_scan(duration, period);
    }
    else
    {
        err = bk_ble_gap_stop_scan();
    }

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();

                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

    return 0;

error:
    atsvr_cmd_rsp_error();
    return err;
}

//AT+BLE=PERADVPARAM,7,120,160,0,0,3,1,1
int ble_set_per_adv_param_handle_gap(int sync, int argc, char **argv)
{
    int err = kNoErr;
    ble_adv_param_t adv_param;

    if (argc != 8)
    {
        LOGE("\nThe count of param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    bk_ble_gap_register_callback(dm_ble_gap_at_cb);

    os_memset(&adv_param, 0, sizeof(ble_adv_param_t));
    adv_param.chnl_map = os_strtoul(argv[0], NULL, 16);
    if (adv_param.chnl_map > 7)
    {
        LOGE("\nThe first(channel_map) param is wrong!\n");
        err = kParamErr;
        goto error;
    }

    adv_param.adv_intv_min = os_strtoul(argv[1], NULL, 16) & 0xFFFFFF;
    adv_param.adv_intv_max = os_strtoul(argv[2], NULL, 16) & 0xFFFFFF;
    if ((adv_param.adv_intv_min > ADV_INTERVAL_MAX || adv_param.adv_intv_min < ADV_INTERVAL_MIN)
        || (adv_param.adv_intv_max > ADV_INTERVAL_MAX || adv_param.adv_intv_max < ADV_INTERVAL_MIN)
        || (adv_param.adv_intv_min > adv_param.adv_intv_max))
    {
        LOGE("input param interval is error\n");
        err = kParamErr;
        goto error;
    }

    adv_param.own_addr_type = os_strtoul(argv[3], NULL, 16) & 0xFF;
    adv_param.adv_type = os_strtoul(argv[4], NULL, 16) & 0xFF;

    if (adv_param.adv_type > 2)
    {
        LOGE("\nThe forth(adv_type) param is wrong!\n");
        err = kParamErr;
        goto error;
    }
    adv_param.adv_prop = os_strtoul(argv[5], NULL, 16) & 0xFFFF;
    adv_param.prim_phy = os_strtoul(argv[6], NULL, 16) & 0xFF;
    if (!(adv_param.prim_phy == 1 || adv_param.prim_phy == 3))
    {
        LOGE("input param prim_phy is error\n");
        err = kParamErr;
        goto error;
    }

    adv_param.second_phy = os_strtoul(argv[7], NULL, 16) & 0xFF;
    if (adv_param.second_phy < 1 || adv_param.second_phy > 3)
    {
        LOGE("input param second_phy is error\n");
        err = kParamErr;
        goto error;
    }

    //first set ext adv param
    bk_ble_gap_ext_adv_params_t ext_adv_param = {0};

    ext_adv_param.interval_max = adv_param.adv_intv_max;
    ext_adv_param.interval_min = adv_param.adv_intv_min;
    ext_adv_param.type = BK_BLE_GAP_SET_EXT_ADV_PROP_NONCONN_NONSCANNABLE_UNDIRECTED;

    ext_adv_param.channel_map = adv_param.chnl_map;
    ext_adv_param.filter_policy = ADV_FILTER_POLICY_ALLOW_SCAN_ANY_CONNECT_ANY;
    ext_adv_param.own_addr_type = adv_param.own_addr_type;
    ext_adv_param.peer_addr_type = BLE_ADDR_TYPE_PUBLIC;
    ext_adv_param.primary_phy = adv_param.prim_phy;
    ext_adv_param.secondary_phy = adv_param.second_phy;
    ext_adv_param.tx_power = 0x7F;
    err = bk_ble_gap_set_adv_params(0, &ext_adv_param);

    if (err != BK_ERR_BLE_SUCCESS)
    {
        bt_at_loge("set adv param err %d", err);
        goto error;
    }

    err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
    if (err != kNoErr)
    {
        bt_at_loge("wait set adv param err %d", err);
        goto error;
    }

    //second set periodic adv param
    bk_ble_gap_periodic_adv_params_t param = {0};
    param.interval_max = adv_param.adv_intv_max;
    param.interval_min = adv_param.adv_intv_min;
    //param.properties = adv_param.adv_prop;

    err = bk_ble_gap_set_periodic_adv_params(0, &param);

    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                s_is_adv_set_periodic = 1;
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

    return 0;

error:
    atsvr_cmd_rsp_error();
    return err;
}

//AT+BLECMD=PERADVDATA,data,length
int ble_set_per_adv_data_handle_gap(int sync, int argc, char **argv)
{
    uint8_t adv_data[255];
    uint8_t adv_len = 0;
    int err = kNoErr;

    if (argc != 2)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    bk_ble_gap_register_callback(dm_ble_gap_at_cb);
    adv_len = os_strtoul(argv[1], NULL, 16) & 0xFF;
    if (adv_len > 255 || adv_len != os_strlen(argv[0]) / 2)
    {
        LOGE("input adv len over limited\n");
        err = kParamErr;
        goto error;
    }

    at_set_data_handle(adv_data, argv[0],  os_strlen(argv[0]));

    err = bk_ble_gap_set_periodic_adv_data_raw(0, adv_len, adv_data);
    if (err != BK_ERR_BLE_SUCCESS)
    {
        goto error;
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }

    return 0;

error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

int ble_set_per_adv_enable_handle_gap(int sync, int argc, char **argv)
{
    int enable = 0;
    int err = kNoErr;

    if (argc != 1)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }

    if (os_strcmp(argv[0], "1") == 0)
    {
        enable = 1;
    }
    else if (os_strcmp(argv[0], "0") == 0)
    {
        enable = 0;
    }
    else
    {
        LOGE("the input param is error\n");
        err = kParamErr;
        goto error;
    }

    if (enable == 1)
    {
        err = bk_ble_gap_periodic_adv_start(0);
    }
    else
    {
        err = bk_ble_gap_periodic_adv_stop(0);
    }

    if (err != BK_ERR_BLE_SUCCESS)
    {
        bt_at_loge("set periodic adv enable err %d", err);
        goto error;
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);

        if (err != kNoErr)
        {
            bt_at_loge("wait set periodic adv enable err %d", err);
            goto error;
        }
    }

    //second set ext adv enable
    bk_ble_gap_ext_adv_t ext_adv_param = {0};

    if (enable)
    {
        ext_adv_param.duration = 0;
        ext_adv_param.instance = 0;
        ext_adv_param.max_events = 0;
        err = bk_ble_gap_adv_start(1, &ext_adv_param);
    }
    else
    {
        uint8_t instance = 0;
        err = bk_ble_gap_adv_stop(1, &instance);
    }

    if (err != BK_ERR_BLE_SUCCESS)
    {
        bt_at_loge("set ext adv enable err %d", err);
        goto error;
    }

    if (ble_at_cmd_sema != NULL)
    {
        err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
        if (err != kNoErr)
        {
            bt_at_loge("wait set ext adv enable err %d", err);
            goto error;
        }
        else
        {
            if (at_cmd_status == BK_ERR_BLE_SUCCESS)
            {
                atsvr_cmd_rsp_ok();
                rtos_deinit_semaphore(&ble_at_cmd_sema);
                return 0;
            }
            else
            {
                err = at_cmd_status;
                goto error;
            }
        }
    }
    return 0;

error:
    atsvr_cmd_rsp_error();
    return err;
}

#endif

beken_semaphore_t *ble_at_get_sema_handle(void)
{
    return &ble_at_cmd_sema;
}

static int ble_add_white_list_handle(int sync, int argc, char **argv)
{
    uint8_t addr_type = 0;
    bd_addr_t bdaddr;
    int err = kNoErr;
    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }
    err = get_addr_from_param(&bdaddr, argv[0]);
    if (err != kNoErr)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }
    if(argc > 1)
    {
        addr_type = os_strtoul(argv[1], NULL, 10);
    }
    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_ble_set_notice_cb(ble_at_notice_cb);
        LOGI("add whl: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x, addr_type:%d\n", 
                                bdaddr.addr[0], bdaddr.addr[1], bdaddr.addr[2], bdaddr.addr[3], bdaddr.addr[4], bdaddr.addr[5],
                                addr_type);
        err = bk_ble_add_devices_to_while_list(&bdaddr, addr_type);
        if(err)
        {
            goto error;
        }
        if (ble_at_cmd_sema != NULL)
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                bt_at_loge("wait add white list err %d", err);
                goto error;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    atsvr_cmd_rsp_ok();
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return 0;
                }
                else
                {
                    err = at_cmd_status;
                    goto error;
                }
            }
        }
        return 0;
    }else
    {
        LOGE("do not support!!");
    }
error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

static int ble_rmv_white_list_handle(int sync, int argc, char **argv)
{
    uint8_t addr_type = 0;
    bd_addr_t bdaddr;
    int err = kNoErr;
    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }
    err = get_addr_from_param(&bdaddr, argv[0]);
    if (err != kNoErr)
    {
        LOGE("input param error\n");
        err = kParamErr;
        goto error;
    }
    if(argc > 1)
    {
        addr_type = os_strtoul(argv[1], NULL, 10);
    }
    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_ble_set_notice_cb(ble_at_notice_cb);
        LOGI("add whl: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x, addr_type:%d\n", 
                                bdaddr.addr[0], bdaddr.addr[1], bdaddr.addr[2], bdaddr.addr[3], bdaddr.addr[4], bdaddr.addr[5],
                                addr_type);
        err = bk_ble_remove_devices_from_while_list(&bdaddr, addr_type);
        if(err)
        {
            goto error;
        }
        if (ble_at_cmd_sema != NULL)
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                bt_at_loge("wait remove white list err %d", err);
                goto error;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    atsvr_cmd_rsp_ok();
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return 0;
                }
                else
                {
                    err = at_cmd_status;
                    goto error;
                }
            }
        }
        return 0;
    }else
    {
        LOGE("do not support!!");
    }
error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

static int ble_cle_white_list_handle(int sync, int argc, char **argv)
{
    int err = kNoErr;
    if (sync)
    {
        err = rtos_init_semaphore(&ble_at_cmd_sema, 1);
        if (err != kNoErr)
        {
            goto error;
        }
    }
    
    if (bk_ble_get_host_stack_type() != BK_BLE_HOST_STACK_TYPE_ETHERMIND)
    {
        bk_ble_set_notice_cb(ble_at_notice_cb);
        LOGI("clear white list \n");
        err = bk_ble_clear_white_list();
        if(err)
        {
            goto error;
        }
        if (ble_at_cmd_sema != NULL)
        {
            err = rtos_get_semaphore(&ble_at_cmd_sema, AT_SYNC_CMD_TIMEOUT_MS);
            if (err != kNoErr)
            {
                bt_at_loge("wait clear white list err %d", err);
                goto error;
            }
            else
            {
                if (at_cmd_status == BK_ERR_BLE_SUCCESS)
                {
                    atsvr_cmd_rsp_ok();
                    rtos_deinit_semaphore(&ble_at_cmd_sema);
                    return 0;
                }
                else
                {
                    err = at_cmd_status;
                    goto error;
                }
            }
        }
        return 0;
    }else
    {
        LOGE("do not support!!");
    }
error:
    atsvr_cmd_rsp_error();
    if (ble_at_cmd_sema != NULL)
    {
        rtos_deinit_semaphore(&ble_at_cmd_sema);
    }
    return err;
}

const struct _atsvr_command ble_cmds_table[] =
{
    //ATSVR_CMD_HADLER("AT+BLE_GETBLENAME", 1, "AT+BLE_GETBLENAME = return ble name", NULL,get_ble_name_handle,true,AT_SYNC_CMD_TIMEOUT_MS,0,NULL,false),
    //ATSVR_CMD_HADLER("AT+BLE_SETBLENAME", 0, "return ble name", NULL,set_ble_name_handle,false,0,0,NULL,false),
    //ATSVR_CMD_HADLER("AT+BLE_GETBLENAME", 0, "return ble name", NULL,get_ble_name_handle,false,0,0,NULL,false),

    ATSVR_CMD_HADLER("AT+BLECREATECONNECT", "create connection:AT+BLECREATECONNECT=<param1>,...,<param6>",
                     NULL, ble_create_connect_handle, true, AT_SYNC_CMD_TIMEOUT_MS, 0, NULL, false),
    ATSVR_CMD_HADLER("AT+BLECANCELCONNECT", "cancel create connection:AT+BLECANCELCONNECT",
                     NULL, ble_cancel_create_connect_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEDISCONNECT", "disconnect current connection:AT+BLEDISCONNECT=<param1>",
                     NULL, ble_disconnect_handle, true, AT_DISCON_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLECONNPARAM", "update connection param:AT+BLECONNPARAM=<param1>,<param2>,<param3>,<param4>,<param5>,[param6]",
                     NULL, ble_update_conn_param_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLECONNECTSTATE", "get connection state:AT+BLECONNECTSTATE=<param1>",
                     NULL, ble_get_conn_state_handle, false, 0, 0, NULL, false),
#ifdef CONFIG_BT
    ATSVR_CMD_HADLER("AT+BLELOCALADDR", "get ble local address:AT+BLELOCALADDR",
                     ble_get_local_addr_handle, ble_set_local_addr_handle, false, 0, 0, NULL, false),
#else
    ATSVR_CMD_HADLER("AT+BLELOCALADDR", "get ble local address:AT+BLELOCALADDR",
                     ble_get_local_addr_handle, ble_set_local_addr_handle, false, 0, 0, NULL, false),
#endif
    //ATSVR_CMD_HADLER("AT+BLELOCALADDR","set ble local address:AT+BLE_SETLOCALADDR=<param1>",
    //              NULL,ble_set_local_addr_handle,false,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BLEREGISTERSERVICE", "register a service:AT+BLEREGISTERSERVICE=1,<ff11>,<ff12>",
                     NULL, ble_register_noti_service_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEPHY", "read current tx/rx phy:AT+BLEPHY=<Param1>",
                     ble_read_phy_handle, ble_set_phy_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    //ATSVR_CMD_HADLER("AT+BLE_SETPHY","set current tx/rx phy:AT+BLE_SETPHY=<Param1>,...,<Param4>",
    //              NULL,ble_set_phy_handle,true,AT_SYNC_CMD_TIMEOUT_MS,true,NULL,false),
    ATSVR_CMD_HADLER("AT+BLEDELETEADV", "delete adv activity:AT+BLEDELETEADV",
                     NULL, ble_delete_adv_activity_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEDELETESCAN", "delete scan activity:AT+BLEDELETESCAN",
                     NULL, ble_delete_scan_activity_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEREGPERFORMANCESERVICE", "register performance service: <task id> <service uuid> <char uuid> [data_len uuid] [interv uuid]:AT+BLEREGPERFORMANCESERVICE=1,0x1234,0x5678",
                     NULL, ble_register_performance_service_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLETXTESTPARAM", "set tx test param: <data_len> <interv>:AT+BLETXTESTPARAM=<param1>,<param2>",
                     NULL, ble_tx_test_param_handle, false, 0, 0, NULL, false),
    ATSVR_CMD_HADLER("AT+BLETXTESTENABLE", "enable tx test: <1|0>:AT+BLETXTESTENABLE=<param1>",
                     NULL, ble_tx_test_enable_handle, false, 0, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEUPDATEMTU2MAX", "update mtu 2 max: <addr>:AT+BLEUPDATEMTU2MAX=<param1>",
                     NULL, ble_update_mtu_2_max_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEBOARDING", "boarding:AT+BLEBOARDING",
                     NULL, ble_boarding_handle, false, 0, 0, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEPERFORMANCESTATISTICENABLE", "enable performance statistic, <tx|rx> <1|0>:AT+BLEPERFORMANCESTATISTICENABLE=<tx|rx>,<param2>",
                     NULL, ble_enable_performance_statistic_handle, false, 1000, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEMAXMTU", "set att maximal mtu:AT+BLEMAXMTU=<param1>",
                     NULL, ble_set_max_mtu_handle, true, 0, 0, NULL, false),
    ATSVR_CMD_HADLER("AT+BLECONNECTBYNAME", "create connection by name:AT+BLECONNECTBYNAME=<param1>,<param2>,<param3>,<param4>,<param5>",
                     NULL, ble_connect_by_name_handle, true, 10000, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEDISCONNECTBYNAME", "disconnect current connection by name:AT+BLEDISCONNECTBYNAME=<param1>",
                     NULL, ble_disconnect_by_name_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLECREATEPERIODICSYNC", "create periodic sync:AT+BLECREATEPERIODICSYNC",
                     NULL, ble_create_periodic_sync_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEPERIODICSYNCENABLE", "set periodic sync start:AT+BLEPERIODICSYNCENABLE=<param1>,<param2>,<param3>,<param4>,<param5>,<param6>,<param7>",
                     NULL, ble_start_periodic_sync_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEPERIODICSYNCDISABLE", "set periodic sync stop:AT+BLEPERIODICSYNCDISABLE=<param1>",
                     NULL, ble_stop_periodic_sync_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEGATTCWR", "att write <ATT_CON_ID> <ATT_ATTR_HANDLE> <value>:AT+BLEGATTCWR=<param1>,<param2>,<param3>",
                     NULL, ble_att_write_handle, false, 0, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEPLRTESTENABLE", "enable packet loss ratio test:AT+BLEPLRTESTENABLE=<param1>[,<param2>,<param3>,<param4><param5>]",
                     NULL, ble_enable_packet_loss_ratio_test_handle, false, 0, 0, NULL, false),
#ifdef CONFIG_ALI_MQTT
    ATSVR_CMD_HADLER("AT+BLEMQTTLOOPBACK", "enable mqtt loopback test:AT+BLEMQTTLOOPBACK=<param1>,<param2>,<param3>",
                     NULL, ble_mqtt_loop_handle, false, 0, true, NULL, false),
#endif
    ATSVR_CMD_HADLER("AT+BLEPOWER", "power on/off:AT+BLEPOWER=<param1>",
                     NULL, ble_power_handle, false, 0, 0, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEATTREAD", "att read <DEVICE_HANDLE> <ATT_CON_ID> <ATT_ATTR_HANDLE>:AT+BLEATTREAD=<param1>,<param2>,<param3>",
                     NULL, ble_att_read_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLESTABILITYTEST", "stability test:AT+BLESTABILITYTEST=<param1>,<param2>,<param3>",
                     NULL, ble_stability_test_handle, false, 0, true, NULL, false),

    /*************************add for new at cmd format*****************************/
    //ATSVR_CMD_HADLER("AT+BLE_ATVERSION", 0, "set at cmd version",
    //              NULL,ble_set_atcmd_version_handle,false,0,0,NULL,false),

#if 1
#ifdef CONFIG_BT
    ATSVR_CMD_HADLER("AT+BLENAME", "return ble name.GET:AT+BLENAME=?;SET:AT+BLENAME=<param1>",
                     get_ble_name_handle, set_ble_name_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    //ATSVR_CMD_HADLER("AT+BLE_BLENAME?", 1, "return ble name", NULL,get_ble_name_handle,false,0,0,NULL,false),
#else
    ATSVR_CMD_HADLER("AT+BLENAME", "return ble name.GET:AT+BLENAME=?;SET:AT+BLENAME=<param1>",
                     get_ble_name_handle, set_ble_name_handle, false, 0, 0, NULL, false),
    // ATSVR_CMD_HADLER("AT+BLE_BLENAME?", 0, "return ble name", NULL,get_ble_name_handle,false,0,0,NULL,false),
#endif
    ATSVR_CMD_HADLER("AT+BLEADVPARAM", "help:AT+BLEADVPARAM=map, min_intval, max_intval, local_addr_type, adv_type, adv_properties, prim_phy, second_phy",
                     NULL, ble_set_adv_param_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEADVDATA", "set adv data:AT+BLEADVDATA=<data>,<length>",
                     NULL, ble_set_adv_data_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEPERADVDATA", "set perodic adv data:AT+BLEPERADVDATA=<data>,<length>",
                     NULL, ble_set_per_adv_data_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLESCANRSPDATA", "set scan response data:AT+BLESCANRSPDATA=<param1>,<param2>",
                     NULL, ble_set_scan_rsp_data_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEADVENABLE", "set adv enable(1)/disable(0):AT+BLEADVENABLE=<param1>",
                     NULL, ble_set_adv_enable_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLESCANPARAM", "set scan param:AT+BLESCANPARAM=<param1>,<param2>,<param3>,<param4>",
                     NULL, ble_set_scan_param_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLESCANENABLE", "set scan enable(1)/disable(0):AT+BLESCANENABLE=<param1>",
                     NULL, ble_set_scan_enable_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLECONNPARAM", "update connection param:AT+BLECONNPARAM=<param1>,<param2>,<param3>,<param4>,<param5>,[param6]",
                     NULL, ble_update_conn_param_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),

    ATSVR_CMD_HADLER("AT+BLECONNECTSTATE", "get connection state:AT+BLECONNECTSTATE=<param1>",
                     NULL, ble_get_conn_state_handle, false, 0, 0, NULL, false),
#ifdef CONFIG_BT
    ATSVR_CMD_HADLER("AT+BLELOCALADDR", "get ble local address:=AT+BLELOCALADDR=? SET:AT+BLELOCALADDR=<param1>",
                     ble_get_local_addr_handle, ble_set_local_addr_handle, false, 0, 0, NULL, false),
#else
    ATSVR_CMD_HADLER("AT+BLELOCALADDR", "get ble local address:=AT+BLELOCALADDR=? SET:AT+BLELOCALADDR=<param1>",
                     ble_get_local_addr_handle, ble_set_local_addr_handle, false, 0, 0, NULL, false),
#endif
    //  ATSVR_CMD_HADLER("AT+BLE_LOCALADDR", 0, "set ble local address",
    //               NULL,ble_set_local_addr_handle,false,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BLEPHY", "read current tx/rx phy:AT+BLEPHY=?,<param1>,<param2>; set current tx/rx phy:AT+BLEPHY=<param1>,<param2>,<param3>,<param4>",
                     ble_read_phy_handle, ble_set_phy_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    //ATSVR_CMD_HADLER("AT+BLE_PHY","set current tx/rx phy:AT+BLE_PHY=<Param1>,...,<Param4>",
    //              NULL,ble_set_phy_handle,true,AT_SYNC_CMD_TIMEOUT_MS,true,NULL,false),
    ATSVR_CMD_HADLER("AT+BLETXTESTPARAM", "set tx test param: <data_len> <interv>:AT+BLETXTESTPARAM=<param1>,<param2>",
                     NULL, ble_tx_test_param_handle, false, 0, 0, NULL, false),
    ATSVR_CMD_HADLER("AT+BLETXTESTENABLE", "enable tx test: <1|0>:AT+BLETXTESTENABLE=<param1>",
                     NULL, ble_tx_test_enable_handle, false, 1000, true, NULL, false), //use timer
    ATSVR_CMD_HADLER("AT+BLEMTU2MAX", "update mtu 2 max: <addr>:AT+BLEMTU2MAX=<param1>",
                     NULL, ble_update_mtu_2_max_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),

    ATSVR_CMD_HADLER("AT+BLEMAXMTU", "set att maximal mtu:AT+BLEMAXMTU=<param1>",
                     NULL, ble_set_max_mtu_handle, true, 0, 0, NULL, false),
    /******************************************************************************/

    ATSVR_CMD_HADLER("AT+BLEUNREGISTERSERVICE", "unregister a service:AT+BLEUNREGISTERSERVICE=<param1>",
                     NULL, ble_unregister_service_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEBOND", "Create Bluetooth Bonding:AT+BLEBOND",
                     NULL, ble_create_bond_handle, true, AT_SYNC_CMD_TIMEOUT_MS, true, NULL, false),

    ATSVR_CMD_HADLER("AT+BLEDISCOVERSERVICE", "discover service, <conn_id>,<start_handle>,<end_handle>[,<16bits_uuid>,[<att_req_uuid_len>]]",
                     NULL, ble_discover_service_handle, true, 5000, true, NULL, false),

    ATSVR_CMD_HADLER("AT+BLEADDWHL", "add white list, <addr>,<addr_type>",
                     NULL, ble_add_white_list_handle, true, 5000, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLERMVWHL", "remove white list, <addr>,<addr_type>",
                     NULL, ble_rmv_white_list_handle, true, 5000, true, NULL, false),
    ATSVR_CMD_HADLER("AT+BLECLEWHL", "clear white list",
                     NULL, ble_cle_white_list_handle, true, 5000, true, NULL, false),

    //#endif
#else
    ATSVR_CMD_HADLER("AT+BLEBLENAME", "return ble name", get_ble_device_name_handle_gap, set_ble_device_name_handle_gap, false, 0, 0, NULL, false),
    //ATSVR_CMD_HADLER("AT+BLE_BLENAME?","return ble name", NULL,get_ble_device_name_handle_gap,false,0,0,NULL,false),
    ATSVR_CMD_HADLER("AT+BLEADVPARAM", "help", NULL, ble_set_adv_param_handle_gap, false, 0, 0, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEADVDATARAW", "set adv data raw", NULL, ble_set_adv_data_raw_handle_gap, false, 0, 0, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEADVDATA", "set adv data", NULL, ble_set_adv_data_handle_gap, false, 0, 0, NULL, false),
    ATSVR_CMD_HADLER("AT+BLESCANRSPDATARAW", "set scan response data raw", NULL, ble_set_scan_rsp_data_raw_handle_gap, false, 0, 0, NULL, false),
    ATSVR_CMD_HADLER("AT+BLESCANRSPDATA", "set scan response data", NULL, ble_set_scan_rsp_data_handle_gap, false, 0, 0, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEADVENABLE", "set adv enable(1)/disable(0)", NULL, ble_set_adv_enable_handle_gap, false, 0, 0, NULL, false),

    ATSVR_CMD_HADLER("AT+BLESCANPARAM", "set scan param", NULL, ble_set_scan_param_handle_gap, false, 0, 0, NULL, false),
    ATSVR_CMD_HADLER("AT+BLESCANENABLE", "set scan enable(1)/disable(0)", NULL, ble_set_scan_enable_handle_gap, false, 0, 0, NULL, false),

    ATSVR_CMD_HADLER("AT+BLEPERADVPARAM", "set perodic adv param", NULL, ble_set_per_adv_param_handle_gap, false, 0, 0, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEPERADVDATA", "set perodic adv data", NULL, ble_set_per_adv_data_handle_gap, false, 0, 0, NULL, false),
    ATSVR_CMD_HADLER("AT+BLEPERADVENABLE", "set perodic adv enable(1)/disable(0)", NULL, ble_set_per_adv_enable_handle_gap, false, 0, 0, NULL, false),
#endif
};

#define BLE_AT_CNT sizeof(ble_cmds_table) / sizeof(ble_cmds_table[0])


void ble_at_cmd_init(void)
{
    int ret;
    ret = atsvr_register_commands(ble_cmds_table, sizeof(ble_cmds_table) / sizeof(ble_cmds_table[0]), "ble", NULL);
    if (0 == ret)
    {
        BK_LOGI(TAG, "BLE AT CMDS INIT OK\r\n");
    }
}
#endif

