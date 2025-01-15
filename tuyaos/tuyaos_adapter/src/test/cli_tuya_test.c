#include "cli.h"
#include "flash.h"
#include "modules/pm.h"
#include "modules/wifi.h"
#include "tuya_cloud_types.h"
#include "tkl_wifi.h"
#include "cli_tuya_test.h"
#include "tkl_display.h"
#include "lwip_netif_address.h"
#include "lwip/inet.h"
#include "driver/hal/hal_efuse_types.h"
#include "driver/otp_types.h"

#define __PRINT_MACRO(x) #x
#define PRINT_MACRO(x) #x"="__PRINT_MACRO(x)
//#pragma message(PRINT_MACRO(AON_RTC_DEFAULT_CLOCK_FREQ))


extern void tkl_system_sleep(const uint32_t num_ms);

static void cli_rf_set_cali_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_printf("set rf calibration flag begin\r\n");

    char *arg[5];
    arg[0] = "txevm";
    arg[1] = "-e";
    arg[2] = "2\r\n";
    tx_evm_cmd_test(pcWriteBuffer, xWriteBufferLen, 3, arg);
    tkl_system_sleep(200);

    arg[0] = "txevm";
    arg[1] = "-g";
    arg[2] = "8\r\n";
    tx_evm_cmd_test(pcWriteBuffer, xWriteBufferLen, 3, arg);
    tkl_system_sleep(200);

    arg[0] = "txevm";
    arg[1] = "-g";
    arg[2] = "0\r\n";
    tx_evm_cmd_test(pcWriteBuffer, xWriteBufferLen, 3, arg);
    tkl_system_sleep(200);

    arg[0] = "txevm";
    arg[1] = "-e";
    arg[2] = "1\r\n";
    tx_evm_cmd_test(pcWriteBuffer, xWriteBufferLen, 3, arg);
    tkl_system_sleep(200);

    arg[0] = "txevm";
    arg[1] = "-s";
    arg[2] = "11";
    arg[3] = "1";
    arg[4] = "20\r\n";
    tx_evm_cmd_test(pcWriteBuffer, xWriteBufferLen, 5, arg);
    tkl_system_sleep(200);

    arg[0] = "txevm";
    arg[1] = "-e";
    arg[2] = "2\r\n";
    tx_evm_cmd_test(pcWriteBuffer, xWriteBufferLen, 3, arg);
    tkl_system_sleep(200);

    arg[0] = "txevm";
    arg[1] = "-e";
    arg[2] = "4";
    arg[3] = "1\r\n";
    tx_evm_cmd_test(pcWriteBuffer, xWriteBufferLen, 4, arg);
    tkl_system_sleep(200);

    arg[0] = "txevm";
    arg[1] = "-g";
    arg[2] = "8\r\n";
    tx_evm_cmd_test(pcWriteBuffer, xWriteBufferLen, 3, arg);
    tkl_system_sleep(200);

    bk_printf("set rf calibration flag end\r\n");
}

#if CONFIG_SYS_CPU0 && CONFIG_SOC_BK7258
#include "tkl_wakeup.h"
#include "tkl_semaphore.h"
#include "tkl_audio.h"
#include "FreeRTOS.h"
#include "task.h"
TaskHandle_t __thread_handle = NULL;
TKL_SEM_HANDLE __test_sem;
static void __test_wakeup_func(void *arg)
{
    tkl_semaphore_create_init(&__test_sem, 0, 1);
    while (1) {
        tkl_semaphore_wait(__test_sem, 0xffffffff);
        bk_printf("ooooooooooooooooo\r\n");
        bk_printf("ooooooooooooooooo\r\n");
        bk_printf("ooooooooooooooooo\r\n");

        // init uvc
        tkl_vi_init(NULL, 0);
        tkl_system_sleep(100);
        // init lcd
        tkl_disp_init(NULL, NULL);
        // init h264
        tkl_venc_init(0, NULL, 0);

        TKL_AUDIO_CONFIG_T config;
        config.put_cb = NULL;
        tkl_ai_init(&config, 0);
    }
}

