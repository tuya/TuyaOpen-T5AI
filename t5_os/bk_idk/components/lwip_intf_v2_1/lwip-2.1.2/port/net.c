#include <common/bk_include.h>
#include <stdio.h>
#include <string.h>

#include <lwip/inet.h>
#include "netif/etharp.h"
#include "lwip/netif.h"
#include <lwip/netifapi.h>
#include <lwip/tcpip.h>
#include <lwip/dns.h>
#include <lwip/dhcp.h>
#include "lwip/prot/dhcp.h"
#include "lwip/apps/mdns.h"

#include <lwip/sockets.h>
#include "wlanif.h"
#if CONFIG_ETH
#include "ethernetif.h"
#include "miiphy.h"
#endif

#include <components/system.h>
#include "bk_drv_model.h"
#include <os/mem.h>
#include "bk_wifi.h" //TODO use standard EVENT instead!!!
#include "lwip_netif_address.h"
#include <os/os.h>
#include "net.h"

#include "bk_wifi_types.h"
#include "bk_wifi.h"
#include <os/str.h>
#include <modules/wifi.h>
#include "bk_feature.h"
#if IP_NAPT
#include "lwip/lwip_napt.h"
#endif
/* forward declaration */
FUNC_1PARAM_PTR bk_wlan_get_status_cb(void);

struct ipv4_config sta_ip_settings = {
	.addr_type = ADDR_TYPE_DHCP,
	.address = 0,
	.gw = 0,
	.netmask = 0,
	.dns1 = 0,
	.dns2 = 0,
};

struct ipv4_config uap_ip_settings = {
	.addr_type = ADDR_TYPE_STATIC,
	.address = 0xc0a80001, //192.168.0.1
	.gw = 0xc0a80001,      //192.168.0.1
	.netmask = 0xffffff00, //255.255.255.0
	.dns1 = 0xc0a80001,    //192.168.0.1
	.dns2 = 0,
};

#ifdef CONFIG_ETH
struct ipv4_config eth_ip_settings = {
#if CONFIG_ETH_DHCP
	.addr_type = ADDR_TYPE_DHCP, // ADDR_TYPE_DHCP
#else
	.addr_type = ADDR_TYPE_STATIC, // ADDR_TYPE_STATIC
#endif
	.address = 0x0afaa8c0, //192.168.250.10, network order
	.gw = 0x01faa8c0,      //192.168.250.1, network order
	.netmask = 0x00ffffff, //255.255.255.0, network order
	.dns1 = 0x01faa8c0,    //192.168.250.1, network order
	.dns2 = 0,
};
#endif

#if CONFIG_BRIDGE
struct ipv4_config br_ip_settings = {
	.addr_type = ADDR_TYPE_DHCP,
	.address = 0,
	.gw = 0,
	.netmask = 0,
	.dns1 = 0,
	.dns2 = 0,
};
#endif

static char up_iface;
static bool sta_ip_start_flag = false;
bool uap_ip_start_flag = false;
#ifdef CONFIG_ETH
static bool eth_ip_start_flag = false;
#endif
#if CONFIG_BRIDGE
static bool bridge_ip_start_flag = false;
#endif

#ifdef CONFIG_IPV6
#define IPV6_ADDR_STATE_TENTATIVE       "Tentative"
#define IPV6_ADDR_STATE_PREFERRED       "Preferred"
#define IPV6_ADDR_STATE_INVALID         "Invalid"
#define IPV6_ADDR_STATE_VALID           "Valid"
#define IPV6_ADDR_STATE_DEPRECATED      "Deprecated"
#define IPV6_ADDR_TYPE_LINKLOCAL        "Link-Local"
#define IPV6_ADDR_TYPE_GLOBAL           "Global"
#define IPV6_ADDR_TYPE_UNIQUELOCAL      "Unique-Local"
#define IPV6_ADDR_TYPE_SITELOCAL        "Site-Local"
#define IPV6_ADDR_UNKNOWN               "Unknown"
#endif

typedef void (*net_sta_ipup_cb_fn)(void *data);

struct iface {
	struct netif netif;
	ip_addr_t ipaddr;
	ip_addr_t nmask;
	ip_addr_t gw;
	const char *name;
};
FUNCPTR sta_connected_func;

static struct iface g_mlan = {{.name = "s0",}, .name = "sta"};
static struct iface g_uap = {{.name = "ap",}, .name = "ap"};
#ifdef CONFIG_ETH
static struct iface g_eth = {{.name = "e0",}, .name = "eth"};
#endif
#if CONFIG_BRIDGE
static struct iface g_br = {{.name = "br"}, .name = "br"};
#endif
net_sta_ipup_cb_fn sta_ipup_cb = NULL;

extern void *net_get_sta_handle(void);
extern void *net_get_uap_handle(void);
extern err_t lwip_netif_init(struct netif *netif);
extern err_t lwip_netif_uap_init(struct netif *netif);
extern int net_get_if_ip_addr(uint32_t *ip, void *intrfc_handle);
extern int net_get_if_ip_mask(uint32_t *nm, void *intrfc_handle);
extern int net_configure_address(struct ipv4_config *addr, void *intrfc_handle);
extern int dhcp_server_start(void *intrfc_handle);
extern void dhcp_server_stop(void);
extern void net_configure_dns(struct iface *, struct wlan_ip_config *ip);
bk_err_t bk_wifi_get_ip_status(IPStatusTypedef *outNetpara, WiFi_Interface inInterface);


