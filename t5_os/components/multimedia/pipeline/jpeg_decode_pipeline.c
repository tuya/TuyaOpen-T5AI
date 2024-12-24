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

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include <driver/jpeg_dec.h>
#include <modules/jpeg_decode_sw.h>
#include <modules/tjpgd.h>
#include <modules/image_scale.h>
#include <modules/pm.h>

#include "media_mailbox_list_util.h"
#include "media_evt.h"
#include "frame_buffer.h"
#include "lcd_decode.h"
#include "yuv_encode.h"
#include <lcd_rotate.h>

#include "mux_pipeline.h"
#if (CONFIG_CPU_CNT > 2)
#include "components/system.h"
#endif

#define TAG "jdec_pip"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


#define JPEGDEC_BUFFER_LENGTH        (60 * 1024)

#ifdef DECODE_DIAG_DEBUG
#define DECODER_FRAME_START()		do { GPIO_UP(GPIO_DVP_D0); } while (0)
#define DECODER_FRAME_END()			do { GPIO_DOWN(GPIO_DVP_D0); } while (0)

#define DECODER_LINE_START()		do { GPIO_UP(GPIO_DVP_D1); } while (0)
#define DECODER_LINE_END()			do { GPIO_DOWN(GPIO_DVP_D1); } while (0)


#define H264_DECODER_NOTIFY() \
	do{                \
		GPIO_DOWN(GPIO_DVP_HSYNC);  \
		GPIO_UP(GPIO_DVP_HSYNC);    \
		GPIO_DOWN(GPIO_DVP_HSYNC);  \
		               \
	}while(0)

#define ROTATE_DECODE_NOTIFY()  \
    do{                             \
        GPIO_DOWN(GPIO_DVP_VSYNC);  \
        GPIO_UP(GPIO_DVP_VSYNC);    \
        GPIO_DOWN(GPIO_DVP_VSYNC);  \
                                    \
    }while(0)

#define SCALE_DECODE_NOTIFY() \
do{ 			   \
	GPIO_DOWN(GPIO_DVP_PCLK);  \
	GPIO_UP(GPIO_DVP_PCLK);    \
	GPIO_DOWN(GPIO_DVP_PCLK);  \
				   \
}while(0)
#else
#define DECODER_LINE_START()
#define DECODER_LINE_END()
#define DECODER_FRAME_START()
#define DECODER_FRAME_END()
#define H264_DECODER_NOTIFY()
#define ROTATE_DECODE_NOTIFY()
#define SCALE_DECODE_NOTIFY()

#endif


typedef struct {
	uint8_t enable;
	uint8_t start;
} module_state_t;

typedef enum
{
	DECODE_STATE_IDLE = 0,
	DECODE_STATE_DECODING,
} decode_state_t;

#define MUX_MAX		(2)

typedef struct {
	uint8_t jdec_type : 1; // by line(0) or by complete frame(1)
	uint8_t task_state : 1;
	uint8_t jdec_init : 1; // flag for jpegdec have know jpeg format(yuv422/yuv420)
	uint8_t jdec_line_count;
	uint8_t sw_dec_init : 1;
	uint8_t trigger;
	media_decode_mode_t jdec_mode; // jpegdec_hw, jpeg_sw
	uint32_t jdec_offset;
	uint8_t *decoder_buf;
	pipeline_mux_buf_t mux_buf[MUX_MAX];
	pipeline_mux_buf_t *work_buf;
	module_state_t module[PIPELINE_MOD_MAX];
	frame_buffer_t *jpeg_frame;
	frame_buffer_t *jdec_frame;
	beken_semaphore_t jdec_sem;
	beken_semaphore_t jdec_cp2_init_sem;
	beken_queue_t jdec_queue;
	beken_thread_t jdec_thread;
	decode_state_t state;
	beken2_timer_t decoder_timer;
	LIST_HEADER_T jpeg_decode_queue;
	uint32_t cp1_last_decode_sequence;
	uint32_t cp2_last_decode_sequence;
	media_software_decode_info_t sw_dec_info[2];
	media_rotate_t rotate_angle;
	mux_request_callback_t cb[PIPELINE_MOD_MAX];
	mux_reset_callback_t   reset_cb[PIPELINE_MOD_MAX];
} jdec_config_t;

typedef struct {
	beken_mutex_t lock;
} jdec_info_t;

static void jpeg_decode_line_complete_handler(jpeg_dec_res_t *result);
static bk_err_t jpeg_h264_line_request_callback(void *param);
static bk_err_t jpeg_rotate_line_request_callback(void *param);
static bk_err_t jpeg_scale_line_request_callback(void *param);
static bk_err_t h264_reset_request_callback(void *param);
static bk_err_t scale_reset_request_callback(void *param);
static bk_err_t rotate_reset_request_callback(void *param);


#if CONFIG_LVGL
extern uint8_t lvgl_disp_enable;
#endif

extern media_debug_t *media_debug;
static jdec_config_t *jdec_config = NULL;
static jdec_info_t *jdec_info = NULL;
media_mailbox_msg_t jdec_msg = {0};

const mux_callback_t mux_callback[PIPELINE_MOD_LINE_MAX] = {
	jpeg_h264_line_request_callback,
	jpeg_rotate_line_request_callback,
	jpeg_scale_line_request_callback,
};

void jpeg_decode_restart(void)
{
	if (jdec_config && jdec_config->jdec_init)
	{
		jdec_config->jdec_init = false;
	}
}

void jpeg_decode_cp2_init_notify(void)
{
	if (jdec_config && jdec_config->jdec_cp2_init_sem)
	{
		rtos_set_semaphore(&jdec_config->jdec_cp2_init_sem);
	}
}

bk_err_t jpeg_decode_task_send_msg(uint8_t type, uint32_t param)
{
	int ret = BK_OK;
	jpeg_msg_t msg;

	if (jdec_config && jdec_config->jdec_queue)
	{
		msg.event = type;
		msg.param = param;

		ret = rtos_push_to_queue(&jdec_config->jdec_queue, &msg, BEKEN_WAIT_FOREVER);

		if (ret != BK_OK)
		{
			LOGE("%s push failed\n", __func__);
		}
	}

	return ret;
}

bk_err_t jpeg_decode_task_send_more_msg(uint8_t type, uint32_t param, uint32_t param1)
{
	int ret = BK_OK;
	jpeg_msg_t msg;

	if (jdec_config && jdec_config->jdec_queue)
	{
		msg.event = type;
		msg.param = param;
		msg.param1 = param1;

		ret = rtos_push_to_queue(&jdec_config->jdec_queue, &msg, BEKEN_WAIT_FOREVER);

		if (ret != BK_OK)
		{
			LOGE("%s push failed\n", __func__);
		}
	}

	return ret;
}

static bool jpeg_decode_check_buf_state(pipeline_mux_buf_t *mux_buf)
{
	bool idle = true;

	for (int i = 0; i < PIPELINE_MOD_MAX; i++)
	{
		if (mux_buf->state[i] != MUX_BUFFER_IDLE)
		{
			idle = false;
		}
	}

	return idle;
}

static void jpeg_decode_err_handler(jpeg_dec_res_t *result)
{
	LOGI("%s\n", __func__);
	decoder_mux_dump();
	jpeg_decode_task_send_msg(JPEGDEC_RESET, 0);
}

static void jpeg_decode_frame_complete_handler(jpeg_dec_res_t *result)
{
	if (jdec_config->task_state)
		jpeg_decode_task_send_msg(JPEGDEC_FINISH, 1);

	jdec_config->state = DECODE_STATE_IDLE;
}

static inline bool jpeg_decode_frame_is_last_line(uint8 index)
{
	return index == (jdec_config->jpeg_frame->height >> 4);
}

static inline uint8_t jpeg_decode_mux_buf_index_get(void)
{
	return (!(jdec_config->jdec_line_count & 0x01)) & 0x01;
}

