#include <common/bk_include.h>
#include "rw_msg_tx.h"
#include <os/mem.h>
#include "bk_rw.h"
#include "bk_wifi.h"
#include "bk_wifi_types.h"
#include "common.h"

////#include <os/os.h>
#include "errno.h"
#include <common/bk_kernel_err.h>
#include "rw_ieee80211.h"
#include "wifi_v2.h"


//TODO should finally delete this file
extern struct mac_scan_result *scanu_search_by_ssid(struct mac_ssid const *ssid);

#ifdef CONFIG_SME
/*---------------------------------------------------------------------------*/
int sa_station_send_associate_cmd(ASSOC_PARAM_T *assoc_param)
{
	int ret;
	struct mac_scan_result *desired_ap_ptr;
	struct sm_assoc_cfm cfm;

	WIFI_LOGI("%s %d\n", __func__, __LINE__);

	if (0/*assoc_param->chan.freq*/) {
		/* for fast connect */
		assoc_param->chan.band = 0;
		assoc_param->chan.flags = 0;
		assoc_param->chan.tx_power = 10;
	} else {
		/* normal case */
		desired_ap_ptr = scanu_search_by_ssid((void *)&assoc_param->ssid);
		if (NULL == desired_ap_ptr)
			return -1;
		assoc_param->chan = *(desired_ap_ptr->chan);
		if (0 == assoc_param->chan.tx_power)
			assoc_param->chan.tx_power = 10;
	}

	ret = rw_msg_send_sm_assoc_req(assoc_param, &cfm);

	return ret;
}

#else /* !CONFIG_SME */

/*---------------------------------------------------------------------------*/
int sa_station_send_associate_cmd(CONNECT_PARAM_T *connect_param)
{
	int ret;
	struct mac_scan_result *desired_ap_ptr;
	struct sm_connect_cfm sm_connect_cfm;

	if (connect_param->chan.freq) {
		/* for fast connect */
		connect_param->chan.band = 0;
		connect_param->chan.flags = 0;
		connect_param->chan.tx_power = VIF_UNDEF_POWER;
	} else {
		/* normal case */
		desired_ap_ptr = scanu_search_by_ssid((void *)&connect_param->ssid);
		if (NULL == desired_ap_ptr)
			return -1;
		connect_param->chan = *(desired_ap_ptr->chan);
		if (0 == connect_param->chan.tx_power)
			connect_param->chan.tx_power = VIF_UNDEF_POWER;
	}

	if (rw_ieee80211_is_scan_rst_in_countrycode(rw_ieee80211_get_chan_id(connect_param->chan.freq)) == 0) {
		WIFI_LOGE("ch:%d not in countrycode\r\n", connect_param->chan.freq);
		return -1;
	}

	ret = rw_msg_send_sm_connect_req(connect_param, &sm_connect_cfm);
	if (ret)
		return ret;

	switch (sm_connect_cfm.status) {
	case CO_OK:
		ret = 0;
		break;

	case CO_BUSY:
		ret = BK_ERR_BUSY;
		break;

	case CO_OP_IN_PROGRESS:
		ret = BK_ERR_IN_PROGRESS;
		break;

	default:
		ret = BK_FAIL;
		break;
	}

	return ret;
}
#endif /*CONFIG_SME*/


/*---------------------------------------------------------------------------*/
static void sa_station_cfg80211_init(void)
{
	if (rwm_mgmt_is_vif_first_used() == NULL) {
		WIFI_LOGD("init 1st vif\n");
		BK_LOG_ON_ERR(rw_msg_send_reset());

#if CONFIG_WIFI6
		/* request lmac/umac features and handle dynamic parameters */
		BK_LOG_ON_ERR(rw_msg_send_version_req());

		BK_LOG_ON_ERR(rwnx_handle_dynparams());
#endif

		BK_LOG_ON_ERR(rw_msg_send_me_config_req());
		BK_LOG_ON_ERR(rw_msg_send_me_chan_config_req());
		BK_LOG_ON_ERR(rw_msg_send_start());
	}
}

#ifndef DISABLE_RECONNECT

void *reconnect_thread_handle = NULL;
uint32_t  reconnect_stack_size = 2000;

void sa_reconnect_main(void *arg)
{
	sa_station_init();
	WIFI_LOGI("sa_reconnect_main\r\n");

	rtos_delete_thread(NULL);
	reconnect_thread_handle = NULL;
}

void sa_reconnect_init(void)
{
	bk_err_t ret;
	return; // try it;
	if (NULL == reconnect_thread_handle) {
		ret = rtos_create_thread(&reconnect_thread_handle,
								 CONFIG_TASK_RECONNECT_PRIO,
								 "reconnect_thread",
								 (beken_thread_function_t)sa_reconnect_main,
								 (unsigned short)reconnect_stack_size,
								 (beken_thread_arg_t)0);
		BK_ASSERT(kNoErr == ret); /* ASSERT VERIFIED */
	} else
		WIFI_LOGI("sa_reconnect_init_strange\r\n");
}
#endif

void sa_station_init(void)
{
	sa_station_cfg80211_init();
}

void sa_station_uninit(void)
{
}

// eof
