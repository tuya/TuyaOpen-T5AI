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
	.clk = LCD_26M,
	.data_out_clk_edge = NEGEDGE_OUTPUT,

	.hsync_pulse_width = 6,
	.hsync_back_porch = 20,
	.hsync_front_porch = 20,
	.vsync_pulse_width = 6,
	.vsync_back_porch = 20,
	.vsync_front_porch = 20,
};

static void lcd_gc9503_boe_config(void)
{
	#define Delay delay_ms
	#define SPI_WriteComm lcd_spi_write_cmd
	#define SPI_WriteData lcd_spi_write_data

	// Delay(10);
	// bk_gpio_set_output_low(LCD_SPI_RST);
	// Delay(10);
	// bk_gpio_set_output_high(LCD_SPI_RST);
	// Delay(10);

	bk_gpio_set_output_high(LCD_SPI_RST);
	delay_ms(15);
	bk_gpio_set_output_low(LCD_SPI_RST);
	delay_ms(20);
	bk_gpio_set_output_high(LCD_SPI_RST);
	delay_ms(50);

	SPI_WriteComm(0xF0);SPI_WriteData(0x55);SPI_WriteData(0xAA);SPI_WriteData(0x52);SPI_WriteData(0x08);SPI_WriteData(0x00);
	SPI_WriteComm(0xF6);SPI_WriteData(0x5A);SPI_WriteData(0x87);
	SPI_WriteComm(0xC1);SPI_WriteData(0x3F);
	SPI_WriteComm(0xCD);SPI_WriteData(0x25);
	SPI_WriteComm(0xC9);SPI_WriteData(0x10);
	SPI_WriteComm(0xF8);SPI_WriteData(0x8A);
	SPI_WriteComm(0xAC);SPI_WriteData(0x45);
	SPI_WriteComm(0xA7);SPI_WriteData(0x47);
	SPI_WriteComm(0xA0);SPI_WriteData(0xDD);
	SPI_WriteComm(0x86);SPI_WriteData(0x99);SPI_WriteData(0xA3);SPI_WriteData(0xA3);SPI_WriteData(0x01);
	SPI_WriteComm(0x86);SPI_WriteData(0x08);SPI_WriteData(0x08);SPI_WriteData(0x00);SPI_WriteData(0x04);
	SPI_WriteComm(0xA3);SPI_WriteData(0x6E);
	SPI_WriteComm(0xFD);SPI_WriteData(0x28);SPI_WriteData(0x3C);SPI_WriteData(0x00);
	SPI_WriteComm(0x9A);SPI_WriteData(0xee);
	SPI_WriteComm(0x9B);SPI_WriteData(0x35);
	SPI_WriteComm(0x82);SPI_WriteData(0x42);SPI_WriteData(0x42);
	SPI_WriteComm(0xB1);SPI_WriteData(0x10);
	SPI_WriteComm(0x7A);SPI_WriteData(0x0F);SPI_WriteData(0x13);
	SPI_WriteComm(0x7B);SPI_WriteData(0x0F);SPI_WriteData(0x13);
	SPI_WriteComm(0x6D);
		SPI_WriteData(0x1E);SPI_WriteData(0x00);SPI_WriteData(0x09);SPI_WriteData(0x0F);SPI_WriteData(0x01);SPI_WriteData(0x1f);SPI_WriteData(0x1E);SPI_WriteData(0x1E);
		SPI_WriteData(0x1E);SPI_WriteData(0x1E);SPI_WriteData(0x1E);SPI_WriteData(0x1E);SPI_WriteData(0x1E);SPI_WriteData(0x1E);SPI_WriteData(0x1E);SPI_WriteData(0x1E);
		SPI_WriteData(0x1E);SPI_WriteData(0x1E);SPI_WriteData(0x1E);SPI_WriteData(0x1E);SPI_WriteData(0x1E);SPI_WriteData(0x1E);SPI_WriteData(0x1E);SPI_WriteData(0x1E);
		SPI_WriteData(0x1E);SPI_WriteData(0x1E);SPI_WriteData(0x1f);SPI_WriteData(0x08);SPI_WriteData(0x10);SPI_WriteData(0x0A);SPI_WriteData(0x00);SPI_WriteData(0x1E);
	SPI_WriteComm(0x64);
		SPI_WriteData(0x18);SPI_WriteData(0x07);SPI_WriteData(0x03);SPI_WriteData(0x5d);SPI_WriteData(0x03);SPI_WriteData(0x03);SPI_WriteData(0x18);SPI_WriteData(0x06);
		SPI_WriteData(0x03);SPI_WriteData(0x5c);SPI_WriteData(0x03);SPI_WriteData(0x03);SPI_WriteData(0x7A);SPI_WriteData(0x7A);SPI_WriteData(0x7A);SPI_WriteData(0x7A);
	SPI_WriteComm(0x67);
		SPI_WriteData(0x18);SPI_WriteData(0x05);SPI_WriteData(0x03);SPI_WriteData(0x5b);SPI_WriteData(0x03);SPI_WriteData(0x03);SPI_WriteData(0x18);SPI_WriteData(0x04);
		SPI_WriteData(0x03);SPI_WriteData(0x5a);SPI_WriteData(0x03);SPI_WriteData(0x03);SPI_WriteData(0x7A);SPI_WriteData(0x7A);SPI_WriteData(0x7A);SPI_WriteData(0x7A);
	SPI_WriteComm(0x60);
		SPI_WriteData(0x18);SPI_WriteData(0x08);SPI_WriteData(0x7A);SPI_WriteData(0x7A);SPI_WriteData(0x18);SPI_WriteData(0x08);SPI_WriteData(0x7A);SPI_WriteData(0x7A);
	SPI_WriteComm(0x63);
		SPI_WriteData(0x18);SPI_WriteData(0x07);SPI_WriteData(0x7A);SPI_WriteData(0x7A);SPI_WriteData(0x18);SPI_WriteData(0x07);SPI_WriteData(0x7A);SPI_WriteData(0x7A);
	SPI_WriteComm(0x69);
		SPI_WriteData(0x14);SPI_WriteData(0x22);SPI_WriteData(0x14);SPI_WriteData(0x22);SPI_WriteData(0x14);SPI_WriteData(0x22);SPI_WriteData(0x08);
	SPI_WriteComm(0x6B);SPI_WriteData(0x03);
	SPI_WriteComm(0xD1);
		SPI_WriteData(0x01);SPI_WriteData(0x1e);SPI_WriteData(0x01);SPI_WriteData(0x26);SPI_WriteData(0x01);SPI_WriteData(0x2f);SPI_WriteData(0x01);SPI_WriteData(0x39);
		SPI_WriteData(0x01);SPI_WriteData(0x43);SPI_WriteData(0x01);SPI_WriteData(0x56);SPI_WriteData(0x01);SPI_WriteData(0x6a);SPI_WriteData(0x01);SPI_WriteData(0x85);
		SPI_WriteData(0x01);SPI_WriteData(0xa3);SPI_WriteData(0x01);SPI_WriteData(0xd1);SPI_WriteData(0x01);SPI_WriteData(0xfa);SPI_WriteData(0x02);SPI_WriteData(0x3e);
		SPI_WriteData(0x02);SPI_WriteData(0x76);SPI_WriteData(0x02);SPI_WriteData(0x78);SPI_WriteData(0x02);SPI_WriteData(0xad);SPI_WriteData(0x02);SPI_WriteData(0xe6);
		SPI_WriteData(0x03);SPI_WriteData(0x0b);SPI_WriteData(0x03);SPI_WriteData(0x3d);SPI_WriteData(0x03);SPI_WriteData(0x5a);SPI_WriteData(0x03);SPI_WriteData(0x95);
		SPI_WriteData(0x03);SPI_WriteData(0xd4);SPI_WriteData(0x03);SPI_WriteData(0xd9);SPI_WriteData(0x03);SPI_WriteData(0xe0);SPI_WriteData(0x03);SPI_WriteData(0xe6);
		SPI_WriteData(0x03);SPI_WriteData(0xf0);SPI_WriteData(0x03);SPI_WriteData(0xff);
	SPI_WriteComm(0xD2);
		SPI_WriteData(0x01);SPI_WriteData(0x1e);SPI_WriteData(0x01);SPI_WriteData(0x26);SPI_WriteData(0x01);SPI_WriteData(0x2f);SPI_WriteData(0x01);SPI_WriteData(0x39);
		SPI_WriteData(0x01);SPI_WriteData(0x43);SPI_WriteData(0x01);SPI_WriteData(0x56);SPI_WriteData(0x01);SPI_WriteData(0x6a);SPI_WriteData(0x01);SPI_WriteData(0x85);
		SPI_WriteData(0x01);SPI_WriteData(0xa3);SPI_WriteData(0x01);SPI_WriteData(0xd1);SPI_WriteData(0x01);SPI_WriteData(0xfa);SPI_WriteData(0x02);SPI_WriteData(0x3e);
		SPI_WriteData(0x02);SPI_WriteData(0x76);SPI_WriteData(0x02);SPI_WriteData(0x78);SPI_WriteData(0x02);SPI_WriteData(0xad);SPI_WriteData(0x02);SPI_WriteData(0xe6);
		SPI_WriteData(0x03);SPI_WriteData(0x0b);SPI_WriteData(0x03);SPI_WriteData(0x3d);SPI_WriteData(0x03);SPI_WriteData(0x5a);SPI_WriteData(0x03);SPI_WriteData(0x95);
		SPI_WriteData(0x03);SPI_WriteData(0xd4);SPI_WriteData(0x03);SPI_WriteData(0xd9);SPI_WriteData(0x03);SPI_WriteData(0xe0);SPI_WriteData(0x03);SPI_WriteData(0xe6);
		SPI_WriteData(0x03);SPI_WriteData(0xf0);SPI_WriteData(0x03);SPI_WriteData(0xff);
	SPI_WriteComm(0xD3);
		SPI_WriteData(0x01);SPI_WriteData(0x1e);SPI_WriteData(0x01);SPI_WriteData(0x26);SPI_WriteData(0x01);SPI_WriteData(0x2f);SPI_WriteData(0x01);SPI_WriteData(0x39);
		SPI_WriteData(0x01);SPI_WriteData(0x43);SPI_WriteData(0x01);SPI_WriteData(0x56);SPI_WriteData(0x01);SPI_WriteData(0x6a);SPI_WriteData(0x01);SPI_WriteData(0x85);
		SPI_WriteData(0x01);SPI_WriteData(0xa3);SPI_WriteData(0x01);SPI_WriteData(0xd1);SPI_WriteData(0x01);SPI_WriteData(0xfa);SPI_WriteData(0x02);SPI_WriteData(0x3e);
		SPI_WriteData(0x02);SPI_WriteData(0x76);SPI_WriteData(0x02);SPI_WriteData(0x78);SPI_WriteData(0x02);SPI_WriteData(0xad);SPI_WriteData(0x02);SPI_WriteData(0xe6);
		SPI_WriteData(0x03);SPI_WriteData(0x0b);SPI_WriteData(0x03);SPI_WriteData(0x3d);SPI_WriteData(0x03);SPI_WriteData(0x5a);SPI_WriteData(0x03);SPI_WriteData(0x95);
		SPI_WriteData(0x03);SPI_WriteData(0xd4);SPI_WriteData(0x03);SPI_WriteData(0xd9);SPI_WriteData(0x03);SPI_WriteData(0xe0);SPI_WriteData(0x03);SPI_WriteData(0xe6);
		SPI_WriteData(0x03);SPI_WriteData(0xf0);SPI_WriteData(0x03);SPI_WriteData(0xff);
	SPI_WriteComm(0xD4);
		SPI_WriteData(0x01);SPI_WriteData(0x1e);SPI_WriteData(0x01);SPI_WriteData(0x26);SPI_WriteData(0x01);SPI_WriteData(0x2f);SPI_WriteData(0x01);SPI_WriteData(0x39);
		SPI_WriteData(0x01);SPI_WriteData(0x43);SPI_WriteData(0x01);SPI_WriteData(0x56);SPI_WriteData(0x01);SPI_WriteData(0x6a);SPI_WriteData(0x01);SPI_WriteData(0x85);
		SPI_WriteData(0x01);SPI_WriteData(0xa3);SPI_WriteData(0x01);SPI_WriteData(0xd1);SPI_WriteData(0x01);SPI_WriteData(0xfa);SPI_WriteData(0x02);SPI_WriteData(0x3e);
		SPI_WriteData(0x02);SPI_WriteData(0x76);SPI_WriteData(0x02);SPI_WriteData(0x78);SPI_WriteData(0x02);SPI_WriteData(0xad);SPI_WriteData(0x02);SPI_WriteData(0xe6);
		SPI_WriteData(0x03);SPI_WriteData(0x0b);SPI_WriteData(0x03);SPI_WriteData(0x3d);SPI_WriteData(0x03);SPI_WriteData(0x5a);SPI_WriteData(0x03);SPI_WriteData(0x95);
		SPI_WriteData(0x03);SPI_WriteData(0xd4);SPI_WriteData(0x03);SPI_WriteData(0xd9);SPI_WriteData(0x03);SPI_WriteData(0xe0);SPI_WriteData(0x03);SPI_WriteData(0xe6);
		SPI_WriteData(0x03);SPI_WriteData(0xf0);SPI_WriteData(0x03);SPI_WriteData(0xff);
	SPI_WriteComm(0xD5);
		SPI_WriteData(0x01);SPI_WriteData(0x1e);SPI_WriteData(0x01);SPI_WriteData(0x26);SPI_WriteData(0x01);SPI_WriteData(0x2f);SPI_WriteData(0x01);SPI_WriteData(0x39);
		SPI_WriteData(0x01);SPI_WriteData(0x43);SPI_WriteData(0x01);SPI_WriteData(0x56);SPI_WriteData(0x01);SPI_WriteData(0x6a);SPI_WriteData(0x01);SPI_WriteData(0x85);
		SPI_WriteData(0x01);SPI_WriteData(0xa3);SPI_WriteData(0x01);SPI_WriteData(0xd1);SPI_WriteData(0x01);SPI_WriteData(0xfa);SPI_WriteData(0x02);SPI_WriteData(0x3e);
		SPI_WriteData(0x02);SPI_WriteData(0x76);SPI_WriteData(0x02);SPI_WriteData(0x78);SPI_WriteData(0x02);SPI_WriteData(0xad);SPI_WriteData(0x02);SPI_WriteData(0xe6);
		SPI_WriteData(0x03);SPI_WriteData(0x0b);SPI_WriteData(0x03);SPI_WriteData(0x3d);SPI_WriteData(0x03);SPI_WriteData(0x5a);SPI_WriteData(0x03);SPI_WriteData(0x95);
		SPI_WriteData(0x03);SPI_WriteData(0xd4);SPI_WriteData(0x03);SPI_WriteData(0xd9);SPI_WriteData(0x03);SPI_WriteData(0xe0);SPI_WriteData(0x03);SPI_WriteData(0xe6);
		SPI_WriteData(0x03);SPI_WriteData(0xf0);SPI_WriteData(0x03);SPI_WriteData(0xff);
	SPI_WriteComm(0xD6);
		SPI_WriteData(0x01);SPI_WriteData(0x1e);SPI_WriteData(0x01);SPI_WriteData(0x26);SPI_WriteData(0x01);SPI_WriteData(0x2f);SPI_WriteData(0x01);SPI_WriteData(0x39);
		SPI_WriteData(0x01);SPI_WriteData(0x43);SPI_WriteData(0x01);SPI_WriteData(0x56);SPI_WriteData(0x01);SPI_WriteData(0x6a);SPI_WriteData(0x01);SPI_WriteData(0x85);
		SPI_WriteData(0x01);SPI_WriteData(0xa3);SPI_WriteData(0x01);SPI_WriteData(0xd1);SPI_WriteData(0x01);SPI_WriteData(0xfa);SPI_WriteData(0x02);SPI_WriteData(0x3e);
		SPI_WriteData(0x02);SPI_WriteData(0x76);SPI_WriteData(0x02);SPI_WriteData(0x78);SPI_WriteData(0x02);SPI_WriteData(0xad);SPI_WriteData(0x02);SPI_WriteData(0xe6);
		SPI_WriteData(0x03);SPI_WriteData(0x0b);SPI_WriteData(0x03);SPI_WriteData(0x3d);SPI_WriteData(0x03);SPI_WriteData(0x5a);SPI_WriteData(0x03);SPI_WriteData(0x95);
		SPI_WriteData(0x03);SPI_WriteData(0xd4);SPI_WriteData(0x03);SPI_WriteData(0xd9);SPI_WriteData(0x03);SPI_WriteData(0xe0);SPI_WriteData(0x03);SPI_WriteData(0xe6);
		SPI_WriteData(0x03);SPI_WriteData(0xf0);SPI_WriteData(0x03);SPI_WriteData(0xff);
	SPI_WriteComm(0x11);
	SPI_WriteComm(0x29);

}

