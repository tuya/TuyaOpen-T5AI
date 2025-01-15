#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

#include "cli.h"

#include "lcd_act.h"
#include "media_app.h"
#if CONFIG_LVGL
#include "lv_vendor.h"
#include "lv_demo_widgets.h"
#endif
#include "driver/drv_tp.h"
#include <driver/lcd.h>
#include "media_service.h"
#include "media_ipc.h"

#define TAG "MAIN"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

#define CMDS_COUNT  (sizeof(cli_widgets_cmd) / sizeof(struct cli_command))

static meida_ipc_t ipc = NULL;

const lcd_open_t lcd_open =
{
    .device_ppi = PPI_480X272,
    .device_name = "ruide",
};

void cli_widgets_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    os_printf("%s\r\n", __func__);
}

static const struct cli_command s_widgets_commands[] =
{
    {"widgets", "widgets", cli_widgets_cmd},
};

int cli_widgets_init(void)
{
    return cli_register_commands(s_widgets_commands, CMDS_COUNT);
}

#if (CONFIG_SYS_CPU1) && (CONFIG_SOC_BK7258)
#include "yuv_encode.h"

void lvgl_event_handle(media_mailbox_msg_t *msg)
{
    os_printf("%s EVENT_LVGL_OPEN_IND \n", __func__);

    lv_vnd_config_t lv_vnd_config = {0};
    lcd_open_t *lcd_open = (lcd_open_t *)msg->param;

#ifdef CONFIG_LVGL_USE_PSRAM
#define PSRAM_DRAW_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)

    lv_vnd_config.draw_pixel_size = ppi_to_pixel_x(lcd_open->device_ppi) * ppi_to_pixel_y(lcd_open->device_ppi);
    lv_vnd_config.draw_buf_2_1 = (lv_color_t *)PSRAM_DRAW_BUFFER;
    lv_vnd_config.draw_buf_2_2 = (lv_color_t *)(PSRAM_DRAW_BUFFER + lv_vnd_config->draw_pixel_size * sizeof(lv_color_t));
#else
#define PSRAM_FRAME_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)
    lv_vnd_config.draw_pixel_size = (120 * 1024) / sizeof(lv_color_t);
    lv_vnd_config.draw_buf_2_1 = LV_MEM_CUSTOM_ALLOC(lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
    lv_vnd_config.draw_buf_2_2 = NULL;
    lv_vnd_config.frame_buf_1 = (lv_color_t *)PSRAM_FRAME_BUFFER;
    lv_vnd_config.frame_buf_2 = (lv_color_t *)(PSRAM_FRAME_BUFFER + ppi_to_pixel_x(lcd_open->device_ppi) * ppi_to_pixel_y(lcd_open->device_ppi) * sizeof(lv_color_t));
#endif
    lv_vnd_config.lcd_hor_res = ppi_to_pixel_x(lcd_open->device_ppi);
    lv_vnd_config.lcd_ver_res = ppi_to_pixel_y(lcd_open->device_ppi);
    lv_vnd_config.rotation = ROTATE_NONE;

    lv_vendor_init(&lv_vnd_config);

    lcd_display_open(lcd_open);

#if (CONFIG_TP)
    drv_tp_open(ppi_to_pixel_x(lcd_open->device_ppi), ppi_to_pixel_y(lcd_open->device_ppi), TP_MIRROR_X_Y_COORD);
#endif

    lv_vendor_disp_lock();
    lv_demo_widgets();
    lv_vendor_disp_unlock();

    lv_vendor_start();

    msg_send_rsp_to_media_major_mailbox(msg, BK_OK, APP_MODULE);
}
#endif

#if (CONFIG_SYS_CPU0)
void widgets_init(void)
{
    cli_widgets_init();

    bk_err_t ret;
    os_printf("!!!BK7258 LVGL WIDGETS!!!\r\n");

    ret = media_app_lcd_pipeline_disp_open((lcd_open_t *)&lcd_open);
    if (ret != BK_OK)
    {
        os_printf("media_app_lcd_pipeline_open failed\r\n");
        return;
    }

    ret = media_ipc_send(&ipc, (void*)&lcd_open, sizeof(lcd_open), MIPC_CHAN_SEND_FLAG_SYNC);

    if (ret != BK_OK)
    {
        LOGI("media_ipc_send failed\n");
    }
}
#endif

static int media_ipc_lvgl_callback(uint8_t *data, uint32_t size, void *param)
{
#if (CONFIG_SYS_CPU1)

    lv_vnd_config_t lv_vnd_config = {0};
    lcd_open_t *lcd_open = (lcd_open_t *)data;

    LOGI("%s %s, size: %d \n", __func__, lcd_open->device_name, size);


#ifdef CONFIG_LVGL_USE_PSRAM
#define PSRAM_DRAW_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)

    lv_vnd_config.draw_pixel_size = ppi_to_pixel_x(lcd_open->device_ppi) * ppi_to_pixel_y(lcd_open->device_ppi);
    lv_vnd_config.draw_buf_2_1 = (lv_color_t *)PSRAM_DRAW_BUFFER;
    lv_vnd_config.draw_buf_2_2 = (lv_color_t *)(PSRAM_DRAW_BUFFER + lv_vnd_config->draw_pixel_size * sizeof(lv_color_t));
#else
#define PSRAM_FRAME_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)
    lv_vnd_config.draw_pixel_size = (120 * 1024) / sizeof(lv_color_t);
    lv_vnd_config.draw_buf_2_1 = LV_MEM_CUSTOM_ALLOC(lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
    lv_vnd_config.draw_buf_2_2 = NULL;
    lv_vnd_config.frame_buf_1 = (lv_color_t *)PSRAM_FRAME_BUFFER;
    lv_vnd_config.frame_buf_2 = (lv_color_t *)(PSRAM_FRAME_BUFFER + ppi_to_pixel_x(lcd_open->device_ppi) * ppi_to_pixel_y(lcd_open->device_ppi) * sizeof(lv_color_t));
#endif
    lv_vnd_config.lcd_hor_res = ppi_to_pixel_x(lcd_open->device_ppi);
    lv_vnd_config.lcd_ver_res = ppi_to_pixel_y(lcd_open->device_ppi);
    lv_vnd_config.rotation = ROTATE_NONE;

#if (CONFIG_TP)
    drv_tp_open(ppi_to_pixel_x(lcd_open->device_ppi), ppi_to_pixel_y(lcd_open->device_ppi), TP_MIRROR_X_Y_COORD);
#endif

    lv_vendor_init(&lv_vnd_config);

    lv_vendor_disp_lock();
    lv_demo_widgets();
    lv_vendor_disp_unlock();

    lv_vendor_start();
#endif

    return 0;
}

void user_app_main(void)
{

}

int main(void)
{
#if (CONFIG_SYS_CPU0)
    rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
    // bk_set_printf_sync(true);
    // shell_set_log_level(BK_LOG_WARN);
#endif
    bk_init();
    media_service_init();

#if (CONFIG_SYS_CPU0 || CONFIG_SYS_CPU1)
    media_ipc_chan_cfg_t cfg = {0};
    cfg.cb = media_ipc_lvgl_callback;
    cfg.name = "app";
    cfg.param = NULL;

    if (media_ipc_channel_open(&ipc, &cfg) != BK_OK)
    {
        LOGE("open ipc failed\n");
    }
#endif


#if (CONFIG_SYS_CPU0)
    widgets_init();
#endif
    return 0;
}
