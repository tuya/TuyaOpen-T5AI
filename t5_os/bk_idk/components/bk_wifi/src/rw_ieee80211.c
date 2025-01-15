#include <common/bk_include.h>
#include "rw_ieee80211.h"
#include "bk_rw.h"
#include "rw_msdu.h"
#include <os/mem.h>
#include <os/str.h>
#include "rwnx_intf.h"
#include "rwnx_defs.h"
#include "rwnx_rx.h"
#include "rwnx_params.h"
#include "bk_workqueue.h"
#include "ctrl_iface.h"
#include "rwnx_defs.h"
#include "components/ate.h"
#if CONFIG_RWNX_TD
#include "rwnx_td.h"
#endif

#if CONFIG_POWER_TABLE
//#include "bk_pwr_tbl.h"
#endif
#include <modules/wifi.h>
#include "bk_wifi.h"
#include "bk_wifi_prop_private.h"
#include "reg_domain.h"

typedef struct _wifi_cn_code_st_ {
	UINT32 init;
	wifi_country_t cfg;
} WIFI_CN_ST, *WIFI_CN_PTR;

WIFI_CN_ST g_country_code = {0};

#define COUNTRY_CODE_CN   {.cc= "CN", .schan=1, .nchan=13, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL};
#define COUNTRY_CODE_US   {.cc= "US", .schan=1, .nchan=11, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL};
#define COUNTRY_CODE_EP   {.cc= "EP", .schan=1, .nchan=13, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL};
#define COUNTRY_CODE_JP   {.cc= "JP", .schan=1, .nchan=14, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL};
#define COUNTRY_CODE_AU   {.cc= "AU", .schan=1, .nchan=13, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL};

static struct ieee80211_channel rw_2ghz_channels[] = {
	CHAN2G(1, 2412, 0),
	CHAN2G(2, 2417, 0),
	CHAN2G(3, 2422, 0),
	CHAN2G(4, 2427, 0),
	CHAN2G(5, 2432, 0),
	CHAN2G(6, 2437, 0),
	CHAN2G(7, 2442, 0),
	CHAN2G(8, 2447, 0),
	CHAN2G(9, 2452, 0),
	CHAN2G(10, 2457, 0),
	CHAN2G(11, 2462, 0),
	CHAN2G(12, 2467, 0),
	CHAN2G(13, 2472, 0),
	CHAN2G(14, 2484, 0),
};

// Total MAC_DOMAINCHANNEL_5G_MAX 5G channels
struct ieee80211_channel rw_5ghz_channels[] = {
	CHAN5G(36, 0),
	CHAN5G(40, 0),
	CHAN5G(44, 0),
	CHAN5G(48, 0),
	CHAN5G(52, 0),
	CHAN5G(56, 0),
	CHAN5G(60, 0),
	CHAN5G(64, 0),
	CHAN5G(100, 0),
	CHAN5G(104, 0),
	CHAN5G(108, 0),
	CHAN5G(112, 0),
	CHAN5G(116, 0),
	CHAN5G(120, 0),
	CHAN5G(124, 0),
	CHAN5G(128, 0),
	CHAN5G(132, 0),
	CHAN5G(136, 0),
	CHAN5G(140, 0),
	CHAN5G(144, 0),
	CHAN5G(149, 0),
	CHAN5G(153, 0),
	CHAN5G(157, 0),
	CHAN5G(161, 0),
	CHAN5G(165, 0),
};

#if CONFIG_WIFI_BAND_5G
//struct for channels list
typedef struct {
	char countrycode[3];
	int channels_5g[26];
	int num_channels;
} Countrychannels;

