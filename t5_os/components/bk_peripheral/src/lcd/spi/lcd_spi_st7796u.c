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

#include <common/bk_include.h>
#include <driver/lcd_types.h>


static const lcd_qspi_init_cmd_t st7796u_init_cmds[] =
{
    {0x11, {0x00}, 0},
    {0x00, {0x78}, 0xFF},
    {0x36, {0x48}, 1},
    {0x3A, {0x55}, 1},
    {0xF0, {0xC3}, 1},
    {0xF0, {0x96}, 1},
    {0xB4, {0x01}, 1},
    {0xB7, {0xC6}, 1},
    {0xB9, {0x02, 0xE0}, 2},
    {0xC0, {0x80, 0x65}, 2},
    {0xC1, {0x13}, 1},
    {0xC2, {0xA7}, 1},
    {0xC5, {0x22}, 1},
    {0xE8, {0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33}, 8},
    {0xE0, {0xD0, 0x0E, 0x14, 0x0E, 0x0F, 0x1A, 0x37, 0X44, 0x48, 0x29, 0x15, 0x13, 0x18, 0x1B}, 14},
    {0XE1, {0xD0, 0x0A, 0x10, 0x0A, 0x0B, 0x17, 0x37, 0x44, 0x48, 0x2C, 0x16, 0x15, 0x1B, 0x1F}, 14},
    {0xF0, {0x3C}, 1},
    {0xF0, {0x69}, 1},
    {0x00, {0x78}, 0xFF},
    {0x29, {0x00}, 0},
};

static const lcd_spi_t lcd_spi_st7796u_config =
{
    .clk = LCD_QSPI_60M,
    .init_cmd = st7796u_init_cmds,
    .device_init_cmd_len = sizeof(st7796u_init_cmds) / sizeof (lcd_qspi_init_cmd_t),
};

const lcd_device_t lcd_device_st7796u =
{
    .id = LCD_DEVICE_ST7796U,
    .name = "st7796u",
    .type = LCD_TYPE_SPI,
    .ppi = PPI_320X480,
    .spi = &lcd_spi_st7796u_config,
    .init = NULL,
    .lcd_off = NULL,
};


