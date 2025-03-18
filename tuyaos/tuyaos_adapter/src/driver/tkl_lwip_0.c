#include "tkl_lwip.h"
#include "tkl_queue.h"
#include "bk_wifi.h"
#include "bk_private/bk_wifi.h"
#ifdef CONFIG_BRIDGE
#include "bridgeif.h"
#include "rwnx_config.h"
#endif
#include "bk_drv_model.h"

#include "common/bk_include.h"
#include <stdio.h>
#include <string.h>
#include "bk_drv_model.h"
#include <os/mem.h>
#ifdef CONFIG_BRIDGE
#include "bridgeif.h"
#include "rwnx_config.h"
#endif
#include <os/os.h>
#include "tkl_ipc.h"
#include "netif.h"
#include "netif/ethernet.h"

#include "tkl_thread.h"
#include "tkl_memory.h"
#include "tkl_wifi.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pbuf.h"

extern OPERATE_RET tuya_ipc_send_sync(struct ipc_msg_s *msg);
extern OPERATE_RET tuya_ipc_send_no_sync(struct ipc_msg_s *msg);

static TaskHandle_t __thread_handle = NULL;
static TKL_QUEUE_HANDLE send_queue = NULL;

extern int bmsg_tx_sender(struct pbuf *p, uint32_t vif_idx);
extern int ke_l2_packet_tx(unsigned char *buf, int len, int flag);

static struct pbuf *__lwip_send_pbuf_copy(struct pbuf *p)
{
    struct pbuf *q = NULL;
    q = pbuf_clone(PBUF_RAW_TX, PBUF_RAM, p);
    return q;
}

void tkl_lwip_ipc_func(struct ipc_msg_s *msg)
{
    int ret = 0;
    switch(msg->subtype) {
        case TKL_IPC_TYPE_LWIP_INIT:
        {
            struct netif *pnetif = (struct netif *)msg->req_param;
            ret = tkl_ethernetif_init(pnetif);
            msg->ret_value = ret;
            tuya_ipc_send_no_sync(msg);
        }
            break;

        case TKL_IPC_TYPE_LWIP_GET_NETIF_BY_IDX:
        {
            uint8_t index = *(uint8_t *)(msg->req_param);
            struct netif *pnetif = tkl_lwip_get_netif_by_index(index);
            msg->res_param = (uint8_t *)pnetif;
            msg->res_len = 4;

            msg->ret_value = ret;
            tuya_ipc_send_no_sync(msg);
        }
            break;

        case TKL_IPC_TYPE_LWIP_SEND:
        {
            struct ipc_msg_param_s *param= (struct ipc_msg_param_s*)msg->req_param;
            TKL_NETIF_HANDLE netif = param->p1;
            TKL_PBUF_HANDLE p = param->p2;

            struct pbuf *q =  __lwip_send_pbuf_copy((struct pbuf *)p);
            if(q) {
                msg->ret_value = 0;
                tuya_ipc_send_no_sync(msg);
                int *recv_param = (int *)tkl_system_malloc(sizeof(int) * 2);
                recv_param[0] = (int)netif;
                recv_param[1] = (int)q;
                if (send_queue) {
                    tkl_queue_post(send_queue, &recv_param, 1000);
                }
            }else {
                bk_printf("lwip send pbuf alloc failed!!\r\n\n");
                msg->ret_value = -1;
                tuya_ipc_send_no_sync(msg);
            }
        }
            break;

        default:
            break;
    }


    return;
}


extern void *net_get_sta_handle(void);
extern void *net_get_uap_handle(void);
struct netif *tkl_lwip_get_netif_by_index(int net_if_idx)
{
    if(net_if_idx == 0)
        return net_get_sta_handle();
    else if(net_if_idx == 1)
        return net_get_uap_handle();

    return NULL;
}

static void low_level_init(struct netif *netif)
{
    uint8_t macptr[6] = {0};
    uint8_t id = 0 ;
    if(strncmp(netif->name, "r0", 2) == 0)	{
        id = 0;
    }else if(strncmp(netif->name, "r1", 2) == 0) {
        id = 1;
    }else {
        bk_printf("netif->name error %c  %c!\r\n", netif->name[0],netif->name[1]);
        return;
    }

    tkl_wifi_get_mac((const WF_IF_E)id, (NW_MAC_S *)macptr);

    /* set MAC hardware address length */
    bk_printf("mac %2x:%2x:%2x:%2x:%2x:%2x\r\n", macptr[0], macptr[1], macptr[2],
            macptr[3], macptr[4], macptr[5]);

    netif->hwaddr_len = 6;
    os_memcpy(netif->hwaddr, macptr, 6);
}

