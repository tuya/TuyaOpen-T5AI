#include "cli.h"
#include "bk_manual_ps.h"
#include "bk_mac_ps.h"
#include "bk_mcu_ps.h"
#include "bk_ps.h"
#include "bk_wifi.h"
#include "modules/pm.h"
#include "sys_driver.h"
#include "bk_pm_internal_api.h"
#include <driver/mailbox_channel.h>
#include <driver/gpio.h>
#include <driver/touch.h>
#include <driver/touch_types.h>
#include <driver/hal/hal_aon_rtc_types.h>
#include <driver/aon_rtc_types.h>
#include <driver/aon_rtc.h>
#include <driver/timer.h>
#include <driver/trng.h>
#include <driver/pwr_clk.h>
#include <driver/rosc_32k.h>
#include <driver/rosc_ppm.h>

#if CONFIG_SYS_CPU0
#if CONFIG_SYSTEM_CTRL
#define PM_MANUAL_LOW_VOL_VOTE_ENABLE          (0)
#define PM_DEEP_SLEEP_REGISTER_CALLBACK_ENABLE (0x1)

static UINT32 s_cli_sleep_mode = 0;
static UINT32 s_pm_vote1       = 0;
static UINT32 s_pm_vote2       = 0;
static UINT32 s_pm_vote3       = 0;

extern void stop_cpu1_core(void);
#if CONFIG_AON_RTC
static void cli_pm_rtc_callback(aon_rtc_id_t id, uint8_t *name_p, void *param)
{
	if(s_cli_sleep_mode == PM_MODE_DEEP_SLEEP)//when wakeup from deep sleep, all thing initial
	{
		os_printf("Attention: unable to enter deepsleep, it's not in a full function state now, please reboot !!!\r\n");
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
	}
	else if(s_cli_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
		bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0x0);
	}
	else
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote1,0x0,0x0);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote2,0x0,0x0);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote3,0x0,0x0);
	}
	os_printf("cli_pm_rtc_callback[%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
}
#endif
#if CONFIG_TOUCH
void cli_pm_touch_callback(void *param)
{
	if(s_cli_sleep_mode == PM_MODE_DEEP_SLEEP)//when wakeup from deep sleep, all thing initial
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
	}
	else if(s_cli_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
		bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0x0);
	}
	else
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote1,0x0,0x0);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote2,0x0,0x0);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote3,0x0,0x0);
	}
	os_printf("cli_pm_touch_callback[%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
}
#endif
void cli_pm_gpio_callback(gpio_id_t gpio_id)
{
	if(s_cli_sleep_mode == PM_MODE_DEEP_SLEEP)//when wakeup from deep sleep, all thing initial
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
	}
	else if(s_cli_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
		bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0x0);
	}
	else
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote1,0x0,0x0);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote2,0x0,0x0);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote3,0x0,0x0);
	}
	os_printf("cli_pm_gpio_callback[%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
}

#define PM_MANUAL_LOW_VOL_VOTE_ENABLE    (0)
#define PM_DEEPSLEEP_RTC_THRESHOLD       (500)
#define PM_SHUTDOWN_RTC_THRESHOLD        (4)        //=500ms
extern void stop_cpu1_core(void);

