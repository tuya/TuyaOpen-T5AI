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
#include <sdkconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <common/bk_include.h>
#include "bk_private/components_init.h"
#include "rtos_init.h"
#include <os/os.h>
#include "sys_driver.h"
#include "gpio_driver.h"
#include <driver/wdt.h>
#include <bk_wdt.h>

#if (CONFIG_SYS_CPU0)
#include <modules/pm.h>
#include <modules/ota.h>
#endif
#include <driver/flash_partition.h>

#include "boot.h"

#if CONFIG_FREERTOS_TRACE
#include "trcRecorder.h"
#endif

#include "stack_base.h"
#if CONFIG_GCOV
#include "gcov_public.h"
#endif

#include "soc/soc.h"

static beken_thread_function_t s_user_app_entry = NULL;
beken_semaphore_t user_app_sema = NULL;

void rtos_set_user_app_entry(beken_thread_function_t entry)
{
	s_user_app_entry = entry;
}

void rtos_user_app_preinit(void)
{
    int ret = rtos_init_semaphore(&user_app_sema, 1);
	if(ret < 0){
		os_printf("init queue failed");
	}
}

void rtos_user_app_launch_over(void)
{
	int ret;

	ret = rtos_set_semaphore(&user_app_sema);
	if(ret < 0){
		os_printf("set sema failed");
	}
}

void rtos_user_app_waiting_for_launch(void)
{
	int ret;

	ret = rtos_get_semaphore(&user_app_sema, BEKEN_WAIT_FOREVER);
	if(ret < 0){
		os_printf("get sema failed");
	}

#if CONFIG_SAVE_BOOT_TIME_POINT
	save_mtime_point(CPU_APP_ENTRY_TIME);
#endif
}

#if (CONFIG_CPU_CNT > 1)
#define MAX_STOP_CPU1_NOTIFICATION_CNT (4)
static stop_cpu1_notification s_stop_cpu1_notifications_array[MAX_STOP_CPU1_NOTIFICATION_CNT];
static void *s_stop_cpu1_notifications_param_array[MAX_STOP_CPU1_NOTIFICATION_CNT];

void stop_cpu1_register_notification(stop_cpu1_notification notification, void *param)
{
	int i;

	for(i = 0; i < MAX_STOP_CPU1_NOTIFICATION_CNT; i++)
	{
		if ((s_stop_cpu1_notifications_array[i] == NULL))
		{
			s_stop_cpu1_notifications_array[i] = notification;
			s_stop_cpu1_notifications_param_array[i] = param;
			break;
		}
	}

	if(i >= MAX_STOP_CPU1_NOTIFICATION_CNT)
		os_printf("Err:%s:%p", __func__, notification);
}

void stop_cpu1_unregister_notification(stop_cpu1_notification notification)
{
	int i;
	for(i = 0; i < MAX_STOP_CPU1_NOTIFICATION_CNT; i++)
	{
		if ((s_stop_cpu1_notifications_array[i] == notification))
		{
			s_stop_cpu1_notifications_array[i] = NULL;
			s_stop_cpu1_notifications_param_array[i] = NULL;
			break;
		}
	}

	if(i >= MAX_STOP_CPU1_NOTIFICATION_CNT)
		os_printf("Err:%s:%p", __func__, notification);
}

void stop_cpu1_handle_notifications()		//CPU1 handle stop notications
{
	for(int i = 0; i < MAX_STOP_CPU1_NOTIFICATION_CNT; i++)
	{
		if (s_stop_cpu1_notifications_array[i])
		{
			s_stop_cpu1_notifications_array[i](s_stop_cpu1_notifications_param_array[i]);
		}
	}
}

static uint32 get_partition_addr(uint32 cpu_id)
{
	(void)cpu_id;

	bk_logic_partition_t *pt = NULL;
	bk_partition_t   part_id = -1;
	uint32    addr = -1;

	switch(cpu_id) {
		case 1:
		{
			part_id = BK_PARTITION_APPLICATION1;
			break;
		} 
		case 2:
		{
			part_id = BK_PARTITION_APPLICATION2;
			break;
		} 
		default:
			return 0;
	}

	pt = bk_flash_partition_get_info(part_id);
	if((pt != NULL) && ((pt->partition_start_addr % 34) == 0))
	{
		addr = (pt->partition_start_addr / 34) * 32;   // CRC16 appended every 32 bytes in flash.  (32 bytes -> 34 bytes).
	}
	else
	{
		os_printf("slave core start addr not valid.\r\n");
	}

	return addr;
}

