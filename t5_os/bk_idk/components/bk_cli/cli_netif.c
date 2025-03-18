#include "bk_cli.h"
#include "bk_wifi_types.h"
#include "bk_private/bk_wifi.h"
#if CONFIG_LWIP
// Modified by TUYA Start
// #include "lwip/ping.h"
// Modified by TUYA End
#include "lwip/etharp.h"
#endif
#include <components/netif.h>
#include "cli.h"

extern void make_tcp_server_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

static const char *ifname[NETIF_IF_COUNT] = {
	"sta", "ap", "bridge", "eth",
};

static inline const char *if_idx_name(netif_if_t ifx)
{
	if (ifx > NETIF_IF_COUNT)
		return "unkown";
	return ifname[ifx];
}

#define CLI_DUMP_IP(_prompt, _ifx, _cfg) do {\
	CLI_LOGI("%s netif(%s) ip4=%s mask=%s gate=%s dns=%s\n", (_prompt),\
			if_idx_name(_ifx),\
			(_cfg)->ip, (_cfg)->mask, (_cfg)->gateway, (_cfg)->dns);\
} while(0)

#if (CLI_CFG_NETIF == 1)

static void ip_cmd_show_ip(int ifx)
{
	netif_ip4_config_t config;

	if (ifx == NETIF_IF_STA || ifx == NETIF_IF_AP || ifx == NETIF_IF_ETH || ifx == NETIF_IF_BRIDGE) {
		BK_LOG_ON_ERR(bk_netif_get_ip4_config(ifx, &config));
		CLI_DUMP_IP(" ", ifx, &config);
	} else {
		BK_LOG_ON_ERR(bk_netif_get_ip4_config(NETIF_IF_STA, &config));
		CLI_DUMP_IP(" ", NETIF_IF_STA, &config);
		BK_LOG_ON_ERR(bk_netif_get_ip4_config(NETIF_IF_AP, &config));
		CLI_DUMP_IP(" ", NETIF_IF_AP, &config);
#ifdef CONFIG_ETH
		BK_LOG_ON_ERR(bk_netif_get_ip4_config(NETIF_IF_ETH, &config));
		CLI_DUMP_IP(" ", NETIF_IF_ETH, &config);
#endif
#if CONFIG_BRIDGE
		BK_LOG_ON_ERR(bk_netif_get_ip4_config(NETIF_IF_BRIDGE, &config));
		CLI_DUMP_IP(" ", NETIF_IF_BRIDGE, &config);
#endif
	}
}

void cli_ip_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;
	char *msg = NULL;
	netif_ip4_config_t config = {0};
	int ifx = NETIF_IF_COUNT;

	if (argc > 1) {
		if (os_strcmp("sta", argv[1]) == 0) {
			ifx = NETIF_IF_STA;
		} else if (os_strcmp("ap", argv[1]) == 0) {
			ifx = NETIF_IF_AP;
#ifdef CONFIG_ETH
		} else if (os_strcmp("eth", argv[1]) == 0) {
			ifx = NETIF_IF_ETH;
#endif
#if CONFIG_BRIDGE
		} else if (os_strcmp("br", argv[1]) == 0) {
			ifx = NETIF_IF_BRIDGE;
#endif
		} else {
			CLI_LOGE("invalid netif name\n");
			goto error;
		}
	}

	if (argc == 1) {
		ip_cmd_show_ip(NETIF_IF_COUNT);
	} else if (argc == 2) {
		ip_cmd_show_ip(ifx);
	} else if (argc == 6) {
		os_strncpy(config.ip, argv[2], NETIF_IP4_STR_LEN);
		os_strncpy(config.mask, argv[3], NETIF_IP4_STR_LEN);
		os_strncpy(config.gateway, argv[4], NETIF_IP4_STR_LEN);
		os_strncpy(config.dns, argv[5], NETIF_IP4_STR_LEN);
		BK_LOG_ON_ERR(bk_netif_set_ip4_config(ifx, &config));
		CLI_DUMP_IP("set static ip, ", ifx, &config);
	} else {
		CLI_LOGE("usage: ip [sta|ap][{ip}{mask}{gate}{dns}]\n");
		goto error;
	}

	if (!ret) {
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}

