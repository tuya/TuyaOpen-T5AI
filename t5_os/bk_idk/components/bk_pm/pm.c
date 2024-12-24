// Copyright 2021-2023 Beken
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
#include <modules/pm.h>
#include "sys_driver.h"
#include <driver/gpio.h>
#include "gpio_ll.h"
#include <driver/aon_rtc.h>
#include <os/mem.h>
#include "aon_pmu_driver.h"
#include <components/system.h>
#include <driver/uart.h>
#include "uart_statis.h"
#include "bk_uart.h"
#include <driver/touch.h>
#include <driver/touch_types.h>
#include <driver/pwr_clk.h>
#include <os/os.h>
#include "bk_pm_internal_api.h"
#include <driver/psram.h>

#if CONFIG_FLASH_ORIGIN_API
#include "bk_flash.h"
#else
#include "driver/flash.h"
#endif

#if CONFIG_INT_WDT
#include <driver/wdt.h>
#include <bk_wdt.h>
#endif
#if (CONFIG_CPU_CNT > 1)
#include <driver/mailbox_channel.h>
#endif
#if CONFIG_GPIO_WAKEUP_SUPPORT
#include "gpio_driver.h"
#endif
#include <driver/rosc_32k.h>

/*=====================DEFINE SECTION START=====================*/
#define PM_WAKEUP_SOURCE_MARK                           (WAKEUP_SOURCE_MARK)

#define ENTER_LOW_VOLTAGE_PROTECT_TIME                  (3) // 3ms
#define ENTER_LOW_VOLTAGE_WAKEUP_PROTECT_TIME           (5) // 5m = 3ms(dpll+xtal 26m stability time) + 2ms(mtime interval),mainly for bt wakeup handle interrupt
#define DEFAULT_WAKEUP_TIME                             (10000) // 10s
#define PM_LOWVOL_CB_SIZE                               (0x2)
#define PM_SLEEP_CB_ENTER_LOWVOL_INDEX                  (0x0)
#define PM_SLEEP_CB_EXIT_LOWVOL_INDEX                   (0x1)
#define PM_DEEPSLEEP_CB_SIZE                            (10)
#if CONFIG_PM_SUPER_DEEP_SLEEP
#define PM_SUPERDEEP_CB_SIZE                            (10)
#endif
#define PM_SLEEP_CB_IND_PRI_0                           (0)
#define PM_SLEEP_CB_IND_PRI_1                           (6)
#define PM_SLEEP_TIME_COMPENSATION_ENABLE               (0x0)
#define SWITCH_32K_WAIT                                 (5000) // 5s
#define SWITCH_32K_DELAY                                (1000) // 1s
#define PM_DEBUG_SYS_REG_BASE                           (SOC_SYSTEM_REG_BASE)
#define PM_DEBUG_PMU_REG_BASE                           (SOC_AON_PMU_REG_BASE)
#define PM_SEND_CMD_CP0_RESPONSE_TIME_OUT               (100)  //100ms

/*=====================DEFINE SECTION END=====================*/

/*=====================VARIABLE SECTION START=================*/
static uint8_t  s_debug_en                    = 0;
static uint32_t s_module_wakeup_time_ms       = DEFAULT_WAKEUP_TIME;

static pm_sleep_mode_e s_pm_sleep_mode        = PM_MODE_DEFAULT;
static uint32_t s_pm_wakeup_source            = 0;

static uint64_t s_previous_tick               = 0;
static uint64_t s_current_tick                = 0;

static uint32_t s_pm_on_modules;
static uint32_t s_pm_off_modules;
static uint32_t s_pm_rf_on_modules;
static uint32_t s_pm_rf_off_modules;
static uint64_t s_pm_sleeped_modules          = 0;
static uint64_t s_pm_enter_low_vol_modules    = 0;
static uint32_t s_pm_enter_deep_sleep_modules = 0;

static uint32_t s_pm_mcu_pm_state             = 0;
static uint8_t  s_pm_app_auto_vote_enable     = 1;
static uint32_t s_pm_lowvol_consume_time_exit_wfi = 0;
static uint32_t s_pm_phy_calibration_state    = 0;
/**
 * phy reinit has two part,one in BSP,another in wifi MAC
 * part 2 must follow part1
 * part1 only vaild in BT close,only wifi case
*/
static bool     s_pm_is_phy_reinit_flag       = false;
#if CONFIG_SYS_CPU0
static uint32_t s_pm_bakp_pm_state                      = 0;
static uint32_t s_pm_ahpb_pm_state                      = 0;
static uint32_t s_pm_audio_pm_state                     = 0;
static uint32_t s_pm_video_pm_state                     = 0;
static uint32_t s_pm_phy_pm_state                       = 0;
static uint32_t s_pm_cp1_auto_power_down_flag           = PM_CP1_AUTO_POWER_DOWN_CTRL;
static pm_mem_auto_ctrl_e s_pm_mem_auto_power_down_flag = PM_MEM_AUTO_CTRL_ENABLE;
#if (CONFIG_CPU_CNT > 1)
static uint32_t s_pm_cp1_psram_malloc_count_state       = 0;
#endif
#endif

static uint64_t s_bt_need_wakeup_time         = 0;
static system_wakeup_param_t s_bt_system_wakeup_param;
static touch_wakeup_param_t s_touch_wakeup_param;
#if CONFIG_SYS_CPU0
static pm_wakeup_source_e s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_NONE;
static pm_wakeup_source_e s_pm_exit_deepsleep_wakeup_source = PM_WAKEUP_SOURCE_INT_NONE;
#endif

#if CONFIG_PM_LIGHT_SLEEP
static pm_cb_conf_t s_pm_light_sleep_enter_cb_conf;
static pm_cb_conf_t s_pm_light_sleep_exit_cb_conf;
#endif
static __attribute__((section(".dtcm_sec_data "))) pm_cb_conf_t s_pm_lowvol_enter_exit_cb_conf[PM_LOWVOL_CB_SIZE][PM_DEV_ID_MAX];
static __attribute__((section(".dtcm_sec_data "))) pm_sleep_cb_t s_pm_deepsleep_enter_cb_conf[PM_DEEPSLEEP_CB_SIZE];
#if CONFIG_PM_SUPER_DEEP_SLEEP
static __attribute__((section(".dtcm_sec_data "))) pm_sleep_cb_t s_pm_superdeep_enter_cb_conf[PM_SUPERDEEP_CB_SIZE];
#endif

static __attribute__((section(".dtcm_sec_data "))) uint8_t s_pm_deepsleep_enter_cb_cnt[2] = {PM_SLEEP_CB_IND_PRI_0, PM_SLEEP_CB_IND_PRI_1};
#if CONFIG_PM_SUPER_DEEP_SLEEP
static __attribute__((section(".dtcm_sec_data "))) uint8_t s_pm_superdeep_enter_cb_cnt[2] = {PM_SLEEP_CB_IND_PRI_0, PM_SLEEP_CB_IND_PRI_1};
#endif

static uint8_t s_pm_cpu_freq[PM_DEV_ID_MAX];
static pm_cpu_freq_e s_pm_current_cpu_freq;

typedef struct
{
	pm_cb_extern32k_cfg_t cfg[PM_32K_MODULE_MAX];
	uint32_t module_count;
} pm_cb_module_cfg_t;

static pm_cb_module_cfg_t s_pm_cb_module_cfg;
static beken_semaphore_t s_sync_sema = NULL;

#if 1
uint32_t pm_wake_int_flag2;
#endif
/*=====================VARIABLE SECTION END=================*/

/*================FUNCTION DECLARATION SECTION START========*/
#if CONFIG_SYS_CPU0
static void pm_enter_low_vol_modules_config();
static void pm_enter_deep_sleep_modules_config();
static void pm_module_check_power_on(pm_power_module_name_e module);
static void pm_module_check_power_off(pm_power_module_name_e module);
static void pm_deep_sleep_wakeup_source_set();
bk_err_t pm_debug_module_state();
#endif
static void pm_check_power_on_module();
static bk_err_t pm_wakeup_from_deepsleep_handle();
static void pm_enter_normal_sleep();
static uint32_t pm_low_voltage_process();
static void pm_deep_sleep_process();
#if CONFIG_PM_SUPER_DEEP_SLEEP
static void pm_super_deep_sleep_process();
#endif

#if CONFIG_INT_WDT
extern int wdt_init(void);
#endif
extern void delay_us(UINT32 us);
/*================FUNCTION DECLARATION SECTION END========*/

/**
 * !NOTIFICATION HERE!
 * pm module is divided into 3 partitions as below:
 * -- 1. sleep mode switch
 * -- 2. voltage & freq ctrl
 * -- 3. debug
 * sleep mode switch is the main partition which further
 * divided into the following submodules:
 * -- 1. sleep state machine
 * -- 2. modules power ctrl
 * -- 3. wakeup source config
 * -- 4. sleep/wakeup functions
 */

void pm_hardware_init()
{
#if CONFIG_SYS_CPU0
	sys_drv_low_power_hardware_init();

	/*config vote for entering low vol modules*/
	pm_enter_low_vol_modules_config();

	/*config vote for entering deepsleep modules*/
	pm_enter_deep_sleep_modules_config();

	/*add sleep vote*/
	pm_sleep_vote_init();

	/*set the wakeup wakeup source to misc module*/
	pm_deep_sleep_wakeup_source_set();

	/*pm vote power on ticket for bakp module*/
#if CONFIG_BAKP_POWER_DOMAIN_PM_CONTROL
	bk_pm_module_vote_power_ctrl(POWER_SUB_MODULE_NAME_BAKP_PM, PM_POWER_MODULE_STATE_ON);
#endif
#endif
}

bk_err_t pm_module_wakeup_time_set(uint32_t module_name, uint32_t wakeup_time)
{
	s_module_wakeup_time_ms = wakeup_time;
	return BK_OK;
}
/*=========================SLEEP STATE MACHINE START========================*/
#if CONFIG_SYS_CPU0
static void pm_enter_low_vol_modules_config()
{
	uint32_t i = 0;
	pm_sleep_module_name_e enter_low_vol_modules[] = PM_ENTER_LOW_VOL_MODULES_CONFIG;

	for (i = 0; i < sizeof(enter_low_vol_modules) / sizeof(pm_sleep_module_name_e); i++)
	{
		s_pm_enter_low_vol_modules |= 0x1ULL << enter_low_vol_modules[i];
	}
}

static void pm_enter_deep_sleep_modules_config()
{
	uint32_t i = 0;
	pm_power_module_name_e enter_deep_sleep_modules[] = PM_ENTER_DEEP_SLEEP_MODULES_CONFIG;

	for (i = 0; i < sizeof(enter_deep_sleep_modules) / sizeof(pm_power_module_name_e); i++)
	{
		s_pm_enter_deep_sleep_modules |= 0x1 << enter_deep_sleep_modules[i];
	}
}
#endif

static uint32_t pm_check_protect_time(uint64_t current_tick, uint64_t previous_tick)
{
	if (s_bt_need_wakeup_time > current_tick)
	{
		if ((s_bt_need_wakeup_time - current_tick) < (ENTER_LOW_VOLTAGE_WAKEUP_PROTECT_TIME * bk_rtc_get_ms_tick_count()))
		{
			if (s_debug_en & 0x1)
			{
				os_printf("protect_time1 %lld %lld %d\r\n", s_bt_need_wakeup_time, current_tick, s_bt_system_wakeup_param.sleep_time);
			}
			return 0;
		}
	}
	else // s_bt_need_wakeup_time not set, it will be 0,or not clear value after bt wakeup
	{
		if (s_debug_en & 0x1)
		{
			os_printf("protect_time2 %lld %lld %d\r\n", s_bt_need_wakeup_time, current_tick, s_bt_system_wakeup_param.sleep_time);
		}
		return 0; // do something
	}
	return 1;
}

