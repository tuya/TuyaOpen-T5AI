/*
 * tkl_display.c
 * Copyright (C) 2023 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "tkl_display.h"
#include "gpio_driver.h"
#include <driver/gpio.h>
#include <driver/media_types.h>
#include "gpio_map.h"
#include <driver/hal/hal_gpio_types.h>
#include <driver/hal/hal_lcd_types.h>
#include <driver/gpio.h>
#include <driver/pwr_clk.h>
#include <driver/int_types.h>
#include <driver/lcd_types.h>
#include <modules/pm.h>
#include <lcd_act.h>
#include "tkl_mutex.h"
#include "tkl_thread.h"
#include "tkl_semaphore.h"
#include "lcd_decode.h"
#include "lcd_scale.h"
#include "lcd_rotate.h"
#include "frame_buffer.h"
#include "lcd_disp_hal.h"
#include "draw_blend.h"
#include "media_app.h"

#include <driver/lcd_spi.h>

#include "sys_ctrl/sys_driver.h"
#include "tkl_system.h"
#include "tkl_thread.h"
#include "tkl_gpio.h"
#include "bk_misc.h"
#include "driver/lcd.h"
#include "tuya_gpio_map.h"

static uint8_t __lcd_init_flag = 0;

// static TKL_DISP_LL_CTRL_S __disp_ll_conf = {
//     // default config: tuya board
//     .bl = {
//         .mode       = TKL_DISP_BL_GPIO,
//         .io         = TUYA_GPIO_NUM_13,
//         .active_level = TUYA_GPIO_LEVEL_HIGH,
//     },
//
//     .spi = {
//         .clk        = TUYA_GPIO_NUM_44,
//         .csx        = TUYA_GPIO_NUM_46,
//         .sda        = TUYA_GPIO_NUM_45,
//         .rst_mode   = TKL_DISP_POWERON_RESET,
//         .rst        = 56,
//     },
//
//     .power_ctrl_pin = TUYA_GPIO_NUM_28,
//     .power_active_level = TUYA_GPIO_LEVEL_HIGH,
//     .rgb_mode       = TKL_DISP_PIXEL_FMT_RGB565,
//
//     .tp = {
//         .i2c = TUYA_I2C_NUM_0,
//         .rst = TUYA_GPIO_NUM_9,
//         .intr = TUYA_GPIO_NUM_8,
//     }
// };

static TKL_DISP_LL_CTRL_S __disp_ll_conf = {
    // default config: tuya board

    .magic = 0x54555941, // TUYA

    .bl = {
        .mode       = TKL_DISP_BL_GPIO,
        .io         = TUYA_GPIO_NUM_7,
        .active_level = TUYA_GPIO_LEVEL_HIGH,
    },

    .spi = {
        .clk        = TUYA_GPIO_NUM_0,
        .csx        = TUYA_GPIO_NUM_12,
        .sda        = TUYA_GPIO_NUM_1,
        .rst_mode   = TKL_DISP_POWERON_RESET,
        .rst        = 56,
    },

    .power_ctrl_pin = TUYA_GPIO_NUM_13,
    .power_active_level = TUYA_GPIO_LEVEL_HIGH,
    .rgb_mode       = TKL_DISP_PIXEL_FMT_RGB565,

    .deivce_ppi = PPI_480X800,

    .tp = {
        .tp_i2c_clk = TUYA_GPIO_NUM_56,
        .tp_i2c_sda = TUYA_GPIO_NUM_56,
        .tp_rst = TUYA_GPIO_NUM_56,
        .tp_intr = TUYA_GPIO_NUM_56,
    }
};

static void __disp_ll_config_dump(void)
{
    const TKL_DISP_LL_CTRL_S *conf = &__disp_ll_conf;
    bk_printf("cpu%d update disp ll config:\r\n", CONFIG_CPU_INDEX);
    bk_printf("lcd name: %s\r\n", conf->ic_name);
    bk_printf("bl mode: %d, io: %d, active level: %d\r\n",
            conf->bl.mode, conf->bl.io, conf->bl.active_level);
    bk_printf("spi clk: %d\r\n", conf->spi.clk);
    bk_printf("spi csx: %d\r\n", conf->spi.csx);
    bk_printf("spi sda: %d\r\n", conf->spi.sda);
    bk_printf("rst: %d, io: %d\r\n", conf->spi.rst_mode, conf->spi.rst);
    bk_printf("power ctrl: %d, active level: %d\r\n",
            conf->power_ctrl_pin, conf->power_active_level);
    bk_printf("rgb interface: %d\r\n", conf->rgb_mode);
    bk_printf("tp i2c clk: %d sda: %d, rst: %d, intr: %d\r\n",
            conf->tp.tp_i2c_clk, conf->tp.tp_i2c_sda, conf->tp.tp_rst, conf->tp.tp_intr);
    bk_printf("rgb init address: %x\r\n", conf->init_param);
}

int tkl_display_ll_tp_config(int type)
{
    switch (type) {
        case 0:
            return __disp_ll_conf.tp.tp_rst;
        case 1:
            return __disp_ll_conf.tp.tp_intr;
        case 2:
            return __disp_ll_conf.tp.tp_i2c_clk;
        case 3:
            return __disp_ll_conf.tp.tp_i2c_sda;
        default:
            return 56;
    }
}

int tkl_display_ll_spi_config(LCD_SPI_GPIO_TYPE_E gpio_type)
{
    switch(gpio_type) {
        case SPI_GPIO_CLK:
            return __disp_ll_conf.spi.clk;
        case SPI_GPIO_CSX:
            return __disp_ll_conf.spi.csx;
        case SPI_GPIO_SDA:
            return __disp_ll_conf.spi.sda;
        case SPI_GPIO_RST:
            if (__disp_ll_conf.spi.rst_mode == TKL_DISP_POWERON_RESET) {
                return 56;  // invalid io
            } else if (__disp_ll_conf.spi.rst_mode == TKL_DISP_GPIO_RESET) {
                return __disp_ll_conf.spi.rst;
            }
        default:
            break;
    }

    return 56;
}

int tkl_display_rgb_mode(void)
{
    return __disp_ll_conf.rgb_mode;
}

int tkl_display_bl_mode(void)
{
    return __disp_ll_conf.bl.mode;
}

OPERATE_RET tkl_display_bl_ctrl_io(uint8_t *io, uint8_t *active_level)
{
    *io = __disp_ll_conf.bl.io;
    *active_level = __disp_ll_conf.bl.active_level;
    return 0;
}

OPERATE_RET tkl_display_power_ctrl_pin(uint8_t *io, uint8_t *active_level)
{
    *io = __disp_ll_conf.power_ctrl_pin;
    *active_level = __disp_ll_conf.power_active_level;
    return 0;
}

void tkl_disp_update_ll_config(void *config)
{
    if (config == NULL)
        return;

    TKL_DISP_LL_CTRL_S *conf = (TKL_DISP_LL_CTRL_S *)config;

//    memcpy(&__disp_ll_conf, conf, sizeof(TKL_DISP_LL_CTRL_S));
    __disp_ll_conf.bl.mode = conf->bl.mode;
    __disp_ll_conf.bl.io = conf->bl.io;
    __disp_ll_conf.bl.active_level = conf->bl.active_level;

    __disp_ll_conf.spi.clk = conf->spi.clk;
    __disp_ll_conf.spi.csx = conf->spi.csx;
    __disp_ll_conf.spi.sda = conf->spi.sda;
    __disp_ll_conf.spi.rst = conf->spi.rst;
    __disp_ll_conf.spi.rst_mode = conf->spi.rst_mode;

    __disp_ll_conf.power_ctrl_pin = conf->power_ctrl_pin;
    __disp_ll_conf.power_active_level = conf->power_active_level;

    __disp_ll_conf.rgb_mode = conf->rgb_mode;

    memcpy(__disp_ll_conf.ic_name, conf->ic_name, IC_NAME_LENGTH);

    __disp_ll_conf.deivce_ppi = conf->deivce_ppi;

    __disp_ll_conf.tp.tp_i2c_clk = conf->tp.tp_i2c_clk;
    __disp_ll_conf.tp.tp_i2c_sda = conf->tp.tp_i2c_sda;
    __disp_ll_conf.tp.tp_rst = conf->tp.tp_rst;
    __disp_ll_conf.tp.tp_intr = conf->tp.tp_intr;

    __disp_ll_conf.init_param = conf->init_param;

    __disp_ll_conf.magic = 0x54555941;                     // TUYA 0x54555941

    __disp_ll_config_dump();

}

uint32_t tkl_disp_get_ppi(void)
{
    return __disp_ll_conf.deivce_ppi;
}

char *tkl_disp_get_lcd_name(void)
{
    return (char *)__disp_ll_conf.ic_name;
}

uint32_t tkl_disp_get_lcd_state(void)
{
    return __lcd_init_flag;
}

/**
 * @brief Init and config display device
 *
 * @param display_device display device
 * @param cfg display configuration
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_init(TKL_DISP_DEVICE_S *display_device, TKL_DISP_EVENT_HANDLER_S *event_handler)
{
    TUYA_GPIO_BASE_CFG_T cfg;
    uint8_t lcd_ldo, lcd_bl, active_level;
    TKL_DISP_INFO_S *info = (TKL_DISP_INFO_S *)display_device->device_info;

    if (info == NULL)
        return OPRT_INVALID_PARM;

    tkl_disp_update_ll_config(&info->ll_ctrl);

    info->ll_ctrl.deivce_ppi = (info->width << 16) | info->height;

    bk_printf("dev: %s, ppi: %x\r\n", info->ll_ctrl.ic_name, info->ll_ctrl.deivce_ppi);

    //tuya_multimedia_power_on();

    cfg.direct = TUYA_GPIO_OUTPUT;
    tkl_display_power_ctrl_pin(&lcd_ldo, &active_level);
    cfg.level = active_level;
    tkl_gpio_init(lcd_ldo, &cfg);

    if (tkl_display_bl_mode() == TKL_DISP_BL_GPIO) {
        tkl_display_bl_ctrl_io(&lcd_bl, &active_level);
        cfg.level = (active_level == TUYA_GPIO_LEVEL_LOW)? TUYA_GPIO_LEVEL_HIGH: TUYA_GPIO_LEVEL_LOW;
        tkl_gpio_init(lcd_bl, &cfg);
    }

    media_app_lcd_fmt(PIXEL_FMT_RGB565_LE);

    media_rotate_t rotate = ROTATE_90;
    switch (info->rotation) {
        case TKL_DISP_ROTATION_0:
            rotate = ROTATE_NONE;
            break;
        case TKL_DISP_ROTATION_90:
            rotate = ROTATE_90;
            break;
        case TKL_DISP_ROTATION_180:
            rotate = ROTATE_180;
            break;
        case TKL_DISP_ROTATION_270:
            rotate = ROTATE_270;
            break;
        default:
            rotate = ROTATE_90;
            break;
    }


    info->ll_ctrl.magic = 0x54555941; // TUYA
    if (info->ll_ctrl.enable_lcd_pipeline) {
        media_app_pipline_set_rotate(rotate);
        media_app_lcd_pipeline_open(&info->ll_ctrl);
    } else {
        media_app_lcd_rotate(rotate);
        media_app_lcd_open(&info->ll_ctrl);
    }

    __lcd_init_flag = 1;
    return OPRT_OK;
}

/**
 * @brief Release display device
 *
 * @param display_device display device
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_deinit(TKL_DISP_DEVICE_S *display_device)
{
	TKL_DISP_INFO_S *info = (TKL_DISP_INFO_S *)display_device->device_info;
    __lcd_init_flag = 0;
    tkl_disp_set_brightness(NULL, 0);

    // TKL_DISP_BLEND_INFO_S cfg;
    // cfg.type = TKL_DISP_BLEND_ALL;
    // tkl_disp_cancel_blend_info(NULL, &cfg);
    // tkl_system_sleep(100);

    if (info->ll_ctrl.enable_lcd_pipeline) {
        media_app_lcd_pipeline_close();
    } else {
        media_app_lcd_close();
    }

    return OPRT_OK;
}


/**
 * @brief Set display info
 *
 * @param display_device display device
 * @param info display device info
 * @return OPERATE_RET
 */