#ifdef CONFIG_IPV6
char *ipv6_addr_state_to_desc(unsigned char addr_state)
{
	if (ip6_addr_istentative(addr_state))
		return IPV6_ADDR_STATE_TENTATIVE;
	else if (ip6_addr_ispreferred(addr_state))
		return IPV6_ADDR_STATE_PREFERRED;
	else if (ip6_addr_isinvalid(addr_state))
		return IPV6_ADDR_STATE_INVALID;
	else if (ip6_addr_isvalid(addr_state))
		return IPV6_ADDR_STATE_VALID;
	else if (ip6_addr_isdeprecated(addr_state))
		return IPV6_ADDR_STATE_DEPRECATED;
	else
		return IPV6_ADDR_UNKNOWN;
}

char *ipv6_addr_type_to_desc(struct ipv6_config *ipv6_conf)
{
	if (ip6_addr_islinklocal((ip6_addr_t *)&ipv6_conf->address))
		return IPV6_ADDR_TYPE_LINKLOCAL;
	else if (ip6_addr_isglobal((ip6_addr_t *)&ipv6_conf->address))
		return IPV6_ADDR_TYPE_GLOBAL;
	else if (ip6_addr_isuniquelocal((ip6_addr_t *)&ipv6_conf->address))
		return IPV6_ADDR_TYPE_UNIQUELOCAL;
	else if (ip6_addr_issitelocal((ip6_addr_t *)&ipv6_conf->address))
		return IPV6_ADDR_TYPE_SITELOCAL;
	else
		return IPV6_ADDR_UNKNOWN;
}
#endif /* CONFIG_IPV6 */

int net_dhcp_hostname_set(char *hostname)
{
	netif_set_hostname(&g_mlan.netif, hostname);
	return 0;
}

void net_ipv4stack_init(void)
{
	static bool tcpip_init_done = 0;

	if (tcpip_init_done)
		return;

	LWIP_LOGI("init TCP/IP\r\n");
	tcpip_init(NULL, NULL);
	tcpip_init_done = true;
}

#ifdef CONFIG_IPV6
void net_ipv6stack_init(struct netif *netif)
{
	bk_get_mac(netif->hwaddr, MAC_TYPE_STA);
	netif->hwaddr_len = 6;
}
#endif /* CONFIG_IPV6 */

void net_wlan_init(void)
{
	static int wlan_init_done = 0;
	int ret;

	if (!wlan_init_done) {
		net_ipv4stack_init();
        ip_addr_set_ip4_u32(&g_mlan.ipaddr,INADDR_ANY);
		ret = netifapi_netif_add(&g_mlan.netif,ip_2_ip4(&g_mlan.ipaddr),
					 ip_2_ip4(&g_mlan.ipaddr), ip_2_ip4(&g_mlan.ipaddr), NULL,
					 lwip_netif_init, tcpip_input);
		if (ret) {
			/*FIXME: Handle the error case cleanly */
			LWIP_LOGE("sta netif add failed");
		}
#ifdef CONFIG_IPV6
		net_ipv6stack_init(&g_mlan.netif);
#endif /* CONFIG_IPV6 */

		ret = netifapi_netif_add(&g_uap.netif, ip_2_ip4(&g_uap.ipaddr),
					 ip_2_ip4(&g_uap.ipaddr), ip_2_ip4(&g_uap.ipaddr), NULL,
					 lwip_netif_uap_init, tcpip_input);
		if (ret) {
			/*FIXME: Handle the error case cleanly */
			LWIP_LOGE("ap netif add failed");
		}
		wlan_init_done = 1;
	}

	return;
}

void net_set_sta_ipup_callback(void *fn)
{
	sta_ipup_cb = (net_sta_ipup_cb_fn)fn;
}

void user_connected_callback(FUNCPTR fn)
{
	sta_connected_func = fn;
}
extern void TOGGLE_GPIO18_DOWN();

static void wm_netif_status_static_callback(struct netif *n)
{
	if (n->flags & NETIF_FLAG_UP) {
		// static IP success;
		LWIP_LOGI("using static ip...\n");
#ifdef CONFIG_WIFI_ENABLE
		if (n == &g_mlan.netif) {
			wifi_netif_notify_sta_got_ip();

			if (bk_feature_fast_dhcp_enable()) {
				/* read stored IP from flash as the static IP */
				struct wlan_fast_connect_info fci = {0};
				wlan_read_fast_connect_info(&fci);
				ip_addr_set_ip4_u32(&n->ip_addr, *((u32 *)&fci.ip_addr));
				ip_addr_set_ip4_u32(&n->netmask, *((u32 *)&fci.netmask));
				ip_addr_set_ip4_u32(&n->gw, *((u32 *)&fci.gw));
				os_memcpy((char *)&n->dns1, (char *)&fci.dns1, sizeof(n->dns1));
				LWIP_LOGI("ip_addr: "BK_IP4_FORMAT" \r\n", BK_IP4_STR(ip_addr_get_ip4_u32(&n->ip_addr)));
			}
#if !CONFIG_DISABLE_DEPRECIATED_WIFI_API
			if (sta_ipup_cb != NULL)
				sta_ipup_cb(NULL);

			if (sta_connected_func != NULL)
				(*sta_connected_func)();
#endif // !CONFIG_DISABLE_DEPRECIATED_WIFI_API
		}
#endif // CONFIG_WIFI_ENABLE
	} else {
		// static IP fail;
	}
}

