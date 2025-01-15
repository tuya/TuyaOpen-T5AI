/**
 * @file
 * lwIP netif implementing an IEEE 802.1D MAC Bridge
 */

/*
 * Copyright (c) 2017 Simon Goldschmidt.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 *
 */

/**
 * @defgroup bridgeif IEEE 802.1D bridge
 * @ingroup netifs
 * This file implements an IEEE 802.1D bridge by using a multilayer netif approach
 * (one hardware-independent netif for the bridge that uses hardware netifs for its ports).
 * On transmit, the bridge selects the outgoing port(s).
 * On receive, the port netif calls into the bridge (via its netif->input function) and
 * the bridge selects the port(s) (and/or its netif->input function) to pass the received pbuf to.
 *
 * Usage:
 * - add the port netifs just like you would when using them as dedicated netif without a bridge
 *   - only NETIF_FLAG_ETHARP/NETIF_FLAG_ETHERNET netifs are supported as bridge ports
 *   - add the bridge port netifs without IPv4 addresses (i.e. pass 'NULL, NULL, NULL')
 *   - don't add IPv6 addresses to the port netifs!
 * - set up the bridge configuration in a global variable of type 'bridgeif_initdata_t' that contains
 *   - the MAC address of the bridge
 *   - some configuration options controlling the memory consumption (maximum number of ports
 *     and FDB entries)
 *   - e.g. for a bridge MAC address 00-01-02-03-04-05, 2 bridge ports, 1024 FDB entries + 16 static MAC entries:
 *     bridgeif_initdata_t mybridge_initdata = BRIDGEIF_INITDATA1(2, 1024, 16, ETH_ADDR(0, 1, 2, 3, 4, 5));
 * - add the bridge netif (with IPv4 config):
 *   struct netif bridge_netif;
 *   netif_add(&bridge_netif, &my_ip, &my_netmask, &my_gw, &mybridge_initdata, bridgeif_init, tcpip_input);
 *   NOTE: the passed 'input' function depends on BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT setting,
 *         which controls where the forwarding is done (netif low level input context vs. tcpip_thread)
 * - set up all ports netifs and the bridge netif
 *
 * - When adding a port netif, NETIF_FLAG_ETHARP flag will be removed from a port
 *   to prevent ETHARP working on that port netif (we only want one IP per bridge not per port).
 * - When adding a port netif, its input function is changed to call into the bridge.
 *
 *
 * @todo:
 * - compact static FDB entries (instead of walking the whole array)
 * - add FDB query/read access
 * - add FDB change callback (when learning or dropping auto-learned entries)
 * - prefill FDB with MAC classes that should never be forwarded
 * - multicast snooping? (and only forward group addresses to interested ports)
 * - support removing ports
 * - check SNMP integration
 * - VLAN handling / trunk ports
 * - priority handling? (although that largely depends on TX queue limitations and lwIP doesn't provide tx-done handling)
 */

#include "netif/bridgeif.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/snmp.h"
#include "lwip/timeouts.h"
#include <string.h>
#if BK_LWIP
#include "lwip/prot/iana.h"
#include "lwip/prot/dhcp.h"
#include "lwip/prot/icmp.h"
#include "bk_list.h"
#include "bk_wifi_types.h"
#include "common/bk_generic.h"
#include "../../dhcpd/dhcp-bootp.h"
#include "os/os.h"
#include "os/mem.h"

#include "lwip/udp.h"
#include "netif/ethernet.h"
#include "net.h"
#include "lwip/etharp.h"
#include <bk_private/bk_rw.h>
#include "dhcp.h"

/*0:disabled 1:enabled*/
uint8 bridge_is_enabled = 0;
extern void *net_get_sta_handle(void);
extern void *net_get_uap_handle(void);

/*
 * If IP_FORWARD is not enabled, packets that received from STA with different IP
 * with br0 will modify destination mac address.
 * if IP_FORWARD is enabled, packets will be routed by ip_forward function.
 */
#if IP_FORWARD && IP_FORWARD_ALLOW_TX_ON_RX_NETIF
#define BRIDGE_MODIFY_DST_ADDR 0
#else
#define BRIDGE_MODIFY_DST_ADDR 1
#endif
#define etharp_ip4_addr_cmp(addr1, addr2)    ((addr1).addrw[0] == (addr2).addrw[0] && (addr1).addrw[1] == (addr2).addrw[1])
#define etharp_ip4_addr_isany_val(addr) (addr.addrw[0] == 0 && addr.addrw[1] == 0)
#define BK_LWIP_FAST_CSUM_MOD  1
#endif // BK_LWIP

#if LWIP_NUM_NETIF_CLIENT_DATA

/* Define those to better describe your network interface. */
#define IFNAME0 'b'
#define IFNAME1 'r'

struct bridgeif_private_s;

/* netif data index to get the bridge on input */
u8_t bridgeif_netif_client_id = 0xff;

/**
 * @ingroup bridgeif
 * Add a static entry to the forwarding database.
 * A static entry marks where frames to a specific eth address (unicast or group address) are
 * forwarded.
 * bits [0..(BRIDGEIF_MAX_PORTS-1)]: hw ports
 * bit [BRIDGEIF_MAX_PORTS]: cpu port
 * 0: drop
 */
err_t
bridgeif_fdb_add(struct netif *bridgeif, const struct eth_addr *addr, bridgeif_portmask_t ports)
{
  int i;
  bridgeif_private_t *br;
  BRIDGEIF_DECL_PROTECT(lev);
  LWIP_ASSERT("invalid netif", bridgeif != NULL);
  br = (bridgeif_private_t *)bridgeif->state;
  LWIP_ASSERT("invalid state", br != NULL);

  BRIDGEIF_READ_PROTECT(lev);
  for (i = 0; i < br->max_fdbs_entries; i++) {
    if (!br->fdbs[i].used) {
      BRIDGEIF_WRITE_PROTECT(lev);
      if (!br->fdbs[i].used) {
        br->fdbs[i].used = 1;
        br->fdbs[i].dst_ports = ports;
        memcpy(&br->fdbs[i].addr, addr, sizeof(struct eth_addr));
        BRIDGEIF_WRITE_UNPROTECT(lev);
        BRIDGEIF_READ_UNPROTECT(lev);
        return ERR_OK;
      }
      BRIDGEIF_WRITE_UNPROTECT(lev);
    }
  }
  BRIDGEIF_READ_UNPROTECT(lev);
  return ERR_MEM;
}

/**
 * @ingroup bridgeif
 * Remove a static entry from the forwarding database
 */
err_t
bridgeif_fdb_remove(struct netif *bridgeif, const struct eth_addr *addr)
{
  int i;
  bridgeif_private_t *br;
  BRIDGEIF_DECL_PROTECT(lev);
  LWIP_ASSERT("invalid netif", bridgeif != NULL);
  br = (bridgeif_private_t *)bridgeif->state;
  LWIP_ASSERT("invalid state", br != NULL);

  BRIDGEIF_READ_PROTECT(lev);
  for (i = 0; i < br->max_fdbs_entries; i++) {
    if (br->fdbs[i].used && !memcmp(&br->fdbs[i].addr, addr, sizeof(struct eth_addr))) {
      BRIDGEIF_WRITE_PROTECT(lev);
      if (br->fdbs[i].used && !memcmp(&br->fdbs[i].addr, addr, sizeof(struct eth_addr))) {
        memset(&br->fdbs[i], 0, sizeof(bridgeif_fdb_static_entry_t));
        BRIDGEIF_WRITE_UNPROTECT(lev);
        BRIDGEIF_READ_UNPROTECT(lev);
        return ERR_OK;
      }
      BRIDGEIF_WRITE_UNPROTECT(lev);
    }
  }
  BRIDGEIF_READ_UNPROTECT(lev);
  return ERR_VAL;
}

/** Get the forwarding port(s) (as bit mask) for the specified destination mac address */
static bridgeif_portmask_t
bridgeif_find_dst_ports(bridgeif_private_t *br, struct eth_addr *dst_addr)
{
  int i;
  BRIDGEIF_DECL_PROTECT(lev);
  BRIDGEIF_READ_PROTECT(lev);
  /* first check for static entries */
  for (i = 0; i < br->max_fdbs_entries; i++) {
    if (br->fdbs[i].used) {
      if (!memcmp(&br->fdbs[i].addr, dst_addr, sizeof(struct eth_addr))) {
        bridgeif_portmask_t ret = br->fdbs[i].dst_ports;
        BRIDGEIF_READ_UNPROTECT(lev);
        return ret;
      }
    }
  }
  if (dst_addr->addr[0] & 1) {
    /* no match found: flood remaining group address */
    BRIDGEIF_READ_UNPROTECT(lev);
    return BR_FLOOD;
  }
  BRIDGEIF_READ_UNPROTECT(lev);
  /* no match found: check dynamic fdb for port or fall back to flooding */
  return bridgeif_fdb_get_dst_ports(br->fdbd, dst_addr);
}

/** Helper function to see if a destination mac belongs to the bridge
 * (bridge netif or one of the port netifs), in which case the frame
 * is sent to the cpu only.
 */