Countrychannels non_radar_avaliable_channel_table[] = {
	{"CN",  {36, 40, 44, 48, 149, 153, 157, 161, 165}, 9},
#if 0
	{"US", {36, 40, 44, 48, 149, 153, 157, 161, 165}, 9},
	{"CAN", {36, 40, 44, 48, 149, 153, 157, 161, 165}, 9},
	{"EP",  {36, 40, 44, 48, 149, 153, 157, 161, 165}, 9},
	{"SWI", {36, 40, 44, 48, 149, 153, 157, 161, 165}, 9},
	{"RU",  {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132,\
			136, 140, 144, 149, 153, 157, 161, 165}, 25},
	{"JP",  {36, 40, 44, 48}, 4},
	{"SIN", {36, 40, 44, 48, 149, 153, 157, 161, 165}, 9},
	{"TAI", {36, 40, 44, 48, 149, 153, 157, 161, 165}, 9},
	{"ISR", {36, 40, 44, 48}, 4},
	{"KOR", {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 149,\
			153, 157, 161, 165}, 21},
	{"TUR", {36, 40, 44, 48, 52, 56, 60, 64}, 8},
	{"AU", {36, 40, 44, 48, 149, 153, 157, 161, 165}, 9},
	{"SAF", {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132,\
			136, 140}, 19},
	{"BRA", {36, 40, 44, 48, 52, 56, 60, 64, 149, 153, 157, 161, 165}, 13},
	{"NZE", {36, 40, 44, 48, 149, 153, 157, 161, 165}, 9},
#endif
};

//define 5g channels list for country
Countrychannels country_channel_table[] = {
	{"CN",  {36, 40, 44, 48, 52, 56, 60, 64, 149, 153, 157, 161, 165}, 13},
#if 0
	{"US", {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, \
			136, 140, 144, 149, 153, 157, 161, 165}, 25},
	{"CAN", {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 132, 136, 140, 144,\
			149, 153, 157, 161, 165}, 22},
	{"EP",  {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132,\
			136, 140, 149, 153, 157, 161, 165}, 24},
	{"SWI", {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132,\
			136, 140, 149, 153, 157, 161, 165}, 24},
	{"RU",  {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132,\
			136, 140, 144, 149, 153, 157, 161, 165}, 25},
	{"JP",  {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132,\
			136, 140}, 19},
	{"SIN", {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132,\
			136, 140, 144, 149, 153, 157, 161, 165}, 25},
	{"TAI", {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132,\
			136, 140, 149, 153, 157, 161, 165}, 24},
	{"ISR", {36, 40, 44, 48, 52, 56, 60, 64}, 8},
	{"KOR", {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 149,\
			153, 157, 161, 165}, 21},
	{"TUR", {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 132, 136, 140, 144,\
			149, 153, 157, 161, 165}, 22},
	{"AU", {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 132, 136, 140, 144, \
			149, 153, 157, 161, 165}, 22},
	{"SAF", {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132,\
			136, 140}, 19},
	{"BRA", {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132,\
			136, 140, 149, 153, 157, 161, 165}, 24},
	{"NZE", {36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132,\
			136, 140, 144, 149, 153, 157, 161, 165}, 25},
#endif
};
#endif //CONFIG_WIFI_BAND_5G

struct wiphy g_wiphy;
struct rwnx_hw g_rwnx_hw;

unsigned char beacon[149] = {
	0x80, 0x00, // Frame Control
	0x00, 0x00, // Duration
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Addr1
	0x12, 0x71, 0x11, 0x71, 0x0B, 0x71, // Addr2
	0x12, 0x71, 0x11, 0x71, 0x0B, 0x71, // Addr3
	0x00, 0x00, // SN
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Timestamp
	0x64, 0x00, // Beacon Interval
	0x31, 0x04, // Capabilities
	0x00, 0x05, 0x4A, 0x49, 0x45, 0x57, 0x55, // SSID
	0x01, 0x08, 0x82, 0x84, 0x8B, 0x96, 0x0C, 0x12, 0x18, 0x24, // Supported rates
	0x03, 0x01, 0x06, // DS
	0x05, 0x04, 0x00, 0x02, 0x00, 0x00, // TIM
	0x2A, 0x01, 0x00, // ERP
	0x32, 0x04, 0x30, 0x48, 0x60, 0x6C, // Extended Supported Rates
	0x2D, 0x1A, 0x0C, 0x00, 0x1B, 0xFF, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x96,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // HT Capabilities
	0x3D, 0x16, 0x08, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // HT Information
	0xDD, 0x18, 0x00, 0x50, 0xF2, 0x02, 0x01, 0x01, 0x00, 0x00, 0x03, 0xA4, 0x00, 0x00, 0x27, 0xA4,
	0x00, 0x00, 0x42, 0x43, 0x5E, 0x00, 0x62, 0x32, 0x2F, 0x00 // WMM
};