static inline bool jpeg_decode_mux_buf_mask(pipeline_mux_buf_t *mux_buf)
{
	bool ret = false;

	if (jpeg_decode_check_buf_state(mux_buf))
	{
		for (int i = 0; i < PIPELINE_MOD_MAX; i++)
		{
			if (jdec_config->module[i].start == true)
			{
				mux_buf->state[i] = MUX_BUFFER_PRESET;
			}
		}

		ret = true;
	}
    else
    {
        LOGI("%s %d buf_mux_false, buf_id %d:[%x %x %x], \n", __func__, __LINE__,mux_buf->buffer.id , mux_buf->state[PIPELINE_MOD_H264], mux_buf->state[PIPELINE_MOD_ROTATE], mux_buf->state[PIPELINE_MOD_SCALE]);
    }

	return ret;
}

static bk_err_t jpeg_h264_line_request_callback(void *param)
{
    H264_DECODER_NOTIFY();
	LOGD("%s\n", __func__);
	return jpeg_decode_task_send_msg(JPEGDEC_H264_NOTIFY, (uint32_t)param);
}

static bk_err_t jpeg_rotate_line_request_callback(void *param)
{   
    ROTATE_DECODE_NOTIFY();
	LOGD("%s\n", __func__);
	return jpeg_decode_task_send_msg(JPEGDEC_ROTATE_NOTIFY, (uint32_t)param);
}

static bk_err_t jpeg_scale_line_request_callback(void *param)
{
    SCALE_DECODE_NOTIFY();
	LOGD("%s\n", __func__);
	return jpeg_decode_task_send_msg(JPEGDEC_SCALE_NOTIFY, (uint32_t)param);
}

static bk_err_t h264_reset_request_callback(void *param)
{
    return jpeg_decode_task_send_msg(JPEGDEC_RESET_RESTART, PIPELINE_MOD_H264);
}
static bk_err_t scale_reset_request_callback(void *param)
{
    return jpeg_decode_task_send_msg(JPEGDEC_RESET_RESTART, PIPELINE_MOD_SCALE);
}
static bk_err_t rotate_reset_request_callback(void *param)
{
    return jpeg_decode_task_send_msg(JPEGDEC_RESET_RESTART, PIPELINE_MOD_ROTATE);
}

static void jpeg_decode_reset_restart(uint32_t param)
{
    if (param == PIPELINE_MOD_H264)
    {
        jdec_config->module[PIPELINE_MOD_H264].start = 0;
        LOGD("%s h264 \n", __func__);
    }
    if (param == PIPELINE_MOD_ROTATE)
    {
        jdec_config->module[PIPELINE_MOD_ROTATE].start = 0;
        LOGD("%s rotate \n", __func__);
    }
    if (param == PIPELINE_MOD_SCALE)
    {
        jdec_config->module[PIPELINE_MOD_SCALE].start = 0;
        LOGD("%s scale \n", __func__);
    }

	if ((jdec_config->module[PIPELINE_MOD_H264].start == 0) 
        && (jdec_config->module[PIPELINE_MOD_SCALE].start == 0)
        && (jdec_config->module[PIPELINE_MOD_ROTATE].start == 0))
	{
        LOGI("%s restart\n", __func__);
        jpeg_get_task_send_msg(JPEGDEC_START, MODULE_DECODER);
        
	}
}

static void jpeg_decode_line_start_continue(void)
{

	jdec_config->state = DECODE_STATE_DECODING;

	if (!rtos_is_oneshot_timer_running(&jdec_config->decoder_timer))
	{
		rtos_start_oneshot_timer(&jdec_config->decoder_timer);
	}

	uint8 index = jdec_config->jdec_line_count & 0x01;
	jdec_config->work_buf = &jdec_config->mux_buf[index];

	if (jdec_config->jdec_mode == JPEGDEC_HW_MODE)
	{
		bk_jpeg_dec_by_line_start();
        DECODER_LINE_START();
	}
	else
	{
		bk_jpeg_dec_sw_start_line(); // there should judge return value
		jpeg_decode_line_complete_handler(NULL);
	}
}

static void jpeg_decode_line_complete_handler(jpeg_dec_res_t *result)
{
	DECODER_LINE_END();
	jdec_config->jdec_line_count++;

	pipeline_mux_buf_t *mux_buf = jdec_config->work_buf;
	mux_buf->buffer.index = jdec_config->jdec_line_count;

	if (result)
	{
		mux_buf->buffer.ok = result->ok;
	}
	else
	{
		mux_buf->buffer.ok = true;
	}

	mux_buf->encoded = true;

	if (jdec_config->task_state)
	{
		LOGD("%s, ID: %d\n", __func__, mux_buf->state);
		jpeg_decode_task_send_msg(JPEGDEC_LINE_DONE, (uint32_t)mux_buf);
	}

	jdec_config->state = DECODE_STATE_IDLE;


#if 0
	if (jpeg_decode_frame_is_last_line(jdec_config->jdec_line_count))
	{
		return;
	}


	uint8_t next_id = !(mux_buf->id);
	pipeline_mux_buf_t *mux_next = &jdec_config->mux_buf[next_id];

	if (jpeg_decode_mux_buf_mask(mux_next))
	{
		jpeg_decode_line_start_continue();
	}
#endif
}

void jpeg_decode_set_rotate_angle(media_rotate_t rotate_angle)
{
	if (jdec_config)
	{
		if (jdec_config->rotate_angle != rotate_angle)
		{
			if (jdec_config->sw_dec_init)
			{
				jdec_config->rotate_angle = rotate_angle;
				software_decode_task_send_msg(JPEGDEC_SET_ROTATE_ANGLE, jdec_config->rotate_angle);

				msg_send_req_to_media_major_mailbox_sync(EVENT_JPEG_DEC_SET_ROTATE_ANGLE_NOTIFY, MINOR_MODULE, jdec_config->rotate_angle, NULL);
			}
		}
	}
	else
	{
		LOGE("%s %d jpeg decode task is closed\r\n", __func__, __LINE__);
	}
}

static void jpeg_decode_software_decode_start_handle(frame_module_t module)
{
	if (!jdec_config->mux_buf[1].state[PIPELINE_MOD_SW_DEC])
	{
		if (module == MODULE_DECODER)
		{
			jdec_config->jdec_frame = frame_buffer_display_malloc(jdec_config->jpeg_frame->width *
										jdec_config->jpeg_frame->height * 2);
			if(jdec_config->jdec_frame == NULL)
			{
				LOGE("%s(%d) jdec_config->jdec_frame is NULL\r\n", __func__, __LINE__);
				frame_buffer_fb_free(jdec_config->jpeg_frame, MODULE_DECODER);
				jdec_config->jpeg_frame = NULL;
				if (!jdec_config->task_state)
				{
					jpeg_decode_task_send_msg(JPEGDEC_STOP, 0);
				}
				else
				{
					jpeg_get_task_send_msg(JPEGDEC_START, MODULE_DECODER);
				}
				return;
			}

			jdec_config->jdec_frame->type = jdec_config->jpeg_frame->type;
			jdec_config->jdec_frame->sequence = jdec_config->jpeg_frame->sequence;
			jdec_config->jdec_frame->width = jdec_config->jpeg_frame->width;
			jdec_config->jdec_frame->height = jdec_config->jpeg_frame->height;
			jdec_config->jdec_frame->fmt = PIXEL_FMT_YUYV;

			jdec_config->sw_dec_info[1].in_frame = jdec_config->jpeg_frame;
			jdec_config->sw_dec_info[1].out_frame = jdec_config->jdec_frame;
			jdec_config->cp2_last_decode_sequence = jdec_config->jpeg_frame->sequence;
			jdec_config->jpeg_frame = NULL;
			jdec_config->jdec_frame = NULL;
			jdec_config->mux_buf[1].state[PIPELINE_MOD_SW_DEC] = MUX_BUFFER_SHAREED;

			jdec_msg.event = EVENT_JPEG_DEC_START_NOTIFY;
			jdec_msg.param = (uint32_t)&jdec_config->sw_dec_info[1];
			msg_send_notify_to_media_major_mailbox(&jdec_msg, MINOR_MODULE);
		}
		else
		{
			jpeg_get_task_send_msg(JPEGDEC_START, MODULE_DECODER);
		}
	}

	if (!jdec_config->mux_buf[0].state[PIPELINE_MOD_SW_DEC])
	{
		if (module != MODULE_DECODER_CP1)
		{
			jpeg_get_task_send_msg(JPEGDEC_START, MODULE_DECODER_CP1);
			return;
		}
		jdec_config->jdec_frame = frame_buffer_display_malloc(jdec_config->jpeg_frame->width *
									jdec_config->jpeg_frame->height * 2);
		if(jdec_config->jdec_frame == NULL)
		{
			LOGE("%s(%d) jdec_config->jdec_frame is NULL\r\n", __func__, __LINE__);
			frame_buffer_fb_free(jdec_config->jpeg_frame, MODULE_DECODER_CP1);
			jdec_config->jpeg_frame = NULL;
			if (!jdec_config->task_state)
			{
				jpeg_decode_task_send_msg(JPEGDEC_STOP, 0);
			}
			else
			{
				jpeg_get_task_send_msg(JPEGDEC_START, MODULE_DECODER_CP1);
			}
			return;
		}

		jdec_config->jdec_frame->type = jdec_config->jpeg_frame->type;
		jdec_config->jdec_frame->sequence = jdec_config->jpeg_frame->sequence;
		jdec_config->jdec_frame->width = jdec_config->jpeg_frame->width;
		jdec_config->jdec_frame->height = jdec_config->jpeg_frame->height;
		jdec_config->jdec_frame->fmt = PIXEL_FMT_YUYV;

		jdec_config->sw_dec_info[0].in_frame = jdec_config->jpeg_frame;
		jdec_config->sw_dec_info[0].out_frame = jdec_config->jdec_frame;
		jdec_config->cp1_last_decode_sequence = jdec_config->jpeg_frame->sequence;
		jdec_config->jpeg_frame = NULL;
		jdec_config->jdec_frame = NULL;
		jdec_config->mux_buf[0].state[PIPELINE_MOD_SW_DEC] = MUX_BUFFER_SHAREED;

		software_decode_task_send_msg(JPEGDEC_START, (uint32_t)&jdec_config->sw_dec_info[0]);
	}
}