static uint32_t pm_check_and_ctrl_sleep()
{
	uint32_t sleep_tick = 0;
	pm_check_power_on_module();
	pm_wakeup_from_deepsleep_handle();

	if (s_pm_sleep_mode == PM_MODE_NORMAL_SLEEP)
	{
		pm_enter_normal_sleep();
	}
	else if (s_pm_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		if (s_debug_en & 0x1)
		{
#if CONFIG_SYS_CPU0
			os_printf("lowvol1 0x%X 0x%llX 0x%llX\r\n", s_pm_sleep_mode, s_pm_sleeped_modules, s_pm_enter_low_vol_modules);
			os_printf("lowvol2 0x%X 0x%X 0x%X 0x%X\r\n", s_pm_ahpb_pm_state, s_pm_video_pm_state, s_pm_audio_pm_state, s_pm_bakp_pm_state);
#endif
		}
		if ((s_pm_sleeped_modules & s_pm_enter_low_vol_modules) == s_pm_enter_low_vol_modules)
		{
#if CONFIG_AON_RTC
			s_current_tick = (uint32_t)bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
#endif
			if (s_pm_on_modules & (0x1 << POWER_MODULE_NAME_BTSP))
			{
				if (!pm_check_protect_time(s_current_tick, s_previous_tick))
				{
					return 0;
				}
			}

			sleep_tick = pm_low_voltage_process();

#if CONFIG_AON_RTC
			s_previous_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
#endif
		}
		else
		{
			pm_enter_normal_sleep();
		}
	}
	else if (s_pm_sleep_mode == PM_MODE_DEEP_SLEEP)
	{
		if (s_debug_en & 0x2)
		{
#if CONFIG_SYS_CPU0
			os_printf("deepsleep1 0x%X 0x%X\r\n", s_pm_off_modules, s_pm_enter_deep_sleep_modules);
			os_printf("deepsleep2 0x%X 0x%X 0x%X 0x%X\r\n", s_pm_ahpb_pm_state, s_pm_video_pm_state, s_pm_audio_pm_state, s_pm_bakp_pm_state);
#endif
		}

		if ((s_pm_off_modules & s_pm_enter_deep_sleep_modules) == s_pm_enter_deep_sleep_modules)
		{
			pm_deep_sleep_process();
		}
		else
		{
			pm_enter_normal_sleep();
		}
	}
#if CONFIG_PM_SUPER_DEEP_SLEEP
	else if (s_pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
	{
		if (s_debug_en & 0x2)
		{
#if CONFIG_SYS_CPU0
			os_printf("superdeepsleep1 0x%X 0x%X\r\n", s_pm_off_modules, s_pm_enter_deep_sleep_modules);
			os_printf("superdeepsleep2 0x%X 0x%X 0x%X\r\n", s_pm_ahpb_pm_state, s_pm_video_pm_state, s_pm_audio_pm_state);
#endif
		}

		if ((s_pm_off_modules & s_pm_enter_deep_sleep_modules) == s_pm_enter_deep_sleep_modules)
		{
			pm_super_deep_sleep_process();
		}
		else
		{
			pm_enter_normal_sleep();
		}
	}
#endif
	else
	{
		if (s_debug_en & 0x1)
		{
#if CONFIG_SYS_CPU0
			os_printf("lowvol1 0x%X 0x%llX 0x%llX\r\n", s_pm_sleep_mode, s_pm_sleeped_modules, s_pm_enter_low_vol_modules);
			os_printf("lowvol2 0x%X 0x%X\r\n", s_pm_video_pm_state, s_pm_audio_pm_state);
#endif
		}
		if ((s_pm_sleeped_modules & s_pm_enter_low_vol_modules) == s_pm_enter_low_vol_modules)
		{
#if CONFIG_AON_RTC
			s_current_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
#endif
			if (s_pm_on_modules & (0x1 << POWER_MODULE_NAME_BTSP))
			{
				if (!pm_check_protect_time(s_current_tick, s_previous_tick))
				{
					return 0;
				}
			}
			sleep_tick = pm_low_voltage_process();

#if CONFIG_AON_RTC
			s_previous_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
#endif
		}
		else
		{
			pm_enter_normal_sleep();
		}
	}

	return sleep_tick;
}

uint32_t pm_state_machine()
{
	uint32_t missed_ticks = 0;

	missed_ticks = pm_check_and_ctrl_sleep();

#if PM_SLEEP_TIME_COMPENSATION_ENABLE
	missed_ticks = (missed_ticks) / (bk_rtc_get_ms_tick_count() * (rtos_get_ms_per_tick()));
#else
	missed_ticks = 0;
#endif

	return missed_ticks;
}

bk_err_t pm_management(uint32_t sleep_ticks)
{
#if CONFIG_MCU_PS
#if CONFIG_PM_LIGHT_SLEEP
	uint64_t previous_tick = 0;
	uint64_t current_tick = 0;
	uint64_t missed_ticks = 0;
	int ret = 0;
	if (s_pm_mcu_pm_state)
	{
		return BK_OK;
	}

	if ((s_pm_light_sleep_enter_cb_conf.cb == NULL) || (s_pm_light_sleep_exit_cb_conf.cb == NULL))
	{

		return BK_FAIL;
	}
	ret = s_pm_light_sleep_enter_cb_conf.cb(sleep_ticks * rtos_get_ms_per_tick(), s_pm_light_sleep_enter_cb_conf.args);
	if (ret)
	{
		return BK_FAIL;
	}
	previous_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	current_tick = previous_tick;
	while (((current_tick - previous_tick)) < (sleep_ticks * rtos_get_ms_per_tick() * bk_rtc_get_ms_tick_count()))
	{
		current_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	}

	// os_printf("idle task after:%d \r\n",sleep_ticks);
	missed_ticks = pm_state_machine();
	missed_ticks = sleep_ticks;
	// bk_update_tick(missed_ticks);

	s_pm_light_sleep_exit_cb_conf.cb(sleep_ticks * rtos_get_ms_per_tick(), s_pm_light_sleep_exit_cb_conf.args);
#else
	uint32_t missed_ticks = 0;
	if (s_pm_mcu_pm_state)
	{
		return BK_OK;
	}
	missed_ticks = pm_state_machine();
	if (missed_ticks != 0)
	{
		// bk_update_tick(missed_ticks);
	}
#endif
#else
#if CONFIG_SYS_CPU1

	if (bk_pm_cp1_ctrl_state_get() == 0x0)
	{
		bk_pm_cp1_ctrl_state_set(PM_MAILBOX_COMMUNICATION_FINISH);
		pm_cp1_mailbox_response(PM_CPU1_BOOT_READY_CMD, 0x1);
		// os_printf("cpu1 already\r\n");
	}

	pm_enter_normal_sleep();
#endif
#endif
	return BK_OK;
}

bk_err_t bk_pm_suppress_ticks_and_sleep(uint32_t sleep_ticks)
{
	return pm_management(sleep_ticks);
}
/*=========================SLEEP STATE MACHINE END========================*/

/*=========================COMMON PM API START========================*/
bk_err_t bk_pm_module_vote_power_ctrl(pm_power_module_name_e module, pm_power_module_state_e power_state)
{
#if CONFIG_SYS_CPU1
#if CONFIG_MAILBOX
	uint64_t previous_tick  = 0;
	uint64_t current_tick   = 0;
	mb_chnl_cmd_t mb_cmd = {0};
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	bk_pm_cp1_pwr_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);
	mb_cmd.hdr.cmd = PM_POWER_CTRL_CMD;
	mb_cmd.param1 = module;
	mb_cmd.param2 = power_state;
	mb_cmd.param3 = 0;
	mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	GLOBAL_INT_RESTORE();

	previous_tick = pm_cp1_aon_rtc_counter_get();
	current_tick = previous_tick;
	while((current_tick - previous_tick) < (PM_SEND_CMD_CP0_RESPONSE_TIME_OUT*PM_AON_RTC_DEFAULT_TICK_COUNT))
	{
	    if (bk_pm_cp1_pwr_ctrl_state_get()) // wait the cp0 response
	    {
			break;
	    }
	    current_tick = pm_cp1_aon_rtc_counter_get();
	}

	if(!bk_pm_cp1_pwr_ctrl_state_get())
	{
	    os_printf("cp1 power ctrl[%d] time out\r\n",module);
	}

	if (s_debug_en & 0x2)
		os_printf("cpu1 vote power\r\n");
#endif
	return BK_OK;
#elif CONFIG_SYS_CPU2
	return BK_OK;
#elif CONFIG_SYS_CPU0
	uint32_t off_modules = 0;
	GLOBAL_INT_DECLARATION();

	if (power_state == PM_POWER_MODULE_STATE_ON) // power on
	{
		if ( (module == PM_POWER_SUB_MODULE_NAME_BAKP_TIMER1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_UART1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_UART2)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_SPI1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_I2C0)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_I2C1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_SADC)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_PWM1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_PWM0)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_TRNG)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_IRDA)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_SDIO)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_COMSEG)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_DMA0)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_DMA1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_LA)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_PM))
		{
			GLOBAL_INT_DISABLE();
			s_pm_bakp_pm_state |= 0x1 << (module % (PM_POWER_MODULE_NAME_BAKP * PM_MODULE_SUB_POWER_DOMAIN_MAX));
			s_pm_off_modules &= ~(0x1 << PM_POWER_MODULE_NAME_BAKP);
			s_pm_on_modules |= 0x1 << PM_POWER_MODULE_NAME_BAKP;

			GLOBAL_INT_RESTORE();

			if (0x0 == sys_drv_module_power_state_get(PM_POWER_MODULE_NAME_BAKP))
			{
				return BK_OK;
			}
			else
			{
				sys_drv_module_power_ctrl(PM_POWER_MODULE_NAME_BAKP, power_state);
			}
		}
		else if ( (module == PM_POWER_SUB_MODULE_NAME_AHBP_CAN)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_QSPI)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_USB)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_PSRAM)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_QSPI1)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_ENET)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_SCR)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_LIN))
		{
			GLOBAL_INT_DISABLE();
			s_pm_ahpb_pm_state |= 0x1 << (module % (PM_POWER_MODULE_NAME_AHBP * PM_MODULE_SUB_POWER_DOMAIN_MAX));
			s_pm_off_modules &= ~(0x1 << PM_POWER_MODULE_NAME_AHBP);
			s_pm_on_modules |= 0x1 << PM_POWER_MODULE_NAME_AHBP;

			GLOBAL_INT_RESTORE();

			if (0x0 == sys_drv_module_power_state_get(PM_POWER_MODULE_NAME_AHBP))
			{
				return BK_OK;
			}
			else
			{
				sys_drv_module_power_ctrl(PM_POWER_MODULE_NAME_AHBP, power_state);
			}
		}
	  	else if ( (module == PM_POWER_SUB_MODULE_NAME_AUDP_FFT)
			|| (module == PM_POWER_SUB_MODULE_NAME_AUDP_SBC)
			|| (module == PM_POWER_SUB_MODULE_NAME_AUDP_AUDIO)
			|| (module == PM_POWER_SUB_MODULE_NAME_AUDP_I2S))
		{
			GLOBAL_INT_DISABLE();
			s_pm_audio_pm_state |= 0x1 << (module % (PM_POWER_MODULE_NAME_AUDP * PM_MODULE_SUB_POWER_DOMAIN_MAX));
			s_pm_off_modules &= ~(0x1 << PM_POWER_MODULE_NAME_AUDP);
			s_pm_on_modules |= 0x1 << PM_POWER_MODULE_NAME_AUDP;
			s_pm_sleeped_modules &= ~(0x1 << PM_POWER_MODULE_NAME_AUDP);
			GLOBAL_INT_RESTORE();

			if (s_pm_cp1_auto_power_down_flag)
			{
				pm_module_check_power_on(PM_POWER_MODULE_NAME_CPU1);
			}
			if (0x0 == sys_drv_module_power_state_get(PM_POWER_MODULE_NAME_AUDP))
			{
				return BK_OK;
			}
			else
			{
				if (s_pm_mem_auto_power_down_flag)
				{
					pm_module_check_power_on(PM_POWER_MODULE_NAME_MEM3);
				}
				sys_drv_module_power_ctrl(PM_POWER_MODULE_NAME_AUDP, power_state);
			}
		}
		else if ((module == PM_POWER_SUB_MODULE_NAME_VIDP_DMA2D)
			|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_YUVBUF)
			|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_JPEG_EN)
			|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_JPEG_DE)
			|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_LCD)
			|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_ROTT)
			|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_SCAL0)
			|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_SCAL1)
			|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_H264))
		{
			GLOBAL_INT_DISABLE();
			s_pm_video_pm_state |= 0x1 << (module % (PM_POWER_MODULE_NAME_VIDP * PM_MODULE_SUB_POWER_DOMAIN_MAX));
			s_pm_off_modules &= ~(0x1 << PM_POWER_MODULE_NAME_VIDP);
			s_pm_on_modules |= 0x1 << PM_POWER_MODULE_NAME_VIDP;
			s_pm_sleeped_modules &= ~(0x1 << PM_POWER_MODULE_NAME_VIDP);
			GLOBAL_INT_RESTORE();

			if (s_pm_cp1_auto_power_down_flag)
			{
				pm_module_check_power_on(PM_POWER_MODULE_NAME_CPU1);
			}
			if (0x0 == sys_drv_module_power_state_get(PM_POWER_MODULE_NAME_VIDP))
			{
				// GLOBAL_INT_RESTORE();
				return BK_OK;
			}
			else
			{
				if (s_pm_mem_auto_power_down_flag)
				{
					pm_module_check_power_on(PM_POWER_MODULE_NAME_MEM3);
				}
				sys_drv_module_power_ctrl(PM_POWER_MODULE_NAME_VIDP, power_state);
			}
		}
		else if ((module == PM_POWER_SUB_MODULE_NAME_PHY_BT)
			|| (module == PM_POWER_SUB_MODULE_NAME_PHY_WIFI))
		{
			GLOBAL_INT_DISABLE();
			s_pm_phy_pm_state |= 0x1 << (module % (PM_POWER_MODULE_NAME_PHY * PM_MODULE_SUB_POWER_DOMAIN_MAX));
			GLOBAL_INT_RESTORE();
			if ((0x0 == sys_drv_module_power_state_get(PM_POWER_MODULE_NAME_PHY)) && (s_pm_phy_calibration_state == 0x1))
			{
				if (s_debug_en & 0x2)
					os_printf("phy power on already\r\n");
				return BK_OK;
			}
			else
			{
				sys_drv_module_power_ctrl(PM_POWER_MODULE_NAME_PHY, power_state);
				if (0x0 == sys_drv_module_power_state_get(PM_POWER_MODULE_NAME_PHY))
				{
#if CONFIG_SYSTEM_CTRL
#if CONFIG_WIFI_ENABLE
					extern void phy_wakeup_reinit();
					phy_wakeup_reinit();
#else
					extern void phy_wakeup_for_bluetooth();
					phy_wakeup_for_bluetooth();
#endif
					s_pm_is_phy_reinit_flag = true;
#endif
					GLOBAL_INT_DISABLE();
					s_pm_phy_calibration_state = 0x1;
					s_pm_off_modules &= ~(0x1 << PM_POWER_MODULE_NAME_PHY);
					s_pm_on_modules |= 0x1 << PM_POWER_MODULE_NAME_PHY;
					GLOBAL_INT_RESTORE();
				}
				else
				{
					if (s_debug_en & 0x2)
					{
						os_printf("phy power on fail 0x%x\r\n", sys_drv_module_power_state_get(PM_POWER_MODULE_NAME_PHY));
					}
					return BK_FAIL;
				}
			}
		}
		else if (module == PM_POWER_MODULE_NAME_BTSP)
		{
			if (s_pm_mem_auto_power_down_flag)
			{
				pm_module_check_power_on(PM_POWER_MODULE_NAME_MEM3);
			}
			sys_drv_module_power_ctrl(module, power_state);
			GLOBAL_INT_DISABLE();
			s_pm_off_modules &= ~(0x1 << PM_POWER_MODULE_NAME_BTSP);
			s_pm_on_modules |= 0x1 << PM_POWER_MODULE_NAME_BTSP;
			s_pm_sleeped_modules &= ~(0x1 << PM_POWER_MODULE_NAME_BTSP);
			GLOBAL_INT_RESTORE();
		}
		else
		{
			sys_drv_module_power_ctrl(module, power_state);
			if (module == PM_POWER_MODULE_NAME_PHY) // phy init call this, phy calibration itself
			{
				s_pm_phy_calibration_state = 0x1;
			}
			GLOBAL_INT_DISABLE();
			s_pm_off_modules &= ~(0x1 << module);
			s_pm_on_modules |= 0x1 << module;
			GLOBAL_INT_RESTORE();
		}
	}
	else // power off
	{
		if ( (module == PM_POWER_SUB_MODULE_NAME_BAKP_TIMER1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_UART1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_UART2)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_SPI1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_I2C0)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_I2C1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_SADC)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_PWM1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_PWM0)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_TRNG)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_IRDA)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_SDIO)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_COMSEG)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_DMA0)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_DMA1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_LA)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_PM))
		{
			GLOBAL_INT_DISABLE();
			s_pm_bakp_pm_state &= ~(0x1 << (module % (PM_POWER_MODULE_NAME_BAKP * PM_MODULE_SUB_POWER_DOMAIN_MAX)));

			if (0x0 == s_pm_bakp_pm_state)
			{
				sys_drv_module_power_ctrl(PM_POWER_MODULE_NAME_BAKP, power_state);
				s_pm_off_modules |= 0x1 << PM_POWER_MODULE_NAME_BAKP;
				s_pm_on_modules &= ~(0x1 << PM_POWER_MODULE_NAME_BAKP);

			}
			GLOBAL_INT_RESTORE();
		}
		else if ( (module == PM_POWER_SUB_MODULE_NAME_AHBP_CAN)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_QSPI)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_USB)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_PSRAM)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_QSPI1)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_ENET)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_SCR)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_LIN))
		{
			GLOBAL_INT_DISABLE();
			s_pm_ahpb_pm_state &= ~(0x1 << (module % (PM_POWER_MODULE_NAME_AHBP * PM_MODULE_SUB_POWER_DOMAIN_MAX)));

			if (0x0 == s_pm_ahpb_pm_state)
			{
				sys_drv_module_power_ctrl(PM_POWER_MODULE_NAME_AHBP, power_state);
				s_pm_off_modules |= 0x1 << PM_POWER_MODULE_NAME_AHBP;
				s_pm_on_modules &= ~(0x1 << PM_POWER_MODULE_NAME_AHBP);

			}
			GLOBAL_INT_RESTORE();
		}
		else if ((module == PM_POWER_SUB_MODULE_NAME_AUDP_FFT)
			|| (module == PM_POWER_SUB_MODULE_NAME_AUDP_SBC)
			|| (module == PM_POWER_SUB_MODULE_NAME_AUDP_AUDIO)
			|| (module == PM_POWER_SUB_MODULE_NAME_AUDP_I2S))
		{
			GLOBAL_INT_DISABLE();
			s_pm_audio_pm_state &= ~(0x1 << (module % (PM_POWER_MODULE_NAME_AUDP * PM_MODULE_SUB_POWER_DOMAIN_MAX)));
			if (0x0 == s_pm_audio_pm_state)
			{
				sys_drv_module_power_ctrl(PM_POWER_MODULE_NAME_AUDP, power_state);
				s_pm_off_modules |= 0x1 << PM_POWER_MODULE_NAME_AUDP;
				s_pm_on_modules &= ~(0x1 << PM_POWER_MODULE_NAME_AUDP);
				s_pm_sleeped_modules |= (0x1 << PM_POWER_MODULE_NAME_AUDP);
			}
			GLOBAL_INT_RESTORE();
		}
		else if ((module == PM_POWER_SUB_MODULE_NAME_VIDP_DMA2D)
			|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_YUVBUF)
			|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_JPEG_EN)
			|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_JPEG_DE)
			|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_LCD)
			|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_ROTT)
			|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_SCAL0)
			|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_SCAL1)
			|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_H264))
		{
			GLOBAL_INT_DISABLE();
			s_pm_video_pm_state &= ~(0x1 << (module % (PM_POWER_MODULE_NAME_VIDP * PM_MODULE_SUB_POWER_DOMAIN_MAX)));

			if (0x0 == s_pm_video_pm_state)
			{
				sys_drv_module_power_ctrl(PM_POWER_MODULE_NAME_VIDP, power_state);
				s_pm_off_modules |= 0x1 << PM_POWER_MODULE_NAME_VIDP;
				s_pm_on_modules &= ~(0x1 << PM_POWER_MODULE_NAME_VIDP);
				s_pm_sleeped_modules |= (0x1 << PM_POWER_MODULE_NAME_VIDP);
			}
			GLOBAL_INT_RESTORE();
		}
		else if ((module == PM_POWER_SUB_MODULE_NAME_PHY_BT)
			|| (module == PM_POWER_SUB_MODULE_NAME_PHY_WIFI))
		{
			GLOBAL_INT_DISABLE();
			s_pm_phy_pm_state &= ~(0x1 << (module % (PM_POWER_MODULE_NAME_PHY * PM_MODULE_SUB_POWER_DOMAIN_MAX)));

			if (0x0 == s_pm_phy_pm_state)
			{
				sys_drv_module_power_ctrl(PM_POWER_MODULE_NAME_PHY, power_state);
				s_pm_phy_calibration_state = 0x0;
				s_pm_off_modules |= 0x1 << PM_POWER_MODULE_NAME_PHY;
				s_pm_on_modules &= ~(0x1 << PM_POWER_MODULE_NAME_PHY);
			}
			GLOBAL_INT_RESTORE();
		}
		else if (module == PM_POWER_MODULE_NAME_CPU1)
		{
			sys_drv_module_power_ctrl(module, power_state);
			GLOBAL_INT_DISABLE();
			s_pm_off_modules |= 0x1 << module;
			s_pm_on_modules &= ~(0x1 << module);
			GLOBAL_INT_RESTORE();
			#if CONFIG_SYS_CPU0&& (CONFIG_CPU_CNT > 1)
			bk_pm_cp1_work_state_set(PM_MAILBOX_COMMUNICATION_INIT);
			#endif
		}
		else
		{
			if ((module == PM_POWER_MODULE_NAME_VIDP)
				|| (module == PM_POWER_MODULE_NAME_AUDP)
				|| (module == PM_POWER_MODULE_NAME_PHY))
			{
				// GLOBAL_INT_RESTORE();
				if (s_debug_en & 0x2)
					os_printf("module[%d] can not directly power off\r\n", module);
				return BK_FAIL;
			}

			if (s_pm_cp1_auto_power_down_flag)
			{
				if (module == PM_POWER_MODULE_NAME_CPU1)
				{
					if (!((s_pm_off_modules & (0x1 << PM_POWER_MODULE_NAME_AUDP)) && (s_pm_off_modules & (0x1 << PM_POWER_MODULE_NAME_VIDP))))
					{
						// GLOBAL_INT_RESTORE();
						if (s_debug_en & 0x2)
							os_printf("module[%d] can not directly power off\r\n", module);
						return BK_FAIL;
					}
				}
			}
			sys_drv_module_power_ctrl(module, power_state);
			GLOBAL_INT_DISABLE();
			s_pm_off_modules |= 0x1 << module;
			s_pm_on_modules &= ~(0x1 << module);
			GLOBAL_INT_RESTORE();
		}
	}
	if (s_pm_cp1_auto_power_down_flag)
	{
		off_modules = (0x1 << PM_POWER_MODULE_NAME_VIDP) | (0x1 << PM_POWER_MODULE_NAME_AUDP);
		if ((s_pm_off_modules & off_modules) == off_modules)
		{
			if (sys_drv_module_power_state_get(PM_POWER_MODULE_NAME_VIDP) && sys_drv_module_power_state_get(PM_POWER_MODULE_NAME_AUDP))
			{
				pm_module_check_power_off(PM_POWER_MODULE_NAME_CPU1);
			}
		}
	}
	if (s_debug_en & 0x2)
		os_printf("cpu0 vote power 0x%X 0x%X\r\n", s_pm_on_modules, s_pm_off_modules);
	return BK_OK;
