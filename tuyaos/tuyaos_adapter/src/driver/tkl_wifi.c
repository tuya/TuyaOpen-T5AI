#include "tkl_wifi.h"
#include "string.h"
#include "tuya_error_code.h"
#include "tkl_semaphore.h"
#include "tkl_queue.h"
#include "tkl_thread.h"
#include "tkl_memory.h"
#include "tkl_system.h"

#include "uart_pub.h"
#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>
#include <modules/wifi.h>
#include <modules/wifi_types.h>
#include <modules/pm.h>
#include <components/netif.h>
#include <components/netif_types.h>
#include <components/event.h>
#include "bk_wifi.h"
#include <driver/aon_rtc_types.h>
#include <driver/hal/hal_aon_rtc_types.h>
#include <driver/aon_rtc.h>
// #include "event.h"
#include <components/bluetooth/bk_dm_bluetooth.h>


#if CONFIG_SAVE_BOOT_TIME_POINT
#include "boot.h"
#endif

/***********************************************************
 *************************micro define***********************
 ***********************************************************/

#define WIFI_MGNT_FRAME_RX_MSG              (1 << 0)
#define WIFI_MGNT_FRAME_TX_MSG              (1 << 1)

#define SCAN_MAX_AP 64

#define DEBUG_EZ_MODE 0

#define BEACON_LOSS_INTERVAL    1    //any value is acceptable,it is recommended that this value less than listen interval
#define BEACON_LOSS_REPEAT_NUM  2    //set beacon loss interval number,the minimum value is 1

#define BEACON_REV_DEF_WIN      6    //default beacon receive window(ms),minimum is 5,less than or equal to max_win.
#define BEACON_REV_CURR_MAX_WIN 10   //max beacon receive window(ms),maximum is 20.
#define BEACON_REV_STEP_WIN     2    //increase the beacon reception time by a step value(ms)

#define BEACON_LOSS_WAIT_CNT    5    //failed to receive beacon for wait_cnt consecutive cycles, close ps,wait_cnt corresponds to dtimx(listen_interval)
#define BEACON_LOSS_WAKE_CNT    50   //after ps is turned off,the beacon fails to receive for wake_cnt consecutive cycles, disconnect;wake_cnt corresponds to dtim1


/***********************************************************
 *************************variable define********************
 ***********************************************************/
typedef struct {
    unsigned char msg_id;
    unsigned int len;
    unsigned char *data;
}wifi_mgnt_frame;

static WF_WK_MD_E wf_mode = WWM_POWERDOWN;
static TKL_SEM_HANDLE scanHandle = NULL;
static SNIFFER_CALLBACK snif_cb = NULL;
static WIFI_REV_MGNT_CB mgnt_recv_cb = NULL;
static WIFI_EVENT_CB wifi_event_cb = NULL;
static unsigned char mgnt_cb_exist_flag = 0;
static unsigned char first_set_flag = TRUE;
static BOOL_T wifi_lp_flag = FALSE;

extern BOOL_T ble_init_flag;

wifi_country_t country_code[] =
{
    {.cc= "CN", .schan=1, .nchan=13, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL},
    {.cc= "US", .schan=1, .nchan=11, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL},
    {.cc= "JP", .schan=1, .nchan=14, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL},
    {.cc= "EU", .schan=1, .nchan=13, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL},
    {.cc= "TR", .schan=1, .nchan=13, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL},
    //{.cc= "AU", .schan=1, .nchan=13, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL}
};

static int wifi_event_init = 1;
static int lp_wifi_cfg_flag = 0;

static void tkl_wifi_powersave_disable(void);
static void tkl_wifi_powersave_enable(void);

extern void os_mem_dump(const char *title, unsigned char *data, uint32_t data_len);
extern void etharp_remove_all_static(void);
extern void _bk_rtc_wakeup_register(unsigned int rtc_time) ;
extern void _bk_rtc_wakeup_unregister() ;
extern void bk_printf(const char *fmt, ...);


