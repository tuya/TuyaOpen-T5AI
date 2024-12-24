// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <components/log.h>
#include <modules/wifi.h>
#include <components/netif.h>
#include <components/event.h>
#include <driver/uart.h>
#include <string.h>
#include "boot.h"
#include <modules/pm.h>
#include "aon_pmu_driver.h"
#include <driver/pwr_clk.h>
#if CONFIG_ROSC_CALIB_SW
#include <driver/rosc_32k.h>
#endif
#if CONFIG_BLUETOOTH
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "components/bluetooth/bk_ble.h"
#include "ble_api_5_x.h"

#endif
#include <bk_wifi_adapter.h>
#include <bk_phy_adapter.h>
#include <bk_rf_adapter.h>
#if (CONFIG_PSRAM)
#include <driver/psram.h>
#endif

#if CONFIG_USB
#include <components/usb.h>
#endif

#if (CONFIG_OTA_UPDATE_DEFAULT_PARTITION && CONFIG_HTTP_AB_PARTITION)
#include <modules/ota.h>
extern void bk_ota_confirm_update_partition(ota_confirm_flag ota_confirm_val);
#endif

#if (CONFIG_CLI)
#include "bk_api_cli.h"
#else
#if CONFIG_SHELL_ASYNCLOG
#include "bk_api_cli.h"
#endif
#endif

#if defined(CONFIG_WIFI_AT_ENABLE) && defined(CONFIG_WIFI_ENABLE) 
#include "wifi_at.h"
#endif
#if defined(CONFIG_BT_AT_ENABLE) && defined(CONFIG_BT) 
#include "bk_at_bluetooth.h"
#endif
#if defined(CONFIG_NETIF_AT_ENABLE) && defined(CONFIG_WIFI_ENABLE) 
#include "bk_at_netif.h"


#endif
#if (CONFIG_NTP_SYNC_RTC)
#include <components/ate.h>
#include <components/app_time_intf.h>
#endif

#define TAG "bk_init"

#ifdef CONFIG_VND_CAL
#include "vnd_cal.h"
#endif

#if CONFIG_BUTTON
#include "key_main.h"
#endif

#if (CONFIG_SOC_BK7236XX) || (CONFIG_SOC_BK7239XX) || (CONFIG_SOC_BK7286XX)
#include "sys_ll.h"
#include "bk_saradc.h"
#include "temp_detect_pub.h"
#endif

#if CONFIG_WIFI_PS_DISABLE
#include "modules/wifi.h"
#endif

#if (CONFIG_SDIO_V2P0 && CONFIG_SDIO_SLAVE)
#include "sdio_slave_driver.h"
#if CONFIG_SDIO_TEST_EN
#include "sdio_test.h"
#endif
#endif

#if CONFIG_AT
#include "atsvr_unite.h"
#endif

#if (CONFIG_DEBUG_FIRMWARE)
extern bk_err_t bk_dbg_init(void);
#endif

#if (CONFIG_SYS_CPU0)
extern void start_cpu1_core(void);
#endif

void rtos_user_app_launch_over(void);

extern const uint8_t build_version[]; // = __DATE__ " " __TIME__;

int app_phy_init(void)
{
#if ((CONFIG_WIFI_ENABLE || CONFIG_BLUETOOTH) && CONFIG_SYS_CPU0)
    bk_phy_adapter_init();
    bk_rf_adapter_init();
#endif
    return BK_OK;
}

static int app_wifi_init(void)
{
#if (CONFIG_WIFI_ENABLE)
	wifi_init_config_t wifi_config = WIFI_DEFAULT_INIT_CONFIG();
	BK_LOG_ON_ERR(bk_event_init());
	BK_LOG_ON_ERR(bk_netif_init());
	BK_LOG_ON_ERR(bk_wifi_init(&wifi_config));

#if (CONFIG_DEBUG_FIRMWARE)
	BK_LOG_ON_ERR(bk_dbg_init());
#endif

#if CONFIG_WIFI_PS_DISABLE
	//disable ps if needed
	bk_wifi_sta_pm_disable();
#endif
#if CONFIG_WIFI_AT_ENABLE
	wifi_at_cmd_init();
#endif
#if CONFIG_NETIF_AT_ENABLE 
	netif_at_cmd_init();
#endif

#endif
	return BK_OK;
}

