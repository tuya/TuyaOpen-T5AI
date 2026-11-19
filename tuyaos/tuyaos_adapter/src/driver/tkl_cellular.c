/**
* @file tkl_cellular.h
* @brief Common process - adapter the cellular api
* @version 0.1
* @date 2025-04-21
*
* @copyright Copyright 2020-2025 Tuya Inc. All Rights Reserved.
*
*/
#include <string.h>
#include "tuya_cloud_types.h"
#include "tuya_error_code.h"
#include "ethernetif.h"
#include "lwip/netifapi.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "tkl_cellular.h"
#include "tkl_thread.h"
#include "tkl_mutex.h"
#include "tkl_system.h"
#include "spi_eth_drv.h"
#if CONFIG_LWIP_PPP_SUPPORT
#include "net.h"
#endif
#if CONFIG_BK_MODEM
#include <common/bk_assert.h>
#include "bk_modem_dce.h"
#include "bk_modem_dte.h"
#include "bk_modem_main.h"
#include "bk_modem_netif.h"
#include "bk_modem_main.h"
#endif

#ifndef IPADDR2STR
#define IPADDR2STR(ip) (unsigned char)(ip & 0xFF), (unsigned char)((ip >> 8) & 0xFF), \
                        (unsigned char)((ip >> 16) & 0xFF), (unsigned char)((ip >> 24) & 0xFF)
#endif /* IPADDR2STR */

#if CONFIG_BK_MODEM
extern struct bk_modem_dce_pdp_ctx_s dce_pdp_ctx;
extern uint8_t bk_modem_status;
extern uint8_t bk_modem_mf_test;
extern uint8_t bk_modem_get_mode(void);
extern bk_err_t bk_modem_deinit(void);
xSemaphoreHandle bk_modem_mf_test_sem = NULL;
extern bk_err_t bk_modem_init(uint8_t comm_proto, uint8_t comm_if);
#endif /* CONFIG_BK_MODEM */

static TKL_CELLULAR_SLEEP_MODE_E s_cellular_sleep_mode = TKL_CELLULAR_NO_SLEEP;

/**
 * @brief  init create cellular link
 *
 * @param[in]   cfg: the configure for cellular link
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_init(TKL_CELLULAR_BASE_CFG_T *cfg)
{
#if CONFIG_BK_MODEM
    bk_err_t ret;
    struct bk_modem_dce_pdp_ctx_s ctx;

    if (cfg->protocol != TUYA_CELLULAR_PROTOCOL_PPP) {
        bk_printf("%s: protocol not support %d\r\n", __func__, cfg->protocol);
        return OPRT_NOT_SUPPORTED;
    }

    memset(&ctx, 0, sizeof(ctx));
    strncpy(ctx.apn, cfg->apn, TKL_CELLULAR_APN_LEN);
    bk_modem_dec_pdp_ctx_init(ctx.apn);

    /* Must set before bk_modem_init(): dial thread may run LP AT soon after. */
    if (cfg->sleep_mode > TKL_CELLULAR_HIBERNATE) {
        s_cellular_sleep_mode = TKL_CELLULAR_NO_SLEEP;
    } else {
        s_cellular_sleep_mode = cfg->sleep_mode;
    }
    bk_printf("%s: sleep_mode %d\r\n", __func__, s_cellular_sleep_mode);

    if (cfg->iface == TUYA_CELLULAR_IF_UART) {
        bk_printf("%s: Start UART Cellular Network\r\n", __func__);
        ret = bk_modem_init(PPP_MODE, UART_IF);
    } else if (cfg->iface == TUYA_CELLULAR_IF_USB) {
        bk_printf("%s: Start USB Cellular Network\r\n", __func__);
        ret = bk_modem_init(PPP_MODE, USB_IF);
    } else {
        return OPRT_NOT_SUPPORTED;
    }

    if (BK_OK != ret) {
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
#else
    return OPRT_NOT_SUPPORTED;
#endif
}

/**
 * @brief Check whether Sleep1 AT (AT+ECPMUCFG=1,2) should be sent
 * @return 0 for NO_SLEEP/IDLE (skip); non-zero for MODE1/MODE2/HIBERNATE (send)
 */
int tkl_cellular_get_sleep_mode(void)
{
    if ((s_cellular_sleep_mode == TKL_CELLULAR_MODE1) ||
        (s_cellular_sleep_mode == TKL_CELLULAR_MODE2) ||
        (s_cellular_sleep_mode == TKL_CELLULAR_HIBERNATE)) {
        return 1;
    }
    return 0;
}