static void jpeg_decode_start_handle(frame_buffer_t *jpeg_frame, frame_module_t module)
{
	int ret = BK_OK;
	sw_jpeg_dec_res_t result;

	// step 1: read a jpeg frame
	if (jdec_config->jdec_mode == JPEGDEC_SW_MODE)
	{
		jdec_config->jpeg_frame = jpeg_frame;
		if (jdec_config->jpeg_frame == NULL)
		{
			return;
		}
		if (module == MODULE_DECODER)
		{
			if (jdec_config->jpeg_frame->sequence != 0 && jdec_config->jpeg_frame->sequence <= jdec_config->cp1_last_decode_sequence)
			{
				frame_buffer_fb_free(jdec_config->jpeg_frame, MODULE_DECODER);
				jdec_config->jpeg_frame = NULL;
				jpeg_get_task_send_msg(JPEGDEC_START, MODULE_DECODER);
				return;
			}
		}
		else if (module == MODULE_DECODER_CP1)
		{
			if (jdec_config->jpeg_frame->sequence != 0 && jdec_config->jpeg_frame->sequence <= jdec_config->cp2_last_decode_sequence)
			{
				frame_buffer_fb_free(jdec_config->jpeg_frame, MODULE_DECODER_CP1);
				jdec_config->jpeg_frame = NULL;
				jpeg_get_task_send_msg(JPEGDEC_START, MODULE_DECODER_CP1);
				return;
			}
		}
		else
		{
			frame_buffer_fb_free(jdec_config->jpeg_frame, module);
			jdec_config->jpeg_frame = NULL;
			return;
		}
	}
	else
	{
		jdec_config->jpeg_frame = jpeg_frame;
		if (jdec_config->jpeg_frame)
		{
			if (jdec_config->jpeg_frame->mix)
			{
				LOGI("%s, not need jpeg_decode\r\n", __func__);
				jpeg_decode_task_send_msg(JPEGDEC_STOP, 0);
				return;
			}
		}
	}

	LOGD("%s, %d, %d-%d\r\n", __func__, __LINE__, jdec_config->jpeg_frame->width, jdec_config->jpeg_frame->height);

	if (!jdec_config->task_state)
	{
		LOGI("%s, %d\r\n", __func__, __LINE__);
		return;
	}

	if (jdec_config->jdec_init == false)
	{
		jdec_config->jdec_init = true;

		yuv_enc_fmt_t yuv_fmt = bk_get_original_jpeg_encode_data_format(jdec_config->jpeg_frame->frame, jdec_config->jpeg_frame->length);

		if (yuv_fmt == YUV_422)
		{
			if (jdec_config->jdec_mode == JPEGDEC_SW_MODE)
			{
				LOGI("%s, FMT: YUV422, PPI: %dX%d, SOFTWARE change to HARDWARE DECODE\r\n",
					__func__, jdec_config->jpeg_frame->width, jdec_config->jpeg_frame->height);
				jdec_config->jdec_mode = JPEGDEC_HW_MODE;
				jdec_config->jdec_init = false;
				frame_buffer_fb_free(jdec_config->jpeg_frame, module);
				jdec_config->jpeg_frame = NULL;
				return;
			}
			if (module == MODULE_DECODER_CP1)
			{
				jdec_config->jdec_mode = JPEGDEC_HW_MODE;
				jdec_config->jdec_init = false;
				frame_buffer_fb_free(jdec_config->jpeg_frame, module);
				jdec_config->jpeg_frame = NULL;
				jpeg_get_task_send_msg(JPEGDEC_START, MODULE_DECODER);
				return;
			}
			LOGI("%s, FMT: YUV422, PPI: %dX%d, use HARDWARE DECODE\r\n",
				__func__, jdec_config->jpeg_frame->width, jdec_config->jpeg_frame->height);
			jdec_config->jdec_mode = JPEGDEC_HW_MODE;
			jdec_config->jdec_type = JPEGDEC_BY_LINE;
		}
		else if (yuv_fmt == YUV_ERR)
		{
			LOGI("%s, FMT:ERR\r\n", __func__);
			frame_buffer_fb_free(jdec_config->jpeg_frame, MODULE_DECODER);
			jdec_config->jpeg_frame = NULL;
			jdec_config->jdec_init = false;
			jpeg_get_task_send_msg(JPEGDEC_START, MODULE_DECODER);
			return;
		}
		else
		{
			LOGI("%s, FMT: YUV420, PPI: %dX%d, use SOFTWARE DECODE\r\n",
				__func__, jdec_config->jpeg_frame->width, jdec_config->jpeg_frame->height);
			jdec_config->jdec_mode = JPEGDEC_SW_MODE;
			jdec_config->jdec_type = JPEGDEC_BY_FRAME;
			if(CPU2_USER_JPEG_SW_DEC == vote_start_cpu2_core(CPU2_USER_JPEG_SW_DEC))	//first owner start CPU2, so needs to wait sem
			{
				rtos_get_semaphore(&jdec_config->jdec_cp2_init_sem, BEKEN_WAIT_FOREVER);
			}
			jdec_config->sw_dec_init = 1;
#if CONFIG_SOFTWARE_DECODE_SRAM_MAPPING
			msg_send_req_to_media_major_mailbox_sync(EVENT_JPEG_DEC_INIT_NOTIFY, MINOR_MODULE, (uint32_t)(mux_sram_buffer->rotate + ROTATE_MAX_PIPELINE_LINE_SIZE), NULL);
#else
			msg_send_req_to_media_major_mailbox_sync(EVENT_JPEG_DEC_INIT_NOTIFY, MINOR_MODULE, 0, NULL);
#endif
			if (jdec_config->rotate_angle != ROTATE_NONE)
			{
				software_decode_task_send_msg(JPEGDEC_SET_ROTATE_ANGLE, jdec_config->rotate_angle);

				msg_send_req_to_media_major_mailbox_sync(EVENT_JPEG_DEC_SET_ROTATE_ANGLE_NOTIFY, MINOR_MODULE, jdec_config->rotate_angle, NULL);
			}
		}
	}

	if (jdec_config->jdec_mode == JPEGDEC_HW_MODE)
	{
		jdec_config->mux_buf[0].buffer.data = jdec_config->decoder_buf;
		jdec_config->mux_buf[1].buffer.data = jdec_config->mux_buf[0].buffer.data + jdec_config->jpeg_frame->width * 2 * PIPELINE_DECODE_LINE;
		jdec_config->work_buf = &jdec_config->mux_buf[0];
		jdec_config->mux_buf[0].buffer.id = 0;
		jdec_config->mux_buf[1].buffer.id = 1;
        jdec_config->jdec_line_count = 0;
	}

	// step 2: start jpeg decode
	if (jdec_config->jdec_mode == JPEGDEC_HW_MODE)
	{
		if (jdec_config->jdec_type == JPEGDEC_BY_LINE)
		{
			rtos_lock_mutex(&jdec_info->lock);
			GLOBAL_INT_DECLARATION();
			GLOBAL_INT_DISABLE();

			for (int i = 0; i < PIPELINE_MOD_MAX; i++)
			{
				if (jdec_config->module[i].enable == true)
				{
					jdec_config->module[i].start = true;
				}
				else
				{
					jdec_config->module[i].start = false;
				}
			}

			GLOBAL_INT_RESTORE();

			DECODER_FRAME_START();
			DECODER_LINE_START();


			if (jpeg_decode_mux_buf_mask(jdec_config->work_buf) == false)
			{
				LOGE("%s buffer error\n", __func__);
			}

			rtos_unlock_mutex(&jdec_info->lock);

			if (!rtos_is_oneshot_timer_running(&jdec_config->decoder_timer))
			{
				rtos_start_oneshot_timer(&jdec_config->decoder_timer);
			}

			ret = bk_jpeg_dec_hw_start(jdec_config->jpeg_frame->length, jdec_config->jpeg_frame->frame, jdec_config->decoder_buf);
		}
		else
		{
			jdec_config->jdec_frame = frame_buffer_display_malloc(jdec_config->jpeg_frame->width *
										jdec_config->jpeg_frame->height * 2);

			if (jdec_config->jdec_frame == NULL)
			{
				LOGE("%s(%d) jdec_config->jdec_frame is NULL\r\n", __func__, __LINE__);
				return;
			}
			jdec_config->jdec_frame->type = jdec_config->jpeg_frame->type;
			jdec_config->jdec_frame->sequence = jdec_config->jpeg_frame->sequence;
			jdec_config->jdec_frame->width = jdec_config->jpeg_frame->width;
			jdec_config->jdec_frame->height = jdec_config->jpeg_frame->height;
			jdec_config->jdec_frame->fmt = PIXEL_FMT_YUYV;

			ret = bk_jpeg_dec_hw_start(jdec_config->jpeg_frame->length, jdec_config->jpeg_frame->frame, jdec_config->jdec_frame->frame);
			if (ret != BK_OK)
			{
				LOGE("%s hw decoder error\n", __func__);
			}
		}
	}
	else
	{
		if (jdec_config->jdec_type == JPEGDEC_BY_LINE)
		{
			rtos_lock_mutex(&jdec_info->lock);
			GLOBAL_INT_DECLARATION();
			GLOBAL_INT_DISABLE();

			for (int i = 0; i < PIPELINE_MOD_MAX; i++)
			{
				if (jdec_config->module[i].enable == true)
				{
					jdec_config->module[i].start = true;
				}
				else
				{
					jdec_config->module[i].start = false;
				}
			}

			GLOBAL_INT_RESTORE();
			DECODER_FRAME_START();

			DECODER_LINE_START();

			if (jpeg_decode_mux_buf_mask(jdec_config->work_buf) == false)
			{
				LOGE("%s buffer error\n", __func__);
			}

			rtos_unlock_mutex(&jdec_info->lock);

			(void)(result, jdec_msg);

			if (!rtos_is_oneshot_timer_running(&jdec_config->decoder_timer))
			{
				rtos_start_oneshot_timer(&jdec_config->decoder_timer);
			}

			ret = bk_jpeg_dec_sw_start(JPEGDEC_BY_LINE, jdec_config->jpeg_frame->frame, jdec_config->decoder_buf,
						jdec_config->jpeg_frame->length, JPEGDEC_BUFFER_LENGTH, &result);
		}
		else
		{
			jpeg_decode_software_decode_start_handle(module);
			return;
		}
	}

	if (ret != BK_OK)
	{
		if (rtos_is_oneshot_timer_running(&jdec_config->decoder_timer))
    	{
    		rtos_stop_oneshot_timer(&jdec_config->decoder_timer);
    	}
		if (jdec_config->work_buf)
		{
			// set work buf to idle
			os_memset(&jdec_config->work_buf->state, 0, sizeof(jdec_config->work_buf->state));
		}

		if (jdec_config->jdec_frame)
		{
			frame_buffer_display_free(jdec_config->jdec_frame);
			jdec_config->jdec_frame = NULL;
		}

		frame_buffer_fb_free(jdec_config->jpeg_frame, MODULE_DECODER);
		jdec_config->jpeg_frame = NULL;

		jpeg_get_task_send_msg(JPEGDEC_START, MODULE_DECODER);
		return;
	}
	else
	{
		// only for jpegdec_software_mode
		if (jdec_config->jdec_mode == JPEGDEC_SW_MODE)
		{
			if (jdec_config->jdec_type == JPEGDEC_BY_LINE)
			{
				jpeg_decode_line_complete_handler(NULL); // suggest to unify struct jpeg_dec_res_t and sw_jpeg_dec_res_t
			}
			else
			{
				jpeg_decode_frame_complete_handler(NULL);
			}
		}
	}
}

