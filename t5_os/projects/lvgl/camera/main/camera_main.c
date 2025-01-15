#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

#include "cli.h"

#if (CONFIG_SYS_CPU1) || (CONFIG_SYS_CPU0)
#include "lcd_act.h"
#include "media_app.h"
#include "media_evt.h"
#endif

#if (CONFIG_LVGL)
#include "lvgl.h"
#include "lv_vendor.h"
#endif
#include "driver/drv_tp.h"
#include "lvgl_vfs_init.h"
#include <driver/lcd.h>
#include "doorbell_comm.h"
#include "media_service.h"

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern void lv_example_meter(void);
extern void lv_example_meter_exit(void);


#if (CONFIG_SOC_BK7256)
const lcd_open_t lcd_open =
{
	.device_ppi = PPI_800X480,
	.device_name = "h050iwv",
};

media_camera_device_t camera_device = {
	.type = UVC_CAMERA,
	.mode = JPEG_MODE,
	.fmt = PIXEL_FMT_JPEG,
	.info.fps = FPS25,
	.info.resolution.width = 800,
	.info.resolution.height = 480,
};

void lvcamera_open(void)
{
	os_printf("%s\r\n", __func__);

#if (CONFIG_TP)
	drv_tp_close();
#endif

#if (CONFIG_LVGL)
	lv_example_meter_exit();
	lv_vendor_stop();
#endif

	media_app_camera_open(&camera_device);
}

void lvcamera_close(void)
{
	os_printf("%s\r\n", __func__);

	media_app_camera_close(camera_device.type);

#if (CONFIG_TP)
	drv_tp_open(ppi_to_pixel_x(lcd_open.device_ppi), ppi_to_pixel_y(lcd_open.device_ppi), TP_MIRROR_NONE);
#endif

#if (CONFIG_LVGL)
	lv_vendor_start();
	lv_example_meter();
#endif
}
#endif

#if (CONFIG_SOC_BK7258) && (CONFIG_SYS_CPU0)
static bool lvcam_is_open = false;
static bool lcd_jdec_is_first_open = true;

const lcd_open_t lcd_open =
{
	.device_ppi = PPI_480X854,
	.device_name = "st7701sn",
};

media_camera_device_t camera_device = {
	.type = UVC_CAMERA,
	.mode = JPEG_MODE,
	.fmt = PIXEL_FMT_JPEG,
	.info.fps = FPS25,
	.info.resolution.width = 864,
	.info.resolution.height = 480,
};

void lvcamera_open(void)
{
    os_printf("%s\r\n", __func__);
    bk_err_t ret;

    if (lvcam_is_open) {
        os_printf("lvcam is already open\r\n");
        return;
    }

    if (lcd_jdec_is_first_open) {
        media_app_pipline_set_rotate(ROTATE_90);

        ret = media_app_lcd_pipeline_jdec_open();
        if (ret != BK_OK) {
            os_printf("media_app_lcd_pipeline_jdec_open failed\r\n");
            return;
        }
        lcd_jdec_is_first_open = false;
    }

    ret = media_app_lvcam_lvgl_close();
    if (ret != BK_OK) {
        os_printf("media_app_lvgl_close failed\r\n");
        return;
    }

    lvcam_is_open = true;
}

void lvcamera_close(void)
{
    os_printf("%s\r\n", __func__);
    bk_err_t ret;

    if (!lvcam_is_open) {
        os_printf("lvcam has not been opened, please input the \"lvcam open \" command\r\n");
        return;
    }

    ret = media_app_lvcam_lvgl_open((lcd_open_t *)&lcd_open);
    if (ret != BK_OK) {
        os_printf("media_app_lvgl_open failed\r\n");
        return;
    }

    lvcam_is_open = false;
}
#endif

#if (CONFIG_SYS_CPU0)
#define CMDS_COUNT  (sizeof(s_lvcamera_commands) / sizeof(struct cli_command))

void cli_lvcamera_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	os_printf("%s\r\n", __func__);

	if (os_strcmp(argv[1], "open") == 0)
	{
		lvcamera_open();
	}

	if (os_strcmp(argv[1], "close") == 0)
	{
		lvcamera_close();
	}
}

static const struct cli_command s_lvcamera_commands[] =
{
	{"lvcam", "lvcam", cli_lvcamera_cmd},
};

int cli_lvcamera_init(void)
{
	return cli_register_commands(s_lvcamera_commands, CMDS_COUNT);
}
#endif