//extern wifi_connect_tick_t sta_tick;
#if IP_NAPT
const ip_addr_t *sta_dns;
#endif
static void wm_netif_status_callback(struct netif *n)
{
	struct dhcp *dhcp;

	if (n->flags & NETIF_FLAG_UP) {
		dhcp = netif_dhcp_data(n);
#ifdef CONFIG_IPV6
		int i;
		u8 *ipv6_addr;

		for (i = 0; i < MAX_IPV6_ADDRESSES; i++) {
			if (ip6_addr_isvalid(netif_ip6_addr_state(n, i))) {
				ipv6_addr = (u8 *)(ip_2_ip6(&n->ip6_addr[i]))->addr;
				LWIP_LOGI("ipv6_addr[%d] : %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\r\n", i,
						  ipv6_addr[0], ipv6_addr[1], ipv6_addr[2], ipv6_addr[3],
						  ipv6_addr[4], ipv6_addr[5], ipv6_addr[6], ipv6_addr[7],
						  ipv6_addr[8], ipv6_addr[9], ipv6_addr[10], ipv6_addr[11],
						  ipv6_addr[12], ipv6_addr[13], ipv6_addr[14], ipv6_addr[15]);
				LWIP_LOGI("ipv6_type[%d] :0x%x\r\n", i, n->ip6_addr[i].type);
				LWIP_LOGI("ipv6_state[%d] :0x%x\r\n", i, n->ip6_addr_state[i]);
			}
		}
#endif

#ifdef CONFIG_MDNS
		if (!ip_addr_isany(&n->ip_addr))
			mdns_resp_restart(n);

#endif
		if (dhcp != NULL) {
			/* dhcp success*/
			if (dhcp->state == DHCP_STATE_BOUND) {
				/*
				LWIP_LOGI("ip_addr: "BK_IP4_FORMAT" \r\n", BK_IP4_STR(ip_addr_get_ip4_u32(&n->ip_addr)));
				sta_tick.sta_ip_tick = rtos_get_time();
				LWIP_LOGD("STA assoc delta:%d, eapol delta:%d, dhcp delta:%d, total:%d\n",
					sta_tick.sta_assoc_tick - sta_tick.sta_start_tick,
					sta_tick.sta_eapol_tick - sta_tick.sta_assoc_tick,
					sta_tick.sta_ip_tick - sta_tick.sta_eapol_tick,
					sta_tick.sta_ip_tick - sta_tick.sta_start_tick);*/
#if IP_NAPT
				sta_dns = dns_getserver(0);
#endif
				if (0) {
#ifdef CONFIG_WIFI_ENABLE
				} else if (n == &g_mlan.netif) {
#if !CONFIG_DISABLE_DEPRECIATED_WIFI_API
					wifi_netif_call_status_cb_when_sta_got_ip();
#endif
					wifi_netif_notify_sta_got_ip();

					if (bk_feature_fast_dhcp_enable()) {
						/* store current IP to flash */
						const ip_addr_t *dns_server;
						dns_server = dns_getserver(0);
						n->dns1 = ip_addr_get_ip4_u32(dns_server);
						struct wlan_fast_connect_info fci = { 0 };
						wlan_read_fast_connect_info(&fci);
						os_memset(&fci.ip_addr, 0, sizeof(fci.ip_addr));
						os_memcpy((char *)&fci.ip_addr, (char *)ip_2_ip4(&n->ip_addr), sizeof(fci.ip_addr));
						os_memcpy((char *)&fci.netmask, (char *)ip_2_ip4(&n->netmask), sizeof(fci.netmask));
						os_memcpy((char *)&fci.gw, (char *)ip_2_ip4(&n->gw), sizeof(fci.gw));
						os_memcpy((char *)&fci.dns1, (char *)&n->dns1, sizeof(fci.dns1));
						wlan_write_fast_connect_info(&fci);
					}

#if !CONFIG_DISABLE_DEPRECIATED_WIFI_API
					if (sta_ipup_cb)
						sta_ipup_cb(NULL);

					if (sta_connected_func)
						(*sta_connected_func)();
#endif
				}
#endif // CONFIG_WIFI_ENABLE
#ifdef CONFIG_ETH
			} else if (n == &g_mlan.netif) {
				// Ethernet DHCP handler, clear ps prevent
				// TODO: ETH, DHCP, IPv6, RA, DHCPv6 handler
#endif
			} else {
				// dhcp fail
#ifdef CONFIG_WIFI_ENABLE
#if !CONFIG_DISABLE_DEPRECIATED_WIFI_API
				if (n == &g_mlan.netif)
					wifi_netif_call_status_cb_when_sta_dhcp_timeout();
#endif
#endif // CONFIG_WIFI_ENABLE
			}
		} else {
			// static IP success;
		}
	} else {
		// dhcp fail;
	}
}

static int check_iface_mask(void *handle, uint32_t ipaddr)
{
	uint32_t interface_ip, interface_mask;
	net_get_if_ip_addr(&interface_ip, handle);
	net_get_if_ip_mask(&interface_mask, handle);
	if (interface_ip > 0)
		if ((interface_ip & interface_mask) ==
			(ipaddr & interface_mask))
			return 0;
	return -1;
}

void *net_ip_to_interface(uint32_t ipaddr)
{
	int ret;
	void *handle;

	/* Check mlan handle */
	handle = net_get_sta_handle();
	ret = check_iface_mask(handle, ipaddr);
	if (ret == 0)
		return handle;

	/* Check uap handle */
	handle = net_get_uap_handle();
	ret = check_iface_mask(handle, ipaddr);
	if (ret == 0)
		return handle;

	/* If more interfaces are added then above check needs to done for
	 * those newly added interfaces
	 */
	return NULL;
}

void *net_sock_to_interface(int sock)
{
	struct sockaddr_in peer;
	unsigned long peerlen = sizeof(struct sockaddr_in);
	void *req_iface = NULL;

	getpeername(sock, (struct sockaddr *)&peer, &peerlen);
	req_iface = net_ip_to_interface(peer.sin_addr.s_addr);
	return req_iface;
}