static void jpeg_decode_line_done_handle(uint32_t param)
{
	pipeline_mux_buf_t *mux_buf = (pipeline_mux_buf_t*)param;
	pipeline_encode_request_t request;

	if (rtos_is_oneshot_timer_running(&jdec_config->decoder_timer))
	{
		rtos_stop_oneshot_timer(&jdec_config->decoder_timer);
	}
    if(jdec_config->jpeg_frame == NULL)
    {
        LOGW("%s jdec err cb already executed, jpeg frame is null\n", __func__);
        return;
    }
	request.jdec_type = jdec_config->jdec_type;
	request.width = jdec_config->jpeg_frame->width;
	request.height = jdec_config->jpeg_frame->height;
	request.buffer = &mux_buf->buffer;

	rtos_lock_mutex(&jdec_info->lock);

	for (int i = 0; i < PIPELINE_MOD_LINE_MAX; i++)
	{
		if (mux_buf->state[i] == MUX_BUFFER_PRESET)
		{
			int ret = BK_FAIL;

			if (jdec_config->cb[i])
			{
				LOGD("%s, %d\n", __func__, i);
				ret = jdec_config->cb[i](&request, mux_callback[i]);
			}

			if (ret != BK_OK)
			{
				mux_buf->state[i] = MUX_BUFFER_IDLE;
			}
			else
			{
				mux_buf->state[i] = MUX_BUFFER_SHAREED;
			}
		}
	}

	rtos_unlock_mutex(&jdec_info->lock);
}

