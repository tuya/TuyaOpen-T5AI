#include "os/mem.h"
#include "os/str.h"
#include "os/os.h"

#include "components/bluetooth/bk_ble_types.h"
#include "components/bluetooth/bk_ble.h"

#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_gap_ble_types.h"

#include "dm_ble_gap_task.h"

enum le_phy_mask
{
    /// The Host prefers to use the LE 1M transmitter/receiver PHY (possibly among others)
    PHY_1MBPS_BIT      = (1<<0),
    PHY_1MBPS_POS      = (0),
    /// The Host prefers to use the LE 2M transmitter/receiver PHY (possibly among others)
    PHY_2MBPS_BIT      = (1<<1),
    PHY_2MBPS_POS      = (1),
    /// The Host prefers to use the LE Coded transmitter/receiver PHY (possibly among others)
    PHY_CODED_BIT      = (1<<2),
    PHY_CODED_POS      = (2),
    /// The Host prefers to use the LE Coded transmitter/receiver PHY (possibly among others)
    PHY_ALL        = (PHY_1MBPS_BIT | PHY_2MBPS_BIT | PHY_CODED_BIT),
};

/*============================= export function =========================================*/
ble_err_t bk_ble_gap_set_scan_params_ex(
    uint8_t    own_address_type,
    uint8_t    scanning_filter_policy,
    uint8_t    scanning_phy,
    uint16_t *scan_type,
    uint16_t *scan_interval,
    uint16_t *scan_window)
{
    bk_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_scan_params_ex_t tmp = {0};
    uint8_t number_of_sets = 0;
    tmp.own_addr_type = own_address_type;
    tmp.scanning_filter_policy = scanning_filter_policy;
    tmp.scanning_phy = scanning_phy;

    if (scanning_phy & PHY_1MBPS_BIT)
    {
        tmp.scan_type[number_of_sets] = scan_type[number_of_sets];
        tmp.scan_interval[number_of_sets] = scan_interval[number_of_sets];
        tmp.scan_window[number_of_sets] = scan_window[number_of_sets];
        number_of_sets ++;
    }

    if (scanning_phy & PHY_CODED_BIT)
    {
        tmp.scan_type[number_of_sets] = scan_type[number_of_sets];
        tmp.scan_interval[number_of_sets] = scan_interval[number_of_sets];
        tmp.scan_window[number_of_sets] = scan_window[number_of_sets];
        number_of_sets ++;
    }

    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_SCAN_PARAMS, &tmp, sizeof(tmp), NULL);
    return ret;
}

ble_err_t bk_ble_gap_set_scan_enable_ex(
    uint8_t   enable,
    uint8_t   filter_duplicates,
    uint16_t  duration,
    uint16_t  period)
{
    bk_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_scan_enable_t tmp = {0};

    tmp.enable = enable;
    tmp.filter_duplicates = filter_duplicates;
    tmp.duration = duration;
    tmp.period = period;

    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_SCAN_ENABLE, &tmp, sizeof(tmp), NULL);
    return ret;
}

ble_err_t bk_ble_gap_set_adv_params_ex(
    uint8_t   adv_handle,
    uint16_t  adv_event_properties,
    uint32_t  primary_advertising_interval_min,
    uint32_t  primary_advertising_interval_max,
    uint8_t   primary_advertising_channel_map,
    uint8_t   own_address_type,
    uint8_t   peer_address_type,
    uint8_t  *peer_address,
    uint8_t   advertising_filter_policy,
    int8_t    advertising_tx_power,
    uint8_t   primary_advertising_phy,
    uint8_t   secondary_adv_max_skip,
    uint8_t   secondary_advertising_phy,
    uint8_t   advertising_set_id,
    uint8_t   scan_req_nfy_enable)
{
    bk_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_adv_params_t tmp = {0};

    tmp.adv_handle = adv_handle;
    tmp.adv_event_properties = adv_event_properties;
    tmp.primary_adv_interval_min = primary_advertising_interval_min;
    tmp.primary_adv_interval_max = primary_advertising_interval_max;
    tmp.primary_adv_channel_map = primary_advertising_channel_map;
    tmp.own_addr_type = own_address_type;
    tmp.peer_addr_type = peer_address_type;

    if (peer_address != NULL)
    {
        os_memcpy(tmp.peer_addr.addr, peer_address, sizeof(tmp.peer_addr));
    }

    tmp.adv_filter_policy = advertising_filter_policy;
    tmp.adv_tx_power = advertising_tx_power;
    tmp.primary_adv_phy = primary_advertising_phy;
    tmp.secondary_adv_max_skip = secondary_adv_max_skip;
    tmp.secondary_adv_phy = secondary_advertising_phy;
    tmp.adv_set_id = advertising_set_id;
    tmp.scan_req_nfy_enable = scan_req_nfy_enable;

    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_ADV_PARAMS, &tmp, sizeof(tmp), NULL);
    return ret;
}


ble_err_t bk_ble_gap_set_adv_data_ex(
    uint8_t   advertising_handle,
    uint8_t   operation,
    uint8_t   frag_pref,
    uint8_t *adv_buff,
    uint8_t adv_len,
    uint32_t cmd_type)
{
    bk_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_adv_data_t tmp = {0};

    tmp.adv_handle = advertising_handle;
    tmp.operation = operation;
    tmp.frag_pref = frag_pref;
    tmp.adv_data_len = adv_len;

    if ( adv_buff == NULL || tmp.adv_data_len > ADV_DATA_LEN)
    {
        ret = BK_ERR_BLE_ADV_DATA;
        return ret;
    }

    ret = set_cmd_type(ADV_DATA_CMD, cmd_type);
    if (ret)
    {
        os_printf("%s err\n", __func__);
        return BK_ERR_BLE_FAIL;
    }

    if (adv_len)
    {
        os_memcpy(tmp.adv_data, adv_buff, adv_len);
    }

    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_ADV_DATA, &tmp, sizeof(tmp), NULL);
    return ret;
}