#endif
}
#if CONFIG_SYS_CPU0
uint32_t bk_pm_low_vol_vote_state_get()
{
	return ((s_pm_sleeped_modules & s_pm_enter_low_vol_modules) == s_pm_enter_low_vol_modules);
}
bk_err_t pm_cp1_psram_malloc_count_state_set(uint32_t value)
{
	#if (CONFIG_CPU_CNT > 1)
	s_pm_cp1_psram_malloc_count_state = value;
	#endif
	return BK_OK;
}
static bk_err_t pm_cp1_psram_malloc_state_get()
{
	#if (CONFIG_CPU_CNT > 1)
	s_pm_cp1_psram_malloc_count_state = bk_pm_get_cp1_psram_malloc_count();
	#endif
	return BK_OK;
}
static bk_err_t pm_psram_malloc_state_and_power_ctrl()
{
  #if CONFIG_PSRAM_AS_SYS_MEMORY

	uint32_t cp0_psram_malloc_count = 0;
	uint32_t cp1_psram_malloc_count = 0;
	/*get the cp1 psram malloc count*/
	#if (CONFIG_CPU_CNT > 1)
	cp1_psram_malloc_count = s_pm_cp1_psram_malloc_count_state;
	if (s_debug_en == 64)
	{
		if(s_pm_cp1_psram_malloc_count_state > 0)
		{
			os_printf("CP1 psram malloc count[%d] > 0\r\n",cp1_psram_malloc_count);
			os_printf("Power consumption will get higher,free them\r\n");

			bk_pm_dump_cp1_psram_malloc_info();
		}
	}
	#endif
	/*get the cp0 psram malloc count*/
	cp0_psram_malloc_count = bk_psram_heap_get_used_count();
	if (s_debug_en == 64)
	{
		if(cp0_psram_malloc_count > 0)
		{
			os_printf("CP0 psram malloc count[%d] > 0\r\n",cp0_psram_malloc_count);
			os_printf("Power consumption will get higher,free them\r\n");

			bk_psram_heap_get_used_state();
		}
	}
	if((cp0_psram_malloc_count == 0)&&(cp1_psram_malloc_count == 0))
	{
		bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_AS_MEM,PM_POWER_MODULE_STATE_OFF);
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_PSRAM, PM_POWER_MODULE_STATE_OFF);
	}

  #endif
	return BK_OK;
}
#endif
bk_err_t bk_pm_module_vote_sleep_ctrl(pm_sleep_module_name_e module, uint32_t sleep_state, uint32_t sleep_time)
{
#if CONFIG_SYS_CPU1
#if CONFIG_MAILBOX
	uint64_t previous_tick  = 0;
	uint64_t current_tick   = 0;

	if(module == PM_SLEEP_MODULE_NAME_LOG)
	{
		return BK_OK;
	}

	mb_chnl_cmd_t mb_cmd = {0};
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	bk_pm_cp1_sleep_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);
	mb_cmd.hdr.cmd = PM_SLEEP_CTRL_CMD;
	mb_cmd.param1 = module;
	mb_cmd.param2 = sleep_state;
	mb_cmd.param3 = sleep_time;
	mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	GLOBAL_INT_RESTORE();

	previous_tick = pm_cp1_aon_rtc_counter_get();
	current_tick = previous_tick;
	while((current_tick - previous_tick) < (PM_SEND_CMD_CP0_RESPONSE_TIME_OUT*PM_AON_RTC_DEFAULT_TICK_COUNT))
	{
	    if (bk_pm_cp1_sleep_ctrl_state_get()) // wait the cp0 response
	    {
			break;
	    }
	    current_tick = pm_cp1_aon_rtc_counter_get();
	}

	if(!bk_pm_cp1_sleep_ctrl_state_get())
	{
	    os_printf("cp1 wait cp0 vote sleep[%d] time out\r\n",module);
	}
