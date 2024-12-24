/**
 * @file lv_port_indev_templ.c
 *
 */

 /*Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include <os/os.h>
#include "lv_port_indev.h"
#include "driver/drv_tp.h"
#include "lv_port_disp.h"
#include "lv_vendor.h"


/*********************
 *      DEFINES
 *********************/
#define INDEV_RESET_TIMEOUT_COUNT (5)  // timeout = INDEV_RESET_TIMEOUT_COUNT * LV_INDEV_DEF_READ_PERIOD

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_touchpad;

/*Initialize your touchpad*/
static void touchpad_init(void)
{
    /*Your code comes here*/

}

int __attribute__((weak)) drv_tp_read(tp_point_infor_t *point)
{
    return kGeneralErr;
}

// Modified by TUYA Start
void  __attribute__((weak)) tuya_app_gui_touch_monitor(void)
{

}
// Modified by TUYA End

extern lv_vnd_config_t vendor_config;
/*Will be called by the library to read the touchpad*/
static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static uint8_t indev_reset_count = 0;
    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;
    static lv_indev_state_t last_state = LV_INDEV_STATE_REL;
    tp_point_infor_t point;
    int ret = kNoErr;

    do{
        ret = drv_tp_read(&point);
        if(kNoErr != ret)
        {
            if (LV_INDEV_STATE_REL != last_state)
            {
                indev_reset_count++;
                if (indev_reset_count >= INDEV_RESET_TIMEOUT_COUNT)
                {
                    indev_reset_count = 0;
                    last_state = LV_INDEV_STATE_REL;
                    last_x = 0;
                    last_y = 0;
                }
            }
            break;
        }

        indev_reset_count = 0;

        if(vendor_config.rotation == ROTATE_90)
        {
            lv_coord_t tmp = point.m_y;
            point.m_y = point.m_x;
            point.m_x = LV_HOR_RES - tmp - 1;
        }

        if(vendor_config.rotation == ROTATE_270)
        {
            lv_coord_t tmp = point.m_x;
            point.m_x = point.m_y;
            point.m_y = LV_VER_RES - tmp - 1;
        }

        last_x = point.m_x;
        last_y = point.m_y;
        last_state = point.m_state? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

        if(point.m_need_continue)
        {
            data->continue_reading = true;
        }

// Modified by TUYA Start
        extern void tuya_app_gui_touch_monitor(void);
        if (last_state == LV_INDEV_STATE_PRESSED) {     //pressing !
            tuya_app_gui_touch_monitor();
        }
        else {  //released !

        }
// Modified by TUYA End
    }while(0);

    /*Set the last pressed coordinates*/
    data->point.x = last_x;
    data->point.y = last_y;
    data->state = last_state;
}


/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_port_indev_init(void)
{
    /**
     * Here you will find example implementation of input devices supported by LittelvGL:
     *  - Touchpad
     *  - Mouse (with cursor support)
     *  - Keypad (supports GUI usage only with key)
     *  - Encoder (supports GUI usage only with: left, right, push)
     *  - Button (external buttons to press points on the screen)
     *
     *  The `..._read()` function are only examples.
     *  You should shape them according to your hardware
     */
    static lv_indev_drv_t indev_drv;
    /*------------------
     * Touchpad
     * -----------------*/

    /*Initialize your touchpad if you have*/
    touchpad_init();

    /*Register a touchpad input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    indev_touchpad = lv_indev_drv_register(&indev_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Touchpad
 * -----------------*/
void lv_port_indev_deinit(void)
{
    lv_indev_delete(indev_touchpad);
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
