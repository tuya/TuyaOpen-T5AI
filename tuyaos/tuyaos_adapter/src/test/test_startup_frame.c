/*
 * test_startup_frame.c
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


extern bk_err_t media_app_lcd_startup_frame_close(void);
extern bk_err_t media_app_lcd_startup_frame_open(void *config);

void cli_sf_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2 || argv == NULL) {
        bk_printf("[%s %d] parameter failed\r\n", __func__, __LINE__);
        return;
    }

    bk_printf("argc: %d\r\n", argc);
    for (int i = 0; i < argc; i++) {
        bk_printf("argv[%d]: %s\r\n", i, argv[i]);
    }

    if (!os_strcmp(argv[1], "open")) {
        tkl_disp_init(NULL, NULL);
        tkl_disp_open_startup_image(NULL, 0x66e000);
    } else if (!os_strcmp(argv[1], "close")) {
        tkl_disp_close_startup_image(NULL);
        tkl_disp_deinit(NULL);
    } else {
        return;
    }
}

