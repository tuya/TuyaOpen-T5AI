/*
 * test_media.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "cli_tuya_test.h"

#include <os/os.h>
#include "media_app.h"
#include "media_evt.h"
#include "tkl_display.h"
#include "tkl_thread.h"
#include "tkl_audio.h"
#include "tkl_video_in.h"
#include "tkl_lvgl.h"

#include "sys_driver.h"
#include "aud_intf.h"
#include "aud_intf_types.h"
#include <modules/g711.h>

#define DEV_CLOSED      0
#define DEV_OPEN        1

char *__test_ic_name = NULL;

static volatile uint8_t rotate = 3;
static volatile uint8_t __lvgl_status = DEV_CLOSED;

static void __test_app_lcd_component(void);

static void __test_media_get_lcd_info(uint16_t *w, uint16_t *h)
{
    if (__test_ic_name == NULL) {
        bk_printf("%s not init lcd\r\n", __func__);
        return;
    }

    if (!os_strcmp(__test_ic_name, "st7701sn")) {
        *w = 480;
        *h = 854;
    } else if (!os_strcmp(__test_ic_name, "T50P181CQ")) {
        *w = 480;
        *h = 864;
    } else if (!os_strcmp(__test_ic_name, "T35P128CQ")) {
        *w = 320;
        *h = 480;
    } else if (!os_strcmp(__test_ic_name, "nv3047")) {
        *w = 480;
        *h = 272;
    }
    return;
}

static void __test_media_open_uvc(void)
{
    TKL_VI_CONFIG_T vi_config;
    TKL_VI_EXT_CONFIG_T ext_conf;

    ext_conf.type = TKL_VI_EXT_CONF_CAMERA;
    ext_conf.camera.camera_type = TKL_VI_CAMERA_TYPE_UVC;
    ext_conf.camera.fmt = TKL_CODEC_VIDEO_MJPEG;
    ext_conf.camera.power_pin = TUYA_GPIO_NUM_28;
    ext_conf.camera.active_level = TUYA_GPIO_LEVEL_HIGH;

    vi_config.isp.width = 864;
    vi_config.isp.height = 480;
    vi_config.isp.fps = 15;
    vi_config.pdata = &ext_conf;

    tkl_vi_init(&vi_config, 0);
}

static void __test_media_open_dvp(void)
{
    TKL_VI_CONFIG_T vi_config;
    TKL_VI_EXT_CONFIG_T ext_conf;

    ext_conf.type = TKL_VI_EXT_CONF_CAMERA;
    ext_conf.camera.camera_type = TKL_VI_CAMERA_TYPE_DVP;
    ext_conf.camera.fmt = TKL_CODEC_VIDEO_MJPEG;
    ext_conf.camera.power_pin = TUYA_GPIO_NUM_28;
    ext_conf.camera.active_level = TUYA_GPIO_LEVEL_HIGH;
    ext_conf.camera.i2c.clk = TUYA_GPIO_NUM_0;
    ext_conf.camera.i2c.sda = TUYA_GPIO_NUM_1;

    vi_config.isp.width = 864;
    vi_config.isp.height = 480;
    vi_config.isp.fps = 25;
    vi_config.pdata = &ext_conf;

    tkl_vi_init(&vi_config, 0);
}

static void __test_media_open_lcd(uint8_t pipeline)
{
    TKL_DISP_DEVICE_S lcd;
    TKL_DISP_INFO_S info;

    memset(&lcd, 0, sizeof(TKL_DISP_DEVICE_S));
    memset(&info, 0, sizeof(TKL_DISP_INFO_S));

    __test_media_get_lcd_info(&info.width, &info.height);
    info.fps = 15;
    info.format = TKL_DISP_PIXEL_FMT_RGB565;
    // info.rotation = TKL_DISP_ROTATION_270;
    info.rotation = rotate;

    // TODO
    if (pipeline) {
        bk_printf("lcd enable pipeline\r\n");
    } else {
        bk_printf("lcd disable pipeline\r\n");
    }
    info.ll_ctrl.enable_lcd_pipeline = pipeline;

    if (!os_strcmp(__test_ic_name, "st7701sn")) {
        info.ll_ctrl.bl.io              = TUYA_GPIO_NUM_7;
        info.ll_ctrl.bl.mode            = TKL_DISP_BL_GPIO;
        info.ll_ctrl.bl.active_level    = TUYA_GPIO_LEVEL_HIGH;

        info.ll_ctrl.spi.clk            = TUYA_GPIO_NUM_0;
        info.ll_ctrl.spi.csx            = TUYA_GPIO_NUM_12;
        info.ll_ctrl.spi.sda            = TUYA_GPIO_NUM_1;
        info.ll_ctrl.spi.rst_mode       = TKL_DISP_POWERON_RESET;
        info.ll_ctrl.spi.rst            = TUYA_GPIO_NUM_56;

        info.ll_ctrl.power_ctrl_pin     = TUYA_GPIO_NUM_13;     // no lcd ldo
        info.ll_ctrl.power_active_level = TUYA_GPIO_LEVEL_HIGH;
        info.ll_ctrl.rgb_mode           = TKL_DISP_PIXEL_FMT_RGB565;

        info.ll_ctrl.tp.tp_i2c_clk      = TUYA_GPIO_NUM_56;
        info.ll_ctrl.tp.tp_i2c_sda      = TUYA_GPIO_NUM_56;
        info.ll_ctrl.tp.tp_rst          = TUYA_GPIO_NUM_56;
        info.ll_ctrl.tp.tp_intr         = TUYA_GPIO_NUM_56;

        info.ll_ctrl.init_param         = NULL;
    } else if (!os_strcmp(__test_ic_name, "T50P181CQ")) {
        info.ll_ctrl.bl.io              = TUYA_GPIO_NUM_36;
        info.ll_ctrl.bl.mode            = TKL_DISP_BL_GPIO;
        info.ll_ctrl.bl.active_level    = TUYA_GPIO_LEVEL_HIGH;

        info.ll_ctrl.spi.clk            = TUYA_GPIO_NUM_35;
        info.ll_ctrl.spi.csx            = TUYA_GPIO_NUM_34;
        info.ll_ctrl.spi.sda            = TUYA_GPIO_NUM_32;
        info.ll_ctrl.spi.rst_mode       = TKL_DISP_GPIO_RESET;
        info.ll_ctrl.spi.rst            = TUYA_GPIO_NUM_28;

        info.ll_ctrl.power_ctrl_pin     = TUYA_GPIO_NUM_56;     // no lcd ldo
        info.ll_ctrl.power_active_level = TUYA_GPIO_LEVEL_HIGH;
        info.ll_ctrl.rgb_mode           = TKL_DISP_PIXEL_FMT_RGB565;

        info.ll_ctrl.tp.tp_i2c_clk      = TUYA_GPIO_NUM_13;
        info.ll_ctrl.tp.tp_i2c_sda      = TUYA_GPIO_NUM_15;
        info.ll_ctrl.tp.tp_rst          = TUYA_GPIO_NUM_27;
        info.ll_ctrl.tp.tp_intr         = TUYA_GPIO_NUM_38;
    } else if (!os_strcmp(__test_ic_name, "T35P128CQ")) {
        info.ll_ctrl.bl.io              = TUYA_GPIO_NUM_9;
        info.ll_ctrl.bl.mode            = TKL_DISP_BL_GPIO;
        info.ll_ctrl.bl.active_level    = TUYA_GPIO_LEVEL_HIGH;

        info.ll_ctrl.spi.clk            = TUYA_GPIO_NUM_49;
        info.ll_ctrl.spi.csx            = TUYA_GPIO_NUM_48;
        info.ll_ctrl.spi.sda            = TUYA_GPIO_NUM_50;
        info.ll_ctrl.spi.rst_mode       = TKL_DISP_GPIO_RESET;
        info.ll_ctrl.spi.rst            = TUYA_GPIO_NUM_53;

        info.ll_ctrl.power_ctrl_pin     = TUYA_GPIO_NUM_56;     // no lcd ldo
        info.ll_ctrl.power_active_level = TUYA_GPIO_LEVEL_HIGH;
        info.ll_ctrl.rgb_mode           = TKL_DISP_PIXEL_FMT_RGB565;

        info.ll_ctrl.tp.tp_i2c_clk      = TUYA_GPIO_NUM_13;
        info.ll_ctrl.tp.tp_i2c_sda      = TUYA_GPIO_NUM_15;
        info.ll_ctrl.tp.tp_rst          = TUYA_GPIO_NUM_54;
        info.ll_ctrl.tp.tp_intr         = TUYA_GPIO_NUM_55;

        info.ll_ctrl.init_param         = NULL;

        // 拉高 lcd rst 引脚
        TUYA_GPIO_BASE_CFG_T gpio_cfg = {
            .direct = TUYA_GPIO_OUTPUT,
            .mode = TUYA_GPIO_PULLUP,
            .level = TUYA_GPIO_LEVEL_HIGH,
        };
        tkl_gpio_init(TUYA_GPIO_NUM_53, &gpio_cfg);
        tkl_gpio_write(TUYA_GPIO_NUM_53, 1);

    } else if (!os_strcmp(__test_ic_name, "nv3047")) {
        info.fps = 30;
        info.format = TKL_DISP_PIXEL_FMT_RGB888;
        info.rotation = TKL_DISP_ROTATION_180;

        info.ll_ctrl.bl.mode            = TKL_DISP_BL_GPIO;
        info.ll_ctrl.bl.io              = TUYA_GPIO_NUM_32;            //屏幕背光的控制!
        info.ll_ctrl.bl.active_level    = TUYA_GPIO_LEVEL_HIGH;
        info.ll_ctrl.spi.clk            = TUYA_GPIO_NUM_MAX;
        info.ll_ctrl.spi.csx            = TUYA_GPIO_NUM_MAX;
        info.ll_ctrl.spi.sda            = TUYA_GPIO_NUM_MAX;
        info.ll_ctrl.spi.rst_mode       = TUYA_GPIO_NUM_MAX;
        info.ll_ctrl.spi.rst            = TUYA_GPIO_NUM_MAX;
        info.ll_ctrl.power_ctrl_pin     = TUYA_GPIO_NUM_MAX;           //屏幕的电源控制脚!(屏幕没有电源控制脚,就配置成一个无效的脚)
        info.ll_ctrl.power_active_level = TUYA_GPIO_LEVEL_HIGH;
        info.ll_ctrl.rgb_mode           = TKL_DISP_PIXEL_FMT_RGB888;
        //设置TP引脚
        info.ll_ctrl.tp.tp_i2c_clk = TUYA_GPIO_NUM_12;
        info.ll_ctrl.tp.tp_i2c_sda = TUYA_GPIO_NUM_13;
        info.ll_ctrl.tp.tp_rst = TUYA_GPIO_NUM_9;
        info.ll_ctrl.tp.tp_intr = TUYA_GPIO_NUM_8;

        info.ll_ctrl.init_param =NULL;//tdd_lcd_driver_query(TUYA_LCD_IC_NAME);
    } else if (!os_strcmp(__test_ic_name, "ty_t50p181cq")) {
        __test_app_lcd_component();
    }

    memset(info.ll_ctrl.ic_name, 0, IC_NAME_LENGTH);
    int len = (IC_NAME_LENGTH < sizeof(__test_ic_name))? IC_NAME_LENGTH: strlen(__test_ic_name);
    memcpy(info.ll_ctrl.ic_name, __test_ic_name, len);

    bk_printf("----- %d %s %s %d\r\n", len, __test_ic_name, info.ll_ctrl.ic_name, rotate);

    lcd.device_info = &info;

    tkl_disp_init(&lcd, NULL);

    tkl_disp_set_brightness(NULL, 100);
}

static int send_mic_data_to_spk(uint8_t *data, unsigned int len)
{
    bk_err_t ret = BK_OK;

    int16_t *pcm_data = (int16_t *)data;
    uint8_t *g711a_data = NULL;
    g711a_data = os_malloc(len/2);

    /* pcm -> g711a */
    for (int i = 0; i < len/2; i++) {
        g711a_data[i] = linear2alaw(pcm_data[i]);
    }

    /* g711a -> pcm */
    for (int i = 0; i< len/2; i++) {
        pcm_data[i] = alaw2linear(g711a_data[i]);
    }

    /* write a fram speaker data to speaker_ring_buff */
    extern bk_err_t bk_aud_intf_write_spk_data(uint8_t *dac_buff, uint32_t size);
    ret = bk_aud_intf_write_spk_data((uint8 *)pcm_data, len);
    if (ret != BK_OK) {
        os_printf("write mic spk data fail \r\n");
        return ret;
    }

    os_free(g711a_data);
    {
        static SYS_TICK_T last_tick = 0;
        static uint32_t spk_recv_size = 0;

        spk_recv_size += len;
        SYS_TICK_T current = tkl_system_get_tick_count();

        if (current - last_tick > 1000) {
            last_tick = current;
            bk_printf("%s %d, %d, %d\r\n", __func__, __LINE__, last_tick, spk_recv_size);
        }
    }
    return len;
}

