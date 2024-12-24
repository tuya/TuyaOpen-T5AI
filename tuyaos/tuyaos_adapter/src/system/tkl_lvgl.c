/*
 * tkl_lvgl.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "sdkconfig.h"
#include "tkl_lvgl.h"
#include "tkl_display.h"
#include "tkl_ipc.h"

#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>

#include "lcd_act.h"
#include "media_app.h"
#include "media_evt.h"

#include "driver/drv_tp.h"
#include <driver/lcd.h>
#include "media_service.h"

#include <lcd_act.h>
#include <tkl_memory.h>
#include <tkl_fs.h>

static lcd_open_t __tkl_lvgl_lcd_info =
{
    .device_ppi = PPI_DEFAULT,
    .device_name = NULL,
};

static TKL_LVGL_CFG_T lvgl_cb = {
    .recv_cb = NULL,
    .recv_arg = NULL,
};

static TKL_IPC_HANDLE g_lvgl_handle = NULL;
static volatile uint8_t g_cpu0_lvgl_inited = 0;
static volatile uint8_t g_cpu0_lvgl_start = 0;

OPERATE_RET tkl_lvgl_ipc_func_cb(TKL_IPC_HANDLE handle, uint8_t *buf, uint32_t buf_len)
{
    if (lvgl_cb.recv_cb != NULL) {
        lvgl_cb.recv_cb(buf, buf_len, lvgl_cb.recv_arg);
    }
    return OPRT_OK;
}

static int __tkl_lvgl_ipc_init(void)
{
    bk_printf("lvgl ipc init\r\n");

    int mipc_status = 0, cnt = 0;

#define IPC_WAIT_TIME 2000
#define IPC_STATUS_CHK_INTERVAL 50
    int max_cnt = IPC_WAIT_TIME / IPC_STATUS_CHK_INTERVAL;

    do {
        mipc_status = media_ipc_status();
        if ((mipc_status != 0) || (cnt > max_cnt)) {
            break;
        }
        cnt ++;
        tkl_system_sleep(IPC_STATUS_CHK_INTERVAL);
    } while(1);

    if ((mipc_status == 0) && (cnt >= max_cnt)) {
        bk_printf("media ipc not init done, please retry\r\n");
        return -1;
    }

    // init ipc
    // TKL_IPC_CONF_T ipc_conf;
    // ipc_conf.cb = tkl_lvgl_ipc_func_cb;
    // uint8_t ipc_cnt = 0;
    // tkl_ipc_init(&ipc_conf, &g_lvgl_handle, &ipc_cnt);
    return 0;
}
/**
 * @brief lvgl init
 *
 * @param[in] cfg configure
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_lvgl_init(TKL_DISP_INFO_S *disp_info, TKL_LVGL_CFG_T *cfg)
{
    if (disp_info == NULL || cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (__tkl_lvgl_lcd_info.device_name == NULL) {
        __tkl_lvgl_lcd_info.device_name = (char *)tkl_system_malloc(IC_NAME_LENGTH);
        if (__tkl_lvgl_lcd_info.device_name == NULL) {
            bk_printf("malloc failed, lvgl init failed\r\n");
            return OPRT_MALLOC_FAILED;
        }
    }

    __tkl_lvgl_lcd_info.device_ppi = (disp_info->width << 16) | disp_info->height; // PPI_480X854;
    memcpy(__tkl_lvgl_lcd_info.device_name, disp_info->ll_ctrl.ic_name, IC_NAME_LENGTH);

    bk_printf("cpu%d lvgl init: %x %s\r\n", CONFIG_CPU_INDEX, __tkl_lvgl_lcd_info.device_ppi, __tkl_lvgl_lcd_info.device_name);
    lvgl_cb.recv_cb = cfg->recv_cb;
    lvgl_cb.recv_arg = cfg->recv_arg;

    g_cpu0_lvgl_inited = 1;

    // init ipc
    // TKL_IPC_CONF_T ipc_conf;
    // ipc_conf.cb = tkl_lvgl_ipc_func_cb;
    // uint8_t cnt = 0;
    // tkl_ipc_init(&ipc_conf, &g_lvgl_handle, &cnt);

    return OPRT_OK;
}

/**
 * @brief lvgl message sync
 *
 * @param[in] msg message
 * @param[in] length
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
extern TKL_IPC_HANDLE __ipc_handle[2];
OPERATE_RET tkl_lvgl_msg_sync(uint8_t *msg, uint32_t length)
{

    OPERATE_RET ret = tkl_ipc_send(__ipc_handle[0], msg, length);
    if (ret != OPRT_OK) {
        bk_printf("Error: lvgl send failed, %d\n", ret);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}


#if (CONFIG_SYS_CPU1)
#include "media_app.h"
#include "media_evt.h"
#include "lvgl.h"
#include "lv_vendor.h"
#include "driver/drv_tp.h"
#include "media_service.h"
#include "frame_buffer.h"
#include "yuv_encode.h"
#include "driver/media_types.h"

extern uint8_t lvgl_disp_enable;
extern lv_vnd_config_t vendor_config;
extern frame_buffer_t *lvgl_frame_buffer;

extern void tuya_gui_main(void);
extern void tuya_gui_pause(void);
extern void tuya_gui_resume(void);

#if 0
void __attribute__((weak)) tuya_gui_main(void)
{
    os_printf("%s , this function should be defined in app\r\n", __func__);
}

void __attribute__((weak)) tuya_gui_pause(void)
{
    os_printf("%s , this function should be defined in app\r\n", __func__);
}

void __attribute__((weak)) tuya_gui_resume(void)
{
    os_printf("%s , this function should be defined in app\r\n", __func__);
}
#endif

volatile uint8_t __is_lvgl_inited = 0;
void __tkl_lvgl_init(media_mailbox_msg_t *msg, lv_vnd_config_t *lv_vnd_config)
{
    extern media_rotate_t uvc_pipeline_get_rotate(void);

    // init fs
    // TODO: 在 cpu0 挂载文件系统
    //tkl_fs_mount("/", DEV_INNER_FLASH);

    // lvgl buffer
    lcd_open_t *lcd_open = (lcd_open_t *)msg->param;

    lv_vnd_config->draw_pixel_size = (60 * 1024) / sizeof(lv_color_t);
    lv_vnd_config->draw_buf_2_1 = LV_MEM_CUSTOM_ALLOC(lv_vnd_config->draw_pixel_size * sizeof(lv_color_t));
    lv_vnd_config->draw_buf_2_2 = NULL;
    lv_vnd_config->frame_buf_1 = (lv_color_t *)psram_malloc(ppi_to_pixel_x(lcd_open->device_ppi) * ppi_to_pixel_y(lcd_open->device_ppi) * sizeof(lv_color_t));
    lv_vnd_config->frame_buf_2 = NULL;

    lv_vnd_config->lcd_hor_res = ppi_to_pixel_x(lcd_open->device_ppi);
    lv_vnd_config->lcd_ver_res = ppi_to_pixel_y(lcd_open->device_ppi);
    lv_vnd_config->rotation = uvc_pipeline_get_rotate();

    bk_printf("lvgl init: \r\n");
    bk_printf("\tx: %d, y: %d, color: %d, rotate: %d\r\n",
            ppi_to_pixel_x(lcd_open->device_ppi),
            ppi_to_pixel_y(lcd_open->device_ppi),
            sizeof(lv_color_t), lv_vnd_config->rotation);

    bk_printf("\tdraw buf1 %x, buf2 %x, size %d\r\n",
            lv_vnd_config->draw_buf_2_1,
            lv_vnd_config->draw_buf_2_2,
            lv_vnd_config->draw_pixel_size);

    bk_printf("\tframe buf1 %x, buf2 %x\r\n",
            lv_vnd_config->frame_buf_1,
            lv_vnd_config->frame_buf_2);

#if (CONFIG_TP)
    drv_tp_open(ppi_to_pixel_x(lcd_open->device_ppi), ppi_to_pixel_y(lcd_open->device_ppi), TP_MIRROR_NONE);
#endif

    __is_lvgl_inited = 1;
}

void lvgl_event_open_handle(media_mailbox_msg_t *msg)
{
    lvgl_disp_enable = 1;

    if (!__is_lvgl_inited) {

        lv_vnd_config_t lv_vnd_config = {0};

        __tkl_lvgl_init(msg, &lv_vnd_config);

        // lvgl vendor adapter init
        lv_vendor_init(&lv_vnd_config);

        // font
#if CONFIG_FREETYPE
        //    extern int ttf_init(void);
        //    if(BK_OK != ttf_init()) {
        //        bk_printf("------ttf init fail------------\r\n");
        //        msg_send_rsp_to_media_major_mailbox(msg, BK_OK, APP_MODULE);
        //        return ;
        //    }
#endif

        //    img_pre_decode();

        lv_vendor_disp_lock();
        tuya_gui_main();
        lv_vendor_disp_unlock();

        lv_vendor_start();

    }
    else
    {
        lv_vendor_start();

        tuya_gui_resume();

        // TODO
        //if you return to displaying a static image, no need to redraw, otherwise you need to redraw ui.
        //lcd_display_frame_request(lvgl_frame_buffer);
    }
    msg_send_rsp_to_media_major_mailbox(msg, BK_OK, APP_MODULE);
}

void lvgl_event_close_handle(media_mailbox_msg_t *msg)
{
    bk_printf("------ [%s %d]", __func__, __LINE__);

    if (lvgl_disp_enable) {
        lv_vendor_stop();

        lvgl_disp_enable = 0;

        /* 切换lvgl到uvc的时候，需要根据业务需要处理当前lvgl业务的状态，
           有定时器或者其他占用资源的业务流程需要stop或者销毁，
           同时在从uvc切回lvgl的时候，相应的业务逻辑需要重新resume或者restart
           */

        tuya_gui_pause();
    }