#define RWNX_HT_CAPABILITIES                                    \
	{                                                               \
		.ht_supported   = true,                                     \
						  .cap            = 0,                                        \
		.ampdu_factor   = IEEE80211_HT_MAX_AMPDU_8K,                \
		.ampdu_density  = IEEE80211_HT_MPDU_DENSITY_16,             \
		.mcs        = {                                             \
		.rx_mask = { 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, },        \
		.rx_highest = cpu_to_le16(65),                          \
		.tx_params = IEEE80211_HT_MCS_TX_DEFINED,               \
		},                                                          \
	}

#define RWNX_VHT_CAPABILITIES                                   \
	{                                                               \
		.vht_supported = false,                                     \
		.cap       =                                                \
			(3 << IEEE80211_VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_SHIFT),\
		.vht_mcs       = {                                          \
		.rx_mcs_map = cpu_to_le16(                              \
			IEEE80211_VHT_MCS_SUPPORT_0_9    << 0  |  \
			IEEE80211_VHT_MCS_NOT_SUPPORTED  << 2  |  \
			IEEE80211_VHT_MCS_NOT_SUPPORTED  << 4  |  \
			IEEE80211_VHT_MCS_NOT_SUPPORTED  << 6  |  \
			IEEE80211_VHT_MCS_NOT_SUPPORTED  << 8  |  \
			IEEE80211_VHT_MCS_NOT_SUPPORTED  << 10 |  \
			IEEE80211_VHT_MCS_NOT_SUPPORTED  << 12 |  \
			IEEE80211_VHT_MCS_NOT_SUPPORTED  << 14),  \
		.tx_mcs_map = cpu_to_le16(                              \
			IEEE80211_VHT_MCS_SUPPORT_0_9    << 0  |  \
			IEEE80211_VHT_MCS_NOT_SUPPORTED  << 2  |  \
			IEEE80211_VHT_MCS_NOT_SUPPORTED  << 4  |  \
			IEEE80211_VHT_MCS_NOT_SUPPORTED  << 6  |  \
			IEEE80211_VHT_MCS_NOT_SUPPORTED  << 8  |  \
			IEEE80211_VHT_MCS_NOT_SUPPORTED  << 10 |  \
			IEEE80211_VHT_MCS_NOT_SUPPORTED  << 12 |  \
			IEEE80211_VHT_MCS_NOT_SUPPORTED  << 14),  \
		}                                                           \
	}

#define RWNX_HE_CAPABILITIES                                    \
	{                                                               \
		.has_he = false,                                            \
		.he_cap_elem = {                                            \
			.mac_cap_info[0] = 0,                                   \
			.mac_cap_info[1] = 0,                                   \
			.mac_cap_info[2] = 0,                                   \
			.mac_cap_info[3] = 0,                                   \
			.mac_cap_info[4] = 0,                                   \
			.mac_cap_info[5] = 0,                                   \
			.phy_cap_info[0] = 0,                                   \
			.phy_cap_info[1] = 0,                                   \
			.phy_cap_info[2] = 0,                                   \
			.phy_cap_info[3] = 0,                                   \
			.phy_cap_info[4] = 0,                                   \
			.phy_cap_info[5] = 0,                                   \
			.phy_cap_info[6] = 0,                                   \
			.phy_cap_info[7] = 0,                                   \
			.phy_cap_info[8] = 0,                                   \
			.phy_cap_info[9] = 0,                                   \
			.phy_cap_info[10] = 0,                                  \
		},                                                          \
		.he_mcs_nss_supp = {                                        \
			.rx_mcs_80 = cpu_to_le16(0xfffa),                       \
			.tx_mcs_80 = cpu_to_le16(0xfffa),                       \
			.rx_mcs_160 = cpu_to_le16(0xffff),                      \
			.tx_mcs_160 = cpu_to_le16(0xffff),                      \
			.rx_mcs_80p80 = cpu_to_le16(0xffff),                    \
			.tx_mcs_80p80 = cpu_to_le16(0xffff),                    \
		},                                                          \
		.ppe_thres = {0x08, 0x1c, 0x07},                            \
	}

