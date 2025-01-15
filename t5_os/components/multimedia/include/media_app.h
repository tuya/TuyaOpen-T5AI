// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <common/bk_include.h>
#include <driver/uvc_camera_types.h>
#include <components/video_types.h>
#include <components/usb_types.h>
#include <driver/h264_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	APP_CAMERA_DVP_JPEG,
	APP_CAMERA_DVP_YUV,
	APP_CAMERA_DVP_MIX,
	APP_CAMERA_UVC_MJPEG,
	APP_CAMERA_UVC_MJPEG_TO_H264,
	APP_CAMERA_UVC_H264,
	APP_CAMERA_NET_MJPEG,
	APP_CAMERA_NET_H264,
	APP_CAMERA_DVP_H264_TRANSFER,
	APP_CAMERA_DVP_H264_LOCAL,
	APP_CAMERA_DVP_H264_ENC_LCD,
	APP_CAMERA_INVALIED,
} app_camera_type_t;

/*
* legacy
*/
#define APP_CAMERA_DVP APP_CAMERA_DVP_JPEG
#define APP_CAMERA_YUV APP_CAMERA_DVP_YUV
#define APP_CAMERA_MIX APP_CAMERA_DVP_MIX
#define APP_CAMERA_UVC APP_CAMERA_UVC_MJPEG

typedef enum
{
	APP_LCD_RGB,
	APP_LCD_MCU,
} app_lcd_type_t;

typedef int (*media_transfer_send_cb)(uint8_t *data, uint32_t length, uint16_t *retry_cnt);
typedef int (*media_transfer_prepare_cb)(uint8_t *data, uint32_t length);
typedef void* (*media_transfer_get_tx_buf_cb)(void);
typedef int (*media_transfer_get_tx_size_cb)(void);
typedef void (*frame_cb_t)(frame_buffer_t *frame);
typedef bool (*media_transfer_drop_check_cb)(frame_buffer_t *frame,uint32_t count, uint16_t ext_size);

typedef struct {
	media_transfer_send_cb send;
	media_transfer_prepare_cb prepare;
	media_transfer_drop_check_cb drop_check;
	media_transfer_get_tx_buf_cb get_tx_buf;
	media_transfer_get_tx_size_cb get_tx_size;
	pixel_format_t fmt;
} media_transfer_cb_t;

bk_err_t media_app_camera_open(media_camera_device_t *device);
bk_err_t media_app_camera_close(camera_type_t type);
bk_err_t media_app_get_h264_encode_config(h264_base_config_t *config);
bk_err_t media_app_set_compression_ratio(compress_ratio_t * ratio);
bk_err_t media_app_uvc_register_info_notify_cb(uvc_device_info_t cb);
bk_err_t media_app_set_uvc_device_param(bk_uvc_config_t *config);
bk_err_t media_app_register_read_frame_callback(pixel_format_t fmt, frame_cb_t cb);
bk_err_t media_app_unregister_read_frame_callback(void);
bk_err_t media_app_usb_open(video_setup_t *setup_cfg);
bk_err_t media_app_usb_close(void);
bk_err_t media_app_lcd_open(void *lcd_open);
bk_err_t media_app_lcd_close(void);
bk_err_t media_app_storage_enable(app_camera_type_t type, uint8_t enable);
bk_err_t media_app_capture(char *name);
bk_err_t media_app_save_start(char *name);
bk_err_t media_app_save_stop(void);
bk_err_t media_app_lcd_set_backlight(uint8_t level);
bk_err_t media_app_mailbox_test(void);
bk_err_t media_app_lcd_rotate(media_rotate_t rotate);
bk_err_t media_app_lcd_resize(media_ppi_t ppi);
bk_err_t media_app_dump_display_frame(void);
bk_err_t media_app_dump_decoder_frame(void);
bk_err_t media_app_dump_jpeg_frame(void);
bk_err_t media_app_lcd_step_mode(bool enable);
bk_err_t media_app_lcd_step_trigger(void);
bk_err_t media_app_transfer_pause(bool pause);
bk_err_t  media_app_lcd_display_file(char *file_name);  //display sd card file
bk_err_t media_app_lcd_display(void* lcd_display);
bk_err_t media_app_lcd_display_beken(void* lcd_display);
bk_err_t media_app_lcd_blend(void *param);
bk_err_t media_app_lcd_gui_blend_open(int blend_x_size, int blend_y_size);
bk_err_t media_app_lcd_gui_blend_close(void);
bk_err_t media_app_lcd_decode(media_decode_mode_t decode_mode);
bk_err_t media_app_lcd_blend_open(bool en);
uint32_t media_app_get_lcd_devices_num(void);
uint32_t media_app_get_lcd_devices_list(void);
uint32_t media_app_get_lcd_device_by_id(uint32_t id);
bk_err_t media_app_lcd_scale(void);
bk_err_t media_app_get_lcd_status(void);
bk_err_t media_app_get_uvc_camera_status(void);

#if CONFIG_VIDEO_AVI
bk_err_t media_app_avi_open(void);
bk_err_t media_app_avi_close(void);
#endif

bk_err_t media_app_lvgl_open(void *lcd_open);
bk_err_t media_app_lvgl_close(void);
bk_err_t media_app_lvcam_lvgl_open(void *lcd_open);
bk_err_t media_app_lvcam_lvgl_close(void);

bk_err_t media_app_rtsp_open(video_config_t *config);
bk_err_t media_app_rtsp_close();

bk_err_t media_app_lcd_pipeline_open(void *config);
bk_err_t media_app_lcd_pipeline_close(void);
bk_err_t media_app_lcd_pipeline_disp_open(void *config);
bk_err_t media_app_lcd_pipeline_disp_close(void);
bk_err_t media_app_lcd_pipeline_jdec_open(void);
bk_err_t media_app_lcd_pipeline_jdec_close(void);
bk_err_t media_app_pipline_set_rotate(media_rotate_t rotate);

bk_err_t media_app_h264_pipeline_open(void);
bk_err_t media_app_h264_pipeline_close(void);
bk_err_t media_app_h264_regenerate_idr(camera_type_t type);

bk_err_t media_app_lcd_fmt(pixel_format_t fmt);
bk_err_t media_app_lcd_pipline_scale_open(void *config);
bk_err_t media_app_lcd_pipline_scale_close(void);

bk_err_t media_app_pipeline_dump(void);

bk_err_t media_app_pipeline_mem_show(void);
bk_err_t media_app_pipeline_mem_leak(void);

bk_err_t media_app_frame_buffer_init(fb_type_t type);
frame_buffer_t *media_app_frame_buffer_jpeg_malloc(void);
frame_buffer_t *media_app_frame_buffer_h264_malloc(void);
bk_err_t media_app_frame_buffer_push(frame_buffer_t *frame);
bk_err_t media_app_frame_buffer_clear(frame_buffer_t *frame);

#ifdef __cplusplus
}
#endif
