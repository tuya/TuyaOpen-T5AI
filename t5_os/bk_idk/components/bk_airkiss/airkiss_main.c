#include <common/bk_include.h>
#include "bk_arm_arch.h"
#include "bk_misc.h"
#include <os/mem.h>
#include "bk_uart.h"
#include "sys_rtos.h"
#include <os/os.h>
#include <common/bk_kernel_err.h>
#include "bk_fake_clock.h"
#include "lwip/sockets.h"
#include "airkiss.h"
#include "airkiss_pingpong.h"
#include "airkiss_main.h"
//#include "flash.h"
#include "bk_wifi.h"
#include "bk_wifi_types.h"
#include "common/bk_err.h"
#include "components/event.h"
#include "components/netif_types.h"
#include <modules/wifi.h>

static airkiss_context_t *ak_contex = NULL;
const airkiss_config_t ak_conf = {
	(airkiss_memset_fn) &os_memset,
	(airkiss_memcpy_fn) &os_memcpy,
	(airkiss_memcmp_fn) &os_memcmp,
	(airkiss_printf_fn) &AIRKISS_PRT
};

beken_timer_t ak_chan_timer;
beken_timer_t ak_doing_timer;
beken_thread_t ak_thread_handle = NULL;
beken_semaphore_t ak_semaphore = NULL;
beken_semaphore_t ak_connect_semaphore = NULL;
airkiss_channel_t g_chans;
airkiss_mac_t g_macs;
volatile u8 airkiss_exit = 0;
u8 *read_buf = NULL;

extern void net_set_sta_ipup_callback(void *fn);

static unsigned char calcrc_1byte(unsigned char abyte)
{
	unsigned char i, crc_1byte;

	crc_1byte = 0;
	for (i = 0; i < 8; i++) {
		if (((crc_1byte ^ abyte) & 0x01)) {
			crc_1byte ^= 0x18;
			crc_1byte >>= 1;
			crc_1byte |= 0x80;
		} else
			crc_1byte >>= 1;
		abyte >>= 1;
	}
	return crc_1byte;
}

static unsigned char calcrc_bytes(unsigned char *p, unsigned int num_of_bytes)
{
	unsigned char crc = 0;
	while (num_of_bytes--)
		crc = calcrc_1byte(crc ^ *p++);
	return crc;
}
void airkiss_count_usefull_packet(const unsigned char *frame, int size)
{
	u8 mac_crc = 0;
	u8 *mac_ptr = 0;
	u16 channel = 0;
	uint32_t elmt_addr, var_part_addr, var_part_len;
	int i;
	struct wifi_mac_hdr *fwifi_mac_hdr = (struct wifi_mac_hdr *)frame;
	struct wifi_bcn_frame const *frm = (struct wifi_bcn_frame const *)frame;
	chan_param_t *cur_chan = &g_chans.chan[g_chans.cur_chan_idx];

	mac_ptr = (u8 *)&fwifi_mac_hdr->addr2;
	mac_crc = calcrc_bytes(mac_ptr, 6);
	if (!frame || !size)
		return;

	if ((MAC_FCTRL_BEACON == (fwifi_mac_hdr->fctl & MAC_FCTRL_TYPESUBTYPE_MASK))
		|| (MAC_FCTRL_PROBERSP == (fwifi_mac_hdr->fctl & MAC_FCTRL_TYPESUBTYPE_MASK))) {
		cur_chan->bcn_cnt++;
		var_part_addr = (uint32_t)frm->variable;
		var_part_len = size - MAC_BEACON_VARIABLE_PART_OFT;
		elmt_addr = bk_wifi_find_ie(var_part_addr, var_part_len, MAC_ELTID_DS);
		if (elmt_addr != 0)
			channel = *(uint8_t*)(elmt_addr + MAC_DS_CHANNEL_OFT);

		for (i = 0; i < g_macs.mac_cnt; i++) {
			if ((mac_crc == g_macs.mac[i].mac_crc)) {
				if (channel != 0)
					g_macs.mac[i].channel = channel;
				break;
			}
		}

		if (i == g_macs.mac_cnt) {
			g_macs.mac[i].mac_crc = mac_crc;
			if (channel != 0)
				g_macs.mac[i].channel = channel;
			g_macs.mac_cnt++;
		}
	} else if (MAC_FCTRL_DATA_T == (fwifi_mac_hdr->fctl & MAC_FCTRL_TYPE_MASK))
		cur_chan->data_cnt++;
}