#if (CONFIG_SYS_CPU1) && (CONFIG_SOC_BK7258)
#include "frame_buffer.h"
#include "yuv_encode.h"
#include "lv_vendor.h"
#include "driver/media_types.h"


extern uint8_t lvgl_disp_enable;
extern lv_vnd_config_t vendor_config;
extern frame_buffer_t *lvgl_frame_buffer;
lv_obj_t * qr;

static void lv_example_qrcode(void)
{
    LV_IMG_DECLARE(bg_img);
    lv_obj_t * img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &bg_img);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_obj_set_width(label, LV_SIZE_CONTENT);
    lv_obj_set_height(label, LV_SIZE_CONTENT);
    lv_obj_set_x(label, 0);
    lv_obj_set_y(label, 200);
    lv_obj_set_align(label, LV_ALIGN_TOP_MID);
    lv_label_set_text(label, "Welcome to BEKEN");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_32, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_color_t bg_color = lv_palette_lighten(LV_PALETTE_LIGHT_BLUE, 5);
    lv_color_t fg_color = lv_palette_darken(LV_PALETTE_BLUE, 4);
    qr = lv_qrcode_create(lv_scr_act(), 260, fg_color, bg_color);

    /*Set data*/
    const char * data = "http://www.bekencorp.com";
    lv_qrcode_update(qr, data, os_strlen(data));
    lv_obj_align(qr, LV_ALIGN_CENTER, 0, 40);

    /*Add a border with bg_color*/
    lv_obj_set_style_border_color(qr, bg_color, 0);
    lv_obj_set_style_border_width(qr, 5, 0);
}