OPERATE_RET tkl_disp_set_info(TKL_DISP_DEVICE_S *display_device, TKL_DISP_INFO_S *info)
{
    return OPRT_NOT_SUPPORTED;
}


/**
 * @brief Get display info
 *
 * @param display_device display device
 * @param info display device info
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_get_info(TKL_DISP_DEVICE_S *display_device, TKL_DISP_INFO_S *info)
{
    return OPRT_OK;
}


/**
 * @brief
 *
 * @param display_device display device
 * @param buf framebuffer
 * @param rect destination area
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_blit(TKL_DISP_DEVICE_S *display_device, TKL_DISP_FRAMEBUFFER_S *buf, TKL_DISP_RECT_S *rect)
{
    return OPRT_NOT_SUPPORTED;
}


/**
 * @brief Fill destination area with color
 *
 * @param display_device display device
 * @param rect destination area to fill
 * @param color color to fill
 * @return OPERATE_RET
 */
OPERATE_RET tkl_disp_fill(TKL_DISP_DEVICE_S *display_device, TKL_DISP_RECT_S *rect, TKL_DISP_COLOR_U color)
{
    return OPRT_NOT_SUPPORTED;
}


/**
 * @brief Flush buffers to display device
 *
 * @param display_device display device
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_flush(TKL_DISP_DEVICE_S *display_device)
{
    return OPRT_OK;
}


/**
 * @brief Wait for vsync signal
 *
 * @param display_device display device
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_wait_vsync(TKL_DISP_DEVICE_S *display_device)
{
    return OPRT_NOT_SUPPORTED;
}


/**
 * @brief Set display brightness(Backlight or HSB)
 *
 * @param display_device display device
 * @param brightness brightness
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_set_brightness(TKL_DISP_DEVICE_S *display_device, int brightness)
{
    if (__disp_ll_conf.bl.mode == TKL_DISP_BL_GPIO) {
        bk_printf("----- %s b: %d\r\n", __func__, brightness);
        if (brightness) {
            tkl_gpio_write(__disp_ll_conf.bl.io, __disp_ll_conf.bl.active_level);
        }
        else {
            int expect = (__disp_ll_conf.bl.active_level == TUYA_GPIO_LEVEL_HIGH)? TUYA_GPIO_LEVEL_LOW: TUYA_GPIO_LEVEL_HIGH;
            tkl_gpio_write(__disp_ll_conf.bl.io, expect);
        }
        return OPRT_OK;
    }

    return OPRT_NOT_SUPPORTED;
}


/**
 * @brief Get display brightness(Backlight or HSB)
 *
 * @param display_device display device
 * @param brightness brightness
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_get_brightness(TKL_DISP_DEVICE_S *display_device, int *brightness)
{
    return OPRT_NOT_SUPPORTED;
}


/**
 * @brief Sets the display screen's power state
 *
 * @param display_device display device
 * @param power_mode power state
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_set_power_mode(TKL_DISP_DEVICE_S *display_device, TKL_DISP_POWER_MODE_E power_mode)
{
    return OPRT_OK;
}


/**
 * @brief Gets the display screen's power state
 *
 * @param display_device display device
 * @param power_mode power state
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_get_power_mode(TKL_DISP_DEVICE_S *display_device, TKL_DISP_POWER_MODE_E *power_mode)
{
    return OPRT_OK;
}



/**
 * @brief Alloc mapped framebuffer or layer
 *
 * @param display_device display device
 * @return void* Pointer to mapped memory
 */
