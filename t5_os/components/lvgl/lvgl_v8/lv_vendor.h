/**
 * @file lvgl_vendor.h
 */

#ifndef LVGL_VENDOR_H
#define LVGL_VENDOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "driver/media_types.h"

typedef enum{
    STATE_INIT,
    STATE_RUNNING,
    STATE_STOP
} lvgl_task_state_t;

typedef struct {
    lv_coord_t lcd_hor_res;         /**< Horizontal resolution.*/
    lv_coord_t lcd_ver_res;         /**< Vertical resolution.*/
    lv_color_t *draw_buf_2_1;
    lv_color_t *draw_buf_2_2;

    lv_color_t *frame_buf_1;
    lv_color_t *frame_buf_2;
    uint32_t draw_pixel_size;
    media_rotate_t rotation;
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