static int
bridgeif_is_local_mac(bridgeif_private_t *br, struct eth_addr *addr)
{
  int i;
  BRIDGEIF_DECL_PROTECT(lev);
  if (!memcmp(br->netif->hwaddr, addr, sizeof(struct eth_addr))) {
    return 1;
  }
  BRIDGEIF_READ_PROTECT(lev);
  for (i = 0; i < br->num_ports; i++) {
    struct netif *portif = br->ports[i].port_netif;
    if (portif != NULL) {
      if (!memcmp(portif->hwaddr, addr, sizeof(struct eth_addr))) {
        BRIDGEIF_READ_UNPROTECT(lev);
        return 1;
      }
    }
  }
  BRIDGEIF_READ_UNPROTECT(lev);
  return 0;
}

#if BK_LWIP
/*implemented according with RFC 1624,modified the algorithm from RFC 1071 and 1141*/
/* Returns the new checksum for a packet in which the checksum field previously
 * contained 'old_csum' and in which a field that contained 'old_u16' was
 * changed to contain 'new_u16'. */
static uint16_t recalc_csum16(uint16_t old_csum, uint16_t old_u16, uint16_t new_u16)
{
   uint32_t csum = (~old_csum & 0xFFFF) + (~old_u16 & 0xFFFF) + new_u16 ;
   csum = (csum >> 16) + (csum & 0xFFFF);
   csum += csum >> 16;
   return ~csum;
}

/* Returns the new checksum for a packet in which the checksum field previously
 * contained 'old_csum' and in which a field that contained 'old_u32' was
 * changed to contain 'new_u32'. */
static uint16_t recalc_csum32(uint16_t old_csum, uint32_t old_u32, uint32_t new_u32)
{
    return recalc_csum16(recalc_csum16(old_csum, old_u32, new_u32),
                         old_u32 >> 16, new_u32 >> 16);
}

static void etharp_remove_static_entry_by_mac(uint8_t *mac);

void exchange_arp_sender(struct netif *netif, struct pbuf *pd)
{
  unsigned short protocol;
  unsigned char *data=NULL;

  data = pd->payload;
  protocol = *((unsigned short *)(data + 2 * 6));

  if(protocol == lwip_htons(ETHTYPE_ARP)) {
    struct etharp_hdr *arp = (struct etharp_hdr *)(data + 14);
    unsigned char *arp_ptr = (unsigned char *)(arp);
    memcpy(arp_ptr+8, netif->hwaddr, 6);
  }
}
static inline bool netif_is_upstream(struct netif *netif)
{
  return netif == net_get_sta_handle();
}

/**
 * Send a raw ARP packet (opcode and all addresses can be modified)
 *
 * @param netif the lwip network interface on which to send the ARP packet
 * @param rx_if_idx don't send packet to this interface
 * @param ethsrc_addr the source MAC address for the ethernet header
 * @param ethdst_addr the destination MAC address for the ethernet header
 * @param hwsrc_addr the source MAC address for the ARP protocol header
 * @param ipsrc_addr the source IP address for the ARP protocol header
 * @param hwdst_addr the destination MAC address for the ARP protocol header
 * @param ipdst_addr the destination IP address for the ARP protocol header
 * @param opcode the type of the ARP packet
 * @return ERR_OK if the ARP packet has been sent
 *         ERR_MEM if the ARP packet couldn't be allocated
 *         any other err_t on failure
 */
static err_t etharp_raw2(struct netif *netif, u8_t rx_if_idx,
          const struct eth_addr *ethsrc_addr, const struct eth_addr *ethdst_addr,
          const struct eth_addr *hwsrc_addr, const ip4_addr_t *ipsrc_addr,
          const struct eth_addr *hwdst_addr, const ip4_addr_t *ipdst_addr,
          const u16_t opcode,
          u8 flag)
{
  struct pbuf *p;
  err_t result = ERR_OK;
  struct etharp_hdr *hdr;

  LWIP_ASSERT("netif != NULL", netif != NULL);

  /* allocate a pbuf for the outgoing ARP request packet */
  p = pbuf_alloc(PBUF_LINK, SIZEOF_ETHARP_HDR, PBUF_RAM);
  /* could allocate a pbuf for an ARP request? */
  if (p == NULL)
    return ERR_MEM;

  hdr = (struct etharp_hdr *)p->payload;
  hdr->opcode = lwip_htons(opcode);

  /* Write the ARP MAC-Addresses */
  memcpy(&hdr->shwaddr, hwsrc_addr, 6);
  memcpy(&hdr->dhwaddr, hwdst_addr, 6);
  /* Copy struct ip4_addr2 to aligned ip4_addr, to support compilers without
   * structure packing. */
  memcpy(&hdr->sipaddr, ipsrc_addr, sizeof(ip4_addr_t));
  memcpy(&hdr->dipaddr, ipdst_addr, sizeof(ip4_addr_t));

  hdr->hwtype = PP_HTONS(LWIP_IANA_HWTYPE_ETHERNET);
  hdr->proto = PP_HTONS(ETHTYPE_IP);
  /* set hwlen and protolen */
  hdr->hwlen = ETH_HWADDR_LEN;
  hdr->protolen = sizeof(ip4_addr_t);

  p->elfags = flag;

  /* send ARP query */
#if LWIP_AUTOIP
  /* If we are using Link-Local, all ARP packets that contain a Link-Local
   * 'sender IP address' MUST be sent using link-layer broadcast instead of
   * link-layer unicast. (See RFC3927 Section 2.5, last paragraph) */
  if (ip4_addr_islinklocal(ipsrc_addr))
    ethernet_output(netif, p, ethsrc_addr, &ethbroadcast, ETHTYPE_ARP);
  else
#endif /* LWIP_AUTOIP */
  ethernet_output(netif, p, ethsrc_addr, ethdst_addr, ETHTYPE_ARP);

  //ETHARP_STATS_INC(etharp.xmit);
  /* free ARP query packet */
  pbuf_free(p);
  p = NULL;
  /* could not allocate pbuf for ARP request */

  return result;
}

int send_gratuitous_arp_downstream(bridgeif_private_t *br, struct netif *rx_if, struct eth_addr *hwaddr, ip4_addr_t *ipaddr)
{
  etharp_raw2(br->netif, netif_get_index(rx_if),
         (struct eth_addr *)br->netif->hwaddr, &ethbroadcast,
         (struct eth_addr *)hwaddr, ipaddr,
         &ethzero, ipaddr,
         ARP_REQUEST, 1);
  /*
   * Gratuitous ARP comes in two flavours, request and reply.
   * Some operating systems only accept request, some only reply.
   * Let's just send both...
   */
  //TODO
  return 0;
}

static void arp_request_handle_reply(bridgeif_private_t *br, struct netif *netif, ip4_addr_t *dip, uint8_t *dhwaddr);
extern int bridge_sap_port;
extern int bridge_sta_port;
extern err_t etharp_update_arp_entry(struct netif *netif, const ip4_addr_t *ipaddr, struct eth_addr *ethaddr, u8_t flags, s16_t *loc);
static const char *dhcp_msg_str[] = {
  "DHCP_NONE",
  "DHCP_DISCOVER",
  "DHCP_OFFER",
  "DHCP_REQUEST",
  "DHCP_DECLINE",
  "DHCP_ACK",
  "DHCP_NAK",
  "DHCP_RELEASE",
  "DHCP_INFORM"
};

#define BK_BRIDGE_DHCP_MIN_LEASE_TIME    600

/* handle DHCP packets from STA interface */
static int bk_bridge_upstream_handle_dhcp(struct netif *rx_if, struct udp_hdr *udphdr, ip4_addr_t *dipaddr, struct dhcp_msg *msg, int len)
{
  uint8_t *client_mac = msg->chaddr;
  ip4_addr_t youripaddr;
  struct bootp_option *opt = (struct bootp_option *)msg->options;
  int consumed;
  const char *msgstr = "DHCP";
  int dhcp_msg_type = -1;

  len -= offsetof(struct dhcp_msg, options);

  memcpy(&youripaddr, &msg->yiaddr, sizeof(ip4_addr_t));

  // If this dhcp message is not ours
  if (memcmp(client_mac, rx_if->hwaddr, ETH_ALEN)) {
    while (len > 0 && opt->type != BOOTP_END_OPTION) {

      switch (opt->type) {
      case BOOTP_OPTION_DHCP_MESSAGE:
        if (opt->length == 1) {
          dhcp_msg_type = opt->value[0];
          if (dhcp_msg_type < ARRAY_SIZE(dhcp_msg_str))
            msgstr = dhcp_msg_str[(uint8_t)dhcp_msg_type];
        }
        break;
      case BOOTP_OPTION_ADDRESS_TIME:
        if (opt->length == 4) {
          uint32_t lease_time = ((uint8_t)opt->value[0] << 24) | ((uint8_t)opt->value[1] << 16) | ((uint8_t)opt->value[2] << 8) | ((uint8_t)opt->value[3]);
          LWIP_LOGI("Router's DHCP lease_time %d\n", lease_time);
          if (lease_time < BK_BRIDGE_DHCP_MIN_LEASE_TIME) {
            uint32_t old_value = lease_time;
            lease_time = BK_BRIDGE_DHCP_MIN_LEASE_TIME;
            opt->value[0] = (lease_time >> 24) & 0xFF;
            opt->value[1] = (lease_time >> 16) & 0xFF;
            opt->value[2] = (lease_time >> 8) & 0xFF;
            opt->value[3] = lease_time & 0xFF;

            // recalcuate checksum
            udphdr->chksum = recalc_csum32(udphdr->chksum, old_value, lease_time);
          }
        }
        break;
      }

      /* look at the next option (if any) */
      consumed = sizeof(struct bootp_option) + opt->length;
      len -= consumed;
      opt = (struct bootp_option *)((char *)opt + consumed);
    }
    LWIP_LOGI("%s client %pm, our %pm, yiaddr %pIn\n", msgstr, client_mac, rx_if->hwaddr, &youripaddr);
    if (dhcp_msg_type == DHCP_ACK && rwm_mgmt_sta_mac2ptr(client_mac)) {
      // remove first and add
      etharp_remove_static_entry_by_mac(client_mac);
      int err = etharp_add_static_entry(&youripaddr, (struct eth_addr *)client_mac);
      if (err < ERR_OK)
        LWIP_LOGE("fail add static entry: %pm, our %pm, yiaddr %pIn err:%d\n", client_mac, rx_if->hwaddr, &youripaddr, err);
    }
  } else {
    LWIP_LOGD("%s: %pm, our %pm, yiaddr: %pIn\n", msgstr, client_mac, rx_if->hwaddr, &youripaddr);
  }

  return 0;
}