#if CONFIG_WIFI6
static struct ieee80211_sband_iftype_data rwnx_he_capa = {
	//	.types_mask = BIT(NL80211_IFTYPE_STATION),
	.he_cap = RWNX_HE_CAPABILITIES,
};
#endif

static struct ieee80211_supported_band rwnx_band_2GHz = {
	.channels   = rw_2ghz_channels,
	.n_channels = ARRAY_SIZE(rw_2ghz_channels),
	//	.bitrates   = rwnx_ratetable,
	//	.n_bitrates = ARRAY_SIZE(rwnx_ratetable),
	.ht_cap     = RWNX_HT_CAPABILITIES,
#if CONFIG_WIFI6
	.iftype_data = &rwnx_he_capa,
	.n_iftype_data = 1,
#else
	.iftype_data = NULL,
	.n_iftype_data = 0,
#endif
};

static struct ieee80211_supported_band rwnx_band_5GHz = {
#if CONFIG_WIFI_BAND_5G
	.channels   = rw_5ghz_channels,
	.n_channels = ARRAY_SIZE(rw_5ghz_channels),
#else
	.channels	= NULL,
	.n_channels = 0,
#endif
	//	.bitrates   = &rwnx_ratetable[4],
	//	.n_bitrates = ARRAY_SIZE(rwnx_ratetable) - 4,
	.ht_cap     = RWNX_HT_CAPABILITIES,
	.vht_cap    = RWNX_VHT_CAPABILITIES,
#if CONFIG_WIFI6
	.iftype_data = &rwnx_he_capa,
	.n_iftype_data = 1,
#else
	.iftype_data = NULL,
	.n_iftype_data = 0,
#endif
};

// TODO: run in workqueue, need some lock.
static void rwnx_csa_finish(void *arg)
{
 	struct rwnx_hw *rwnx_hw = (struct rwnx_hw *)arg;
	int ret;

	if (rwnx_hw->csa) {
		ret = rw_msg_send_bcn_change(rwnx_hw->csa);  // FIXME: handle Only for AP/P2P-GO
		if (ret)
			RWNX_LOGE("%s: csa finish failed\n", __func__);

		os_free(rwnx_hw->csa->bcn_ptr);
		os_free(rwnx_hw->csa);
		rwnx_hw->csa = 0;

		bk_wlan_ap_set_channel_config(rwnx_hw->freq_params.channel);

		// cfg80211_ch_switch_notify
		wpa_ctrl_event_copy(WPA_CTRL_EVENT_CHAN_SWITCH_IND, &rwnx_hw->freq_params,
				sizeof(rwnx_hw->freq_params));
	} else {
		RWNX_LOGI("CSA finish indication but no active CSA\n");
	}
}

extern void rwnxl_register_connector(RW_CONNECTOR_T *intf);