static void cli_pm_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_sleep_mode = 0;
	UINT32 pm_vote1 = 0,pm_vote2 = 0,pm_vote3=0;
	UINT32 pm_wake_source = 0;
	UINT32 pm_param1 = 0,pm_param2 = 0,pm_param3 = 0;
	//rtc_wakeup_param_t      rtc_wakeup_param         = {0};
	system_wakeup_param_t   system_wakeup_param      = {0};
	#if CONFIG_TOUCH
	touch_wakeup_param_t    touch_wakeup_param       = {0};
	#endif
	usbplug_wakeup_param_t  usbplug_wakeup_param     = {0};

	if (argc != 9) 
	{
		os_printf("set low power parameter invalid %d\r\n",argc);
		return;
	}
	pm_sleep_mode  = os_strtoul(argv[1], NULL, 10);
	pm_wake_source = os_strtoul(argv[2], NULL, 10);
	pm_vote1       = os_strtoul(argv[3], NULL, 10);
	pm_vote2       = os_strtoul(argv[4], NULL, 10);
	pm_vote3       = os_strtoul(argv[5], NULL, 10);
	pm_param1      = os_strtoul(argv[6], NULL, 10);
	pm_param2      = os_strtoul(argv[7], NULL, 10);
	pm_param3      = os_strtoul(argv[8], NULL, 10);
	
	os_printf("cli_pm_cmd %d %d %d %d %d %d %d!!! \r\n",
				pm_sleep_mode,
				pm_wake_source,
				pm_vote1,
				pm_vote2,
				pm_vote3,
				pm_param1,
				pm_param2);
	if((pm_sleep_mode > PM_MODE_DEFAULT)||(pm_wake_source > PM_WAKEUP_SOURCE_INT_NONE))
	{
		os_printf("set low power  parameter value  invalid\r\n");
		return;
	}

	if(pm_sleep_mode == PM_MODE_DEEP_SLEEP || pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
	{
		if((pm_vote1 > PM_POWER_MODULE_NAME_NONE) ||(pm_vote2 > PM_POWER_MODULE_NAME_NONE) ||(pm_vote3 > PM_POWER_MODULE_NAME_NONE))
		{
			os_printf("set pm vote deepsleep parameter value invalid\r\n");
			return;
		}
	}

	if(pm_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		if((pm_vote1 > PM_SLEEP_MODULE_NAME_MAX) ||(pm_vote2 > PM_SLEEP_MODULE_NAME_MAX) ||(pm_vote3 > PM_SLEEP_MODULE_NAME_MAX))
		{
			os_printf("set pm vote low vol parameter value invalid\r\n");
			return;
		}
	}

	s_cli_sleep_mode = pm_sleep_mode;
	s_pm_vote1 = pm_vote1;
	s_pm_vote2 = pm_vote2;
	s_pm_vote3 = pm_vote3;

	/*set wakeup source*/
	if(pm_wake_source == PM_WAKEUP_SOURCE_INT_RTC)
	{
		if (pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
		{
			#if CONFIG_RTC_ANA_WAKEUP_SUPPORT
			if(pm_param1 < PM_SHUTDOWN_RTC_THRESHOLD)
			{
				os_printf("param %d invalid ! must > %d which means 500ms.\r\n",pm_param1,PM_SHUTDOWN_RTC_THRESHOLD);
				return;
			}
			bk_rtc_ana_register_wakeup_source(pm_param1);
			/*workaround fix for unexpecting wakeup from super deep*/
			sys_drv_gpio_ana_wakeup_enable(1, GPIO_4, GPIO_INT_TYPE_MAX);
			#endif
		}
		else
		{
			#if CONFIG_AON_RTC
			alarm_info_t low_valtage_alarm = {
											"low_vol",
											pm_param1*AON_RTC_MS_TICK_CNT,
											1,
											cli_pm_rtc_callback,
											NULL
											};
			if(pm_param1 < PM_DEEPSLEEP_RTC_THRESHOLD)
			{
				os_printf("param %d invalid ! must > %dms.\r\n",pm_param1,PM_DEEPSLEEP_RTC_THRESHOLD);
				return;
			}
			//force unregister previous if doesn't finish.
			bk_alarm_unregister(AON_RTC_ID_1, low_valtage_alarm.name);
			bk_alarm_register(AON_RTC_ID_1, &low_valtage_alarm);
			#endif //CONFIG_AON_RTC
			bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
		}
	}
	else if(pm_wake_source == PM_WAKEUP_SOURCE_INT_GPIO)
	{
		if (pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
		{
			#if CONFIG_GPIO_ANA_WAKEUP_SUPPORT
			bk_gpio_ana_register_wakeup_source(pm_param1,pm_param2);
			#endif
		}
		else
		{
			#if CONFIG_GPIO_WAKEUP_SUPPORT
			bk_gpio_register_isr(pm_param1, cli_pm_gpio_callback);
			bk_gpio_register_wakeup_source(pm_param1,pm_param2);
			bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);
			#endif //CONFIG_GPIO_WAKEUP_SUPPORT
		}
	}
	else if(pm_wake_source == PM_WAKEUP_SOURCE_INT_SYSTEM_WAKE)
	{   
		if(pm_param1 == WIFI_WAKEUP)
		{
			system_wakeup_param.wifi_bt_wakeup = WIFI_WAKEUP;
		}
		else
		{
			system_wakeup_param.wifi_bt_wakeup = BT_WAKEUP;
		}

		bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_SYSTEM_WAKE, &system_wakeup_param);
	}
	else if(pm_wake_source == PM_WAKEUP_SOURCE_INT_TOUCHED)
	{
		#if CONFIG_TOUCH
		touch_wakeup_param.touch_channel = pm_param1;
		bk_touch_register_touch_isr((1<< touch_wakeup_param.touch_channel), cli_pm_touch_callback, NULL);
		bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_TOUCHED, &touch_wakeup_param);
		#endif
	}
	else if(pm_wake_source == PM_WAKEUP_SOURCE_INT_USBPLUG)
	{
		bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_USBPLUG, &usbplug_wakeup_param);
	}
	else
	{
		;
	}

	/*vote*/
	if(pm_sleep_mode == PM_MODE_DEEP_SLEEP || pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
	{
		if(pm_vote3 == PM_POWER_MODULE_NAME_CPU1)
		{
			#if CONFIG_SYS_CPU0 && (CONFIG_CPU_CNT > 1)
				stop_cpu1_core();
			#endif
		}

	}
	else if(pm_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		#if PM_MANUAL_LOW_VOL_VOTE_ENABLE
		if(pm_vote1 == PM_SLEEP_MODULE_NAME_APP)
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote1,0x1,pm_param3);
		}
		else
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote1,0x1,0x0);
		}

		if(pm_vote2 == PM_SLEEP_MODULE_NAME_APP)
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote2,0x1,pm_param3);
		}
		else
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote2,0x1,0x0);
		}

		if(pm_vote3 == PM_SLEEP_MODULE_NAME_APP)
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote3,0x1,pm_param3);
		}
		else
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote3,0x1,0x0);
		}
		#endif

		if((pm_vote1 == PM_SLEEP_MODULE_NAME_APP)||(pm_vote2 == PM_SLEEP_MODULE_NAME_APP)||(pm_vote3 == PM_SLEEP_MODULE_NAME_APP))
		{
			bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x1,pm_param3);
		}
	}
	else
	{
		;//do something
	}

	bk_pm_sleep_mode_set(pm_sleep_mode);

	pm_printf_current_temperature();

}
static void cli_pm_debug(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if !CONFIG_SYS_CPU2
	UINT32 pm_debug  = 0;
	if (argc != 2)
	{
		os_printf("set low power debug parameter invalid %d\r\n",argc);
		return;
	}

	pm_debug = os_strtoul(argv[1], NULL, 10);

	pm_debug_ctrl(pm_debug);

	if(pm_debug == PM_DEBUG_CTRL_STATE)
	{
		#if CONFIG_SYS_CPU0
		pm_debug_pwr_clk_state();
		pm_debug_lv_state();
		#endif
	}
	/*for temp debug*/
	if(pm_debug == 16)
	{
		#if CONFIG_SYS_CPU1
		bk_pm_cp1_recovery_response(PM_CP1_RECOVERY_CMD, PM_CP1_PREPARE_CLOSE_MODULE_NAME_MEDIA,PM_CP1_MODULE_RECOVERY_STATE_FINISH);
		#endif
	}

	if(pm_debug == 32)
	{
		#if CONFIG_SYS_CPU1
		bk_pm_cp1_recovery_response(PM_CP1_RECOVERY_CMD, PM_CP1_PREPARE_CLOSE_MODULE_NAME_MEDIA,PM_CP1_MODULE_RECOVERY_STATE_INIT);
		#endif
	}
#endif
}
static void cli_pm_vote_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_sleep_mode   = 0;
	UINT32 pm_vote         = 0;
	UINT32 pm_vote_value   = 0;
	UINT32 pm_sleep_time   = 0;
	if (argc != 5)
	{
		os_printf("set low power vote parameter invalid %d\r\n",argc);
		return;
	}
	pm_sleep_mode        = os_strtoul(argv[1], NULL, 10);
	pm_vote              = os_strtoul(argv[2], NULL, 10);
	pm_vote_value        = os_strtoul(argv[3], NULL, 10);
	pm_sleep_time        = os_strtoul(argv[4], NULL, 10);
	if((pm_sleep_mode > PM_MODE_DEFAULT)|| (pm_vote > PM_SLEEP_MODULE_NAME_MAX)||(pm_vote_value > 1))
	{
		os_printf("set low power vote parameter value  invalid\r\n");
		return;
	}
	/*vote*/
	if(pm_sleep_mode == LOW_POWER_DEEP_SLEEP)
	{
		if((pm_vote == POWER_MODULE_NAME_BTSP)||(pm_vote == POWER_MODULE_NAME_WIFIP_MAC))
		{
			bk_pm_module_vote_power_ctrl(pm_vote,pm_vote_value);
		}
	}
	else if(pm_sleep_mode == LOW_POWER_MODE_LOW_VOLTAGE)
	{
		bk_pm_module_vote_sleep_ctrl(pm_vote,pm_vote_value,pm_sleep_time);
	}
	else
	{
		;//do something
	}
	pm_printf_current_temperature();
}
#if CONFIG_DEBUG_FIRMWARE
static void cli_pm_vol(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_vol  = 0;
	if (argc != 2)
	{
		os_printf("set pm voltage parameter invalid %d\r\n",argc);
		return;
	}

	pm_vol = os_strtoul(argv[1], NULL, 10);
	if ((pm_vol < 0) || (pm_vol > 7))
	{
		os_printf("set pm voltage value invalid %d\r\n",pm_vol);
		return;
	}

	bk_pm_lp_vol_set(pm_vol);

}
static void cli_pm_clk(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_clk_state  = 0;
	UINT32 pm_module_id  = 0;
	if (argc != 3)
	{
		os_printf("set pm clk parameter invalid %d\r\n",argc);
		return;
	}

	pm_module_id = os_strtoul(argv[1], NULL, 10);
	pm_clk_state = os_strtoul(argv[2], NULL, 10);
	if ((pm_clk_state < 0) || (pm_clk_state > 1) || (pm_module_id < 0) || (pm_module_id > 31))
	{
		os_printf("set pm clk value invalid %d %d\r\n",pm_clk_state,pm_module_id);
		return;
	}
	bk_pm_clock_ctrl(pm_module_id,pm_clk_state);

}
static void cli_pm_power(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_power_state  = 0;
	UINT32 pm_module_id  = 0;
	if (argc != 3)
	{
		os_printf("set pm power parameter invalid %d\r\n",argc);
		return;
	}

	pm_module_id = os_strtoul(argv[1], NULL, 10);
	pm_power_state = os_strtoul(argv[2], NULL, 10);
	if (pm_power_state > 1)
	{
		os_printf("set pm power value invalid %d %d \r\n",pm_power_state,pm_module_id);
		return;
	}

	bk_pm_module_vote_power_ctrl(pm_module_id,pm_power_state);

}
static void cli_pm_freq(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_freq  = 0;
	UINT32 pm_module_id  = 0;
	pm_cpu_freq_e module_freq = 0;
	pm_cpu_freq_e current_max_freq = 0;
	if (argc != 3)
	{
		os_printf("set pm freq parameter invalid %d\r\n",argc);
		return;
	}

	pm_module_id = os_strtoul(argv[1], NULL, 10);
	pm_freq = os_strtoul(argv[2], NULL, 10);
	if ((pm_freq > PM_CPU_FRQ_DEFAULT) || (pm_module_id > PM_DEV_ID_MAX))
	{
		os_printf("set pm freq value invalid %d %d \r\n",pm_freq,pm_module_id);
		return;
	}

	bk_pm_module_vote_cpu_freq(pm_module_id,pm_freq);

	module_freq =  bk_pm_module_current_cpu_freq_get(pm_module_id);

	current_max_freq = bk_pm_current_max_cpu_freq_get();

	os_printf("pm cpu freq test id: %d; freq: %d; current max cpu freq: %d;\r\n",pm_module_id,module_freq,current_max_freq);

}
static void cli_pm_lpo(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_SYS_CPU0
	UINT32 pm_lpo  = 0;
	if (argc != 2)
	{
		os_printf("set pm lpo parameter invalid %d\r\n",argc);
		return;
	}

	pm_lpo = os_strtoul(argv[1], NULL, 10);
	if ((pm_lpo < 0) || (pm_lpo > 3))
	{
		os_printf("set  pm lpo value invalid %d\r\n",pm_lpo);
		return;
	}

	bk_pm_lpo_src_set(pm_lpo);
#endif
}
static void cli_pm_ctrl(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_ctrl  = 0;
	if (argc != 2)
	{
		os_printf("set pm ctrl parameter invalid %d\r\n",argc);
		return;
	}

	pm_ctrl = os_strtoul(argv[1], NULL, 10);
	if ((pm_ctrl < 0) || (pm_ctrl > 1))
	{
		os_printf("set pm ctrl value invalid %d\r\n",pm_ctrl);
		return;
	}

	bk_pm_mcu_pm_ctrl(pm_ctrl);

}
static void cli_pm_pwr_state(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_pwr_module        = 0;
	UINT32 pm_pwr_module_state  = 0;
	if (argc != 2)
	{
		os_printf("set pm pwr state parameter invalid %d\r\n",argc);
		return;
	}

	pm_pwr_module = os_strtoul(argv[1], NULL, 10);
	if ((pm_pwr_module < 0) || (pm_pwr_module >= PM_POWER_MODULE_NAME_NONE))
	{
		os_printf("pm module[%d] not support ,get power state fail\r\n",pm_pwr_module);
		return;
	}

	pm_pwr_module_state = bk_pm_module_power_state_get(pm_pwr_module);
	os_printf("Get module[%d] power state[%d] \r\n",pm_pwr_module,pm_pwr_module_state);

}
static void cli_pm_auto_vote(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_ctrl  = 0;
	if (argc != 2)
	{
		os_printf("set pm auto_vote parameter invalid %d\r\n",argc);
		return;
	}

	pm_ctrl = os_strtoul(argv[1], NULL, 10);
	if ((pm_ctrl < 0) || (pm_ctrl > 1))
	{
		os_printf("set pm auto vote value invalid %d\r\n",pm_ctrl);
		return;
	}
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0x0);
}