/**
 * @payload: l4 payload
 * return: 1: drop packet, 0: continue
 */
static int bk_bridge_upstream_recv_filter_l3(bridgeif_private_t *br, struct pbuf *p,
              uint16_t type, uint8_t proto, struct ip_hdr *iphdr, void *payload)
{
  if (proto == IP_PROTO_UDP) {
    struct udp_hdr *udphdr = payload;
    uint16_t src, dst;

    /* convert src and dest ports to host byte order */
    src = lwip_ntohs(udphdr->src);
    dst = lwip_ntohs(udphdr->dest);

    if ((IP_MULTICAST(htonl(iphdr->dest.addr)))) {
      // * netbios-dgm NETBIOS Datagram Service
      if (src == 138 && dst == 138) {
        LWIP_LOGI("drop NBDS pkt\n");
        return 1;
      }
      // * NETBIOS Name Service
      if (dst == 137) {
        LWIP_LOGI("drop NBNS pkt\n");
        return 1;
      }
      // * SSDP discover
      if (dst == 1900) {
        LWIP_LOGI("drop SSDP pkt\n");
        return 1;
      }
      // * Add other filters here
    }
  } else if (proto == IP_PROTO_IGMP) {
    // drop IGMP packets
    LWIP_LOGI("drop IGMP pkt\n");
    return 1;
  } else if (proto == IP_PROTO_ICMP) {
    // drop ICMP Request packets that not directed to us
    struct icmp_echo_hdr *iecho = payload;

    if (iecho->type == ICMP_ECHO && memcmp(&br->netif->ip_addr, &iphdr->dest, 4)) {
      LWIP_LOGI("Drop ICMP Request for %pIn, not for us\n", &iphdr->dest);
      return 1;
    }
  }
  return 0;
}

static int bk_bridge_upstream_recv_filter_l2(struct pbuf *p, uint16_t type, struct eth_hdr *ethhdr)
{
#if 0 // ethernetif_input function already does this filter

  // drop lldp packet
  if (type == PP_HTONS(ETHTYPE_LLDP)) {
    LWIP_LOGI("drop LLDP pkt\n");
    return 1;
  }

  // drop ipv6 packet
  if (type == PP_HTONS(ETHTYPE_IPV6)) {
    LWIP_LOGI("drop IPV6 pkt\n");
    return 1;
  }

  if (PP_NTOHS(type) < 0x0800) {
    // RFC1042 LLC SNAP Format
    // FIXME: here we drop rfc1042 ethernet packet
    // drop stp packet
    LWIP_LOGI("drop rfc1042 pkt\n");
    return 1;
  }
#endif

  return 0;
}

/*
 * return: 1: eaten, 2: dont free 0: continue
 */
static int bk_bridge_upstream_if_recv(bridgeif_private_t *br, struct pbuf *p, struct netif *rx_if)
{
  struct eth_hdr *ethhdr;
  u16_t type;

  if (!netif_is_upstream(rx_if))  //sta0
    return 0;

  if (p->len <= SIZEOF_ETH_HDR)
    return 0;

  /* etharp_gratuitous, send_gratuitous_arp, etharp_raw2
   * ethernet_input */
  ethhdr = p->payload;
  type = ethhdr->type;

  if (type == PP_HTONS(ETHTYPE_ARP)) {
    /*
     *  Process ARP packets
     *  a: ARP Request
     *    If we found an arp entry in our arp_table, send ARP response with mac=ourmac, ip=node'sip,
     *    and send an gratious arp to all nodes in out mesh network.
     *    If not found, build arp request to all nodes in our mesh network.
     *   b: ARP Response
     *    update arp_table, may be optimized(send arp response immediately, or send gratioius arp).
     */
    struct etharp_hdr *arphdr = (struct etharp_hdr *)(ethhdr + 1);
    bool is_probe = false;
    bool is_gratious = false;
    bool is_announcement __maybe_unused = false;
    bool is_local __maybe_unused = false;

    /* RFC 826 "Packet Reception": */
    if ((arphdr->hwtype != PP_HTONS(LWIP_IANA_HWTYPE_ETHERNET)) ||
      (arphdr->hwlen != ETH_HWADDR_LEN) ||
      (arphdr->protolen != sizeof(ip4_addr_t)) ||
      (arphdr->proto != PP_HTONS(ETHTYPE_IP)))
      return 1;

    /* from wireshark */
    /* ARP requests/replies with the same sender and target protocol
     address are flagged as "gratuitous ARPs", i.e. ARPs sent out as,
     in effect, an announcement that the machine has MAC address
     XX:XX:XX:XX:XX:XX and IPv4 address YY.YY.YY.YY. Requests are to
     provoke complaints if some other machine has the same IPv4 address,
     replies are used to announce relocation of network address, like
     in failover solutions. */
     if ((arphdr->opcode == PP_HTONS(ARP_REQUEST) || arphdr->opcode == PP_HTONS(ARP_REPLY)) &&
         etharp_ip4_addr_cmp(arphdr->sipaddr, arphdr->dipaddr)) {
      is_gratious = true;
      if (arphdr->opcode == PP_HTONS(ARP_REQUEST) &&
          !memcmp(&arphdr->dhwaddr, &ethzero, ETH_HWADDR_LEN))
        is_announcement = true;
    } else {
      is_gratious = false;
      if (arphdr->opcode == PP_HTONS(ARP_REQUEST) &&
          !memcmp(&arphdr->dhwaddr, &ethzero, ETH_HWADDR_LEN) &&
          etharp_ip4_addr_isany_val(arphdr->sipaddr))
        is_probe = true;
    }

    // os_printf("ARP %s, shw %pm, sip %pIn, dhw %pm, dip %pIn\n", arphdr->opcode == PP_HTONS(ARP_REQUEST) ? "Req" : "Rsp",
    //  &arphdr->shwaddr, &arphdr->sipaddr, &arphdr->dhwaddr, &arphdr->dipaddr);
    // os_printf("recv: opcode %d, local %d, is_gratious %d, is_announcement %d, is_probe %d\n",
    //  PP_NTOHS(arphdr->opcode), is_local, is_gratious, is_announcement, is_probe);

    if (arphdr->opcode == PP_HTONS(ARP_REQUEST)) {
      const ip4_addr_t *unused_ipaddr;
      struct eth_addr *dst_ethaddr;
      u8_t for_us;
      ip4_addr_t sipaddr, dipaddr;

      /* Copy struct ip4_addr2 to aligned ip4_addr, to support compilers without
       * structure packing (not using structure copy which breaks strict-aliasing rules). */
      memcpy(&sipaddr, &arphdr->sipaddr, sizeof(ip4_addr_t));
      memcpy(&dipaddr, &arphdr->dipaddr, sizeof(ip4_addr_t));

      /* send gratious ARP, mark the pbuf inport with upstream, so packet will not forward to extAP.*/
      //send_gratuitous_arp_downstream(br, rx_if, &sipaddr);

      /* this interface is not configured? */
      if (ip4_addr_isany_val(*netif_ip4_addr(br->netif))) {
        for_us = 0;
      } else {
        /* ARP packet directed to us? */
        for_us = (u8_t)ip4_addr_cmp(&dipaddr, netif_ip4_addr(br->netif));
      }

      if (for_us) {
        /* is this for one of the local ports? */
        /* yes, send to cpu port only */
        LWIP_DEBUGF(BRIDGEIF_FW_DEBUG, ("for_us: br -> input(%p) local\n", (void *)p));
        br->netif->input(p, br->netif);  // lwip arp module will update arptable and send ARP REPLY
        return 2;
      }

      if (is_gratious || is_probe)
        // ignore here, simply flood
        return 1;

      /* for other hosts */
      if (etharp_find_addr(NULL, &dipaddr, &dst_ethaddr, &unused_ipaddr) >= 0) {
        bridgeif_portmask_t dst_port_msk = bridgeif_fdb_get_dst_ports(br->fdbd, dst_ethaddr);
        // if this mac belongs to lan, bit0: sta, bit1: sap
        if (dst_port_msk == BIT(bridge_sap_port)) {
          // build ARP Reply
          etharp_raw2(rx_if, netif_get_index(rx_if),  // use upstream netif
                 (struct eth_addr *)rx_if->hwaddr, &arphdr->shwaddr,
                 (struct eth_addr *)rx_if->hwaddr, &dipaddr,  /* replace with upstream's hwaddr */
                 &arphdr->shwaddr, &sipaddr,
                 ARP_REPLY, 0);
        }
      } else {
        // resend ARP Request, flood to other bridge ports, FIXME: use bridge to forward???
        // TODO: when mesh sta arp replies, send arp reply to external STA immediatedly
        etharp_raw2(br->netif, netif_get_index(rx_if),  // use upstream netif
             (struct eth_addr *)br->netif->hwaddr, &ethhdr->dest,
             &arphdr->shwaddr, &sipaddr,
             &arphdr->dhwaddr, &dipaddr,
             ARP_REQUEST, 1);
      }

      return 1;    // eat this packet
    } else if (arphdr->opcode == PP_HTONS(ARP_REPLY)) {
      /* optimize */
      ip4_addr_t sipaddr;
      memcpy(&sipaddr, &arphdr->sipaddr, sizeof(ip4_addr_t));

      br->netif->input(p, br->netif);  // lwip arp module will update arptable and send ARP REPLY

      // send_gratuitous_arp_downstream(br, rx_if, &arphdr->shwaddr, &ipaddr);
      arp_request_handle_reply(br, rx_if, &sipaddr, (uint8_t *)&arphdr->shwaddr);

      return 2;    // eat this packet
    }
  } else if (type == PP_HTONS(ETHTYPE_IP)) {
    /*
     * DHCP
     */
    s16_t ip_hdr_offset = SIZEOF_ETH_HDR;  //XXX: VLAN
    struct ip_hdr *iphdr = (struct ip_hdr *)((u8_t *)p->payload + ip_hdr_offset);
    u16_t iphdr_hlen;
    u16_t iphdr_len;
    u8_t proto;
    struct udp_hdr *udphdr;
    u16_t src, dest;

    if (IPH_V(iphdr) != 4)
      return 0;

    /* obtain IP header length in number of 32-bit words */
    iphdr_hlen = IPH_HL(iphdr);
    /* calculate IP header length in bytes */
    iphdr_hlen *= 4;
    /* obtain ip length in bytes */
    iphdr_len = lwip_ntohs(IPH_LEN(iphdr));

    /* Trim pbuf. This is especially required for packets < 60 bytes. */

    /* header length exceeds first pbuf length, or ip length exceeds total pbuf length? */
    if ((iphdr_hlen > p->len) || (iphdr_len > p->tot_len) || (iphdr_hlen < IP_HLEN)) {
      if (iphdr_hlen < IP_HLEN) {
        LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
              ("ip4_input: short IP header (%"U16_F" bytes) received, IP packet dropped\n", iphdr_hlen));
      }
      if (iphdr_hlen > p->len) {
        LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
              ("IP header (len %"U16_F") does not fit in first pbuf (len %"U16_F"), IP packet dropped.\n",
               iphdr_hlen, p->len));
      }
      if (iphdr_len > p->tot_len) {
        LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
              ("IP (len %"U16_F") is longer than pbuf (len %"U16_F"), IP packet dropped.\n",
               iphdr_len, p->tot_len));
      }

      return 0;
    }