ble_err_t bk_ble_gap_set_scan_rsp_data_ex(
    uint8_t   advertising_handle,
    uint8_t   operation,
    uint8_t   fragment_pref,
    uint8_t  *scan_response_data,
    uint8_t   scan_response_data_length,
    uint32_t cmd_type)
{
    bk_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_scan_rsp_data_t tmp = {0};

    tmp.adv_handle = advertising_handle;
    tmp.operation = operation;
    tmp.frag_pref = fragment_pref;
    tmp.scan_response_data_len = scan_response_data_length;

    if (tmp.scan_response_data_len > ADV_DATA_LEN)
    {
        ret = BK_ERR_BLE_ADV_DATA;
        return ret;
    }

    ret = set_cmd_type(SCAN_RSP_DATA_CMD, cmd_type);
    if (ret)
    {
        return BK_ERR_BLE_FAIL;
    }

    if (scan_response_data_length)
    {
        os_memcpy(tmp.scan_response_data, scan_response_data, scan_response_data_length);
    }

    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_SCAN_RSP_DATA, &tmp, sizeof(tmp), NULL);
    return ret;

}

ble_err_t bk_ble_gap_set_adv_enable_ex(
    uint8_t    enable,
    uint8_t    number_of_sets,
    uint8_t   *advertising_handle,
    uint16_t *duration,
    uint8_t   *max_extd_adv_evts)
{
    bk_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_adv_enable_t tmp = {0};
    tmp.enable = enable;
    tmp.number_of_sets = number_of_sets;
    ret = set_cmd_type(ADV_ENABLE_CMD, tmp.enable);
    if (ret)
    {
        return BK_ERR_BLE_FAIL;
    }

    if (number_of_sets)
    {
        os_memcpy(tmp.adv_handle, advertising_handle, number_of_sets * sizeof(*tmp.adv_handle));
        os_memcpy(tmp.duration, duration, number_of_sets * sizeof(*tmp.duration));
        os_memcpy(tmp.max_extd_adv_evts, max_extd_adv_evts, number_of_sets * sizeof(*tmp.max_extd_adv_evts));
    }
    else
    {
        return BK_ERR_BLE_FAIL;
    }

    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_ADV_ENABLE, &tmp, sizeof(tmp), NULL);
    return ret;
}

#define LEN_UUID_16     2
#define LEN_UUID_32     4
#define LEN_UUID_128    16

typedef struct
{
    UINT16          len;
    union
    {
        UINT16      uuid16;
        UINT32      uuid32;
        UINT8       uuid128[16];
    } uu;
} uuid_t;

static const unsigned char base_uuid[16] =
{
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

int uuid_type(unsigned char *p_uuid)
{
    int i = 0;
    int match = 0;
    int all_zero = 1;

    for (i = 0; i != 16; ++i)
    {
        if (i == 12 || i == 13)
        {
            continue;
        }

        if (p_uuid[i] == base_uuid[i])
        {
            ++match;
        }

        if (p_uuid[i] != 0)
        {
            all_zero = 0;
        }
    }
    if (all_zero)
    {
        return 0;
    }
    if (match == 12)
    {
        return LEN_UUID_32;
    }
    if (match == 14)
    {
        return LEN_UUID_16;
    }
    return LEN_UUID_128;
}

void btc128_to_bta_uuid(uuid_t *p_dest, uint8_t *p_src)
{
    int i = 0;
    p_dest->len = uuid_type(p_src);

    switch (p_dest->len)
    {
        case LEN_UUID_16:
            p_dest->uu.uuid16 = (p_src[13] << 8) + p_src[12];
            break;


        case LEN_UUID_32:
            p_dest->uu.uuid32 = (p_src[13] << 8) + p_src[12];
            p_dest->uu.uuid32 += (p_src[15] << 24) + (p_src[14] << 16);
            break;

        case LEN_UUID_128:
            for (i = 0; i != 16; ++i)
            {
                p_dest->uu.uuid128[i] = p_src[i];
            }
            break;

        default:
            break;
    }
}

/*=======================================================================================*/




ble_err_t bk_ble_gap_register_callback(bk_ble_gap_cb_t callback)
{
    bk_ble_gap_set_callback(callback);
    return BK_ERR_BLE_SUCCESS;
}

ble_err_t bk_ble_gap_update_conn_params(bk_ble_conn_update_params_t *params)
{
    bk_err_t ret = BK_ERR_BLE_SUCCESS;
    bk_ble_conn_update_params_t tmp;
    tmp.min_int = params->min_int;
    tmp.max_int = params->max_int;
    tmp.latency = params->latency;
    tmp.timeout = params->timeout;

    os_memcpy(tmp.bda, params->bda, sizeof(params->bda));
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_UPDATE_CONNECTION_PARAMS, &tmp, sizeof(tmp), NULL);

    return ret;
}

ble_err_t bk_ble_gap_set_pkt_data_len(bk_bd_addr_t remote_device, uint16_t tx_data_length)
{
    bk_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_pkt_data_len_t args = {0};
    args.peer_addr_type = 1;
    args.tx_data_len = tx_data_length;
    args.tx_time = 0x0148;//0x4290
    os_memcpy(args.peer_addr.addr, remote_device, sizeof(bk_bd_addr_t));
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_DATA_LENGTH, &args, sizeof(args), NULL);
    return ret;
}

ble_err_t bk_ble_gap_set_rand_addr(bk_bd_addr_t rand_addr)
{
    bk_err_t ret = BK_ERR_BLE_SUCCESS;
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_RANDOM_ADDR, &rand_addr, sizeof(bk_bd_addr_t), NULL);
    return ret;
}

ble_err_t bk_ble_gap_config_local_privacy(bool privacy_enable)
{
#if 0
    set_privacy_mode_t args = {0};
    args.privacy_mode = privacy_enable;
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PRIVACY_MODE, &args, sizeof(args), NULL);
#else
    return BK_ERR_BLE_SUCCESS;
