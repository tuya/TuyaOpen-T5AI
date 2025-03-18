/*
 * tuya_tkl_ipc.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "sdkconfig.h"
#include "tkl_ipc.h"
#include "tkl_fs.h"
#include "tkl_semaphore.h"
#include "tkl_mutex.h"

// for debug

#if TKL_IPC_DEBUG
const char *ipc_event_str[4] = {"wifi", "hci", "lwip", "system"};
const char *ipc_subevent_str[4][32] = {
    "wf_scan",  "wf_scan_res", "wf_scan_free_mem", "wf_ap_start", "wf_ap_stop", "wf_channel_set", "wf_channel_get", "wf_sniffer_set", "wf_sniffer_data", "wf_ip_get", "wf_ipv6_get", "wf_ip_set", "wf_mac_set", "wf_mac_get", "wf_workmode_set", "wf_workmode_get", "wf_fastconn_info_get", "wf_bssid_get", "wf_ccode_set", "wf_ccode_get", "wf_rf_calibrated_set", "wf_lp_mode_set", "wf_fast_connect", "wf_connect", "wf_connect_res", "wf_disconnect", "wf_rssi_get", "wf_station_status_get", "wf_mgnt_send", "wf_recv_mgnt_set", "wf_mgnt_recv", "wf_ioctl",
    "hci_init", "hci_deinit",   "hci_reset", "hci_cmd_send", "hci_acl_send", "hci_event_to_host", "hci_acl_to_host", "hci_reg_callback", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event", "no_hci_event",
    "lwip_init","lwip_get_netif_by_idx", "lwip_send", "lwip_recv", "lwip_pbuf_alloc", "lwip_pbuf_cat", "lwip_pbuf_coalesce", "lwip_pbuf_free", "lwip_pbuf_header", "lwip_pbuf_ref", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event", "no_lwip_event",
    "reboot", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event", "no_sys_event"
};
#endif


TKL_SEM_HANDLE ipc_ack_sem = NULL;
TKL_MUTEX_HANDLE ipc_mutex0= NULL;
TKL_MUTEX_HANDLE ipc_mutex1= NULL;
struct ipc_msg_s *ipc_req_msg = NULL;
extern void tkl_wifi_ipc_func(struct ipc_msg_s *msg);
extern void tkl_hci_ipc_func(struct ipc_msg_s *msg);
extern void tkl_lwip_ipc_func(struct ipc_msg_s *msg);

static OPERATE_RET __tuya_ipc_cb(struct ipc_msg_s *msg )
{
#if TKL_IPC_DEBUG
    tkl_ipc_debug(IPC_RECV, msg->type, msg->subtype);
#endif
    if(ipc_req_msg && ipc_req_msg->type == msg->type && ipc_req_msg->subtype == msg->subtype) {//res...��req��ͬһ��msg��ַ
        tkl_semaphore_post(ipc_ack_sem);
    }else {
        switch (msg->type) {
            //wifi
            case TKL_IPC_TYPE_WIFI:
                tkl_wifi_ipc_func(msg);
                break;

            case TKL_IPC_TYPE_HCI:
                tkl_hci_ipc_func(msg);
                break;

            case TKL_IPC_TYPE_LWIP:
                tkl_lwip_ipc_func(msg);
                break;

            case TKL_IPC_TYPE_SYS:
                tkl_sys_ipc_func(msg);
                break;

            case TKL_IPC_TYPE_TEST:
#if CONFIG_SYS_CPU1
                // tkl_test_ipc_func(msg);
#endif
                break;

            default:
                break;

        }
    }

    return 0;
}

OPERATE_RET tuya_ipc_init(void)
{
    TKL_IPC_CONF_T ipc_conf;
    ipc_conf.cb = __tuya_ipc_cb;
    OPERATE_RET ret = tkl_ipc_init(&ipc_conf);
    if(ret)
        return ret;

    ret = tkl_semaphore_create_init(&ipc_ack_sem, 0, 1);
    if(ret)
        return ret;

    ret = tkl_mutex_create_init(&ipc_mutex0);
    if(ret) {
        tkl_semaphore_release(ipc_ack_sem);
        ipc_ack_sem = NULL;
        return ret;
    }

    ret = tkl_mutex_create_init(&ipc_mutex1);
    if(ret) {
        tkl_semaphore_release(ipc_ack_sem);
        tkl_mutex_release(ipc_mutex0);
        ipc_ack_sem = NULL;
        return ret;
    }

    return 0;
}

OPERATE_RET tuya_ipc_send_sync(struct ipc_msg_s *msg)
{
    if(msg == NULL)
        return OPRT_INVALID_PARM;

    tkl_mutex_lock(ipc_mutex1);

    ipc_req_msg = msg;

#if TKL_IPC_DEBUG
    tkl_ipc_debug(IPC_SEND_SYNC_START, msg->type, msg->subtype);
#endif
    tkl_ipc_send_no_sync((uint8_t *)msg, sizeof(struct ipc_msg_s));

    // wait ack
    OPERATE_RET ret = tkl_semaphore_wait(ipc_ack_sem, 5000);
#if TKL_IPC_DEBUG
    if (ret == OPRT_OS_ADAPTER_SEM_WAIT_FAILED) {
        tkl_ipc_debug(IPC_SEND_SYNC_TIMEOUT, msg->type, msg->subtype);
    } else {
        tkl_ipc_debug(IPC_SEND_SYNC_COMPLETE, msg->type, msg->subtype);
    }
#endif

    ipc_req_msg = NULL;

    tkl_mutex_unlock(ipc_mutex1);

    return ret;
}


OPERATE_RET tuya_ipc_send_no_sync(struct ipc_msg_s *msg)
{
    if(msg == NULL)
        return OPRT_INVALID_PARM;

    tkl_mutex_lock(ipc_mutex0);
#if TKL_IPC_DEBUG
    tkl_ipc_debug(IPC_SEND_NO_SYNC_START, msg->type, msg->subtype);
#endif

    tkl_ipc_send_no_sync((uint8_t *)msg, sizeof(struct ipc_msg_s));

#if TKL_IPC_DEBUG
    tkl_ipc_debug(IPC_SEND_NO_SYNC_COMPLETE, msg->type, msg->subtype);
#endif

    tkl_mutex_unlock(ipc_mutex0);

    return 0;
}