void airkiss_set_scan_all_channel(void)
{
	int i;
	os_memset(&g_chans, 0, sizeof(airkiss_channel_t));
	os_memset(&g_macs, 0, sizeof(airkiss_mac_t));

	g_chans.all_chan_nums = MAX_CHANNELS - 1;
	for (i = 0; i < g_chans.all_chan_nums; i++)
		g_chans.chan[i].channel = i + 1;

	g_chans.mode = AIRKISS_SCAN_ALL_CHAN;
	g_chans.cur_chan_idx = 0;

	AIRKISS_WARN("change to all scan mode\r\n");
}

u32 airkiss_calc_time_for_selected_channel(u8 valid_cnt)
{
	u32 timer_cnt;

	timer_cnt = (valid_cnt / MIN_VALID_DATACNT_INCHAN) * MIN_SEL_CHAN_TIMER;

	if (timer_cnt < MIN_SEL_CHAN_TIMER)
		return MIN_SEL_CHAN_TIMER;
	else if (timer_cnt > MAX_SEL_CHAN_TIMER)
		return MAX_SEL_CHAN_TIMER;

	return timer_cnt;
}

void airkiss_add_channel(u8 channel, u8 bcn_cnt, u8 data_cnt)
{
	int i;

	if (data_cnt < MIN_VALID_DATACNT_INCHAN)
		return;
	if (bcn_cnt < MIN_VALID_BCNCNT_INCHAN)
		return;

	for (i = 0; i < g_chans.selected_chan_nums; i++) {
		u8 cur_cnt = g_chans.chan[i].data_cnt;
		u8 cur_chan = g_chans.chan[i].channel;
		if (cur_cnt < data_cnt) {
			if (cur_chan != channel) {
				u8 move_cnt = g_chans.selected_chan_nums - i;
				os_memmove(&g_chans.chan[i + 1], &g_chans.chan[i], move_cnt * sizeof(chan_param_t));
				g_chans.selected_chan_nums += 1;
			}

			g_chans.chan[i].channel = channel;
			g_chans.chan[i].data_cnt = data_cnt;
			return;
		}
	}

	if (i == g_chans.selected_chan_nums) {
		g_chans.selected_chan_nums ++;
		g_chans.chan[i].channel = channel;
		g_chans.chan[i].data_cnt = data_cnt;
	}

}

void airkiss_switch_channel_callback(void *data)
{
	int ret;
	u8 bcn_cnt = 0,  data_cnt = 0;
	u32 timer_cnt = 0;
	u8 channel = 0;
	chan_param_t *cur_chan = &g_chans.chan[g_chans.cur_chan_idx];
	wifi_channel_t chan = {0};

	bcn_cnt = cur_chan->bcn_cnt;
	data_cnt = cur_chan->data_cnt;
	channel = cur_chan->channel;
	AIRKISS_PRT("finish scan ch:%02d, bcn:%03d, data:%03d\r\n",
				channel, bcn_cnt, data_cnt);
	AIRKISS_PRT("\r\n");

	switch (g_chans.mode) {
	case AIRKISS_SCAN_ALL_CHAN:
		airkiss_add_channel(channel, bcn_cnt, data_cnt);

		g_chans.cur_chan_idx++;
		timer_cnt = AIRKISS_SWITCH_TIMER;

		if (g_chans.cur_chan_idx >= g_chans.all_chan_nums) {
			g_chans.cur_chan_idx = 0;
			if (g_chans.selected_chan_nums) {
				g_chans.mode = AIRKISS_SCAN_SELECTED_CHAN;
				data_cnt = g_chans.chan[g_chans.cur_chan_idx].data_cnt;
				timer_cnt = airkiss_calc_time_for_selected_channel(data_cnt);
				AIRKISS_WARN("change to selected scan mode\r\n");
			} else
				airkiss_set_scan_all_channel();
		}
		break;

	case AIRKISS_SCAN_SELECTED_CHAN:
		g_chans.cur_chan_idx++;
		data_cnt = g_chans.chan[g_chans.cur_chan_idx].data_cnt;
		timer_cnt = airkiss_calc_time_for_selected_channel(data_cnt);

		if (g_chans.cur_chan_idx >= g_chans.selected_chan_nums) {
			g_chans.cur_chan_idx = 0;
			airkiss_set_scan_all_channel();
			g_chans.mode = AIRKISS_SCAN_ALL_CHAN;
			timer_cnt = AIRKISS_SWITCH_TIMER;
		}
		break;

	default:
		AIRKISS_WARN("unknow state:%d\r\n", g_chans.mode);
		g_chans.mode = AIRKISS_SCAN_ALL_CHAN;
		return;
		break;
	}

	channel = g_chans.chan[g_chans.cur_chan_idx].channel;

	AIRKISS_PRT("start scan ch:%02d/%02d, time_intval:%d\r\n", g_chans.cur_chan_idx, channel, timer_cnt);
	chan.primary = channel;
	BK_LOG_ON_ERR(bk_wifi_monitor_set_channel(&chan));
	airkiss_change_channel(ak_contex);

	if (!airkiss_exit) {
		ret = rtos_change_period(&ak_chan_timer, timer_cnt);
		BK_ASSERT(kNoErr == ret);
	}

}