#define CLI_DVFS_FREQUNCY_DIV_MAX      (15)
#define CLI_DVFS_FREQUNCY_DIV_BUS_MAX  (1)
static void cli_dvfs_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 cksel_core = 0;
	UINT32 ckdiv_core = 0;
	UINT32 ckdiv_bus  = 0;
	UINT32 ckdiv_cpu0 = 0;
	UINT32 ckdiv_cpu1 = 0;

	if (argc != 6) 
	{
		os_printf("set dvfs parameter invalid %d\r\n",argc);
		return;
	}

	GLOBAL_INT_DECLARATION();
	cksel_core   = os_strtoul(argv[1], NULL, 10);
	ckdiv_core   = os_strtoul(argv[2], NULL, 10);
	ckdiv_bus    = os_strtoul(argv[3], NULL, 10);
	ckdiv_cpu0   = os_strtoul(argv[4], NULL, 10);
	ckdiv_cpu1   = os_strtoul(argv[5], NULL, 10);

	os_printf("cli_dvfs_cmd %d %d %d %d %d !!! \r\n",
				cksel_core,
				ckdiv_core,
				ckdiv_bus,
				ckdiv_cpu0,
				ckdiv_cpu1);
	GLOBAL_INT_DISABLE();
	if(cksel_core > 3)
	{
		os_printf("set dvfs cksel core > 3 invalid %d\r\n",cksel_core);
		GLOBAL_INT_RESTORE();
		return;
	}

	if((ckdiv_core > CLI_DVFS_FREQUNCY_DIV_MAX) || (ckdiv_bus > CLI_DVFS_FREQUNCY_DIV_BUS_MAX)||(ckdiv_cpu0 > CLI_DVFS_FREQUNCY_DIV_MAX)||(ckdiv_cpu0 > CLI_DVFS_FREQUNCY_DIV_MAX))
	{
		os_printf("set dvfs ckdiv_core ckdiv_bus ckdiv_cpu0  ckdiv_cpu0  > 15 invalid\r\n");
		GLOBAL_INT_RESTORE();
		return;
	}
	pm_core_bus_clock_ctrl(cksel_core, ckdiv_core,ckdiv_bus, ckdiv_cpu0,ckdiv_cpu1);
	GLOBAL_INT_RESTORE();
	os_printf("switch cpu frequency ok 0x%x 0x%x 0x%x\r\n",sys_drv_all_modules_clk_div_get(CLK_DIV_REG0),sys_drv_cpu_clk_div_get(0),sys_drv_cpu_clk_div_get(1));
}