void static __fast_connect_ap_info_dump(char *tag, struct wlan_fast_connect_info *fci)
{
    bk_printf("----------------------------------------------------------------------------------------\r\n");
    if (tag != NULL) {
        bk_printf("%s\r\n", tag);
    }
    bk_printf("ssid: %s\r\n", fci->ssid);
    bk_printf("bssid: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            fci->bssid[0], fci->bssid[1], fci->bssid[2], fci->bssid[3], fci->bssid[4], fci->bssid[5]);
    bk_printf("security: %d, chan: %d\r\n", fci->security, fci->channel);
    bk_printf("psk: ********************************, pmf: %d\r\n", fci->pmf);
    bk_printf("pwd: ****************\r\n");
    bk_printf("ip: %d.%d.%d.%d\r\n", fci->ip_addr[0], fci->ip_addr[1], fci->ip_addr[2], fci->ip_addr[3]);
    bk_printf("netmask: %d.%d.%d.%d\r\n", fci->netmask[0], fci->netmask[1], fci->netmask[2], fci->netmask[3]);
    bk_printf("gw: %d.%d.%d.%d\r\n", fci->gw[0], fci->gw[1], fci->gw[2], fci->gw[3]);
    bk_printf("dns1: %d.%d.%d.%d\r\n", fci->dns1[0], fci->dns1[1], fci->dns1[2], fci->dns1[3]);

#if CONFIG_WLAN_FAST_CONNECT_WITHOUT_SCAN
    bk_printf("freq: %d, beacon_int: %d, caps: %d, level: %d, ie_len: %d\r\n", fci->freq, fci->beacon_int, fci->caps, fci->level, fci->ie_len);
#endif

#if CONFIG_WLAN_FAST_CONNECT_WPA3
    bk_printf("====== wpa3 ======\r\n");
    bk_printf("pmk_len: %d, pmk:\r\n", fci->pmk_len);
    for (int i = 0; i < fci->pmk_len; i++) {
        bk_printf("%02x", fci->pmk[i]);
    }
    bk_printf("\r\npmkid:\r\n");
    for (int i = 0; i < 16; i++) {
        bk_printf("%02x\r\n", fci->pmkid[i]);
    }
    bk_printf("\r\n");
    bk_printf("akmp: %x\r\n", fci->akmp);
#endif
    bk_printf("----------------------------------------------------------------------------------------\r\n");

}

static __notify_wifi_event(WF_EVENT_E event, void *arg)
{
    if (wifi_event_cb == NULL) {
        return;
	} else if (wifi_event_cb) {
        bk_printf("%s report event: %d\r\n", __func__, event);
        wifi_event_cb(event, arg);
    }
}

BOOL_T tkl_get_lp_flag(void)
{
    return wifi_lp_flag;
}

/**
 * @brief set wifi station work status changed callback
 *
 * @param[in]      cb        the wifi station work status changed callback
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_init(WIFI_EVENT_CB cb)
{
    bk_printf("%s\r\n", __func__);
    wifi_event_cb = cb;

    return OPRT_OK;
}

/**
 * @brief 原厂 scan 结束的CB
 *
 * @param[out]
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
static int scan_cb(void *arg, event_module_t event_module,
								  int event_id, void *_event_data)
{
	uint32_t thread_id = (uint32_t)arg;
	wifi_event_scan_done_t *event_data = _event_data;

	if (thread_id == event_data->scan_id) {
        if(scanHandle) {
            tkl_semaphore_post(scanHandle);
        }
    }

    return 0;
}

static UCHAR_T _bk_wifi_security_to_ty(wifi_security_t bk_security)
{
    switch(bk_security)  {
        case WIFI_SECURITY_NONE:
            return WAAM_OPEN;
        break;

        case WIFI_SECURITY_WEP:
            return WAAM_WEP;
        break;

        case WIFI_SECURITY_WPA_TKIP:
        case WIFI_SECURITY_WPA_AES:
        case WIFI_SECURITY_WPA_MIXED:
        case WIFI_SECURITY_WPA2_TKIP:
        case WIFI_SECURITY_WPA2_AES:
        case WIFI_SECURITY_WPA2_MIXED:
            return WAAM_WPA_WPA2_PSK;
        break;

        case WIFI_SECURITY_WPA3_SAE:
        case WIFI_SECURITY_WPA3_WPA2_MIXED:
            return WAAM_WPA_WPA3_SAE;
        break;

        case WIFI_SECURITY_EAP:
        case WIFI_SECURITY_OWE:
        case WIFI_SECURITY_AUTO:
            return WAAM_UNKNOWN;
        break;

        default:
            return WAAM_UNKNOWN;
        break;
    }
}

/**
 * @brief scan current environment and obtain all the ap
 *        infos in current environment
 *
 * @param[out]      ap_ary      current ap info array
 * @param[out]      num         the num of ar_ary
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
static OPERATE_RET tkl_wifi_all_ap_scan(AP_IF_S **ap_ary, unsigned int *num)
{
    AP_IF_S *item;
    AP_IF_S *array = NULL;
    OPERATE_RET ret;
    int i;
    int scan_cnt;
    int ssid_len;
    wifi_scan_result_t scan_result = {0};
    uint32_t semaphore_timeout = 5000;  //ms

    if((NULL == ap_ary) || (NULL == num) || NULL != scanHandle) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    ret = tkl_semaphore_create_init(&scanHandle, 0, 1);
    if (ret !=  OPRT_OK) {
        return ret;
    }

    bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_SCAN_DONE,
            scan_cb, rtos_get_current_thread());
    BK_LOG_ON_ERR(bk_wifi_scan_start(NULL));

    ret = tkl_semaphore_wait(scanHandle,semaphore_timeout);
    tkl_semaphore_release(scanHandle);
    scanHandle = NULL;
    if (ret !=  OPRT_OK) {
        //bk_printf("scan wait sem error\r\n");
        return ret;
    }

    if ((bk_wifi_scan_get_result(&scan_result) != 0) || (0 == scan_result.ap_num)) {
        bk_printf("scan err\r\n");
        goto SCAN_ERR;
    }

    scan_cnt = scan_result.ap_num;

    if (scan_cnt > SCAN_MAX_AP) {
        scan_cnt = SCAN_MAX_AP;
    }

    if(0 == scan_cnt) {
        goto SCAN_ERR;
    }

    array = ( AP_IF_S *)tkl_system_malloc(SIZEOF(AP_IF_S) * scan_cnt);
    if(NULL == array){
        goto SCAN_ERR;
    }

    tkl_system_memset(array, 0, (sizeof(AP_IF_S) * scan_cnt));
    for (i = 0; i < scan_cnt; i++) {
        item = &array[i];

        item->security = _bk_wifi_security_to_ty(scan_result.aps[i].security);
        item->channel = scan_result.aps[i].channel;
        item->rssi = scan_result.aps[i].rssi;
        os_memcpy(item->bssid, scan_result.aps[i].bssid, 6);

        ssid_len = os_strlen(scan_result.aps[i].ssid);
        if (ssid_len > WIFI_SSID_LEN) {
            ssid_len = WIFI_SSID_LEN;
        }

        memset(item->ssid, 0, ssid_len);
        os_strncpy((char *)item->ssid, scan_result.aps[i].ssid, ssid_len);
        item->s_len = ssid_len;

    }

    *ap_ary = array;
    *num = scan_cnt & 0xff;
    if (scan_result.aps != NULL) {
        tkl_system_free(scan_result.aps);
    }

    return  OPRT_OK;

SCAN_ERR:
    if (scan_result.aps != NULL) {
        tkl_system_free(scan_result.aps);
        scan_result.aps = NULL;
    }

    if(array) {
        tkl_system_free(array);
        array = NULL;
    }

    return OPRT_OS_ADAPTER_COM_ERROR;
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
OPERATE_RET tkl_wifi_scan_ap(const SCHAR_T *ssid, AP_IF_S **ap_ary, uint32_t *num)
{
    if(first_set_flag) {
        // extern void extended_app_waiting_for_launch(void);
        // extended_app_waiting_for_launch(); /* 首次等待wifi初始化OK */
        first_set_flag = FALSE;
    }

    if(NULL == ssid)
    {
        return tkl_wifi_all_ap_scan(ap_ary, num);
    }

    AP_IF_S *array = NULL;
    OPERATE_RET ret;
    int i = 0, j = 0, ssid_len;
    wifi_scan_result_t apList;
    uint32_t semaphore_timeout = 5000;  //ms

    tkl_system_memset(&apList, 0, sizeof(wifi_scan_result_t));

    if((NULL == ssid) || (NULL == ap_ary) ||  NULL != scanHandle) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    ret = tkl_semaphore_create_init(&scanHandle, 0, 1);
    if ( ret !=  OPRT_OK ) {
        return ret;
    }

    bk_event_register_cb(EVENT_MOD_WIFI, EVENT_WIFI_SCAN_DONE,
            scan_cb, rtos_get_current_thread());
    wifi_scan_config_t scan_config = {0};
    strncpy(scan_config.ssid, (char*)ssid, WIFI_SSID_STR_LEN);
    BK_LOG_ON_ERR(bk_wifi_scan_start(&scan_config));

    ret = tkl_semaphore_wait(scanHandle,semaphore_timeout);
    tkl_semaphore_release(scanHandle);
    scanHandle = NULL;

    if (ret !=  OPRT_OK) {
        //bk_printf("scan wait sem error\r\n");
        return ret;
    }

    if ((bk_wifi_scan_get_result(&apList) != 0) || (0 == apList.ap_num)) {
        bk_printf("scan err\r\n");
        goto SCAN_ERR;
    }

    array = (AP_IF_S *)tkl_system_malloc(sizeof(AP_IF_S));
    if(NULL == array) {
        goto SCAN_ERR;
    }

    tkl_system_memset(array, 0, sizeof(AP_IF_S));
    array->rssi = -100;

    for (i = 0; i < apList.ap_num ; i++) {
        /* skip non-matched ssid */
        if (os_strcmp(apList.aps[i].ssid, (char *)ssid))
            continue;

        /* found */
        if (apList.aps[i].rssi < array->rssi) { /* rssi 信号强度比较 */
            continue;
        }

        array->security = _bk_wifi_security_to_ty(apList.aps[i].security);
        array->channel = apList.aps[i].channel;
        array->rssi = apList.aps[i].rssi;
        os_memcpy(array->bssid, apList.aps[i].bssid, 6);

        ssid_len = os_strlen(apList.aps[i].ssid);
        if (ssid_len > WIFI_SSID_LEN) {
            ssid_len = WIFI_SSID_LEN;
        }

        memset(array->ssid, 0, ssid_len);
        os_strncpy((char *)array->ssid, apList.aps[i].ssid, ssid_len);
        array->s_len = ssid_len;

        j++;
    }

    if (j == 0) {
        goto SCAN_ERR;
    }
    *ap_ary = array;
    if (apList.aps != NULL) {
        tkl_system_free(apList.aps);
    }

    return OPRT_OK;

