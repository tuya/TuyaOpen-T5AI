/*
 * test_usb.c
 * Copyright (C) 2025 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "cli.h"
#include "cli_tuya_test.h"
#include "tuya_cloud_types.h"
#include "tkl_gpio.h"


void cli_usb_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int status = 0;
    int count = 0;

    bk_printf("\r\n");

    TUYA_GPIO_BASE_CFG_T cfg;
    cfg.mode = TUYA_GPIO_PULLUP;
    cfg.direct = TUYA_GPIO_OUTPUT;
    cfg.level = TUYA_GPIO_LEVEL_HIGH;
    tkl_gpio_init(TUYA_GPIO_NUM_28, &cfg);

    tkl_gpio_write(TUYA_GPIO_NUM_28, TUYA_GPIO_LEVEL_HIGH);

    for (int i = 0; i < argc; i++) {
        bk_printf("argv[%d]: %s\r\n", i, argv[i]);
    }

    do {
        status = media_app_get_usb_connect_status();
        if (status) {
            bk_printf("found\r\n\r\n");
            break;
        }
        bk_printf("not found, next\r\n\r\n");
        tkl_system_sleep(50);
    } while (count++ < 10);

    tkl_gpio_write(TUYA_GPIO_NUM_28, TUYA_GPIO_LEVEL_LOW);
    return;
}