static void jpeg_decode_finish_handle(uint32_t param)
{
	int ret = BK_OK;
	// step 1: free current jpeg frame

	if (jdec_config->jpeg_frame)
	{
        frame_buffer_fb_free(jdec_config->jpeg_frame, MODULE_DECODER);
        jdec_config->jpeg_frame = NULL;
	}

    if (param == 1)
    {
        media_debug->isr_decoder++;
    }

	if (jdec_config->jdec_type == JPEGDEC_BY_LINE)
	{
		jdec_config->jdec_line_count = 0;

		os_memset(&jdec_config->mux_buf[0].state, 0, sizeof(jdec_config->mux_buf[0].state));
		os_memset(&jdec_config->mux_buf[1].state, 0, sizeof(jdec_config->mux_buf[1].state));

        rtos_lock_mutex(&jdec_info->lock);
		for (int i = 0; i < PIPELINE_MOD_MAX; i++)
		{
			if (jdec_config->module[i].enable == false)
			{
				jdec_config->module[i].start = false;
			}
		}
        rtos_unlock_mutex(&jdec_info->lock);

		// step 2: jpeg decode a new frame
		if(param != MUX_DEC_TIMEOUT)
        {
		    jpeg_get_task_send_msg(JPEGDEC_START, MODULE_DECODER);
        }
	}
	else
	{
		if (jdec_config->module[PIPELINE_MOD_H264].start)
		{
			pipeline_encode_request_t *jdec_pipeline_info = os_malloc(sizeof(pipeline_encode_request_t));

			//jdec_pipeline_info->decode_buf = (void *)jdec_config->jdec_frame;
			jdec_pipeline_info->width = jdec_config->jdec_frame->width;
			jdec_pipeline_info->height = jdec_config->jdec_frame->height;
			jdec_pipeline_info->jdec_type = jdec_config->jdec_type;
			//LOGI("%s, %d %p, %d-%d\r\n", __func__, __LINE__, jdec_pipeline_info->decode_buf, jdec_pipeline_info->width, jdec_pipeline_info->height);

			// step 1: send decode buf to h264
			ret = h264_encode_task_send_msg(H264_ENCODE_START, (uint32_t)jdec_pipeline_info);
		}
		else
		{
			ret = BK_FAIL;
			jpeg_get_task_send_msg(JPEGDEC_START, MODULE_DECODER);
		}

		if (ret != BK_OK)
		{
			lcd_display_frame_request(jdec_config->jdec_frame);
		}
	}
    DECODER_LINE_END();
	DECODER_FRAME_END();
}

static void jpeg_decode_get_next_frame()
{
	if (!jdec_config->mux_buf[1].state[PIPELINE_MOD_SW_DEC])
	{
		jpeg_get_task_send_msg(JPEGDEC_START, MODULE_DECODER);
	}
	if (!jdec_config->mux_buf[0].state[PIPELINE_MOD_SW_DEC])
	{
		jpeg_get_task_send_msg(JPEGDEC_START, MODULE_DECODER_CP1);
	}
}


static void jpeg_decode_task_deinit(void)
{
	LOGI("%s\r\n", __func__);
	jpeg_get_task_close();
	if(check_software_decode_task_is_open())
	{
		software_decode_task_close();
	}
	if (jdec_config)
	{
		/* // maybe do not judge jpeg decode mode
		if (jdec_config->jdec_mode == JPEGDEC_HW_MODE)
		{
			bk_jpeg_dec_driver_deinit();
		}
		else
		{
			// deinit jpegdec_sw
			bk_jpeg_dec_sw_deinit();
		}*/
		bk_jpeg_dec_driver_deinit();

		if (jdec_config->decoder_buf)
		{
			LOGD("%s free decoder_buf\n", __func__);
			jdec_config->decoder_buf = NULL;
			jdec_config->mux_buf[0].buffer.data = NULL;
			jdec_config->mux_buf[1].buffer.data = NULL;
		}
 
		if (jdec_config->jpeg_frame)
		{
			LOGD("%s free jpeg_frame\n", __func__);
			frame_buffer_fb_free(jdec_config->jpeg_frame, MODULE_DECODER);
			jdec_config->jpeg_frame = NULL;
		}

		if (jdec_config->sw_dec_info[0].out_frame)
		{
			frame_buffer_display_free(jdec_config->sw_dec_info[0].out_frame);
			jdec_config->sw_dec_info[0].out_frame = NULL;
		}
		if (jdec_config->sw_dec_info[1].out_frame)
		{
			frame_buffer_display_free(jdec_config->sw_dec_info[1].out_frame);
			jdec_config->sw_dec_info[1].out_frame = NULL;
		}

		if (jdec_config->jdec_frame)
		{
			LOGD("%s free decode_frame\n", __func__);
			frame_buffer_display_free(jdec_config->jdec_frame);
			jdec_config->jdec_frame = NULL;
		}
		jpeg_decode_list_clear(&jdec_config->jpeg_decode_queue);

		frame_buffer_fb_deregister(MODULE_DECODER, FB_INDEX_JPEG);

		if (jdec_config->jdec_queue)
		{
			rtos_deinit_queue(&jdec_config->jdec_queue);
			jdec_config->jdec_queue = NULL;
		}

		if(jdec_config->jdec_sem)
		{
			rtos_deinit_semaphore(&jdec_config->jdec_sem);
		}

		if(jdec_config->jdec_cp2_init_sem)
		{
			rtos_deinit_semaphore(&jdec_config->jdec_cp2_init_sem);
		}

		jdec_config->jdec_thread = NULL;

		os_free(jdec_config);
		jdec_config = NULL;
	}

#if CONFIG_SOC_BK7256XX
#else
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_DECODER, PM_CPU_FRQ_DEFAULT);
#endif
	LOGI("%s complete\r\n", __func__);
}


static void jpeg_decode_notify_handle(uint32_t param, pipeline_module_t module)
{
	complex_buffer_t *decoder_buffer = (complex_buffer_t*)param;
	pipeline_mux_buf_t *mux_buf = NULL;

	for (int i = 0; i < 2; i++)
	{
		if (jdec_config->mux_buf[i].buffer.data == decoder_buffer->data)
		{
			if (jdec_config->mux_buf[i].buffer.index != decoder_buffer->index)
			{
				LOGE("%s unknow index: %d\n", __func__, decoder_buffer->index);
				goto out;
			}

			mux_buf = &jdec_config->mux_buf[i];
			break;
		}
	}

	if (mux_buf == NULL)
	{
		LOGE("%s error: %p, %d\n", __func__, mux_buf, module);
		return;
	}

	rtos_lock_mutex(&jdec_info->lock);

	mux_buf->state[module] = MUX_BUFFER_IDLE;

	/*
	 * frame complete
	*/

	if (jpeg_decode_check_buf_state(mux_buf))
	{
		if (mux_buf->encoded)
		{
			mux_buf->encoded = false;
		}
		else
		{
			//LOGE("%s multi notify from: %d, index: %d, ignore\n", __func__, module, mux_buf->index);
			BK_ASSERT_EX(0, "%s multi notify from: %d, index: %d, input state: %d, ignore\n",
				__func__, module, mux_buf->buffer.index, decoder_buffer->state);
			//return;
		}

		if (jpeg_decode_frame_is_last_line(mux_buf->buffer.index))
		{
			LOGD("%s, %d %d\r\n", __func__, jdec_config->jdec_line_count, mux_buf->buffer.ok);
			jpeg_decode_task_send_msg(JPEGDEC_FINISH, mux_buf->buffer.ok);
		}
		else
		{
			if (jdec_config->state == DECODE_STATE_IDLE)
			{
				uint8_t next_id = !(mux_buf->buffer.id);
				pipeline_mux_buf_t *mux_next = &jdec_config->mux_buf[next_id];

				if (jpeg_decode_mux_buf_mask(mux_next))
				{
					jpeg_decode_line_start_continue();
				}
                else
                {
                    LOGE("%s, %d\r\n", __func__, mux_next->buffer.id, mux_next->buffer.index);
                }
			}
		}
	}

	rtos_unlock_mutex(&jdec_info->lock);

out:

	os_free(decoder_buffer);
}

void jdec_decode_clear_h264_buffer_handle(void)
{
	LOGD("%s\n", __func__);
	if (!jdec_config->jdec_type)
	{
		for (int i = 0; i < MUX_MAX; i++)
		{
			if (jdec_config->mux_buf[i].state[PIPELINE_MOD_H264] == MUX_BUFFER_PRESET)
			{
				complex_buffer_t *buffer = os_malloc(sizeof(complex_buffer_t));
				os_memcpy(buffer, &jdec_config->mux_buf[i].buffer, sizeof(complex_buffer_t));
				jpeg_decode_task_send_msg(JPEGDEC_H264_NOTIFY, (uint32_t)buffer);
				LOGI("%s, start: %d, encoded:%d\n", __func__, jdec_config->module[PIPELINE_MOD_H264].start,
					 jdec_config->mux_buf[i].encoded);
			}
		}
	}
}

