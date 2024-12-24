/**
 * @file lvgl_vendor.h
 */

#ifndef LVGL_VENDOR_H
#define LVGL_VENDOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <driver/rott_types.h>

typedef enum{
    STATE_INIT,
    STATE_RUNNING,
    STATE_STOP
} lvgl_task_state_t;

typedef struct {
    int32_t lcd_hor_res;         /**< Horizontal resolution.*/
    int32_t lcd_ver_res;         /**< Vertical resolution.*/

    #if (CONFIG_LV_COLOR_DEPTH == 16)
    lv_color16_t *draw_buf_2_1;
    lv_color16_t *draw_buf_2_2;
    lv_color16_t *frame_buf_1;
    lv_color16_t *frame_buf_2;
    #elif (CONFIG_LV_COLOR_DEPTH == 24)
    lv_color_t *draw_buf_2_1;
    lv_color_t *draw_buf_2_2;
    lv_color_t *frame_buf_1;
    lv_color_t *frame_buf_2;
    #elif (CONFIG_LV_COLOR_DEPTH == 32)
    lv_color32_t *draw_buf_2_1;
    lv_color32_t *draw_buf_2_2;
    lv_color32_t *frame_buf_1;
    lv_color32_t *frame_buf_2;
    #endif

    uint32_t draw_pixel_size;
    uint8_t rotation;
} lv_vnd_config_t;


void lv_vendor_init(lv_vnd_config_t *config);
void lv_vendor_deinit(void);
void lv_vendor_start(void);
void lv_vendor_stop(void);
void lv_vendor_disp_lock(void);
void lv_vendor_disp_unlock(void);
int lv_vendor_display_frame_cnt(void);
int lv_vendor_draw_buffer_cnt(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LVGL_VENDOR_H*/