#if XXX
    /// ??? why droped, TBD
    if (IP_MULTICAST(htonl(iphdr->dest.addr))) {
      LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
            ("STA MULTICAST, IP packet dropped. %"U16_F".%"U16_F".%"U16_F".%"U16_F", ipdst: %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
             ip4_addr1_16(&iphdr->src), ip4_addr2_16(&iphdr->src), ip4_addr3_16(&iphdr->src), ip4_addr4_16(&iphdr->src),
             ip4_addr1_16(&iphdr->dest), ip4_addr2_16(&iphdr->dest), ip4_addr3_16(&iphdr->dest), ip4_addr4_16(&iphdr->dest)));
      return 1;
    }
#endif

    // dmac: root, dip: node
    // find ip, if this ip not equal to us, set ether hdr src to arp table's mac
#if BRIDGE_MODIFY_DST_ADDR
    const ip4_addr_t *unused_ipaddr;
    struct eth_addr *ethaddr;
    ip_addr_t localip = br->netif->ip_addr;
#endif
    ip4_addr_t sipaddr, dipaddr;
    /* Copy struct ip4_addr2 to aligned ip4_addr, to support compilers without
     * structure packing (not using structure copy which breaks strict-aliasing rules). */
    memcpy(&sipaddr, &iphdr->src, sizeof(ip4_addr_t));
    memcpy(&dipaddr, &iphdr->dest, sizeof(ip4_addr_t));


    // parse dhcp message, create arp entry if needed
    proto = IPH_PROTO(iphdr);
    if (proto == IP_PROTO_UDP) {
      /*
       * II: DHCP relay (we handle bcast packets from extAP, DHCP Offer, DHCP ACK)
       *   a: DHCP Offer, DHCP ACK: replace extAP's mac with our mac (softap's mac ?), and use bridge flood
       *   b: others, ignore
       */
      udphdr = (struct udp_hdr *)((u8_t *)p->payload + ip_hdr_offset + iphdr_hlen);

      /* convert src and dest ports to host byte order */
      src = lwip_ntohs(udphdr->src);
      dest = lwip_ntohs(udphdr->dest);

      if (src == DHCP_SERVER_PORT && dest == DHCP_CLIENT_PORT) {
        bk_bridge_upstream_handle_dhcp(rx_if, udphdr, &dipaddr, (struct dhcp_msg *)(udphdr + 1), p->len - ip_hdr_offset - iphdr_hlen - sizeof(*udphdr));
        // packet is modified, skip udp checksum
        if (dipaddr.addr == IPADDR_BROADCAST) {
          LWIP_LOGI("dhcp packet\n");
          return 0;
        }

      } else if (src == DHCP_CLIENT_PORT && dest == DHCP_SERVER_PORT) {
        // drop this packet
        LWIP_LOGI("drop dhcp server packet\n");
        return 1;
      }
    }

    if (bk_bridge_upstream_recv_filter_l3(br, p, type, proto, iphdr, (u8_t *)p->payload + ip_hdr_offset + iphdr_hlen))
      return 1;

#if BRIDGE_MODIFY_DST_ADDR
  // find mac address of dip, if this dip not equal to us, set ether hdr src to arp table's mac
    if (dipaddr.addr != IPADDR_BROADCAST && !IP_MULTICAST(htonl(dipaddr.addr)) && ip_2_ip4(&localip)->addr != dipaddr.addr) {
      if (etharp_find_addr(NULL, &dipaddr, &ethaddr, &unused_ipaddr) >= 0) {
        memcpy(&ethhdr->dest, ethaddr, ETH_HWADDR_LEN);
      } else {
        // arp query, send ARP Request, flood to other bridge ports
        etharp_raw2(br->netif, netif_get_index(rx_if),  // use upstream netif
               (struct eth_addr *)br->netif->hwaddr, &ethbroadcast,
               (struct eth_addr *)br->netif->hwaddr, netif_ip4_addr(br->netif), /* replace with upstream's hwaddr */
               &ethzero, &dipaddr,
               ARP_REQUEST, 0);
      }
    }
#endif

#if XXX
    // ???? TBD
    if (dipaddr.addr == IPADDR_BROADCAST) {
        LWIP_LOGI("STA BROADCAST, packet dropped.\n");
        return 1;
    }
#endif

  } else if (bk_bridge_upstream_recv_filter_l2(p, type, ethhdr)) {
      return 1;
  }

  return 0;
}

static struct list_head g_arp_request = LIST_HEAD_INIT(g_arp_request);
struct arp_req_entry {
  struct list_head node;  // list node
  ip4_addr_t dip;    // ARP request's DIP
  ip4_addr_t sip;    // ARP request's SIP
  uint8_t  shwaddr[ETH_ALEN];  // ARP request's HW addr
  uint32_t ctime;    // added/updated time
};

static struct arp_req_entry *arp_request_find(uint8_t *shwaddr, uint32_t dip)
{
  struct arp_req_entry *pos;
  list_for_each_entry(pos, &g_arp_request, node) {
    if (pos->dip.addr == dip && !memcmp(pos->shwaddr, shwaddr, ETH_ALEN))
      return pos;
  }

  return NULL;
}

