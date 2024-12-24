#include <common/bk_include.h>
#include "bk_airkiss.h"
#include "airkiss.h"
#include <os/mem.h>
#include "lwip/sockets.h"
#include <modules/wifi.h>
#include "bk_wifi.h"

static int bk_airkiss_decode_status;
uint32_t airkiss_start_flag = 0;
static volatile uint8_t airkiss_current_channel;
beken_thread_t airkiss_thread_handle = NULL;
static beken_semaphore_t airkiss_decode_over_sema;
static airkiss_context_t *airkiss_context_ptr = NULL;
static beken_timer_t airkiss_switch_channel_timer;
static beken_timer_t airkiss_lock_channel_timer;
const airkiss_config_t airkiss_cfg_ptr = {
	(airkiss_memset_fn) &os_memset,
	(airkiss_memcpy_fn) &os_memcpy,
	(airkiss_memcmp_fn) &os_memcmp,
	(airkiss_printf_fn) &os_printf,
};

void bk_airkiss_set_start_flag(void)
{
	airkiss_start_flag = 1;
}

void bk_airkiss_clear_start_flag(void)
{
	airkiss_start_flag = 0;
}

uint32_t bk_airkiss_is_start(void)
{
	return airkiss_start_flag;
}

static void bk_airkiss_switch_channel_timer_handler(void *parameter)
{
	wifi_channel_t chan = {0};
	airkiss_current_channel ++;
	if (airkiss_current_channel > MAX_CHANNEL_NUM)
		airkiss_current_channel = 1;

	BK_AIRKISS_PRT("switch_channel_to_index:%d \n", airkiss_current_channel);
	chan.primary = airkiss_current_channel;
	BK_LOG_ON_ERR(bk_wifi_monitor_set_channel(&chan));
	airkiss_change_channel(airkiss_context_ptr);
}

static void bk_airkiss_lock_timeout_handler(void *parameter)
{
	int ret;

	BK_AIRKISS_PRT("\r\nairkiss_lock_timeout_handler\r\n");

	ret = airkiss_init(airkiss_context_ptr, &airkiss_cfg_ptr);
	if (ret != 0)
		BK_AIRKISS_PRT("airkiss_init_failed\r\n");

	rtos_start_timer(&airkiss_switch_channel_timer);
}

static int bk_airkiss_monitor_callback(const uint8_t *data, uint32_t len, const wifi_frame_info_t *user_data)
{
	bk_airkiss_decode_status = airkiss_recv(airkiss_context_ptr, data, len);
	if (bk_airkiss_decode_status == AIRKISS_STATUS_CHANNEL_LOCKED) {
		rtos_stop_timer(&airkiss_switch_channel_timer);
		BK_AIRKISS_PRT("lock_channel in %d \n", airkiss_current_channel);

		rtos_start_timer(&airkiss_lock_channel_timer);
	} else if (bk_airkiss_decode_status == AIRKISS_STATUS_COMPLETE) {
		rtos_stop_timer(&airkiss_lock_channel_timer);

		rtos_set_semaphore(&airkiss_decode_over_sema);
		BK_AIRKISS_PRT("AIRKISS_STATUS_COMPLETE \n");
	}
	return BK_OK;
}

uint32_t bk_airkiss_resource_destroy(void)
{
	BK_AIRKISS_PRT("bk_airkiss_resource_destroy\r\n");

	if (airkiss_context_ptr != NULL) {
		os_free(airkiss_context_ptr);
		airkiss_context_ptr = NULL;
	}

	rtos_stop_timer(&airkiss_switch_channel_timer);
	rtos_deinit_timer(&airkiss_switch_channel_timer);

	rtos_stop_timer(&airkiss_lock_channel_timer);
	rtos_deinit_timer(&airkiss_lock_channel_timer);

	if (airkiss_decode_over_sema) {
		rtos_deinit_semaphore(&airkiss_decode_over_sema);
		airkiss_decode_over_sema = 0;
	}

	return AIRKISS_SUCCESS;
}