/**
 * @brief  get the link status of cellular link
 *
 * @param[out]  is_up: the cellular link status is up or not
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_get_status(TKL_CELLULAR_STAT_E *status)
{
#if CONFIG_BK_MODEM
#if CONFIG_LWIP_PPP_SUPPORT
    struct netif *netif;
    uint32_t ip;
    uint32_t mask;
    uint32_t gw;

    netif = (struct netif *)net_get_ppp_netif_handle();
    if (NULL == netif) {
        return OPRT_COM_ERROR;
    }

    ip = ip4_addr_get_u32(ip_2_ip4(&netif->ip_addr));
    mask = ip4_addr_get_u32(ip_2_ip4(&netif->netmask));
    gw = ip4_addr_get_u32(ip_2_ip4(&netif->gw));

    if (netif_is_up(netif) && ip && mask && gw) {
        *status = TKL_CELLULAR_LINK_UP;
    } else {
        *status = TKL_CELLULAR_LINK_DOWN;
    }

    //bk_printf("%s: wired link status %d\r\n", __func__, *status);
    return OPRT_OK;
#endif /* CONFIG_LWIP_PPP_SUPPORT */
#else  
    return OPRT_NOT_SUPPORTED;
#endif /* CONFIG_BK_MODEM */
}

/**
 * @brief  set the status change callback
 *
 * @param[in]   cb: the callback when link status changed
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_set_status_cb(TKL_CELLULAR_STATUS_CHANGE_CB cb)
{
#if CONFIG_BK_MODEM
#if CONFIG_LWIP_PPP_SUPPORT
    //bk_printf("%s: Set status cb %p\r\n", __func__, cb);
    bk_modem_ppp_netif_state_cb_register((BK_MODEM_NETIF_STATE_NOTIFY)cb);
    return OPRT_OK;
#endif
#else
    return OPRT_NOT_SUPPORTED;
#endif /* CONFIG_BK_MODEM */    
}