static void tkl_lwip_send_thread(void *arg)
{
    int *msg = NULL;
    while (1) {
        tkl_queue_fetch(send_queue, &msg, portMAX_DELAY);
        if (msg != NULL) {
            tkl_ethernetif_output((TKL_NETIF_HANDLE)msg[0], (TKL_PBUF_HANDLE)msg[1]);
            pbuf_free((struct pbuf *)(msg[1]));
            tkl_system_free(msg);
            msg = NULL;
        }
    }
}

/**
 * @brief ethernet interface hardware init
 *
 * @param[in]      netif     the netif to which to send the packet
 * @return  err_t  SEE "err_enum_t" in "lwip/err.h" to see the lwip err(ERR_OK: SUCCESS other:fail)
 */
OPERATE_RET tkl_ethernetif_init(TKL_NETIF_HANDLE netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    /* initialize the hardware */
    low_level_init(netif);

    if (__thread_handle == NULL) {
        extern void sys_init(void);
        sys_init();
        tkl_queue_create_init(&send_queue, 4, 64);
        xTaskCreate(tkl_lwip_send_thread, "lwip_send", 4096, NULL, 7, &__thread_handle);
    }

    return OPRT_OK;
}

/**
 * @brief ethernet interface sendout the pbuf packet
 *
 * @param[in]      netif     the netif to which to send the packet
 * @param[in]      p         the packet to be send, in pbuf mode
 * @return  err_t  SEE "err_enum_t" in "lwip/err.h" to see the lwip err(ERR_OK: SUCCESS other:fail)
 */
OPERATE_RET tkl_ethernetif_output(TKL_NETIF_HANDLE netif, TKL_PBUF_HANDLE p)
{
    int ret;
    err_t err = ERR_OK;
    uint8_t vif_idx = wifi_netif_vif_to_vifid(((struct netif *)netif)->state);
    // Sanity check
    if (vif_idx == 0xff)
        return ERR_ARG;

#if 0
#if CONFIG_WIFI6_CODE_STACK
    //bk_printf("output:%x\r\n", p);
    extern bool special_arp_flag;
    if(special_arp_flag)
    {
        ret = bmsg_special_tx_sender(p, (uint32_t)vif_idx);
        special_arp_flag = false;
    }
    else
#endif
#endif
    {
        ret = bmsg_tx_sender(p, (uint32_t)vif_idx);

        // struct ipc_msg_s lwip_ipc_msg = {0};
        // lwip_ipc_msg.type = TKL_IPC_TYPE_LWIP;
        // lwip_ipc_msg.subtype = TKL_IPC_TYPE_LWIP_SEND_RES;
        // lwip_ipc_msg.ret_value = ret;

        // tuya_ipc_send_sync(&lwip_ipc_msg);

    }
    if(0 != ret)
    {
        err = ERR_TIMEOUT;
    }

    return err;
}

/**
 * @brief ethernet interface recv the packet
 *
 * @param[in]      netif       the netif to which to recieve the packet
 * @param[in]      total_len   the length of the packet recieved from the netif
 * @return  void
 */
OPERATE_RET tkl_ethernetif_recv(TKL_NETIF_HANDLE netif, TKL_PBUF_HANDLE p)
{
    struct ipc_msg_s lwip_ipc_msg = {0};
    lwip_ipc_msg.type = TKL_IPC_TYPE_LWIP;
    lwip_ipc_msg.subtype = TKL_IPC_TYPE_LWIP_RECV;

    struct ipc_msg_param_s param = {0};
    param.p1 = netif;
    param.p2 = p;

    lwip_ipc_msg.req_param = (uint8_t *)&param;
    lwip_ipc_msg.req_len = sizeof(struct ipc_msg_param_s);

    OPERATE_RET ret = tuya_ipc_send_sync(&lwip_ipc_msg);
    // pbuf_free((struct pbuf *)p);
    if(ret)
        return ret;

    return lwip_ipc_msg.ret_value;
}