void jdec_decode_clear_rotate_buffer_handle(void)
{
	LOGD("%s\n", __func__);
	if (!jdec_config->jdec_type)
	{
		if (jdec_config->mux_buf[0].state[PIPELINE_MOD_H264])
		{
			LOGD("%s, clear buffer 0\n", __func__);
			//jpeg_decode_task_send_msg(JPEGDEC_ROTATE_NOTIFY, (uint32_t)&jdec_config->mux_buf[0]);
		}

		if (jdec_config->mux_buf[1].state[PIPELINE_MOD_H264])
		{
			LOGD("%s, clear buffer 1\n", __func__);
			//jpeg_decode_task_send_msg(JPEGDEC_ROTATE_NOTIFY, (uint32_t)&jdec_config->mux_buf[1]);
		}
	}
}

static void jpeg_decode_reset(void)
{
	LOGI("%s %d \n", __func__, __LINE__);
	if (rtos_is_oneshot_timer_running(&jdec_config->decoder_timer))
	{
		rtos_stop_oneshot_timer(&jdec_config->decoder_timer);
	}
	bk_jpeg_dec_stop();
	jpeg_decode_finish_handle(MUX_DEC_TIMEOUT);
}

void h264_frame_start()
{
	frame_buffer_t *frame = jpeg_decode_list_pop(&jdec_config->jpeg_decode_queue);
	bk_err_t ret = BK_OK;
	if (frame)
	{
		pipeline_encode_request_t *jdec_pipeline_info = os_malloc(sizeof(pipeline_encode_request_t));
		if (jdec_pipeline_info == NULL)
		{
			LOGE("%s %d malloc error\r\n", __func__, __LINE__);
			return;
		}
		jdec_config->mux_buf[0].buffer.data = (uint8_t *)frame;
		jdec_config->mux_buf[0].buffer.index = 0;
		jdec_pipeline_info->buffer = &jdec_config->mux_buf[0].buffer;
		jdec_pipeline_info->width = frame->width;
		jdec_pipeline_info->height = frame->height;
		jdec_pipeline_info->jdec_type = jdec_config->jdec_type;
		jdec_pipeline_info->buffer->index = 1;
		jdec_config->mux_buf[0].state[PIPELINE_MOD_H264] = MUX_BUFFER_SHAREED;
		ret = h264_encode_task_send_msg(H264_ENCODE_START, (uint32_t)jdec_pipeline_info);
		if (ret != BK_OK)
		{
			LOGE("%s %d h264_encode_task_send_msg error\r\n", __func__, __LINE__);
			if (jdec_pipeline_info)
			{
				os_free(jdec_pipeline_info);
			}
		}
	}
}

static void jpeg_decode_software_decode_finish_handle(frame_module_t module, uint32_t result)
{
	int index_1 = 0, index_2 = 1;
	if (module == MODULE_DECODER)
	{
		index_1 = 1;
		index_2 = 0;
	}
	else if (module == MODULE_DECODER_CP1)
	{
		index_1 = 0;
		index_2 = 1;
	}
	else
	{
		return;
	}
	jdec_config->mux_buf[index_1].state[PIPELINE_MOD_SW_DEC] = MUX_BUFFER_IDLE;
	if (jdec_config->sw_dec_info[index_1].in_frame != NULL)
	{
		frame_buffer_fb_free(jdec_config->sw_dec_info[index_1].in_frame, module);
		jdec_config->sw_dec_info[index_1].in_frame = NULL;
	}

	if (!jdec_config->task_state)
	{
		frame_buffer_display_free(jdec_config->sw_dec_info[index_1].out_frame);
		jpeg_decode_task_send_msg(JPEGDEC_STOP, 0);
		goto out;
	}

	if (result == BK_OK)
	{
		jpeg_decode_list_push(jdec_config->sw_dec_info[index_1].out_frame, &jdec_config->jpeg_decode_queue);
		if (check_h264_task_is_open())
		{
			if ((jdec_config->sw_dec_info[index_2].out_frame != NULL) && (jdec_config->sw_dec_info[index_1].out_frame->sequence > jdec_config->sw_dec_info[index_2].out_frame->sequence))
			{
//				LOGE("%s %d wait for cp2 finish cp1 %d cp2 %d\r\n", __func__, __LINE__, jdec_config->sw_dec_info[index_1].out_frame->sequence, jdec_config->sw_dec_info[index_2].out_frame->sequence);
				goto out;
			}
			if (jdec_config->mux_buf[0].state[PIPELINE_MOD_H264] == MUX_BUFFER_IDLE)
			{
				h264_frame_start();
			}
		}
		else if (check_lcd_task_is_open())
		{
			if ((jdec_config->sw_dec_info[index_2].out_frame != NULL) && (jdec_config->sw_dec_info[index_1].out_frame->sequence > jdec_config->sw_dec_info[index_2].out_frame->sequence))
			{
//				LOGE("%s %d wait for cp2 finish cp1 %d cp2 %d\r\n", __func__, __LINE__, jdec_config->sw_dec_info[index_1].out_frame->sequence, jdec_config->sw_dec_info[index_2].out_frame->sequence);
				goto out;
			}
			int count = jpeg_decode_list_get_count(&jdec_config->jpeg_decode_queue);
			for (int i = 0 ; i < count ; i ++)
			{
				frame_buffer_t *frame = jpeg_decode_list_pop(&jdec_config->jpeg_decode_queue);
				if (frame == NULL)
				{
					break;
				}

			#if CONFIG_LVGL
				if (lvgl_disp_enable) {
					jpeg_decode_list_del_node(frame, &jdec_config->jpeg_decode_queue);
					frame_buffer_display_free(frame);
				}
				else
			#endif
				{
					lcd_display_frame_request(frame);
					jpeg_decode_list_del_node(frame, &jdec_config->jpeg_decode_queue);
				}
			}
		}
		else
		{
			jpeg_decode_list_del_node(jdec_config->sw_dec_info[index_1].out_frame, &jdec_config->jpeg_decode_queue);
			frame_buffer_display_free(jdec_config->sw_dec_info[index_1].out_frame);
		}
	}
	else
	{
		jpeg_decode_list_del_node(jdec_config->sw_dec_info[index_1].out_frame, &jdec_config->jpeg_decode_queue);
		frame_buffer_display_free(jdec_config->sw_dec_info[index_1].out_frame);
	}
out:
	jdec_config->sw_dec_info[index_1].out_frame = NULL;
	media_debug->isr_decoder++;
	jpeg_decode_get_next_frame();
}

static void jpeg_decode_h264_frame_notify(complex_buffer_t *decoder_buffer)
{
	frame_buffer_t *frame_buffer = NULL;

	if (decoder_buffer)
	{
		frame_buffer = (frame_buffer_t*)decoder_buffer->data;
	}

	jdec_config->mux_buf[0].state[PIPELINE_MOD_H264] = MUX_BUFFER_IDLE;
	if (frame_buffer == NULL)
	{
		jpeg_decode_list_clear(&jdec_config->jpeg_decode_queue);
		goto out;
	}
	else
	{
		uint8_t delete_status = jpeg_decode_list_del_node(frame_buffer, &jdec_config->jpeg_decode_queue);
		if (delete_status == false)
		{
			goto out;
		}
	}

	if (!jdec_config->task_state)
	{
		jpeg_decode_task_send_msg(JPEGDEC_STOP, 0);
		goto out;
	}

	if (check_lcd_task_is_open())
	{
	#if CONFIG_LVGL
		if (lvgl_disp_enable) {
			frame_buffer_display_free(frame_buffer);
		}
		else
	#endif
		{
			lcd_display_frame_request(frame_buffer);
		}
	}
	else
	{
		frame_buffer_display_free(frame_buffer);
	}
	h264_frame_start();

	jpeg_decode_get_next_frame();

out:
	if (decoder_buffer)
	{
		os_free(decoder_buffer);
	}

}