#if CONFIG_LWIP
extern int hwaddr_aton(const char *txt, u8 *addr);

void cli_arp_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int i;

	if (argc <= 1) {
		/* find requested entry */
		for (i = 0; i < ARP_TABLE_SIZE; i++) {
			ip4_addr_t *ip;
			struct netif *netif;
			struct eth_addr *ethaddr;

			if (etharp_get_entry(i, &ip, &netif, &ethaddr)) {
				os_printf("%s @ %pm, %c%c\n", ip4addr_ntoa(ip), ethaddr,
				  netif ? netif->name[0] : ' ', netif? netif->name[1] : ' ');
			}
		}
	}

	// add static entry
	if (argc == 4 && strcmp(argv[1], "-s") == 0) {
		char *ip = argv[2];
		char *mac = argv[3];
		struct eth_addr ethaddr;;
		ip4_addr_t ipaddr;
		ipaddr_aton(ip, &ipaddr);
		hwaddr_aton(mac, (uint8_t *)&ethaddr);
		etharp_add_static_entry(&ipaddr, &ethaddr);

		return ;
	}

	// remove static entry
	if (argc == 3 && strcmp(argv[1], "-d") == 0) {
		char *ip = argv[2];
		ip4_addr_t ipaddr;
		ipaddr_aton(ip, &ipaddr);

		etharp_remove_static_entry(&ipaddr);
	}
}
#endif

#if 0// CONFIG_IPV6
static void ip6_cmd_show_ip(int ifx)
{
	if (ifx == NETIF_IF_STA || ifx == NETIF_IF_AP) {
		bk_netif_get_ip6_addr_info(ifx);
	} else {
		CLI_LOGI("[sta]\n");
		bk_netif_get_ip6_addr_info(NETIF_IF_STA);
		CLI_LOGI("[ap]\n");
		bk_netif_get_ip6_addr_info(NETIF_IF_AP);
	}
}

void cli_ip6_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ifx = NETIF_IF_COUNT;
	int ret = 0;
	char *msg = NULL;

	if (argc > 1) {
		if (os_strcmp("sta", argv[1]) == 0) {
			ifx = NETIF_IF_STA;
		} else if (os_strcmp("ap", argv[1]) == 0) {
			ifx = NETIF_IF_AP;
		} else {
			CLI_LOGE("invalid netif name\n");
			goto error;
		}
	}

	if (argc == 1) {
		ip6_cmd_show_ip(NETIF_IF_COUNT);
	} else if (argc == 2) {
		ip6_cmd_show_ip(ifx);
	}

	if (!ret) {
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}
#endif

void cli_dhcpc_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret =0;
	char *msg = NULL;

	ret = bk_netif_dhcpc_start(NETIF_IF_STA);
	CLI_LOGI("STA start dhcp client\n");

	if(ret == 0)
		msg = WIFI_CMD_RSP_SUCCEED;
	else
		msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

void arp_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	CLI_LOGI("arp_Command\r\n");

	msg = WIFI_CMD_RSP_SUCCEED;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

// Modified by TUYA Start
#if 0
void cli_ping_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_LWIP
	int ret = 0;
	char *msg = NULL;
	uint32_t cnt = 4;
	if (argc == 1) {
		CLI_LOGE("Please input: ping <host address>\n");
		goto error;
	}
	if (argc == 2 && (os_strcmp("--stop", argv[1]) == 0)) {
		ping_stop();
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}
	if (argc > 2)
		cnt = os_strtoul(argv[2], NULL, 10);

	CLI_LOGI("ping IP address:%s\n", argv[1]);
	ping_start(argv[1], cnt, 0);

	if (!ret) {
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;

#endif
}
#endif
// Modified by TUYA End

#if CONFIG_ALI_MQTT
extern void test_mqtt_start(const char *host_name, const char *username,
                     		const char *password, const char *topic);
void cli_ali_mqtt_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	int ret = 0;

	CLI_LOGI("start test mqtt...\n");
	if (argc == 4) {
		test_mqtt_start(argv[1], argv[2], argv[3], NULL);
	} else if (argc == 5) {
		test_mqtt_start(argv[1], argv[2], argv[3], argv[4]);
	} else {
		// mqttali 222.71.10.2 aclsemi ****** /aclsemi/bk7256/cmd/1234
		CLI_LOGE("usage: mqttali [host name|ip] [username] [password] [topic]\n");
		goto error;
	}

	if (!ret) {
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}