#endif
	return BK_OK;
#elif CONFIG_SYS_CPU2
	return BK_OK;
#elif CONFIG_SYS_CPU0
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	if (sleep_state == 0x1) // enter sleep
	{
		if (module == PM_SLEEP_MODULE_NAME_BTSP)
		{
			s_bt_system_wakeup_param.wifi_bt_wakeup = BT_WAKEUP;
			s_pm_wakeup_source |= 0x1 << PM_WAKEUP_SOURCE_INT_SYSTEM_WAKE;
			s_bt_system_wakeup_param.sleep_time = ((sleep_time * bk_rtc_get_ms_tick_count()) / 1000) + 1; // bt provide sleep time convent to 32k tick;+1:protect the precision miss
#if CONFIG_AON_RTC
			s_bt_need_wakeup_time = bk_aon_rtc_get_current_tick(AON_RTC_ID_1) + s_bt_system_wakeup_param.sleep_time;
#endif
			/*set bt wakeup source*/
			sys_drv_enable_bt_wakeup_source();
		}
		else if (module == PM_SLEEP_MODULE_NAME_WIFIP_MAC)
		{
			// s_wifi_system_wakeup_param.wifi_bt_wakeup = WIFI_WAKEUP;
			s_pm_wakeup_source |= 0x1 << PM_WAKEUP_SOURCE_INT_SYSTEM_WAKE;
			sys_drv_enable_mac_wakeup_source();
		}
		else if (module == PM_SLEEP_MODULE_NAME_APP)
		{
		}

		s_pm_sleeped_modules |= 0x1ULL << module;
	}
	else // exit sleep
	{
		if (module == PM_SLEEP_MODULE_NAME_BTSP)
		{
			s_bt_system_wakeup_param.wifi_bt_wakeup = BT_WAKEUP;
			s_bt_system_wakeup_param.sleep_time = 0;
			s_bt_need_wakeup_time = 0;
			/*disable bt wakeup source*/
			aon_pmu_drv_clear_wakeup_source(WAKEUP_SOURCE_INT_BT);
			// os_printf("bt exit sleep\r\n");
		}
		else if (module == PM_SLEEP_MODULE_NAME_WIFIP_MAC)
		{
			/*disable WIFI wakeup source*/
			aon_pmu_drv_clear_wakeup_source(WAKEUP_SOURCE_INT_WIFI);
		}
		else if (module == PM_SLEEP_MODULE_NAME_APP)
		{
		}
		s_pm_sleeped_modules &= ~(0x1 << module);
	}
	if (s_debug_en & 0x1)
		os_printf("pm sleep state 0x%llX 0x%x %d\r\n", s_pm_sleeped_modules, s_pm_on_modules, module);
	GLOBAL_INT_RESTORE();

	return BK_OK;
#endif
}

bk_err_t bk_pm_sleep_mode_set(pm_sleep_mode_e sleep_mode)
{
	s_pm_sleep_mode = sleep_mode;

	if (s_pm_sleep_mode == PM_MODE_DEEP_SLEEP)
	{
		for (uint8_t i = 0; i < PM_DEEPSLEEP_CB_SIZE; i++)
		{
			if (s_pm_deepsleep_enter_cb_conf[i].cfg.cb != NULL)
			{
				s_pm_deepsleep_enter_cb_conf[i].cfg.cb(0, s_pm_deepsleep_enter_cb_conf[i].cfg.args);
			}
		}
	}
#if CONFIG_PM_SUPER_DEEP_SLEEP
	else if (s_pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
	{
		for (uint8_t i = 0; i < PM_SUPERDEEP_CB_SIZE; i++)
		{
			if (s_pm_superdeep_enter_cb_conf[i].cfg.cb != NULL)
			{
				// os_printf("%d %d\r\n", i, s_pm_superdeep_enter_cb_conf[i].id);
				s_pm_superdeep_enter_cb_conf[i].cfg.cb(0, s_pm_superdeep_enter_cb_conf[i].cfg.args);
			}
		}
	}
#endif
	else if (s_pm_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		// to do
	}
	else
	{
	}

	return BK_OK;
}

bk_err_t bk_pm_wakeup_source_set(pm_wakeup_source_e wakeup_source, void *source_param)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	s_pm_wakeup_source |= 0x1 << wakeup_source;
	if (source_param == NULL)
	{
		GLOBAL_INT_RESTORE();
		return BK_OK;
	}
	switch (wakeup_source)
	{
	case PM_WAKEUP_SOURCE_INT_GPIO:
		break;

	case PM_WAKEUP_SOURCE_INT_RTC:
		break;

	case PM_WAKEUP_SOURCE_INT_SYSTEM_WAKE: // wifi/bt
		break;

	case PM_WAKEUP_SOURCE_INT_USBPLUG:
		break;

	case PM_WAKEUP_SOURCE_INT_TOUCHED:
		os_memcpy(&s_touch_wakeup_param, (touch_wakeup_param_t *)source_param, sizeof(touch_wakeup_param_t));
		break;

	default:
		break;
	}
	GLOBAL_INT_RESTORE();
	return BK_OK;
}
/*=========================COMMON PM API END========================*/

/*=========================SPECIFIC API START========================*/
uint32_t bk_pm_mcu_pm_state_get()
{
	return s_pm_mcu_pm_state;
}

bk_err_t bk_pm_mcu_pm_ctrl(uint32_t power_state)
{
	s_pm_mcu_pm_state = power_state;
	return BK_OK;
}

void bk_pm_enter_sleep()
{
	uint32_t sleep_ticks = 0;
	pm_management(sleep_ticks);
}

uint32_t bk_pm_app_auto_vote_state_get()
{
	return s_pm_app_auto_vote_enable;
}

bk_err_t bk_pm_app_auto_vote_state_set(uint32_t value)
{
	s_pm_app_auto_vote_enable = value;
	return BK_OK;
}

uint32_t bk_pm_wakeup_from_lowvol_consume_time_get()
{
	return ((s_pm_lowvol_consume_time_exit_wfi * 1000) / bk_rtc_get_ms_tick_count()); // unit: us
}

uint32_t bk_pm_cp1_boot_flag_get()
{
	return 0; // s_pm_cp1_boot_ready;
}

uint32_t bk_pm_phy_cali_state_get()
{
	return s_pm_phy_calibration_state;
}

bool bk_pm_phy_reinit_flag_get()
{
	return s_pm_is_phy_reinit_flag;
}