/**
 * @brief  get the ip address of the cellular link
 * 
 * @param[in]   ip: the ip address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_get_ip(NW_IP_S *ip)
{
#if CONFIG_BK_MODEM    
#if CONFIG_LWIP_PPP_SUPPORT
    unsigned int ip_addr;
    struct netif *netif;

    netif = (struct netif *)net_get_ppp_netif_handle();
    if (NULL == netif) {
        return OPRT_COM_ERROR;
    }

    if (0 != ip4_addr_get_u32(ip_2_ip4(&netif->ip_addr))) {
        ip_addr = ip4_addr_get_u32(ip_2_ip4(&netif->ip_addr));
        sprintf(ip->nwipstr, "%d.%d.%d.%d", IPADDR2STR(ip_addr));

        ip_addr = ip4_addr_get_u32(ip_2_ip4(&netif->gw));
        sprintf(ip->nwgwstr, "%d.%d.%d.%d", IPADDR2STR(ip_addr));

        ip_addr = ip4_addr_get_u32(ip_2_ip4(&netif->netmask));
        sprintf(ip->nwmaskstr, "%d.%d.%d.%d", IPADDR2STR(ip_addr));

        return OPRT_OK;
    }
    //bk_printf("%s: get wired ip %s mask %s gw %s\r\n",
    //    __func__, ip->ip, ip->mask, ip->gw);
    return OPRT_COM_ERROR;
#endif
#else
    return OPRT_NOT_SUPPORTED;
#endif    
}

/**
 * @brief  get the ip address of the cellular link
 * 
 * @param[in]   ip: the ip address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_get_ipv6(NW_IP_TYPE type, NW_IP_S *ip)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief  get the ccid of the cellular link
 * 
 * @param[out]   ccid: ccid string
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_get_ccid(char *ccid)
{
#if CONFIG_BK_MODEM
    BK_ASSERT(NULL != ccid);

    if (bk_modem_mf_test && !bk_modem_dce_get_ccid()) {
        return OPRT_COM_ERROR;
    }

    if (dce_pdp_ctx.cid[0] == '\0') {
        return OPRT_COM_ERROR;
    }

    strncpy((char *)ccid, (char *)dce_pdp_ctx.cid, TKL_CELLULAR_CCID_LEN);
#endif
    return OPRT_OK;
}

/**
 * @brief  get the rssi of the cellular link
 * 
 * @param[out]   rssi: rssi value
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_get_rssi(char *rssi)
{
#if CONFIG_BK_MODEM
    BK_ASSERT(NULL != rssi);

    if (bk_modem_mf_test && !bk_modem_dce_check_signal()) {
        return OPRT_COM_ERROR;
    }

    *rssi = dce_pdp_ctx.rssi;
#endif
    return OPRT_OK;
}

/**
 * @brief  get the voltage of the cellular module
 * 
 * @param[out]   volt: voltage value
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_get_volt(uint32_t *volt)
{
#if CONFIG_BK_MODEM
    BK_ASSERT(NULL != volt);

    if (bk_modem_mf_test && !bk_modem_dce_get_cbc()) {
        return OPRT_COM_ERROR;
    }

    *volt = dce_pdp_ctx.volt;
#endif

    return OPRT_OK;
}

/**
 * @brief  get the IMEI of the cellular module
 * 
 * @param[out]   imei: IMEI value
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_get_imei(char *imei)
{
#if CONFIG_BK_MODEM
    BK_ASSERT(NULL != imei);

    if (bk_modem_mf_test && !bk_modem_dce_get_cgsn()) {
        return OPRT_COM_ERROR;
    }

    if (dce_pdp_ctx.imei[0] == '\0') {
        return OPRT_COM_ERROR;
    }

    strncpy((char *)imei, (char *)dce_pdp_ctx.imei, TKL_CELLULAR_IMEI_LEN);
    return OPRT_OK;
#else
    return OPRT_NOT_SUPPORTED;
#endif
}

/**
 * @brief  get the Serial Number of the cellular module
 * 
 * @param[out]   sn: Serial Number
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_get_sn(char *sn)
{
#if CONFIG_BK_MODEM
    BK_ASSERT(NULL != sn);

    if (bk_modem_mf_test && !bk_modem_dce_get_cfsn()) {
        return OPRT_COM_ERROR;
    }

    if (dce_pdp_ctx.sn[0] == '\0') {
        return OPRT_COM_ERROR;
    }

    strncpy((char *)sn, (char *)dce_pdp_ctx.sn, TKL_CELLULAR_SN_LEN);
    return OPRT_OK;
#else
    return OPRT_NOT_SUPPORTED;
#endif
}

/**
 * @brief  get the Software version of the cellular module
 * 
 * @param[out]   ver: Software version
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_get_sw_ver(char *ver)
{
#if CONFIG_BK_MODEM
    BK_ASSERT(NULL != ver);

    if (bk_modem_mf_test && !bk_mode_dce_get_cgmr()) {
        return OPRT_COM_ERROR;
    }

    if (dce_pdp_ctx.sw_ver[0] == '\0') {
        return OPRT_COM_ERROR;
    }

    strncpy((char *)ver, (char *)dce_pdp_ctx.sw_ver, TKL_CELLULAR_SW_VER_LEN);
    return OPRT_OK;
#else
    return OPRT_NOT_SUPPORTED;
#endif
}

/**
 * @brief  start cellular mf test
 *
 * @param[in]   cfg: the configure for cellular mf test
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_mf_test_start(TKL_CELLULAR_BASE_CFG_T *cfg)
{
#if CONFIG_BK_MODEM
    BK_ASSERT(NULL != cfg);

    if ((NULL == bk_modem_mf_test_sem)) {
        bk_modem_mf_test_sem = xSemaphoreCreateCounting(1, 0);
    }

    if (NULL == bk_modem_mf_test_sem) {
        return OPRT_COM_ERROR;
    }

    bk_modem_mf_test = 1;
    if (bk_modem_status && (bk_modem_get_mode() == PPP_DATA_MODE)) {
        bk_modem_deinit();
        tkl_system_sleep(500);
    }
    
    if (OPRT_OK != tkl_cellular_init(cfg)) {
        vSemaphoreDelete(bk_modem_mf_test_sem);
        bk_modem_mf_test_sem = NULL;
        return OPRT_COM_ERROR;
    }

    xSemaphoreTake(bk_modem_mf_test_sem, (5000 / portTICK_RATE_MS));
    return OPRT_OK;
#else
    return OPRT_NOT_SUPPORTED;
#endif
}

/**
 * @brief  stop cellular mf test
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_cellular_mf_test_stop(void)
{
#if CONFIG_BK_MODEM
    bk_modem_deinit();
    if (bk_modem_mf_test_sem) {
        vSemaphoreDelete(bk_modem_mf_test_sem);
        bk_modem_mf_test_sem = NULL;
    }
    bk_modem_mf_test = 0;
    return OPRT_OK;
#else
    return OPRT_NOT_SUPPORTED;
#endif
}