void *net_get_sta_handle(void)
{
	return &g_mlan.netif;
}

void *net_get_uap_handle(void)
{
	return &g_uap.netif;
}

#if CONFIG_BRIDGE
void *net_get_br_handle(void)
{
	return &g_br.netif;
}
#endif

void *net_get_netif_handle(uint8_t iface)
{
	return NULL;
}

void net_interface_up(void *intrfc_handle)
{
	struct iface *if_handle = (struct iface *)intrfc_handle;

	netifapi_netif_set_up(&if_handle->netif);
}

void net_interface_down(void *intrfc_handle)
{
	struct iface *if_handle = (struct iface *)intrfc_handle;

	netifapi_netif_set_down(&if_handle->netif);
}


void net_interface_dhcp_stop(void *intrfc_handle)
{
	struct iface *if_handle = (struct iface *)intrfc_handle;

	netifapi_dhcp_stop(&if_handle->netif);
	netif_set_status_callback(&if_handle->netif, NULL);
}

void sta_ip_down(void)
{
	if (sta_ip_start_flag) {
		LWIP_LOGI("sta ip down\r\n");

		sta_ip_start_flag = false;

		netif_set_status_callback(&g_mlan.netif, NULL);
		netifapi_dhcp_stop(&g_mlan.netif);
		netifapi_netif_set_down(&g_mlan.netif);
#if LWIP_IPV6
		for (u8_t addr_idx = 1; addr_idx < LWIP_IPV6_NUM_ADDRESSES; addr_idx++) {
			netif_ip6_addr_set(&g_mlan.netif, addr_idx, (const ip6_addr_t *)IP6_ADDR_ANY);
			g_mlan.netif.ip6_addr_state[addr_idx] = IP6_ADDR_INVALID;
		}
#endif
	}
}

void sta_ip_start(void)
{
#ifdef CONFIG_WIFI_ENABLE
	struct wlan_ip_config address = { 0 };

	if (!sta_ip_start_flag) {
		LWIP_LOGD("sta ip start\r\n");
		sta_ip_start_flag = true;
		net_configure_address(&sta_ip_settings, net_get_sta_handle());
		return;
	}

	LWIP_LOGI("sta ip start: %pIn\n", &address.ipv4.address);
	net_get_if_addr(&address, net_get_sta_handle());
	if (wifi_netif_sta_is_connected() && address.ipv4.address)
		wifi_netif_notify_sta_got_ip();
#endif
}

#if CONFIG_BRIDGE
void bridge_set_ip_start_flag(bool enable)
{
	bridge_ip_start_flag = enable;
}

void bridge_ip_start(void)
{

	if (!bridge_ip_start_flag) {
		LWIP_LOGD("bridge ip start\r\n");
		bridge_ip_start_flag = true;
		net_configure_address(&br_ip_settings, net_get_br_handle());
		return;
	}
}

extern void bridgeif_deinit(struct netif *netif);
extern u8_t bridgeif_netif_client_id;

static void beken_reset_bridge(struct netif *netif)
{
	netif_set_client_data((struct netif *)net_get_sta_handle(), bridgeif_netif_client_id, NULL);
	netif_set_flags((struct netif *)net_get_sta_handle(), NETIF_FLAG_ETHARP);
	netif_set_client_data((struct netif *)net_get_uap_handle(), bridgeif_netif_client_id, NULL);
	netif_set_flags((struct netif *)net_get_uap_handle(), NETIF_FLAG_ETHARP);
	LWIP_LOGI("bridg ip down\r\n");
	netif_set_status_callback(&g_br.netif, NULL);
}

void bridge_ip_stop(void)
{
	if (bridge_ip_start_flag) {
		struct netif *br = (struct netif *)net_get_br_handle();

		netifapi_netif_common(br, beken_reset_bridge, NULL);
		netifapi_netif_remove(br);
		netifapi_netif_common(br, bridgeif_deinit, NULL);
#if LWIP_IPV6
		for (u8_t addr_idx = 1; addr_idx < LWIP_IPV6_NUM_ADDRESSES; addr_idx++) {
			netif_ip6_addr_set(&g_br.netif, addr_idx, (const ip6_addr_t *)IP6_ADDR_ANY);
			g_br.netif.ip6_addr_state[addr_idx] = IP6_ADDR_INVALID;
		}
#endif

		bridge_ip_start_flag = false;
	}
}

uint32_t bridge_ip_is_start(void)
{
	return bridge_ip_start_flag;
}
#endif

void sta_set_default_netif(void)
{
	netifapi_netif_set_default(net_get_sta_handle());
}

void ap_set_default_netif(void)
{
	// as the default netif is sta's netif, so ap need to send
	// boardcast or not sub net packets, need set ap netif before
	// send those packets, after finish sending, reset default netif
	// to sat's netif.
	netifapi_netif_set_default(net_get_uap_handle());
#ifdef CONFIG_WIFI_REAPETER
#if (IP_FORWARD && IP_NAPT)
	if (netif_is_up(&g_mlan.netif))
		netifapi_netif_set_default(net_get_sta_handle());
#endif
#endif

#ifdef CONFIG_ETH
#if (IP_FORWARD && IP_NAPT)
	if (netif_is_up(&g_eth.netif) && netif_is_link_up(&g_eth.netif))
		netifapi_netif_set_default(&g_eth.netif);
#endif
#endif
}

void reset_default_netif(void)
{
	if (sta_ip_is_start()) {
		netifapi_netif_set_default(net_get_sta_handle());
	} else {
		netifapi_netif_set_default(NULL);
	}
}