void bk_pm_phy_reinit_flag_clear()
{
	s_pm_is_phy_reinit_flag = false;
}
/*=========================SPECIFIC API END========================*/

/*=========================MODULES POWER CTRL START========================*/
static void pm_check_power_on_module()
{
#if CONFIG_SYS_CPU0
	uint32_t off_modules = 0;
#endif

	if (!(s_pm_on_modules & ((0x1 << PM_POWER_MODULE_NAME_BTSP)))) // when the module not power on , set the module sleep state
	{
		s_pm_sleeped_modules |= 0x1 << PM_POWER_MODULE_NAME_BTSP;
		s_pm_off_modules |= 0x1 << PM_POWER_MODULE_NAME_BTSP;
		// os_printf("bt not power on \r\n");
	}

	if (!(s_pm_on_modules & (0x1 << PM_POWER_MODULE_NAME_WIFIP_MAC))) // when the module not power on , set the module sleep state
	{
		s_pm_sleeped_modules |= 0x1 << PM_POWER_MODULE_NAME_WIFIP_MAC;
		s_pm_off_modules |= 0x1 << PM_POWER_MODULE_NAME_WIFIP_MAC;
		// os_printf("wifi not power on \r\n");
	}

	if (!(s_pm_on_modules & (0x1 << PM_POWER_MODULE_NAME_AUDP))) // when the module not power on , set the module sleep state
	{
		s_pm_sleeped_modules |= 0x1 << PM_POWER_MODULE_NAME_AUDP;
		s_pm_off_modules |= 0x1 << PM_POWER_MODULE_NAME_AUDP;
		// os_printf("audio not power on \r\n");
	}

	if (!(s_pm_on_modules & (0x1 << PM_POWER_MODULE_NAME_VIDP))) // when the module not power on , set the module sleep state
	{
		s_pm_sleeped_modules |= 0x1 << PM_POWER_MODULE_NAME_VIDP;
		s_pm_off_modules |= 0x1 << PM_POWER_MODULE_NAME_VIDP;
		// os_printf("video not power on \r\n");
	}

#if CONFIG_SYS_CPU0
	if (s_pm_mem_auto_power_down_flag)
	{
		off_modules = (0x1 << PM_POWER_MODULE_NAME_VIDP) | (0x1 << PM_POWER_MODULE_NAME_AUDP) | (0x1 << PM_POWER_MODULE_NAME_BTSP);
		if ((s_pm_off_modules & off_modules) == off_modules)
		{
			pm_module_check_power_off(PM_POWER_MODULE_NAME_MEM3);
		}
	}

	if (s_pm_cp1_auto_power_down_flag)
	{
		off_modules = (0x1 << PM_POWER_MODULE_NAME_VIDP) | (0x1 << PM_POWER_MODULE_NAME_AUDP);
		if ((s_pm_off_modules & off_modules) == off_modules)
		{
#if !CONFIG_SOC_BK7236XX && !CONFIG_SOC_BK7239XX && !CONFIG_SOC_BK7286XX  // temp mofify for bk7236
			if (sys_drv_module_power_state_get(PM_POWER_MODULE_NAME_VIDP) && sys_drv_module_power_state_get(PM_POWER_MODULE_NAME_AUDP))
#endif
			{
				// pm_module_check_power_off(PM_POWER_MODULE_NAME_CPU1);
			}
		}
	}
#endif
}

void pm_rf_power_ctrl(pm_power_module_name_e module, pm_power_module_state_e power_state)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	if (power_state == PM_POWER_MODULE_STATE_ON)
	{
		s_pm_rf_on_modules |= 0x1 << module;
	}
	else
	{
		s_pm_rf_off_modules |= 0x1 << module;
	}
	sys_drv_module_RF_power_ctrl(module, power_state);
	GLOBAL_INT_RESTORE();
}

void pm_rf_switch(pm_power_module_name_e name)
{
}

int32 bk_pm_module_power_state_get(pm_power_module_name_e module)
{
	if ( (module == PM_POWER_SUB_MODULE_NAME_BAKP_TIMER1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_UART1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_UART2)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_SPI1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_I2C0)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_I2C1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_SADC)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_PWM1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_PWM0)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_TRNG)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_IRDA)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_SDIO)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_COMSEG)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_DMA0)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_DMA1)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_LA)
		|| (module == PM_POWER_SUB_MODULE_NAME_BAKP_PM))
	{
		return sys_drv_module_power_state_get(PM_POWER_MODULE_NAME_BAKP);
	}
	else if ( (module == PM_POWER_SUB_MODULE_NAME_AHBP_CAN)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_QSPI)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_USB)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_PSRAM)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_QSPI1)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_ENET)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_SCR)
		|| (module == PM_POWER_SUB_MODULE_NAME_AHBP_LIN))
	{
		return sys_drv_module_power_state_get(PM_POWER_MODULE_NAME_AHBP);
	}
	else if ((module == PM_POWER_SUB_MODULE_NAME_AUDP_FFT)
		|| (module == PM_POWER_SUB_MODULE_NAME_AUDP_SBC)
		|| (module == PM_POWER_SUB_MODULE_NAME_AUDP_AUDIO)
		|| (module == PM_POWER_SUB_MODULE_NAME_AUDP_I2S))
	{
		return sys_drv_module_power_state_get(PM_POWER_MODULE_NAME_AUDP);
	}
	else if ((module == PM_POWER_SUB_MODULE_NAME_VIDP_DMA2D)
		|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_YUVBUF)
		|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_JPEG_EN)
		|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_JPEG_DE)
		|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_LCD)
		|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_ROTT)
		|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_SCAL0)
		|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_SCAL1)
		|| (module == PM_POWER_SUB_MODULE_NAME_VIDP_H264))
	{
		return sys_drv_module_power_state_get(PM_POWER_MODULE_NAME_VIDP);
	}
	else if ((module == PM_POWER_SUB_MODULE_NAME_PHY_BT)
		|| (module == PM_POWER_SUB_MODULE_NAME_PHY_WIFI))
	{
		return sys_drv_module_power_state_get(PM_POWER_MODULE_NAME_PHY);
	}
	else
	{
		if (module >= PM_POWER_MODULE_NAME_NONE)
		{
			os_printf("pm module[%d] not support ,get power state fail %d\r\n", module);
			return BK_ERR_NOT_SUPPORT;
		}
		else
		{
			return sys_drv_module_power_state_get(module);
		}
	}
}

#if CONFIG_SYS_CPU0
static void pm_module_check_power_on(pm_power_module_name_e module)
{
	if (PM_POWER_MODULE_STATE_OFF == sys_drv_module_power_state_get(module))
	{
		sys_drv_module_power_ctrl(module, PM_POWER_MODULE_STATE_ON);

		if (module == PM_POWER_MODULE_NAME_MEM3)
		{
#if CONFIG_SYSTEM_CTRL
#if CONFIG_SYS_CPU0
			extern void smem_reset_lastblock(void);
			smem_reset_lastblock();
#endif
#endif
		}

		if (module == PM_POWER_MODULE_NAME_CPU1)
		{
#if (CONFIG_SYS_CPU0) && (CONFIG_CPU_CNT > 1)
			extern void start_cpu1_core();
			start_cpu1_core();
			// s_pm_cp1_boot_ready = 0x0;
#endif
		}

		s_pm_off_modules &= ~(0x1 << module);
		s_pm_on_modules |= 0x1 << module;
	}
}

static void pm_module_check_power_off(pm_power_module_name_e module)
{
	if (PM_POWER_MODULE_STATE_ON == sys_drv_module_power_state_get(module))
	{
		if (module == PM_POWER_MODULE_NAME_CPU1)
		{
#if (CONFIG_SYS_CPU0) && (CONFIG_CPU_CNT > 1)
			extern void stop_cpu1_core(void);
			stop_cpu1_core();
#endif
		}
		if (s_debug_en & 0x2)
		{
			os_printf("pm_module_check_power_off module[%d][%d]\r\n", module, sys_drv_module_power_state_get(module));
		}
		sys_drv_module_power_ctrl(module, PM_POWER_MODULE_STATE_OFF);

		if (module == PM_POWER_MODULE_NAME_CPU1)
		{
			// s_pm_cp1_boot_ready = 0x0;
		}
		s_pm_off_modules |= 0x1 << module;
		s_pm_on_modules &= ~(0x1 << module);
	}
}

uint32_t bk_pm_cp1_auto_power_down_state_get()
{
	return s_pm_cp1_auto_power_down_flag;
}

bk_err_t bk_pm_cp1_auto_power_down_state_set(uint32_t value)
{
	s_pm_cp1_auto_power_down_flag = value;
	return BK_OK;
}

pm_mem_auto_ctrl_e bk_pm_mem_auto_power_down_state_get()
{
	return s_pm_mem_auto_power_down_flag;
}

bk_err_t bk_pm_mem_auto_power_down_state_set(pm_mem_auto_ctrl_e value)
{
	s_pm_mem_auto_power_down_flag = value;
	return BK_OK;
}
#endif
/*=========================MODULES POWER CTRL END========================*/

/*=========================WAKEUP SOURCE CONFIG START========================*/
static void pm_touched_wakeup_low_voltage()
{
	sys_drv_touch_wakeup_enable(s_touch_wakeup_param.touch_channel);
}

static void pm_gpio_wakeup_deep_sleep()
{
#if CONFIG_GPIO_WAKEUP_SUPPORT
	gpio_enter_low_power((void *)0);
#endif
}

static void pm_rtc_wakeup_deep_sleep()
{
	aon_pmu_drv_set_wakeup_source(WAKEUP_SOURCE_INT_RTC);
}

static void pm_touched_wakeup_deep_sleep()
{
}

#if CONFIG_SYS_CPU0
static void pm_deep_sleep_wakeup_source_set()
{
#if CONFIG_SYSTEM_CTRL
	uint32_t pmu_state = 0;
	if (aon_pmu_drv_reg_get(PMU_REG2) & BIT(BIT_SLEEP_FLAG_DEEP_SLEEP))
	{
		pmu_state = 0;
		pmu_state = aon_pmu_drv_reg_get(PMU_REG0x71);
		pmu_state = (pmu_state >> 20) & PM_WAKEUP_SOURCE_MARK;

		switch (pmu_state)
		{
		case 0x1: // gpio
			bk_misc_set_reset_reason(RESET_SOURCE_DEEPPS_GPIO);
			s_pm_exit_deepsleep_wakeup_source = PM_WAKEUP_SOURCE_INT_GPIO;
			break;
		case 0x2: // rtc
			bk_misc_set_reset_reason(RESET_SOURCE_DEEPPS_RTC);
			s_pm_exit_deepsleep_wakeup_source = PM_WAKEUP_SOURCE_INT_RTC;
			break;
		case 0x10: // bk7256 use touch and bk7236 use usb
			bk_misc_set_reset_reason(RESET_SOURCE_DEEPPS_TOUCH);
			s_pm_exit_deepsleep_wakeup_source = PM_WAKEUP_SOURCE_INT_TOUCHED;
			break;
		case 0x20: // touch
			bk_misc_set_reset_reason(RESET_SOURCE_DEEPPS_TOUCH);
			s_pm_exit_deepsleep_wakeup_source = PM_WAKEUP_SOURCE_INT_TOUCHED;
			break;
		default:
			s_pm_exit_deepsleep_wakeup_source = PM_WAKEUP_SOURCE_INT_NONE;
			break;
		}
#if 0
		/*clear the wakeup source*/
		pmu_state = 0;
		pmu_state = aon_pmu_drv_reg_get(PMU_REG0x43);
		pmu_state |= (0x1 << 17);
		aon_pmu_drv_reg_set(PMU_REG0x43, pmu_state);
#endif
	}
#endif
}