void lvgl_event_open_handle(media_mailbox_msg_t *msg)
{
	os_printf("%s EVENT_LVGL_OPEN_IND \n", __func__);
    lvgl_disp_enable = 1;

	lv_vnd_config_t lv_vnd_config = {0};
	lcd_open_t *lcd_open = (lcd_open_t *)msg->param;

    frame_buffer_t *lv_frame_buffer= frame_buffer_display_malloc(ppi_to_pixel_x(lcd_open->device_ppi) * ppi_to_pixel_y(lcd_open->device_ppi) * sizeof(lv_color_t));
    if (lv_frame_buffer == NULL) {
        os_printf("[%s] lv_frame_buffer malloc fail\r\n", __func__);
        msg_send_rsp_to_media_major_mailbox(msg, BK_FAIL, APP_MODULE);
        return;
    }

    lv_vnd_config.draw_pixel_size = (60 * 1024) / sizeof(lv_color_t);
	lv_vnd_config.draw_buf_2_1 = LV_MEM_CUSTOM_ALLOC(lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
	lv_vnd_config.draw_buf_2_2 = NULL;
	lv_vnd_config.frame_buf_1 = (lv_color_t *)lv_frame_buffer->frame;
	lv_vnd_config.frame_buf_2 = NULL;

    lv_vnd_config.lcd_hor_res = ppi_to_pixel_x(lcd_open->device_ppi);
	lv_vnd_config.lcd_ver_res = ppi_to_pixel_y(lcd_open->device_ppi);
	lv_vnd_config.rotation = ROTATE_NONE;

#if (CONFIG_TP)
	drv_tp_open(ppi_to_pixel_x(lcd_open->device_ppi), ppi_to_pixel_y(lcd_open->device_ppi), TP_MIRROR_NONE);
#endif

	lv_vendor_init(&lv_vnd_config);

    lv_vendor_disp_lock();
    lv_example_qrcode();
    lv_vendor_disp_unlock();

    lv_vendor_start();

    msg_send_rsp_to_media_major_mailbox(msg, BK_OK, APP_MODULE);
}

void lvgl_event_close_handle(media_mailbox_msg_t *msg)
{
    lv_vendor_stop();

    lvgl_disp_enable = 0;

    lv_vendor_deinit();

#if (CONFIG_TP)
    drv_tp_close();
#endif

    os_printf("%s\r\n", __func__);

    msg_send_rsp_to_media_major_mailbox(msg, BK_OK, APP_MODULE);
}

void lvgl_event_lvcam_lvgl_open_handle(media_mailbox_msg_t *msg)
{
    lvgl_disp_enable = 1;

    lv_vendor_start();

//    lv_vendor_disp_lock();
//    lv_example_qrcode();
//    lv_vendor_disp_unlock();

    //if you return to displaying a static image, no need to redraw, otherwise you need to redraw ui.
    lcd_display_frame_request(lvgl_frame_buffer);

    msg_send_rsp_to_media_major_mailbox(msg, BK_OK, APP_MODULE);
}

void lvgl_event_lvcam_lvgl_close_handle(media_mailbox_msg_t *msg)
{
    lv_vendor_stop();
//    lv_qrcode_delete(qr);
    lvgl_disp_enable = 0;
    msg_send_rsp_to_media_major_mailbox(msg, BK_OK, APP_MODULE);
}

void lvgl_event_handle(media_mailbox_msg_t *msg)
{
	switch (msg->event)
	{
		case EVENT_LVGL_OPEN_IND:
			lvgl_event_open_handle(msg);
			break;

        case EVENT_LVGL_CLOSE_IND:
            lvgl_event_close_handle(msg);
            break;

        case EVENT_LVCAM_LVGL_OPEN_IND:
            lvgl_event_lvcam_lvgl_open_handle(msg);
            break;

		case EVENT_LVCAM_LVGL_CLOSE_IND:
			lvgl_event_lvcam_lvgl_close_handle(msg);
			break;

		default:
			break;
	}
}
#endif


#if (CONFIG_SOC_BK7256)
#ifdef CONFIG_CACHE_CUSTOM_SRAM_MAPPING
const unsigned int g_sram_addr_map[4] =
{
	0x38000000,
	0x30020000,
	0x38020000,
	0x30000000
};
#endif
#endif

#if (CONFIG_SYS_CPU0)
void lvcamera_main_init(void)
{
	cli_lvcamera_init();

#if CONFIG_SOC_BK7256
#ifdef CONFIG_LV_USE_DEMO_METER
	bk_err_t ret;
	lv_vnd_config_t lv_vnd_config;

	os_printf("meter Start\r\n");

	lvgl_vfs_init();

#ifdef CONFIG_LVGL_USE_PSRAM
#define PSRAM_DRAW_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)

	lv_vnd_config.draw_pixel_size = ppi_to_pixel_x(lcd_open.device_ppi) * ppi_to_pixel_y(lcd_open.device_ppi);
	lv_vnd_config.draw_buf_2_1 = (lv_color_t *)PSRAM_DRAW_BUFFER;
	lv_vnd_config.draw_buf_2_2 = (lv_color_t *)(PSRAM_DRAW_BUFFER + lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
#else
#define PSRAM_FRAME_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)

	lv_vnd_config.draw_pixel_size = (30 * 1024) / sizeof(lv_color_t);
	lv_vnd_config.draw_buf_2_1 = LV_MEM_CUSTOM_ALLOC(lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
	lv_vnd_config.draw_buf_2_2 = NULL;
	lv_vnd_config.frame_buf_1 = (lv_color_t *)PSRAM_FRAME_BUFFER;
	lv_vnd_config.frame_buf_2 = (lv_color_t *)(PSRAM_FRAME_BUFFER + ppi_to_pixel_x(lcd_open.device_ppi)*ppi_to_pixel_y(lcd_open.device_ppi) * sizeof(lv_color_t));
#endif
    lv_vnd_config.lcd_hor_res = ppi_to_pixel_x(lcd_open.device_ppi);
    lv_vnd_config.lcd_ver_res = ppi_to_pixel_y(lcd_open.device_ppi);
	lv_vnd_config.rotation = ROTATE_NONE;

#if (CONFIG_TP)
	drv_tp_open(ppi_to_pixel_x(lcd_open.device_ppi), ppi_to_pixel_y(lcd_open.device_ppi), TP_MIRROR_NONE);
#endif

	lv_vendor_init(&lv_vnd_config);
	media_app_lcd_rotate(ROTATE_NONE);
	ret = media_app_lcd_open((lcd_open_t *)&lcd_open);
	if (ret != BK_OK)
	{
		os_printf("media_app_lcd_open failed\r\n");
	}

	lv_vendor_start();
    lcd_driver_backlight_open();

	lv_example_meter();
#endif
#else
    bk_err_t ret;

    ret = media_app_lcd_pipeline_disp_open((lcd_open_t *)&lcd_open);
    if (ret != BK_OK) {
        os_printf("media_app_lcd_pipeline_open failed\r\n");
        return;
    }

    ret = media_app_lvgl_open((lcd_open_t *)&lcd_open);
    if (ret != BK_OK) {
        os_printf("media_app_lvgl_draw failed\r\n");
        return;
    }
#endif
}
#endif

void user_app_main(void)
{
#if CONFIG_INTEGRATION_DOORBELL
	doorbell_core_init();
#endif
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

#if (CONFIG_SYS_CPU0)
	lvcamera_main_init();
#endif

	return 0;
}