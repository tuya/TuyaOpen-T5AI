#include "common/bk_include.h"

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
#include "tkl_ipc.h"
#include "tkl_semaphore.h"
#include "tkl_system.h"

static TKL_SEM_HANDLE lwip_send_sem = NULL;

extern OPERATE_RET tuya_ipc_send_sync(struct ipc_msg_s *msg);
extern OPERATE_RET tuya_ipc_send_no_sync(struct ipc_msg_s *msg);

static struct pbuf * __lwip_recv_pbuf_copy(struct pbuf *p)
{
    struct pbuf *q0 = NULL;
    struct pbuf *q1 = NULL;
    struct pbuf *Nq = NULL;
    struct pbuf *Np = p;

    while(Np) {
        if(Np->len) {
            Nq = pbuf_alloc(PBUF_RAW,  Np->len, PBUF_RAM);
            if(Nq) {
                memcpy(Nq->payload, Np->payload, Np->len);
                Nq->tot_len = Np->tot_len;
                if(q0 == NULL) {
                    q0 = Nq;
                    q1 = Nq;
                } else {
                    q1->next = Nq;
                    q1 = Nq;
                }
            } else {
                bk_printf("pbuf malloc failed!\r\n");
                if(q0)
                    pbuf_free(q0);
                return NULL;
            }
        }

        Np = Np->next;
    }

    return q0;
}


void tkl_lwip_ipc_func(struct ipc_msg_s *msg)
{
    //uint8_t ret = 0;
    if(!msg) {
        bk_printf("cpu1 failed, msg is NULL %d\r\n", __LINE__);
        return;
    }

    switch(msg->subtype) {
        case TKL_IPC_TYPE_LWIP_RECV:
        {
			struct ipc_msg_param_s *param= (struct ipc_msg_param_s *)msg->req_param;
			TKL_NETIF_HANDLE netif = param->p1;
			TKL_PBUF_HANDLE p = param->p2;
            struct pubf *q = (struct pubf *)__lwip_recv_pbuf_copy(p);
            msg->ret_value = q ? 0 : 1;
            tuya_ipc_send_no_sync(msg);
            if(q)
			    tkl_ethernetif_recv(netif, q);
        }
            break;
        default:
            break;
    }
    return;
}

/**
 * @brief get netif by index
 *
 * @param[in]      net_if_idx     netif index
 * @return  err_t  SEE "err_enum_t" in "lwip/err.h" to see the lwip err(ERR_OK: SUCCESS other:fail)
 */
struct netif *tkl_lwip_get_netif_by_index(int netif_idx)
{
	struct ipc_msg_s lwip_ipc_msg = {0};
    lwip_ipc_msg.type = TKL_IPC_TYPE_LWIP;
    lwip_ipc_msg.subtype = TKL_IPC_TYPE_LWIP_GET_NETIF_BY_IDX;
	lwip_ipc_msg.req_param = (uint8_t *)&netif_idx;
	lwip_ipc_msg.req_len = 4;

    OPERATE_RET ret = tuya_ipc_send_sync(&lwip_ipc_msg);
    if(ret)
        return NULL;

    if(lwip_ipc_msg.ret_value)
        return NULL;

    return (struct netif *)lwip_ipc_msg.res_param;
}

/**
 * @brief ethernet interface hardware init
 *
 * @param[in]      netif     the netif to which to send the packet
 * @return  err_t  SEE "err_enum_t" in "lwip/err.h" to see the lwip err(ERR_OK: SUCCESS other:fail)
 */
OPERATE_RET tkl_ethernetif_init(TKL_NETIF_HANDLE netif)
{
    OPERATE_RET ret;
	struct ipc_msg_s lwip_ipc_msg = {0};

    lwip_ipc_msg.type = TKL_IPC_TYPE_LWIP;
    lwip_ipc_msg.subtype = TKL_IPC_TYPE_LWIP_INIT;
	lwip_ipc_msg.req_param = netif;
	lwip_ipc_msg.req_len = 4;

    if (lwip_send_sem == NULL) {
        ret = tkl_semaphore_create_init(&lwip_send_sem, 0, 1);
        if(ret)
            return ret;
    }

    ret = tuya_ipc_send_sync(&lwip_ipc_msg);
    if(ret)
        return ret;

    return lwip_ipc_msg.ret_value;
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
	struct ipc_msg_s lwip_ipc_msg = {0};
    lwip_ipc_msg.type = TKL_IPC_TYPE_LWIP;
    lwip_ipc_msg.subtype = TKL_IPC_TYPE_LWIP_SEND;

	struct ipc_msg_param_s param = {0};
    param.p1 = netif;
    param.p2 = p;

	lwip_ipc_msg.req_param = (uint8_t *)&param;
	lwip_ipc_msg.req_len = sizeof(struct ipc_msg_param_s);

    //  tuya_ipc_send_no_sync(&lwip_ipc_msg);

    int ret = tuya_ipc_send_sync(&lwip_ipc_msg);
    if(ret)
        return ret;

    return lwip_ipc_msg.ret_value;


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
	struct netif *pnetif = netif;
	if (pnetif->input(p, pnetif) != ERR_OK) {
		pbuf_free(p);
		p = NULL;
		return -1;
	}

	return 0;
}


