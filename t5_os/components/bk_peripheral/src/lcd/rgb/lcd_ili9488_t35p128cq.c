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


#include "FreeRTOS.h"
#include "task.h"

static const lcd_rgb_t lcd_rgb =
{
    //TODO
    .clk = LCD_15M,
    .data_out_clk_edge = NEGEDGE_OUTPUT,

    .hsync_pulse_width = 20,
    .hsync_back_porch  = 80,
    .hsync_front_porch = 80,
    .vsync_pulse_width = 4,
    .vsync_back_porch  = 8,
    .vsync_front_porch = 8,
};

static void lcd_ili9488_config(void)
{
    // TODO
#define Delay rtos_delay_milliseconds
#define SPI_WriteComm lcd_spi_write_cmd
#define SPI_WriteData lcd_spi_write_data

    SPI_WriteComm(0xC0);
    SPI_WriteData(0x0E);
    SPI_WriteData(0x0E);

    SPI_WriteComm(0xC1);
    SPI_WriteData(0x46);

    SPI_WriteComm(0xC5);
    SPI_WriteData(0x00);
    SPI_WriteData(0x2D);
    SPI_WriteData(0x80);

    SPI_WriteComm(0xB0);
    SPI_WriteData(0x00);

    SPI_WriteComm(0xB1);
    SPI_WriteData(0xA0);

    SPI_WriteComm(0xB4);
    SPI_WriteData(0x02);

    SPI_WriteComm(0xB5);
    SPI_WriteData(0x08);
    SPI_WriteData(0x0C);
    SPI_WriteData(0x50);
    SPI_WriteData(0x64);

    SPI_WriteComm(0xB6);
    SPI_WriteData(0x32);
    SPI_WriteData(0x02);

    SPI_WriteComm(0x36);
    SPI_WriteData(0x48);

    SPI_WriteComm(0x3A);
    SPI_WriteData(0x70);

    SPI_WriteComm(0x21);
    SPI_WriteData(0x00);

    SPI_WriteComm(0xE9);
    SPI_WriteData(0x01);

    SPI_WriteComm(0xF7);
    SPI_WriteData(0xA9);
    SPI_WriteData(0x51);
    SPI_WriteData(0x2C);
    SPI_WriteData(0x82);

    SPI_WriteComm(0xF8);
    SPI_WriteData(0x21);
    SPI_WriteData(0x05);

    SPI_WriteComm(0xE0);
    SPI_WriteData(0x00);
    SPI_WriteData(0x0C);
    SPI_WriteData(0x10);
    SPI_WriteData(0x03);
    SPI_WriteData(0x0F);
    SPI_WriteData(0x05);
    SPI_WriteData(0x37);
    SPI_WriteData(0x66);
    SPI_WriteData(0x4D);
    SPI_WriteData(0x03);
    SPI_WriteData(0x0C);
    SPI_WriteData(0x0A);
    SPI_WriteData(0x2F);
    SPI_WriteData(0x35);
    SPI_WriteData(0x0F);

    SPI_WriteComm(0xE1);
    SPI_WriteData(0x00);
    SPI_WriteData(0x0F);
    SPI_WriteData(0x16);
    SPI_WriteData(0x06);
    SPI_WriteData(0x13);
    SPI_WriteData(0x07);
    SPI_WriteData(0x3B);
    SPI_WriteData(0x35);
    SPI_WriteData(0x51);
    SPI_WriteData(0x07);
    SPI_WriteData(0x10);
    SPI_WriteData(0x0D);
    SPI_WriteData(0x36);
    SPI_WriteData(0x3B);
    SPI_WriteData(0x0F);

    SPI_WriteComm(0x11);
    Delay(120);
    SPI_WriteComm(0x29);
    Delay(20);
}

static void lcd_ili9488_init(void)
{
    os_printf("lcd_ili9488: init.\r\n");
    lcd_spi_init_gpio();
    lcd_ili9488_config();
}

const lcd_device_t lcd_device_t35p128cq =
{
    .id = LCD_DEVICE_ILI9488,
    .name = "T35P128CQ",
    .type = LCD_TYPE_RGB565,
    .ppi = PPI_320X480,
    .rgb = &lcd_rgb,
    .out_fmt = PIXEL_FMT_RGB565,
    .init = lcd_ili9488_init,
    .lcd_off = NULL,
};


