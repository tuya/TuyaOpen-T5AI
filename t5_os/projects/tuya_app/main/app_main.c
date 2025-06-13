#include "bk_private/bk_init.h"
#include <components/system.h>
#include <components/ate.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "tuya_cloud_types.h"
#include "FreeRTOS.h"

#include "media_service.h"


#if (CONFIG_SYS_CPU0)
#include "driver/pwr_clk.h"
#include "modules/pm.h"
#endif

extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern void tkl_system_sleep(const uint32_t num_ms);

#include "FreeRTOS.h"
#include "task.h"
#include "tkl_system.h"
#include "tkl_watchdog.h"
#include "driver/wdt.h"

#if (CONFIG_SYS_CPU0)
extern size_t xPortGetMinimumEverFreeHeapSize( void );
extern int tkl_system_get_free_heap_size(void);
TaskHandle_t __wdg_handle;
static void __feed_wdg(void *arg)
{
#define WDT_TIME    30000
    uint32_t cnt = 0;
    TUYA_WDOG_BASE_CFG_T cfg = {.interval_ms = WDT_TIME};
    tkl_watchdog_init(&cfg);

    while (1) {
        if (cnt++ == 20) {
            // WDT_TIME / 2 = 15s
            // 5min = 15s * 20
            cnt = 0;
            bk_printf("cpu0 heap: %d / %d\r\n", tkl_system_get_free_heap_size(), xPortGetMinimumEverFreeHeapSize());
        }
        tkl_watchdog_refresh();
        tkl_system_sleep(WDT_TIME / 2);
    }
}
#endif

#if (CONFIG_SYS_CPU1)
uint32_t start_tuya_thread = 0;
#endif

extern void tuya_app_main(void);
#if (CONFIG_SYS_CPU0)
void user_app_main(void)
{

}
#endif

int main(void)
{
#if (CONFIG_SYS_CPU0)
    // TODO ate mode
    xTaskCreate(__feed_wdg, "feed_wdg", 1024, NULL, 6, (TaskHandle_t * const )&__wdg_handle);
    rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
#endif

    bk_init();

    media_service_init();

    extern OPERATE_RET tuya_ipc_init(void);
    tuya_ipc_init();

#if (CONFIG_TUYA_TEST_CLI)
    extern int cli_tuya_test_init(void);
    cli_tuya_test_init();
#endif

#if (CONFIG_SYS_CPU0)
    bk_printf("\r\nstart cp1\r\n");
    bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_APP, PM_POWER_MODULE_STATE_ON);
#endif

#if (CONFIG_SYS_CPU1)
    bk_printf("\r\nstart tuya_app_main\r\n");
    start_tuya_thread = 1;
    tuya_app_main();
    // TUYA_LwIP_Init(); // �������ע�� tuya_app_main, ��Ҫ��ʼ��lwip���ײ�mac���ݴ�����Ҫpbuf��Դ
#endif
    return 0;
}
