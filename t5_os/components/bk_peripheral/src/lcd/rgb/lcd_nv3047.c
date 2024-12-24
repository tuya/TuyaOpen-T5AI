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

static const lcd_rgb_t lcd_rgb =
{
	.clk = LCD_10M,
	.data_out_clk_edge = POSEDGE_OUTPUT,

	.hsync_back_porch = 43,
	.hsync_front_porch = 8,
	.hsync_pulse_width = 4,

	.vsync_back_porch = 12,
	.vsync_front_porch = 8,
	.vsync_pulse_width = 4,
};


static void lcd_nv3047_config(void)
{
    // do nothing
}

static void lcd_nv3047_init(void)
{
	os_printf("lcd_nv3047: init.\r\n");
	lcd_spi_init_gpio();
	lcd_nv3047_config();
}

const lcd_device_t lcd_device_nv3047 =
{
	.name = "nv3047",
	.type = LCD_TYPE_RGB,
	.ppi = PPI_480X272,
	.rgb = &lcd_rgb,
	.src_fmt = PIXEL_FMT_RGB565,
	.out_fmt = PIXEL_FMT_RGB888,
	.init = lcd_nv3047_init,
};


