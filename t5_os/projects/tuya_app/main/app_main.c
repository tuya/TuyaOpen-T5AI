#include "bk_private/bk_init.h"
#include <components/system.h>
#include <components/ate.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "tuya_cloud_types.h"
#include "FreeRTOS.h"

#include "media_service.h"

extern void tuya_app_main(void);
extern void user_reg_usbenum_cb(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern void tkl_system_sleep(uint32_t num_ms);

#include "FreeRTOS.h"
#include "task.h"
#include "tkl_system.h"
#include "tkl_watchdog.h"
#include "driver/wdt.h"
TaskHandle_t __wdg_handle;
static void __feed_wdg(void *arg)
{
#define WDT_TIME    30000
    TUYA_WDOG_BASE_CFG_T cfg = {.interval_ms = WDT_TIME};
    tkl_watchdog_init(&cfg);
    while (1) {
        tkl_watchdog_refresh();
        tkl_system_sleep(WDT_TIME / 2);
    }
}

#if (CONFIG_SYS_CPU0)

#if 0
#define OTP_FLASH_DATA_SIZE        1024
#define OTP_FLASH_RFDATA_SIZE       512
#define PARTITION_SIZE         (1 << 12) /* 4KB */

#include "string.h"
#include "bk_wifi_types.h"
#include "flash_bypass.h"
#include <common/bk_typedef.h>
#include "driver/flash.h"
#include <driver/flash_partition.h>
#include <os/mem.h>
#include <modules/wifi.h>

extern void flash_lock(void);
extern void flash_unlock(void);

static void __read_otp_flash_rfcali_data(uint8_t *otp_data, uint16_t len)
{
    flash_bypass_otp_ctrl_t otp_op     = {0};
    otp_op.otp_idx		= 1;  // "1 or 2 or 3"
    otp_op.addr_offset  = 0;
    otp_op.write_len 	= 0;
    otp_op.write_buf	= NULL;
    otp_op.read_len		= len;
    otp_op.read_buf		= otp_data;

    int ret = flash_bypass_otp_operation(FLASH_BYPASS_OTP_READ, &otp_op);
	if (ret != BK_OK)
		bk_printf("otp flash read failed\n");
}

static int __check_otp_flash_rfcali_data(uint8_t *otp_data, uint16_t len)
{
    struct txpwr_elem_st
    {
        UINT32 type;
        UINT32 len;
    } *head;

    head = (struct txpwr_elem_st *)otp_data;
    if (head->type != INFO_TLV_HEADER) {
        bk_printf("otp flash data type error %x\n", head->type);
        return -1;
    }
    return 0;
}

void backup_rfcali_data(void)
{
    uint32_t addr;
    uint8_t *dst;
    uint32_t size;

    dst = os_malloc(OTP_FLASH_DATA_SIZE);
    if (dst == NULL) {
        bk_printf("malloc rfcali data failed\n");
        return;
    }

    memset(dst, 0, OTP_FLASH_DATA_SIZE);

    flash_bypass_otp_ctrl_t otp_op     = {0};
    otp_op.otp_idx      = 1;  // "1 or 2 or 3"
    otp_op.addr_offset  = 0;
    otp_op.write_len	= 0;
    otp_op.write_buf	= NULL;
    otp_op.read_len		= OTP_FLASH_DATA_SIZE;
    otp_op.read_buf		= dst;
    int ret = flash_bypass_otp_operation(FLASH_BYPASS_OTP_READ, &otp_op);
	if (ret != BK_OK) {
        bk_printf("read otp flash failed\n");
		if (dst)
			os_free(dst);

		return ;
	}

    /* TODO: need to consider whether to use locks at the TKL layer*/
    flash_lock();

	bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_RF_FIRMWARE);
    addr = pt->partition_start_addr;
    size = OTP_FLASH_RFDATA_SIZE;
	bk_flash_read_bytes(addr, (uint8_t *)dst, size);

    /* TODO: need to consider whether to use locks at the TKL layer*/
    flash_unlock();

	memset(&otp_op, 0, sizeof(flash_bypass_otp_ctrl_t));
    otp_op.otp_idx      = 1;  // "1 or 2 or 3"
    otp_op.addr_offset  = 0;
    otp_op.write_len	= OTP_FLASH_DATA_SIZE;
    otp_op.write_buf	= dst;
    otp_op.read_len		= 0;
    otp_op.read_buf		= NULL;

    ret = flash_bypass_otp_operation(FLASH_BYPASS_OTP_WRITE, &otp_op);
	if (ret != BK_OK)
        bk_printf("write otp flash failed\n");

    if (dst)
        os_free(dst);
}