static void jpeg_decode_main(beken_thread_arg_t data)
{
	int ret = BK_OK;
	jdec_config->task_state = true;

	rtos_set_semaphore(&jdec_config->jdec_sem);

	while(1)
	{
		jpeg_msg_t msg;
		ret = rtos_pop_from_queue(&jdec_config->jdec_queue, &msg, BEKEN_WAIT_FOREVER);
		if (ret == BK_OK)
		{
			switch (msg.event)
			{
				case JPEGDEC_START:
					if (jdec_config->task_state)
					{
						jpeg_decode_start_handle((frame_buffer_t *)msg.param, msg.param1);
					}
					else
					{
						frame_buffer_fb_free((frame_buffer_t *)msg.param, MODULE_DECODER);
						LOGI("free jpeg frame \n");
					}
					break;

				case JPEGDEC_LINE_DONE:
					jpeg_decode_line_done_handle(msg.param);
					break;

				case JPEGDEC_FINISH:
					jpeg_decode_finish_handle(msg.param);
					break;

				case JPEGDEC_FRAME_CP1_FINISH:
					jpeg_decode_software_decode_finish_handle(MODULE_DECODER_CP1, msg.param);
					break;

				case JPEGDEC_FRAME_CP2_FINISH:
					jpeg_decode_software_decode_finish_handle(MODULE_DECODER, msg.param);
					break;

				case JPEGDEC_H264_NOTIFY:
					jpeg_decode_notify_handle(msg.param, PIPELINE_MOD_H264);
					break;

				case JPEGDEC_H264_CLEAR_NOTIFY:
					//jdec_decode_clear_h264_buffer_handle();
					break;

				case JPEGDEC_ROTATE_NOTIFY:
					jpeg_decode_notify_handle(msg.param, PIPELINE_MOD_ROTATE);
					break;

				case JPEGDEC_ROTATE_CLEAR_NOTIFY:
					jdec_decode_clear_rotate_buffer_handle();
					break;

				case JPEGDEC_SCALE_NOTIFY:
					jpeg_decode_notify_handle(msg.param, PIPELINE_MOD_SCALE);
					break;

				case JPEGDEC_H264_FRAME_NOTIFY:
				{
					complex_buffer_t *decoder_buffer = (complex_buffer_t *)msg.param;
					jpeg_decode_h264_frame_notify(decoder_buffer);
					break;
				}

				case JPEGDEC_RESET:
                    jpeg_decode_reset();
					if(jdec_config->module[PIPELINE_MOD_H264].enable)
					{
						jdec_config->reset_cb[PIPELINE_MOD_H264](h264_reset_request_callback);
					}
                    if(jdec_config->module[PIPELINE_MOD_SCALE].enable)
					{
						jdec_config->reset_cb[PIPELINE_MOD_SCALE](scale_reset_request_callback);
					}
					if(jdec_config->module[PIPELINE_MOD_ROTATE].enable)
					{
						jdec_config->reset_cb[PIPELINE_MOD_ROTATE](rotate_reset_request_callback);
					}
					break;

                case JPEGDEC_RESET_RESTART:
                    jpeg_decode_reset_restart(msg.param);
                    break;

				case JPEGDEC_STOP:
				{
					if (jdec_config->jdec_mode == JPEGDEC_SW_MODE)
					{
						LOGI("%s %d status cp1:%d cp2:%d h264:%d\n", __func__, __LINE__,
							jdec_config->mux_buf[0].state[PIPELINE_MOD_SW_DEC],
							jdec_config->mux_buf[1].state[PIPELINE_MOD_SW_DEC],
							jdec_config->mux_buf[0].state[PIPELINE_MOD_H264]);

						if (jdec_config->mux_buf[0].state[PIPELINE_MOD_SW_DEC] ||
							jdec_config->mux_buf[1].state[PIPELINE_MOD_SW_DEC] ||
							jdec_config->mux_buf[0].state[PIPELINE_MOD_H264])
						{
							break;
						}
					}
					if (jdec_config->sw_dec_init == 1)
					{
						msg_send_req_to_media_major_mailbox_sync(EVENT_JPEG_DEC_DEINIT_NOTIFY, MINOR_MODULE, 0, NULL);
					}
					if (rtos_is_oneshot_timer_running(&jdec_config->decoder_timer))
					{
						rtos_stop_oneshot_timer(&jdec_config->decoder_timer);
					}

					if (rtos_is_oneshot_timer_init(&jdec_config->decoder_timer))
					{
						rtos_deinit_oneshot_timer(&jdec_config->decoder_timer);
					}
				}
				goto exit;

				default:
					break;
			}
		}
	}

exit:
	LOGI("%s complete, %d\n", __func__, __LINE__);
	rtos_set_semaphore(&jdec_config->jdec_sem);
	LOGI("%s, exit\r\n", __func__);
	rtos_delete_thread(NULL);
}

static void jpeg_decode_init(void)
{
	if (jdec_config->jdec_mode == JPEGDEC_HW_MODE)
	{
		bk_jpeg_dec_driver_init();
		bk_jpeg_dec_isr_register(DEC_ERR, jpeg_decode_err_handler);
		if (jdec_config->jdec_type == JPEGDEC_BY_LINE)
		{
		    if (PIPELINE_DECODE_LINE == 16)
			    bk_jpeg_dec_line_num_set(LINE_16);
            else
                LOGE("%s, to config decode line \n", __func__);
			bk_jpeg_dec_isr_register(DEC_EVERY_LINE_INT, jpeg_decode_line_complete_handler);
		}
		else
		{
			bk_jpeg_dec_isr_register(DEC_END_OF_FRAME, jpeg_decode_frame_complete_handler);
		}
	}
	else
	{
		// not adapt
	}
}

bool check_jpeg_decode_task_is_open(void)
{
	if (jdec_config == NULL)
	{
		return false;
	}
	else
	{
		return jdec_config->task_state;
	}
}

void decoder_mux_dump(void)
{
	LOGI("%s, timeout [%d, %d, %d : %d, %d] [%d, %d, %d : %d, %d], line: %d\n",
		__func__,
		jdec_config->mux_buf[0].state[PIPELINE_MOD_H264],
		jdec_config->mux_buf[0].state[PIPELINE_MOD_ROTATE],
		jdec_config->mux_buf[0].state[PIPELINE_MOD_SCALE],
		jdec_config->mux_buf[0].buffer.index,
		jdec_config->mux_buf[0].buffer.ok,
		jdec_config->mux_buf[1].state[PIPELINE_MOD_H264],
		jdec_config->mux_buf[1].state[PIPELINE_MOD_ROTATE],
		jdec_config->mux_buf[1].state[PIPELINE_MOD_SCALE],
		jdec_config->mux_buf[1].buffer.index,
		jdec_config->mux_buf[1].buffer.ok,
		jdec_config->jdec_line_count);

	LOGD("%s, %p %p %p %p\n",
		__func__,
		&jdec_config->mux_buf[0],
		jdec_config->mux_buf[0].buffer.data,
		&jdec_config->mux_buf[1],
		jdec_config->mux_buf[1].buffer.data);
}


static void decoder_timer_handle(void *arg1, void *arg2)
{
	LOGI("%s, timeout\n", __func__);
	decoder_mux_dump();
	jpeg_decode_task_send_msg(JPEGDEC_RESET, 0);
}