void ethernetif_input(int iface, struct pbuf *p, int dst_idx)
{
    struct eth_hdr *ethhdr;
    struct netif *netif;
    void *vif;

    if (p->len <= 14) {
        pbuf_free(p);
        return;
    }

    vif = wifi_netif_vifid_to_vif(iface);
    netif = (struct netif *)wifi_netif_get_vif_private_data(vif);
    if(!netif) {
        //bk_printf("ethernetif_input no netif found %d\r\n", iface);
        pbuf_free(p);
        p = NULL;
        return;
    }

    /* points to packet payload, which starts with an Ethernet header */
    ethhdr = p->payload;

    /* need to forward */
#if 0
    if (wifi_netif_vif_to_netif_type(vif) == NETIF_IF_AP) {
        // If dest sta is known, or packet is multicast, forward this packet
        if ((ethhdr->dest.addr[0] & 1) || dst_idx != 0xff) {
            // check if is arp request to us, doesn't need to forward
            struct pbuf *q;

            // unicast frame, check da staidx
            // for softap+ap, if sta under softap sends packets to router,
            // dst_idx will be valid, and packets will be forwarded in our
            // softap bss.
            if (!(ethhdr->dest.addr[0] & 1) && dst_idx < NX_REMOTE_STA_MAX) {
                void *sta = sta_mgmt_get_entry(dst_idx);
                // if STA doesn't belong to this vif
                if (mac_sta_mgmt_get_inst_nbr(sta) != mac_vif_mgmt_get_index(vif)) {
                    goto process;
                }
            }

            if (ethhdr->type == PP_HTONS(ETHTYPE_ARP)) {
                struct etharp_hdr *hdr = (struct etharp_hdr *)(ethhdr + 1);
                if (hdr->opcode != PP_HTONS(ARP_REQUEST))
                    goto forward;
#if CONFIG_BRIDGE
                // FIXME: Handle ARP Probe
                struct netif *brif;
                bridgeif_port_t *port;
                if (bridgeif_netif_client_id != 0xff) {
                    port = (bridgeif_port_t *)netif_get_client_data(netif, bridgeif_netif_client_id);
                    if (!port || !port->bridge || !port->bridge->netif)
                        goto forward;
                    brif = port->bridge->netif;
                    if (!memcmp(&hdr->dipaddr, &brif->ip_addr, 4)) {
                        // os_printf("DIP TO BR\n");
                        goto process;
                    }
                } else {
                    if (!memcmp(&hdr->dipaddr, &netif->ip_addr, 4)) {
                        // os_printf("DIP TO SOFTAP\n");
                        goto process;
                    }
                }
#else
                if (!memcmp(&hdr->dipaddr, &netif->ip_addr, 4)) {
                    // os_printf("DIP TO SOFTAP\n");
                    goto process;
                }
#endif
            }
forward:
            q = pbuf_clone(PBUF_RAW_TX, PBUF_RAM, p);
            if (q != NULL) {
                low_level_output(netif, q);
                pbuf_free(q);
            } else {
                bk_printf("alloc pbuf failed, don't forward\r\n");
            }
        }
    }
#endif

// process:
    switch (htons(ethhdr->type))
    {
#if 0
        /* IP or ARP packet? */
        case ETHTYPE_IP:
        case ETHTYPE_ARP:
#ifdef CONFIG_IPV6
        case ETHTYPE_IPV6:
            wlan_set_multicast_flag();
#endif
#if PPPOE_SUPPORT
            /* PPPoE packet? */
        case ETHTYPE_PPPOEDISC:
        case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */
            /* full packet send to tcpip_thread to process */
            if (netif->input(p, netif) != ERR_OK)    // ethernet_input
            {
                LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\r\n"));
                pbuf_free(p);
                p = NULL;
            }
            break;
#endif
        case 0x888E:    // ETHTYPE_EAPOL
            ke_l2_packet_tx(p->payload, p->len, iface);
            pbuf_free(p);
            p = NULL;
            break;

        default:
            tkl_ethernetif_recv(netif, p);
            pbuf_free(p);
            p = NULL;
            break;
    }

    return;
}