void reset_cpu1_core(uint32 offset, uint32_t start_flag)
{
	os_printf("reset_cpu1_core at: %08x, start=%d\r\n", offset, start_flag);

	#if CONFIG_SOC_BK7256XX //u-mode
	extern void mon_reset_cpu1(u32 enable, u32 start_addr);

	mon_reset_cpu1(start_flag, offset);
	#else	
	sys_drv_set_cpu1_pwr_dw(0);
	sys_drv_set_cpu1_boot_address_offset(offset >> 8);
	sys_drv_set_cpu1_reset(start_flag);
	#endif
}

extern void mb_ipc_reset_notify(u32 power_on);

void start_cpu1_core(void)
{
	uint32  addr = get_partition_addr(1);
#if CONFIG_SOC_BK7236XX
	reset_cpu1_core(SOC_FLASH_DATA_BASE + addr, 1);
#else
	reset_cpu1_core(addr, 1);
#endif
	mb_ipc_reset_notify(1);
}

void stop_cpu1_core(void)
{
	reset_cpu1_core(0, 0);
	mb_ipc_reset_notify(0);
}

void reset_cpu2_core(uint32 offset, uint32_t start_flag)
{
	os_printf("reset_cpu2_core at: %08x, start=%d\r\n", offset, start_flag);

#if (CONFIG_CPU_CNT > 2)
	sys_drv_set_cpu2_pwr_dw(0);
	sys_drv_set_cpu2_boot_address_offset(offset >> 8);
	sys_drv_set_cpu2_reset(start_flag);
#endif
}

void start_cpu2_core(void)
{
#if (CONFIG_CPU_CNT > 2)
	uint32  addr = get_partition_addr(2);

	reset_cpu2_core(SOC_FLASH_DATA_BASE + addr, 1);
#endif
}

void stop_cpu2_core(void)
{
	reset_cpu2_core(0, 0);
}

#if (CONFIG_CPU_CNT > 2)
static uint32_t s_cpu2_users_id;
static beken_mutex_t s_mutex_cpu2_users;
bk_err_t management_cpu2_init(void)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	if(s_mutex_cpu2_users == NULL) {
		bk_err_t error_state = rtos_init_mutex(&s_mutex_cpu2_users);
		if (error_state != BK_OK) {
			GLOBAL_INT_RESTORE();
			BK_ASSERT(0);
		}
	}
	GLOBAL_INT_RESTORE();

	return BK_OK;
}

int32_t vote_start_cpu2_core(cpu2_user_id_t user_id)
{
	int32_t ret = user_id;
	management_cpu2_init();

	rtos_lock_mutex(&s_mutex_cpu2_users);
	if(s_cpu2_users_id == 0) {
		bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU2, PM_POWER_MODULE_STATE_ON);
		start_cpu2_core();
	}
	else
		ret = -1;
	s_cpu2_users_id |= (0x1 << user_id);
	rtos_unlock_mutex(&s_mutex_cpu2_users);

	return ret;
}

int32_t vote_stop_cpu2_core(cpu2_user_id_t user_id)
{
	int32_t ret = user_id;
	management_cpu2_init();

	rtos_lock_mutex(&s_mutex_cpu2_users);
	s_cpu2_users_id &= ~(0x1 << user_id);
	if(s_cpu2_users_id == 0) {
		stop_cpu2_core();
		bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU2, PM_POWER_MODULE_STATE_OFF);
	}
	else
		ret = -1;
	rtos_unlock_mutex(&s_mutex_cpu2_users);

	return ret;
}
#endif	// (CONFIG_CPU_CNT > 2)

#endif // (CONFIG_CPU_CNT > 1)


void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id) {
#if CONFIG_DEBUG_FIRMWARE

	if (cpu_id == 0) {
		(void)sys_drv_set_jtag_mode(0);
	} else if (cpu_id == 1) {
		(void)sys_drv_set_jtag_mode(1);
	} else if (cpu_id == 2) {
		(void)sys_drv_set_jtag_mode(2);
	} else {
		os_printf("Unsupported cpu id(%d).\r\n", cpu_id);
		return;
	}

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT,PM_CPU_FRQ_120M);

	/*close watchdog*/