bk_err_t jpeg_decode_task_open(media_decode_mode_t jdec_mode, media_decode_type_t jdec_type, media_rotate_t rotate_angle)
{
	int ret = BK_OK;
	LOGI("%s, %d-%d\r\n", __func__, jdec_mode, jdec_type);

	rtos_lock_mutex(&jdec_info->lock);

	// step 1: check jdec_task state
	if (jdec_config != NULL && jdec_config->task_state)
	{
		LOGE("%s have been opened!\r\n", __func__);
		rtos_unlock_mutex(&jdec_info->lock);
		return ret;
	}

	// step 2: init jdec config
	jdec_config = (jdec_config_t *)os_malloc(sizeof(jdec_config_t));
	if (jdec_config == NULL)
	{
		LOGE("%s, malloc jdec_config failed\r\n", __func__);
		rtos_unlock_mutex(&jdec_info->lock);
		return BK_FAIL;
	}

	os_memset(jdec_config, 0, sizeof(jdec_config_t));

	jdec_config->rotate_angle = rotate_angle;
	jpeg_get_task_open();


	if (jdec_type == JPEGDEC_BY_LINE)
	{
		jdec_config->decoder_buf = mux_sram_buffer->decoder;
		LOGI("%s decode sram %p\n", __func__, jdec_config->decoder_buf);
	}

	jdec_config->jdec_mode = jdec_mode;
	jdec_config->jdec_type = jdec_type;

	if (!rtos_is_oneshot_timer_init(&jdec_config->decoder_timer))
	{
		ret = rtos_init_oneshot_timer(&jdec_config->decoder_timer, 200, decoder_timer_handle, NULL, NULL);

		if (ret != BK_OK)
		{
			LOGE("create decoder timer failed\n");
		}
	}


	media_debug->isr_decoder = 0;
	media_debug->err_dec = 0;

	ret = rtos_init_semaphore(&jdec_config->jdec_sem, 1);

	if (ret != BK_OK)
	{
		LOGE("%s, init jdec_config->jdec_sem failed\r\n", __func__);
		goto error;
	}

	ret = rtos_init_semaphore(&jdec_config->jdec_cp2_init_sem, 1);

	if (ret != BK_OK)
	{
		LOGE("%s, init jdec_config->jdec_cp2_init_sem failed\r\n", __func__);
		goto error;
	}

	// step 4: init jpeg_dec driver
	jpeg_decode_init();
#if CONFIG_SOC_BK7256XX
#else
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_DECODER, PM_CPU_FRQ_480M);
#endif

	// step 5: init jdec_task
	frame_buffer_fb_register(MODULE_DECODER, FB_INDEX_JPEG);

	INIT_LIST_HEAD(&jdec_config->jpeg_decode_queue);
	ret = software_decode_task_open();
	if (ret != BK_OK)
	{
		LOGE("%s, software_decode_task_open failed\r\n", __func__);
		goto error;
	}

	ret = rtos_init_queue(&jdec_config->jdec_queue,
							"jdec_que",
							sizeof(jpeg_msg_t),
							15);

	if (ret != BK_OK)
	{
		LOGE("%s, init jdec_queue failed\r\n", __func__);
		goto error;
	}

	ret = rtos_create_thread(&jdec_config->jdec_thread,
							BEKEN_DEFAULT_WORKER_PRIORITY - 1,
							"jdec_task",
							(beken_thread_function_t)jpeg_decode_main,
							1024 * 4,
							NULL);

	if (ret != BK_OK)
	{
		LOGE("%s, init jdec_task failed\r\n", __func__);
		goto error;
	}

	rtos_get_semaphore(&jdec_config->jdec_sem, BEKEN_NEVER_TIMEOUT);

	rtos_unlock_mutex(&jdec_info->lock);
    DECODER_LINE_END();
	DECODER_FRAME_END();

	return ret;

error:

	LOGE("%s, open failed\r\n", __func__);

	jpeg_decode_task_deinit();

	rtos_unlock_mutex(&jdec_info->lock);

	return ret;
}

bk_err_t jpeg_decode_task_close()
{
	LOGI("%s  %d\n", __func__, __LINE__);

	rtos_lock_mutex(&jdec_info->lock);

	if (jdec_config == NULL || !jdec_config->task_state)
	{
		rtos_unlock_mutex(&jdec_info->lock);
		return BK_FAIL;
	}

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	jdec_config->task_state = false;
	GLOBAL_INT_RESTORE();

	jpeg_get_task_close();

	if(check_software_decode_task_is_open())
	{
		software_decode_task_close();
	}

	rtos_unlock_mutex(&jdec_info->lock);

	jpeg_decode_task_send_msg(JPEGDEC_STOP, 0);
	rtos_get_semaphore(&jdec_config->jdec_sem, BEKEN_NEVER_TIMEOUT);

	if (jdec_config->sw_dec_init == 1)
	{
		vote_stop_cpu2_core(CPU2_USER_JPEG_SW_DEC);
	}

	rtos_lock_mutex(&jdec_info->lock);
	jpeg_decode_task_deinit();
	rtos_unlock_mutex(&jdec_info->lock);

	LOGI("%s complete, %d\n", __func__, __LINE__);

	return BK_OK;
}

void bk_jdec_buffer_request_register(pipeline_module_t module, mux_request_callback_t cb, mux_reset_callback_t reset_cb)
{
	LOGI("%s module: %d\n", __func__, module);

	rtos_lock_mutex(&jdec_info->lock);

	if (jdec_config)
	{

		GLOBAL_INT_DECLARATION();
		GLOBAL_INT_DISABLE();
		jdec_config->module[module].enable = true;
		jdec_config->cb[module] = cb;
		jdec_config->reset_cb[module] = reset_cb;
		GLOBAL_INT_RESTORE();

		if (jdec_config->trigger == false)
		{
			jpeg_get_task_send_msg(JPEGDEC_START, MODULE_DECODER);
			jdec_config->trigger = true;
		}
	}
	rtos_unlock_mutex(&jdec_info->lock);
}

void bk_jdec_buffer_request_deregister(pipeline_module_t module)
{
	LOGI("%s module: %d\n", __func__, module);

	rtos_lock_mutex(&jdec_info->lock);

	if (jdec_config)
	{

		GLOBAL_INT_DECLARATION();
		GLOBAL_INT_DISABLE();
		jdec_config->cb[module] = NULL;
        jdec_config->reset_cb[module] = NULL;
		jdec_config->module[module].enable = false;
		jdec_config->module[module].start = false;
		GLOBAL_INT_RESTORE();

		if (module < PIPELINE_MOD_LINE_MAX)
		{
			int event = -1;

			if (module == PIPELINE_MOD_H264)
			{
				event = JPEGDEC_H264_NOTIFY;
			}
			else if (module == PIPELINE_MOD_ROTATE)
			{
				event = JPEGDEC_ROTATE_NOTIFY;
			}
			else if (module == PIPELINE_MOD_SCALE)
			{
				event = JPEGDEC_SCALE_NOTIFY;
			}

			if (event != -1)
			{
				for (int i = 0; i < MUX_MAX; i++)
				{
					if (jdec_config->mux_buf[i].state[module] == MUX_BUFFER_PRESET
						&& jdec_config->mux_buf[i].encoded == true)
					{
						jdec_config->mux_buf[i].state[module] = MUX_BUFFER_RELEASE;

						complex_buffer_t *buffer = os_malloc(sizeof(complex_buffer_t));
						os_memcpy(buffer, &jdec_config->mux_buf[i].buffer, sizeof(complex_buffer_t));
						jpeg_decode_task_send_msg(event, (uint32_t)buffer);
					}
				}
			}
		}

	}

	rtos_unlock_mutex(&jdec_info->lock);
}



bk_err_t bk_jdec_pipeline_init(void)
{
	bk_err_t ret = BK_FAIL;

    if(jdec_info != NULL)
    {
        os_free(jdec_info);
        jdec_info = NULL;
    }
	jdec_info = (jdec_info_t*)os_malloc(sizeof(jdec_info_t));

	if (jdec_info == NULL)
	{
		LOGE("%s malloc rotates_info failed\n", __func__);
		return BK_FAIL;
	}

	os_memset(jdec_info, 0, sizeof(jdec_info_t));

	ret = rtos_init_mutex(&jdec_info->lock);

	if (ret != BK_OK)
	{
		LOGE("%s, init mutex failed\r\n", __func__);
		return BK_FAIL;
	}

	return ret;
}

uint8_t *jdec_decode_get_yuv_buffer(void)
{
	if (jdec_config == NULL || !jdec_config->task_state)
	{
		LOGE("%s not opened!\r\n", __func__);
		return NULL;
	}

	return jdec_config->decoder_buf;
}