uint32_t sta_ip_is_start(void)
{
	return sta_ip_start_flag;
}

void uap_ip_down(void)
{
	if (uap_ip_start_flag) {
		LWIP_LOGI("uap ip down\r\n");
		uap_ip_start_flag = false;

		netifapi_netif_set_down(&g_uap.netif);
		netif_set_status_callback(&g_uap.netif, NULL);
		dhcp_server_stop();
	}
}

void uap_ip_start(void)
{
	if (!uap_ip_start_flag) {
		LWIP_LOGD("uap ip start\r\n");
		uap_ip_start_flag = true;
		net_configure_address(&uap_ip_settings, net_get_uap_handle());
#if IP_NAPT
		ip_napt_enable(ip4_addr_get_u32(ip_2_ip4(&g_uap.ipaddr)), 1);
#endif
	}
}

uint32_t uap_ip_is_start(void)
{
	return uap_ip_start_flag;
}

#define DEF_UAP_IP        0xc0a80a01UL /* 192.168.10.1 */

void ip_address_set(int iface, int dhcp, char *ip, char *mask, char *gw, char *dns)
{
	uint32_t tmp;
	struct ipv4_config addr;

	memset(&addr, 0, sizeof(struct ipv4_config));
	if (dhcp == 1) {
		addr.addr_type = ADDR_TYPE_DHCP;
	} else {
		addr.addr_type = ADDR_TYPE_STATIC;
		tmp = inet_addr((char *)ip);
		addr.address = (tmp);
		tmp = inet_addr((char *)mask);
		if (tmp == 0xFFFFFFFF)
			tmp = 0x00FFFFFF; // if not set valid netmask, set as 255.255.255.0
		addr.netmask = (tmp);
		tmp = inet_addr((char *)gw);
		addr.gw = (tmp);

		tmp = inet_addr((char *)dns);
		addr.dns1 = (tmp);
	}

	if (iface == 0)
		memcpy(&uap_ip_settings, &addr, sizeof(addr));
	else if (iface == 1) // Station
		memcpy(&sta_ip_settings, &addr, sizeof(addr));
#if CONFIG_BRIDGE
	else if (iface == 2)
		memcpy(&br_ip_settings, &addr, sizeof(addr));
#endif
}

bk_err_t bk_wifi_get_ip_status(IPStatusTypedef *outNetpara, WiFi_Interface inInterface)
{
	bk_err_t ret = kNoErr;
	struct wlan_ip_config addr;

	os_memset(&addr, 0, sizeof(struct wlan_ip_config));

	switch (inInterface) {
	case BK_SOFT_AP:
		net_get_if_addr(&addr, net_get_uap_handle());
		net_get_if_macaddr(outNetpara->mac, net_get_uap_handle());
		break;

	case BK_STATION:
		net_get_if_addr(&addr, net_get_sta_handle());
		net_get_if_macaddr(outNetpara->mac, net_get_sta_handle());
		break;

	default:
		ret = kGeneralErr;
		break;
	}

	if (ret == kNoErr) {
		outNetpara->dhcp = addr.ipv4.addr_type;
		os_strcpy(outNetpara->ip, inet_ntoa(addr.ipv4.address));
		os_strcpy(outNetpara->mask, inet_ntoa(addr.ipv4.netmask));
		os_strcpy(outNetpara->gate, inet_ntoa(addr.ipv4.gw));
		os_strcpy(outNetpara->dns, inet_ntoa(addr.ipv4.dns1));
	}

	return ret;
}

void sta_ip_mode_set(int dhcp)
{
	if (dhcp == 1) {
		ip_address_set(1, DHCP_CLIENT, NULL, NULL, NULL, NULL);
	} else if (dhcp == 2) {
		uint32_t use_fast_dhcp = 0;
		if (bk_feature_fast_dhcp_enable()) {
			/* read stored IP from flash as the static IP */
			struct wlan_fast_connect_info fci = {0};
			wlan_read_fast_connect_info(&fci);
			if ((fci.ip_addr[0] != 0) && (fci.ip_addr[0] != 0xFF))
			{
				struct ipv4_config* n = &sta_ip_settings;
				n->addr_type = ADDR_TYPE_FAST_DHCP;
				os_memcpy((char *)&n->address, (char *)&fci.ip_addr, sizeof(fci.ip_addr));
				os_memcpy((char *)&n->netmask, (char *)&fci.netmask, sizeof(fci.netmask));
				os_memcpy((char *)&n->gw, (char *)&fci.gw, sizeof(fci.gw));
				os_memcpy((char *)&n->dns1, (char *)&fci.dns1, sizeof(fci.dns1));
				use_fast_dhcp = 1;
			}
		}
		if(use_fast_dhcp == 0)
			ip_address_set(1, DHCP_CLIENT, NULL, NULL, NULL, NULL);
	} else {
		IPStatusTypedef ipStatus;
		bk_err_t ret = kNoErr;
		os_memset(&ipStatus, 0x0, sizeof(IPStatusTypedef));
		ret = bk_wifi_get_ip_status(&ipStatus, BK_STATION);
		if (ret == kNoErr)
			ip_address_set(1, DHCP_DISABLE, ipStatus.ip, ipStatus.mask, ipStatus.gate, ipStatus.dns);
	}
}

void net_restart_dhcp(void)
{
	if (!sta_ip_is_start()) {
		LWIP_LOGI("bk wifi sta is not started or disconnected\r\n");
		return;
	}
	sta_ip_down();
	ip_address_set(BK_STATION, DHCP_CLIENT, NULL, NULL, NULL, NULL);
	sta_ip_start();
}