uint32_t bk_airkiss_resource_create(void)
{
	int result = AIRKISS_SUCCESS;

	BK_AIRKISS_PRT("bk_airkiss_resource_create\r\n");
	result = rtos_init_timer(&airkiss_switch_channel_timer,
							 BK_AIRKISS_SWITCH_TIMER,
							 bk_airkiss_switch_channel_timer_handler,
							 (void *)0);
	BK_ASSERT(kNoErr == result);

	result = rtos_init_timer(&airkiss_lock_channel_timer,
							 BK_AIRKISS_CYCLE_TIMER,
							 bk_airkiss_lock_timeout_handler,
							 (void *)0);
	BK_ASSERT(kNoErr == result);

	if (NULL == airkiss_decode_over_sema) {
		result = rtos_init_semaphore(&airkiss_decode_over_sema, 1);
		BK_ASSERT(kNoErr == result);
	}

	airkiss_context_ptr = (airkiss_context_t *)os_malloc(sizeof(airkiss_context_t));
	if (!airkiss_context_ptr) {
		BK_AIRKISS_PRT("Malloc memory for airkiss context \n");
		result = AIRKISS_FAILURE;
		goto init_exit;
	}

	os_memset(airkiss_context_ptr, 0, sizeof(airkiss_context_t));

	result = airkiss_init(airkiss_context_ptr, &airkiss_cfg_ptr);
	if (result != 0) {
		BK_AIRKISS_PRT("Airkiss init failed!!\r\n");
		result = AIRKISS_FAILURE;
		goto init_exit;
	}

	BK_AIRKISS_PRT("Airkiss version: %s\r\n", airkiss_version());

init_exit:
	return result;
}

static void bk_airkiss_wifi_connect(airkiss_result_t *airkiss_result)
{
	BK_AIRKISS_PRT("wifi_connect_req %s, %s\n", airkiss_result->ssid, airkiss_result->pwd);
	BK_AIRKISS_PRT("wifi_connect_req %d, %d\n", airkiss_result->ssid_length, airkiss_result->pwd_length);

	demo_sta_app_init(airkiss_result->ssid, airkiss_result->pwd);
}

void bk_airkiss_start_udp_boardcast(u8 random_data)
{
	int err, i;
	int udp_broadcast_fd = -1;
	struct sockaddr_in remote_skt;

	BK_AIRKISS_PRT("bk_airkiss_start_udp_boardcast\n");
	udp_broadcast_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udp_broadcast_fd == -1) {
		BK_AIRKISS_WARN("Socket failed\r\n");
		return;
	}
	os_memset(&remote_skt, 0, sizeof(struct sockaddr_in));
	remote_skt.sin_family = AF_INET;
	remote_skt.sin_addr.s_addr = INADDR_BROADCAST;//INADDR_ANY;
	remote_skt.sin_port = htons(10000);

	i = MAX_UDP_RANDOM_SEND;
	while (i --) {
		BK_AIRKISS_PRT("udp-sendto:%d\r\n", i);

		err = sendto(udp_broadcast_fd, &random_data, 1, 0, (struct sockaddr *)&remote_skt, sizeof(remote_skt));
		rtos_delay_milliseconds(2);

		if (err == -1)
			BK_AIRKISS_WARN("send udp boardcast failed\r\n");
	}

	BK_AIRKISS_PRT("close socket\r\n");
	close(udp_broadcast_fd);
}

void bk_airkiss_decode_complete_handle(void)
{
	int8_t err, chunk = 0;
	airkiss_result_t airkiss_result;

	BK_AIRKISS_PRT("bk_airkiss_decode_complete_handle\r\n");
	err = airkiss_get_result(airkiss_context_ptr, &airkiss_result);
	if (0 == err) {
		BK_AIRKISS_PRT("airkiss_get_result() ok!\n");
		BK_AIRKISS_PRT(" ssid = %s \n pwd = %s \n ssid_length = %d \n pwd_length = %d \n, random = 0x%02x\r\n",
					   airkiss_result.ssid,
					   airkiss_result.pwd,
					   airkiss_result.ssid_length,
					   airkiss_result.pwd_length,
					   airkiss_result.random);
	}

	BK_LOG_ON_ERR(bk_wifi_monitor_stop());
	BK_LOG_ON_ERR(bk_wifi_monitor_register_cb(NULL));

	BK_AIRKISS_PRT("bk_airkiss_wifi_connecting\r\n");
	bk_airkiss_wifi_connect(&airkiss_result);
	BK_AIRKISS_PRT("bk_airkiss_wifi_connect\r\n");

	do {
		chunk ++;
		rtos_delay_milliseconds(1000);

		BK_AIRKISS_PRT("airkiss is waiting for network connection\r\n");
		if (chunk >= 30) {
			BK_AIRKISS_PRT("GET IP Time Out!!! \n");
			return;
		}
	} while (!wifi_netif_sta_is_got_ip());

	bk_airkiss_start_udp_boardcast(airkiss_result.random);
}

