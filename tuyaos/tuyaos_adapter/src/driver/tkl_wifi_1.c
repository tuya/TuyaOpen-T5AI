#include <os/os.h>
#include <os/str.h>
#include <modules/wifi_types.h>
#include "tkl_wifi.h"
#include "tuya_error_code.h"
#include "tkl_semaphore.h"
#include "tkl_mutex.h"
#include "tkl_memory.h"
#include "tkl_system.h"
#include "tkl_ipc.h"
#include "tkl_lwip.h"
#include "lwip/netif.h"
#include "lwip/apps/dhcpserver.h"
#include "lwip/apps/dhcpserver_options.h"
/***********************************************************
 *************************micro define***********************
 ***********************************************************/


/***********************************************************
 *************************variable define********************
 ***********************************************************/
struct wlan_fast_connect_info1
{
	uint8_t ssid[33]; /**< SSID of AP */
	uint8_t bssid[6]; /**< BSSID of AP */
	uint8_t security; /**< Security of AP */
	uint8_t channel;  /**< Channel of AP */
	uint8_t psk[65];  /**< PSK of AP */
	uint8_t pwd[65];  /**< password of AP */
	uint8_t ip_addr[4];
	uint8_t netmask[4];
	uint8_t gw[4];
	uint8_t dns1[4];
};//此结构体存在的作用是为了解析和设置cpu0用到的快连结构体（这样cpu1不需要引用cpu0 wifi的头文件）

static TKL_SEM_HANDLE scanHandle = NULL;
// static SNIFFER_CALLBACK snif_cb = NULL;
// static WIFI_REV_MGNT_CB mgnt_recv_cb = NULL;
static WIFI_EVENT_CB wifi_event_cb = NULL;
extern void os_mem_dump(const char *title, unsigned char *data, uint32_t data_len);
extern void bk_printf(const char *fmt, ...);

// struct ipc_msg_s wf_ipc_msg = {0};
FAST_DHCP_INFO_T fast_dhcp_s = {0};
AP_IF_S *scanap = NULL;
uint32_t apcnt = 0;
static uint8_t get_ip_flag = 0;
static uint8_t station_connect_flag = 0;
static WF_WK_MD_E wf_mode = WWM_POWERDOWN;
extern OPERATE_RET tuya_ipc_send_sync(struct ipc_msg_s *msg);
extern OPERATE_RET tuya_ipc_send_no_sync(struct ipc_msg_s *msg);
/***********************************************************
 *************************function define********************
 ***********************************************************/