static int __test_media_ai_cb(TKL_AUDIO_FRAME_INFO_T *pframe)
{
    static SYS_TICK_T last_tick = 0;
    static uint32_t audio_recv_size = 0;

    audio_recv_size += pframe->buf_size;
    SYS_TICK_T current = tkl_system_get_tick_count();

    if (current - last_tick > 1000) {
        last_tick = current;
        bk_printf("%s %d, %d, %d\r\n", __func__, __LINE__, last_tick, audio_recv_size);
    }

    send_mic_data_to_spk(pframe->pbuf, pframe->buf_size);

    return pframe->buf_size;
}

static void __test_media_open_audio(void)
{
    TKL_AUDIO_CONFIG_T config ={0};
    config.enable = true;
    config.card = TKL_AUDIO_TYPE_BOARD;
    config.ai_chn = 0;
    config.sample = 8000;                      // sample
    config.datebits = 16;                    // datebit
    config.channel = 1;                     // channel num
    config.codectype = TKL_CODEC_AUDIO_PCM;                   // codec type
    config.fps = 25;                         // frame per second，suggest 25
    config.put_cb = __test_media_ai_cb;
    config.spk_gpio = 28;
    tkl_ai_init(&config, 1);
    tkl_ai_start(0,0);
}


void __attribute__((weak)) app_recv_lv_event(uint8_t *buf, uint32_t len, void *args)
{
    os_printf("%s , this function should be defined in app\r\n", __func__);
}