static void __recovery_rfcali_data(uint8_t *otp_data, uint16_t len)
{
    uint32_t addr;

	bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_RF_FIRMWARE);
    addr = pt->partition_start_addr;

    /* TODO: need to consider whether to use locks at the TKL layer*/
    flash_lock();

    bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	bk_flash_erase_sector(addr);
    bk_flash_write_bytes(addr, (const uint8_t *)otp_data, OTP_FLASH_RFDATA_SIZE);
    bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);

    /* TODO: need to consider whether to use locks at the TKL layer*/
    flash_unlock();

	bk_printf("recovery rfcali data success\n");
}
static int user_recovery_rfcali_data(void)
{
	int stat = bk_wifi_manual_cal_rfcali_status();

    if(stat == BK_OK) {
		return 0;
    }

    bk_printf("[NOTE]: rfcali data isn't exist\n");

    uint8_t *otp_data = os_malloc(OTP_FLASH_DATA_SIZE);
    if(otp_data == NULL) {
        bk_printf("malloc rfcali data failed\n");
        return -1;
    }
    memset(otp_data, 0, OTP_FLASH_DATA_SIZE);
    __read_otp_flash_rfcali_data(otp_data, OTP_FLASH_DATA_SIZE);

    if (__check_otp_flash_rfcali_data(otp_data, OTP_FLASH_DATA_SIZE) < 0) {
        bk_printf("check rfcali data failed\n");
    } else {
        bk_printf("check rfcali data success\n");
        __recovery_rfcali_data(otp_data, OTP_FLASH_RFDATA_SIZE);
    }
    os_free(otp_data);

    return 0;
}
#endif

static beken_semaphore_t g_wait_cpu0_media_init_done_sem;
void user_app_main(void)
{
    rtos_get_semaphore(&g_wait_cpu0_media_init_done_sem, BEKEN_NEVER_TIMEOUT);
    rtos_deinit_semaphore(&g_wait_cpu0_media_init_done_sem);

    // disable shell echo for gpio test
    shell_echo_set(0);
#if CONFIG_USB
    user_reg_usbenum_cb();
#endif // CONFIG_USB

    // user_recovery_rfcali_data();

    extern void tuya_app_main(void);
    tuya_app_main();

#if (CONFIG_TUYA_TEST_CLI)
    extern int cli_tuya_test_init(void);
    cli_tuya_test_init();
#endif

}
#endif

int main(void)
{
#if (CONFIG_SYS_CPU0)
    bk_printf("-------- left heap: %d, reset reason: %x\r\n",
            xPortGetFreeHeapSize(), bk_misc_get_reset_reason() & 0xFF);
    extern int tuya_upgrade_main(void);
    // extern TUYA_OTA_PATH_E tkl_ota_is_under_seg_upgrade(void);
    bk_err_t ret = BK_FAIL;
    ret = rtos_init_semaphore_ex(&g_wait_cpu0_media_init_done_sem, 1, 0);
    if (BK_OK != ret) {
        bk_printf("%s semaphore init failed\n", __func__);
        return -1;
    }

    // if(TUYA_OTA_PATH_INVALID != tkl_ota_is_under_seg_upgrade()) {
    //     bk_printf("goto tuya_upgrade_main\r\n");
    //     rtos_set_user_app_entry((beken_thread_function_t)tuya_upgrade_main);
    // } else {
    {
        if (!ate_is_enabled()) {
            bk_printf("go to tuya\r\n");
            rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
        } else {
            // in ate mode, feed dog
            xTaskCreate(__feed_wdg, "feed_wdg", 1024, NULL, 6, (TaskHandle_t * const )&__wdg_handle);
        }
    }
    // bk_set_printf_sync(true);
    // shell_set_log_level(BK_LOG_WARN);
#endif // CONFIG_SYS_CPUx
	bk_init();
    media_service_init();

#if (CONFIG_SYS_CPU1 && CONFIG_TUYA_LOG_OPTIMIZATION)
    bk_set_printf_enable(1);
#endif

extern OPERATE_RET tuya_ipc_init(void);
    tuya_ipc_init();
#if (CONFIG_SYS_CPU1)
    bk_set_printf_enable(1);
#endif

#if (CONFIG_SYS_CPU0)
    if(g_wait_cpu0_media_init_done_sem) {
        rtos_set_semaphore(&g_wait_cpu0_media_init_done_sem);
    }
#endif

    return 0;
}
