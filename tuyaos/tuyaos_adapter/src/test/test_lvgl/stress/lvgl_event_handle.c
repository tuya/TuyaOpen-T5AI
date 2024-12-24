/*
 * lvgl_event_handle.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */
#include "tuya_cloud_types.h"

void app_recv_lv_event(void *buf, uint32_t len, void *args)
{
    uint32_t *tmp = (uint32_t *)buf;

    bk_printf("%s %d %d\r\n", __func__, tmp[0], len);
}

void cli_lvgl_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    return 0;
}