#endif
}

ble_err_t bk_ble_gap_config_local_icon(uint16_t icon)
{
    return BK_ERR_BLE_SUCCESS;
}

ble_err_t bk_ble_gap_update_whitelist(bool add_remove, bk_bd_addr_t remote_bda, bk_ble_wl_addr_type_t wl_addr_type)
{
    bk_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_update_whitelist_t args = {0};
    args.update_mode = add_remove;
    args.peer_addr_type = wl_addr_type;
    os_memcpy(args.peer_addr.addr, remote_bda, sizeof(bk_bd_addr_t));
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_UPDATE_WHILELIST, &args, sizeof(args), NULL);
    return ret;
}

ble_err_t bk_ble_gap_clear_whitelist(void)
{
    bk_err_t ret = BK_ERR_BLE_SUCCESS;
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_CLEAR_WHILELIST, NULL, 0, NULL);
    return ret;

}

ble_err_t bk_ble_gap_get_whitelist_size(uint16_t *length)
{
    *length = bk_ble_gap_get_whitelist_avail_size_private();
    return BK_ERR_BLE_SUCCESS;
}

ble_err_t bk_ble_gap_set_device_name(const char *name)
{
    bk_err_t ret = BK_ERR_BLE_SUCCESS;
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_LOCAL_NAME, (char *)name, os_strlen(name), NULL);
    return ret;
}

ble_err_t bk_ble_gap_get_device_name(char *name, uint32_t *size)
{
    char *new_name = (char *)bk_ble_gap_get_local_name_private();
    if (new_name != NULL)
    {
        *size = os_strlen(new_name);
        if (name != NULL)
        {
            os_memcpy(name, new_name, *size);
            name[*size] = '\0';
        }
    }
    else
    {
        *size = 0;
    }
    return BK_ERR_BLE_SUCCESS;
}

ble_err_t bk_ble_gap_get_local_used_addr(bk_bd_addr_t local_used_addr, uint8_t *addr_type)
{
    bk_err_t ret = BK_ERR_BLE_SUCCESS;
    if (1)
    {
        ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_READ_LOCAL_ADDR, NULL, 0, NULL);
    }
    else
    {
//        extern UCHAR hci_random_address[BT_BD_ADDR_SIZE];
//        os_memcpy(local_used_addr, hci_random_address, 6);
    }
    return ret;
}

ble_err_t bk_ble_gap_read_rssi(bk_bd_addr_t remote_addr)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_read_adv_report_t adv_report = {0};
    os_memcpy(adv_report.peer_addr.addr, remote_addr, 6);
    adv_report.peer_addr_type = 1;
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_READ_RSSI, &adv_report, sizeof(adv_report), NULL);
    return ret;
}

ble_err_t bk_ble_gap_set_security_param(bk_ble_sm_param_t param_type,
                                        void *value, uint8_t len)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
//    ret = set_cmd_type(SECURITY_PARAM_CMD, param_type);
//    if (ret)
//    {
//        return BK_ERR_BLE_FAIL;
//    }

    ble_gap_security_param_t param;

    os_memset(&param, 0, sizeof(param));
    param.type = param_type;
    param.len = len;
    param.data = os_malloc(len);

    if(!param.data)
    {
        os_printf("%s malloc err\n", __func__);
        return -1;
    }

    os_memcpy(param.data, value, len);

    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_SECURITY_PARAM, &param, sizeof(param), NULL);

    return ret;
}

ble_err_t bk_ble_gap_security_rsp(bk_bd_addr_t bd_addr,  bool accept)
{
    return BK_ERR_BLE_SUCCESS;
}

ble_err_t bk_ble_set_encryption(bk_bd_addr_t bd_addr, bk_ble_sec_act_t sec_act)
{
    return BK_ERR_BLE_SUCCESS;
}

ble_err_t bk_ble_passkey_reply(bk_bd_addr_t bd_addr, bool accept, uint32_t passkey)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_pair_reply_t param;

    os_memset(&param, 0, sizeof(param));

    param.method = 1;
    param.accept = accept;
    os_memcpy(param.peer_addr.addr, bd_addr, sizeof(param.peer_addr.addr));
    param.passkey = passkey;

    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PAIR_REPLY, &param, sizeof(param), NULL);
    return ret;
}

ble_err_t bk_ble_confirm_reply(bk_bd_addr_t bd_addr, bool accept)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_pair_reply_t param;

    os_memset(&param, 0, sizeof(param));

    param.method = 0;
    param.accept = accept;
    os_memcpy(param.peer_addr.addr, bd_addr, sizeof(param.peer_addr.addr));

    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PAIR_REPLY, &param, sizeof(param), NULL);
    return ret;
}

ble_err_t bk_ble_pair_key_reply(bk_bd_addr_t bd_addr, bool accept, bk_ble_key_type_t type, void *data, uint32_t len)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_key_reply_t param;

    os_memset(&param, 0, sizeof(param));

    param.type = type;
    param.accept = accept;
    os_memcpy(param.peer_addr.addr, bd_addr, sizeof(param.peer_addr.addr));

    if(data && len)
    {
        os_memcpy(&param.key, data, len);
    }

    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_KEY_REPLY, &param, sizeof(param), NULL);
    return ret;
}

ble_err_t bk_ble_remove_bond_device(bk_bd_addr_t bd_addr)
{
    return BK_ERR_BLE_SUCCESS;
}

int bk_ble_get_bond_device_num(void)
{
    return BK_ERR_BLE_SUCCESS;
}

ble_err_t bk_ble_get_bond_device_list(int *dev_num, bk_ble_bond_dev_t *dev_list)
{
    return BK_ERR_BLE_SUCCESS;
}