// dip: target IP
// sip: sender's IP
// shwaddr: sender's MAC
static void arp_request_add(uint8_t *shwaddr, uint32_t sip, uint32_t dip)
{
  // find first
  struct arp_req_entry *pos;
  GLOBAL_INT_DECLARATION();

  GLOBAL_INT_DISABLE();
  pos = arp_request_find(shwaddr, dip);
  if (pos) {
    pos->ctime = rtos_get_time();
    pos->sip.addr = sip;
    GLOBAL_INT_RESTORE();
    LWIP_LOGD("%s: already added arp request: shwaddr %pm, sip %pIn, dip %pIn\n",
      __func__, shwaddr, &sip, &dip);
    return;
  }
  GLOBAL_INT_RESTORE();

  // add to list
  pos = os_malloc(sizeof(*pos));
  if (pos) {
    pos->dip.addr = dip;
    pos->sip.addr = sip;
    memcpy(pos->shwaddr, shwaddr, ETH_ALEN);
    pos->ctime = rtos_get_time();
    GLOBAL_INT_DISABLE();
    list_add_tail(&pos->node, &g_arp_request);
    GLOBAL_INT_RESTORE();
    LWIP_LOGD("%s: add arp request: shwaddr %pm, sip %pIn, dip %pIn\\n",
      __func__, shwaddr, &sip, &dip);
  }
}

// dip: ARP reply's sip
// dhwaddr: ARP reply's shwaddr
static void arp_request_handle_reply(bridgeif_private_t *br, struct netif *netif, ip4_addr_t *dip, uint8_t *dhwaddr)
{
  // iterate list to find dip request
  struct list_head reqs;
  struct arp_req_entry *pos, *tmp;
  int count = 0;
  GLOBAL_INT_DECLARATION();

  INIT_LIST_HEAD(&reqs);

  // move arp request whose dip matches our request to list reqs.
  GLOBAL_INT_DISABLE();
  list_for_each_entry_safe(pos, tmp, &g_arp_request, node) {
    if (pos->dip.addr == dip->addr) {
      list_del(&pos->node);
      list_add_tail(&pos->node, &reqs);
      count++;
    }
  }
  GLOBAL_INT_RESTORE();

  if (count) {
    LWIP_LOGD("send arp reply to %d STA\n", count);
  } else {
    return;
  }

  // iterate for all req
  list_for_each_entry_safe(pos, tmp, &reqs, node) {
    list_del(&pos->node);
    // send arp reply
    LWIP_LOGI("external ARP reply, send to STA\n");
    etharp_raw2(br->netif, netif_get_index(netif),
           (struct eth_addr *)dhwaddr, (struct eth_addr *)pos->shwaddr, // ethernet hdr
           (struct eth_addr *)dhwaddr, dip, // shw, sip
           (struct eth_addr *)pos->shwaddr, &pos->sip,  //
           ARP_REPLY, 1);
    os_free(pos);
  }
}

#define time_after(a,b)     \
     ((long)(b) - (long)(a) < 0)

/* flush mesh nodes arp request */
void arp_request_timeout_handler()
{
  struct arp_req_entry *pos, *tmp;
  uint32_t ctime = rtos_get_time();

  GLOBAL_INT_DECLARATION();
  GLOBAL_INT_DISABLE();
  list_for_each_entry_safe(pos, tmp, &g_arp_request, node) {
    if (time_after(ctime, pos->ctime + (5 * rtos_get_tick_count()/*HZ*/))) {
      list_del(&pos->node);
      LWIP_LOGD("%s: free arp request: shwaddr %pm, sip 0x%x, dip 0x%x\n",
        __func__, pos->shwaddr, pos->sip.addr, pos->dip.addr);
      os_free(pos);
    }
  }
  GLOBAL_INT_RESTORE();
}

#ifndef DHCP_FLAG_BROADCAST
#define DHCP_FLAG_BROADCAST  (1 << 15)
#endif

static uint32_t checksum32(uint32_t start_value, uint8_t *data, size_t len)
{
    uint32_t checksum32 = start_value;
    uint16_t data16 = 0;
    int i;

    for(i = 0; i < (len / 2 * 2); i += 2) {
        data16 = (data[i] << 8) | data[i + 1];
        checksum32 += data16;
    }

    if(len % 2) {
        data16 = data[len - 1] << 8;
        checksum32 += data16;
    }

    return checksum32;
}

static uint16_t checksum32to16(uint32_t checksum32)
{
    uint16_t checksum16 = 0;

    checksum32 = (checksum32 >> 16) + (checksum32 & 0x0000ffff);
    checksum32 = (checksum32 >> 16) + (checksum32 & 0x0000ffff);
    checksum16 = (uint16_t) ~(checksum32 & 0xffff);

    return checksum16;
}

u16_t calculate_chksum_pseudo(uint8_t *payload, struct udp_hdr *uhdr, uint8_t protocol, u16_t ulen,u32_t *src, u32_t *dest)
{
    uint32_t udp_checksum32 = 0;
    uint16_t udp_checksum16 = 0;

    uint8_t *uhdr_data=NULL;
    // pseudo header
    uint8_t pseudo_header[12] = {/*srcip*/ 0x00, 0x00, 0x00, 0x00 /*srcip*/, /*dstip*/ 0x00, 0x00, 0x00, 0x00 /*dstip*/, 0x00, /*protocol*/ 0x00 /*protocol*/, /*l4len*/ 0x00, 0x00 /*l4len*/};
    uhdr_data = (uint8_t *)uhdr;
    memcpy(&pseudo_header[0], src, 4);
    memcpy(&pseudo_header[4], dest, 4);
    pseudo_header[8] = 0x00;
    pseudo_header[9] = protocol;

    pseudo_header[10] = (uint8_t) (ulen >> 8);
    pseudo_header[11] = (uint8_t) (ulen & 0xff);

    udp_checksum32 = checksum32(udp_checksum32, pseudo_header, sizeof(pseudo_header));
    udp_checksum32 = checksum32(udp_checksum32, uhdr_data, sizeof(struct udp_hdr));
    udp_checksum32 = checksum32(udp_checksum32, payload, (ulen-8));
    udp_checksum16 = checksum32to16(udp_checksum32);
    return udp_checksum16;
}

/*
 * For BK-MESH-ROOT upstream (sta0).
 * I: proxy ARP
 *    If packet is ARP Request, duplicate pbuf and replace srcmac with sta's mac.
 *    If packet is ARP Reply, send gratious ARP to extAP.
 *
 * II: DHCP relay (DHCP Discover, DHCP Request)
 *    a: replace mac addr with our mac (sta's mac or br0's mac).
 *    b: change DHCP bootp's flags from unicast to bcast.
 *    c: arp_table will automatically be updated.
 */