typedef struct{
	uint32_t cksel_core;  // 0:XTAL       1 : clk_DCO      2 : 320M      3 : 480M
	uint32_t ckdiv_core;  // Frequency division : F/(1+N), N is the data of the reg value 0--15
	uint32_t ckdiv_bus;   // Frequency division : F/(1+N), N is the data of the reg value:0--1
	uint32_t ckdiv_cpu0;  // Frequency division : F/(1+N), N is the data of the reg value:0--15
	uint32_t ckdiv_cpu1;  // 0: cpu0,cpu1 and bus  sel same clock frequence 1: cpu0_clk and  bus_clk is half cpu1_clk
}core_bus_clock_ctrl_t;

#define CORE_BUS_CLOCK_AUTO_TEST 0
#if CORE_BUS_CLOCK_AUTO_TEST
#define CORE_BUS_CLOCK_MAP \
{ \
	{0x0,0x0,0x0,0x0,0x0 },  /*0:XTAL */\
	{0x2,0x1,0x0,0x0,0x0 },  /*2 : 320M  ckdiv_cpu1 = 0 */ \
	{0x2,0x2,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x3,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x4,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x5,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x6,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x7,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x8,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x9,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xA,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xB,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xC,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xD,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xE,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xF,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x0,0x0,0x0,0x1 },  /*2 : 320M  ckdiv_cpu1 = 1*/ \
	{0x2,0x1,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x2,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x3,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x4,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x5,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x6,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x7,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x8,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x9,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xA,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xB,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xC,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xD,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xE,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xF,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x3,0x2,0x0,0x0,0x0 },  /*3 : 480M ckdiv_cpu1 = 0*/ \
	{0x3,0x3,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x4,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x5,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x6,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x7,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x8,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x9,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xA,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xB,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xC,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xD,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xE,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xF,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x1,0x0,0x0,0x1 },  /*3 : 480M ckdiv_cpu1 = 1*/ \
	{0x3,0x2,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x3,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x4,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x5,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x6,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x7,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x8,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x9,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xA,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xB,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xC,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xD,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xE,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xF,0x0,0x0,0x1 },  /*3 : 480M */ \
}
#else
#define CORE_BUS_CLOCK_MAP \
{ \
	{0x0,0x0,0x0,0x0,0x0 },  /*0:XTAL */\
	{0x2,0x1,0x0,0x0,0x0 },  /*2 : 320M  ckdiv_cpu1 = 0 */ \
	{0x3,0x2,0x0,0x0,0x0 },  /*3 : 480M  ckdiv_cpu1 = 0 */ \
	{0x3,0x1,0x0,0x0,0x1 },  /*3 : 480M  ckdiv_cpu1 = 1 */ \
}
#endif
#define DVFS_AUTO_TEST_COUNT (2)
extern void delay_us(uint32 num);
static void cli_dvfs_auto_test_timer_isr(timer_id_t chan)
{
	uint8_t rand_num;
	uint8_t i;
	for(i=0; i<DVFS_AUTO_TEST_COUNT; i++)
	{
		rand_num = (uint32_t)bk_rand()%PM_CPU_FRQ_DEFAULT;
		//os_printf("dvfs random %d \r\n",rand_num);
		delay_us(5);
		sys_drv_switch_cpu_bus_freq(rand_num);
	}
}

core_bus_clock_ctrl_t core_bus_clock[] = CORE_BUS_CLOCK_MAP;
static void cli_dvfs_auto_test_all_timer_isr(timer_id_t chan)
{
#if CONFIG_SYS_CPU0
	uint8_t rand_num;
	uint8_t i;
	
	for(i=0; i<DVFS_AUTO_TEST_COUNT; i++)
	{
		rand_num = (uint32_t)bk_rand()%(sizeof(core_bus_clock)/sizeof(core_bus_clock_ctrl_t));
		//os_printf("dvfs random %d \r\n",rand_num);
		//os_printf("[cksel:%d] [ckdiv_core:%d] [ckdiv_bus:%d] [ckdiv_cpu0:%d] [ckdiv_cpu1:%d]\r\n",
			//core_bus_clock[rand_num].cksel_core, core_bus_clock[rand_num].ckdiv_core, core_bus_clock[rand_num].ckdiv_bus, core_bus_clock[rand_num].ckdiv_cpu0, core_bus_clock[rand_num].ckdiv_cpu1);
		delay_us(5);
		pm_core_bus_clock_ctrl(core_bus_clock[rand_num].cksel_core, core_bus_clock[rand_num].ckdiv_core, core_bus_clock[rand_num].ckdiv_bus, core_bus_clock[rand_num].ckdiv_cpu0, core_bus_clock[rand_num].ckdiv_cpu1);
	}
#endif
}

