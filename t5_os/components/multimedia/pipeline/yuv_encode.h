// Copyright 2020-2023 Beken
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

#ifdef __cplusplus
extern "C" {
#endif

#include <common/bk_include.h>
#include <driver/media_types.h>
#include <driver/psram_types.h>
#include <modules/tjpgd.h>

#include "lcd_act.h"

typedef enum {
	H264_ENCODE_START = 0,
	H264_ENCODE_LINE_DONE,
	H264_ENCODE_FINISH,
	H264_ENCODE_STOP,
	H264_ENCODE_RESET
} h264_msg_type_t;

typedef enum {
	JPEGDEC_START = 0,
	JPEGDEC_LINE_DONE,
	JPEGDEC_FINISH,
	JPEGDEC_FRAME_CP1_FINISH,
	JPEGDEC_FRAME_CP2_FINISH,
	JPEGDEC_H264_NOTIFY,
	JPEGDEC_H264_CLEAR_NOTIFY,
	JPEGDEC_H264_FRAME_NOTIFY,
	JPEGDEC_ROTATE_NOTIFY,
	JPEGDEC_ROTATE_CLEAR_NOTIFY,
	JPEGDEC_SCALE_NOTIFY,
	JPEGDEC_STOP,
	JPEGDEC_SET_ROTATE_ANGLE,
	JPEGDEC_RESET,
	JPEGDEC_RESET_RESTART,
} jdec_msg_type_t;

typedef enum {
	ROTATE_START = 0,
	ROTATE_FINISH,
	ROTATE_MEMCOPY_COMPLETE,
	ROTATE_DEC_LINE_NOTIFY,  //receive decoe line complete notify
	ROTATE_LINE_COPY_START,
	ROTATE_LINE_DMA_COPY_START,
	ROTATE_STOP,
	ROTATE_NO_ROTATE_DIRECT_COPY,
	ROTATE_RESET,
} rotate_msg_type_t;

typedef enum {
	SCALE_START,
	SCALE_FINISH,
	SCALE_STOP,
	SCALE_LINE_START_LOOP,
	SCALE_LINE_SOURCE_FREE,
	SCALE_LINE_DEST_FREE,
	SCALE_LINE_SCALE_COMPLETE,
	SCALE_RESET,
} scale_msg_type_t;

typedef struct {
	frame_buffer_t *in_frame;
	frame_buffer_t *out_frame;
} media_software_decode_info_t;

bk_err_t h264_encode_task_send_msg(uint8_t type, uint32_t param);

bk_err_t h264_encode_task_open(media_camera_device_t *device);

bk_err_t h264_encode_task_close(void);
bk_err_t h264_encode_regenerate_idr_frame(void);

bool check_h264_task_is_open(void);

void jpeg_decode_restart(void);

bk_err_t jpeg_decode_task_send_msg(uint8_t type, uint32_t param);
bk_err_t jpeg_decode_task_send_more_msg(uint8_t type, uint32_t param, uint32_t param1);

bk_err_t jpeg_decode_task_open(media_decode_mode_t jdec_mode, media_decode_type_t jdec_type, media_rotate_t rotate_angle);

bk_err_t jpeg_decode_task_close();

bool check_jpeg_decode_task_is_open(void);

bk_err_t lcd_display_open(lcd_open_t *config);

bk_err_t lcd_display_close(void);

bool check_lcd_task_is_open(void);

bk_err_t rotate_task_open(rot_open_t *rot_open);

bk_err_t rotate_task_close(void);

bk_err_t rotate_task_send_msg(uint8_t type, uint32_t param);

bool check_rotate_task_is_open(void);

uint8_t *jdec_decode_get_yuv_buffer(void);

void jdec_decode_clear_rotate_buffer_handle(void);


bk_err_t jpeg_dec_task_open(uint32_t rotate_buffer);
bk_err_t jpeg_dec_task_close();
bk_err_t jpeg_dec_task_send_msg(uint32_t type, uint32_t param);
void jpeg_dec_set_rotate_angle(media_rotate_t rotate_angle);

bool check_software_decode_task_is_open(void);
bk_err_t software_decode_task_open();
bk_err_t software_decode_task_close();
bk_err_t software_decode_task_send_msg(uint8_t type, uint32_t param);

bk_err_t scale_task_open(lcd_scale_t *lcd_scale);
bk_err_t scale_task_close(void);
bk_err_t scale_task_send_msg(uint8_t type, uint32_t param);

bk_err_t lcd_display_frame_request(frame_buffer_t *frame);
bk_err_t jpeg_decode_list_push(frame_buffer_t *frame, LIST_HEADER_T *list);
frame_buffer_t *jpeg_decode_list_pop(LIST_HEADER_T *list);
uint8_t jpeg_decode_list_del_node(frame_buffer_t *frame, LIST_HEADER_T *list);
void jpeg_decode_list_clear(LIST_HEADER_T *list);
uint8_t jpeg_decode_list_get_count(LIST_HEADER_T *list);

bk_err_t jpeg_get_task_send_msg(uint8_t type, uint32_t param);
bk_err_t jpeg_get_task_open(void);
bk_err_t jpeg_get_task_close();

void jpeg_decode_cp2_init_notify(void);
bk_err_t jpeg_decode_single_frame(frame_buffer_t *in_frame,
										frame_buffer_t *out_frame,
										uint8_t scale,
										JD_FORMAT_OUTPUT format,
										media_rotate_t rotate_angle,
										uint8_t *work_buf,
										uint8_t *rotate_buf);
void jpeg_decode_set_rotate_angle(media_rotate_t rotate_angle);


bool check_uvc_status(void);

void rotate_set_dma2d_cb(void);
void sw_dec_set_dma2d_cb(void);

bk_err_t yuv_frame_rotate_handle(uint32_t param);

#ifdef __cplusplus
}
#endif