static int bk_bridge_upstream_if_xmit(bridgeif_private_t *br, struct pbuf *p, struct netif *tx_if)
{
  struct eth_hdr *ethhdr;
  u16_t type;
  struct netif *brif;
  bool ip_mcast = false;

  if (!netif_is_upstream(tx_if))  // sta0
    return 0;

  // local node output
  if (p->if_idx == NETIF_NO_INDEX)
    return 0;

  if (p->len <= SIZEOF_ETH_HDR)
    return 1;

  //etharp_gratuitous, send_gratuitous_arp, etharp_raw
  //ethernet_input
  ethhdr = p->payload;
  type = ethhdr->type;
  brif = br->netif;

  if (type == PP_HTONS(ETHTYPE_ARP)) {
    struct etharp_hdr *hdr = (struct etharp_hdr *)(ethhdr + 1);
    bool is_probe = false;
    bool is_gratious = false;
    bool is_announcement __maybe_unused = false;
    bool is_local = false;

    /* RFC 826 "Packet Reception": */
    if ((hdr->hwtype != PP_HTONS(LWIP_IANA_HWTYPE_ETHERNET)) ||
      (hdr->hwlen != ETH_HWADDR_LEN) ||
      (hdr->protolen != sizeof(ip4_addr_t)) ||
      (hdr->proto != PP_HTONS(ETHTYPE_IP)))
      return 1;

    // If this ARP Request is sent by bridge itself
    if (!memcmp(&ethhdr->src, brif->hwaddr, ETH_ALEN))
      is_local = true;

    /* from wireshark */
    /* ARP requests/replies with the same sender and target protocol
     address are flagged as "gratuitous ARPs", i.e. ARPs sent out as,
     in effect, an announcement that the machine has MAC address
     XX:XX:XX:XX:XX:XX and IPv4 address YY.YY.YY.YY. Requests are to
     provoke complaints if some other machine has the same IPv4 address,
     replies are used to announce relocation of network address, like
     in failover solutions. */

    //os_printf("opcode: %d, shw %pm, sip %pIn, dhw %pm, dip %pIn\n", PP_NTOHS(hdr->opcode),
    //  &hdr->shwaddr, &hdr->sipaddr, &hdr->dhwaddr, &hdr->dipaddr);
    //os_printf("%d %d %d\n", hdr->opcode == PP_HTONS(ARP_REQUEST),
    //  hdr->opcode == PP_HTONS(ARP_REPLY), etharp_ip4_addr_cmp(hdr->sipaddr, hdr->dipaddr));
    if ((hdr->opcode == PP_HTONS(ARP_REQUEST) || hdr->opcode == PP_HTONS(ARP_REPLY)) &&
        etharp_ip4_addr_cmp(hdr->sipaddr, hdr->dipaddr)) {
      is_gratious = true;
      if (hdr->opcode == PP_HTONS(ARP_REQUEST) &&
          !memcmp(&hdr->dhwaddr, &ethzero, ETH_HWADDR_LEN))
        is_announcement = true;
    } else {
      is_gratious = false;
      if (hdr->opcode == PP_HTONS(ARP_REQUEST) &&
          !memcmp(&hdr->dhwaddr, &ethzero, ETH_HWADDR_LEN) &&
          etharp_ip4_addr_isany_val(hdr->sipaddr))
        is_probe = true;
    }

    // os_printf("xmit: opcode %d, local %d, is_gratious %d, is_announcement %d, is_probe %d\n",
    //  PP_NTOHS(hdr->opcode), is_local, is_gratious, is_announcement, is_probe);

    if (hdr->opcode == PP_HTONS(ARP_REQUEST)) {
      struct pbuf *r;
      const ip4_addr_t *unused_ipaddr;
      struct eth_addr *dethaddr;
      ip4_addr_t sipaddr, dipaddr;

      // If this ARP Request is sent by ourself
      if (is_local)
        return 0;

      /* Copy struct ip4_addr2 to aligned ip4_addr, to support compilers without
       * structure packing (not using structure copy which breaks strict-aliasing rules). */
      memcpy(&sipaddr, &hdr->sipaddr, sizeof(ip4_addr_t));
      memcpy(&dipaddr, &hdr->dipaddr, sizeof(ip4_addr_t));

      if (is_gratious || is_probe) {
        /* Allocate reply and copy */
        r = pbuf_clone(PBUF_RAW_TX, PBUF_RAM, p);
        if (r == NULL)
          return 1;

        ethhdr = (struct eth_hdr *)r->payload;
        hdr = (struct etharp_hdr *)(ethhdr + 1);
        MEMCPY(&ethhdr->src, (struct eth_addr *)tx_if->hwaddr, ETH_HWADDR_LEN);
        if (memcmp(&hdr->shwaddr, &ethzero, ETH_HWADDR_LEN))
          MEMCPY(&hdr->shwaddr, (struct eth_addr *)tx_if->hwaddr, ETH_HWADDR_LEN);
        tx_if->linkoutput(tx_if, r);
        pbuf_free(r);

        return 1;
      }

      // for STA under softap broadcast ARP request whose dip equals to us
      if (!memcmp(&dipaddr, &br->netif->ip_addr, 4)) {
        LWIP_LOGD("DIP to US\n");
        return 1;
      }

      /* check if we already have arpinfo */
      if (etharp_find_addr(NULL, &dipaddr, &dethaddr, &unused_ipaddr) >= 0) {
#if XXX
        //build ARP Reply if belong to non-softap
        bridgeif_portmask_t dst_port_msk = bridgeif_fdb_get_dst_ports(br->fdbd, dethaddr);
        // if this mac belongs to sta, bit0: sta, bit1: sap
        if (dst_port_msk == BIT(bridge_sta_port))
        {
          LWIP_LOGD("found ARP cache for %pIn, send ARP rsp to STA\n", &dipaddr);
          etharp_raw2(br->netif, netif_get_index(tx_if),  // use upstream netif
                 (struct eth_addr *)br->netif->hwaddr, &hdr->shwaddr,  // ethernet hdr
                 dethaddr, &dipaddr,  /* replace with upstream's hwaddr */
                 &hdr->shwaddr, &sipaddr,
                 ARP_REPLY, 1);
        }
        return 1;  //eat, continue send arp request to external PC
#else
        //build ARP Reply
        LWIP_LOGD("found ARP cache for %pIn, send ARP rsp to STA\n", &dipaddr);
        etharp_raw2(br->netif, netif_get_index(tx_if),  // use upstream netif
               (struct eth_addr *)br->netif->hwaddr, &hdr->shwaddr,  // ethernet hdr
               dethaddr, &dipaddr,  /* replace with upstream's hwaddr */
               &hdr->shwaddr, &sipaddr,
               ARP_REPLY, 1);
        //return 1;  //eat, continue send arp request to external PC
#endif
      } else {
        // add to queue
        LWIP_LOGI("cannot find ARP for ip %pIn, add to request\n", &dipaddr);
        arp_request_add((uint8_t *)&hdr->shwaddr, sipaddr.addr, dipaddr.addr);
      }

      /* Allocate reply and copy */
      r = pbuf_clone(PBUF_RAW_TX, PBUF_RAM, p);
      if (r == NULL)
        return 1;

      ethhdr = (struct eth_hdr *)r->payload;
      hdr = (struct etharp_hdr *)(ethhdr + 1);
      MEMCPY(&ethhdr->src, (struct eth_addr *)tx_if->hwaddr, ETH_HWADDR_LEN);
      if (memcmp(&hdr->shwaddr, &ethzero, ETH_HWADDR_LEN))
        MEMCPY(&hdr->shwaddr, (struct eth_addr *)tx_if->hwaddr, ETH_HWADDR_LEN);
      tx_if->linkoutput(tx_if, r);
      pbuf_free(r);

      return 1;
    } else if (hdr->opcode == PP_HTONS(ARP_REPLY)) {
      /* optimize: must check this ARP entry is in LAN */
      struct pbuf *r;

      /* Allocate reply and copy */
      r = pbuf_clone(PBUF_RAW_TX, PBUF_RAM, p);
      if (r == NULL)
        return 1;

      ethhdr = (struct eth_hdr *)r->payload;
      hdr = (struct etharp_hdr *)(ethhdr + 1);
      // replace with root's mac addr
      MEMCPY(&ethhdr->src, (struct eth_addr *)tx_if->hwaddr, ETH_HWADDR_LEN);
      MEMCPY(&hdr->shwaddr, (struct eth_addr *)tx_if->hwaddr, ETH_HWADDR_LEN);
      tx_if->linkoutput(tx_if, r);
      pbuf_free(r);

      return 1;
    }
  } else if (type == PP_HTONS(ETHTYPE_IP)) {
    /*
     * DHCP
     */
    s16_t ip_hdr_offset = SIZEOF_ETH_HDR;  //XXX: VLAN
    struct ip_hdr *iphdr = (struct ip_hdr *)((u8_t *)p->payload + ip_hdr_offset);
    u16_t iphdr_hlen;
    u16_t iphdr_len;
    struct udp_hdr *udphdr;
    u16_t src, dest;
    s16_t proto;

    if (IPH_V(iphdr) != 4)
      return 1;

    /* obtain IP header length in number of 32-bit words */
    iphdr_hlen = IPH_HL(iphdr);
    /* calculate IP header length in bytes */
    iphdr_hlen *= 4;
    /* obtain ip length in bytes */
    iphdr_len = lwip_ntohs(IPH_LEN(iphdr));

    /* Trim pbuf. This is especially required for packets < 60 bytes. */

    /* header length exceeds first pbuf length, or ip length exceeds total pbuf length? */
    if ((iphdr_hlen > p->len) || (iphdr_len > p->tot_len) || (iphdr_hlen < IP_HLEN)) {
      if (iphdr_hlen < IP_HLEN) {
        LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
              ("ip4_input: short IP header (%"U16_F" bytes) received, IP packet dropped\n", iphdr_hlen));
      }
      if (iphdr_hlen > p->len) {
        LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
              ("IP header (len %"U16_F") does not fit in first pbuf (len %"U16_F"), IP packet dropped.\n",
               iphdr_hlen, p->len));
      }
      if (iphdr_len > p->tot_len) {
        LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
              ("IP (len %"U16_F") is longer than pbuf (len %"U16_F"), IP packet dropped.\n",
               iphdr_len, p->tot_len));
      }
      goto recheck;
    }

    if (IP_MULTICAST(htonl(iphdr->dest.addr))){
      LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
            ("IP packet. %"U16_F".%"U16_F".%"U16_F".%"U16_F", ipdst: %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
             ip4_addr1_16(&iphdr->src), ip4_addr2_16(&iphdr->src), ip4_addr3_16(&iphdr->src), ip4_addr4_16(&iphdr->src),
             ip4_addr1_16(&iphdr->dest), ip4_addr2_16(&iphdr->dest), ip4_addr3_16(&iphdr->dest), ip4_addr4_16(&iphdr->dest)));
      ip_mcast = true;
    }

    proto = IPH_PROTO(iphdr);

    if (proto == IP_PROTO_UDP) {
      /*
       * II: DHCP relay (we handle bcast packets from extAP, DHCP Offer, DHCP ACK)
       *   a: DHCP Offer, DHCP ACK: replace extAP's mac with our mac (softap's mac ?), and use bridge flood
       *   b: others, ignore
       */
      udphdr = (struct udp_hdr *)((u8_t *)p->payload + ip_hdr_offset + iphdr_hlen);

      /* convert src and dest ports to host byte order */
      src = lwip_ntohs(udphdr->src);
      dest = lwip_ntohs(udphdr->dest);

      /* DHCP Discover and Request */
      if (src == DHCP_CLIENT_PORT && dest == DHCP_SERVER_PORT) {
        struct pbuf *r;
        struct dhcp_msg *out;

        // If this DHCP frame is sent by ourself
        if (!memcmp(&ethhdr->src, brif->hwaddr, ETH_ALEN))
          return 0;

        /* Allocate reply and copy */
        r = pbuf_clone(PBUF_RAW_TX, PBUF_RAM, p);
        if (r == NULL)
          return 1;
        ethhdr = (struct eth_hdr *)r->payload;
        udphdr = (struct udp_hdr *)((u8_t *)r->payload + ip_hdr_offset + iphdr_hlen);
        out = (struct dhcp_msg *)(udphdr + 1);

        if (!out->flags) {
#ifndef BK_LWIP_FAST_CSUM_MOD
          uint8_t *udp_data=NULL;
          u32_t src_addr=0;
          u32_t dest_addr=0;
#else
          u16_t old_value = *(uint16_t *)(&out->flags);
#endif

          out->flags |= lwip_htons(DHCP_FLAG_BROADCAST);

          /* recalc checksum */
#ifndef BK_LWIP_FAST_CSUM_MOD
          udp_data = (uint8_t *)out;
          src_addr = iphdr->src.addr;
          dest_addr= iphdr->dest.addr;
          udphdr->chksum = calculate_chksum_pseudo(udp_data, udphdr, 0x11, lwip_ntohs(udphdr->len), &src_addr, &dest_addr);
          udphdr->chksum = lwip_htons(udphdr->chksum);
#else
          udphdr->chksum = recalc_csum16(udphdr->chksum, old_value, *(uint16_t *)(&out->flags));
#endif
        }

        /* replace eth hdr address */
        MEMCPY(&ethhdr->src, (struct eth_addr *)tx_if->hwaddr, ETH_HWADDR_LEN);  // XXX: use downstream iface's mac
        tx_if->linkoutput(tx_if, r);    // low_level_output
        pbuf_free(r);

        return 1;  // eaten
      }
    }

    if (!ip_mcast) {
      // Change SMAC address
      MEMCPY(&ethhdr->src, (struct eth_addr *)tx_if->hwaddr, ETH_HWADDR_LEN);
      struct pbuf *r;
      /* Allocate reply and copy */
      r = pbuf_clone(PBUF_RAW_TX, PBUF_RAM, p);
      if (r == NULL)
        return 1;
      tx_if->linkoutput(tx_if, r);    // low_level_output
      pbuf_free(r);
      return 1;
      //goto recheck;
    } else {
      return 0;
    }
  } else if (type == PP_HTONS(ETHTYPE_IPV6)) {
    // FIXME
    return 1;
  } else {
    // other eth type
    return 0;
  }