static void cli_dvfs_auto_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_SYS_CPU0
	uint32_t period_us;
	
	if (argc != 3)
	{
		os_printf("set dvfs_auto_test parameter invalid %d\r\n",argc);
		return;
	}
	period_us = os_strtoul(argv[1], NULL, 10);
	os_printf("dvfs auto test period set %d us!\r\n",period_us);

	bk_trng_driver_init();
	bk_trng_start();
	
	if (os_strcmp(argv[2], "default") == 0)
	{
		bk_timer_delay_with_callback(TIMER_ID5,period_us,cli_dvfs_auto_test_timer_isr);
	}
	else if (os_strcmp(argv[2], "all") == 0)
	{
		bk_timer_delay_with_callback(TIMER_ID5,period_us,cli_dvfs_auto_test_all_timer_isr);
	}
	else
	{
		bk_timer_delay_with_callback(TIMER_ID5,period_us,cli_dvfs_auto_test_timer_isr);
	}
#endif
}

#if CONFIG_AON_RTC
static UINT32 s_pre_tick;
static void cli_pm_timer_isr(timer_id_t chan)
{
	UINT32 current_tick = 0;
	double current_freq = 0;
	INT32 current_ppm = 0;
	INT32 current_delta = 0;

#if CONFIG_CKMN
	current_freq = bk_rosc_32k_get_freq();
	current_ppm = bk_rosc_32k_get_ppm();
#endif
	current_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	current_delta = current_tick - s_pre_tick;

	os_printf("rosc %d %8.3f %d %d\r\n",
						current_delta,
						current_freq,
						current_ppm,
						current_tick);
	s_pre_tick = current_tick;
}
#if CONFIG_CKMN
static uint32_t prog_intval = 32;
static void cli_pm_rosc_timer_isr(timer_id_t chan)
{
	bk_rosc_32k_ckest_prog(prog_intval);
}
#endif
#endif
static void cli_pm_rosc_accuracy(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_AON_RTC
	UINT32 timer_count_interval = 0;

	if (argc != 2)
	{
		os_printf("set rosc_accuracy parameter invalid %d\r\n",argc);
		return;
	}

	timer_count_interval   = os_strtoul(argv[1], NULL, 10);
	bk_timer_start(0, timer_count_interval, cli_pm_timer_isr);
#endif
}
static void cli_pm_rosc_cali(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 cali_interval = 0;
	UINT32 cali_mode = 0;

	if (argc < 3)
	{
		os_printf("set rosc cali parameter invalid %d\r\n",argc);
		return;
	}

	cali_mode   = os_strtoul(argv[1], NULL, 10);
	cali_interval   = os_strtoul(argv[2], NULL, 10);
	if (cali_mode == 4) {
#if CONFIG_CKMN
		prog_intval = os_strtoul(argv[3], NULL, 10);
		bk_timer_start(1, cali_interval, cli_pm_rosc_timer_isr);
#endif
	} else {
		bk_pm_rosc_calibration(cali_mode, cali_interval);
	}
}
static void cli_pm_clk_pin(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 lpo_clk = 0;
	UINT32 lpo_digital;
	UINT32 reg;
	//dco_cali_speed_e lpo_clk;
	#define SYSTEM_BASE_ADDR    (0x44010000)
	#define AON_GPIO_BASE_ADDR  (0x44000400)
	#define AON_PMU_BASE_ADDR   (0x44000000)

	if (argc < 3)
	{
		os_printf("parameter invalid %d\r\n", argc);
		return;
	}

	// lpo_clk      0:rosc    1:xtall    2:dco
	// lpo_digital  0:analog  1:digital
	lpo_clk     = os_strtoul(argv[1], NULL, 10);
	lpo_digital = os_strtoul(argv[2], NULL, 10);

	if (lpo_digital == 1) //gpio24 output digital clock
	{
	}
	else  //gpio24 output analog clock
	{
		REG_WRITE((AON_GPIO_BASE_ADDR + 0x18 * 4), 0x8); // 0x44000460

		// system clock test enable
		reg = REG_READ(SYSTEM_BASE_ADDR + 0x44 * 4);     // 0x44010110
		reg |= BIT(22);
		REG_WRITE((SYSTEM_BASE_ADDR + 0x44 * 4), reg);   // 0x44010110

		if (lpo_clk == 0) { // output rosc clock

			reg = REG_READ(SYSTEM_BASE_ADDR + 0x45 * 4);     // 0x44010114
			reg |= BIT(21);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x45 * 4), reg);   // 0x44010114

			reg = REG_READ(SYSTEM_BASE_ADDR + 0x45 * 4);     // 0x44010114
			reg |= BIT(12);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x45 * 4), reg);   // 0x44010114
			os_printf("gpio 24 output analog rosc 32k\r\n");

		} else if (lpo_clk == 1) { // output xtall clock
			reg = REG_READ(SYSTEM_BASE_ADDR + 0x45 * 4);     // 0x44010114
			reg |= BIT(1);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x45 * 4), reg);   // 0x44010114

			reg = REG_READ(SYSTEM_BASE_ADDR + 0x45 * 4);     // 0x44010114
			reg |= BIT(20);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x45 * 4), reg);   // 0x44010114
			os_printf("gpio 24 output analog xtall 32.768k\r\n");

		}else if (lpo_clk == 2) {  // output dco clock
			reg = REG_READ(SYSTEM_BASE_ADDR + 0x41 * 4);     // 0x44010104
			reg &= ~ BIT(26);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x41 * 4), reg);   // 0x44010104

			reg = REG_READ(SYSTEM_BASE_ADDR + 0x45 * 4);     // 0x44010114
			reg |= BIT(2);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x45 * 4), reg);   // 0x44010114

			reg = REG_READ(SYSTEM_BASE_ADDR + 0x41 * 4);     // 0x44010104
			reg |= (0x3 << 11);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x41 * 4), reg);   // 0x44010104

			reg = REG_READ(SYSTEM_BASE_ADDR + 0x41 * 4);     // 0x44010104
			reg |= (0x127 << 16);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x41 * 4), reg);   // 0x44010104
			// frequency division 3
			reg = REG_READ(SYSTEM_BASE_ADDR + 0x41 * 4);     // 0x44010104
			reg |= (0x2 << 27);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x41 * 4), reg);   // 0x44010104

			for(int i = 0; i < 2; i++)
			{
				reg = REG_READ(SYSTEM_BASE_ADDR + 0x41 * 4);     // 0x44010104
				reg &= ~ BIT(15);
				REG_WRITE((SYSTEM_BASE_ADDR + 0x41 * 4), reg);   // 0x44010104
				reg = REG_READ(SYSTEM_BASE_ADDR + 0x41 * 4);     // 0x44010104
				reg |= BIT(15);
				REG_WRITE((SYSTEM_BASE_ADDR + 0x41 * 4), reg);   // 0x44010104
			}
			os_printf("gpio 24 output analog DCO 20M digital core 80M.\r\n");
		}
		//clock test signal selection rosc/xtall/dco
		reg = REG_READ(SYSTEM_BASE_ADDR + 0x44 * 4);     // 0x44010110
		reg |= (lpo_clk << 20);
		REG_WRITE((SYSTEM_BASE_ADDR + 0x44 * 4), reg);   // 0x44010110

	}
	
}
static void cli_pm_wakeup_source(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 sleep_mode = 0;

	if (argc != 2)
	{
		os_printf("set get wakeup source parameter invalid %d\r\n",argc);
		return;
	}

	sleep_mode   = os_strtoul(argv[1], NULL, 10);
	if(sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		#if CONFIG_SYS_CPU0
		os_printf("low voltage wakeup source [%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
		#endif
	}
	else if(sleep_mode == PM_MODE_DEEP_SLEEP)
	{
		#if CONFIG_SYS_CPU0
		os_printf("deepsleep wakeup source [%d]\r\n",bk_pm_deep_sleep_wakeup_source_get());
		#endif
	}
	else
	{
		os_printf("it not support the sleep mode[%d] for wakeup source \r\n",sleep_mode);
	}

}

static void cli_pm_rosc_ppm(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_SYS_CPU0
	UINT32 timer_interval, count= 0;
	INT32  ppm_val;

	if (argc < 3)
	{
		os_printf("parameter invalid %d\r\n", argc);
		return;
	}

	timer_interval = os_strtoul(argv[1], NULL, 10);
	count          = os_strtoul(argv[2], NULL, 10);

	bk_rosc_ppm_statistic_start(TIMER_ID1, timer_interval, count);
	rtos_delay_milliseconds(timer_interval * count + 100);
	bk_rosc_ppm_statistics_get(&ppm_val);
	os_printf("ppm val = %d\r\n", ppm_val);

	return ;
#endif
}

static void cli_pm_cp1_ctrl(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_SYS_CPU0 && (CONFIG_CPU_CNT > 1)
	UINT32 cp1_ctrl = 0;

	if (argc != 2)
	{
		os_printf("cp1 ctrl parameter invalid %d\r\n",argc);
		return;
	}

	cp1_ctrl   = os_strtoul(argv[1], NULL, 10);
	bk_pm_cp1_auto_power_down_state_set(cp1_ctrl);
#endif
}
static void cli_pm_boot_cp1(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_SYS_CPU0 && (CONFIG_CPU_CNT > 1)
	UINT32 boot_cp1_state = 0;
	UINT32 module_name    = 0;
	if (argc != 3)
	{
		os_printf("cp1 ctrl parameter invalid %d\r\n",argc);
		return;
	}
	module_name   = os_strtoul(argv[1], NULL, 10);
	boot_cp1_state   = os_strtoul(argv[2], NULL, 10);
	bk_pm_module_vote_boot_cp1_ctrl(module_name,boot_cp1_state);
#endif
}
#if (CONFIG_CPU_CNT > 2)
static void cli_pm_boot_cp2(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_SYS_CPU0
	UINT32 boot_cp2_state = 0;
	UINT32 module_name    = 0;
	if (argc != 3)
	{
		os_printf("cp2 ctrl parameter invalid %d\r\n",argc);
		return;
	}
	module_name   = os_strtoul(argv[1], NULL, 10);
	boot_cp2_state   = os_strtoul(argv[2], NULL, 10);
	bk_pm_module_vote_boot_cp2_ctrl(module_name,boot_cp2_state);
#endif
}
#endif//CONFIG_CPU_CNT > 2
static void cli_pm_ldo(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
//#if CONFIG_SYS_CPU0 && (CONFIG_CPU_CNT > 1)
	UINT32 gpio_id                = 0;
	UINT32 module_name            = 0;
	UINT32 gpio_output_state_e    = 0;
	if (argc != 4)
	{
		os_printf("cp1 ctrl parameter invalid %d\r\n",argc);
		return;
	}
	module_name           = os_strtoul(argv[1], NULL, 10);
	gpio_id               = os_strtoul(argv[2], NULL, 10);
	gpio_output_state_e   = os_strtoul(argv[3], NULL, 10);
	bk_pm_module_vote_ctrl_external_ldo(module_name,gpio_id,gpio_output_state_e);
//#endif
}
static void cli_pm_psram(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_SYS_CPU0
	UINT32 module_name            = 0;
	UINT32 power_psram_state      = 0;
	if (argc != 3)
	{
		os_printf("psram ctrl parameter invalid %d\r\n",argc);
		return;
	}
	module_name         = os_strtoul(argv[1], NULL, 10);
	power_psram_state   = os_strtoul(argv[2], NULL, 10);
	bk_pm_module_vote_psram_ctrl(module_name,power_psram_state);
#endif
}
#endif//CONFIG_DEBUG_FIRMWARE

#endif//CONFIG_SYSTEM_CTRL

#endif//CONFIG_SYS_CPU0
#if CONFIG_WIFI_ENABLE
static void cli_ps_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if PS_SUPPORT_MANUAL_SLEEP
	UINT32 standby_time = 0;
	UINT32 dtim_wait_time = 0;
#endif

	if (argc != 3)
		goto _invalid_ps_arg;

#if CONFIG_MCU_PS
#if !CONFIG_SOC_BK7236XX && (!CONFIG_SOC_BK7239XX) && (!CONFIG_SOC_BK7286XX) //temp mofify for bk7236
	if (0 == os_strcmp(argv[1], "mcudtim")) {
		UINT32 dtim = os_strtoul(argv[2], NULL, 10);
		if (dtim == 1)
			bk_wlan_mcu_ps_mode_enable();
		else if (dtim == 0)
			bk_wlan_mcu_ps_mode_disable();
		else
			goto _invalid_ps_arg;
	}
#endif
#endif
#if CONFIG_STA_PS
	else if (0 == os_strcmp(argv[1], "rfdtim")) {
		UINT32 dtim = os_strtoul(argv[2], NULL, 10);
		if (dtim == 1) {
			if (bk_wlan_ps_enable())
				os_printf("dtim enable failed\r\n");
		} else if (dtim == 0) {
			if (bk_wlan_ps_disable())
				os_printf("dtim disable failed\r\n");
		} else
			goto _invalid_ps_arg;
	}
#if PS_USE_KEEP_TIMER
	else if (0 == os_strcmp(argv[1], "rf_timer")) {
		UINT32 dtim = os_strtoul(argv[2], NULL, 10);

		if (dtim == 1) {
			extern int bk_wlan_ps_timer_start(void);
			bk_wlan_ps_timer_start();
		} else  if (dtim == 0) {
			extern int bk_wlan_ps_timer_pause(void);
			bk_wlan_ps_timer_pause();
		} else
			goto _invalid_ps_arg;
	}
#endif
#endif
	else
		goto _invalid_ps_arg;


	return;

_invalid_ps_arg:
	os_printf("Usage:ps {rfdtim|mcudtim|rf_timer} {1/0}\r\n");
}
#endif
#if !CONFIG_SYSTEM_CTRL
#if CONFIG_MCU_PS