void airkiss_doing_timeout_callback(void *data)
{
	wifi_channel_t chan = {0};
	int ret;

	AIRKISS_WARN("airkiss_doing_timeout, restart channel switch timer\r\n");

	// stop doing timer
	ret = rtos_stop_timer(&ak_doing_timer);
	BK_ASSERT(kNoErr == ret);

	airkiss_change_channel(ak_contex);

	// restart scan process
	ret = airkiss_init(ak_contex, &ak_conf);
	if (0 != ret)
		AIRKISS_FATAL("re-airkiss init failed!!\r\n");

	airkiss_set_scan_all_channel();
	g_chans.cur_chan_idx = 0;  // set channel 1
	chan.primary = g_chans.chan[g_chans.cur_chan_idx].channel;
	BK_LOG_ON_ERR(bk_wifi_monitor_set_channel(&chan));
	if (!airkiss_exit) {
		ret = rtos_change_period(&ak_chan_timer, AIRKISS_SWITCH_TIMER);
		BK_ASSERT(kNoErr == ret);
	}
}

bk_err_t airkiss_monitor_callback(const uint8_t *data, uint32_t len, const wifi_frame_info_t *info)
{
	GLOBAL_INT_DECLARATION();

	if (len < AIRKISS_MIN_RX_BUF_SIZE)
		return BK_ERR_PARAM;

	airkiss_count_usefull_packet(data, len);

	GLOBAL_INT_DISABLE();
	write_to_pingpong_buf((uint8_t *)data, AIRKISS_MIN_RX_BUF_SIZE, len);
	GLOBAL_INT_RESTORE();

	if (ak_semaphore)
		rtos_set_semaphore(&ak_semaphore);
	return BK_OK;
}

int process_airkiss(const unsigned char *packet, int size)
{
	int ret, result, i;
	u8 mac_crc = 0;
	u8 *mac_ptr = 0;
	struct wifi_mac_hdr *fwifi_mac_hdr = (struct wifi_mac_hdr *)packet;
	wifi_channel_t chan = {0};

	mac_ptr = (u8 *)&fwifi_mac_hdr->addr2;
	mac_crc = calcrc_bytes(mac_ptr, 6);

	ret = airkiss_recv(ak_contex, (void *)packet, size);
	if (ret == AIRKISS_STATUS_CONTINUE) {
	} else if (ret == AIRKISS_STATUS_CHANNEL_LOCKED) {
		result = rtos_stop_timer(&ak_chan_timer);
		BK_ASSERT(kNoErr == result);
		for (i = 0; i < g_macs.mac_cnt; i++) {
			if ((mac_crc == g_macs.mac[i].mac_crc)) {
				if (g_chans.chan[g_chans.cur_chan_idx].channel != g_macs.mac[i].channel) {
					g_chans.chan[g_chans.cur_chan_idx].channel = g_macs.mac[i].channel;
					chan.primary = g_chans.chan[g_chans.cur_chan_idx].channel;
					BK_LOG_ON_ERR(bk_wifi_monitor_set_channel(&chan));
				}
			}
		}

		AIRKISS_WARN("Lock channel in %d\r\n", g_chans.chan[g_chans.cur_chan_idx].channel);
		AIRKISS_WARN("start airkiss doing timer\r\n");

		result = rtos_start_timer(&ak_doing_timer);
		BK_ASSERT(kNoErr == result);
	} else if (ret == AIRKISS_STATUS_COMPLETE) {
		result = rtos_stop_timer(&ak_doing_timer);
		BK_ASSERT(kNoErr == result);
	}

	return ret;
}

int airkiss_connected_to_bssid(void *arg, event_module_t event_module,
					   int event_id, void *event_data)
{
	if (ak_connect_semaphore)
		rtos_set_semaphore(&ak_connect_semaphore);

	return BK_OK;
}

