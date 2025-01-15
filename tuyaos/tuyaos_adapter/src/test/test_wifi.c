/*
 * test_wifi.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "cli.h"
#include "cli_tuya_test.h"
#include "tkl_wifi.h"

static void __cmd_usage(void)
{
    bk_printf("xwifi [ap|connect] [ssid] [password]\r\n");
}
void cli_wifi_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2) {
        bk_printf("parameter error\r\n");
        __cmd_usage();
        return;
    }
    for (int i = 0; i < argc; i++) {
        bk_printf("argv[%d]: %s\r\n", i, argv[i]);
    }

    if (!os_strcmp(argv[1], "connect")) {
        if (argv[2] != NULL) {
            bk_printf("station connect ssid: %s pwd: %s\r\n", argv[2], argv[3]);
            tkl_wifi_set_work_mode(WWM_STATION);
            tkl_system_sleep(1000);
            tkl_wifi_station_connect(argv[2], argv[3]);
        } else {
            __cmd_usage();
        }
    } else if (!os_strcmp(argv[1], "ap")) {
        bk_printf("not support ap test\r\n");
    }
}