static void cli_deep_sleep_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	PS_DEEP_CTRL_PARAM deep_sleep_param;

	deep_sleep_param.wake_up_way            = 0;

	deep_sleep_param.gpio_index_map         = os_strtoul(argv[1], NULL, 16);
	deep_sleep_param.gpio_edge_map          = os_strtoul(argv[2], NULL, 16);
	deep_sleep_param.gpio_last_index_map    = os_strtoul(argv[3], NULL, 16);
	deep_sleep_param.gpio_last_edge_map     = os_strtoul(argv[4], NULL, 16);
	deep_sleep_param.sleep_time             = os_strtoul(argv[5], NULL, 16);
	deep_sleep_param.wake_up_way            = os_strtoul(argv[6], NULL, 16);
	deep_sleep_param.gpio_stay_lo_map       = os_strtoul(argv[7], NULL, 16);
	deep_sleep_param.gpio_stay_hi_map       = os_strtoul(argv[8], NULL, 16);

	if (argc == 9) {
		os_printf("---deep sleep test param : 0x%0X 0x%0X 0x%0X 0x%0X %d %d\r\n",
				  deep_sleep_param.gpio_index_map,
				  deep_sleep_param.gpio_edge_map,
				  deep_sleep_param.gpio_last_index_map,
				  deep_sleep_param.gpio_last_edge_map,
				  deep_sleep_param.sleep_time,
				  deep_sleep_param.wake_up_way);

#if (!CONFIG_SOC_BK7271) && (CONFIG_DEEP_PS)
		bk_enter_deep_sleep_mode(&deep_sleep_param);
#endif
	} else
		os_printf("---argc error!!! \r\n");
}