static int app_ble_init(void)
{
#if CONFIG_BLUETOOTH
    BK_LOG_ON_ERR(bk_bluetooth_init());
#endif
	return BK_OK;
}

static int app_bt_init(void)
{
#if (CONFIG_BT)
	BK_LOGI(TAG, "BT active\r\n");
#if 0//TODO
	if (!ate_is_enabled())
		bt_activate(NULL);
#endif
#if (CONFIG_BT_AT_ENABLE) 
	bt_at_cmd_init();
#endif

#endif
	return BK_OK;
}

static int app_key_init(void)
{
#if CONFIG_BUTTON
	key_initialization();
#endif
	return BK_OK;
}

static int app_mp3_player_init(void)
{
#if (CONFIG_MP3PLAYER)
	key_init();
	media_thread_init();
#endif
	return BK_OK;
}

int app_sdio_init(void)
{
#if (CONFIG_SDIO_V2P0 && CONFIG_SDIO_SLAVE)
	bk_sdio_slave_driver_init();
#if CONFIG_SDIO_TEST_EN
	bk_sdio_test_init();
#endif
#endif
	return BK_OK;
}

int app_usb_init(void)
{
#if CONFIG_USB
	BK_LOGI(TAG, "fusb init\r\n");
#if 0//TODO
	if (!ate_is_enabled())
		fusb_init();
#endif
#endif
	return BK_OK;
}

static int app_cli_init(void)
{
#if (CONFIG_CLI)
#if !CONFIG_FULLY_HOSTED
	bk_cli_init();
#endif
#else
#if CONFIG_SHELL_ASYNCLOG
	bk_cli_init();
#endif
#endif
	return BK_OK;
}

static int app_usb_charge_init(void)
{
#if CONFIG_USB_CHARGE
	extern void usb_plug_func_open(void);
	usb_plug_func_open();
#endif
	return BK_OK;
}

static int app_uart_debug_init_todo(void)
{
#if CONFIG_UART_DEBUG
#ifndef KEIL_SIMULATOR
	BK_LOGI(TAG, "uart debug init\r\n");
	uart_debug_init();
#endif
#endif
	return BK_OK;
}

#if CONFIG_ETH
extern int net_eth_start();
static int app_eth_init(void)
{
	BK_LOGI(TAG, "ETH init\n");
	net_eth_start();
	return BK_OK;
}
#endif

#if CONFIG_ENABLE_WIFI_DEFAULT_CONNECT
extern void demo_wifi_fast_connect(void);
#endif

int bk_init(void)
{
	BK_LOGI(TAG, "armino app init: %s\n", build_version);

#ifdef APP_VERSION
	BK_LOGI(TAG, "APP Version: %s\n", APP_VERSION);
#endif

#if CONFIG_SYS_CPU0
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT,PM_CPU_FRQ_120M);
#endif

#ifdef CONFIG_VND_CAL
	vnd_cal_overlay();
#endif

	bk_pm_mailbox_init();
#if CONFIG_AT
	at_server_init();
#endif
#if (CONFIG_SYS_CPU0)

#if CONFIG_SDIO
	app_sdio_init();
#endif
	app_key_init();
	app_usb_charge_init();

#if CONFIG_SAVE_BOOT_TIME_POINT
	save_mtime_point(CPU_START_WIFI_INIT_TIME);
#endif