void static __fast_connect_ap_info_dump(char *tag, struct wlan_fast_connect_info1 *fci)
{
    bk_printf("----------------------------------------------------------------------------------------\r\n");
    if (tag != NULL) {
        bk_printf("%s\r\n", tag);
    }
    bk_printf("ssid: %s\r\n", fci->ssid);
    bk_printf("bssid: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            fci->bssid[0], fci->bssid[1], fci->bssid[2], fci->bssid[3], fci->bssid[4], fci->bssid[5]);
    bk_printf("security: %d, chan: %d\r\n", fci->security, fci->channel);
    bk_printf("psk: ********************************\r\n");
    bk_printf("pwd: ********************************\r\n");
    bk_printf("ip: %d.%d.%d.%d\r\n", fci->ip_addr[0], fci->ip_addr[1], fci->ip_addr[2], fci->ip_addr[3]);
    bk_printf("netmask: %d.%d.%d.%d\r\n", fci->netmask[0], fci->netmask[1], fci->netmask[2], fci->netmask[3]);
    bk_printf("gw: %d.%d.%d.%d\r\n", fci->gw[0], fci->gw[1], fci->gw[2], fci->gw[3]);
    bk_printf("dns1: %d.%d.%d.%d\r\n", fci->dns1[0], fci->dns1[1], fci->dns1[2], fci->dns1[3]);
    bk_printf("----------------------------------------------------------------------------------------\r\n");
}

void __dhcp_cb(LWIP_EVENT_E event, void *arg)
{
    bk_printf("__dhcp_cb event %d  %p\r\n", event, wifi_event_cb);
    if(event == IPV4_DHCP_SUCC) {
        // __dhcp_status = TKL_WIFI_DHCP_SUCCESS;
        get_ip_flag = 1;
        wifi_event_cb(WFE_CONNECTED, arg);
    }else if(event == IPV4_DHCP_FAIL){
        // __dhcp_status = TKL_WIFI_DHCP_FAIL;
        get_ip_flag = 0;
        wifi_event_cb(WFE_CONNECT_FAILED, arg);
    }

}

extern void tuya_dhcpv4_client_start_by_wq(void (*cb)(LWIP_EVENT_E event, void *arg), FAST_DHCP_INFO_T *ip_info);

static void __handle_wifi_event(WF_EVENT_E event, void *arg)
{
    bk_printf("_wifi_event_cb %d\r\n", event);
    switch (event) {
        case EVENT_WIFI_STA_CONNECTED:
            bk_printf("EVENT_WIFI_STA_CONNECTED %d %p %p\r\n", event, tuya_dhcpv4_client_start_by_wq, __dhcp_cb);
            get_ip_flag = 0;
            station_connect_flag = 1;
            tuya_dhcpv4_client_start_by_wq(__dhcp_cb, &fast_dhcp_s);
            break;
        case EVENT_WIFI_STA_DISCONNECTED:
            get_ip_flag = 0;
            station_connect_flag = 0;
            wifi_event_cb(WFE_CONNECT_FAILED, arg);
            break;

        default:
            bk_printf("rx event <%d>\r\n", event);
            break;
    }
}

void tkl_wifi_ipc_func(struct ipc_msg_s *msg)
{
    switch(msg->subtype) {
        case TKL_IPC_TYPE_WF_SCAN_RES:
            tuya_ipc_send_no_sync(msg);
            struct ipc_msg_param_s *res = (struct ipc_msg_param_s *)msg->req_param;

            scanap = res->p1;
            apcnt = *(int *)(res->p2);
            if(scanHandle)
                tkl_semaphore_post(scanHandle);

            break;

        case TKL_IPC_TYPE_WF_CONNECT_RES:
        {
            tuya_ipc_send_no_sync(msg);
            struct ipc_msg_param_s *res = (struct ipc_msg_param_s *)msg->req_param;
            int event_id = *(WF_EVENT_E *)(res->p1);
            int disconnect_reason = (res->p2) ? *(int *)(res->p2) : 0;
            __handle_wifi_event(event_id, &disconnect_reason);
        }
            break;

        default:
            break;
    }

    // tuya_ipc_send_no_sync(msg);

    return;
}

/**
 * @brief set wifi station work status changed callback
 *
 * @param[in]      cb        the wifi station work status changed callback
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_init(WIFI_EVENT_CB cb)
{
    wifi_event_cb = cb;

    return OPRT_OK;
}

/**
 * @brief scan current environment and obtain the ap
 *        infos in current environment
 *
 * @param[in]       ssid        the specific ssid
 * @param[out]      ap_ary      current ap info array
 * @param[out]      num         the num of ar_ary
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 *
 * @note if ssid == NULL means scan all ap, otherwise means scan the specific ssid
 */
OPERATE_RET tkl_wifi_scan_ap(const signed char *ssid, AP_IF_S **ap_ary, uint32_t *num)
{
    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_SCAN;
    if(ssid) {
        wf_ipc_msg.req_param = (uint8_t *)ssid;
        wf_ipc_msg.req_len = strlen((const char *)ssid);
    }

    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    if(wf_ipc_msg.ret_value)
        return wf_ipc_msg.ret_value;

    ret = tkl_semaphore_create_init(&scanHandle, 0, 1);
    if (ret !=  OPRT_OK) {
        return ret;
    }

    ret = tkl_semaphore_wait(scanHandle, 5000);
    tkl_semaphore_release(scanHandle);
    scanHandle = NULL;
    if (ret !=  OPRT_OK) {
        return ret;
    }

    *ap_ary = scanap;
    *num = apcnt;
    bk_printf("cpu1 recv scan res %d %p\r\n", apcnt, scanap);
    scanap = NULL;
    apcnt = 0;
    return (*num) ? 0 : -1;
}

/**
 * @brief release the memory malloced in <tkl_wifi_ap_scan>
 *        if needed. tuyaos will call this function when the
 *        ap info is no use.
 *
 * @param[in]       ap          the ap info
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_release_ap(AP_IF_S *ap)
{
    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_SCAN_FREE_MEM;
    wf_ipc_msg.req_param = (uint8_t *)ap;
    wf_ipc_msg.req_len = 0xff;//whatever
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    return wf_ipc_msg.ret_value;
}


static void _dhcps_cb(u8_t client_ip[4])
{
    bk_printf("dhcps : station req ip %x%x%x%x\r\n", client_ip[0], client_ip[1], client_ip[2], client_ip[3]);
}
/**
 * @brief start a soft ap
 *
 * @param[in]       cfg         the soft ap config
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_start_ap(const WF_AP_CFG_IF_S *cfg)
{
    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_AP_START;
    wf_ipc_msg.req_param = (uint8_t *)cfg;
    wf_ipc_msg.req_len = sizeof(WF_AP_CFG_IF_S);
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    if(wf_ipc_msg.ret_value)
        return wf_ipc_msg.ret_value;

    struct netif *netif = tkl_lwip_get_netif_by_index(1);
    ip4_addr_t ip,gw,netmask;
    ip.addr = ipaddr_addr(cfg->ip.ip);
    netmask.addr = ipaddr_addr(cfg->ip.mask);
    gw.addr = ipaddr_addr(cfg->ip.gw);
    netifapi_netif_set_addr(netif, &ip, &netmask, &gw);

    dhcps_set_new_lease_cb(_dhcps_cb);

    dhcps_start(netif, ip);

    return 0;
}

/**
 * @brief stop a soft ap
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_stop_ap(void)
{
    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_AP_STOP;
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    if(wf_ipc_msg.ret_value)
        return wf_ipc_msg.ret_value;

    struct netif * netif = tkl_lwip_get_netif_by_index(1);
    dhcps_stop(netif);

    return 0;
}

/**
 * @brief set wifi interface work channel
 *
 * @param[in]       chan        the channel to set
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_cur_channel(const uint8_t chan)
{
    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_CHANNEL_SET;
    wf_ipc_msg.req_param = (uint8_t *)&chan;
    wf_ipc_msg.req_len = 1;
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    return wf_ipc_msg.ret_value;
}

/**
 * @brief get wifi interface work channel
 *
 * @param[out]      chan        the channel wifi works
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_cur_channel(uint8_t *chan)
{
    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_CHANNEL_GET;
    wf_ipc_msg.res_param = (uint8_t *)&chan;
    wf_ipc_msg.res_len = 1;
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    return wf_ipc_msg.ret_value;
}

/**
 * @brief enable / disable wifi sniffer mode.
 *        if wifi sniffer mode is enabled, wifi recv from
 *        packages from the air, and user shoud send these
 *        packages to tuya-sdk with callback <cb>.
 *
 * @param[in]       en          enable or disable
 * @param[in]       cb          notify callback
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_sniffer(const BOOL_T en, const SNIFFER_CALLBACK cb)
{
#if 0
    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_SNIFFER_SET;
    wf_ipc_msg.req_param = &en;
    wf_ipc_msg.req_len = 1;
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    bk_printf("%s %d\r\n", __func__, __LINE__);

    if(ret)
        return ret;

    if(wf_ipc_msg.ret_value)
        return wf_ipc_msg.ret_value;

    if(en)
        snif_cb = cb;
    else
        snif_cb = NULL;

#endif
    return 0;
}

/**
 * @brief get wifi ip info.when wifi works in
 *        ap+station mode, wifi has two ips.
 *
 * @param[in]       wf          wifi function type
 * @param[out]      ip          the ip addr info
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_ip(const WF_IF_E wf, NW_IP_S *ip)
{
    int ret = OPRT_OK;

    if ((wf != WF_STATION) && (wf != WF_AP)) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    struct netif *netif = tkl_lwip_get_netif_by_index(wf);

    ip4addr_ntoa_r(&netif->ip_addr, ip->ip, 16);
    ip4addr_ntoa_r(&netif->netmask, ip->mask, 16);
    ip4addr_ntoa_r(&netif->gw, ip->gw, 16);

    return ret;
}


/**
 * @brief wifi set ip
 *
 * @param[in]       wf     wifi function type
 * @param[in]       ip     the ip addr info
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_ip(const WF_IF_E wf, NW_IP_S *ip)
{
    //通过lwip实现，不需要核间通信
    int ret = OPRT_OK;
    return ret;
}


/**
 * @brief set wifi mac info.when wifi works in
 *        ap+station mode, wifi has two macs.
 *
 * @param[in]       wf          wifi function type
 * @param[in]       mac         the mac info
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_mac(const WF_IF_E wf, const NW_MAC_S *mac)
{
    struct ipc_msg_s wf_ipc_msg = {0};
    struct ipc_msg_param_s param = {0};
    param.p1 = (void *)&wf;
    param.p2 = (void *)mac;
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_MAC_SET;
    wf_ipc_msg.req_param = (uint8_t *)&param;
    wf_ipc_msg.req_len = sizeof(struct ipc_msg_param_s);
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    return wf_ipc_msg.ret_value;
}

/**
 * @brief get wifi mac info.when wifi works in
 *        ap+station mode, wifi has two macs.
 *
 * @param[in]       wf          wifi function type
 * @param[out]      mac         the mac info
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_mac(const WF_IF_E wf, NW_MAC_S *mac)
{
    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_MAC_GET;
    wf_ipc_msg.req_param = (uint8_t *)&wf;
    wf_ipc_msg.req_len = 1;
    wf_ipc_msg.res_param = (uint8_t *)mac;
    wf_ipc_msg.res_len = 6;
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    return wf_ipc_msg.ret_value;
}

/**
 * @brief set wifi work mode
 *
 * @param[in]       mode        wifi work mode
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_work_mode(const WF_WK_MD_E mode)
{

    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_WORKMODE_SET;
    wf_ipc_msg.req_param = (uint8_t *)&mode;
    wf_ipc_msg.req_len = 4;
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    wf_mode = mode;
    return wf_ipc_msg.ret_value;
}

/**
 * @brief get wifi work mode
 *
 * @param[out]      mode        wifi work mode
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_work_mode(WF_WK_MD_E *mode)
{
    *mode = wf_mode;
    return OPRT_OK;
}

/**
 * @brief : get ap info for fast connect
 * @param[out]      fast_ap_info
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_connected_ap_info(FAST_WF_CONNECTED_AP_INFO_T **fast_ap_info)
{
    if(!fast_ap_info)
        return OPRT_COM_ERROR;

    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_FASTCONN_INFO_GET;
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    if(wf_ipc_msg.ret_value)
        return wf_ipc_msg.ret_value;

    FAST_WF_CONNECTED_AP_INFO_T *ap_info = NULL;
    ap_info = (FAST_WF_CONNECTED_AP_INFO_T *)tkl_system_malloc(wf_ipc_msg.res_len);
    if (ap_info == NULL) {
        return OPRT_COM_ERROR;
    }

    memcpy(ap_info, wf_ipc_msg.res_param, wf_ipc_msg.res_len);

    struct wlan_fast_connect_info1 *fci = (struct wlan_fast_connect_info1 *)ap_info->data;
    struct netif *netif = tkl_lwip_get_netif_by_index(0);

    memcpy(fci->ip_addr, &netif->ip_addr, 4);
    memcpy(fci->netmask, &netif->netmask, 4);
    memcpy(fci->gw, &netif->gw, 4);
    memcpy(fci->dns1, dns_getserver(0), 4);//get dns

    // bk_printf("get fast conn ip info: ip:%x, gw:%x, netmask:%x, dns:%x\r\n", *(uint32_t *)(fci->ip_addr),*(uint32_t *)(fci->gw),*(uint32_t *)(fci->netmask),*(uint32_t *)(fci->dns1));

    *fast_ap_info = ap_info;

	__fast_connect_ap_info_dump("get connected ap info", fci);
    return 0;
}

/**
 * @brief get wifi bssid
 *
 * @param[out]      mac         uplink mac
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_bssid(uint8_t *mac)
{
    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_BSSID_GET;
    wf_ipc_msg.res_param = mac;
    wf_ipc_msg.res_len = 6;
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    return wf_ipc_msg.ret_value;
}

/**
 * @brief set wifi country code
 *
 * @param[in]       ccode  country code
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_country_code(const COUNTRY_CODE_E ccode)
{
    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_CCODE_SET;
    wf_ipc_msg.req_param = (uint8_t *)&ccode;
    wf_ipc_msg.req_len = 4;
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    return wf_ipc_msg.ret_value;
}

/**
 * @brief set wifi country code
 *
 * @param[in]       ccode  country code
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_country_code_v2(const uint8_t *ccode)
{
    COUNTRY_CODE_E country_code = COUNTRY_CODE_CN;

    if(!strcmp((char *)ccode, "UNKNOW")) {
        bk_printf("country code is UNKNOW !!!\r\n");
        return OPRT_OK;
    }

    #define GET_CC(a,b)    (((((a)&0xff)<<8)) | ((b)&0xff))
    uint16_t result = GET_CC(ccode[0], ccode[1]);
    switch (result) {
        case GET_CC('C','N'): // CN
            country_code = COUNTRY_CODE_CN;
            break;
        case GET_CC('U','S'): // US
            country_code = COUNTRY_CODE_US;
            break;
        case GET_CC('J','P'): // JP
            country_code = COUNTRY_CODE_JP;
            break;
        case GET_CC('E','U'): // EU
            country_code = COUNTRY_CODE_EU;
            break;
        case GET_CC('T','R'): // TR
            country_code = COUNTRY_CODE_TR;
            break;
        default:
            bk_printf("country code not found !!!\r\n");
            break;
    }

    return tkl_wifi_set_country_code(country_code);
}

/**
 * @brief wifi_get_country_code    use wifi scan to get country code
 *
 * @param[in]       ccode   country code buffer to restore
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_country_code(uint8_t *ccode)
{
    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_CCODE_GET;
    wf_ipc_msg.res_param = ccode;
    wf_ipc_msg.res_len = 3;
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    if(wf_ipc_msg.ret_value)
        return wf_ipc_msg.ret_value;

    if(ret || wf_ipc_msg.ret_value) {
        memcpy(ccode, "UNKNOW", sizeof("UNKNOW"));
        return OPRT_NOT_FOUND;
    }

    return 0;
}

/**
 * @brief do wifi calibration
 *
 * @note called when test wifi
 *
 * @return true on success. faile on failure
 */
BOOL_T tkl_wifi_set_rf_calibrated(void)
{
    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_RF_CALIBRATED_SET;
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    return wf_ipc_msg.ret_value;
}

/**
 * @brief set wifi lowpower mode
 *
 * @param[in]       enable      enbale lowpower mode
 * @param[in]       dtim     the wifi dtim
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_lp_mode(const BOOL_T enable, const uint8_t dtim)
{
    struct ipc_msg_s wf_ipc_msg = {0};
    struct ipc_msg_param_s param = {0};
    param.p1 = (void *)&enable;
    param.p2 = (void *)&dtim;
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_LP_MODE_SET;
    wf_ipc_msg.req_param = (uint8_t *)&param;
    wf_ipc_msg.req_len = sizeof(struct ipc_msg_param_s);
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    return wf_ipc_msg.ret_value;
}

/**
 * @brief : fast connect
 * @param[in]      fast_ap_info
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_station_fast_connect(const FAST_WF_CONNECTED_AP_INFO_T *fast_ap_info)
{
    memset(&fast_dhcp_s, 0, sizeof(FAST_DHCP_INFO_T));
    struct wlan_fast_connect_info1 *fci = (struct wlan_fast_connect_info1 *)fast_ap_info->data;
    memcpy(fast_dhcp_s.ip, ip4addr_ntoa((const ip4_addr_t *)fci->ip_addr), 16);
    memcpy(fast_dhcp_s.mask, ip4addr_ntoa((const ip4_addr_t *)fci->netmask), 16);
    memcpy(fast_dhcp_s.gw, ip4addr_ntoa((const ip4_addr_t *)fci->gw), 16);
    memcpy(fast_dhcp_s.dns, ip4addr_ntoa((const ip4_addr_t *)fci->dns1), 16);
    bk_printf("fast conn ip info: ip:%s, gw:%s, netmask:%s, dns:%s\r\n", fast_dhcp_s.ip,fast_dhcp_s.gw,fast_dhcp_s.mask,fast_dhcp_s.dns);

    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_FAST_CONNECT;
    wf_ipc_msg.req_param = (uint8_t *)fast_ap_info;
    wf_ipc_msg.req_len = sizeof(FAST_WF_CONNECTED_AP_INFO_T) + fast_ap_info->len;

    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    return wf_ipc_msg.ret_value;
}


/**
 * @brief connect wifi with ssid and passwd
 *
 * @param[in]       ssid
 * @param[in]       passwd
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_station_connect(const signed char *ssid, const signed char *passwd)
{
    memset(&fast_dhcp_s, 0, sizeof(FAST_DHCP_INFO_T));

    struct ipc_msg_s wf_ipc_msg = {0};
    struct ipc_msg_param_s param = {0};
    param.p1 = (void *)ssid;
    param.p2 = (void *)passwd;
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_CONNECT;
    wf_ipc_msg.req_param = (uint8_t *)&param;
    wf_ipc_msg.req_len = sizeof(struct ipc_msg_param_s);
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    return wf_ipc_msg.ret_value;
}

/**
 * @brief disconnect wifi from connect ap
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_station_disconnect(void)
{
    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_DISCONNECT;
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    return wf_ipc_msg.ret_value;
}

/**
 * @brief get wifi connect rssi
 *
 * @param[out]      rssi        the return rssi
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_station_get_conn_ap_rssi(signed char *rssi)
{
    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_RSSI_GET;
    wf_ipc_msg.res_param = (uint8_t *)rssi;
    wf_ipc_msg.res_len = 1;
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    return wf_ipc_msg.ret_value;
}

/**
 * @brief get wifi station work status
 *
 * @param[out]      stat        the wifi station work status
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_station_get_status(WF_STATION_STAT_E *stat)
{
    WF_WK_MD_E mode;

    tkl_wifi_get_work_mode(&mode);
    if (mode != WWM_STATION) {
        *stat = WSS_IDLE;
        return OPRT_OK;
    }

    if (get_ip_flag == 1) {
        *stat = WSS_GOT_IP;
    } else if (station_connect_flag == 1) {
        *stat = WSS_CONN_SUCCESS;
    } else {
        *stat = WSS_CONN_FAIL;
    }

    // bk_printf("%s %d, stat: %d\r\n", __func__, __LINE__, *stat);
    return OPRT_OK;
}

/**
 * @brief send wifi management
 *
 * @param[in]       buf         pointer to buffer
 * @param[in]       len         length of buffer
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_send_mgnt(const uint8_t *buf, const uint32_t len)
{
#if 0
    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_MGNT_SEND;
    wf_ipc_msg.req_param = &buf;
    wf_ipc_msg.res_len = len;
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    return wf_ipc_msg.ret_value;
#endif
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief register receive wifi management callback
 *
 * @param[in]       enable
 * @param[in]       recv_cb     receive callback
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_register_recv_mgnt_callback(const BOOL_T enable, const WIFI_REV_MGNT_CB recv_cb)
{
#if 0
    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_RECV_MGNT_SET;
    wf_ipc_msg.req_param = &enable;
    wf_ipc_msg.res_len = 1;
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    if(wf_ipc_msg.ret_value)
        return wf_ipc_msg.ret_value;

    if (enable)
        mgnt_recv_cb = recv_cb;
    else
        mgnt_recv_cb = NULL;

    return 0;
#endif
    return OPRT_NOT_SUPPORTED;
}


/**
 * @brief wifi ioctl
 *
 * @param[in]       cmd     refer to WF_IOCTL_CMD_E
 * @param[in]       args    args associated with the command
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_ioctl(WF_IOCTL_CMD_E cmd, void *args)
{
    return OPRT_NOT_SUPPORTED;
#if 0
    struct parm{
        WF_IOCTL_CMD_E cmd;
        void *args;
    };
    struct parm tmp  = {0};
    tmp.cmd = cmd;
    tmp.args = args;

    struct ipc_msg_s wf_ipc_msg = {0};
    memset(&wf_ipc_msg, 0, sizeof(struct ipc_msg_s));
    wf_ipc_msg.type = TKL_IPC_TYPE_WIFI;
    wf_ipc_msg.subtype = TKL_IPC_TYPE_WF_IOCTL;
    wf_ipc_msg.req_param = (uint8_t *)&tmp;
    wf_ipc_msg.res_len = sizeof(struct parm);
    OPERATE_RET ret = tuya_ipc_send_sync(&wf_ipc_msg);
    if(ret)
        return ret;

    return wf_ipc_msg.ret_value;
#endif
}