ble_err_t bk_ble_gap_disconnect(bk_bd_addr_t remote_device)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_disconnect_t tmp = {0};
    os_memcpy(tmp.addr.addr, remote_device, sizeof(bd_addr_t));
    tmp.addr_type = 1;
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_DISCONNECT, &tmp, sizeof(tmp), NULL);
    return ret;
}

ble_err_t bk_ble_get_current_conn_params(bk_bd_addr_t bd_addr, bk_gap_conn_params_t *conn_params)
{
    return BK_ERR_BLE_SUCCESS;
}

ble_err_t bk_gap_ble_set_channels(bk_ble_gap_channels channels)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_CHANNELS, channels, sizeof(bk_ble_gap_channels), NULL);
    return ret;
}

ble_err_t bk_ble_gap_read_phy(bk_bd_addr_t bd_addr)
{
    ble_gap_preferred_phy_t phy = {0};
    ble_err_t ret = BK_ERR_BLE_SUCCESS;

    os_memcpy(phy.peer_addr.addr, bd_addr, 6);
    phy.peer_addr_type = 1;
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_READ_PHY, &phy, sizeof(phy), NULL);
    return ret;
}

ble_err_t bk_ble_gap_set_preferred_default_phy(bk_ble_gap_phy_mask_t tx_phy_mask, bk_ble_gap_phy_mask_t rx_phy_mask)
{
    ble_gap_preferred_phy_t phy = {0};
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    if (tx_phy_mask == 0)
    {
        phy.allphy |= 0x01;
    }
    if (rx_phy_mask == 0)
    {
        phy.allphy |= 0x02;
    }
    phy.txphy = tx_phy_mask;
    phy.rxphy = rx_phy_mask;

    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PREFERRED_DEFAULT_PHY, &phy, sizeof(phy), NULL);
    return ret;
}

ble_err_t bk_ble_gap_set_preferred_phy(bk_bd_addr_t bd_addr,
                                       bk_ble_gap_all_phys_t all_phys_mask,
                                       bk_ble_gap_phy_mask_t tx_phy_mask,
                                       bk_ble_gap_phy_mask_t rx_phy_mask,
                                       bk_ble_gap_prefer_phy_options_t phy_options)
{
    ble_gap_preferred_phy_t phy = {0};
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    os_memcpy(phy.peer_addr.addr, bd_addr, 6);
    phy.peer_addr_type = 1;
    phy.allphy = all_phys_mask;
    phy.txphy = tx_phy_mask;
    phy.rxphy = rx_phy_mask;
    phy.phy_options = phy_options;

    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PREFERRED_PHY, &phy, sizeof(phy), NULL);
    return ret;
}

ble_err_t bk_ble_gap_set_adv_rand_addr(uint8_t instance, bk_bd_addr_t rand_addr)
{
    ble_gap_adv_set_rand_addr_t args = {0};
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    args.addr_type = 1;
    args.adv_handler = instance;
    os_memcpy(args.addr.addr, rand_addr, 6);

    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_ADV_SET_RANDOM_ADDR, &args, sizeof(args), NULL);
    return ret;
}

ble_err_t bk_ble_gap_set_adv_params(uint8_t instance, const bk_ble_gap_ext_adv_params_t *params)
{
    uint32_t  primary_advertising_interval_min = params->interval_min;
    uint32_t  primary_advertising_interval_max = params->interval_max;
    uint8_t   primary_advertising_channel_map = params->channel_map;
    uint8_t   own_address_type = params->own_addr_type;
    uint8_t   primary_advertising_phy = params->primary_phy;
    uint8_t   secondary_advertising_phy = params->secondary_phy;
    uint8_t   adv_handle = instance;
    uint16_t  adv_event_properties = params->type;
    uint8_t   peer_address_type = params->peer_addr_type;
    uint8_t  *peer_address = (uint8_t *)params->peer_addr;
    uint8_t   advertising_filter_policy = params->filter_policy;
    int8_t    advertising_tx_power = params->tx_power;
    uint8_t   secondary_adv_max_skip = params->max_skip;
    uint8_t   advertising_set_id = params->sid;
    uint8_t   scan_req_nfy_enable = params->scan_req_notif;

    return bk_ble_gap_set_adv_params_ex(
               adv_handle, adv_event_properties, primary_advertising_interval_min, primary_advertising_interval_max,
               primary_advertising_channel_map, own_address_type, peer_address_type, peer_address,
               advertising_filter_policy, advertising_tx_power, primary_advertising_phy, secondary_adv_max_skip,
               secondary_advertising_phy, advertising_set_id, scan_req_nfy_enable);

}