//    lv_vendor_deinit();
//
//#if (CONFIG_TP)
//    drv_tp_close();
//#endif

    msg_send_rsp_to_media_major_mailbox(msg, BK_OK, APP_MODULE);
    bk_printf("------ [%s %d]", __func__, __LINE__);
}

// ********************************************************
//  for test start
// ********************************************************
void lvgl_event_lvcam_lvgl_open_handle(media_mailbox_msg_t *msg)
{
    os_printf("%s\r\n", __func__);
    lvgl_disp_enable = 1;

    lv_vendor_start();

    lv_vendor_disp_lock();
    extern void tuya_gui_main(void);
    tuya_gui_main();
    lv_vendor_disp_unlock();

    //if you return to displaying a static image, no need to redraw, otherwise you need to redraw ui.
    lcd_display_frame_request(lvgl_frame_buffer);

    msg_send_rsp_to_media_major_mailbox(msg, BK_OK, APP_MODULE);
}

void lvgl_event_lvcam_lvgl_close_handle(media_mailbox_msg_t *msg)
{
    os_printf("%s\r\n", __func__);
    lv_vendor_stop();
    lvgl_disp_enable = 0;
    msg_send_rsp_to_media_major_mailbox(msg, BK_OK, APP_MODULE);
}
// ********************************************************
//  for test end
// ********************************************************

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

#elif (CONFIG_SYS_CPU0)
/**
 * @brief lvgl start
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_lvgl_start(void)
{
    if ((g_cpu0_lvgl_inited == 0) || (__tkl_lvgl_lcd_info.device_ppi == PPI_DEFAULT)) {
        bk_printf("not init\r\n");
        return OPRT_COM_ERROR;
    }
    // send lvgl open event to cpu1
    media_app_lvgl_open((lcd_open_t *)&__tkl_lvgl_lcd_info);

    g_cpu0_lvgl_start = 1;
    return OPRT_OK;
}

/**
 * @brief lvgl stop
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_lvgl_stop(void)
{
    if (g_cpu0_lvgl_start == 1) {
        media_app_lvgl_close();
        g_cpu0_lvgl_start = 0;
    }
    return OPRT_OK;
}

/**
 * @brief lvgl uninit
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_lvgl_uninit(void)
{
    // TODO
    return OPRT_OK;
}

#endif // CONFIG_SYS_CPU0