pm_wakeup_source_e bk_pm_deep_sleep_wakeup_source_get()
{
	return s_pm_exit_deepsleep_wakeup_source;
}

pm_wakeup_source_e bk_pm_exit_low_vol_wakeup_source_get()
{
	return s_pm_exit_low_vol_wakeup_source;
}

bk_err_t bk_pm_exit_low_vol_wakeup_source_set()
{
#if CONFIG_SYSTEM_CTRL
	uint32_t pmu_state = 0;
	if (aon_pmu_drv_reg_get(PMU_REG2) & BIT(BIT_SLEEP_FLAG_LOW_VOLTAGE))
	{
		pmu_state = 0;
		pmu_state = aon_pmu_drv_reg_get(PMU_REG0x71);
		pmu_state = (pmu_state >> 20) & PM_WAKEUP_SOURCE_MARK;

		switch (pmu_state)
		{
		case 0x1: // gpio
			s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_GPIO;
			break;
		case 0x2: // rtc
			s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_RTC;
			break;
		case 0x4: // WIFI wakeup
			s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_WIFI;
			break;
		case 0x8: // bk7256 use usb and bk7236 use BT wakeup
			s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_BT;
			break;
		case 0x10: // bk7256 use touch and bk7236 use usb wakeup
			s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_TOUCHED;
			break;
		case 0x20: // touch
			s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_TOUCHED;
			break;
		default:
			s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_NONE;
			break;
		}
	}
#endif
	return BK_OK;
}

bk_err_t bk_pm_exit_low_vol_wakeup_source_clear()
{
	/*clear the wakeup source*/
#if CONFIG_SYSTEM_CTRL
	uint32_t pmu_state = 0;
	pmu_state = aon_pmu_drv_reg_get(PMU_REG0x43);
	pmu_state |= (0x1 << 17);
	aon_pmu_drv_reg_set(PMU_REG0x43, pmu_state);
#endif
	s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_NONE;
	return BK_OK;
}
#endif
/*=========================WAKEUP SOURCE CONFIG END========================*/

/*=========================SLEEP/WAKEUP FUNCTION START========================*/
static void pm_enter_normal_sleep()
{
	sys_drv_enter_normal_sleep(0);
}

static void pm_enter_low_voltage()
{
	sys_drv_enter_low_voltage();
}

static void pm_enter_deep_sleep()
{
	sys_drv_enter_deep_sleep(NULL);
}

#if CONFIG_PM_SUPER_DEEP_SLEEP
static void pm_enter_super_deep_sleep()
{
	uint8_t use_super_deep = 1;
	bk_misc_set_reset_reason(RESET_SOURCE_SUPER_DEEP);
	sys_drv_enter_deep_sleep(&use_super_deep);
}
#endif

static void pm_low_voltage_resource_set()
{
	pm_dev_id_e dev_id = 0;
	#if CONFIG_SYS_CPU0
	pm_psram_malloc_state_and_power_ctrl();
	#endif

	bk_pm_exit_low_vol_wakeup_source_clear();

	for (dev_id = 0; dev_id < PM_DEV_ID_MAX; dev_id++)
	{
		if (s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_ENTER_LOWVOL_INDEX][dev_id].cb != NULL)
		{
			s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_ENTER_LOWVOL_INDEX][dev_id].cb(0, s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_ENTER_LOWVOL_INDEX][dev_id].args);
		}
	}

#if CONFIG_GPIO_WAKEUP_SUPPORT
	gpio_enter_low_power((void *)0);
#endif

	if (s_pm_wakeup_source & (0x1 << PM_WAKEUP_SOURCE_INT_TOUCHED))
	{
		pm_touched_wakeup_low_voltage();
	}

#if CONFIG_BAKP_POWER_DOMAIN_PM_CONTROL
	bk_pm_module_vote_power_ctrl(POWER_SUB_MODULE_NAME_BAKP_PM, PM_POWER_MODULE_STATE_OFF);
#endif

	/*flash line mode 4->2 when enter low voltage*/
#if CONFIG_FLASH_ORIGIN_API
	flash_set_line_mode(2);
#else
	bk_flash_set_line_mode(2);
#endif

}

void pm_low_voltage_bsp_restore(void)
{
	/*flash line mode 2->4 when exit low voltage*/
#if CONFIG_FLASH_ORIGIN_API
	flash_set_line_mode(flash_get_line_mode());
#else
	bk_flash_set_line_mode(bk_flash_get_line_mode());
#endif

#if CONFIG_CKMN
	bk_rosc_32k_ckest_prog(32);
#endif

#if CONFIG_INT_WDT
	wdt_init();
#endif

	bk_pm_exit_low_vol_wakeup_source_set();

}

static void pm_low_voltage_resource_restore()
{
	pm_dev_id_e dev_id = 0;

#if CONFIG_BAKP_POWER_DOMAIN_PM_CONTROL
	bk_pm_module_vote_power_ctrl(POWER_SUB_MODULE_NAME_BAKP_PM, PM_POWER_MODULE_STATE_ON);
#endif

#if CONFIG_GPIO_WAKEUP_SUPPORT
	gpio_exit_low_power((void *)0);
#endif

	for (dev_id = 0; dev_id < PM_DEV_ID_MAX; dev_id++)
	{
		if (s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_EXIT_LOWVOL_INDEX][dev_id].cb != NULL)
		{
			if (dev_id == PM_DEV_ID_MAC)
			{
				if (bk_pm_exit_low_vol_wakeup_source_get() == PM_WAKEUP_SOURCE_INT_GPIO)
				{
					s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_EXIT_LOWVOL_INDEX][dev_id].cb(0, s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_EXIT_LOWVOL_INDEX][dev_id].args);
					if (s_debug_en & 0x2)
					{
						os_printf("wifi exit low vol arg:%d\r\n", s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_EXIT_LOWVOL_INDEX][dev_id].args);
					}
				}
				else
				{
					continue;
				}
			}
			else
			{
				s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_EXIT_LOWVOL_INDEX][dev_id].cb(0, s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_EXIT_LOWVOL_INDEX][dev_id].args);
			}
		}
	}
}

static uint32_t pm_low_voltage_process()
{
	volatile uint32_t int_level = 0;
	uint32_t sleep_tick = 0;
#if CONFIG_AON_RTC
	uint32_t entry_tick = 0;
	uint32_t exit_tick = 0;
#endif
	int_level = rtos_enter_critical();
#if CONFIG_AON_RTC
	entry_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
#endif
	pm_low_voltage_resource_set();

	pm_enter_low_voltage();

	pm_low_voltage_resource_restore();
	if (s_debug_en & 0x2)
		os_printf("low voltage int open before\r\n");

#if CONFIG_AON_RTC
	exit_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	sleep_tick = exit_tick - entry_tick;
#endif
	rtos_exit_critical(int_level);

	if (s_debug_en & 0x2)
		os_printf("low voltage int open after 0x%x 0x%x \r\n", /*int_mie,*/ int_level, bk_pm_lp_vol_get());

	return sleep_tick;
}

static void pm_deep_sleep_process()
{
	// if(s_pm_wakeup_source&(0x1 << PM_WAKEUP_SOURCE_INT_GPIO))
	{
		pm_gpio_wakeup_deep_sleep();
	}

	if (s_pm_wakeup_source & (0x1 << PM_WAKEUP_SOURCE_INT_RTC))
	{
		pm_rtc_wakeup_deep_sleep();
	}

	if (s_pm_wakeup_source & (0x1 << PM_WAKEUP_SOURCE_INT_TOUCHED))
	{
		pm_touched_wakeup_deep_sleep();
	}

	pm_enter_deep_sleep();
}

#if CONFIG_PM_SUPER_DEEP_SLEEP
static void pm_super_deep_sleep_process()
{
	uint64_t skip_io = 0;

#if CONFIG_GPIO_RETENTION_SUPPORT
	gpio_retention_sync(false);
	skip_io = gpio_retention_map_get();
#endif

#if CONFIG_GPIO_WAKEUP_SUPPORT
	gpio_hal_switch_to_low_power_status(BIT64(16) | skip_io);
#endif

	pm_enter_super_deep_sleep();
}
#endif

static bk_err_t pm_wakeup_from_deepsleep_handle()
{
#if CONFIG_SYSTEM_CTRL
	uint32_t pmu_state = 0;
	if (aon_pmu_drv_reg_get(PMU_REG2) & BIT(BIT_SLEEP_FLAG_DEEP_SLEEP))
	{
		pmu_state = 0;
		pmu_state = aon_pmu_drv_reg_get(PMU_REG2);
		pmu_state &= ~(BIT(BIT_SLEEP_FLAG_DEEP_SLEEP));
		aon_pmu_drv_reg_set(PMU_REG2, pmu_state);
	}
#endif
	return BK_OK;
}
/*=========================SLEEP/WAKEUP FUNCTION END========================*/

/*=========================SLEEP CB REGISTER API START========================*/
static void pm_sleep_cb_push_item(pm_sleep_cb_t cb_arr[], uint8_t *cb_cnt_p, pm_sleep_cb_t cb_item)
{
	if (*cb_cnt_p == PM_DEEPSLEEP_CB_SIZE)
	{
		if (s_debug_en & 0x2)
			os_printf("call back function overflow, dev %d regist fail!\r\n", cb_item.id);
		if (s_debug_en & 0x1)
		{
			os_printf("cb functions dump: [ ");
			for (uint8_t i = 0; i < PM_DEEPSLEEP_CB_SIZE; i++)
				os_printf("%d ", cb_arr[i].id);
			os_printf("]\r\n");
		}
		return;
	} else if (cb_arr[*cb_cnt_p].cfg.cb != NULL) {
		if (s_debug_en & 0x2)
			os_printf("cb functions have overlap warning, dev %d -> %d\r\n", cb_item.id, cb_arr[*cb_cnt_p].id);
	}

	for (uint8_t i = 0; i < *cb_cnt_p; i++)
	{
		if (cb_arr[i].id == cb_item.id)
		{
			cb_arr[i].cfg.cb   = cb_item.cfg.cb;
			cb_arr[i].cfg.args = cb_item.cfg.args;
			return;
		}
	}

	cb_arr[*cb_cnt_p].id       = cb_item.id;
	cb_arr[*cb_cnt_p].cfg.cb   = cb_item.cfg.cb;
	cb_arr[*cb_cnt_p].cfg.args = cb_item.cfg.args;

	(*cb_cnt_p)++;
}

static void pm_sleep_cb_pop_item(pm_sleep_cb_t cb_arr[], uint8_t *cb_cnt_p, uint8_t item_ind)
{
	uint8_t tail_ind = (*cb_cnt_p) - 1;

	cb_arr[item_ind].id        = cb_arr[tail_ind].id;
	cb_arr[item_ind].cfg.cb    = cb_arr[tail_ind].cfg.cb;
	cb_arr[item_ind].cfg.args  = cb_arr[tail_ind].cfg.args;

	cb_arr[tail_ind].id       = PM_DEV_ID_MAX;
	cb_arr[tail_ind].cfg.cb   = NULL;
	cb_arr[tail_ind].cfg.args = NULL;

	(*cb_cnt_p)--;
}

