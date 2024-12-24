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

#include <driver/gpio.h>
#include <driver/media_types.h>
#include <driver/lcd_types.h>
#include <driver/lcd_spi.h>
#include "bk_misc.h"
#include "lcd_panel_devices.h"
#include "gpio_driver.h"
#include <driver/lcd.h>
#include <tkl_display.h>

static lcd_rgb_t tuya_lcd_rgb =
{
	.clk = LCD_30M,
	.data_out_clk_edge = NEGEDGE_OUTPUT,

	.hsync_pulse_width = 3,
	.vsync_pulse_width = 4,
	.hsync_back_porch = 17,
	.hsync_front_porch = 20,
	.vsync_back_porch = 15,
	.vsync_front_porch = 4,
};

static struct lcd_init_s *ty_lcd_init_sequence = NULL;

static void lcd_tuya_common_config(void)
{
    int i = 0;
#define Delay rtos_delay_milliseconds
#define SPI_WriteComm lcd_spi_write_cmd
#define SPI_WriteData lcd_spi_write_data
	Delay(20);

    if (ty_lcd_init_sequence == NULL)
        return;

    for (;;) {
        switch (ty_lcd_init_sequence[i].type) {
            case 0: {
                // reset
                bk_printf("reset %s\r\n", lcd_device_tuya_rgb.name);
                struct lcd_reset_seq *reset = ty_lcd_init_sequence[i].reset;
                int rst_gpio = tkl_display_ll_spi_config(SPI_GPIO_RST);
                for (int r = 0; r < 3; r++) {
                    if (reset[r].gpio_level)
                        bk_gpio_set_output_high(rst_gpio);
                    else
                        bk_gpio_set_output_low(rst_gpio);
                    //delay
                    Delay(reset[r].delay_time);
                }
                break;
            }
            case 1: {
                // set reg value
                struct lcd_reg_set *reg = &ty_lcd_init_sequence[i].reg;
                SPI_WriteComm(reg->r);
                if (reg->len != 0) {
                    for (int l = 0; l < reg->len; l++) {
                        SPI_WriteData(reg->v[l]);
                    }
                }
                bk_printf("set reg: %x\r\n", reg->r);
                break;
            }
            case 2: {
                if (ty_lcd_init_sequence[i].delay_time != 0) {
                    Delay(ty_lcd_init_sequence[i].delay_time);
                }
                break;
            }
            case 3: {
                bk_printf("init %s end\r\n", lcd_device_tuya_rgb.name);
                return;
            }
            default: {
                bk_printf("init %s error, type %d\r\n", lcd_device_tuya_rgb.name, ty_lcd_init_sequence[i].type);
                return;
            }
        }

        i++;
    }

}

static void tuya_common_lcd_init(void)
{
	os_printf("lcd_tuya: init.\r\n");
	lcd_spi_init_gpio();
	lcd_tuya_common_config();
}

lcd_device_t lcd_device_tuya_rgb =
{
	.id = LCD_DEVICE_TUYA_COMMON,
	.name = "tuya_common_lcd",
	.type = LCD_TYPE_RGB,
	.ppi = PPI_480X864,
	.rgb = &tuya_lcd_rgb,
	.out_fmt = PIXEL_FMT_RGB888,
	.init = tuya_common_lcd_init,
	.lcd_off = NULL,
};