SCAN_ERR:
    if (apList.aps != NULL) {
        tkl_system_free(apList.aps);
        apList.aps = NULL;
    }

    if (array) {
        tkl_system_free(array);
        array = NULL;
    }
    return OPRT_OS_ADAPTER_COM_ERROR;
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
    if(NULL == ap) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    tkl_system_free(ap);
    ap = NULL;

    return OPRT_OK;
}

/**
 * @brief receive wifi management 接收到数据原厂的回调处理函数,将数据放入到queue中
 * @param[in]       *frame
 * @param[in]       len
 * @param[in]       *param
 * @return  none
 */
static void wifi_mgnt_frame_rx_notify(unsigned char *frame, int len, void *param)
{
    if(mgnt_recv_cb) {
        mgnt_recv_cb((unsigned char *)frame, len);
    }
}

/**
 * @brief start a soft ap
 *
 * @param[in]       cfg         the soft ap config
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_start_ap(const WF_AP_CFG_IF_S *cfg)
{
    int ret = OPRT_OK;
    wifi_ap_config_t wApConfig = WIFI_DEFAULT_AP_CONFIG();
    netif_ip4_config_t wIp4_config = {0};

    tkl_system_memset(&wApConfig, 0x0, sizeof(wifi_ap_config_t));

    switch ( cfg->md ) {
        case WAAM_OPEN :
            wApConfig.security = WIFI_SECURITY_NONE;
            break;
        case WAAM_WEP :
            wApConfig.security = WIFI_SECURITY_WEP;
            break;
        case WAAM_WPA_PSK :
            wApConfig.security = WIFI_SECURITY_WPA2_TKIP;
            break;
        case WAAM_WPA2_PSK :
            wApConfig.security = WIFI_SECURITY_WPA2_AES;
            break;
        case WAAM_WPA_WPA2_PSK:
            wApConfig.security = WIFI_SECURITY_WPA2_MIXED;
            break;
        case WAAM_WPA_WPA3_SAE:
            wApConfig.security = WIFI_SECURITY_WPA3_SAE;
        default:
            ret = OPRT_INVALID_PARM;
            break;
    }

    //bk_printf("ap net info ip: %s, mask: %s, gw: %s\r\n", cfg->ip.ip, cfg->ip.mask, cfg->ip.gw);
    if (OPRT_OK == ret) {
        wApConfig.channel = cfg->chan;
        tkl_system_memcpy((char *)wApConfig.ssid, cfg->ssid, cfg->s_len);
        tkl_system_memcpy((char *)wApConfig.password, cfg->passwd, cfg->p_len);

        os_strcpy((char *)wIp4_config.ip, cfg->ip.ip);
        os_strcpy((char *)wIp4_config.mask, cfg->ip.mask);
        os_strcpy((char *)wIp4_config.gateway, cfg->ip.gw);
        os_strcpy((char *)wIp4_config.dns, cfg->ip.gw);

        bk_printf("ssid:%s, key:%s, channel: %d\r\n", wApConfig.ssid, wApConfig.password, wApConfig.channel);
        BK_LOG_ON_ERR(bk_netif_set_ip4_config(NETIF_IF_AP, &wIp4_config));
        wApConfig.max_con = 1; // max connect 1
        BK_LOG_ON_ERR(bk_wifi_ap_set_config(&wApConfig));
        BK_LOG_ON_ERR(bk_wifi_ap_start());
    }

    extern void ap_set_default_netif(void);
    ap_set_default_netif();

    return ret;
}

/**
 * @brief stop a soft ap
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_stop_ap(void)
{
    BK_LOG_ON_ERR(bk_wifi_ap_stop());
    return OPRT_OK;
}

/**
 * @brief set wifi interface work channel
 *
 * @param[in]       chan        the channel to set
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_cur_channel(const UCHAR_T chan)
{
    int status;
    wifi_channel_t channel = {0};

    if ((chan > 14) || (chan < 1)) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    channel.primary = chan;
    if (mgnt_recv_cb != NULL) { /* 为了避免路由器断开之后，管理祯接收NG的问题 */
        bk_printf("mgnt recv cb<<\r\n");
        bk_wifi_filter_register_cb((wifi_filter_cb_t)wifi_mgnt_frame_rx_notify);/* 将回调注册到原厂中 */
    }


    status = bk_wifi_monitor_set_channel(&channel);
    if (status) {
        return OPRT_OS_ADAPTER_CHAN_SET_FAILED;
    } else {
        return OPRT_OK;
    }

}