int net_configure_address(struct ipv4_config *addr, void *intrfc_handle)
{
	if (!intrfc_handle)
		return -1;

	struct iface *if_handle = (struct iface *)intrfc_handle;

	char *ip_type = NULL;
	if(addr->addr_type == ADDR_TYPE_DHCP)
		ip_type = "DHCP client";
	else if(addr->addr_type == ADDR_TYPE_STATIC)
		ip_type = "Static IP";
	else if(addr->addr_type == ADDR_TYPE_FAST_DHCP)
		ip_type = "Fast DHCP client";

	LWIP_LOGI("configuring iface %s (with %s)\n", if_handle->name, ip_type);
	netifapi_netif_set_down(&if_handle->netif);

	/* De-register previously registered DHCP Callback for correct
	 * address configuration.
	 */
	netif_set_status_callback(&if_handle->netif, NULL);

	switch (addr->addr_type) {
	case ADDR_TYPE_STATIC:
		ip_addr_set_ip4_u32(&if_handle->ipaddr, addr->address);
		ip_addr_set_ip4_u32(&if_handle->nmask, addr->netmask);
		ip_addr_set_ip4_u32(&if_handle->gw, addr->gw);

		netifapi_netif_set_addr(&if_handle->netif, ip_2_ip4(&if_handle->ipaddr),
								ip_2_ip4(&if_handle->nmask), ip_2_ip4(&if_handle->gw));

		if (if_handle == &g_mlan) {
			netif_set_status_callback(&if_handle->netif, wm_netif_status_static_callback);
			netifapi_netif_set_up(&if_handle->netif);
			net_configure_dns(if_handle, (struct wlan_ip_config *)addr);
#ifdef CONFIG_ETH
		} else if (if_handle == &g_eth) {
			netif_set_status_callback(&if_handle->netif, wm_netif_status_static_callback);
			netifapi_netif_set_up(&if_handle->netif);
			net_configure_dns(if_handle, (struct wlan_ip_config *)addr);
#endif
#if CONFIG_BRIDGE
		} else if (if_handle == &g_br) {
			netifapi_netif_set_default(net_get_br_handle());
#endif
		} else {
			/*AP never configure DNS server address!!!*/
			netifapi_netif_set_up(&if_handle->netif);
		}
		break;

	case ADDR_TYPE_DHCP:
	case ADDR_TYPE_FAST_DHCP:
		/* Reset the address since we might be transitioning from static to DHCP */
		memset(&if_handle->ipaddr, 0, sizeof(ip_addr_t));
		memset(&if_handle->nmask, 0, sizeof(ip_addr_t));
		memset(&if_handle->gw, 0, sizeof(ip_addr_t));
		netifapi_netif_set_addr(&if_handle->netif, ip_2_ip4(&if_handle->ipaddr),
				ip_2_ip4(&if_handle->nmask), ip_2_ip4(&if_handle->gw));

		netif_set_status_callback(&if_handle->netif, wm_netif_status_callback);
		netifapi_netif_set_up(&if_handle->netif);
		netifapi_dhcp_start(&if_handle->netif);
		break;

	default:
		break;
	}
	/* Finally this should send the following event. */
	if (if_handle == &g_mlan) {
		// static IP up;

		/* XXX For DHCP, the above event will only indicate that the
		 * DHCP address obtaining process has started. Once the DHCP
		 * address has been obtained, another event,
		 * WD_EVENT_NET_DHCP_CONFIG, should be sent to the wlcmgr.
		 */
		up_iface = 1;

		// we always set sta netif as the default.
		sta_set_default_netif();
#ifdef CONFIG_ETH
	} else if (if_handle == &g_eth) {
#endif
	} else {
		// softap IP up, start dhcp server;
		dhcp_server_start(net_get_uap_handle());
		ap_set_default_netif();
		up_iface = 0;
	}

	return 0;
}

struct ipv4_config* bk_wifi_get_sta_settings(void)
{
	return &sta_ip_settings;
}

int net_get_if_addr(struct wlan_ip_config *addr, void *intrfc_handle)
{
	const ip_addr_t *tmp;
	struct iface *if_handle = (struct iface *)intrfc_handle;

	if (netif_is_up(&if_handle->netif)) {
		if (if_handle == &g_mlan
#ifdef CONFIG_ETH
			|| if_handle == &g_eth
#endif
			) {
			/* STA or ETH Mode */
			addr->ipv4.address = ip_addr_get_ip4_u32(&if_handle->netif.ip_addr);
			addr->ipv4.netmask = ip_addr_get_ip4_u32(&if_handle->netif.netmask);
			addr->ipv4.gw = ip_addr_get_ip4_u32(&if_handle->netif.gw);

			tmp = dns_getserver(0);
			addr->ipv4.dns1 = ip_addr_get_ip4_u32(tmp);
			tmp = dns_getserver(1);
			addr->ipv4.dns2 = ip_addr_get_ip4_u32(tmp);
		} else {
			/* SoftAP Mode */
			addr->ipv4.address = ip_addr_get_ip4_u32(&if_handle->netif.ip_addr);
			addr->ipv4.netmask = ip_addr_get_ip4_u32(&if_handle->netif.netmask);
			addr->ipv4.gw = ip_addr_get_ip4_u32(&if_handle->netif.gw);
		}
	}

	return 0;
}

int net_get_if_macaddr(void *macaddr, void *intrfc_handle)
{
	struct iface *if_handle = (struct iface *)intrfc_handle;

	os_memcpy(macaddr, &if_handle->netif.hwaddr[0], if_handle->netif.hwaddr_len);

	return 0;
}