UINT32 rw_ieee80211_init(void)
{
	struct rwnx_hw *rwnx_hw;
	int i;
	RW_CONNECTOR_T intf = {0, };
	struct wiphy *wiphy;

	wiphy = &g_wiphy;
	memset(wiphy, 0, sizeof(*wiphy));

	rwnx_hw = &g_rwnx_hw;
	memset(rwnx_hw, 0, sizeof(*rwnx_hw));

	rwnx_hw->txq = malloc(NX_NB_TXQ*sizeof(struct rwnx_txq));
	if(rwnx_hw->txq == NULL){
		BK_ASSERT(0);
	}
	else{
		memset(rwnx_hw->txq, 0, (NX_NB_TXQ*sizeof(struct rwnx_txq)));
	}

	rwnx_hw->wiphy = wiphy;
	rwnx_hw->mod_params = &rwnx_mod_params;

	if (ate_is_enabled())
		rwnx_hw->mod_params->use_2040 = true;

	rwnx_hw->vif_started = 0;
	rwnx_hw->monitor_vif = RWNX_INVALID_VIF;
	rwnx_hw->adding_sta = false;

	for (i = 0; i < NX_VIRT_DEV_MAX + NX_REMOTE_STA_MAX; i++)
		rwnx_hw->avail_idx_map |= BIT(i);

#if CONFIG_RWNX_SW_TXQ
	rwnx_hwq_init(rwnx_hw);
	rwnx_txq_prepare(rwnx_hw);
#endif

#if CONFIG_RWNX_TD
	rwnx_td_init();
#endif

	bk_work_init(&rwnx_hw->csa_work, rwnx_csa_finish, rwnx_hw);

	wiphy->bands[IEEE80211_BAND_2GHZ] = &rwnx_band_2GHz;
	wiphy->bands[IEEE80211_BAND_5GHZ] = &rwnx_band_5GHz;

	intf.msg_outbound_func = mr_kmsg_fwd;
	intf.data_outbound_func = rwm_upload_data;
	intf.rx_alloc_func = rwm_get_rx_free_node;
	intf.monitor_outbound_func = rwm_rx_monitor;

	rwnxl_register_connector(&intf);

	/* init country code */
	g_country_code.cfg.cc[0] = 'C';
	g_country_code.cfg.cc[1] = 'N';
	g_country_code.cfg.cc[2] = 0;
	g_country_code.cfg.schan = 1;
	g_country_code.cfg.nchan = 13;
	g_country_code.cfg.max_tx_power = 0;
#if CONFIG_WIFI_BAND_5G  // FIXME: BK7239, temporarily code for 5G because country code is not suitable for 5G/6G
	g_country_code.cfg.policy = WIFI_COUNTRY_POLICY_AUTO;
#else
	#if CONFIG_WIFI_AUTO_COUNTRY_CODE
	g_country_code.cfg.policy = WIFI_COUNTRY_POLICY_AUTO;
	#else
	g_country_code.cfg.policy = WIFI_COUNTRY_POLICY_MANUAL;
	#endif
#endif


	g_country_code.init = 1;

	return 0;
}

/* FIXME: Use with caution */
void rwnx_hw_reinit(void)
{
	struct rwnx_hw *rwnx_hw;

	rwnx_hw = &g_rwnx_hw;
	struct rwnx_txq *txq_temp = rwnx_hw->txq;

	memset(rwnx_hw, 0, sizeof(*rwnx_hw));

	if(txq_temp == NULL){
		rwnx_hw->txq = malloc(NX_NB_TXQ*sizeof(struct rwnx_txq));
		if(rwnx_hw->txq == NULL){
			BK_ASSERT(0);
		}
	}
	else{
		rwnx_hw->txq = txq_temp;
	}

	memset(rwnx_hw->txq, 0, (NX_NB_TXQ*sizeof(struct rwnx_txq)));

}