static void __pm_gpio_callback(void *arg)
{
    bk_printf("test __pm_gpio_callback[%d]: %d\r\n",
            bk_pm_exit_low_vol_wakeup_source_get(), (uint32_t)arg);
    tkl_semaphore_post(__test_sem);
}

static void __set_test_wakeup_source(uint32_t type, uint32_t param)
{
    TUYA_WAKEUP_SOURCE_BASE_CFG_T cfg;
    if (type == 2) {
        TUYA_GPIO_IRQ_T irq_cfg;
        irq_cfg.mode = TUYA_GPIO_IRQ_RISE;
        irq_cfg.cb = __pm_gpio_callback;
        irq_cfg.arg = (void *)33;
        tkl_gpio_irq_init(33, &irq_cfg);

        if (__thread_handle == NULL)
            xTaskCreate(__test_wakeup_func, "test_func", 1024, NULL, 6, (TaskHandle_t * const )&__thread_handle);

        cfg.source = TUYA_WAKEUP_SOURCE_GPIO;
        cfg.wakeup_para.gpio_param.gpio_num = 33;
        cfg.wakeup_para.gpio_param.level = 1;
    } else if (type == 1) {
        uint32_t t = (param == 0)? 5: param;
        cfg.source = TUYA_WAKEUP_SOURCE_TIMER;
        cfg.wakeup_para.timer_param.timer_num = TUYA_TIMER_NUM_5;
        cfg.wakeup_para.timer_param.mode = TUYA_TIMER_MODE_ONCE;
        cfg.wakeup_para.timer_param.ms = t * 1000;
    }

    tkl_wakeup_source_set(&cfg);
    tkl_system_sleep(200);
}

static void cli_wifi_set_interval_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint8_t interval = 0;
    int ret = 0;
    char *msg = NULL;

    if (argc < 2) {
        bk_printf("invalid argc num\r\n");
        goto error;
    }

    if (!os_strcmp(argv[1], "deepsleep")) {
        TUYA_WAKEUP_SOURCE_BASE_CFG_T  cfg;
        if (!os_strcmp(argv[2], "rtc")) {
            __set_test_wakeup_source(1, 12);
        }else if (!os_strcmp(argv[2], "gpio")) {
            __set_test_wakeup_source(2, 0);
        } else {
            goto error;
        }
        tkl_cpu_sleep_mode_set(1, TUYA_CPU_DEEP_SLEEP);
    } else if (!os_strcmp(argv[1], "sleep")) {
        if (!os_strcmp(argv[2], "rtc")) {
            __set_test_wakeup_source(1, 12);
        } else if (!os_strcmp(argv[2], "gpio")) {
            __set_test_wakeup_source(2, 0);
        } else {
            goto error;
        }
        tkl_cpu_sleep_mode_set(1, TUYA_CPU_SLEEP);
    } else if (!os_strcmp(argv[1], "exit")) {
        tkl_cpu_sleep_mode_set(0, 0);
    } else {
        interval = (uint8_t)os_strtoul(argv[1], NULL, 10);
        bk_printf("\r\nset wifi dtim %d\r\n", interval);
        ret = bk_wifi_send_listen_interval_req(interval);
        if (!ret) {
            bk_printf("set_interval ok");
            msg = WIFI_CMD_RSP_SUCCEED;
            os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
            return;
        } else {
            bk_printf("set_interval failed");
            goto error;
        }
        bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 1, 0);
    }

    return;

error:
    bk_printf("Usage: lp deepsleep [rtc|gpio] [rtc time<seconds>|gpio num]");
    msg = WIFI_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return;
}
#endif // CONFIG_SYS_CPU0 && CONFIG_SOC_BK7258

static void __get_flash_id(void)
{
    uint32_t flash_size;
    uint32_t flash_id = bk_flash_get_id();

    flash_size = 2 << ((flash_id & 0xff) - 1);

    bk_printf("flash id: 0x%08x, flash size: %x / %dM\r\n", flash_id, flash_size, flash_size / 1048576);
}

#if CONFIG_FREERTOS_V10
extern void port_check_isr_stack(void);
#endif // CONFIG_FREERTOS_V10
#include "tuya_cloud_types.h"