#ifdef CONFIG_IPV6
int net_get_if_ipv6_addr(struct wlan_ip_config *addr, void *intrfc_handle)
{
	struct iface *if_handle = (struct iface *)intrfc_handle;
	int i;

	for (i = 0; i < MAX_IPV6_ADDRESSES; i++) {
		memcpy(&addr->ipv6[i].address,
			   ip_2_ip6(&(if_handle->netif.ip6_addr[i])), 16);
		addr->ipv6[i].addr_state = if_handle->netif.ip6_addr_state[i];
	}
	/* TODO carry out more processing based on IPv6 fields in netif */
	return 0;
}

int net_get_if_ipv6_pref_addr(struct wlan_ip_config *addr, void *intrfc_handle)
{
	int i, ret = 0;
	struct iface *if_handle = (struct iface *)intrfc_handle;

	for (i = 0; i < MAX_IPV6_ADDRESSES; i++) {
		if (if_handle->netif.ip6_addr_state[i] == IP6_ADDR_PREFERRED) {
			memcpy(&addr->ipv6[ret++].address,
				   &(if_handle->netif.ip6_addr[i]), 16);
		}
	}
	return ret;
}
#endif /* CONFIG_IPV6 */

int net_get_if_ip_addr(uint32_t *ip, void *intrfc_handle)
{
	struct iface *if_handle = (struct iface *)intrfc_handle;

	*ip = ip_addr_get_ip4_u32(&if_handle->netif.ip_addr);
	return 0;
}

int net_get_if_gw_addr(uint32_t *ip, void *intrfc_handle)
{
	struct iface *if_handle = (struct iface *)intrfc_handle;

	*ip = ip_addr_get_ip4_u32(&if_handle->netif.gw);

	return 0;
}

int net_get_if_ip_mask(uint32_t *nm, void *intrfc_handle)
{
	struct iface *if_handle = (struct iface *)intrfc_handle;

	*nm = ip_addr_get_ip4_u32(&if_handle->netif.netmask);
	return 0;
}

void net_configure_dns(struct iface *if_handle, struct wlan_ip_config *ip)
{
	ip_addr_t tmp;

	if (ip->ipv4.addr_type == ADDR_TYPE_STATIC) {
		if (ip->ipv4.dns1 == 0) {
			if (if_handle == &g_mlan && bk_feature_fast_dhcp_enable()) {
#ifdef CONFIG_WIFI_ENABLE
				struct wlan_fast_connect_info fci = {0};
				wlan_read_fast_connect_info(&fci);
				os_memcpy((char *)&ip->ipv4.dns1, (char *)&fci.dns1, sizeof(fci.dns1));
#endif
			} else {
				ip->ipv4.dns1 = ip->ipv4.gw;
			}
		}
		if (ip->ipv4.dns2 == 0)
			ip->ipv4.dns2 = ip->ipv4.dns1;

		ip_addr_set_ip4_u32(&tmp, ip->ipv4.dns1);
		dns_setserver(0, &tmp);
		ip_addr_set_ip4_u32(&tmp, ip->ipv4.dns2);
		dns_setserver(1, &tmp);
	}

	/* DNS MAX Retries should be configured in lwip/dns.c to 3/4 */
	/* DNS Cache size of about 4 is sufficient */
}

void net_wlan_initial(void)
{
	net_ipv4stack_init();

#ifdef CONFIG_IPV6
	net_ipv6stack_init(&g_mlan.netif);
#endif /* CONFIG_IPV6 */

#if CONFIG_MDNS
	mdns_resp_init();
#endif
}

int net_wlan_add_netif(uint8_t *mac)
{
	struct iface *wlan_if = NULL;
	netif_if_t netif_if;
	void *vif = NULL;
	int vifid = 0;
	err_t err;

	vifid = wifi_netif_mac_to_vifid(mac);
	vif = wifi_netif_mac_to_vif(mac);
	netif_if = wifi_netif_vif_to_netif_type(vif);
	if (netif_if == NETIF_IF_AP) {
		wlan_if = &g_uap;
	} else if (netif_if == NETIF_IF_STA) {
		wlan_if = &g_mlan;
	} else {
		LWIP_LOGE("unknown netif(%d)\n", netif_if);
		return ERR_ARG;
	}

	ip_addr_set_ip4_u32(&wlan_if->ipaddr, INADDR_ANY);
	err = netifapi_netif_add(&wlan_if->netif,
		ip_2_ip4(&wlan_if->ipaddr),
		ip_2_ip4(&wlan_if->ipaddr),
		ip_2_ip4(&wlan_if->ipaddr),
		vif,
		wlanif_init,
		tcpip_input);
	if (err) {
		LWIP_LOGE("net_wlan_add_netif failed(%d)\n", err);
		return err;
	} else {
		wifi_netif_set_vif_private_data(vif, &wlan_if->netif);
#if CONFIG_IPV6
		if (netif_if == NETIF_IF_STA) {
			netif_create_ip6_linklocal_address(&wlan_if->netif, 1);
			netif_set_ip6_autoconfig_enabled(&wlan_if->netif, 1);
		}
#endif
	}

	LWIP_LOGD("add vif%d\n", vifid);
	return ERR_OK;
}