#if CONFIG_POWER_TABLE
static UINT32 rw_ieee80211_set_chan_power(void)
{
	uint8_t chan_idx;
	uint16_t txpwr = 0;
    UINT32 i;
    PW_LMT_REGULATION_TYPE regulation = PW_LMT_REGU_NULL;

	if (g_country_code.init == 0)
		return kNotInitializedErr;

	if (g_country_code.cfg.policy == WIFI_COUNTRY_POLICY_AUTO)
		return kNoErr;

    for(i=0; i < bk_get_country_regulation_table_size(); i++)
    {
        if(os_strcmp(g_country_code.cfg.cc,country_regulation_table[i].countrycode) == 0)
            {
                regulation = country_regulation_table[i].regulation;
                break;
            }
    }

	if (g_country_code.cfg.max_tx_power < 0) {
		// disable this channel
		txpwr = 0;
	} else if (g_country_code.cfg.max_tx_power == 0) {
		// use default
		txpwr = 30;
	}

	for (chan_idx = 1; chan_idx <= 14; chan_idx++) {
		if (rw_ieee80211_is_scan_rst_in_countrycode(chan_idx) == 1)
        {
            if(PW_LMT_REGU_NULL != regulation)
            {
                txpwr = bk_get_txpwr_lmt_tbl_entry_power(chan_idx, regulation)/2;
            }
            else
            {
                txpwr = 30;   
            }
            pwr_tbl_set_chan_pwr(chan_idx, txpwr*10, txpwr*10, txpwr*10, txpwr*10);  ///unit  0.1dbm
        }
		else {
			// disable this channel
			pwr_tbl_set_chan_pwr(chan_idx, 0, 0, 0, 0);
		}
	}
    manual_cal_set_cc_backoff_flag(true);
	RWNX_LOGI("set regulation %d chan maxpower:%ddbm\r\n", regulation,txpwr);

	return kNoErr;
}
#endif

static int country_validate(const wifi_country_t *country)
{
	if (!country)
		return BK_ERR_NULL_PARAM;

	int min_chan = country->schan;
	int max_chan = country->schan + country->nchan - 1;

	if ((min_chan < WIFI_MIN_CHAN_NUM) || (max_chan > WIFI_MAX_CHAN_NUM) ||
		(country->nchan <= 0))
		return BK_ERR_WIFI_CHAN_RANGE;

	if ((country->policy != WIFI_COUNTRY_POLICY_MANUAL) &&
		(country->policy != WIFI_COUNTRY_POLICY_AUTO))
		return BK_ERR_WIFI_COUNTRY_POLICY;

	return BK_OK;
}

int rw_ieee80211_set_country(const wifi_country_t *country)
{
	int ret = country_validate(country);
	UINT32 prev_policy;

	if (ret != BK_OK)
		return ret;

	if (g_country_code.init == 0)
		return BK_ERR_WIFI_NOT_INIT;

	prev_policy = g_country_code.cfg.policy;

	os_memcpy(&g_country_code.cfg, country, sizeof(wifi_country_t));
	RWNX_LOGI("set country code {cc=%s, chan=<%d-%d> policy=%s}\r\n", country->cc,
			  country->schan, (country->schan + country->nchan - 1),
			  country->policy == WIFI_COUNTRY_POLICY_MANUAL ? "manual" : "auto");

	if (g_country_code.cfg.policy == WIFI_COUNTRY_POLICY_AUTO) {
		if (prev_policy != g_country_code.cfg.policy) {
			RWNX_LOGI("need change softap beacon\r\n");
			//TODO
		}
	}

	//TODO apply the country
#if CONFIG_POWER_TABLE
	rw_ieee80211_set_chan_power();
#endif
	return BK_OK;
}

int rw_ieee80211_get_country(wifi_country_t *country)
{
	if (country) {
		if (g_country_code.init == 0)
			return BK_ERR_WIFI_NOT_INIT;

		os_memcpy(country, &g_country_code.cfg, sizeof(wifi_country_t));
		return BK_OK;
	} else
		return BK_ERR_PARAM;
}