/**
 * @brief get wifi interface work channel
 *
 * @param[out]      chan        the channel wifi works
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_cur_channel(UCHAR_T *chan)
{
    *chan = (unsigned char)bk_wifi_get_channel();
    return OPRT_OK;
}

#if DEBUG_EZ_MODE
static int s_receive[2] = {0};
static void check_frame_type(unsigned char *data, unsigned short len)
{
    unsigned char frame_head = data[0];
    unsigned char frame_type = frame_head&0x0c;

    //tuya_cli_printf("frame_head = 0x%x frame_type=%d\r\n",frame_head,frame_type);
    switch(frame_type){
        case 0://管理帧
            {
                if (0 == (s_receive[0] & 0xff)) {
                    bk_printf("recv mgmt count (%d).\r\n",s_receive[0] + 1);
                    os_mem_dump("mgmt frame", data, len);
                }

                s_receive[0] ++;
            }
            break;
        case 8://数据帧
            {
                if (0 == (s_receive[1] & 0xff)) {
                    bk_printf("recv data count (%d).\r\n",s_receive[1] + 1);
                    os_mem_dump("data frame", data, len);
                }
                s_receive[1] ++;
            }
            break;
    }
}
#endif

static int _wf_sniffer_set_cb(const uint8_t *data, uint32_t len, const wifi_frame_info_t *info)
{
#if DEBUG_EZ_MODE
    check_frame_type(data, len);
#endif

    if (NULL != snif_cb) {
        (*snif_cb)(data, len, info->rssi);
    }

    return 0;
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
    bk_printf("tkl_wifi_set_sniffer %d, cb(%p).\r\n",en, cb);

    if(en) {
        snif_cb = cb;
        bk_wifi_monitor_register_cb(_wf_sniffer_set_cb);
        bk_wifi_sta_pm_disable();
        bk_wifi_scan_stop();
        bk_wifi_monitor_start();
    }else {
        bk_wifi_monitor_stop();
        bk_wifi_sta_pm_enable();
        bk_wifi_monitor_register_cb(NULL);
    }
    return OPRT_OK;
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
    netif_ip4_config_t wIp4Config;
    netif_if_t iface;

    tkl_system_memset(&wIp4Config, 0x0, sizeof(netif_ip4_config_t));

    switch (wf) {
        case WF_STATION:
            iface = NETIF_IF_STA;
            break;

        case WF_AP:
            iface = NETIF_IF_AP;
            break;

        default:
            ret = OPRT_OS_ADAPTER_INVALID_PARM;
            break;
    }

    if (OPRT_OK == ret) {
        ret = bk_netif_get_ip4_config(iface, &wIp4Config);
        os_strcpy(ip->ip, wIp4Config.ip);
        os_strcpy(ip->mask, wIp4Config.mask);
        os_strcpy(ip->gw, wIp4Config.gateway);
    }

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
    if (bk_wifi_set_mac_address((char *)mac) == BK_OK) {
        return OPRT_OK;
    } else {
        return OPRT_OS_ADAPTER_MAC_SET_FAILED;
    }
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
    if(WF_STATION == wf) {
        bk_wifi_sta_get_mac((uint8_t *)mac);
    }else{
        bk_wifi_ap_get_mac((uint8_t *)mac);
    }

    return OPRT_OK;
}

static OPERATE_RET _wf_wk_mode_exit(WF_WK_MD_E last_mode, WF_WK_MD_E curr_mode)
{
    int ret = OPRT_OK;

    switch(last_mode) {
        case WWM_POWERDOWN :
            if(curr_mode == WWM_POWERDOWN) {
                if(tkl_get_lp_flag()) {
                    _bk_rtc_wakeup_register(1000);  //由于默认设置cpu是处于sleep模式， 当资源释放后进入idle线程，cpu就会进入睡眠。需要先设置rtc唤醒，否则再不能唤醒cpu，导致程序异常
                    if(!ble_init_flag) {  //当 tuyaos 没有初始化使用蓝牙时候，进低功耗前需要先把 ble ctrl 资源释放掉
                        bk_bluetooth_deinit();
                    }
                    //针对保活低功耗， 进低功耗前需要释放wifi资源。所以在连接路由器失败后释放 wifi 资源
                    bk_wifi_sta_stop();
                }
            }
            break;

        case WWM_SNIFFER:
            bk_wifi_monitor_stop();
            break;

        case WWM_STATION:
            bk_wifi_sta_stop();
            break;

        case WWM_SOFTAP:
            bk_wifi_ap_stop();
            break;

        case WWM_STATIONAP:
            bk_wifi_ap_stop();
            bk_wifi_sta_stop();
            break;

        default:
            break;
    }

    return ret;
}

/**
 * @brief set wifi work mode
 *
 * @param[in]       mode        wifi work mode
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_work_mode(const WF_WK_MD_E mode)
{
    OPERATE_RET ret = OPRT_OK;
    WF_WK_MD_E current_mode;

    if(first_set_flag) {
        //extern void extended_app_waiting_for_launch(void);
        //extended_app_waiting_for_launch(); /* 首次等待wifi初始化OK */
        first_set_flag = FALSE;
    }

    ret = tkl_wifi_get_work_mode(&current_mode);
    if((OPRT_OK == ret) && (current_mode != WWM_POWERDOWN ? current_mode != mode : 1)) {
        _wf_wk_mode_exit(current_mode, mode);
        if(mode == WWM_POWERDOWN) {   //这里没有退出 powerdown 模式概念， 只有进入 powerdown 概念
            _wf_wk_mode_exit(mode, mode);
        }
    }

    wf_mode = mode;

    return ret;
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
	int ret;
	struct wlan_fast_connect_info *fci;
	FAST_WF_CONNECTED_AP_INFO_T *ap_info = NULL;
	wifi_sta_config_t sta_cfg;

	if (!fast_ap_info)
		return OPRT_COM_ERROR;

	// 外部调用完成后释放
	ap_info = (FAST_WF_CONNECTED_AP_INFO_T *)tkl_system_malloc(sizeof(FAST_WF_CONNECTED_AP_INFO_T) + sizeof(struct wlan_fast_connect_info));
	if (ap_info == NULL) {
		return OPRT_COM_ERROR;
	}

    os_memset(ap_info, 0, sizeof(FAST_WF_CONNECTED_AP_INFO_T) + sizeof(struct wlan_fast_connect_info));

	os_memset(&sta_cfg, 0, sizeof(sta_cfg));
	ret = bk_wifi_sta_get_config(&sta_cfg);
	if (ret) {
		return OPRT_COM_ERROR;
	}

	ap_info->len = sizeof(struct wlan_fast_connect_info);
	fci = (struct wlan_fast_connect_info *)ap_info->data;

	os_memcpy(fci->ssid, sta_cfg.ssid, WIFI_SSID_STR_LEN);
	os_memcpy(fci->bssid, sta_cfg.bssid, WIFI_BSSID_LEN);
	fci->security = sta_cfg.security;
	fci->channel = sta_cfg.channel;
	os_memcpy(fci->psk, sta_cfg.psk, WIFI_PASSWORD_LEN);
	os_memcpy(fci->pwd, sta_cfg.password, WIFI_PASSWORD_LEN);
	os_memcpy(fci->ip_addr, sta_cfg.ip_addr, 4);
	os_memcpy(fci->netmask, sta_cfg.netmask, 4);
	os_memcpy(fci->gw, sta_cfg.gw, 4);
	os_memcpy(fci->dns1, sta_cfg.dns1, 4);
	fci->pmf = sta_cfg.pmf;
	os_memcpy(fci->tk, sta_cfg.tk, 16);

	__fast_connect_ap_info_dump("get connected ap info", fci);

	*fast_ap_info = ap_info;

	return OPRT_OK;

}