int net_wlan_remove_netif(uint8_t *mac)
{
	struct netif *netif = NULL;
	void *vif;
	err_t err;
	int vifid;

	vifid = wifi_netif_mac_to_vifid(mac);
	vif = wifi_netif_mac_to_vif(mac);
	if (!vif) {
		LWIP_LOGE("remove netif, vif%d not found\n", vifid);
		return ERR_ARG;
	}

	netif = (struct netif *)wifi_netif_get_vif_private_data(vif);
	if (!netif) {
		LWIP_LOGE("remove netif, netif is null\n");
		return ERR_ARG;
	}

	err = netifapi_netif_remove(netif);
	if (err != ERR_OK) {
		LWIP_LOGE("remove netif, failed(%d)\n", err);
		return err;
	} else {
		netif->state = NULL;
	}

	LWIP_LOGI("remove vif%d\n", vifid);
	return ERR_OK;
}

#if CONFIG_WIFI6_CODE_STACK
bool etharp_tmr_flag = false;
bool g_bk_ap_connected = false;
void net_begin_send_arp_reply(bool is_send_arp, bool is_allow_send_req)
{
	// If connected to beken repeater, don't send arp response
	if (g_bk_ap_connected)
		return;

	//send reply
	if (is_send_arp && !is_allow_send_req) {
		etharp_tmr_flag = true;
		LWIP_LOGI("send reply %s\n", __func__);
	}
	//stop send reply
	if (!is_send_arp && is_allow_send_req) {
		etharp_tmr_flag = false;
		LWIP_LOGI("stop send reply %s\n", __func__);
		return;
	}
	etharp_reply();
}
#endif


#ifdef CONFIG_ETH
void *net_get_eth_handle(void)
{
	return &g_eth.netif;
}

int net_eth_add_netif(uint8_t *mac)
{
	struct iface *eth_if = &g_eth;
	err_t err;

	ip_addr_set_ip4_u32(&eth_if->ipaddr, INADDR_ANY);
	err = netifapi_netif_add(&eth_if->netif,
		ip_2_ip4(&eth_if->ipaddr),
		ip_2_ip4(&eth_if->ipaddr),
		ip_2_ip4(&eth_if->ipaddr),
		NULL,
		ethernetif_init,
		tcpip_input);

	if (err) {
		LWIP_LOGE("net_wlan_add_netif failed(%d)\n", err);
		return err;
	}

	/* disable SW checksum calculation */
	NETIF_SET_CHECKSUM_CTRL(&eth_if->netif, NETIF_CHECKSUM_DISABLE_ALL);

	return ERR_OK;
}

int net_eth_remove_netif(void)
{
	err_t err = netifapi_netif_remove(&g_eth.netif);

	if (err != ERR_OK) {
		LWIP_LOGE("remove netif, failed(%d)\n", err);
		return err;
	}

	return ERR_OK;
}

#if LWIP_NETIF_LINK_CALLBACK
/**
 * @brief  Notify the User about the network iface config status
 * @param  netif: the network iface
 * @retval None
 */
static void ethernet_link_status_updated(struct netif *netif)
{
	LWIP_LOGI("%s netif->flags 0x%x\n", __func__, netif->flags);

	if (netif_is_up(netif)) {
	} else {
		/* netif is down */
	}
}
#endif

int net_eth_start()
{
	int ret;
	uint8_t mac[BK_MAC_ADDR_LEN];

	miiphy_init();

	ieee8023_phy_init();

	// Init TCP/IP Stack
	net_ipv4stack_init();

	// Get ETH MAC address
	bk_get_mac(mac, MAC_TYPE_ETH);

	// Add netif
	ret = net_eth_add_netif(mac);
	if (ret) {
		return ret;
	}

	/* Registers the default network iface */
	netifapi_netif_set_default(&g_eth.netif);

	// Lock when accessing netif link functions
	LOCK_TCPIP_CORE();

	if (netif_is_link_up(&g_eth.netif)) {
		/* When the netif is fully configured this function must be called */
		netif_set_up(&g_eth.netif);
	} else {
		/* When the netif link is down this function must be called */
		netif_set_down(&g_eth.netif);
	}

#if LWIP_NETIF_LINK_CALLBACK
	/* Set the link callback function, this function is called on change of link status*/
	netif_set_link_callback(&g_eth.netif, ethernet_link_status_updated);
#endif

	UNLOCK_TCPIP_CORE();

	if (rtos_create_thread(NULL, BEKEN_APPLICATION_PRIORITY, "eth_link",
						   ethernet_link_thread, 0x1000, &g_eth.netif))
		LWIP_LOGE("Create eth link thread failed\n");

	return 0;
}

void eth_ip_start(void)
{
	struct wlan_ip_config address = { 0 };

	if (!eth_ip_start_flag) {
		LWIP_LOGI("eth ip start\r\n");
		eth_ip_start_flag = true;
		net_configure_address(&eth_ip_settings, net_get_eth_handle());
		return;
	}

	net_get_if_addr(&address, net_get_eth_handle());
	LWIP_LOGI("eth ip start: %pIn\n", &address.ipv4.address);
}

void eth_ip_down(void)
{
	if (eth_ip_start_flag) {
		LWIP_LOGI("eth ip down\n");

		eth_ip_start_flag = false;

		netifapi_netif_set_link_down(&g_eth.netif);
		netifapi_netif_set_down(&g_eth.netif);
		netif_set_status_callback(&g_eth.netif, NULL);
		netifapi_dhcp_stop(&g_eth.netif);
#if LWIP_IPV6
		for (u8_t addr_idx = 1; addr_idx < LWIP_IPV6_NUM_ADDRESSES; addr_idx++) {
			netif_ip6_addr_set(&g_eth.netif, addr_idx, (const ip6_addr_t *)IP6_ADDR_ANY);
			g_eth.netif.ip6_addr_state[addr_idx] = IP6_ADDR_INVALID;
		}
#endif
	}
}
#endif