int mqtt_cmd_msg_send(char *topic, char *msg);

void cli_ali_mqtt_send_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;

	if (argc == 3) {
		ret = mqtt_cmd_msg_send(argv[1], argv[2]);
	} else {
		// mqttsend  /aclsemi/bk7256/cmd/1234  12345678999999999
		CLI_LOGE("usage: mqttsend [topic] [msg]\n");
		return;
	}

	CLI_LOGI("send mqtt topic...%d.\n", ret);
}
#endif

#if CONFIG_HTTP
extern void LITE_openlog(const char *ident);
extern void LITE_closelog(void);
void cli_http_debug_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32 http_log = 0;
	int ret = 0;
	char *msg = NULL;

	if (argc == 2) {
		http_log = os_strtoul(argv[1], NULL, 10);
		if (1 == http_log ) {
			LITE_openlog("http");
		} else {
			LITE_closelog();
		}
	} else {
		CLI_LOGE("usage: httplog [1|0].\n");
		goto error;
	}

	if (!ret) {
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}

error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}
#endif

__attribute__((weak)) void set_output_bitmap(uint32 output_bitmap);

void set_per_packet_info_output_bitmap(const char *bitmap)
{
	uint32 output_bitmap = os_strtoul(bitmap, NULL, 16);

	set_output_bitmap(output_bitmap);
	CLI_LOGI("set per_packet_info_output_bitmap:0x%x\n", output_bitmap);
}

void cli_per_packet_info_output_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;

	if (argc == 2) {
		set_per_packet_info_output_bitmap(argv[1]);
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	} else {
		CLI_LOGE("usage: per_packet_info [per_packet_info_output_bitmap(base 16)]\n");
		msg = WIFI_CMD_RSP_ERROR;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	}
}

#define NETIF_CMD_CNT (sizeof(s_netif_commands) / sizeof(struct cli_command))
static const struct cli_command s_netif_commands[] = {
#if !CONFIG_CLI_CODE_SIZE_OPTIMIZE_ENABLE
	{"ip", "ip [sta|ap][{ip}{mask}{gate}{dns}]", cli_ip_cmd},
	{"ipconfig", "ipconfig [sta|ap][{ip}{mask}{gate}{dns}]", cli_ip_cmd},
	{"dhcpc", "dhcpc", cli_dhcpc_cmd},
// Modified by TUYA Start
//	{"ping", "ping <ip>", cli_ping_cmd},
// Modified by TUYA End
#if CONFIG_LWIP
	{"arp", "arp [-s] [-d]<ip>", cli_arp_cmd},
#endif
#if 0//def CONFIG_IPV6
	{"ping6", "ping6 xxx", cli_ping_cmd},
	{"ip6", "ip6 [sta|ap][{ip}{state}]", cli_ip6_cmd},
#endif
#ifdef TCP_CLIENT_DEMO
	{"tcp_cont", "tcp_cont [ip] [port]", tcp_make_connect_server_command},
#endif

#if CONFIG_TCP_SERVER_TEST
	{"tcp_server", "tcp_server [ip] [port]", make_tcp_server_command },
#endif
#if CONFIG_ALI_MQTT
	{"mqttali", "ali mqtt test", cli_ali_mqtt_cmd},
	{"mqttsend", "mqttsend [topic] [msg]", cli_ali_mqtt_send_cmd},
#endif
#if CONFIG_OTA_HTTP
	{"httplog", "httplog [1|0].", cli_http_debug_cmd},
#endif
	{"per_packet_info", "per_packet_info [per_packet_info_output_bitmap(base 16)]", cli_per_packet_info_output_cmd},
#else
	{"ip", "ip [sta|ap][{ip}{mask}{gate}{dns}]", cli_ip_cmd},
// Modified by TUYA Start
//	{"ping", "ping <ip>", cli_ping_cmd},
// Modified by TUYA End
#endif

};

int cli_netif_init(void)
{
	return cli_register_commands(s_netif_commands, NETIF_CMD_CNT);
}

#endif //#if (CLI_CFG_NETIF == 1)