/**
 * @brief get wifi bssid
 *
 * @param[out]      mac         uplink mac
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_bssid(UCHAR_T *mac)
{
    int ret = OPRT_OK;
    wifi_link_status_t link_status;

    if(NULL == mac) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    ret = bk_wifi_sta_get_link_status(&link_status);
    if (OPRT_OK == ret) {
        tkl_system_memcpy(mac, link_status.bssid, 6);
    }

    return ret;
}

/**
 * @brief set wifi country code
 *
 * @param[in]       ccode  country code
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_country_code(const COUNTRY_CODE_E ccode)
{
    int ret = OPRT_OK;

    int country_num = sizeof(country_code) / sizeof(wifi_country_t);
    wifi_country_t *country = (ccode < country_num) ? &country_code[ccode] : &country_code[COUNTRY_CODE_CN];

    ret = bk_wifi_set_country(country);
    if (ret != OPRT_OK) {
        bk_printf("set country err!\r\n");
    }

    return OPRT_OK;
}

/**
 * @brief set wifi country code
 *
 * @param[in]       ccode  country code
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_country_code_v2(const UCHAR_T *ccode)
{
    bk_printf("%s: set country code [%s]\r\n", __func__, ccode);
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
    //bk_printf("ccode:%s, country_code:%d\r\n", ccode, country_code);
    tkl_wifi_set_country_code(country_code);

    return OPRT_OK;
}

/**
 * @brief wifi_get_country_code    use wifi scan to get country code
 *
 * @param[in]       ccode   country code buffer to restore
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_country_code(UCHAR_T *ccode)
{
	uint8_t country_code[3] = {0};

	int ret = bk_scan_country_code(country_code);
    if(ret != 0) {
        strncpy((char *)ccode, (char *)country_code, 2);
        return OPRT_OK;
    }

    memcpy(ccode, "UNKNOW", sizeof("UNKNOW"));
    return OPRT_NOT_FOUND;
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
    int stat = bk_wifi_manual_cal_rfcali_status();

    if(stat == BK_OK) {
        return true;
    }

    return false;
}

/**
 * @brief 进入低功耗处理
 *
 * @param[in] none
 * @return  none
 */
