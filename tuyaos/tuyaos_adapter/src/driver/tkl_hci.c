#include "tkl_hci.h"
#include <stdbool.h>

#include "components/bluetooth/bk_ble_types.h"
#include "common/bk_err.h"
#include <components/bluetooth/bk_ble_types.h>
#include <components/bluetooth/bk_dm_bluetooth.h>
#include <components/bluetooth/bk_ble.h>


#define TKL_DEBUG 1

#if !CFG_USE_BK_HOST
extern void ble_entry(void);
extern void bk_printf(const char *fmt, ...);

static TKL_HCI_FUNC_CB s_evt_cb = NULL;
static TKL_HCI_FUNC_CB s_acl_cb = NULL;
BOOL_T ble_init_flag = FALSE;

OPERATE_RET tkl_hci_init(void)
{
    bk_printf("%s\n", __func__);
    //bk_bluetooth_init();
    ble_init_flag = TRUE;
    return OPRT_OK;
}

OPERATE_RET tkl_hci_deinit(void)
{
#if TKL_DEBUG
    bk_printf("%s\n", __func__);
#endif

    bk_bluetooth_deinit();

    return OPRT_OK;
}

OPERATE_RET tkl_hci_reset(void)
{
#if TKL_DEBUG
    bk_printf("%s\n", __func__);
#endif

    bk_bluetooth_deinit();
    bk_bluetooth_init();

    return OPRT_OK;
}

OPERATE_RET tkl_hci_cmd_packet_send(const UCHAR_T *p_buf, USHORT_T buf_len)
{
#if TKL_DEBUG >= 5
    bk_printf("%s op 0x%04X\n", __func__, (uint16_t)((((uint16_t)p_buf[1]) << 8) | p_buf[0]));
    bk_printf("====================>\n");
    tal_log_hex_dump(0, __FILE__,  __LINE__, "data", 64, p_buf, buf_len);
    bk_printf("<====================\n");
#endif
    if (!bk_bluetooth_get_status()) {
        return OPRT_COM_ERROR;
    }

    ble_err_t ret = 0;
    ret = bk_ble_hci_to_controller(BK_BLE_HCI_TYPE_CMD, (uint8_t *)p_buf, buf_len);
    if(ret == BK_ERR_BLE_SUCCESS) {
        return OPRT_OK;
    } else {
        return OPRT_COM_ERROR;
    }

}


OPERATE_RET tkl_hci_acl_packet_send(const UCHAR_T *p_buf, USHORT_T buf_len)
{
    if (!bk_bluetooth_get_status()) {
        return OPRT_COM_ERROR;
    }

#if TKL_DEBUG  >= 5
    bk_printf("%s handle 0x%04X\n", __func__, (uint16_t)((((uint16_t)p_buf[1]) << 8) | p_buf[0]));
    bk_printf("====================>\n");
    tal_log_hex_dump(0, __FILE__,  __LINE__, "data", 64, p_buf, buf_len);
    bk_printf("<====================\n");
#endif

    ble_err_t ret = 0;
    ret = bk_ble_hci_to_controller(BK_BLE_HCI_TYPE_ACL, (uint8_t *)p_buf, buf_len);
    if(ret == BK_ERR_BLE_SUCCESS) {
        return OPRT_OK;
    } else {
        return OPRT_COM_ERROR;
    }
}

static ble_err_t bk_ble_hci_evt_to_host_cb(uint8_t *buf, uint16_t len)
{
    // bk_printf("bk_ble_hci_evt_to_host_cb: s_evt_cb %p.\n", s_evt_cb);
    if(s_evt_cb) {
        s_evt_cb(buf, len);
    }

    return OPRT_OK;
}

static ble_err_t bk_ble_hci_acl_to_host_cb(uint8_t *buf, uint16_t len)
{
    if(s_acl_cb) {
        s_acl_cb(buf, len);
    }

    return OPRT_OK;
}


OPERATE_RET tkl_hci_callback_register(const TKL_HCI_FUNC_CB hci_evt_cb, const TKL_HCI_FUNC_CB acl_pkt_cb)
{
#if TKL_DEBUG
    bk_printf("%s %p %p\n", __func__, hci_evt_cb, acl_pkt_cb);
#endif
    // hci_set_event_callback((hci_func_evt_cb)hci_evt_cb, (hci_func_evt_cb)acl_pkt_cb);
    ble_err_t ret = 0;

    s_evt_cb = hci_evt_cb;
    s_acl_cb = acl_pkt_cb;

    ret = bk_ble_reg_hci_recv_callback(bk_ble_hci_evt_to_host_cb, bk_ble_hci_acl_to_host_cb);
    if(ret == BK_ERR_BLE_SUCCESS) {
        return OPRT_OK;
    } else {
        return OPRT_COM_ERROR;
    }
}

#endif