ble_err_t bk_ble_gap_set_adv_data(bk_ble_adv_data_t *adv_data)
{
    bk_err_t ret = BK_ERR_BLE_SUCCESS;
    uint8_t adv_data_buf[31];
    uint8_t adv_data_len = 0;

    if (adv_data == NULL)
    {
        return BK_ERR_BLE_ADV_DATA;
    }

    if (adv_data->service_uuid_len & 0xf) //not 16*n
    {
        return BK_ERR_BLE_ADV_DATA;
    }

    uint8_t   advertising_handle = 0x00;//1;
    uint8_t   operation = 0x03;
    uint8_t   frag_pref = 0x01;

    if (adv_data->flag != 0)
    {
        if (adv_data_len + 1 + 2 > 31)
        {
            return BK_ERR_BLE_ADV_DATA;
        }
        adv_data_buf[adv_data_len] = 2;
        adv_data_buf[adv_data_len + 1] = BK_BLE_AD_TYPE_FLAG;
        adv_data_buf[adv_data_len + 2] = adv_data->flag;
        adv_data_len += 3;
    }

    if (adv_data->max_interval >= adv_data->min_interval
        && adv_data->max_interval > 0
        && adv_data->min_interval > 0)
    {
        if (adv_data_len + 4 + 2 > 31)
        {
            return BK_ERR_BLE_ADV_DATA;
        }
        adv_data_buf[adv_data_len] = 5;
        adv_data_buf[adv_data_len + 1] = BK_BLE_AD_TYPE_INT_RANGE;
        adv_data_buf[adv_data_len + 2] = adv_data->min_interval & 0xFF;
        adv_data_buf[adv_data_len + 3] = adv_data->min_interval >> 8;
        adv_data_buf[adv_data_len + 4] = adv_data->max_interval & 0xFF;
        adv_data_buf[adv_data_len + 5] = adv_data->max_interval >> 8;
        adv_data_len += 6;
    }

    if (adv_data->include_txpower)
    {
        if (adv_data_len + 1 + 2 > 31)
        {
            return BK_ERR_BLE_ADV_DATA;
        }
        adv_data_buf[adv_data_len] = 2;
        adv_data_buf[adv_data_len + 1] = BK_BLE_AD_TYPE_TX_PWR;
        adv_data_buf[adv_data_len + 2] = 0x05;
        adv_data_len += 3;
    }

    if (adv_data->appearance != 0)
    {
        if (adv_data_len + 2 + 2 > 31)
        {
            return BK_ERR_BLE_ADV_DATA;
        }
        adv_data_buf[adv_data_len] = 3;
        adv_data_buf[adv_data_len + 1] = BK_BLE_AD_TYPE_APPEARANCE;
        adv_data_buf[adv_data_len + 2] = (adv_data->appearance >> 8) & 0xFF;
        adv_data_buf[adv_data_len + 3] = (adv_data->appearance & 0xFF);
        adv_data_len += 4;
    }

    if (adv_data->manufacturer_len > 0 && adv_data->p_manufacturer_data != NULL)
    {
        if (adv_data_len + adv_data->manufacturer_len + 2 > 31)
        {
            return BK_ERR_BLE_ADV_DATA;
        }
        adv_data_buf[adv_data_len] = adv_data->manufacturer_len + 1;
        adv_data_buf[adv_data_len + 1] = BK_BLE_AD_TYPE_MANU;
        os_memcpy(adv_data_buf + adv_data_len + 2, adv_data->p_manufacturer_data, adv_data->manufacturer_len);
        adv_data_len += adv_data->manufacturer_len + 2;
    }

    if (adv_data->service_data_len > 0 && adv_data->p_service_data != NULL)
    {
        if (adv_data_len + adv_data->service_data_len + 2 > 31)
        {
            return BK_ERR_BLE_ADV_DATA;
        }
        adv_data_buf[adv_data_len] = adv_data->service_data_len + 1;
        adv_data_buf[adv_data_len + 1] = BK_BLE_AD_TYPE_SERVICE_DATA;
        os_memcpy(adv_data_buf + adv_data_len + 2, adv_data->p_service_data, adv_data->service_data_len);
        adv_data_len += adv_data->service_data_len + 2;
    }

    if (adv_data->service_uuid_len > 0 && adv_data->p_service_uuid != NULL)
    {
        uint8_t uuid16[32];
        uint8_t uuid16_len = 0;
        uint8_t uuid32[32];
        uint8_t uuid32_len = 0;
        uint8_t uuid128[32];
        uint8_t uuid128_len = 0;
        for (int position = 0; position < adv_data->service_uuid_len; position += LEN_UUID_128)
        {
            uuid_t uuid;
            btc128_to_bta_uuid(&uuid, adv_data->p_service_uuid + position);

            switch (uuid.len)
            {
                case (LEN_UUID_16):
                {
                    os_memcpy(uuid16, &uuid.uu.uuid16, 2);
                    uuid16_len += 2;
                    break;
                }

                case (LEN_UUID_32):
                {
                    os_memcpy(uuid32, &uuid.uu.uuid32, 4);
                    uuid32_len += 4;
                    break;
                }

                case (LEN_UUID_128):
                {
                    os_memcpy(uuid128, uuid.uu.uuid128, 16);
                    uuid128_len += 16;
                    break;
                }

                default:
                    break;
            }
        }

        if (uuid16_len)
        {
            if (adv_data_len + uuid16_len + 2 > 31)
            {
                return BK_ERR_BLE_ADV_DATA;
            }
            adv_data_buf[adv_data_len] = uuid16_len + 1;
            adv_data_buf[adv_data_len + 1] = BK_BLE_AD_TYPE_16SRV_CMPL;
            os_memcpy(adv_data_buf + adv_data_len + 2, uuid16, uuid16_len);
            adv_data_len += uuid16_len + 2;
        }
        if (uuid32_len)
        {
            if (adv_data_len + uuid32_len + 2 > 31)
            {
                return BK_ERR_BLE_ADV_DATA;
            }
            adv_data_buf[adv_data_len] = uuid32_len + 1;
            adv_data_buf[adv_data_len + 1] = BK_BLE_AD_TYPE_32SRV_CMPL;
            os_memcpy(adv_data_buf + adv_data_len + 2, uuid32, uuid32_len);
            adv_data_len += uuid32_len + 2;
        }
        if (uuid128_len)
        {
            if (adv_data_len + uuid128_len + 2 > 31)
            {
                return BK_ERR_BLE_ADV_DATA;
            }
            adv_data_buf[adv_data_len] = uuid128_len;
            adv_data_buf[adv_data_len + 1] = BK_BLE_AD_TYPE_128SRV_CMPL;
            os_memcpy(adv_data_buf + adv_data_len + 2, uuid128, uuid128_len);
            adv_data_len += uuid128_len + 2;
        }
    }

    if (adv_data->include_name)
    {
        char *name = (char *)bk_ble_gap_get_local_name_private();
        int max_len = 31 - adv_data_len - 2;
        if (name && max_len > 0)
        {
            if (max_len > os_strlen(name))
            {
                adv_data_buf[adv_data_len] = os_strlen(name) + 1;
                adv_data_buf[adv_data_len + 1] = BK_BLE_AD_TYPE_NAME_CMPL;
                os_memcpy(adv_data_buf + adv_data_len + 2, name, os_strlen(name));
                adv_data_len += os_strlen(name) + 2;
            }
            else
            {
                adv_data_buf[adv_data_len] = max_len + 1;
                adv_data_buf[adv_data_len + 1] = BK_BLE_AD_TYPE_NAME_SHORT;
                os_memcpy(adv_data_buf + adv_data_len + 2, name, max_len);
                adv_data_len += max_len + 2;
            }
        }
    }

    if (adv_data->set_scan_rsp)
    {
//        ret = set_cmd_type(SCAN_RSP_DATA_CMD, CMD_TYPE_SCAN_RSP_NORMAL);
//        if (ret)
//        {
//            return BK_ERR_BLE_FAIL;
//        }
        ret = bk_ble_gap_set_scan_rsp_data_ex(advertising_handle, operation, frag_pref, adv_data_buf, adv_data_len, CMD_TYPE_SCAN_RSP_NORMAL);
    }
    else
    {
//        ret = set_cmd_type(ADV_DATA_CMD, CMD_TYPE_ADV_NORMAL);
//        if (ret)
//        {
//            return BK_ERR_BLE_FAIL;
//        }
        ret = bk_ble_gap_set_adv_data_ex(advertising_handle, operation, frag_pref, adv_data_buf, adv_data_len, CMD_TYPE_ADV_NORMAL);
    }
    return ret;
}

