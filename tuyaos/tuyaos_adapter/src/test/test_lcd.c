/*
 * test_lcd.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "stdint.h"

#include "cli.h"
#include <driver/hal/hal_lcd_types.h>
#include <driver/int_types.h>
#include <driver/lcd_types.h>
#include <lcd_act.h>

#include "tuya_cloud_types.h"
#include "cli_tuya_test.h"

static uint32_t color_value = 0;
static beken_timer_t lcd_rgb_timer;
#define pdata_addr	0x60700000

#define LCD_WIDTH       320             // 480
#define LCD_HIGTH       480                //480             // 800
#define LCD_PPI         PPI_320X480         //PPI_480X480               //PPI_320X480     // PPI_480X800
#define TEST_LCD_NAME   "ili9488"       //"st7701sn"                       // "st7701s"

static uint32_t lcd_width = 320;
static uint32_t lcd_height = 480;
static uint32_t lcd_ppi = PPI_320X480;
static char *lcd_name = NULL;
static int __check_chip_support(const char *str)
{
    if (!os_strcmp(str, "ili9488") ||
        !os_strcmp(str, "st7701sn") ||
        !os_strcmp(str, "gc9503_boe") ||
        !os_strcmp(str, "st7701s")) {
        return 1;
    }
    bk_printf("lcd driver unsupport\r\n");
    return 0;
}
static int __get_string_to_ppi(const char *str)
{
    if (!os_strcmp(str, "320x480")) {
        lcd_width  = 320;
        lcd_height = 480;
        lcd_ppi = PPI_320X480;
    } else if (!os_strcmp(str, "480x800")) {
        lcd_width  = 480;
        lcd_height = 800;
        lcd_ppi = PPI_480X800;
    } else if (!os_strcmp(str, "480x854")) {
        lcd_width  = 480;
        lcd_height = 854;
        lcd_ppi = PPI_480X854;
    } else if (!os_strcmp(str, "480x480")) {
        lcd_width  = 480;
        lcd_height = 480;
        lcd_ppi = PPI_480X480;
    } else {
        bk_printf("PPI unsupport\r\n");
        return -1;
    }
    return 0;
}

static void color_test_usage(void)
{
    bk_printf("color test usage:\r\n");
    bk_printf("\txlcd [device] [PPI] [color] [set color value]\r\n");
    bk_printf("\t\tdevice: ili9488|st7701s|st7701s|gc9503_boe ...\r\n");
    bk_printf("\t\tPPI: 320x480|480x854|480x480|480x800 ...\r\n");
    bk_printf("\t\tcolor: red|blue|green|purple or set [value]\r\n");
    bk_printf("\texample:\r\n");
    bk_printf("\tcolor random:          xlcd ili9488 320x480\r\n");
    bk_printf("\tcolor red:             xlcd ili9488 320x480 red\r\n");
    bk_printf("\tcolor self-defined:    xlcd ili9488 320x480 set 7EF\r\n");
}

static void cpu_lcd_fill_test(uint32_t *addr, uint32_t color)
{
    uint32_t *p_addr = addr;
    for(int i=0; i<lcd_width*lcd_height; i++)
    {
        *(p_addr + i) = color;
    }
}
static void lcd_get_rand_color(uint32_t *color)
{
    uint32_t color_rand = 0;
    uint32_t color_rand_tmp = 0;
    color_rand = (uint32_t)rand();
    color_rand_tmp = (color_rand & 0xffff0000) >> 16;
    *color = (color_rand & 0xffff0000) | color_rand_tmp;
    bk_printf("color set: 0x%x\r\n", *color);
}

static void lcd_get_spec_color(const char *str, uint32_t *color)
{
    if (!os_strcmp(str, "red"))
        *color = 0xF800;
    else if (!os_strcmp(str, "green"))
        *color = 0x3666;
    else if (!os_strcmp(str, "blue"))
        *color = 0x18CE;
    else if (!os_strcmp(str, "purple"))
        *color = 0x8010;
    else {
        *color = 0x7EF;
        bk_printf("color unsupport\r\n");
        color_test_usage();
    }
    bk_printf("color set: 0x%x\r\n", *color);
}
static void lcd_rgb_change_color(const char *str)
{
    static uint8_t state = 0;
    bk_err_t ret = BK_FAIL;
    lcd_open_t lcd_open;
    uint32_t color = 0;
    lcd_display_t lcd_display = {0};
    if(state == 0) {
        lcd_open.device_ppi = lcd_ppi;
        lcd_open.device_name = lcd_name;
        ret = media_app_lcd_open(&lcd_open);
        if(BK_OK != ret) {
            os_printf("%s, not found device\n", __func__);
            return;
        }
        state = 1;
    }

    if (str == NULL)
        lcd_get_rand_color(&color);
    else if  (!os_strcmp(str, "set"))
        color = color_value;
    else
        lcd_get_spec_color(str, &color);

    cpu_lcd_fill_test((uint32_t *)pdata_addr, color);

    lcd_display.display_type = LCD_TYPE_RGB565;
    lcd_display.image_addr = pdata_addr;
    lcd_display.x_start = 0;
    lcd_display.x_end = lcd_width - 1;
    lcd_display.y_start = 0;
    lcd_display.y_end = lcd_height - 1;
    os_printf("--- %s, %d %d\n", lcd_name, lcd_width, lcd_height);
    ret = media_app_lcd_display(&lcd_display);
    if(BK_OK != ret)
    {
        os_printf("%s, lcd display fail\n", __func__);
    }
    return;
}

void cli_xlcd_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    static uint8_t lcd_rgb_timer_stat = 0;

    if (argc < 3) {
        color_test_usage();
        return;
    }

    bk_printf("argc: %d\r\n", argc);
    for (int i = 0; i < argc; i++) {
        bk_printf("argv[%d]: %s\r\n", i, argv[i]);
    }

    bk_printf("------- not used ------\r\n");
    return;

    lcd_name = argv[1];
    if (!__check_chip_support(lcd_name)) {
        color_test_usage();
        return;
    }

    int ret = __get_string_to_ppi(argv[2]);
    if (ret < 0) {
        color_test_usage();
        return;
    }

    if (argc == 3) {
        bk_printf("random color\r\n");
        if (lcd_rgb_timer_stat == 0) {
            rtos_init_timer(&lcd_rgb_timer, 3000,  (timer_handler_t)lcd_rgb_change_color, 0);
        }
        rtos_start_timer(&lcd_rgb_timer);
        lcd_rgb_timer_stat = 1;
        return;
    } else if (argc == 4) {
        if (lcd_rgb_timer_stat == 1) {
            rtos_stop_timer(&lcd_rgb_timer);
            lcd_rgb_timer_stat = 0;
        }
        bk_printf("color: %s\r\n", argv[3]);
        lcd_rgb_change_color(argv[3]);
    } else if (argc == 5) {
        if (lcd_rgb_timer_stat == 1) {
            rtos_stop_timer(&lcd_rgb_timer);
            lcd_rgb_timer_stat = 0;
        }
        if (!os_strcmp(argv[3], "set")) {
            color_value = 0;
            lcd_rgb_change_color(argv[4]);
            color_value = os_strtoul(argv[4], NULL, 16);
            bk_printf("set color: %x\r\n", color_value);
            lcd_rgb_change_color(argv[4]);
        } else {
            color_test_usage();
        }

    } else {
        color_test_usage();
    }
    return;
}