#if CONFIG_ATE_TEST
	/*not init the wifi, in order to save the boot time in ATE test after deepsleep(note:at the wifi ate test not enter power save)*/
	/*it need first finish test the wifi, at the end test deepsleep, wait wakeup(deepsleep), then test low voltage */
	if(!(aon_pmu_drv_reg_get(PMU_REG2)&BIT(BIT_SLEEP_FLAG_DEEP_SLEEP)))
	{
		app_wifi_init();
#if (CONFIG_BLUETOOTH)
		app_bt_init();
		app_ble_init();
#endif
	}
#else //!CONFIG_ATE_TEST
#if CONFIG_WIFI_ENABLE
    app_wifi_init();
#else
#if CONFIG_BLUETOOTH
	#if (CONFIG_SOC_BK7256XX || CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX)
    extern int bk_cal_if_init(void);
    bk_cal_if_init();
	#endif
#endif
#endif

#if (CONFIG_BLUETOOTH)
	app_bt_init();
	app_ble_init();
#endif

#endif //CONFIG_ATE_TEST

#if CONFIG_ETH
	app_eth_init();
#endif

#if CONFIG_SAVE_BOOT_TIME_POINT
	save_mtime_point(CPU_FINISH_WIFI_INIT_TIME);
#endif

#if CONFIG_ROSC_CALIB_SW
	bk_rosc_32k_calib();
#endif

	rtos_user_app_launch_over();

	app_mp3_player_init();
	app_uart_debug_init_todo();

#else //!(CONFIG_SYS_CPU1)


#endif //!(CONFIG_SYS_CPU1)

	app_cli_init();
#if CONFIG_AT 
	extern int atsvr_app_init(void);
	if(0 != atsvr_app_init())
		return -1;	
#endif

#if (CONFIG_VAULT_SUPPORT)
#if (CONFIG_SECURITYIP)
	extern bk_err_t bk_securityip_adaptor_init(void);
	bk_securityip_adaptor_init();
#endif
#endif

#if (CONFIG_SOC_BK7258 && CONFIG_SYS_CPU0)
	//bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU1, PM_POWER_MODULE_STATE_ON);
	//start_cpu1_core();
#endif


#if (CONFIG_NTP_SYNC_RTC)
    if(ate_is_enabled() == 0)
    {
	    app_time_rtc_ntp_sync_init();
    }
#endif


#if (CONFIG_FREERTOS)
#if CONFIG_SEMI_HOSTED
	semi_host_init();
#endif
#endif

#if CONFIG_UDISK_MP3
	um_init();
#endif

#if CONFIG_ENABLE_WIFI_DEFAULT_CONNECT
	demo_wifi_fast_connect();
#endif

#if (CONFIG_OTA_UPDATE_DEFAULT_PARTITION&& CONFIG_HTTP_AB_PARTITION)
#if (CONFIG_OTA_POSITION_INDEPENDENT_AB)
	bk_ota_double_check_for_execution();
#else
#ifdef CONFIG_OTA_UPDATE_B_PARTITION
	os_printf("exec part a\r\n");
	bk_ota_confirm_update_partition(CONFIRM_EXEC_A);
#else
	os_printf("exec part b\r\n");
	bk_ota_confirm_update_partition(CONFIRM_EXEC_B);
#endif
#endif
#endif

#if CONFIG_MICROPYTHON
extern int mp_do_startup(int heap_len);
	mp_do_startup(0);
#endif

#if CONFIG_SYS_CPU0
#if CONFIG_CPU_DEFAULT_FREQ_60M
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT,PM_CPU_FRQ_60M);
#endif
#endif
#if CONFIG_SYS_CPU1
#if CONFIG_PSRAM
	REG_READ(SOC_PSRAM_DATA_BASE);//check psram whether valid
#endif
	bk_pm_cp1_boot_ok_response_set();
#endif
#if CONFIG_USB //&& CONFIG_MENTOR_USB
	bk_usb_driver_init();
#endif

#if (CONFIG_PSRAM)
	bk_psram_id_auto_detect();
#endif
	return 0;
}