void cli_mac_ps_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if (CONFIG_MCU_PS | CONFIG_STA_PS)
	UINT32 dtim = 0;
#endif

#if PS_SUPPORT_MANUAL_SLEEP
	UINT32 standby_time = 0;
	UINT32 dtim_wait_time = 0;
#endif

	if (argc < 2)
		goto _invalid_mac_ps_arg;
#if CONFIG_FAKE_RTC_PS
	if (0 == os_strcmp(argv[1], "idleps")) {
		GLOBAL_INT_DECLARATION();
		int count = 0;
		bk_printf("[ARF]rwnxl_reset_evt\r\n");
		HAL_FATAL_ERROR_RECOVER(0); // rwnxl_reset_evt(0);

		rtos_delay_milliseconds(10);

		while (1) {
			GLOBAL_INT_DISABLE();
			evt_field_t field = ke_evt_get();
			GLOBAL_INT_RESTORE();

			if (!(field & KE_EVT_RESET_BIT))
				break;

			rtos_delay_milliseconds(10);
			if (++count > 10000) {
				bk_printf("%s: failed\r\n", __func__);
				break;
			}
		}

		//bk_enable_unconditional_sleep();
		count = 100;
		while ((1 == bk_unconditional_normal_sleep(0xFFFFFFFF, 1))) {
			rtos_delay_milliseconds(2);
			count--;
			if (count == 0) {
				bk_printf("IDLE_SLEEP timeout\r\n");
				break;
			}
		}

		bk_printf("idle Sleep out\r\n");
	}
#endif

#if CONFIG_STA_PS
	else if (0 == os_strcmp(argv[1], "rfwkup")) {
		mac_ps_wakeup_immediately();
		os_printf("done.\r\n");
	} else if (0 == os_strcmp(argv[1], "bcmc")) {
		if (argc != 3)
			goto _invalid_mac_ps_arg;

		dtim = os_strtoul(argv[2], NULL, 10);

		if (dtim == 0 || dtim == 1)
			power_save_sm_set_all_bcmc(dtim);
		else
			goto _invalid_mac_ps_arg;
	} else if (0 == os_strcmp(argv[1], "listen")) {
		if (argc != 4)
			goto _invalid_mac_ps_arg;

		if (0 == os_strcmp(argv[2], "dtim")) {
			dtim = os_strtoul(argv[3], NULL, 10);
			power_save_set_dtim_multi(dtim);

		} else
			goto _invalid_mac_ps_arg;

	} else if (0 == os_strcmp(argv[1], "dump")) {
#if CONFIG_MCU_PS
		mcu_ps_dump();
#endif
		power_save_dump();
	}
#endif
	else
		goto _invalid_mac_ps_arg;

	return;
_invalid_mac_ps_arg:
	os_printf("Usage:mac_ps {func} [param1] [param2]\r\n");
#if CONFIG_FAKE_RTC_PS
	os_printf("mac_ps {idleps}\r\n");
#endif
#if CONFIG_STA_PS
	os_printf("mac_ps {rfwkup}\r\n");
	os_printf("mac_ps {bcmc} {1|0}\r\n");
	os_printf("mac_ps {listen} {dtim} {dtim_val}\r\n");
#endif
	os_printf("mac_ps dump\r\n");
}

void cli_pwr_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int pwr = 0;

	if (argc != 3) {
		os_printf("Usage: pwr [hex:5~15].");
		return;
	}

	pwr = os_strtoul(argv[2], NULL, 16);
	if (0 == os_strcmp(argv[1], "sta")) {
		bk_wifi_sta_set_power(pwr);
	} else if (0 == os_strcmp(argv[1], "ap")) {
		bk_wifi_ap_set_power(pwr);
	}
}
#endif 
#endif //!CONFIG_SYSTEM_CTRL
#if CONFIG_BUCK_ENABLE //temp mofify 
/**
 * BK7236XX use buck power supply as default and should not close buck any time during use.
 * This function can only be used when buck power supply is unstable (hardware v4).
 * Please remove it when buck is stable (next hardware version).
*/
extern void sys_hal_buck_switch(uint32_t flag);
static void cli_pm_buck(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 flag = 0;
	if (argc != 2)
	{
		os_printf("set buck power supply invalid %d\r\n",argc);
		return;
	}

	flag = os_strtoul(argv[1], NULL, 10);

	sys_hal_buck_switch(flag);
}
#endif
#if CONFIG_SOC_BK7236XX
static void cli_pm_ana(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 flag = 0;
	if (argc != 2)
	{
		os_printf("set analog register invalid %d\r\n",argc);
		return;
	}

	flag = os_strtoul(argv[1], NULL, 10);

	if(flag)
	{
		/*will cause lv sleep abnormal*/
		sys_ll_set_ana_reg5_en_dco(0);
		sys_ll_set_ana_reg13_enpsram(0);

	}
	else
	{
		sys_ll_set_ana_reg5_en_dco(1);
		sys_ll_set_ana_reg13_enpsram(1);
	}
}