uint32_t bk_airkiss_get_decode_status(void)
{
	return bk_airkiss_decode_status;
}

uint32_t bk_airkiss_decode_isnot_over(void)
{
	return (AIRKISS_STATUS_COMPLETE != bk_airkiss_decode_status);
}

static void bk_airkiss_thread_entry(void *parameter)
{
	wifi_channel_t chan = {0};
	uint32_t ret;

	/*step 0: init resource of airkiss*/
	ret = bk_airkiss_resource_create();
	if (AIRKISS_FAILURE == ret)
		goto _exit;

	/*step 1: init the parameter of switching channel, and then monitor*/
	airkiss_current_channel = 1;
	chan.primary = airkiss_current_channel;
	BK_LOG_ON_ERR(bk_wifi_monitor_set_channel(&chan));
	BK_LOG_ON_ERR(bk_wifi_monitor_register_cb(bk_airkiss_monitor_callback));
	BK_LOG_ON_ERR(bk_wifi_monitor_start());

	rtos_start_timer(&airkiss_switch_channel_timer);

	/*step 2: waiting for airkiss decode completion, or timeout*/
	if (rtos_get_semaphore(airkiss_decode_over_sema, BK_AIRKISS_TIMEOUT) != kNoErr)
		BK_AIRKISS_PRT("airkiss_thread_timeout\r\n");

	/*step 3: handle decode phase*/
	bk_wifi_monitor_stop();
	if (AIRKISS_STATUS_COMPLETE == bk_airkiss_decode_status)
		bk_airkiss_decode_complete_handle();

_exit:
	bk_airkiss_clear_start_flag();
	bk_airkiss_resource_destroy();
	BK_AIRKISS_PRT("bk_airkiss_thread_entry exit:%d\r\n", rtos_get_free_heap_size());

	airkiss_thread_handle = NULL;
	rtos_delete_thread(NULL);
}

uint32_t bk_airkiss_init(void)
{
	bk_err_t ret;

	BK_AIRKISS_PRT("bk_airkiss_init:%d\r\n", rtos_get_free_heap_size());

	BK_LOG_ON_ERR(bk_wifi_monitor_stop());
	bk_airkiss_set_start_flag();

	if (NULL == airkiss_thread_handle) {
		ret = rtos_create_thread(&airkiss_thread_handle,
								 BK_AIRKISS_THD_PRIORITY,
								 "air_kiss",
								 (beken_thread_function_t)bk_airkiss_thread_entry,
								 2048,
								 (beken_thread_arg_t)0);
		if (ret != kNoErr) {
			BK_AIRKISS_PRT("Error: airkiss_start_process: %d\r\n", ret);

			return -1;
		}
	}

	return AIRKISS_SUCCESS;
}

uint32_t bk_airkiss_uninit(void)
{
	BK_AIRKISS_PRT("bk_airkiss_uninit\r\n");
	return AIRKISS_SUCCESS;
}

uint32_t bk_airkiss_stop(void)
{
	BK_AIRKISS_PRT("bk_airkiss_stop\r\n");

	if (rtos_is_timer_init(&airkiss_switch_channel_timer)
		&& rtos_is_timer_running(&airkiss_switch_channel_timer))
		rtos_stop_timer(&airkiss_switch_channel_timer);

	if (rtos_is_timer_init(&airkiss_lock_channel_timer)
		&& rtos_is_timer_running(&airkiss_lock_channel_timer))
		rtos_stop_timer(&airkiss_lock_channel_timer);

	if (bk_airkiss_is_start() && bk_airkiss_decode_isnot_over())
		rtos_set_semaphore(&airkiss_decode_over_sema);

	return AIRKISS_SUCCESS;
}

uint32_t bk_airkiss_start(void)
{
	BK_AIRKISS_PRT("bk_airkiss_start\r\n");
	return bk_airkiss_init();
}

uint32_t bk_airkiss_process(uint32_t start)
{
	uint32_t ret;

	if (start)
		ret = bk_airkiss_start();
	else
		ret = bk_airkiss_stop();

	return ret;
}

// eof