recheck:
  if (memcmp(&ethhdr->src, br->netif->hwaddr, 6)) {  // src addr is not routers mac
    LWIP_DEBUGF(BRIDGEIF_FW_DEBUG, (""); print_hex_dump("GARBAGE: ", p->payload, p->len));  // FIXME: SSDP
    return 1;  // don't forward to extAP
  } else {
    return 0;
  }
}

static void etharp_remove_static_entry_by_mac(uint8_t *mac)
{
  int i;
  /* find requested entry */
  for (i = 0; i < ARP_TABLE_SIZE; i++) {
    ip4_addr_t *ip;
    struct netif *netif;
    struct eth_addr *ethaddr;

    if (etharp_get_entry(i, &ip, &netif, &ethaddr)) {
      if (!memcmp(mac, ethaddr, ETH_HWADDR_LEN)) {
        LWIP_LOGI("remove static ARP entry for %pIn/%pm\n", ip, mac);
        etharp_remove_static_entry(ip);
        // break;
      }
    }
  }
}

void bk_bridge_event_hapd_sta_disconnected(uint8_t *mac)
{
  etharp_remove_static_entry_by_mac(mac);
}

#endif // BK_LWIP

/* Output helper function */
static err_t
#if BK_LWIP
bridgeif_send_to_port(bridgeif_private_t *br, struct pbuf *p, u8_t dstport_idx, bool copy)
#else
bridgeif_send_to_port(bridgeif_private_t *br, struct pbuf *p, u8_t dstport_idx)
#endif
{
  if (dstport_idx < BRIDGEIF_MAX_PORTS) {
    /* possibly an external port */
    if (dstport_idx < br->max_ports) {
      struct netif *portif = br->ports[dstport_idx].port_netif;
      if ((portif != NULL) && (portif->linkoutput != NULL)) {
        /* prevent sending out to rx port */
        if (netif_get_index(portif) != p->if_idx) {
          if (netif_is_link_up(portif)) {
#if BK_LWIP
            if ((void *)portif == net_get_sta_handle() && p->elfags == 1) {
              LWIP_LOGD("DONT xmit via STA interface\n");
              return ERR_OK;
            }
            LWIP_DEBUGF(BRIDGEIF_FW_DEBUG, ("br -> flood(%p:%d) -> %d\n", (void *)p, p->if_idx, netif_get_index(portif)));
            if (!bk_bridge_upstream_if_xmit(br, p, portif)) {  // if packet not eat
              if (copy) {
                struct pbuf *r;
                int ret = 0;
                /* Allocate reply and copy */
                r = pbuf_clone(PBUF_RAW_TX, PBUF_RAM, p);
                if (r == NULL)
                  return 1;
                ret = portif->linkoutput(portif, r);
                pbuf_free(r);
                return ret;
              } else {
                /* no copy needed */
                return portif->linkoutput(portif, p);
              }
            }
#else
            LWIP_DEBUGF(BRIDGEIF_FW_DEBUG, ("br -> flood(%p:%d) -> %d\n", (void *)p, p->if_idx, netif_get_index(portif)));
            return portif->linkoutput(portif, p);
#endif
          }
        }
      }
    }
  } else {
    LWIP_ASSERT("invalid port index", dstport_idx == BRIDGEIF_MAX_PORTS);
  }
  return ERR_OK;
}

#if BK_LWIP
bool bridgeif_pbuf_need_copy(bridgeif_portmask_t n)
{
    int count = 0;
    while (n) {
        n &= (n - 1);
        count++;
        if (count > 1)
          return true;
    }
    return false;
}
#endif

/** Helper function to pass a pbuf to all ports marked in 'dstports'
 */
static err_t
bridgeif_send_to_ports(bridgeif_private_t *br, struct pbuf *p, bridgeif_portmask_t dstports)
{
  err_t err, ret_err = ERR_OK;
  u8_t i;
  bridgeif_portmask_t mask = 1;
  BRIDGEIF_DECL_PROTECT(lev);
#if BK_LWIP
  bool copy = bridgeif_pbuf_need_copy(dstports);
#endif

  BRIDGEIF_READ_PROTECT(lev);
  for (i = 0; i < BRIDGEIF_MAX_PORTS; i++, mask = (bridgeif_portmask_t)(mask << 1)) {
    if (dstports & mask) {
#if BK_LWIP
      err = bridgeif_send_to_port(br, p, i, copy);
#else
      err = bridgeif_send_to_port(br, p, i);
#endif
      if (err != ERR_OK) {
        ret_err = err;
      }
    }
  }
  BRIDGEIF_READ_UNPROTECT(lev);
  return ret_err;
}

/** Output function of the application port of the bridge (the one with an ip address).
 * The forwarding port(s) where this pbuf is sent on is/are automatically selected
 * from the FDB.
 */
static err_t
bridgeif_output(struct netif *netif, struct pbuf *p)
{
  err_t err;
  bridgeif_private_t *br = (bridgeif_private_t *)netif->state;
  struct eth_addr *dst = (struct eth_addr *)(p->payload);

  bridgeif_portmask_t dstports = bridgeif_find_dst_ports(br, dst);
  err = bridgeif_send_to_ports(br, p, dstports);

  MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
  if (((u8_t *)p->payload)[0] & 1) {
    /* broadcast or multicast packet*/
    MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
  } else {
    /* unicast packet */
    MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
  }
  /* increase ifoutdiscards or ifouterrors on error */

  LINK_STATS_INC(link.xmit);

  return err;
}

/** The actual bridge input function. Port netif's input is changed to call
 * here. This function decides where the frame is forwarded.
 */