extern bk_err_t gpio_hal_switch_to_low_power_status(uint64_t skip_io);
static void cli_pm_gpio(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_SYS_CPU0
	UINT32 flag = 0;
	if (argc != 2)
	{
		os_printf("set analog register invalid %d\r\n",argc);
		return;
	}

	flag = os_strtoul(argv[1], NULL, 10);

	if(flag)
		gpio_hal_switch_to_low_power_status(0xC00);
#endif
}
static void cli_pm_vcore(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_SYS_CPU0
	UINT32 value = 0;
	if (argc != 2)
	{
		os_printf("set analog register invalid %d\r\n",argc);
		return;
	}

	value = os_strtoul(argv[1], NULL, 10);
	extern void sys_hal_v_core_h_sel(uint32_t value);

	sys_hal_v_core_h_sel(value);
#endif
}
#endif

#define PWR_CMD_CNT (sizeof(s_pwr_commands) / sizeof(struct cli_command))
static const struct cli_command s_pwr_commands[] = {
#if !CONFIG_SYSTEM_CTRL
#if CONFIG_MCU_PS
	{"mac_ps", "mac_ps {func} [param1] [param2]", cli_mac_ps_cmd},
	{"deep_sleep", "deep_sleep [param]", cli_deep_sleep_cmd},
#endif
#if CONFIG_TPC_PA_MAP
	{"pwr", "pwr {sta|ap} pwr", cli_pwr_cmd },
#endif
#endif //!CONFIG_SYSTEM_CTRL

#if CONFIG_SYS_CPU0
#if CONFIG_SYSTEM_CTRL
#if CONFIG_DEBUG_FIRMWARE
	{"pm", "pm [sleep_mode] [wake_source] [vote1] [vote2] [vote3] [param1] [param2] [param3]", cli_pm_cmd},
	{"dvfs", "dvfs [cksel_core] [ckdiv_core] [ckdiv_bus] [ckdiv_cpu0] [ckdiv_cpu1]", cli_dvfs_cmd},
	{"dvfs_auto_test", "dvfs_auto_test [period]", cli_dvfs_auto_test},
	{"pm_vote", "pm_vote [pm_sleep_mode] [pm_vote] [pm_vote_value] [pm_sleep_time]", cli_pm_vote_cmd},
	{"pm_debug", "pm_debug [debug_en_value]", cli_pm_debug},
	{"pm_lpo", "pm_lpo [lpo_type]", cli_pm_lpo},
	{"pm_vol", "pm_vol [vol_value]", cli_pm_vol},
	{"pm_clk", "pm_clk [module_name][clk_state]", cli_pm_clk},
	{"pm_power", "pm_power [module_name][ power state]", cli_pm_power},
	{"pm_freq", "pm_freq [module_name][ frequency]", cli_pm_freq},
	{"pm_ctrl", "pm_ctrl [ctrl_value]", cli_pm_ctrl},
	{"pm_pwr_state", "pm_pwr_state [pwr_state]", cli_pm_pwr_state},
	{"pm_auto_vote", "pm_auto_vote [auto_vote_value]", cli_pm_auto_vote},
	{"pm_rosc", "pm_rosc [rosc_accuracy_count_interval]", cli_pm_rosc_accuracy},
	{"pm_rosc_cali", "pm_rosc_cali [cali_mode][cal_intval]", cli_pm_rosc_cali},
	{"pm_rosc_pin", "pm_rosc_pin [lpo_clk:0:ana;1:dig]", cli_pm_clk_pin},
	{"pm_wakeup_source", "pm_wakeup_source [pm_sleep_mode]", cli_pm_wakeup_source},
	{"pm_rosc_ppm", "pm_rosc_ppm [interval] [count]", cli_pm_rosc_ppm},
	{"pm_cp1_ctrl", "pm_cp1_ctrl [cp1_auto_pw_ctrl]", cli_pm_cp1_ctrl},
	{"pm_boot_cp1", "pm_boot_cp1 [module_name] [ctrl_state:0x0:bootup; 0x1:shutdowm]", cli_pm_boot_cp1},
#if (CONFIG_CPU_CNT > 2)
	{"pm_boot_cp2", "pm_boot_cp2 [module_name] [ctrl_state:0x0:bootup; 0x1:shutdowm]", cli_pm_boot_cp2},
#endif
	{"pm_ldo", "pm_ldo[module_name][gpio id][gpio_output_state:0x0->low voltage, 0x1->high voltage]", cli_pm_ldo},
	{"pm_psram", "pm_psram[module_name][ctrl_state:0x0:power&clk on; 0x1:power&clk off]", cli_pm_psram},
#if CONFIG_BUCK_ENABLE  //temp mofify
	{"pm_buck", "pm_buck [1/0]", cli_pm_buck},
#endif
#if CONFIG_SOC_BK7236XX
	{"pm_ana", "pm_ana [1/0]", cli_pm_ana},
	{"pm_gpio", "pm_gpio [1/0]", cli_pm_gpio},
	{"pm_vcore", "pm_vcore [value]", cli_pm_vcore},
#endif

#else
	{"pm", "pm [sleep_mode] [wake_source] [vote1] [vote2] [vote3] [param1] [param2] [param3]", cli_pm_cmd},
	{"pm_vote", "pm_vote [pm_sleep_mode] [pm_vote] [pm_vote_value] [pm_sleep_time]", cli_pm_vote_cmd},
	{"pm_debug", "pm_debug [debug_en_value]", cli_pm_debug},
#endif //CONFIG_DEBUG_FIRMWARE
#endif //CONFIG_SYSTEM_CTRL
#endif //CONFIG_SYS_CPU0
};

int cli_pwr_init(void)
{
	return cli_register_commands(s_pwr_commands, PWR_CMD_CNT);
}