UINT32 rw_ieee80211_get_centre_frequency(UINT32 chan_id)
{
	UINT32 freq = 0;
	struct ieee80211_channel *channels = NULL;

	// FIXME: BK7239 6E, 6E channel will start with channel 1
	if ((chan_id >= 1) && (chan_id <= 14)) {
		channels = (struct ieee80211_channel *)rw_2ghz_channels;
		freq = channels[chan_id - 1].center_freq;
	} else {
		int num_chans = ARRAY_SIZE(rw_5ghz_channels);
		channels = (struct ieee80211_channel *)rw_5ghz_channels;

		for (int i = 0; i < num_chans; i++) {
			if (channels[i].hw_value == chan_id) {
				freq = channels[i].center_freq;
			}
		}
	}

	if (freq != 0) {
		return freq;
	} else {
		RWNX_LOGI("centre freq is 0, chan_id %d\n", chan_id);
		return 0;
	}
}

UINT8 rw_ieee80211_get_chan_id(UINT32 freq)
{
	int i;
	struct ieee80211_channel *channels = NULL;
	int num_chans = 0;

	if (freq >= 5925) {
		// FIXME: BK7239 6E
	} else if (freq >= 4900) {
		channels = (struct ieee80211_channel *)rw_5ghz_channels;
		num_chans = ARRAY_SIZE(rw_5ghz_channels);
	} else {
		channels = (struct ieee80211_channel *)rw_2ghz_channels;
		num_chans = ARRAY_SIZE(rw_2ghz_channels);
	}

	for (i = 0; i < num_chans; i++) {
		if (channels[i].center_freq == freq)
			return channels[i].hw_value;
	}


	return 0;
}

#if CONFIG_WIFI_BAND_5G
/* select 5g non radar avaliable channels list according to country code*/
int* rw_select_5g_non_radar_avaliable_channels(int *selected_channel_size)
{
	int *selected_non_radar_avaliable_channels = NULL;
	for (int i = 0; i < sizeof(non_radar_avaliable_channel_table) / sizeof(non_radar_avaliable_channel_table[0]); i++) {
		if ((non_radar_avaliable_channel_table[i].countrycode[0] == g_country_code.cfg.cc[0]) &&
			(non_radar_avaliable_channel_table[i].countrycode[1] == g_country_code.cfg.cc[1]) &&
			(non_radar_avaliable_channel_table[i].countrycode[2] == g_country_code.cfg.cc[2])) {
			*selected_channel_size = non_radar_avaliable_channel_table[i].num_channels;
			selected_non_radar_avaliable_channels = non_radar_avaliable_channel_table[i].channels_5g;
		}
	}
	return selected_non_radar_avaliable_channels;
}

/* select channels list according to country code for 5g */
int* rw_select_5g_channels_by_regulatory_domain(int *selected_channels_size)
{
	int *selected_channels_5g = NULL;
	for (int i = 0; i < sizeof(country_channel_table) / sizeof(country_channel_table[0]); i++) {
		if ((country_channel_table[i].countrycode[0] == g_country_code.cfg.cc[0]) &&
			(country_channel_table[i].countrycode[1] == g_country_code.cfg.cc[1]) &&
			(country_channel_table[i].countrycode[2] == g_country_code.cfg.cc[2])) {
			*selected_channels_size = country_channel_table[i].num_channels;
			selected_channels_5g = country_channel_table[i].channels_5g;
		}
	}
	return selected_channels_5g;
}
#endif

