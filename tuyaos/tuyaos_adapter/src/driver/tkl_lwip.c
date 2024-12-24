#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "lwip/icmp.h"
#include "lwip/inet.h"
#include "netif/etharp.h"
#include "lwip/err.h"
#include "tkl_lwip.h"
// #include "rwnx_config.h"
// #include "rw_pub.h"
// #include "sk_intf.h"

/**
 * @brief ethernet interface hardware init
 *
 * @param[in]      netif     the netif to which to send the packet
 * @return  err_t  SEE "err_enum_t" in "lwip/err.h" to see the lwip err(ERR_OK: SUCCESS other:fail)
 */
OPERATE_RET tkl_ethernetif_init(TKL_NETIF_HANDLE netif)
{
    // struct netif *p_netif = (struct netif *)netif;
    
    // bk_printf("tkl_ethernetif_init\r\n");
    // p_netif->flags |= NETIF_FLAG_LINK_UP;

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
	err_t ret = ERR_OK;
    // struct pbuf *p_buf = (struct pbuf *)p;
    // struct netif *p_netif = (struct netif *)netif;
    // uint8_t vif_idx = rwm_mgmt_get_netif2vif(p_netif);
    // extern int bmsg_tx_sender(struct pbuf *p, uint32_t vif_idx);

	// if (vif_idx >= NX_VIRT_DEV_MAX)
	// {
	// 	bk_printf("%s: invalid vif: %d!\r\n", __func__, vif_idx);
	// 	return ERR_ARG;
	// }
    
	// ret = bmsg_tx_sender(p_buf, (uint32_t)vif_idx);
	// if(0 != ret)
	// {
	// 	err = ERR_TIMEOUT;
	// }

    return ret;
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
//     uint8_t vif_idx = 0;
//     struct eth_hdr *ethhdr;
//     struct pbuf *p_buf = (struct pbuf *)p;
//     struct netif *p_netif = (struct netif *)netif;
    
// 	if (p_buf->len <= SIZEOF_ETH_HDR) {
// 		pbuf_free(p_buf);
// 		return OPRT_COM_ERROR; 
// 	} 

//     if(!p_netif) {
//         //ETH_INTF_PRT("ethernetif_input no netif found %d\r\n", iface);
//         pbuf_free(p_buf);
//         p_buf = NULL;
//         return OPRT_COM_ERROR;
//     }
    
//     /* points to packet payload, which starts with an Ethernet header */
//     ethhdr = p_buf->payload;
//     switch (htons(ethhdr->type))
//     {
//         /* IP or ARP packet? */
//     case ETHTYPE_IP:
//     case ETHTYPE_ARP:
// #if PPPOE_SUPPORT
//         /* PPPoE packet? */
//     case ETHTYPE_PPPOEDISC:
//     case ETHTYPE_PPPOE:
// #endif /* PPPOE_SUPPORT */
//         /* full packet send to tcpip_thread to process */
//         if (p_netif->input(p_buf, p_netif) != ERR_OK)    // ethernet_input
//         {
//             bk_printf("ethernetif_input: IP input error\r\n");
//             pbuf_free(p_buf);
//             p_buf = NULL;
//             return OPRT_COM_ERROR;
//         }
//         break;
        
//     case ETHTYPE_EAPOL:
//         vif_idx = rwm_mgmt_get_netif2vif(netif);
//         ke_l2_packet_tx(p_buf->payload, p_buf->len, vif_idx);
//         pbuf_free(p_buf);
//         p_buf = NULL;
//         break;
        
//     default:
//         pbuf_free(p_buf);
//         p_buf = NULL;
//         break;
//     }
    
	return OPRT_OK;
}