int tuya_lcd_update_config(void *config)
{
    bk_printf("entry %s\r\n", __func__);
    if (config == NULL) {
        // need ??
        return -1;
    }

    TKL_DISP_LL_CTRL_S *conf = (TKL_DISP_LL_CTRL_S *)config;
    if (conf->init_param == NULL) {
        // not tuya common lcd driver
        return -1;
    }

    if (conf->init_param->lcd_init_sequence == NULL) {
        bk_printf("init tuya common lcd failed, invalid lcd_init_sequence\r\n");
        return -1;
    }

    uint32_t level = rtos_enter_critical();

    ty_lcd_init_sequence = conf->init_param->lcd_init_sequence;

    if (conf->init_param->type == TUYA_LCD_TYPE_RGB) {
        lcd_device_tuya_rgb.type = LCD_TYPE_RGB;
    } else {
        // TODO SPI / QSPI / MCU
        bk_printf("type %d not support now\r\n", conf->init_param->type);
        goto __tuya_common_error_exit;
    }

    lcd_device_tuya_rgb.ppi = conf->deivce_ppi;

    if (conf->init_param->fmt == TKL_DISP_PIXEL_FMT_RGB888)
        lcd_device_tuya_rgb.out_fmt = PIXEL_FMT_RGB888;
    else if (conf->init_param->fmt == TKL_DISP_PIXEL_FMT_RGB666)
        lcd_device_tuya_rgb.out_fmt = PIXEL_FMT_RGB666;
    else if (conf->init_param->fmt == TKL_DISP_PIXEL_FMT_RGB565)
        lcd_device_tuya_rgb.out_fmt = PIXEL_FMT_RGB565;
    else {
        bk_printf("fmt %d not support now\r\n", lcd_device_tuya_rgb.out_fmt);
        goto __tuya_common_error_exit;
    }

    TY_RGB_CFG_T *rgb = conf->init_param->rgb;
    if (rgb == NULL) {
        bk_printf("rgb parameter is null\r\n");
        goto __tuya_common_error_exit;
    }

    switch (rgb->clk) {
        case TUYA_LCD_80M:
            tuya_lcd_rgb.clk = LCD_80M;
            break;
        case TUYA_LCD_64M:
            tuya_lcd_rgb.clk = LCD_64M;
            break;
        case TUYA_LCD_60M:
            tuya_lcd_rgb.clk = LCD_60M;
            break;
        case TUYA_LCD_54M:
            tuya_lcd_rgb.clk = LCD_54M;
            break;
        case TUYA_LCD_45M: //45.7M
            tuya_lcd_rgb.clk = LCD_45M;
            break;
        case TUYA_LCD_40M:
            tuya_lcd_rgb.clk = LCD_40M;
            break;
        case TUYA_LCD_35M: //35.5
            tuya_lcd_rgb.clk = LCD_35M;
            break;
        case TUYA_LCD_32M:
            tuya_lcd_rgb.clk = LCD_32M;
            break;
        case TUYA_LCD_30M:
            tuya_lcd_rgb.clk = LCD_30M;
            break;
        case TUYA_LCD_26M: //26.6M
            tuya_lcd_rgb.clk = LCD_26M;
            break;
        case TUYA_LCD_24M: //24.6M
            tuya_lcd_rgb.clk = LCD_24M;
            break;
        case TUYA_LCD_22M: //22.85M
            tuya_lcd_rgb.clk = LCD_22M;
            break;
        case TUYA_LCD_20M:
            tuya_lcd_rgb.clk = LCD_20M;
            break;
        case TUYA_LCD_17M: //17.1M
            tuya_lcd_rgb.clk = LCD_17M;
            break;
        case TUYA_LCD_15M:
            tuya_lcd_rgb.clk = LCD_15M;
            break;
        case TUYA_LCD_12M:
            tuya_lcd_rgb.clk = LCD_12M;
            break;
        case TUYA_LCD_10M:
            tuya_lcd_rgb.clk = LCD_10M;
            break;
        case TUYA_LCD_9M:  //9.2M
            tuya_lcd_rgb.clk = LCD_9M;
            break;
        case TUYA_LCD_8M:
            tuya_lcd_rgb.clk = LCD_8M;
            break;
        case TUYA_LCD_7M:
            tuya_lcd_rgb.clk = LCD_7M;
            break;
        default:
            bk_printf("rgb clk not support %d\r\n", tuya_lcd_rgb.clk);
            goto __tuya_common_error_exit;
    }

    if (rgb->active_edge == NEGATIVE_EDGE)
        tuya_lcd_rgb.data_out_clk_edge = NEGEDGE_OUTPUT;
    else
        tuya_lcd_rgb.data_out_clk_edge = POSEDGE_OUTPUT;

    tuya_lcd_rgb.hsync_pulse_width = rgb->hsync_pulse_width;
    tuya_lcd_rgb.vsync_pulse_width = rgb->vsync_pulse_width;
    tuya_lcd_rgb.hsync_back_porch  = rgb->hsync_back_porch;
    tuya_lcd_rgb.hsync_front_porch = rgb->hsync_front_porch;
    tuya_lcd_rgb.vsync_back_porch  = rgb->vsync_back_porch;
    tuya_lcd_rgb.vsync_front_porch = rgb->vsync_front_porch;

    // bk_printf("name: %s\r\n", conf->init_param->name);
    // lcd_device_tuya_rgb.name = conf->init_param->name;
    lcd_device_tuya_rgb.name = conf->ic_name;

    bk_printf("tuya common lcd init complete\r\n");

    rtos_exit_critical(level);
    return 0;

__tuya_common_error_exit:
    bk_printf("tuya common lcd init falied\r\n");
    rtos_exit_critical(level);
    return -1;
}