void airkiss_start_udp_boardcast(u8 random_data)
{
	int err, i;

	int udp_broadcast_fd = -1;
	struct sockaddr_in remote_skt;

	udp_broadcast_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udp_broadcast_fd == -1) {
		AIRKISS_WARN("Socket failed\r\n");
		return;
	}
	os_memset(&remote_skt, 0, sizeof(struct sockaddr_in));
	remote_skt.sin_family = AF_INET;
	remote_skt.sin_addr.s_addr = INADDR_BROADCAST;//INADDR_ANY;
	remote_skt.sin_port = htons(10000);

	i = MIN_UDP_RANDOM_SEND;
	while (i--) {
		err = sendto(udp_broadcast_fd, &random_data, 1, 0, (struct sockaddr *)&remote_skt, sizeof(remote_skt));
		rtos_delay_milliseconds(2);
		if (err == -1)
			AIRKISS_WARN("send udp boardcast failed\r\n");
	}

	close(udp_broadcast_fd);
}

void airkiss_main(void *arg)
{
	int result;
	u32 con_time;
	airkiss_result_t ak_result;
	u8 *airkiss_read_buf = NULL;
	wifi_channel_t chan = {0};

	result = pingpong_init();
	if (0 != result) {
		AIRKISS_FATAL("Airkiss pingpong_init failed!!\r\n");
		goto kiss_exit;
	}

	result = rtos_init_timer(&ak_chan_timer,
							 AIRKISS_SWITCH_TIMER,
							 airkiss_switch_channel_callback,
							 (void *)0);
	BK_ASSERT(kNoErr == result);

	result = rtos_init_timer(&ak_doing_timer,
							 AIRKISS_DOING_TIMER,
							 airkiss_doing_timeout_callback,
							 (void *)0);
	BK_ASSERT(kNoErr == result);

	ak_contex = (airkiss_context_t *)os_malloc(sizeof(airkiss_context_t));
	airkiss_read_buf = (u8 *)os_malloc(sizeof(u8) * AIRKISS_MIN_RX_BUF_SIZE);
	if ((!ak_contex) || (!airkiss_read_buf)) {
		AIRKISS_FATAL("Airkiss no buffer\r\n");
		goto kiss_exit;
	}

	read_buf = airkiss_read_buf;

	result = airkiss_init(ak_contex, &ak_conf);
	if (result != 0) {
		AIRKISS_FATAL("Airkiss init failed!!\r\n");
		goto kiss_exit;
	}

	AIRKISS_WARN("Airkiss version: %s\r\n", airkiss_version());

	BK_LOG_ON_ERR(bk_wifi_monitor_stop());
	BK_LOG_ON_ERR(bk_wifi_monitor_register_cb(airkiss_monitor_callback));
	BK_LOG_ON_ERR(bk_wifi_monitor_start());

	// start from first channel
	airkiss_set_scan_all_channel();
	g_chans.cur_chan_idx = 0;  // set channel 1
	chan.primary = g_chans.chan[g_chans.cur_chan_idx].channel;
	chan.second = WIFI_SECOND_CHANNEL_NONE;
	BK_LOG_ON_ERR(bk_wifi_monitor_set_channel(&chan));

	result = rtos_start_timer(&ak_chan_timer);
	BK_ASSERT(kNoErr == result);

	airkiss_exit = 0;
	ak_result.ssid = NULL;

	while (0 == airkiss_exit) {
		uint32_t actual;

		result = rtos_get_semaphore(&ak_semaphore, BEKEN_WAIT_FOREVER);

		// count received packet
		actual = 0;
		read_from_pingpong_buf(read_buf, AIRKISS_MIN_RX_BUF_SIZE, &actual);

		if (g_chans.mode == AIRKISS_SCAN_SELECTED_CHAN) {
			if (AIRKISS_STATUS_COMPLETE == process_airkiss(airkiss_read_buf, actual)) {
				AIRKISS_WARN("Airkiss completed.\r\n");
				airkiss_get_result(ak_contex, &ak_result);

				AIRKISS_WARN("Result:\r\n");
				AIRKISS_WARN("ssid:[%s]\r\n", ak_result.ssid);
				AIRKISS_WARN("ssid_len:[%d]\r\n", ak_result.ssid_length);
				AIRKISS_WARN("ssid_crc:[%x]\r\n", ak_result.reserved);
				AIRKISS_WARN("key:[%s]\r\n", ak_result.pwd);
				AIRKISS_WARN("key_len:[%d]\r\n", ak_result.pwd_length);
				AIRKISS_WARN("random:[0x%02x]\r\n", ak_result.random);

				break;
			}
		}
	}

	// stop monitor mode
	BK_LOG_ON_ERR(bk_wifi_monitor_stop());
	BK_LOG_ON_ERR(bk_wifi_monitor_register_cb(NULL));

	if (ak_result.ssid) {
		if (ak_connect_semaphore == NULL) {
			result = rtos_init_semaphore(&ak_connect_semaphore, 1);
			BK_ASSERT(kNoErr == result);
		}

		// register GOT_IP4 event
		bk_event_register_cb(EVENT_MOD_NETIF, EVENT_NETIF_GOT_IP4,
				airkiss_connected_to_bssid, NULL);

		// connect to this bssid
		demo_sta_app_init(ak_result.ssid, ak_result.pwd);

		// wait for connect to bssid
		con_time = AIRKISS_CONNECT_TIMER;
		result = rtos_get_semaphore(&ak_connect_semaphore, con_time);
		if (result == kNoErr) {
			// start udp boardcast
			if (airkiss_exit)
				BK_LOG_ON_ERR(bk_wifi_sta_stop());
			else
				airkiss_start_udp_boardcast(ak_result.random);
		} else {
			BK_LOG_ON_ERR(bk_wifi_sta_stop());
			AIRKISS_FATAL("airkiss connect to bssid timeout\r\n");
		}

		// unregister GO_IP4 event
		bk_event_unregister_cb(EVENT_MOD_NETIF, EVENT_NETIF_GOT_IP4,
					airkiss_connected_to_bssid);

		rtos_deinit_semaphore(&ak_connect_semaphore);
		ak_connect_semaphore = NULL;
	}

kiss_exit:
	AIRKISS_WARN("Airkiss exit.\r\n");

	result = rtos_stop_timer(&ak_chan_timer);
	BK_ASSERT(kNoErr == result);
	result = rtos_stop_timer(&ak_doing_timer);
	BK_ASSERT(kNoErr == result);

	do {
		rtos_delay_milliseconds(10);
	} while (rtos_is_timer_running(&ak_chan_timer) || (rtos_is_timer_running(&ak_doing_timer)));

	result = rtos_deinit_timer(&ak_chan_timer);
	BK_ASSERT(kNoErr == result);

	result = rtos_deinit_timer(&ak_doing_timer);
	BK_ASSERT(kNoErr == result);

	rtos_deinit_semaphore(&ak_semaphore);
	ak_semaphore = NULL;

	if (ak_contex) {
		os_free(ak_contex);
		ak_contex = NULL;
	}

	if (airkiss_read_buf) {
		os_free(airkiss_read_buf);
		airkiss_read_buf = NULL;
	}

	ak_thread_handle = NULL;
	pingpong_free();
	rtos_delete_thread(NULL);
}