TKL_DISP_FRAMEBUFFER_S *tkl_disp_alloc_framebuffer(TKL_DISP_DEVICE_S *display_device)
{
    return NULL;
}


/**
 * @brief Free mapped framebuffer or layer
 *
 * @param display_device display device
 * @param buf Pointer to mapped memory
 * @return void
 */
void tkl_disp_free_framebuffer(TKL_DISP_DEVICE_S *display_device, TKL_DISP_FRAMEBUFFER_S *buf)
{
    return;
}

/**
 * @brief Get capabilities supported by display(For external display device. e.g. HDMI/VGA)
 *
 * @param display_device display device
 * @param cfg configurations
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_get_capabilities(TKL_DISP_DEVICE_S *display_device, TKL_DISP_INFO_S **cfg)
{
    return OPRT_OK;
}


/**
 * @brief Free capabilities get by tkl_disp_get_capabilities()
 *
 * @param display_device display device
 * @param cfg configurations
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_free_capabilities(TKL_DISP_DEVICE_S *display_device, TKL_DISP_INFO_S *cfg)
{
    return OPRT_OK;
}

/**
 * @brief Set lcd blend info
 *
 * @param display_device display device
 * @param cfg configurations
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_set_blend_info(TKL_DISP_DEVICE_S *display_device, TKL_DISP_BLEND_INFO_S *cfg)
{
    lcd_blend_msg_t blend = {0} ;

    if (cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    uint8_t type = cfg->type;
    if (type > TKL_DISP_BLEND_TIME)
        return OPRT_INVALID_PARM;

    blend.blend_on = 1;
    switch (type) {
        case TKL_DISP_BLEND_WIFI:
            blend.lcd_blend_type = LCD_BLEND_WIFI;
            // 0 not connect
            // TODO xxx
            if (cfg->data[0] == 127) {
                blend.data[0] = 0;
            } else if (cfg->data[0] > -40) {
                blend.data[0] = 4;
            } else if (cfg->data[0] > -50) {
                blend.data[0] = 3;
            } else if (cfg->data[0] > -60) {
                blend.data[0] = 2;
            } else if (cfg->data[0] > -70) {
                blend.data[0] = 1;
            } else {
                blend.data[0] = 1;
            }
            bk_printf("wifi rssi set %d %d\r\n", cfg->data[0], blend.data[0]);
            break;
        case TKL_DISP_BLEND_VERSION:
            blend.lcd_blend_type = LCD_BLEND_VERSION;
            os_memcpy(blend.data, cfg->data, sizeof(cfg->data));
            bk_printf("version: %s\r\n", blend.data);
            break;
        case TKL_DISP_BLEND_TIME:
            blend.lcd_blend_type = LCD_BLEND_TIME;
            os_memcpy(blend.data, cfg->data, sizeof(cfg->data));
            break;
        case TKL_DISP_BLEND_DATA:
            blend.lcd_blend_type = LCD_BLEND_DATA;
            os_memcpy(blend.data, cfg->data, sizeof(cfg->data));
            bk_printf("version: %s\r\n", blend.data);
            break;
        default:
            bk_printf("unknow param %d\r\n", type);
            return OPRT_INVALID_PARM;
    }

    media_app_lcd_blend(&blend);

    return OPRT_OK;
}

OPERATE_RET tkl_disp_cancel_blend_info(TKL_DISP_DEVICE_S *display_device, TKL_DISP_BLEND_INFO_S *cfg)
{
    lcd_blend_msg_t blend = {0} ;
    if (cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    uint8_t type = cfg->type;
    if (type > TKL_DISP_BLEND_TIME)
        return OPRT_INVALID_PARM;

    blend.blend_on = 0;
    switch (type) {
        case TKL_DISP_BLEND_WIFI:
            blend.lcd_blend_type = LCD_BLEND_WIFI;
            break;
        case TKL_DISP_BLEND_VERSION:
            blend.lcd_blend_type = LCD_BLEND_VERSION;
            break;
        case TKL_DISP_BLEND_TIME:
            blend.lcd_blend_type = LCD_BLEND_TIME;
            break;
        case TKL_DISP_BLEND_DATA:
            blend.lcd_blend_type = LCD_BLEND_DATA;
            break;
        case TKL_DISP_BLEND_ALL:
            blend.lcd_blend_type = LCD_BLEND_DATA | LCD_BLEND_WIFI | LCD_BLEND_VERSION | LCD_BLEND_TIME;
            break;
        default:
            bk_printf("unknow param %d\r\n", type);
            return OPRT_INVALID_PARM;
    }
    media_app_lcd_blend(&blend);

    return 0;
}

volatile uint8_t display_startup_image = 0;
OPERATE_RET tkl_disp_open_startup_image(TKL_DISP_DEVICE_S *display_device, uint32_t address)
{
    if (display_startup_image) {
        return 0;
    }
    display_startup_image = 1;
    media_app_lcd_startup_frame_open((void *)address);
}

OPERATE_RET tkl_disp_close_startup_image(TKL_DISP_DEVICE_S *display_device)
{
    media_app_lcd_startup_frame_close();
    display_startup_image = 0;
    return 0;
}


