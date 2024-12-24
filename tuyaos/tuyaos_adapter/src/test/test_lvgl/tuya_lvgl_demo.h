/**
 * @file tuya_lvgl_demo.h
 *
 */

#ifndef __TUYA_LVGL_DEMO_H__
#define __TUYA_LVGL_DEMO_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "tkl_lvgl.h"

/*********************
 *      DEFINES
 *********************/

#define LV_DEMO_STRESS_TIME_STEP    50

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void tuya_gui_main(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TUYA_LVGL_DEMO_H__ */