ble_err_t bk_ble_gap_set_adv_data_raw(uint8_t instance, uint16_t length, const uint8_t *data)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    uint8_t   advertising_handle = instance;//1;
    uint8_t   operation = 0x03;
    uint8_t   frag_pref = 0x01;
    if (data == NULL)
    {
        return BK_ERR_NULL_PARAM;
    }
//    ret = set_cmd_type(ADV_DATA_CMD, 1);
//    if (ret)
//    {
//        return BK_ERR_BLE_FAIL;
//    }
    ret = bk_ble_gap_set_adv_data_ex(advertising_handle, operation, frag_pref, (uint8_t *)data, length, CMD_TYPE_ADV_RAW);
    return ret;
}

ble_err_t bk_ble_gap_set_scan_rsp_data_raw(uint8_t instance, uint16_t length,
                                           const uint8_t *scan_rsp_data)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    uint8_t   advertising_handle = instance;//1;
    uint8_t   operation = 0x03;
    uint8_t   frag_pref = 0x01;
    if (scan_rsp_data == NULL)
    {
        return BK_ERR_NULL_PARAM;
    }
//    ret = set_cmd_type(SCAN_RSP_DATA_CMD, 1);
//    if (ret)
//    {
//        return BK_ERR_BLE_FAIL;
//    }
    ret = bk_ble_gap_set_scan_rsp_data_ex(advertising_handle, operation, frag_pref, (uint8_t *)scan_rsp_data, length, CMD_TYPE_SCAN_RSP_RAW);
    return ret;
}

ble_err_t bk_ble_gap_adv_start(uint8_t num_adv, const bk_ble_gap_ext_adv_t *ext_adv)
{
    //ble_err_t ret = BK_ERR_BLE_SUCCESS;
    uint8_t instance[0x3F] = {0};
    uint16_t duration[0x3F] = {0};
    uint8_t max_extd_adv_evts[0x3F] = {0};
    for (int i = 0 ; i < num_adv; i ++)
    {
        instance[i] = ext_adv[i].instance;
        duration[i] = ext_adv[i].duration;
        max_extd_adv_evts[i] = ext_adv[i].max_events;
    }
//    ret = set_cmd_type(ADV_ENABLE_CMD, 1);
//    if (ret)
//    {
//        return BK_ERR_BLE_FAIL;
//    }
    return bk_ble_gap_set_adv_enable_ex(1, num_adv, instance, duration, max_extd_adv_evts);
}

ble_err_t bk_ble_gap_adv_stop(uint8_t num_adv, const uint8_t *ext_adv_inst)
{
    //ble_err_t ret = BK_ERR_BLE_SUCCESS;
    uint8_t instance[0x3F] = {0};
    uint16_t duration[0x3F] = {0};
    uint8_t max_extd_adv_evts[0x3F] = {0};
    for (int i = 0 ; i < num_adv; i ++)
    {
        instance[i] = ext_adv_inst[i];
    }
//    ret = set_cmd_type(ADV_ENABLE_CMD, 0);
//    if (ret)
//    {
//        return BK_ERR_BLE_FAIL;
//    }
    return bk_ble_gap_set_adv_enable_ex(0, num_adv, instance, duration, max_extd_adv_evts);
}

ble_err_t bk_ble_gap_adv_set_remove(uint8_t instance)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_REMOVE_ADV_SET, &instance, 1, NULL);
    return ret;
}

ble_err_t bk_ble_gap_adv_set_clear(void)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_CLEAR_ADV_SET, NULL, 0, NULL);
    return ret;
}

ble_err_t bk_ble_gap_set_periodic_adv_params(uint8_t instance, const bk_ble_gap_periodic_adv_params_t *params)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_periodic_adv_params_t periodic_adv_params = {0};
    periodic_adv_params.adv_handler = instance;
    periodic_adv_params.interval_min = params->interval_min;
    periodic_adv_params.interval_max = params->interval_max;
    periodic_adv_params.properties = params->properties;
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PERIODIC_ADV_PARAMS, &periodic_adv_params, sizeof(periodic_adv_params), NULL);
    return ret;
}