bk_err_t bk_pm_sleep_register_cb(pm_sleep_mode_e sleep_mode, pm_dev_id_e dev_id, pm_cb_conf_t *enter_config, pm_cb_conf_t *exit_config)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	if (sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		if (enter_config != NULL)
		{
			if ((enter_config->cb != NULL) && (dev_id < PM_DEV_ID_MAX))
			{
				s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_ENTER_LOWVOL_INDEX][dev_id].cb = enter_config->cb;
				s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_ENTER_LOWVOL_INDEX][dev_id].args = enter_config->args;
			}
		}

		if (exit_config != NULL)
		{
			if ((exit_config->cb != NULL) && (dev_id < PM_DEV_ID_MAX))
			{
				s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_EXIT_LOWVOL_INDEX][dev_id].cb = exit_config->cb;
				s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_EXIT_LOWVOL_INDEX][dev_id].args = exit_config->args;
			}
		}
	}
	else if (sleep_mode == PM_MODE_DEEP_SLEEP)
	{
		if (enter_config != NULL)
		{
			pm_sleep_cb_t cb_item = {dev_id, *enter_config};
			// use exit_config to indicate the execution priority when entering sleep
			if ((enter_config->cb != NULL) && (exit_config != NULL) && ((pm_cb_priority_e)exit_config->args < PM_CB_PRIORITY_MAX))
			{
				pm_sleep_cb_push_item(s_pm_deepsleep_enter_cb_conf, &s_pm_deepsleep_enter_cb_cnt[(pm_cb_priority_e)exit_config->args], cb_item);
			}
			else if ((enter_config->cb != NULL) && (dev_id < PM_DEV_ID_DEFAULT))
			{
				pm_sleep_cb_push_item(s_pm_deepsleep_enter_cb_conf, &s_pm_deepsleep_enter_cb_cnt[PM_CB_PRIORITY_0], cb_item);
			}
			else if ((enter_config->cb != NULL) && (dev_id < PM_DEV_ID_MAX))
			{
				pm_sleep_cb_push_item(s_pm_deepsleep_enter_cb_conf, &s_pm_deepsleep_enter_cb_cnt[PM_CB_PRIORITY_1], cb_item);
			}
		}
	}
#if CONFIG_PM_SUPER_DEEP_SLEEP
	else if (sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
	{
		if (enter_config != NULL)
		{
			pm_sleep_cb_t cb_item = {dev_id, *enter_config};
			if ((enter_config->cb != NULL) && (dev_id < PM_DEV_ID_DEFAULT))
			{
				pm_sleep_cb_push_item(s_pm_superdeep_enter_cb_conf, &s_pm_superdeep_enter_cb_cnt[PM_CB_PRIORITY_0], cb_item);
			}
			else if ((enter_config->cb != NULL) && (dev_id < PM_DEV_ID_MAX))
			{
				pm_sleep_cb_push_item(s_pm_superdeep_enter_cb_conf, &s_pm_superdeep_enter_cb_cnt[PM_CB_PRIORITY_1], cb_item);
			}
		}
	}
#endif
	else
	{
		os_printf("The sleep mode[%d] not support register call back \r\n", sleep_mode);
	}
	GLOBAL_INT_RESTORE();
	return BK_OK;
}

bk_err_t bk_pm_sleep_unregister_cb(pm_sleep_mode_e sleep_mode, pm_dev_id_e dev_id, bool enter_cb, bool exit_cb)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	if (sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		if ((enter_cb == true) && (dev_id < PM_DEV_ID_MAX))
		{
			s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_ENTER_LOWVOL_INDEX][dev_id].cb = NULL;
			s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_ENTER_LOWVOL_INDEX][dev_id].args = NULL;
		}

		if (exit_cb == true)
		{
			s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_EXIT_LOWVOL_INDEX][dev_id].cb = NULL;
			s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_EXIT_LOWVOL_INDEX][dev_id].args = NULL;
		}
	}
	else if (sleep_mode == PM_MODE_DEEP_SLEEP)
	{
		if ((enter_cb == true) && (dev_id < PM_DEV_ID_DEFAULT))
		{
			for (uint8_t i = PM_SLEEP_CB_IND_PRI_0; i < PM_SLEEP_CB_IND_PRI_1; i++)
			{
				if (s_pm_deepsleep_enter_cb_conf[i].id == dev_id)
				{
					pm_sleep_cb_pop_item(s_pm_deepsleep_enter_cb_conf, &s_pm_deepsleep_enter_cb_cnt[PM_CB_PRIORITY_0], i);
					break;
				}
			}
		}
		else if ((enter_cb == true) && (dev_id < PM_DEV_ID_MAX))
		{
			for (uint8_t i = PM_SLEEP_CB_IND_PRI_1; i < PM_DEEPSLEEP_CB_SIZE; i++)
			{
				if (s_pm_deepsleep_enter_cb_conf[i].id == dev_id)
				{
					pm_sleep_cb_pop_item(s_pm_deepsleep_enter_cb_conf, &s_pm_deepsleep_enter_cb_cnt[PM_CB_PRIORITY_1], i);
					break;
				}
			}
		}
	}
#if CONFIG_PM_SUPER_DEEP_SLEEP
	else if (sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
	{
		if ((enter_cb == true) && (dev_id < PM_DEV_ID_DEFAULT))
		{
			for (uint8_t i = PM_SLEEP_CB_IND_PRI_0; i < PM_SLEEP_CB_IND_PRI_1; i++)
			{
				if (s_pm_superdeep_enter_cb_conf[i].id == dev_id)
				{
					pm_sleep_cb_pop_item(s_pm_superdeep_enter_cb_conf, &s_pm_superdeep_enter_cb_cnt[PM_CB_PRIORITY_0], i);
					break;
				}
			}
		}
		else if ((enter_cb == true) && (dev_id < PM_DEV_ID_MAX))
		{
			for (uint8_t i = PM_SLEEP_CB_IND_PRI_1; i < PM_SUPERDEEP_CB_SIZE; i++)
			{
				if (s_pm_superdeep_enter_cb_conf[i].id == dev_id)
				{
					pm_sleep_cb_pop_item(s_pm_superdeep_enter_cb_conf, &s_pm_superdeep_enter_cb_cnt[PM_CB_PRIORITY_1], i);
					break;
				}
			}
		}
	}
#endif
	else
	{
		os_printf("The sleep mode[%d] not support unregister call back \r\n", sleep_mode);
	}
	GLOBAL_INT_RESTORE();

	return BK_OK;
}

bk_err_t bk_pm_light_sleep_register_cb(pm_cb_conf_t *enter_config, pm_cb_conf_t *exit_config)
{
#if !defined(CONFIG_PM_LIGHT_SLEEP)
	return BK_ERR_NOT_SUPPORT;
#else
	if (enter_config != NULL)
	{
		s_pm_light_sleep_enter_cb_conf.cb = enter_config->cb;
		s_pm_light_sleep_enter_cb_conf.args = enter_config->args;
	}
	if (exit_config != NULL)
	{
		s_pm_light_sleep_exit_cb_conf.cb = exit_config->cb;
		s_pm_light_sleep_exit_cb_conf.args = exit_config->args;
	}
	return BK_OK;
#endif
}

bk_err_t bk_pm_light_sleep_unregister_cb(bool enter_cb, bool exit_cb)
{
#if !defined(CONFIG_PM_LIGHT_SLEEP)
	return BK_ERR_NOT_SUPPORT;
#else
	if (enter_cb)
	{
		s_pm_light_sleep_enter_cb_conf.cb = NULL;
	}

	if (exit_cb)
	{
		s_pm_light_sleep_exit_cb_conf.cb = NULL;
	}

	return BK_OK;
#endif
}
/*=========================SLEEP CB REGISTER API END========================*/

/*=========================POWER/VOLTAGE CTRL START========================*/
bk_err_t bk_pm_external_ldo_ctrl(uint32_t value)
{
	uint32_t i = 0;
	uint32_t gpio_ctrl_ldo_output_high_map[] = GPIO_CTRL_LDO_OUTPUT_HIGH_MAP;
	uint32_t gpio_ctrl_ldo_output_low_map[] = GPIO_CTRL_LDO_OUTPUT_LOW_MAP;

	if (value == 0x1) // output higt
	{
		for (i = 0; i < sizeof(gpio_ctrl_ldo_output_high_map) / sizeof(uint32_t); i++)
		{
			bk_gpio_enable_output(gpio_ctrl_ldo_output_high_map[i]);
			bk_gpio_set_output_high(gpio_ctrl_ldo_output_high_map[i]);
		}
	}
	else if (value == 0x0) // output low
	{
		for (i = 0; i < sizeof(gpio_ctrl_ldo_output_low_map) / sizeof(uint32_t); i++)
		{
			bk_gpio_enable_output(gpio_ctrl_ldo_output_high_map[i]);
			bk_gpio_set_output_low(gpio_ctrl_ldo_output_low_map[i]);
		}
	}
	else
	{
	}

	return BK_OK;
}

uint32_t bk_pm_lp_vol_get()
{
	return sys_drv_lp_vol_get();
}

int bk_pm_lp_vol_set(uint32_t value)
{
	sys_drv_lp_vol_set(value);
	return BK_OK;
}

uint32_t bk_pm_rf_tx_vol_get()
{
	return sys_drv_rf_tx_vol_get();
}

int bk_pm_rf_tx_vol_set(uint32_t value)
{
	sys_drv_rf_tx_vol_set(value);
	return BK_OK;
}

uint32_t bk_pm_rf_rx_vol_get()
{
	return sys_drv_rf_rx_vol_get();
}

int bk_pm_rf_rx_vol_set(uint32_t value)
{
	sys_drv_rf_rx_vol_set(value);
	return BK_OK;
}
/*=========================POWER/VOLTAGE CTRL END========================*/

/*=========================CLK/FREQ CTRL START========================*/
bk_err_t pm_core_bus_clock_ctrl(uint32_t cksel_core, uint32_t ckdiv_core, uint32_t ckdiv_bus, uint32_t ckdiv_cpu0, uint32_t ckdiv_cpu1)
{
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	sys_drv_core_bus_clock_ctrl(cksel_core, ckdiv_core, ckdiv_bus, ckdiv_cpu0, ckdiv_cpu1);
	GLOBAL_INT_RESTORE();

	return BK_OK;
}

pm_cpu_freq_e bk_pm_current_max_cpu_freq_get()
{
	return s_pm_current_cpu_freq;
}

pm_cpu_freq_e bk_pm_module_current_cpu_freq_get(pm_dev_id_e module)
{
	return s_pm_cpu_freq[module];
}

bk_err_t bk_pm_module_vote_cpu_freq(pm_dev_id_e module, pm_cpu_freq_e cpu_freq)
{
#if CONFIG_SYS_CPU1
#if CONFIG_MAILBOX
	uint64_t previous_tick  = 0;
	uint64_t current_tick   = 0;

	mb_chnl_cmd_t mb_cmd    = {0};
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	bk_pm_cp1_cpu_freq_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);
	mb_cmd.hdr.cmd = PM_CPU_FREQ_CTRL_CMD;
	mb_cmd.param1 = module;
	mb_cmd.param2 = cpu_freq;
	mb_cmd.param3 = 0;
	mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	GLOBAL_INT_RESTORE();

	previous_tick = pm_cp1_aon_rtc_counter_get();
	current_tick = previous_tick;
	os_printf("cp1 vote freq_B[%lld]\r\n",previous_tick);
	while((current_tick - previous_tick) < (PM_SEND_CMD_CP0_RESPONSE_TIME_OUT*PM_AON_RTC_DEFAULT_TICK_COUNT))
	{
	    if (bk_pm_cp1_cpu_freq_ctrl_state_get()) // wait the cp0 response
	    {
			break;
	    }
	    current_tick = pm_cp1_aon_rtc_counter_get();
	}

	if(!bk_pm_cp1_cpu_freq_ctrl_state_get())
	{
	    os_printf("cp1 vote freq[%d]time out\r\n",module);
	}
	os_printf("cp1 vote freq_E[%lld]\r\n",current_tick);

	if(s_debug_en&0x2)
		os_printf("cpu1 vote cpu freq\r\n");