uint32_t  __attribute__((weak)) ty_app_memory_occupied(void)
{
    return 0;
}

static void cli_system_info_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc > 1) {
        for (int i = 0; i < argc; i++) {
            switch (argv[i][0]) {
                case 't':
                    rtos_dump_backtrace();
                    break;
#if CONFIG_FREERTOS_V10
                case 's':
                    port_check_isr_stack();
                    break;
#endif // CONFIG_FREERTOS_V10
#if CONFIG_FREERTOS && CONFIG_MEM_DEBUG
                case 'm':
                    os_dump_memory_stats(0, 0, NULL);
                    break;
#endif // CONFIG_FREERTOS && CONFIG_MEM_DEBUG
                default:
                    bk_printf("unknown param: %s\r\n", argv[i]);
                    break;
            }
        }

    }

    rtos_dump_task_list();

#if CONFIG_FREERTOS
    rtos_dump_task_runtime_stats();
#endif // CONFIG_FREERTOS

    WF_WK_MD_E mode = WWM_UNKNOWN;
    NW_IP_S ip;
    NW_MAC_S mac;

    os_memset(&ip, 0, sizeof(NW_IP_S));
    os_memset(&mac, 0, sizeof(NW_MAC_S));

    tkl_wifi_get_work_mode(&mode);
    if (mode == WWM_STATION) {
        tkl_wifi_get_ip(WF_STATION, &ip);
        tkl_wifi_get_mac(WF_STATION, &mac);
        bk_printf("sta ");
    } else if (mode == WWM_SOFTAP) {
        tkl_wifi_get_ip(WF_AP, &ip);
        tkl_wifi_get_mac(WF_AP, &mac);
        bk_printf("ap ");
    }

#if defined(ENABLE_IPv6) && (ENABLE_IPv6 == 1)
    if (ip.type == TY_AF_INET) {
        bk_printf("ip: %s mask: %s gw: %s\r\n",
                ip.addr.ip4.ip, ip.addr.ip4.mask, ip.addr.ip4.gw);
    } else if (ip.type == TY_AF_INET6) {
        bk_printf("ip: %s\r\n", ip.addr.ip6.ip);
    }
#else
    bk_printf("ip: %s mask: %s gw: %s\r\n",
            ip.ip, ip.mask, ip.gw);
#endif
    bk_printf("mac: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            mac.mac[0], mac.mac[1], mac.mac[2], mac.mac[3], mac.mac[4], mac.mac[5]);

//    extern uint32_t app_malloc_cnt;
//    extern uint32_t app_free_cnt;
//    extern uint32_t ty_app_memory_occupied(void);
//    bk_printf("\r\n app malloc/free: %d / %d\r\n", app_malloc_cnt, app_free_cnt);
//    bk_printf("app occupied: %d\r\n", ty_app_memory_occupied());
    bk_printf("sram left heap: %d\r\n", xPortGetFreeHeapSize());
    bk_printf("psram left: %d, total: %d, use count: %d\r\n",
            xPortGetPsramFreeHeapSize(),
            xPortGetPsramTotalHeapSize(),
            bk_psram_heap_get_used_count());
    bk_printf("runtime: %d\r\n", xTaskGetTickCount());

    __get_flash_id();

    uint8_t dev_id[EFUSE_DEVICE_ID_BYTE_NUM];
    bk_otp_apb_read(OTP_DEVICE_ID, dev_id, EFUSE_DEVICE_ID_BYTE_NUM);
    bk_printf("dev id: ");
    for (int i = 0; i < EFUSE_DEVICE_ID_BYTE_NUM; i++) {
        bk_printf(" %02x", dev_id[i]);
    }
    bk_printf("\r\n");

#if CONFIG_SYS_CPU0
    pm_debug_pwr_clk_state();
    pm_debug_lv_state();
#endif
    return;
}

