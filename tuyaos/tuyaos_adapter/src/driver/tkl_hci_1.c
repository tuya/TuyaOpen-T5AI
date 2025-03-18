#include <stdbool.h>
#include "tkl_hci.h"
#include "tkl_ipc.h"
/***********************************************************
 *************************micro define***********************
 ***********************************************************/

#define TKL_DEBUG 1

 /***********************************************************
 *************************variable define********************
 ***********************************************************/
static TKL_HCI_FUNC_CB s_evt_cb = NULL;
static TKL_HCI_FUNC_CB s_acl_cb = NULL;
extern OPERATE_RET tuya_ipc_send_sync(struct ipc_msg_s *msg);
extern OPERATE_RET tuya_ipc_send_no_sync(struct ipc_msg_s *msg);
extern void bk_printf(const char *fmt, ...);
static int _ble_hci_evt_to_host_cb(uint8_t *buf, uint16_t len);
static int _ble_hci_acl_to_host_cb(uint8_t *buf, uint16_t len);

/***********************************************************
 *************************function define********************
 ***********************************************************/
void tkl_hci_ipc_func(struct ipc_msg_s *msg)
{
    switch(msg->subtype) {
        case TKL_IPC_TYPE_HCI_EVENT_TO_HOST:
        {
            tuya_ipc_send_no_sync(msg);
            uint8_t *buf = (uint8_t *)msg->req_param;
            uint16_t len = msg->req_len;
            _ble_hci_evt_to_host_cb(buf, len);
        }
            break;

        case TKL_IPC_TYPE_HCI_ACL_TO_HOST:
        {
            tuya_ipc_send_no_sync(msg);
            uint8_t *buf = (uint8_t *)msg->req_param;
            uint16_t len = msg->req_len;
            _ble_hci_acl_to_host_cb(buf, len);
        }
            break;

        default:
            break;
    }

    // tuya_ipc_send_no_sync(msg);

    return;
}


OPERATE_RET tkl_hci_init(VOID)
{
    struct ipc_msg_s hci_msg;
    memset(&hci_msg, 0, sizeof(struct ipc_msg_s));
    hci_msg.type = TKL_IPC_TYPE_HCI;
    hci_msg.subtype = TKL_IPC_TYPE_HCI_INIT;
    OPERATE_RET ret = tuya_ipc_send_sync(&hci_msg);
    if(ret)
        return ret;

    return hci_msg.ret_value;
}

OPERATE_RET tkl_hci_deinit(VOID)
{
    struct ipc_msg_s hci_msg;
    memset(&hci_msg, 0, sizeof(struct ipc_msg_s));
    hci_msg.type = TKL_IPC_TYPE_HCI;
    hci_msg.subtype = TKL_IPC_TYPE_HCI_DEINIT;
    OPERATE_RET ret = tuya_ipc_send_sync(&hci_msg);
    if(ret)
        return ret;

    return hci_msg.ret_value;
}

OPERATE_RET tkl_hci_reset(VOID)
{
    struct ipc_msg_s hci_msg;
    memset(&hci_msg, 0, sizeof(struct ipc_msg_s));
    hci_msg.type = TKL_IPC_TYPE_HCI;
    hci_msg.subtype = TKL_IPC_TYPE_HCI_RESET;
    OPERATE_RET ret = tuya_ipc_send_sync(&hci_msg);
    if(ret)
        return ret;

    return hci_msg.ret_value;
}

OPERATE_RET tkl_hci_cmd_packet_send(CONST UCHAR_T *p_buf, USHORT_T buf_len)
{
    struct ipc_msg_s hci_msg;
    memset(&hci_msg, 0, sizeof(struct ipc_msg_s));
    hci_msg.type = TKL_IPC_TYPE_HCI;
    hci_msg.subtype = TKL_IPC_TYPE_HCI_CMD_SEND;
    hci_msg.req_param = (uint8_t *)p_buf;
    hci_msg.req_len = buf_len;

#if TKL_DEBUG >= 5
    bk_printf("%s op 0x%04X\n", __func__, (uint16_t)((((uint16_t)p_buf[1]) << 8) | p_buf[0]));
    bk_printf("====================>\n");
    tal_log_hex_dump(0, __FILE__,  __LINE__, "data", 64, p_buf, buf_len);
    bk_printf("<====================\n");
#endif


    OPERATE_RET ret = tuya_ipc_send_sync(&hci_msg);//这里这么做的前提是，cpu0 ble数据发送是同步的或者做了一层数据拷贝
    if(ret)
        return ret;

    return hci_msg.ret_value;

}


OPERATE_RET tkl_hci_acl_packet_send(CONST UCHAR_T *p_buf, USHORT_T buf_len)
{
    struct ipc_msg_s hci_msg;
    memset(&hci_msg, 0, sizeof(struct ipc_msg_s));
    hci_msg.type = TKL_IPC_TYPE_HCI;
    hci_msg.subtype = TKL_IPC_TYPE_HCI_ACL_SEND;
    hci_msg.req_param = (uint8_t *)p_buf;
    hci_msg.req_len = buf_len;

#if TKL_DEBUG  >= 5
    bk_printf("%s handle 0x%04X\n", __func__, (uint16_t)((((uint16_t)p_buf[1]) << 8) | p_buf[0]));
    bk_printf("====================>\n");
    tal_log_hex_dump(0, __FILE__,  __LINE__, "data", 64, p_buf, buf_len);
    bk_printf("<====================\n");
#endif

    OPERATE_RET ret = tuya_ipc_send_sync(&hci_msg); //这里这么做的前提是，cpu0 ble数据发送是同步的或者做了一层数据拷贝
    if(ret)
        return ret;

    return hci_msg.ret_value;

}

static int _ble_hci_evt_to_host_cb(uint8_t *buf, uint16_t len)
{

#if TKL_DEBUG  >= 5
    bk_printf("%s\n", __func__);
    bk_printf("====================>\n");
    tal_log_hex_dump(0, __FILE__,  __LINE__, "data", 64, buf, len);
    bk_printf("<====================\n");
#endif

    if(s_evt_cb) {
        s_evt_cb(buf, len);
    }else {
        bk_printf("s_evt_cb = NULL!  error!!!");
    }

    return OPRT_OK;
}

static int _ble_hci_acl_to_host_cb(uint8_t *buf, uint16_t len)
{

#if TKL_DEBUG  >= 5
    bk_printf("%s\n", __func__);
    bk_printf("====================>\n");
    tal_log_hex_dump(0, __FILE__,  __LINE__, "data", 64, buf, len);
    bk_printf("<====================\n");
#endif

    if(s_acl_cb) {
        s_acl_cb(buf, len);
    }else {
        bk_printf("s_acl_cb = NULL!  error!!!");
    }

    return OPRT_OK;
}


OPERATE_RET tkl_hci_callback_register(CONST TKL_HCI_FUNC_CB hci_evt_cb, CONST TKL_HCI_FUNC_CB acl_pkt_cb)
{
    s_evt_cb = hci_evt_cb;
    s_acl_cb = acl_pkt_cb;

    struct ipc_msg_s hci_msg;
    memset(&hci_msg, 0, sizeof(struct ipc_msg_s));
    hci_msg.type = TKL_IPC_TYPE_HCI;
    hci_msg.subtype = TKL_IPC_TYPE_HCI_REG_CALLBACK;
    OPERATE_RET ret = tuya_ipc_send_sync(&hci_msg);
    if(ret)
        return ret;

    return hci_msg.ret_value;
}