#define LCD_BL_EN_PIN   (GPIO_7)

static void lcd_gc9503_boe_backlight_init(void)
{
	os_printf("lcd_gc9503_boe: backlight init.\r\n");

    gpio_dev_unmap(LCD_BL_EN_PIN);
    bk_gpio_set_capacity(LCD_BL_EN_PIN, 0);
    BK_LOG_ON_ERR(bk_gpio_disable_input(LCD_BL_EN_PIN));
    BK_LOG_ON_ERR(bk_gpio_enable_output(LCD_BL_EN_PIN));
	#if 1
    bk_gpio_set_output_low(LCD_BL_EN_PIN);
	#else
	bk_gpio_set_output_high(LCD_BL_EN_PIN);
	#endif
}

static bk_err_t lcd_gc9503_boe_backlight_open(void)
{
	os_printf("lcd_gc9503_boe: backlight open.\r\n");

	bk_gpio_set_output_high(LCD_BL_EN_PIN);

	return 0;
}

static bk_err_t lcd_gc9503_boe_backlight_close(void)
{
	os_printf("lcd_gc9503_boe: backlight close.\r\n");

	bk_gpio_set_output_low(LCD_BL_EN_PIN);
	
	return 0;
}

static void lcd_gc9503_boe_init(void)
{
	os_printf("lcd_gc9503_boe: init.\r\n");
	lcd_gc9503_boe_backlight_init();
	lcd_spi_init_gpio();
	lcd_gc9503_boe_config();
}

const lcd_device_t lcd_device_gc9503_boe =
{
	.id = LCD_DEVICE_GC9503_BOE,
	.name = "gc9503_boe",
	.type = LCD_TYPE_RGB,
	.ppi = PPI_480X854,
	.rgb = &lcd_rgb,
	.init = lcd_gc9503_boe_init,
	.lcd_off = lcd_gc9503_boe_backlight_close,
	.src_fmt = PIXEL_FMT_RGB565,
	.out_fmt = PIXEL_FMT_RGB888,
};

