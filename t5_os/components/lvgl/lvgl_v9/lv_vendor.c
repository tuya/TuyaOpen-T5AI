#include <stdio.h>
#include <string.h>
#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lv_vendor.h"
#include "driver/dma2d.h"
#if CONFIG_FATFS
#include "lv_fatfs.h"
#endif


static beken_thread_t g_disp_thread_handle;
static u32 g_init_stack_size = (1024 * 4);
static beken_mutex_t g_disp_mutex;
static beken_semaphore_t lvgl_sem;
static u8 lvgl_task_state = STATE_INIT;
static bool lv_vendor_initialized = false;
lv_vnd_config_t vendor_config = {0};


#define TAG "lvgl"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#if LV_USE_LOG
static void lv_log_print(lv_log_level_t level, const char * buf)
{
    bk_printf_raw(level, NULL, buf);
}
#endif

static uint32_t lv_tick_get_callback(void)
{
    return rtos_get_time();
}

void lv_vendor_disp_lock(void)
{
    rtos_lock_mutex(&g_disp_mutex);
}

void lv_vendor_disp_unlock(void)
{
    rtos_unlock_mutex(&g_disp_mutex);
}

void lv_vendor_init(lv_vnd_config_t *config)
{
    bk_err_t ret;

    if (lv_vendor_initialized) {
        LOGI("%s already init\n", __func__);
        return;
    }

    os_memcpy(&vendor_config, config, sizeof(lv_vnd_config_t));

    lv_init();

    lv_port_disp_init();

    lv_port_indev_init();

#if LV_USE_LOG
    lv_log_register_print_cb(lv_log_print);
#endif

#if (CONFIG_FATFS) && (LV_USE_FS_FATFS)
    lv_fatfs_init();
#endif

    lv_tick_set_cb(lv_tick_get_callback);

    ret = rtos_init_mutex(&g_disp_mutex);
    if (BK_OK != ret) {
        LOGE("%s g_disp_mutex init failed\n", __func__);
        return;
    }

    ret = rtos_init_semaphore_ex(&lvgl_sem, 1, 0);
    if (BK_OK != ret) {
        LOGE("%s semaphore init failed\n", __func__);
        return;
    }

    lv_vendor_initialized = true;

    LOGI("%s complete\n", __func__);
}

void lv_vendor_deinit(void)
{
    if (lv_vendor_initialized == false) {
        LOGI("%s already deinit\n", __func__);
        return;
    }

#if (CONFIG_FATFS) && (LV_USE_FS_FATFS)
    lv_fatfs_deinit();
#endif

#if LV_USE_LOG
    lv_log_register_print_cb(NULL);
#endif

    lv_port_disp_deinit();

    lv_port_indev_deinit();

    lv_tick_set_cb(NULL);

    rtos_deinit_mutex(&g_disp_mutex);

    rtos_deinit_semaphore(&lvgl_sem);

    os_memset(&vendor_config, 0x00, sizeof(lv_vnd_config_t));

    lv_vendor_initialized = false;

    LOGI("%s complete\n", __func__);
}

static void lv_tast_entry(void *arg)
{
    uint32_t sleep_time;

    lvgl_task_state = STATE_RUNNING;

    rtos_set_semaphore(&lvgl_sem);

    while(lvgl_task_state == STATE_RUNNING) {
        lv_vendor_disp_lock();
        sleep_time = lv_task_handler();
        lv_vendor_disp_unlock();

        #if CONFIG_LVGL_TASK_SLEEP_TIME_CUSTOMIZE
            sleep_time = CONFIG_LVGL_TASK_SLEEP_TIME;
        #else
            if (sleep_time > 500) {
                sleep_time = 500;
            } else if (sleep_time < 4) {
                sleep_time = 4;
            }
        #endif

        rtos_delay_milliseconds(sleep_time);
        // Modified by TUYA Start
        extern void tuya_app_gui_feed_watchdog(void);
        tuya_app_gui_feed_watchdog();
        // Modified by TUYA End
    }

    rtos_set_semaphore(&lvgl_sem);

    rtos_delete_thread(NULL);
}

void lv_vendor_start(void)
{
    bk_err_t ret;

    if (lvgl_task_state == STATE_RUNNING) {
        LOGI("%s already start\n", __func__);
        return;
    }

    ret = rtos_create_thread(&g_disp_thread_handle,
                             CONFIG_LVGL_TASK_PRIORITY,
                             "lvgl",
                             (beken_thread_function_t)lv_tast_entry,
                             (unsigned short)g_init_stack_size,
                             (beken_thread_arg_t)0);
    if(BK_OK != ret) {
        LOGE("%s lvgl task create failed\n", __func__);
        return;
    }

    ret = rtos_get_semaphore(&lvgl_sem, BEKEN_NEVER_TIMEOUT);
    if (BK_OK != ret) {
        LOGE("%s lvgl_sem get failed\n", __func__);
    }

    LOGI("%s complete\n", __func__);
}

void lv_vendor_stop(void)
{
    bk_err_t ret;

    if (lvgl_task_state == STATE_STOP) {
        LOGI("%s already stop\n", __func__);
        return;
    }

    lvgl_task_state = STATE_STOP;

    ret = rtos_get_semaphore(&lvgl_sem, BEKEN_NEVER_TIMEOUT);
    if (BK_OK != ret) {
        LOGE("%s lvgl_sem get failed\n", __func__);
    }

    if (lv_vendor_display_frame_cnt() == 2 || lv_vendor_draw_buffer_cnt() == 2) {
        while (bk_dma2d_is_transfer_busy()) {}
    }

    LOGI("%s complete\n", __func__);
}

int lv_vendor_display_frame_cnt(void)
{
    if (vendor_config.frame_buf_1 && vendor_config.frame_buf_2) {
        return 2;
    } else if (vendor_config.frame_buf_1 || vendor_config.frame_buf_2) {
        return 1;
    } else {
        return 0;
    }
}

int lv_vendor_draw_buffer_cnt(void)
{
    if (vendor_config.draw_buf_2_1 && vendor_config.draw_buf_2_2) {
        return 2;
    } else {
        return 1;
    }
}

// Modified by TUYA Start
void __attribute__((weak)) tuya_app_gui_feed_watchdog(void)
{

}

void __attribute__((weak)) lvMsgHandle(void)
{
}

void __attribute__((weak)) lvMsgEventReg(lv_obj_t *obj, lv_event_code_t eventCode)
{
}

void __attribute__((weak)) lvMsgEventDel(lv_obj_t *obj)
{
}
// Modified by TUYA End
