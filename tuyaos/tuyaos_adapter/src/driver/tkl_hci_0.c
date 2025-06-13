#include "tkl_hci.h"
#include <stdbool.h>

#include "components/bluetooth/bk_ble_types.h"
#include "common/bk_err.h"
#include <components/bluetooth/bk_ble_types.h>
#include <components/bluetooth/bk_dm_bluetooth.h>
#include <components/bluetooth/bk_ble.h>
#include "tkl_ipc.h"

#define TKL_DEBUG 1

#if !CFG_USE_BK_HOST
extern void ble_entry(void);
extern void bk_printf(const char *fmt, ...);
extern OPERATE_RET tuya_ipc_send_sync(struct ipc_msg_s *msg);
extern OPERATE_RET tuya_ipc_send_no_sync(struct ipc_msg_s *msg);

VOID tal_log_hex_dump(const int     level,
                      const char              *file,
                      const int               line,
                      const char                    *title,
                      uint8_t                   width,
                      uint8_t                   *buf,
                      uint16_t                  size)
{
    int i = 0;

    if (width < 64) {
        width = 64;
    }
    bk_printf("cpu 0: %s %d <%p>", title, size, buf);
    for (i = 0; i < size; i++) {
        bk_printf("%02x ", buf[i]&0xFF);
        if ((i+1)%width == 0) {
            bk_printf("\r\n");
        }
    }
    bk_printf("\r\n\r\n");
}


// static TKL_HCI_FUNC_CB s_evt_cb = NULL;
// static TKL_HCI_FUNC_CB s_acl_cb = NULL;
BOOL_T ble_init_flag = FALSE;

void tkl_hci_ipc_func(struct ipc_msg_s *msg)
{
    OPERATE_RET ret = 0;
    switch(msg->subtype) {
        case TKL_IPC_TYPE_HCI_INIT:
            ret = tkl_hci_init();
            break;

        case TKL_IPC_TYPE_HCI_DEINIT:
            ret = tkl_hci_deinit();
            break;

        case TKL_IPC_TYPE_HCI_RESET:
            ret = tkl_hci_reset();
            break;

        case TKL_IPC_TYPE_HCI_CMD_SEND:
        {
            uint8_t *p_buf = (uint8_t *)msg->req_param;
            uint16_t buf_len = msg->req_len;
            ret = tkl_hci_cmd_packet_send(p_buf, buf_len);
        }
            break;

        case TKL_IPC_TYPE_HCI_ACL_SEND:
        {
            uint8_t *p_buf = (uint8_t *)msg->req_param;
            uint16_t buf_len = msg->req_len;
            ret = tkl_hci_acl_packet_send(p_buf, buf_len);
        }
            break;

        case TKL_IPC_TYPE_HCI_REG_CALLBACK:
            ret = tkl_hci_callback_register(NULL, NULL);
            break;

        default:
            break;
    }

    msg->ret_value = ret;
    tuya_ipc_send_no_sync(msg);

    return;
}

OPERATE_RET tkl_hci_init(VOID)
{
//    bk_printf("%s\n", __func__);
    //bk_bluetooth_init();
    ble_init_flag = TRUE;
    return OPRT_OK;
}

OPERATE_RET tkl_hci_deinit(VOID)
{
#if TKL_DEBUG
    bk_printf("%s\n", __func__);
#endif

    bk_bluetooth_deinit();

    return OPRT_OK;
}

OPERATE_RET tkl_hci_reset(VOID)
{
#if TKL_DEBUG
    bk_printf("%s\n", __func__);
#endif

    bk_bluetooth_deinit();
    bk_bluetooth_init();

    return OPRT_OK;
}

OPERATE_RET tkl_hci_cmd_packet_send(const uint8_t *p_buf, uint16_t buf_len)
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


OPERATE_RET tkl_hci_acl_packet_send(const uint8_t *p_buf, uint16_t buf_len)
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

static ble_err_t _ble_hci_evt_to_host_cb(uint8_t *buf, uint16_t len)
{
    struct ipc_msg_s hci_msg = {0};
    memset(&hci_msg, 0, sizeof(struct ipc_msg_s));
    hci_msg.type = TKL_IPC_TYPE_HCI;
    hci_msg.subtype = TKL_IPC_TYPE_HCI_EVENT_TO_HOST;
    hci_msg.req_param = buf;
    hci_msg.req_len = len;

#if TKL_DEBUG  >= 5
    bk_printf("cpu 0: %s\n", __func__);
    bk_printf("cpu 0: ====================>\n");
    tal_log_hex_dump(0, __FILE__,  __LINE__, "data", 64, buf, len);
    bk_printf("cpu 0: <====================\n");
#endif

    OPERATE_RET ret = tuya_ipc_send_sync(&hci_msg);
    if(ret)
        return ret;

    return hci_msg.ret_value;
}

static ble_err_t _ble_hci_acl_to_host_cb(uint8_t *buf, uint16_t len)
{
    struct ipc_msg_s hci_msg = {0};
    memset(&hci_msg, 0, sizeof(struct ipc_msg_s));
    hci_msg.type = TKL_IPC_TYPE_HCI;
    hci_msg.subtype = TKL_IPC_TYPE_HCI_ACL_TO_HOST;
    hci_msg.req_param = buf;
    hci_msg.req_len = len;

#if TKL_DEBUG  >= 5
    bk_printf("cpu 0: %s\n", __func__);
    bk_printf("cpu 0: ====================>\n");
    tal_log_hex_dump(0, __FILE__,  __LINE__, "data", 64, buf, len);
    bk_printf("cpu 0: <====================\n");
#endif

    OPERATE_RET ret = tuya_ipc_send_sync(&hci_msg);
    if(ret)
        return ret;

    return hci_msg.ret_value;
}


OPERATE_RET tkl_hci_callback_register(const TKL_HCI_FUNC_CB hci_evt_cb, const TKL_HCI_FUNC_CB acl_pkt_cb)
{
#if TKL_DEBUG
    bk_printf("%s %p %p\n", __func__, hci_evt_cb, acl_pkt_cb);
#endif

    ble_err_t ret = 0;
    ret = bk_ble_reg_hci_recv_callback(_ble_hci_evt_to_host_cb, _ble_hci_acl_to_host_cb);
    if(ret == BK_ERR_BLE_SUCCESS) {
        return OPRT_OK;
    } else {
        return OPRT_COM_ERROR;
    }
}

#endif