#if CONFIG_INT_WDT
	bk_wdt_stop();
#endif
#if CONFIG_TASK_WDT
	bk_task_wdt_stop();
#endif

	if (group_id == 0) {
		gpio_jtag_sel(0);
	} else if (group_id == 1) {
		gpio_jtag_sel(1);
	} else {
		os_printf("Unsupported group id(%d).\r\n", group_id);
		return;
	}
#endif
}

static void user_app_thread( void *arg )
{
	rtos_user_app_waiting_for_launch();
	/* add your user_main*/
	os_printf("user app entry(0x%0x)\r\n", s_user_app_entry);
	if(NULL != s_user_app_entry) {
		s_user_app_entry(0);
	}

#if CONFIG_SAVE_BOOT_TIME_POINT
	save_mtime_point(CPU_APP_FINISH_TIME);
#endif

	rtos_deinit_semaphore(&user_app_sema);

	rtos_delete_thread( NULL );
}

static void start_user_app_thread(void)
{
	os_printf("start user app thread.\r\n");
	rtos_create_thread(NULL,
					BEKEN_APPLICATION_PRIORITY,
					"app",
					(beken_thread_function_t)user_app_thread,
					CONFIG_APP_MAIN_TASK_STACK_SIZE,
					(beken_thread_arg_t)0);
}

extern int main(void);
extern bool ate_is_enabled(void);

static void app_main_thread(void *arg)
{
#if CONFIG_SAVE_BOOT_TIME_POINT
	save_mtime_point(CPU_MAIN_ENTRY_TIME);
#endif

#if (CONFIG_SYS_CPU0)
	rtos_user_app_preinit();
#endif

#ifdef RTOS_FUNC_TEST
	/*rtos thread func test, for bk7256 bringup.*/
	rtos_thread_func_test();
	//if nessary ,close the main() function.
#endif

#if CONFIG_TFM_FWU
	bk_ota_accept_image();
#endif
	main();

#if CONFIG_MATTER_START
	extern void ChipTest(void);
    beken_thread_t matter_thread_handle = NULL;

    os_printf("start matter\r\n");
    rtos_create_thread(&matter_thread_handle,
		BEKEN_DEFAULT_WORKER_PRIORITY,
		 "matter",
		(beken_thread_function_t)ChipTest,
		8192,
		0);
#endif
    if(ate_is_enabled())
    {
        os_printf("ATE enabled = 1\r\n");
    }

#if CONFIG_SAVE_BOOT_TIME_POINT
	save_mtime_point(CPU_MIAN_FINISH_TIME);
#endif

	rtos_delete_thread(NULL);
}

void start_app_main_thread(void)
{
	rtos_create_thread(NULL, CONFIG_APP_MAIN_TASK_PRIO,
		"main",
		(beken_thread_function_t)app_main_thread,
		CONFIG_APP_MAIN_TASK_STACK_SIZE,
		(beken_thread_arg_t)0);
}

void entry_main(void)
{
#if CONFIG_SAVE_BOOT_TIME_POINT
	save_mtime_point(CPU_MAIN_ENTRY_TIME);
#endif

	rtos_init();

#if CONFIG_GCOV
	__gcov_call_constructors();
#endif

#if (CONFIG_ATE_TEST)
	bk_set_printf_enable(0);
#endif

	if(components_init())
		return;

#if (CONFIG_FREERTOS_TRACE)
	xTraceEnable(TRC_START);
	uint32_t trace_addr = (uint32_t)xTraceGetTraceBuffer();
	uint32_t trace_size = uiTraceGetTraceBufferSize();

	rtos_regist_plat_dump_hook(trace_addr, trace_size);
#endif

#if CONFIG_SAVE_BOOT_TIME_POINT
	save_mtime_point(CPU_INIT_DRIVER_TIME);
#endif

	start_app_main_thread();
#if (CONFIG_SYS_CPU0)
	start_user_app_thread();
#endif

#if (CONFIG_SYS_CPU0) && (CONFIG_FREERTOS_V10)
	extern void rtos_init_base_time(void);
	rtos_init_base_time();
#endif

#if CONFIG_SAVE_BOOT_TIME_POINT
	save_mtime_point(CPU_START_SCHE_TIME);
#endif

	rtos_start_scheduler();
}
// eof