ble_err_t bk_ble_gap_set_periodic_adv_data_raw(uint8_t instance, uint16_t length, const uint8_t *data)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_periodic_adv_data_t periodic_adv_data_raw = {0};
    if (length == 0 || length > HCI_PER_ADV_DATA_FRAG_MAX_LEN * 5)
    {
        return BK_ERR_PARAM;
    }

    periodic_adv_data_raw.adv_handler = instance;
    if (length <= HCI_PER_ADV_DATA_FRAG_MAX_LEN)
    {
        set_cmd_type(PERIODIC_ADV_DATA_CMD, 3);
        periodic_adv_data_raw.operation = 3;
        periodic_adv_data_raw.len = length;
        os_memcpy(periodic_adv_data_raw.data, data, length);
        ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PERIODIC_ADV_DATA, &periodic_adv_data_raw, sizeof(periodic_adv_data_raw), NULL);
    }
    else if (length > HCI_PER_ADV_DATA_FRAG_MAX_LEN && length <= HCI_PER_ADV_DATA_FRAG_MAX_LEN * 2)
    {
        set_cmd_type(PERIODIC_ADV_DATA_CMD, 1);
        periodic_adv_data_raw.operation = 1;
        periodic_adv_data_raw.len = HCI_PER_ADV_DATA_FRAG_MAX_LEN;
        os_memcpy(periodic_adv_data_raw.data, data, HCI_PER_ADV_DATA_FRAG_MAX_LEN);
        ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PERIODIC_ADV_DATA, &periodic_adv_data_raw, sizeof(periodic_adv_data_raw), NULL);
        if (ret != BK_ERR_BLE_SUCCESS)
        {
            goto error;
        }

        set_cmd_type(PERIODIC_ADV_DATA_CMD, 2);
        periodic_adv_data_raw.operation = 2;
        periodic_adv_data_raw.len = length - HCI_PER_ADV_DATA_FRAG_MAX_LEN;
        os_memcpy(periodic_adv_data_raw.data, data + HCI_PER_ADV_DATA_FRAG_MAX_LEN, length - HCI_PER_ADV_DATA_FRAG_MAX_LEN);
        ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PERIODIC_ADV_DATA, &periodic_adv_data_raw, sizeof(periodic_adv_data_raw), NULL);
    }
    else if (length > 2 * HCI_PER_ADV_DATA_FRAG_MAX_LEN && length <= HCI_PER_ADV_DATA_FRAG_MAX_LEN * 5)
    {
        uint16_t send_len = 0;
        set_cmd_type(PERIODIC_ADV_DATA_CMD, 1);
        periodic_adv_data_raw.operation = 1;
        periodic_adv_data_raw.len = HCI_PER_ADV_DATA_FRAG_MAX_LEN;
        os_memcpy(periodic_adv_data_raw.data, data, HCI_PER_ADV_DATA_FRAG_MAX_LEN);
        ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PERIODIC_ADV_DATA, &periodic_adv_data_raw, sizeof(periodic_adv_data_raw), NULL);
        if (ret != BK_ERR_BLE_SUCCESS)
        {
            goto error;
        }
        send_len += HCI_PER_ADV_DATA_FRAG_MAX_LEN;
        while (length - send_len > HCI_PER_ADV_DATA_FRAG_MAX_LEN)
        {
            set_cmd_type(PERIODIC_ADV_DATA_CMD, 0);
            periodic_adv_data_raw.operation = 0;
            periodic_adv_data_raw.len = HCI_PER_ADV_DATA_FRAG_MAX_LEN;
            os_memcpy(periodic_adv_data_raw.data, data + send_len, HCI_PER_ADV_DATA_FRAG_MAX_LEN);
            ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PERIODIC_ADV_DATA, &periodic_adv_data_raw, sizeof(periodic_adv_data_raw), NULL);
            if (ret != BK_ERR_BLE_SUCCESS)
            {
                goto error;
            }
            send_len += HCI_PER_ADV_DATA_FRAG_MAX_LEN;
        }
        set_cmd_type(PERIODIC_ADV_DATA_CMD, 2);
        periodic_adv_data_raw.operation = 2;
        periodic_adv_data_raw.len = length - send_len;
        os_memcpy(periodic_adv_data_raw.data, data + send_len, length - send_len);
        ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_SET_PERIODIC_ADV_DATA, &periodic_adv_data_raw, sizeof(periodic_adv_data_raw), NULL);

    }
error:
    return ret;
}

ble_err_t bk_ble_gap_periodic_adv_start(uint8_t instance)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_periodic_adv_status_t args = {0};
    args.adv_handler = instance;
    args.enable = 0x1;
    ret = set_cmd_type(PERIODIC_ADV_ENABLE_CMD, 1);
    if (ret)
    {
        return BK_ERR_BLE_FAIL;
    }
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PERIODIC_ADV_START, &args, sizeof(args), NULL);
    return ret;
}

ble_err_t bk_ble_gap_periodic_adv_stop(uint8_t instance)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_periodic_adv_status_t args = {0};
    args.adv_handler = instance;
    args.enable = 0;
    ret = set_cmd_type(PERIODIC_ADV_ENABLE_CMD, 0);
    if (ret)
    {
        return BK_ERR_BLE_FAIL;
    }
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PERIODIC_ADV_STOP, &args, sizeof(args), NULL);
    return ret;
}

ble_err_t bk_ble_gap_periodic_adv_create_sync(const bk_ble_gap_periodic_adv_sync_params_t *params)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_periodic_adv_create_sync_t args = {0};
    os_memcpy(args.addr.addr, params->addr, 6);
    args.addr_type = params->addr_type;
    args.filter_policy = params->filter_policy;
    args.sid = params->sid;
    args.skip = params->skip;
    args.sync_timeout = params->sync_timeout;
    args.sync_cte_type = 0;
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PERIODIC_ADV_CREATE_SYNC, &args, sizeof(args), NULL);
    return ret;
}

ble_err_t bk_ble_gap_periodic_adv_sync_cancel(void)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PERIODIC_ADV_SYNC_CANCEL, NULL, 0, NULL);
    return ret;
}

ble_err_t bk_ble_gap_periodic_adv_sync_terminate(uint16_t sync_handle)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PERIODIC_ADV_SYNC_TERMINATE, &sync_handle, 2, NULL);
    return ret;
}