#endif
	return BK_OK;
#else
	bk_err_t ret = BK_OK;
	uint32_t i = 0;
	uint32_t freq_max = 0;
	uint32_t freq_max_index = 0;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	/*save the cpu freq first*/
	if (PM_CPU_FRQ_DEFAULT == cpu_freq)
	{
		cpu_freq = PM_CPU_FRQ_26M; // it will use the PM_DEV_ID_DEFAULT vote cpu frequency
	}
	s_pm_cpu_freq[module] = (uint8_t)cpu_freq;

	/*get the max cpu freq*/
	freq_max = s_pm_cpu_freq[0];
	for (i = 1; i < PM_DEV_ID_MAX; i++)
	{
		if (freq_max < s_pm_cpu_freq[i])
		{
			freq_max = s_pm_cpu_freq[i];
			freq_max_index = i;
		}
	}

	/*updete the current cpu frequency*/
	if (s_pm_current_cpu_freq == freq_max)
	{
		GLOBAL_INT_RESTORE();
		return BK_OK;
	}
	else
	{
		ret = sys_drv_switch_cpu_bus_freq(freq_max);
	}
	if (ret == BK_OK)
		s_pm_current_cpu_freq = freq_max;

	GLOBAL_INT_RESTORE();

	if (ret != BK_OK)
	{
		os_printf("cpu0 switch cpu freq error\r\n");
		return ret;
	}
	if (s_debug_en & 0x2)
		os_printf("cpu0 switch cpu freq %d %d\r\n", freq_max, freq_max_index);
	return BK_OK;
#endif
}

bk_err_t bk_pm_clock_ctrl(pm_dev_clk_e module, pm_dev_clk_pwr_e clock_state)
{
#if CONFIG_SYS_CPU1
#if CONFIG_MAILBOX
	uint64_t previous_tick  = 0;
	uint64_t current_tick   = 0;
	mb_chnl_cmd_t mb_cmd    = {0};

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	bk_pm_cp1_clk_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);
	mb_cmd.hdr.cmd = PM_CLK_CTRL_CMD;
	mb_cmd.param1 = module;
	mb_cmd.param2 = clock_state;
	mb_cmd.param3 = 0;
	mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	GLOBAL_INT_RESTORE();

	previous_tick = pm_cp1_aon_rtc_counter_get();
	current_tick = previous_tick;
	while((current_tick - previous_tick) < (PM_SEND_CMD_CP0_RESPONSE_TIME_OUT*PM_AON_RTC_DEFAULT_TICK_COUNT))
	{
	    if (bk_pm_cp1_clk_ctrl_state_get()) // wait the cp0 response
	    {
			break;
	    }
	    current_tick = pm_cp1_aon_rtc_counter_get();
	}

	if(!bk_pm_cp1_clk_ctrl_state_get())
	{
	    os_printf("cp1 vote freq[%d] time out\r\n",module);
	}

#endif
	return BK_OK;
#else
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	sys_drv_dev_clk_pwr_up(module, clock_state);
	GLOBAL_INT_RESTORE();
	return BK_OK;
#endif
}

pm_lpo_src_e bk_pm_lpo_src_get()
{
	return (pm_lpo_src_e)aon_pmu_drv_lpo_src_get();
}

bk_err_t bk_pm_lpo_src_set(pm_lpo_src_e lpo_src)
{
	aon_pmu_drv_lpo_src_set((uint32_t)lpo_src);
	return BK_OK;
}

bk_err_t bk_pm_rosc_calibration(pm_rosc_cali_mode_e rosc_cali_mode, uint32_t cali_interval)
{
	bk_err_t ret = BK_OK;
	ret = sys_drv_rosc_calibration(rosc_cali_mode, cali_interval);
	if (ret != BK_OK)
	{
		os_printf("set rosc calibration parameter error %d %d \r\n", rosc_cali_mode, cali_interval);
		return ret;
	}
	return BK_OK;
}

bk_err_t pm_clk_32k_source_notify_cb(void)
{
	bk_err_t ret;

	ret = rtos_set_semaphore(&s_sync_sema);

	return ret;
}

bk_err_t pm_extern32k_register_cb(pm_cb_extern32k_cfg_t *cfg)
{
	if (cfg == NULL)
		return BK_FAIL;

	s_pm_cb_module_cfg.cfg[cfg->cb_module].cb_func = cfg->cb_func;

	return BK_OK;
}

bk_err_t pm_extern32k_unregister_cb(pm_cb_extern32k_cfg_t *cfg)
{
	if (cfg == NULL)
		return BK_FAIL;

	s_pm_cb_module_cfg.cfg[cfg->cb_module].cb_func = NULL;

	return BK_OK;
}

bk_err_t pm_extern32k_cb_exec(pm_sw_step_e sw_step, pm_lpo_src_e lpo_src)
{
	int i;
	s_pm_cb_module_cfg.module_count = 0;

	for (i = 0; i < PM_32K_MODULE_MAX; i++)
	{
		if (s_pm_cb_module_cfg.cfg[i].cb_func)
		{
			/*call cb and register notify callback*/
			s_pm_cb_module_cfg.module_count++;
			s_pm_cb_module_cfg.cfg[i].cb_func(sw_step, lpo_src, pm_clk_32k_source_notify_cb);
		}
	}

	return BK_OK;
}

bk_err_t pm_clk_32k_source_switch(pm_lpo_src_e lpo_src)
{
#if CONFIG_EXTERN_32K_PIN_MULT
	pm_lpo_src_e clk_src;
	bk_err_t ret;
	uint32_t count;

	clk_src = bk_clk_32k_customer_config_get();

	if (s_debug_en & 0x2)
	{
		os_printf("customer set clk %d\r\n", clk_src);
	}

	if (clk_src != PM_LPO_SRC_X32K)
	{
		return BK_OK;
	}

	clk_src = bk_pm_lpo_src_get();
	if (s_debug_en & 0x2)
	{
		os_printf("current clk %d,set clk %d\r\n", clk_src, lpo_src);
	}

	if (clk_src == lpo_src)
	{
		return BK_OK;
	}

	if (NULL == s_sync_sema)
	{
		rtos_init_semaphore(&s_sync_sema, PM_32K_MODULE_MAX);
	}

	/*when close DVP,wait for the external clock to stabilize*/
	if (lpo_src == PM_LPO_SRC_X32K)
	{
		aon_pmu_drv_lpo_src_extern32k_enable();
		rtos_delay_milliseconds(SWITCH_32K_DELAY);
	}

	/*Before clock switch,execute BTWF callback and wait BTWF notify*/
	pm_extern32k_cb_exec(PM_32K_STEP_BEGIN, lpo_src);

	count = s_pm_cb_module_cfg.module_count;
	while (count)
	{
		/*Wait for the BTWF clock switch to complete*/
		ret = rtos_get_semaphore(&s_sync_sema, SWITCH_32K_WAIT);
		if (ret < 0)
		{
			os_printf(" ERR: wait sema timeout\r\n");
			rtos_deinit_semaphore(&s_sync_sema);
			return BK_FAIL;
		}
		count--;
	}

	/*switch 32k source*/
	bk_pm_lpo_src_set(lpo_src);

	/*after clock switch,execute BTWF callback and wait BTWF notify*/
	pm_extern32k_cb_exec(PM_32K_STEP_FINISH, lpo_src);

	rtos_deinit_semaphore(&s_sync_sema);
#endif
	return BK_OK;
}
/*=========================CLK/FREQ CTRL END========================*/

/*=========================DEBUG/TEST CTRL START========================*/
#if PM_DEBUG
#define AON_GPIO_BASE (0x44000400)
void TOGGLE_GPIO18()
{
	uint32_t param = 0;

	param = 0x2;
	REG_WRITE(AON_GPIO_BASE + 0x12 * 4, param);
	param = 0x0;
	REG_WRITE(AON_GPIO_BASE + 0x12 * 4, param);
}
#endif

uint32_t pm_debug_mode()
{
	return s_debug_en;
}
bk_err_t pm_debug_module_state()
{
#if CONFIG_SYS_CPU0
	if(s_pm_video_pm_state > 0)
	{
		os_printf("Video not PD[modulue:0x%x]\r\n",s_pm_video_pm_state);
	}
	if(s_pm_audio_pm_state > 0)
	{
		os_printf("Audio not PD[modulue:0x%x]\r\n",s_pm_audio_pm_state);
	}

	if(!bk_pm_module_power_state_get(PM_POWER_MODULE_NAME_CPU1))
	{
		os_printf("Cp1 not PD[state:0x%x]\r\n",bk_pm_module_power_state_get(PM_POWER_MODULE_NAME_CPU1));
	}

	if(!(REG_READ(PM_DEBUG_SYS_REG_BASE+0x6*4)&0x2))
	{
		os_printf("Cp2 not PD[state:0x%x]\r\n",REG_READ(PM_DEBUG_SYS_REG_BASE+0x6*4));
	}

	#if CONFIG_PSRAM_AS_SYS_MEMORY
	uint32_t cp0_psram_malloc_count = 0;
	/*get the cp1 psram malloc count*/
	#if (CONFIG_CPU_CNT > 1)
	uint32_t cp1_psram_malloc_count = s_pm_cp1_psram_malloc_count_state;
	if(cp1_psram_malloc_count > 0)
	{
		os_printf("CP1 psram malloc count[%d] > 0\r\n",cp1_psram_malloc_count);
		os_printf("Power consumption will get higher, please free them\r\n");
		bk_pm_dump_cp1_psram_malloc_info();
	}
	#endif
	/*get the cp0 psram malloc count*/
	cp0_psram_malloc_count = bk_psram_heap_get_used_count();
	if(cp0_psram_malloc_count > 0)
	{
		os_printf("CP0 psram malloc count[%d] > 0\r\n",cp0_psram_malloc_count);
		os_printf("power consumption will get higher,free them\r\n");
		bk_psram_heap_get_used_state();
	}
	#endif
#endif
	return BK_OK;
}
void pm_debug_ctrl(uint32_t debug_en)
{
	s_debug_en = debug_en;
#if CONFIG_SYS_CPU0
	if(debug_en == PM_DEBUG_CTRL_STATE)
	{
		os_printf("pm video&&audio:0x%x 0x%x \r\n",s_pm_video_pm_state,s_pm_audio_pm_state);
		os_printf("pm ahpb&&bakp:0x%x 0x%x\r\n",s_pm_ahpb_pm_state,s_pm_bakp_pm_state);
		os_printf("pm low vol[module:0x%llx] [need module:0x%llx]\r\n",s_pm_sleeped_modules,s_pm_enter_low_vol_modules);
		os_printf("pm deepsleep[module:0x%x][need module:0x%x]\r\n",s_pm_off_modules,s_pm_enter_deep_sleep_modules);
		os_printf("pm power&&pmu[0x%x][0x%x]\r\n",REG_READ(PM_DEBUG_SYS_REG_BASE+0x10*4),REG_READ(PM_DEBUG_PMU_REG_BASE+0x41*4));

		if(s_pm_ahpb_pm_state > 0)
		{
			os_printf("Ahbp not PD[module:0x%x]\r\n",s_pm_ahpb_pm_state);
		}
		if(s_pm_bakp_pm_state > 0)
		{
			os_printf("Bakp not PD[module:0x%x]\r\n",s_pm_bakp_pm_state);
		}
		pm_cp1_psram_malloc_state_get();
		pm_debug_module_state();
	}
#endif
}

/*=========================DEBUG/TEST CTRL END========================*/