static uint8_t lvgl_state = 0;
static void __test_media_open_lvgl(void)
{
    TKL_DISP_INFO_S info;

    memset(&info, 0, sizeof(TKL_DISP_INFO_S));

    __test_media_get_lcd_info(&info.width, &info.height);
    os_printf("%s , %d %d\r\n", __func__, info.width, info.height);

    int len = (IC_NAME_LENGTH < strlen(__test_ic_name))? IC_NAME_LENGTH: strlen(__test_ic_name);
    memcpy(info.ll_ctrl.ic_name, __test_ic_name, len);

    TKL_LVGL_CFG_T lv_cfg = {
        .recv_cb = app_recv_lv_event,
        .recv_arg = NULL,
    };
    tkl_lvgl_init(&info, &lv_cfg);

    tkl_lvgl_start();

    lvgl_state = 1;
    __lvgl_status = DEV_OPEN;
}

extern void __tuya_lcd_test_func(void);
static void __test_app_lcd_component(void)
{
    // __tuya_lcd_test_func();
}

void cli_tuya_media_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2) {
        bk_printf("Usage: xmt open|close lvgl|uvc|lcd|h264|audio\r\n");
        return;
    }
    bk_printf("argc: %d\r\n cmd: ", argc);
    for (int i = 0; i < argc; i++) {
        bk_printf("%s ", argv[i]);
    }
    bk_printf("\r\n");

    uint32_t tick = xTaskGetTickCount();
    if (tick < 1000) {
        bk_printf("Wait startup complete, ignore\r\n");
        return;
    }

    if ((__test_ic_name == NULL) && (!os_strcmp(argv[1], "lcd"))) {
        bk_printf("set lcd name first\r\n");
        return;
    }

    if (!os_strcmp(argv[1], "set")) {
        if (!os_strcmp(argv[2], "T50P181CQ")) {
            __test_ic_name = "T50P181CQ";
        } else if (!os_strcmp(argv[2], "st7701sn")) {
            __test_ic_name = "st7701sn";
        } else if (!os_strcmp(argv[2], "nv3047")) {
            __test_ic_name = "nv3047";
        } else if (!os_strcmp(argv[2], "T35P128CQ")) {
            __test_ic_name = "T35P128CQ";
        } else {
            bk_printf("not support %s", argv[2]);
            return;
        }
        bk_printf("set ic: %s\r\n", argv[2]);
        return;
    } else if (!os_strcmp(argv[1], "open")) {
        //lpmgr_register(TY_LP_KEEP_ALIVE);
        if (argc == 2) {
            bk_printf("no spec open parameter\r\n");
        } else if (!os_strcmp(argv[2], "uvc")) {
            __test_media_open_uvc();
        } else if (!os_strcmp(argv[2], "dvp")) {
            __test_media_open_dvp();
        } else if (!os_strcmp(argv[2], "lvgl")) {
            if (__lvgl_status != DEV_OPEN) {
                __test_media_open_lvgl();
            }
        } else if (!os_strcmp(argv[2], "lcd")) {
            if (argv[3] == NULL) {
                __test_media_open_lcd(0);
            } else {
                __test_media_open_lcd(1);
            }
        } else if (!os_strcmp(argv[2], "h264")) {
            tkl_venc_init(0, NULL, 0);
        } else if (!os_strcmp(argv[2], "audio")) {
            __test_media_open_audio();
        }
    } else if (!os_strcmp(argv[1], "close")) {
        if (argc == 2) {
            bk_printf("no spec close parameter\r\n");
        } else if (!os_strcmp(argv[2], "uvc")) {
            tkl_vi_uninit(TKL_VI_CAMERA_TYPE_UVC);
        } else if (!os_strcmp(argv[2], "dvp")) {
            tkl_vi_uninit(TKL_VI_CAMERA_TYPE_DVP);
        } else if (!os_strcmp(argv[2], "lvgl")) {
            if (__lvgl_status == DEV_OPEN) {
                tkl_lvgl_stop();
                __lvgl_status = DEV_CLOSED;
            }
        } else if (!os_strcmp(argv[2], "lcd")) {
            if (__lvgl_status == DEV_OPEN) {
                tkl_lvgl_stop();
                __lvgl_status = DEV_CLOSED;
            }
            tkl_disp_set_brightness(NULL, 0);
            tkl_disp_deinit(NULL);
        } else if (!os_strcmp(argv[2], "audio")) {
            tkl_ai_stop(0, 0);
            tkl_ai_uninit();
        } else if (!os_strcmp(argv[2], "h264")) {
            tkl_venc_uninit(0);
        }
        //lpmgr_unregister(TY_LP_KEEP_ALIVE);
    } else if (!os_strcmp(argv[1], "rotate")) {
        uint8_t stat = tkl_disp_get_lcd_state();
        if (stat) {
            rotate++;
            rotate %= 4;

            media_send_msg_sync(EVENT_PIPELINE_SET_ROTATE_IND, rotate);

            tkl_disp_set_brightness(NULL, 0);
            tkl_disp_deinit(NULL);
            tkl_system_sleep(200);
            if (os_strcmp(argv[2] == NULL)) {
                __test_media_open_lcd(0);
            } else {
                __test_media_open_lcd(1);
            }
        } else {
            bk_printf("lcd not init\r\n");
        }
    } else if (!os_strcmp(argv[1], "ac")) {
        __test_app_lcd_component();
    } else {
        bk_printf("Usage: xmt rotate\r\n");
        bk_printf("       xmt set [lcd_name]\r\n");
        bk_printf("       xmt open|close lvgl|uvc|lcd|h264|audio\r\n");
    }
    return;
}