ble_err_t bk_ble_gap_periodic_adv_add_dev_to_list(bk_ble_addr_type_t addr_type,
                                                  bk_bd_addr_t addr,
                                                  uint8_t sid)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_periodic_adv_dev_t periodic_adv_dev = {0};
    periodic_adv_dev.addr_type = addr_type;
    periodic_adv_dev.sid = sid;
    os_memcpy(periodic_adv_dev.addr.addr, addr, 6);
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PERIODIC_ADV_ADD_DEV_TO_LIST, &periodic_adv_dev, sizeof(periodic_adv_dev), NULL);
    return ret;
}

ble_err_t bk_ble_gap_periodic_adv_remove_dev_from_list(bk_ble_addr_type_t addr_type,
                                                       bk_bd_addr_t addr,
                                                       uint8_t sid)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_periodic_adv_dev_t periodic_adv_dev = {0};
    periodic_adv_dev.addr_type = addr_type;
    periodic_adv_dev.sid = sid;
    os_memcpy(periodic_adv_dev.addr.addr, addr, 6);
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PERIODIC_ADV_REMOVE_DEV_FROM_LIST, &periodic_adv_dev, sizeof(periodic_adv_dev), NULL);
    return ret;
}

ble_err_t bk_ble_gap_periodic_adv_clear_dev(void)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_PERIODIC_ADV_CLEAR_DEV, NULL, 0, NULL);
    return ret;
}

ble_err_t bk_ble_gap_set_scan_params(const bk_ble_ext_scan_params_t *params)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    if (params == NULL)
    {
        return BK_ERR_NULL_PARAM;
    }
    bk_ble_gap_set_scan_duplicate(params->scan_duplicate);
    UINT16 scan_type[2] = {0};
    UINT16 scan_intv[2] = {0};
    UINT16 scan_wd[2] = {0};
    scan_type[0] = params->uncoded_cfg.scan_type;
    scan_type[1] = params->coded_cfg.scan_type;
    scan_intv[0] = params->uncoded_cfg.scan_interval;
    scan_intv[1] = params->coded_cfg.scan_interval;
    scan_wd[0] = params->uncoded_cfg.scan_window;
    scan_wd[1] = params->coded_cfg.scan_window;
    ret = bk_ble_gap_set_scan_params_ex(params->own_addr_type, params->filter_policy, params->cfg_mask, scan_type, scan_intv, scan_wd);
    return ret;
}

ble_err_t bk_ble_gap_start_scan(uint32_t duration, uint16_t period)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ret = set_cmd_type(SCAN_ENABLE_CMD, 1);
    if (ret)
    {
        return BK_ERR_BLE_FAIL;
    }
    ret = bk_ble_gap_set_scan_enable_ex(1, bk_ble_gap_get_scan_duplicate(), duration, period);
    return ret;
}

ble_err_t bk_ble_gap_stop_scan(void)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ret = set_cmd_type(SCAN_ENABLE_CMD, 0);
    if (ret)
    {
        return BK_ERR_BLE_FAIL;
    }
    ret = bk_ble_gap_set_scan_enable_ex(0, 0, 0, 0);
    return ret;
}

ble_err_t bk_ble_gap_prefer_connect_params_set(bk_bd_addr_t addr,
                                               bk_ble_gap_phy_mask_t phy_mask,
                                               const bk_ble_gap_conn_params_t *phy_1m_conn_params,
                                               const bk_ble_gap_conn_params_t *phy_2m_conn_params,
                                               const bk_ble_gap_conn_params_t *phy_coded_conn_params)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_conn_params_t args = {0};

    args.init_filr_policy = 0;
    os_memcpy(args.peer_addr.addr, addr, 6);

    args.peer_addr_type = 1;
    args.own_addr_type = 1;
    args.phy_mask = phy_mask;
    if (phy_mask & 0x01)
    {
        if (!phy_1m_conn_params)
        {
            return BK_ERR_PARAM;
        }
        os_memcpy(&args.phy_1m_conn_params, phy_1m_conn_params, sizeof(bk_ble_gap_conn_params_t));
    }
    if (phy_mask & 0x02)
    {
        if (!phy_2m_conn_params)
        {
            return BK_ERR_PARAM;
        }
        os_memcpy(&args.phy_2m_conn_params, phy_2m_conn_params, sizeof(bk_ble_gap_conn_params_t));
    }
    if (phy_mask & 0x04)
    {
        if (!phy_coded_conn_params)
        {
            return BK_ERR_PARAM;
        }
        os_memcpy(&args.phy_coded_conn_params, phy_coded_conn_params, sizeof(bk_ble_gap_conn_params_t));
    }
    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_CREATE_CONNECTION, &args, sizeof(args), NULL);
    return ret;
}

ble_err_t bk_ble_gap_create_bond(bk_bd_addr_t bd_addr)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_create_bond_t param;

    os_memset(&param, 0, sizeof(param));

    os_memcpy(param.peer_addr.addr, bd_addr, sizeof(param.peer_addr.addr));

    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_CREATE_BOND, &param, sizeof(param), NULL);
    return ret;
}

ble_err_t bk_ble_gap_bond_dev_list_operation(bk_gap_bond_dev_list_operation_t op, bk_ble_bond_dev_t *device)
{
    ble_err_t ret = BK_ERR_BLE_SUCCESS;
    ble_gap_bond_dev_list_t param;

    os_memset(&param, 0, sizeof(param));

    param.action = op;

    if(device)
    {
        os_memcpy(&param.info, device, sizeof(*device));
    }

    ret = ble_ethermind_post_msg(BLE_ETHERMIND_MSG_GAP_API_REQ, BLE_ETHERMIND_GAP_API_REQ_SUBMSG_BOND_DEV_LIST_OP, &param, sizeof(param), NULL);
    return ret;
}