static void tkl_wifi_powersave_enable(void)
{
    bk_wifi_sta_pm_enable();
}

/**
 * @brief 退出低功耗处理
 *
 * @param[in] none
 * @return  none
 */
static void tkl_wifi_powersave_disable(void)
{
    bk_wifi_sta_pm_disable();
}


/**
 * @brief set wifi lowpower mode
 *
 * @param[in]       enable      enbale lowpower mode
 * @param[in]       dtim     the wifi dtim
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_lp_mode(const BOOL_T enable, const UCHAR_T dtim)
{
    bk_printf("-- wifi lp set enable: %d, dtim:%d\r\n", enable, dtim);
    static int lp_enable = FALSE;

    if(dtim == 10 || dtim == 20 || dtim == 30) {
        // wifi_lp_flag = TRUE;
    }

    if(tkl_get_lp_flag()) {
        WF_WK_MD_E work_mode;
        tkl_wifi_get_work_mode(&work_mode);
        if(work_mode == WWM_POWERDOWN) {
            // powerdown mode, don't set lp mode
            return OPRT_OK;
        }

        if(TRUE == enable) {
            bk_wifi_send_listen_interval_req(dtim);
            if(!lp_enable && dtim != 0) {
                lp_enable = TRUE;
                tkl_wifi_powersave_enable();
            }
        }else {
            if(lp_enable && dtim == 0) {
                lp_enable = FALSE;
                //tkl_wifi_powersave_disable();
            }
        }
    } else {
        bk_wifi_send_listen_interval_req(dtim);
        if(TRUE == enable) {
            if(!lp_enable) {
                lp_enable = TRUE;
                tkl_wifi_powersave_enable();
            }
        }else {
            if(lp_enable) {
                lp_enable = FALSE;
                tkl_wifi_powersave_disable();
            }
        }
    }
    return OPRT_OK;
}

static int fast_connect_flag = FALSE;

int _wifi_event_cb(void *arg, event_module_t event_module,
					  int event_id, void *event_data)
{
	wifi_event_sta_disconnected_t *sta_disconnected;

    bk_printf("_wifi_event_cb %d\r\n", event_id);
	switch (event_id) {
	case EVENT_WIFI_STA_CONNECTED:
		bk_printf("EVENT_WIFI_STA_CONNECTED %d\r\n", event_id);
        bk_wifi_send_arp_set_rate_req(60); //set arp keepalive to 6Mbps
		break;
	case EVENT_WIFI_STA_DISCONNECTED:
		sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
        etharp_remove_all_static();
        if(sta_disconnected->disconnect_reason == 0)
            break;

        if(tkl_get_lp_flag()) {
            _bk_rtc_wakeup_register(1000);  //由于默认设置cpu是处于sleep模式， 当资源释放后进入idle线程，cpu就会进入睡眠。需要先设置rtc唤醒，否则再不能唤醒cpu，导致程序异常
            if(!ble_init_flag) {  //当 tuyaos 没有初始化使用蓝牙时候，进低功耗前需要先把 ble ctrl 资源释放掉
                bk_bluetooth_deinit();
            }
            bk_wifi_sta_stop(); //针对保活低功耗， 进低功耗前需要释放wifi资源。所以在连接路由器失败后释放 wifi 资源
        }
        switch(sta_disconnected->disconnect_reason) {
            case WIFI_REASON_DEAUTH_LEAVING:
            case WIFI_REASON_NO_AP_FOUND:
            case WIFI_REASON_DISASSOC_AP_BUSY:
            case WIFI_REASON_PREV_AUTH_NOT_VALID:
            case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
            case WIFI_REASON_MICHAEL_MIC_FAILURE:
            case WIFI_REASON_WRONG_PASSWORD:
            case WIFI_REASON_DHCP_TIMEOUT:
                bk_printf("RW_EVT_STA_CONNECT_FAILED %d\r\n", sta_disconnected->disconnect_reason);
                __notify_wifi_event(WFE_CONNECT_FAILED, NULL);
            break;
            default:
                bk_printf("WFE_DISCONNECTED %d\r\n", sta_disconnected->disconnect_reason);
                __notify_wifi_event(WFE_DISCONNECTED, NULL);
            break;
        }

		break;

	case EVENT_WIFI_AP_CONNECTED:
		//ap_connected = (wifi_event_ap_connected_t *)event_data;
		break;

	case EVENT_WIFI_AP_DISCONNECTED:
		//ap_disconnected = (wifi_event_ap_disconnected_t *)event_data;
		break;

	default:
		bk_printf("rx event <%d %d>\r\n", event_module, event_id);
		break;
	}

	return BK_OK;
}

int _netif_event_cb(void *arg, event_module_t event_module,
					   int event_id, void *event_data)
{
    bk_printf("_netif_event_cb %d\r\n", event_id);
	switch (event_id) {
	case EVENT_NETIF_GOT_IP4:
        bk_printf("WFE_CONNECTED %d\r\n", event_id);
        __notify_wifi_event(WFE_CONNECTED, NULL);
        if(tkl_get_lp_flag()) {
            if(fast_connect_flag) {
                bk_bluetooth_deinit();
            }
        }
		break;
    case EVENT_NETIF_DHCP_TIMEOUT:
        bk_printf("WFE_CONNECT_FAILED %d\r\n", event_id);
        etharp_remove_all_static();
        __notify_wifi_event(WFE_CONNECT_FAILED, NULL);
        break;
	default:
		bk_printf("rx event <%d %d>\n", event_module, event_id);
		break;
	}

	return BK_OK;
}

/**
 * @brief : fast connect
 * @param[in]      fast_ap_info
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_station_fast_connect(const FAST_WF_CONNECTED_AP_INFO_T *fast_ap_info)
{
    bk_printf("%s\r\n", __func__);
    int ret = OPRT_COM_ERROR;
	struct wlan_fast_connect_info *fci;
	wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();

	if (!fast_ap_info)
		return ret;

	if (fast_ap_info->len != sizeof(struct wlan_fast_connect_info))
		return ret;

    if(wifi_event_init) {
        wifi_event_init = 0;
        bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, _wifi_event_cb, NULL);
        bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, _netif_event_cb, NULL);
    }

	fci = (struct wlan_fast_connect_info *)fast_ap_info->data;
	//__fast_connect_ap_info_dump("last connected ap info", fci);

	os_memcpy(sta_config.ssid, fci->ssid, sizeof(sta_config.ssid));
	os_memcpy(sta_config.password, fci->pwd, sizeof(sta_config.password));
	os_memcpy(sta_config.bssid, fci->bssid, sizeof(sta_config.bssid));
	sta_config.security = fci->security;
	sta_config.channel = fci->channel;
	os_memcpy(sta_config.psk, fci->psk, sizeof(sta_config.psk));
	os_memcpy(sta_config.ip_addr, fci->ip_addr, sizeof(sta_config.ip_addr));
	os_memcpy(sta_config.netmask, fci->netmask, sizeof(sta_config.netmask));
	os_memcpy(sta_config.gw, fci->gw, sizeof(sta_config.gw));
	os_memcpy(sta_config.dns1, fci->dns1, sizeof(sta_config.dns1));
	sta_config.pmf = fci->pmf;
	os_memcpy(sta_config.tk, fci->tk, sizeof(sta_config.tk));
	sta_config.is_not_support_auto_fci = 1;
	sta_config.is_user_fast_connect = 1;

    sta_config.auto_reconnect_count = 1;
    sta_config.disable_auto_reconnect_after_disconnect = true;

    BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
	ret = bk_wifi_sta_start();

    if(tkl_get_lp_flag()) {
        if(!lp_wifi_cfg_flag) {
            lp_wifi_cfg_flag = 1;
            bk_printf("Wi-Fi low power configuration settings\r\n");
            bk_wifi_capa_config(WIFI_CAPA_ID_TX_AMPDU_EN, 0);
            bk_wifi_send_bcn_loss_int_req(BEACON_LOSS_INTERVAL, BEACON_LOSS_REPEAT_NUM);
            bk_wifi_set_bcn_recv_win(BEACON_REV_DEF_WIN, BEACON_REV_CURR_MAX_WIN, BEACON_REV_STEP_WIN);
            bk_wifi_set_bcn_loss_time(BEACON_LOSS_WAIT_CNT, BEACON_LOSS_WAKE_CNT);
        }
        _bk_rtc_wakeup_unregister();  //开始配网时，关闭rtc唤醒
    }

    fast_connect_flag = TRUE;

	return OPRT_OK;

}


/**
 * @brief connect wifi with ssid and passwd
 *
 * @param[in]       ssid
 * @param[in]       passwd
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_station_connect(const SCHAR_T *ssid, const SCHAR_T *passwd)
{
    bk_printf("%s\r\n", __func__);
    int ret = OPRT_COM_ERROR;
    wifi_sta_config_t sta_config = WIFI_DEFAULT_STA_CONFIG();

#if CONFIG_SAVE_BOOT_TIME_POINT
    save_mtime_point(CPU_START_CONNECT_TIME);
#endif

    if(wifi_event_init) {
        wifi_event_init = 0;
        bk_event_register_cb(EVENT_MOD_WIFI, EVENT_ID_ALL, _wifi_event_cb, NULL);
        bk_event_register_cb(EVENT_MOD_NETIF, EVENT_ID_ALL, _netif_event_cb, NULL);
    }

    os_strcpy((char *)sta_config.ssid, (const char *)ssid);
    os_strcpy((char *)sta_config.password, (const char *)passwd);
    sta_config.is_user_fast_connect = 0;
    sta_config.is_not_support_auto_fci = 1;
    sta_config.auto_reconnect_count = 1;
    sta_config.disable_auto_reconnect_after_disconnect = true;

    BK_LOG_ON_ERR(bk_wifi_sta_set_config(&sta_config));
    ret = bk_wifi_sta_start();

    if(tkl_get_lp_flag()) {
        if(!lp_wifi_cfg_flag) {
            lp_wifi_cfg_flag = 1;
            bk_printf("Wi-Fi low power configuration settings !!!!\r\n");
            bk_wifi_capa_config(WIFI_CAPA_ID_TX_AMPDU_EN, 0);
            bk_wifi_send_bcn_loss_int_req(BEACON_LOSS_INTERVAL, BEACON_LOSS_REPEAT_NUM);
            bk_wifi_set_bcn_recv_win(BEACON_REV_DEF_WIN, BEACON_REV_CURR_MAX_WIN, BEACON_REV_STEP_WIN);
            bk_wifi_set_bcn_loss_time(BEACON_LOSS_WAIT_CNT, BEACON_LOSS_WAKE_CNT);
        }
        _bk_rtc_wakeup_unregister();  //开始配网时，关闭rtc唤醒
    }

    return ret;
}

/**
 * @brief disconnect wifi from connect ap
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_station_disconnect(void)
{
    bk_printf("station disconnect ap\r\n");
    etharp_remove_all_static();
    bk_wifi_sta_stop(); /* 不需要实现，由于在start中一开始就会停止 */
    return OPRT_OK;
}