UINT8 rw_ieee80211_init_scan_chan(struct scanu_start_req *req)
{
	UINT32 i, start_chan, num_chan_2g, num_chan_5g = 0, num_chan_6g = 0;  // FIXME: bk7239 6E

	BK_ASSERT(g_country_code.init); /* ASSERT VERIFIED */

	start_chan = g_country_code.cfg.schan;

	if (g_country_code.cfg.policy == WIFI_COUNTRY_POLICY_MANUAL)
		num_chan_2g = g_country_code.cfg.nchan;
	else {
		// auto mode, sta need scan all channel
		num_chan_2g = g_wiphy.bands[IEEE80211_BAND_2GHZ]->n_channels;
	}

	for (i = 0; i < num_chan_2g; i ++) {
		req->chan[i].band = IEEE80211_BAND_2GHZ;
		req->chan[i].flags = 0;
		req->chan[i].freq = rw_ieee80211_get_centre_frequency(i + start_chan);
		req->chan[i].tx_power = VIF_UNDEF_POWER;

#if CONFIG_WIFI_AUTO_COUNTRY_CODE
		uint16_t freq = req->chan[i].freq;
		// If auto mode, disable 12, 13 active scan
		if ((g_country_code.cfg.policy == WIFI_COUNTRY_POLICY_AUTO) &&
			(freq == 2467 || freq == 2472 || freq == 2484)) {
			req->chan[i].flags |= CHAN_NO_IR;
			// os_printf("XXX disable IR for freq %d\n", freq);
		}
#endif // CONFIG_WIFI_AUTO_COUNTRY_CODE
	}

#if CONFIG_WIFI_BAND_5G
	int selected_channels_size = 0;
	int *selected_channels_5g = rw_select_5g_channels_by_regulatory_domain(&selected_channels_size);

	for (i = 0; i < selected_channels_size; i++) {
		req->chan[i + num_chan_2g].band = IEEE80211_BAND_5GHZ;
		req->chan[i + num_chan_2g].flags = 0;
		req->chan[i + num_chan_2g].freq = (5000 + (5 * selected_channels_5g[i]));
		req->chan[i + num_chan_2g].tx_power = VIF_UNDEF_POWER;

		BK_ASSERT(req->chan[i + num_chan_2g].freq);
	}
	num_chan_5g = selected_channels_size;
#endif

	req->chan_cnt = num_chan_2g + num_chan_5g + num_chan_6g;

	return 0;
}

UINT8 rw_ieee80211_is_scan_rst_in_countrycode(UINT8 freq)
{
	UINT32 start_chan, end_chan;

	BK_ASSERT(g_country_code.init); /* ASSERT VERIFIED */

	if (g_country_code.cfg.policy != WIFI_COUNTRY_POLICY_MANUAL) {
		// auto mode, sta need scan all channel, and get all rst
		return 1;
	}

	start_chan = g_country_code.cfg.schan;
	end_chan = (start_chan + g_country_code.cfg.nchan - 1);

	if ((freq < start_chan) || (freq > end_chan))
		return 0;

	return 1;
}

UINT8 rw_ieee80211_get_scan_default_chan_num(void)
{
	UINT8 chan_num = 0;
	UINT8 num_chan_2g = 0;
	UINT8 num_chan_5g = 0;

	BK_ASSERT(g_country_code.init); /* ASSERT VERIFIED */

	if (g_country_code.cfg.policy == WIFI_COUNTRY_POLICY_MANUAL)
		num_chan_2g = g_country_code.cfg.nchan;
	else {
		// auto mode, sta need scan all channel
		num_chan_2g = g_wiphy.bands[IEEE80211_BAND_2GHZ]->n_channels;
	}

#if CONFIG_WIFI_BAND_5G
	int selected_channels_size = 0;
	rw_select_5g_channels_by_regulatory_domain(&selected_channels_size);

	num_chan_5g = selected_channels_size;
#endif

	chan_num = num_chan_2g + num_chan_5g;

	return chan_num;
}

#if CONFIG_WIFI_AUTO_COUNTRY_CODE
bool country_code_policy_is_auto(void)
{
	return (g_country_code.cfg.policy == WIFI_COUNTRY_POLICY_AUTO);
}
#endif // CONFIG_WIFI_AUTO_COUNTRY_CODE

#if CONFIG_WIFI4
void rw_ieee80211_set_ht_cap(UINT8 ht_supp)
{
	g_wiphy.bands[IEEE80211_BAND_2GHZ]->ht_cap.ht_supported = ht_supp;
	rw_msg_send_me_config_req();
}
#endif

uint64_t rwnx_hw_mm_features()
{
	return g_rwnx_hw.version_cfm.features;
}

// eof