#include "tkl_wifi.h"
static void cli_scan_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2) {
        bk_printf("xscan [ssid]");
        return;
    }
    bk_printf("ssid: %s\r\n", argv[1]);

    tkl_wifi_set_work_mode(WWM_STATION);   //设置为station开始扫描

    uint32_t num = 0;
    AP_IF_S *ap = NULL;
    tkl_wifi_scan_ap(argv[1], &ap, &num);
    if (ap) {
        bk_printf("ap rssi %d, bssid: %02x:%02x:%02x:%02x:%02x:%02x, ssid %s\r\n",
                ap->rssi, ap->bssid[0], ap->bssid[1], ap->bssid[2],
                ap->bssid[3], ap->bssid[4], ap->bssid[5], ap->ssid);
    }

    if (ap) {
        tkl_wifi_release_ap(ap);
        ap = NULL;
    }
}

#include <modules/pm.h>
#include <driver/aon_rtc_types.h>
static void cli_xxxt_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    float t1 = AON_RTC_MS_TICK_CNT;
    float t2 = (float)AON_RTC_MS_TICK_CNT;
    uint32_t t3 = (uint32_t)AON_RTC_MS_TICK_CNT;

    bk_printf("t1: %f %d\r\n", t1, t1);
    bk_printf("t2: %f %d\r\n", t2, t2);
    bk_printf("t3: %d\r\n", t3);

    bk_printf("rtc clk: %d %f %d\r\n",
            bk_rtc_get_clock_freq(),
            bk_rtc_get_ms_tick_count(),
            bk_rtc_get_ms_tick_count());
}

#include <driver/qspi.h>
#include <driver/qspi_flash.h>
#include "qspi_hal.h"
#include <driver/qspi.h>
//#include "qspi_driver.h"
//#include "qspi_hal.h"

typedef enum {
    TEST_ENUM_0 = 0,
    TEST_ENUM_1,
    TEST_ENUM_2,
    TEST_ENUM_3,
    TEST_ENUM_4,
    TEST_ENUM_5,
    TEST_ENUM_MAX = 257
} __TEST_ENUM_T;

extern OPERATE_RET tkl_wifi_station_disconnect(void);
extern void tkl_system_psram_free(void* ptr);
extern void* tkl_system_psram_malloc(const SIZE_T size);
static void cli_uart_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_printf("1111111111111111111111111111111111111111\r\n");
    bk_printf("xxx: %d\r\n", sizeof(__TEST_ENUM_T));
    bk_printf("2222222222222222222222222222222222222222\r\n");
}


#define TUYA_TEST_CMD_CNT (sizeof(s_sinfo_commands) / sizeof(struct cli_command))
static const struct cli_command s_sinfo_commands[] = {
    {"info", "system info", cli_system_info_cmd},
#if CONFIG_SYS_CPU0 && CONFIG_SOC_BK7258
    {"lp", "set wifi dtim", cli_wifi_set_interval_cmd},
#endif // CONFIG_SYS_CPU0 && CONFIG_SOC_BK7258
    {"rf_cali", "set rf calibration flag, just for test", cli_rf_set_cali_cmd},
    {"audio_test", "mic to speaker test", cli_audio_test_cmd},
    {"xadc", "adc test", cli_adc_cmd},
    {"xgpio", "gpio test", cli_gpio_cmd},
    {"xscan", "scan", cli_scan_cmd},
    {"xlcd", "lcd test", cli_xlcd_cmd},
    {"xwifi", "wifi test", cli_wifi_cmd},

    {"xpwm", "pwm test", cli_pwm_cmd},

#if (CONFIG_CPU_INDEX == 0)
    {"xmt", "tuya media test", cli_tuya_media_cmd},
#endif // CONFIG_CPU_INDEX == 0

    {"sf", "startup frame", cli_sf_cmd},

    {"xq", "test", cli_xxxt_cmd},
    {"xu", "uart test", cli_uart_test_cmd},
    {"xqspi", "qspi test", cli_xqspi_cmd},
    {"lfs", "little fs test", cli_littlefs_cmd},
    // {"xt", "timer test", cli_tkl_timer_test},
    {"xt", "timer test", cli_timer_cmd},
};

int cli_tuya_test_init(void)
{
    cli_register_commands(s_sinfo_commands, TUYA_TEST_CMD_CNT);
    cli_mp3_init();
    return 0;
}