/**
 * @brief get wifi connect rssi
 *
 * @param[out]      rssi        the return rssi
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_station_get_conn_ap_rssi(SCHAR_T *rssi)
{
    int ret = OPRT_OK;
    SHORT_T tmp_rssi = 0, sum_rssi = 0;
    SCHAR_T max_rssi = -128, min_rssi = 127;
    wifi_link_status_t link_status = {0};
    int i = 0, error_cnt = 0;

    for (i = 0; i < 5; i++) {
        memset(&link_status, 0x00, sizeof(link_status));
        ret = bk_wifi_sta_get_link_status(&link_status);
        if (OPRT_OK == ret) {
            tmp_rssi = link_status.rssi;
            if (tmp_rssi > max_rssi) {
                max_rssi = tmp_rssi;
            }
            if (tmp_rssi < min_rssi) {
                min_rssi = tmp_rssi;
            }
            //bk_printf("get rssi: %d\r\n", tmp_rssi);
        } else {
            //bk_printf("get rssi error\r\n");
            error_cnt++;
        }
        sum_rssi += tmp_rssi;
        tkl_system_sleep(10);
    }
    if (error_cnt > 2) {
        ret = OPRT_COM_ERROR;
    } else {
        *rssi = (sum_rssi - min_rssi - max_rssi)/(3 - error_cnt);
    }

    return ret;
}

/**
 * @brief get wifi station work status
 *
 * @param[out]      stat        the wifi station work status
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_station_get_status(WF_STATION_STAT_E *stat)
{
    wifi_linkstate_reason_t info = {0};
    WF_WK_MD_E mode;

    tkl_wifi_get_work_mode(&mode);
    if (mode == WWM_SOFTAP) {
        *stat = WSS_IDLE;
        return OPRT_OK;
    }

    bk_wifi_sta_get_linkstate_with_reason(&info);
    switch (info.state) {
        case WIFI_LINKSTATE_STA_IDLE:
            *stat = WSS_IDLE;
            break;
        case WIFI_LINKSTATE_STA_CONNECTING:
            *stat = WSS_CONNECTING;
            break;
        case WIFI_LINKSTATE_STA_DISCONNECTED:
            switch (info.reason_code) {
                case WIFI_REASON_NO_AP_FOUND:
                    *stat = WSS_NO_AP_FOUND;
                    break;
                case WIFI_REASON_WRONG_PASSWORD:
                    *stat = WSS_PASSWD_WRONG;
                    break;
                case WIFI_REASON_DHCP_TIMEOUT:
                    *stat = WSS_DHCP_FAIL;
                    break;

                case WIFI_REASON_PREV_AUTH_NOT_VALID:
                case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
                case WIFI_REASON_MICHAEL_MIC_FAILURE:
                default:
                    *stat = WSS_CONN_FAIL;
                    break;
            }
            break;
        case WIFI_LINKSTATE_STA_CONNECTED:
            *stat = WSS_CONN_SUCCESS;
            break;
        case WIFI_LINKSTATE_STA_GOT_IP:
            *stat = WSS_GOT_IP;
            break;
        default:
            *stat = WSS_CONN_FAIL;
            break;
    }

    return OPRT_OK;
}

/**
 * @brief send wifi management
 *
 * @param[in]       buf         pointer to buffer
 * @param[in]       len         length of buffer
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_send_mgnt(const UCHAR_T *buf, const uint32_t len)
{
    int ret = bk_wifi_send_raw((uint8_t *)buf, len);
    if(ret < 0) {
        return OPRT_OS_ADAPTER_MGNT_SEND_FAILED;
    }

    return OPRT_OK;
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
    bk_printf("recv mgnt callback enable %d\r\n",enable);
    if (enable) {
        WF_WK_MD_E mode;
        int ret = tkl_wifi_get_work_mode(&mode);
        if (OPRT_OK != ret) {
            bk_printf("recv mgnt, get mode err\r\n");
            return OPRT_OS_ADAPTER_COM_ERROR;
        }

        if ((mode == WWM_POWERDOWN) || (mode == WWM_SNIFFER)) {
            bk_printf("recv mgnt, but mode is err\r\n");
            return OPRT_OS_ADAPTER_COM_ERROR;
        }
        tkl_wifi_powersave_disable();        /* 强制退出低功耗的模式 */
        mgnt_recv_cb = recv_cb;
        bk_wifi_filter_register_cb((wifi_filter_cb_t)wifi_mgnt_frame_rx_notify); /* 将回调注册到原厂中 */

        mgnt_cb_exist_flag = 1;

    }else {
        bk_wifi_filter_register_cb(NULL);
        mgnt_recv_cb = NULL;
    }
    return OPRT_OK;
}


/**
 * @brief wifi ioctl
 *
 * @param[in]       cmd     refer to WF_IOCTL_CMD_E
 * @param[in]       args    args associated with the command
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_ioctl(WF_IOCTL_CMD_E cmd,  void *args)
{
    return OPRT_NOT_SUPPORTED;
}