uint32_t airkiss_is_at_its_context(void)
{
	return (NULL != ak_thread_handle);
}

uint32_t airkiss_start(void)
{
	uint32_t ret = kNoErr;

	if (NULL == ak_semaphore) {
		ret = rtos_init_semaphore(&ak_semaphore, 1);
		BK_ASSERT(kNoErr == ret);
	}

	if (NULL == ak_thread_handle) {
		ret = rtos_create_thread(&ak_thread_handle,
								 BEKEN_DEFAULT_WORKER_PRIORITY,
								 "airkiss",
								 (beken_thread_function_t)airkiss_main,
								 4096,
								 (beken_thread_arg_t)0);
		if (ret != kNoErr) {
			AIRKISS_FATAL("Error: airkiss_start_process: %d\r\n", ret);
			ret = kGeneralErr;
		}
	}

	return ret;
}

uint32_t airkiss_stop(void)
{
	GLOBAL_INT_DECLARATION();

	if (ak_thread_handle && ak_semaphore) {
		GLOBAL_INT_DISABLE();
		airkiss_exit = 1;
		GLOBAL_INT_RESTORE();

		if (ak_connect_semaphore)
			rtos_set_semaphore(&ak_connect_semaphore);
	}

	return kNoErr;
}

u32 airkiss_process(u8 start)
{
	int ret;

	AIRKISS_FATAL("airkiss_process:%d\r\n", start);

	if (start)
		ret = airkiss_start();
	else
		ret = airkiss_stop();

	return ret;
}
// eof

