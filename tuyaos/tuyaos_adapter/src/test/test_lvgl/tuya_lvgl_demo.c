/*
 * tuya_lvgl_demo.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include <components/system.h>
#include <os/os.h>
#include "driver/media_types.h"
#include "cli_tuya_test.h"
#include "tuya_lvgl_demo.h"


/**********************
 *   GLOBAL FUNCTIONS
 **********************/
static void __lv_cb(void *buf, uint32_t len, void *args)
{
    bk_printf("-------[%s %d] \r\n", __func__, __LINE__);
}

// call tkl_lvgl_init to init lvgl ipc
static void __lvgl_local_init(void)
{
    TKL_DISP_INFO_S info;

    memset(info.ll_ctrl.ic_name, 0, IC_NAME_LENGTH);
//    extern lcd_open_t __tkl_lvgl_lcd_info;
//    info.width  = (__tkl_lvgl_lcd_info.device_ppi >> 16) & 0xffff;
//    info.height = __tkl_lvgl_lcd_info.device_ppi & 0xffff;
//    memcpy(info.ll_ctrl.ic_name, __tkl_lvgl_lcd_info.device_name, strlen(__tkl_lvgl_lcd_info.device_name));

    info.width  = 480;
    info.height = 864;
    memcpy(info.ll_ctrl.ic_name, "T50P181CQ", strlen("T50P181CQ"));
    // info.width  = 480;
    // info.height = 272;
    // memcpy(info.ll_ctrl.ic_name, "nv3047", strlen("nv3047"));

    TKL_LVGL_CFG_T lv_cfg = {
        .recv_cb = __lv_cb,
        .recv_arg = NULL,
    };
    tkl_lvgl_init(&info, &lv_cfg);
    bk_printf("-------[%s %d] \r\n", __func__, __LINE__);
}

// called by CPU1, after tkl_lvgl_start
void __attribute__((weak)) tuya_gui_main(void)
{
    __lvgl_local_init();
    bk_printf("-------[%s %d] \r\n", __func__, __LINE__);
    lv_demo_widgets();
    bk_printf("-------[%s %d] \r\n", __func__, __LINE__);
    // tuya_lvgl_stress_main();
}

void __attribute__((weak)) tuya_gui_pause(void)
{
    // tuya_lvgl_stress_pause();
}

void __attribute__((weak)) tuya_gui_resume(void)
{
    // tuya_lvgl_stress_resume();
}