static err_t
bridgeif_input(struct pbuf *p, struct netif *netif)
{
  u8_t rx_idx;
  bridgeif_portmask_t dstports;
  struct eth_addr *src, *dst;
  bridgeif_private_t *br;
  bridgeif_port_t *port;
  if (p == NULL || netif == NULL) {
    return ERR_VAL;
  }
  port = (bridgeif_port_t *)netif_get_client_data(netif, bridgeif_netif_client_id);
  LWIP_ASSERT("port data not set", port != NULL);
  if (port == NULL || port->bridge == NULL) {
    return ERR_VAL;
  }
  br = (bridgeif_private_t *)port->bridge;
#if BK_LWIP
  if (br->fdbd == NULL) {
    return ERR_VAL;
  }
#endif
  rx_idx = netif_get_index(netif);
  /* store receive index in pbuf */
  p->if_idx = rx_idx;

  dst = (struct eth_addr *)p->payload;
  src = (struct eth_addr *)(((u8_t *)p->payload) + sizeof(struct eth_addr));

  if ((src->addr[0] & 1) == 0) {
    /* update src for all non-group addresses */
    bridgeif_fdb_update_src(br->fdbd, src, port->port_num);
  }

#if BK_LWIP
  /*
   * If we are act as BK MESH root and this br port (sta0) is upstream, do the following
   * I: proxy ARP
   *   a: ARP Request
   *     If we found an arp entry in our arp_table, send ARP response with mac=ourmac, ip=node'sip,
   *     and send an gratious arp to all nodes in out mesh network.
   *     If not found, build arp request to all nodes in our mesh network.
   *   b: ARP Response
   *     update arp_table, may be optimized(send arp response immediately, or send gratioius arp).
   * II: DHCP relay (we handle bcast packets from extAP, DHCP Offer, DHCP ACK)
   *   a: DHCP Offer, DHCP ACK: replace extAP's mac with our mac (softap's mac ?), and use bridge flood
   *   b: others, ignore
   */
  int ret;
  if ((ret = bk_bridge_upstream_if_recv(br, p, netif)) != 0) {
    if (ret == 1) {
      pbuf_free(p);
    }
    /* always return ERR_OK here to prevent the caller freeing the pbuf */
    return ERR_OK;
  }
#endif

  if (dst->addr[0] & 1) {
    /* group address -> flood + cpu? */
    dstports = bridgeif_find_dst_ports(br, dst);
    bridgeif_send_to_ports(br, p, dstports);
    if (dstports & (1 << BRIDGEIF_MAX_PORTS)) {
      /* we pass the reference to ->input or have to free it */
      LWIP_DEBUGF(BRIDGEIF_FW_DEBUG, ("br -> input(%p)\n", (void *)p));
#if BK_LWIP
      p->if_idx = NETIF_NO_INDEX;  // reset if_idx, netif->input() will set if_idx
#endif
      if (br->netif->input(p, br->netif) != ERR_OK) {
        pbuf_free(p);
      }
    } else {
      /* all references done */
      pbuf_free(p);
    }
    /* always return ERR_OK here to prevent the caller freeing the pbuf */
    return ERR_OK;
  } else {
    /* is this for one of the local ports? */
    if (bridgeif_is_local_mac(br, dst)) {
      /* yes, send to cpu port only */
      LWIP_DEBUGF(BRIDGEIF_FW_DEBUG, ("br -> input(%p)\n", (void *)p));
#if BK_LWIP
      p->if_idx = NETIF_NO_INDEX;  // reset if_idx, netif->input() will set if_idx
#endif
      return br->netif->input(p, br->netif);
    }

    /* get dst port */
    dstports = bridgeif_find_dst_ports(br, dst);
    bridgeif_send_to_ports(br, p, dstports);
    /* no need to send to cpu, flooding is for external ports only */
    /* by  this, we consumed the pbuf */
    pbuf_free(p);
    /* always return ERR_OK here to prevent the caller freeing the pbuf */
    return ERR_OK;
  }
}

#if !BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT
/** Input function for port netifs used to synchronize into tcpip_thread.
 */
static err_t
bridgeif_tcpip_input(struct pbuf *p, struct netif *netif)
{
  return tcpip_inpkt(p, netif, bridgeif_input);
}
#endif /* BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT */

/**
 * @ingroup bridgeif
 * Initialization function passed to netif_add().
 *
 * ATTENTION: A pointer to a @ref bridgeif_initdata_t must be passed as 'state'
 *            to @ref netif_add when adding the bridge. I supplies MAC address
 *            and controls memory allocation (number of ports, FDB size).
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t
bridgeif_init(struct netif *netif)
{
  bridgeif_initdata_t *init_data;
  bridgeif_private_t *br;
  size_t alloc_len_sizet;
  mem_size_t alloc_len;

  LWIP_ASSERT("netif != NULL", (netif != NULL));
  LWIP_ASSERT("bridgeif needs an input callback", (netif->input != NULL));
#if !BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT
  if (netif->input == tcpip_input) {
    LWIP_DEBUGF(BRIDGEIF_DEBUG | LWIP_DBG_ON, ("bridgeif does not need tcpip_input, use netif_input/ethernet_input instead"));
  }
#endif

  if (bridgeif_netif_client_id == 0xFF) {
    bridgeif_netif_client_id = netif_alloc_client_data_id();
  }

#if BK_LWIP
  if (netif_dhcp_data(netif) == NULL) {
    LWIP_DEBUGF(BRIDGEIF_DEBUG, ("bridgeif set dhcp data\n"));
    struct dhcp *dhcp = netif_dhcp_data((struct netif *)net_get_sta_handle());
    netif_set_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP, dhcp);
  }
#endif

  init_data = (bridgeif_initdata_t *)netif->state;
  LWIP_ASSERT("init_data != NULL", (init_data != NULL));
  LWIP_ASSERT("init_data->max_ports <= BRIDGEIF_MAX_PORTS",
              init_data->max_ports <= BRIDGEIF_MAX_PORTS);

  alloc_len_sizet = sizeof(bridgeif_private_t) + (init_data->max_ports * sizeof(bridgeif_port_t) + (init_data->max_fdb_static_entries * sizeof(bridgeif_fdb_static_entry_t)));
  alloc_len = (mem_size_t)alloc_len_sizet;
  LWIP_ASSERT("alloc_len == alloc_len_sizet", alloc_len == alloc_len_sizet);
  LWIP_DEBUGF(BRIDGEIF_DEBUG, ("bridgeif_init: allocating %d bytes for private data\n", (int)alloc_len));
  br = (bridgeif_private_t *)mem_calloc(1, alloc_len);
  if (br == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("bridgeif_init: out of memory\n"));
    return ERR_MEM;
  }
  memcpy(&br->ethaddr, &init_data->ethaddr, sizeof(br->ethaddr));
  br->netif = netif;

  br->max_ports = init_data->max_ports;
  br->ports = (bridgeif_port_t *)(br + 1);

  br->max_fdbs_entries = init_data->max_fdb_static_entries;
  br->fdbs = (bridgeif_fdb_static_entry_t *)(((u8_t *)(br + 1)) + (init_data->max_ports * sizeof(bridgeif_port_t)));

  br->max_fdbd_entries = init_data->max_fdb_dynamic_entries;
  br->fdbd = bridgeif_fdb_init(init_data->max_fdb_dynamic_entries);
  if (br->fdbd == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("bridgeif_init: out of memory in fdb_init\n"));
    mem_free(br);
    return ERR_MEM;
  }

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, 0);

  netif->state = br;
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
#if LWIP_IPV4
  netif->output = etharp_output;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  netif->linkoutput = bridgeif_output;

  /* set MAC hardware address length */
  netif->hwaddr_len = ETH_HWADDR_LEN;

  /* set MAC hardware address */
  memcpy(netif->hwaddr, &br->ethaddr, ETH_HWADDR_LEN);

  /* maximum transfer unit */
  netif->mtu = 1500;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6 | NETIF_FLAG_LINK_UP;

#if LWIP_IPV6 && LWIP_IPV6_MLD
  /*
   * For hardware/netifs that implement MAC filtering.
   * All-nodes link-local is handled by default, so we must let the hardware know
   * to allow multicast packets in.
   * Should set mld_mac_filter previously. */
  if (netif->mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

  return ERR_OK;
}

#if BK_LWIP
void bridgeif_deinit(struct netif *netif)
{
  bridgeif_private_t *br = netif->state;

  /* restore input function when port added */
  for (int i = 0; i < br->num_ports; i++) {
    struct netif *portif = br->ports[i].port_netif;
    if (portif != NULL) {
      portif->input = portif->input_origin;
    }
  }

  bridgeif_fdb_deinit(br);
  mem_free(br);
  br = NULL;
}
#endif

/**
 * @ingroup bridgeif
 * Add a port to the bridge
 */
err_t
#if BK_LWIP
bridgeif_add_port(struct netif *bridgeif, struct netif *portif, int *port_num)
#else
bridgeif_add_port(struct netif *bridgeif, struct netif *portif)
#endif
{
  bridgeif_private_t *br;
  bridgeif_port_t *port;

  LWIP_ASSERT("bridgeif != NULL", bridgeif != NULL);
  LWIP_ASSERT("bridgeif->state != NULL", bridgeif->state != NULL);
  LWIP_ASSERT("portif != NULL", portif != NULL);

  if (!(portif->flags & NETIF_FLAG_ETHARP) || !(portif->flags & NETIF_FLAG_ETHERNET)) {
    /* can only add ETHERNET/ETHARP interfaces */
    return ERR_VAL;
  }

  br = (bridgeif_private_t *)bridgeif->state;

  if (br->num_ports >= br->max_ports) {
    return ERR_VAL;
  }
  port = &br->ports[br->num_ports];
  port->port_netif = portif;
  port->port_num = br->num_ports;
#if BK_LWIP
  if (port_num)
    *port_num = port->port_num;
  /* backup original input */
  portif->input_origin = portif->input;
#endif
  port->bridge = br;
  br->num_ports++;

  /* let the port call us on input */
#if BRIDGEIF_PORT_NETIFS_OUTPUT_DIRECT
  portif->input = bridgeif_input;
#else
  portif->input = bridgeif_tcpip_input;
#endif
  /* store pointer to bridge in netif */
  netif_set_client_data(portif, bridgeif_netif_client_id, port);
  /* remove ETHARP flag to prevent sending report events on netif-up */
  netif_clear_flags(portif, NETIF_FLAG_ETHARP);

  return ERR_OK;
}

#endif /* LWIP_NUM_NETIF_CLIENT_DATA */
