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
	.clk = LCD_30M,
	.data_out_clk_edge = NEGEDGE_OUTPUT,

	.hsync_pulse_width = 2,
	.vsync_pulse_width = 6,
	.hsync_back_porch = 44,
	.hsync_front_porch = 46,
	.vsync_back_porch = 16,
	.vsync_front_porch = 50,
};

static void lcd_st7701s_ly_config(void)
{
#define Delay rtos_delay_milliseconds
#define SPI_WriteComm lcd_spi_write_cmd
#define SPI_WriteData lcd_spi_write_data

	bk_gpio_set_output_high(LCD_SPI_RST);
	bk_delay(10);
	bk_gpio_set_output_low(LCD_SPI_RST);
	bk_delay(120);
	bk_gpio_set_output_high(LCD_SPI_RST);
	bk_delay(60);

	//****************************************************************************//
	SPI_WriteComm(0x11);
	Delay(60); //Delay 120ms
	//---------------------------------------Bank0 Setting-------------------------------------------------//
	//------------------------------------Display Control setting----------------------------------------------//
	SPI_WriteComm(0xFF);
	SPI_WriteData(0x77);
	SPI_WriteData(0x01);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x10);
	SPI_WriteComm(0xC0);
	SPI_WriteData(0x63);
	SPI_WriteData(0x00);
	SPI_WriteComm(0xC1);
	SPI_WriteData(0x11);
	SPI_WriteData(0x02);
	SPI_WriteComm(0xC2);
	SPI_WriteData(0x31);
	SPI_WriteData(0x08);
	SPI_WriteComm(0xC3);
	SPI_WriteData(0x10);
	SPI_WriteComm(0xCC);
	SPI_WriteData(0x10);
	//-------------------------------------Gamma Cluster Setting------------------------------------------- //
	SPI_WriteComm(0xB0);
	SPI_WriteData(0x40);
	SPI_WriteData(0x01);
	SPI_WriteData(0x46);
	SPI_WriteData(0x0D);
	SPI_WriteData(0x13);
	SPI_WriteData(0x09);
	SPI_WriteData(0x05);
	SPI_WriteData(0x09);
	SPI_WriteData(0x09);
	SPI_WriteData(0x1B);
	SPI_WriteData(0x07);
	SPI_WriteData(0x15);
	SPI_WriteData(0x12);
	SPI_WriteData(0x4C);
	SPI_WriteData(0x10);
	SPI_WriteData(0xC8);
	SPI_WriteComm(0xB1);
	SPI_WriteData(0x40);
	SPI_WriteData(0x02);
	SPI_WriteData(0x86);
	SPI_WriteData(0x0D);
	SPI_WriteData(0x13);
	SPI_WriteData(0x09);
	SPI_WriteData(0x05);
	SPI_WriteData(0x09);
	SPI_WriteData(0x09);
	SPI_WriteData(0x1F);
	SPI_WriteData(0x07);
	SPI_WriteData(0x15);
	SPI_WriteData(0x12);
	SPI_WriteData(0x15);
	SPI_WriteData(0x19);
	SPI_WriteData(0x08);
	//--------------------------------------- End Gamma Setting---------------------------------------------- //
	//------------------------------------End Display Control setting----------------------------------------//
	//----------------------------------------- Bank0 Setting End--------------------------------------------- //
	//------------------------------------------- Bank1 Setting--------------------------------------------------- //
	//-------------------------------- Power Control Registers Initial -------------------------------------- //
	SPI_WriteComm(0xFF);
	SPI_WriteData(0x77);
	SPI_WriteData(0x01);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x11);
	SPI_WriteComm(0xB0);
	SPI_WriteData(0x50);
	//------------------------------------------- Vcom Setting--------------------------------------------------- //
	SPI_WriteComm(0xB1);
	SPI_WriteData(0x68);
	//----------------------------------------- End Vcom Setting----------------------------------------------- //
	SPI_WriteComm(0xB2);
	SPI_WriteData(0x07);
	SPI_WriteComm(0xB3);
	SPI_WriteData(0x80);
	SPI_WriteComm(0xB5);
	SPI_WriteData(0x47);
	SPI_WriteComm(0xB7);
	SPI_WriteData(0x85);
	SPI_WriteComm(0xB8);
	SPI_WriteData(0x21);
	SPI_WriteComm(0xB9);
	SPI_WriteData(0x10);
	SPI_WriteComm(0xC1);
	SPI_WriteData(0x78);
	SPI_WriteComm(0xC2);
	SPI_WriteData(0x78);
	SPI_WriteComm(0xD0);
	SPI_WriteData(0x88);
	//---------------------------------End Power Control Registers Initial -------------------------------//
	Delay(100);
	//------------------------------------------ ---GIP Setting---------------------------------------------------- //
	SPI_WriteComm(0xE0);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x02);
	SPI_WriteComm(0xE1);
	SPI_WriteData(0x08);
	SPI_WriteData(0x00);
	SPI_WriteData(0x0A);
	SPI_WriteData(0x00);
	SPI_WriteData(0x07);
	SPI_WriteData(0x00);
	SPI_WriteData(0x09);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x33);
	SPI_WriteData(0x33);
	SPI_WriteComm(0xE2);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteComm(0xE3);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x33);
	SPI_WriteData(0x33);
	SPI_WriteComm(0xE4);
	SPI_WriteData(0x44);
	SPI_WriteData(0x44);
	SPI_WriteComm(0xE5);
	SPI_WriteData(0x0E);
	SPI_WriteData(0x2D);
	SPI_WriteData(0xA0);
	SPI_WriteData(0xA0);
	SPI_WriteData(0x10);
	SPI_WriteData(0x2D);
	SPI_WriteData(0xA0);
	SPI_WriteData(0xA0);
	SPI_WriteData(0x0A);
	SPI_WriteData(0x2D);
	SPI_WriteData(0xA0);
	SPI_WriteData(0xA0);
	SPI_WriteData(0x0C);
	SPI_WriteData(0x2D);
	SPI_WriteData(0xA0);
	SPI_WriteData(0xA0);
	SPI_WriteComm(0xE6);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x33);
	SPI_WriteData(0x33);
	SPI_WriteComm(0xE7);
	SPI_WriteData(0x44);
	SPI_WriteData(0x44);
	SPI_WriteComm(0xE8);
	SPI_WriteData(0x0D);
	SPI_WriteData(0x2D);
	SPI_WriteData(0xA0);
	SPI_WriteData(0xA0);
	SPI_WriteData(0x0F);
	SPI_WriteData(0x2D);
	SPI_WriteData(0xA0);
	SPI_WriteData(0xA0);
	SPI_WriteData(0x09);
	SPI_WriteData(0x2D);
	SPI_WriteData(0xA0);
	SPI_WriteData(0xA0);
	SPI_WriteData(0x0B);
	SPI_WriteData(0x2D);
	SPI_WriteData(0xA0);
	SPI_WriteData(0xA0);
	SPI_WriteComm(0xEB);
	SPI_WriteData(0x02);
	SPI_WriteData(0x01);
	SPI_WriteData(0xE4);
	SPI_WriteData(0xE4);
	SPI_WriteData(0x44);
	SPI_WriteData(0x00);
	SPI_WriteData(0x40);
	SPI_WriteComm(0xEC);
	SPI_WriteData(0x02);
	SPI_WriteData(0x01);
	SPI_WriteComm(0xED);
	SPI_WriteData(0xAB);
	SPI_WriteData(0x89);
	SPI_WriteData(0x76);
	SPI_WriteData(0x54);
	SPI_WriteData(0x01);
	SPI_WriteData(0xFF);
	SPI_WriteData(0xFF);
	SPI_WriteData(0xFF);
	SPI_WriteData(0xFF);
	SPI_WriteData(0xFF);
	SPI_WriteData(0xFF);
	SPI_WriteData(0x10);
	SPI_WriteData(0x45);
	SPI_WriteData(0x67);
	SPI_WriteData(0x98);
	SPI_WriteData(0xBA);
	//-------------------------------------------- End GIP Setting----------------------------------------------- //
	//------------------------------ Power Control Registers Initial End-----------------------------------//
	//------------------------------------------ Bank1 Setting---------------------------------------------------- //
	SPI_WriteComm(0xFF);
	SPI_WriteData(0x77);
	SPI_WriteData(0x01);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);
	SPI_WriteData(0x00);

	//SPI_WriteComm(0x36);
	//SPI_WriteData(0x00);

	SPI_WriteComm(0x3A);
	SPI_WriteData(0x77);
	//SPI_WriteComm(0x21);
	SPI_WriteComm(0x29);
}


static void lcd_st7701s_ly_init(void)
{
	os_printf("lcd_st7701s_ly: init.\r\n");
	lcd_spi_init_gpio();
	lcd_st7701s_ly_config();
}

const lcd_device_t lcd_device_st7701s_ly =
{
	.id = LCD_DEVICE_ST7701S_LY,
	.name = "st7701s_ly",
	.type = LCD_TYPE_RGB565,
	.ppi = PPI_480X800,
	.rgb = &lcd_rgb,
	.out_fmt = PIXEL_FMT_RGB888,
	.init = lcd_st7701s_ly_init,
	.lcd_off = NULL,
};

