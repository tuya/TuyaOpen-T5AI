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
#include <components/log.h>
#include <driver/lcd.h>

#include "media_mailbox_list_util.h"
#include "media_evt.h"

#include "yuv_encode.h"
#include "uvc_pipeline_act.h"

#include "mux_pipeline.h"

#define TAG "uvc_pipe"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static pixel_format_t lcd_fmt = PIXEL_FMT_UNKNOW;
static media_rotate_t pipeline_rotate = ROTATE_90;
static uint8_t lcd_scale = 0;

extern uint8_t *media_bt_share_buffer;
mux_sram_buffer_t *mux_sram_buffer = NULL;

static bk_err_t h264_jdec_pipeline_open(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;

	media_camera_device_t device = DEFAULT_CAMERA_CONFIG();
	// step 1: init h264_encode_task
	device.type = UVC_CAMERA;
	device.mode = H264_MODE;
	device.fmt = PIXEL_FMT_H264;
	ret = h264_encode_task_open(&device);
	if (ret != BK_OK)
	{
	    goto error;
	}

	// step 2: init jpeg_decode_task
	if (!check_jpeg_decode_task_is_open())
	{
		ret = jpeg_decode_task_open(JPEGDEC_HW_MODE, JPEGDEC_BY_LINE, pipeline_rotate);

		if (ret != BK_OK)
		{
			LOGE("%s %d jpeg_decode_task_open fail\n", __func__, __LINE__);
			goto error;
		}
		bk_jdec_buffer_request_register(PIPELINE_MOD_H264, bk_h264_encode_request, bk_h264_reset_request);
		LOGI("%s, jdec_h264_enc_en \n", __func__);
	}
	else
	{
		bk_jdec_buffer_request_register(PIPELINE_MOD_H264, bk_h264_encode_request, bk_h264_reset_request);
		LOGI("%s, jdec_h264_enc_en \n", __func__);
	}
	return ret;

error:
	bk_jdec_buffer_request_deregister(PIPELINE_MOD_H264);
    h264_encode_task_close();
    jpeg_decode_task_close();
    return BK_FAIL;
}

static bk_err_t h264_jdec_pipeline_close(media_mailbox_msg_t *msg)
{
	LOGI("%s %d\n", __func__, __LINE__);

	if (check_rotate_task_is_open() || check_lcd_task_is_open())
	{
		bk_jdec_buffer_request_deregister(PIPELINE_MOD_H264);
		LOGI("%s jdec_h264_enc_en = 0 %d \n", __func__, __LINE__);
		//rtos_delay_milliseconds(200);
		h264_encode_task_close();
	}
	else
	{
		bk_jdec_buffer_request_deregister(PIPELINE_MOD_H264);
		h264_encode_task_close();
		jpeg_decode_task_close();
		LOGI("%s decode task close complete \n", __func__);

	}
	LOGI("%s complete, %d \n", __func__, __LINE__);

	return BK_OK;
}

static bk_err_t lcd_set_fmt(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;

	uint32_t fmt = msg->param;
	lcd_fmt = fmt;
	LOGE("%s, fmt %x\n", __func__, lcd_fmt);

	return ret;
}

static bk_err_t pipeline_set_rotate(media_mailbox_msg_t *msg)
{
    pipeline_rotate = (uint32_t)msg->param;
    LOGI("%s, rotate angle = %d (0:0, 1:90,2:180,3:270)\r\n", __func__, pipeline_rotate);
	jpeg_decode_set_rotate_angle(pipeline_rotate);
	return BK_OK;
}

static bk_err_t lcd_disp_pipeline_open(media_mailbox_msg_t *msg)
{
    int ret = BK_OK;

    ret = lcd_display_open((lcd_open_t *)msg->param);
    if (ret != BK_OK)
    {
        LOGE("%s %d lcd display open fail\r\n", __func__, __LINE__);
    }

    return ret;
}

static bk_err_t lcd_disp_pipeline_close(media_mailbox_msg_t *msg)
{
    int ret = BK_OK;

    ret = lcd_display_close();

    return ret;

}

//Modified by TUYA Start
static bk_err_t lcd_sframe_open(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;
    extern void lcd_startup_frame_open(void *config);
    lcd_startup_frame_open((void *)msg->param);
    return ret;
}

static bk_err_t lcd_sframe_close(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;
    extern void lcd_startup_frame_close(void *config);
    lcd_startup_frame_close((void *)msg->param);
    return ret;
}
//Modified by TUYA End

static bk_err_t lcd_jdec_pipeline_open(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;
	rot_open_t rot_open = {0};

#if SUPPORTED_IMAGE_MAX_720P
	lcd_scale_t local_lcd_scale = {PPI_1280X720, PPI_864X480};  // {PPI_864X480, PPI_480X480}, {PPI_1280X720, PPI_864X480}, {PPI_640X480, PPI_480X800};{PPI_480X320, PPI_480X864};
	ret = scale_task_open(&local_lcd_scale);
	if (ret != BK_OK)
	{
		goto error;
	}
#endif

	if (lcd_fmt == PIXEL_FMT_UNKNOW || lcd_fmt == PIXEL_FMT_RGB565 || lcd_fmt == PIXEL_FMT_RGB565_LE)
	{
		rot_open.fmt = PIXEL_FMT_RGB565_LE;
		rot_open.mode = HW_ROTATE;
	}
	else
	{
		rot_open.mode = SW_ROTATE;
		rot_open.fmt = lcd_fmt;
	}

	rot_open.angle = pipeline_rotate;

	ret = rotate_task_open(&rot_open);

	if (ret != BK_OK)
	{
		LOGE("%s %d rotate_task_open fail\n", __func__, __LINE__);
		goto error;
	}

	if (!check_jpeg_decode_task_is_open())
	{
		ret = jpeg_decode_task_open(JPEGDEC_HW_MODE, JPEGDEC_BY_LINE, pipeline_rotate);

		if (ret != BK_OK)
		{
			LOGE("%s, jpeg_decode_task_open fail\n", __func__);
			return ret;
		}
#if SUPPORTED_IMAGE_MAX_720P
		bk_jdec_buffer_request_register(PIPELINE_MOD_SCALE, bk_scale_encode_request, bk_scale_reset_request);
#else
		bk_jdec_buffer_request_register(PIPELINE_MOD_ROTATE, bk_rotate_encode_request, bk_rotate_reset_request);
#endif
	}
	else
	{
#if SUPPORTED_IMAGE_MAX_720P
        bk_jdec_buffer_request_register(PIPELINE_MOD_SCALE, bk_scale_encode_request, bk_scale_reset_request);
#else
        bk_jdec_buffer_request_register(PIPELINE_MOD_ROTATE, bk_rotate_encode_request, bk_rotate_reset_request);
#endif
	}
	LOGI("%s %d\n", __func__, __LINE__);
	return ret;

error:
	LOGI("%s fail\n", __func__, __LINE__);
	rotate_task_close();

	return BK_FAIL;
}

static bk_err_t lcd_jdec_pipeline_close(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;

	LOGI("%s %d\n", __func__, __LINE__);

	if (check_h264_task_is_open())
	{
#if SUPPORTED_IMAGE_MAX_720P
		LOGI("%s deregister scale, %d \n", __func__, __LINE__);
		rotate_task_close();
		bk_jdec_buffer_request_deregister(PIPELINE_MOD_SCALE);
		scale_task_close();
#else
		LOGI("%s deregister rotate, %d \n", __func__, __LINE__);
		bk_jdec_buffer_request_deregister(PIPELINE_MOD_ROTATE);
		rotate_task_close();
#endif
	}
	else
	{
#if SUPPORTED_IMAGE_MAX_720P
		LOGI("%s deregister scale, %d \n", __func__, __LINE__);
		rotate_task_close();
		bk_jdec_buffer_request_deregister(PIPELINE_MOD_SCALE);
		scale_task_close();
#else
		LOGI("%s deregister rotate, %d \n", __func__, __LINE__);
		bk_jdec_buffer_request_deregister(PIPELINE_MOD_ROTATE);
		rotate_task_close();
#endif

		ret = jpeg_decode_task_close();
		if (ret != BK_OK)
		{
			LOGE("%s %d decode task close fail\n", __func__, __LINE__);
			return ret;
		}
		LOGI("%s decode task close complete, %d \n", __func__, __LINE__);
	}

	LOGI("%s complete, %d \n", __func__, __LINE__);

	return BK_OK;
}

static bk_err_t lcd_scale_pipeline_open(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;
	ret = scale_task_open((lcd_scale_t *)msg->param);
	if (ret != BK_OK)
	{
        goto error;
	}
    
    lcd_scale = 1;
	LOGI("%s complete, %d \n", __func__, __LINE__);
	return ret;
error:
    lcd_scale = 0;
    scale_task_close();
    return BK_FAIL;
}

static bk_err_t lcd_scale_pipline_close(media_mailbox_msg_t *msg)
{
	LOGI("%s %d\n", __func__, __LINE__);
    scale_task_close();
    lcd_scale = 0;
	return BK_OK;
}

#if CONFIG_MEDIA_UNIT_TEST
void pipeline_mem_show(void)
{
	uint32_t total_size,free_size,mini_size;
//	LOGI("================Static memory================\r\n");
//	os_show_memory_config_info();

	LOGI("================Dynamic memory================\r\n");
	LOGI("%-5s   %-5s   %-5s	 %-5s	%-5s\r\n",
		"name", "total", "free", "minimum", "peak");

	total_size = rtos_get_total_heap_size();
	free_size  = rtos_get_free_heap_size();
	mini_size  = rtos_get_minimum_free_heap_size();
	LOGI("heap\t%d\t%d\t%d\t%d\r\n",	total_size,free_size,mini_size,total_size-mini_size);

#if CONFIG_PSRAM_AS_SYS_MEMORY
	total_size = rtos_get_psram_total_heap_size();
	free_size  = rtos_get_psram_free_heap_size();
	mini_size  = rtos_get_psram_minimum_free_heap_size();
	LOGI("psram\t%d\t%d\t%d\t%d\r\n", total_size,free_size,mini_size,total_size-mini_size);
#endif
}

void pipeline_mem_leak(void)
{
	LOGI("%s %d\n", __func__, __LINE__);
#if CONFIG_MEM_DEBUG
	os_dump_memory_stats(0, 0, NULL);
#endif
}
#endif

// Modified by TUYA Start
media_rotate_t uvc_pipeline_get_rotate(void)
{
    return pipeline_rotate;
}
// Modified by TUYA End

void uvc_pipeline_event_handle(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_OK;

	switch (msg->event)
	{
		//Modified by TUYA Start
	    case EVENT_PIPELINE_STARTUP_OPEN_IND:
            ret = lcd_sframe_open(msg);
			break;

	    case EVENT_PIPELINE_STARTUP_CLOSE_IND:
            ret = lcd_sframe_close(msg);
			break;
		//Modified by TUYA End

		case EVENT_PIPELINE_LCD_DISP_OPEN_IND:
			ret = lcd_disp_pipeline_open(msg);
			break;

		case EVENT_PIPELINE_LCD_DISP_CLOSE_IND:
			ret = lcd_disp_pipeline_close(msg);
			break;

		case EVENT_PIPELINE_LCD_JDEC_OPEN_IND:
			ret = lcd_jdec_pipeline_open(msg);
			break;

		case EVENT_PIPELINE_LCD_JDEC_CLOSE_IND:
			ret = lcd_jdec_pipeline_close(msg);
			break;

		case EVENT_PIPELINE_SET_ROTATE_IND:
			ret = pipeline_set_rotate(msg);
			break;

		case EVENT_PIPELINE_H264_OPEN_IND:
			ret = h264_jdec_pipeline_open(msg);
			break;

		case EVENT_PIPELINE_H264_CLOSE_IND:
			ret = h264_jdec_pipeline_close(msg);
			break;

		case EVENT_PIPELINE_H264_RESET_IND:
			ret = h264_encode_regenerate_idr_frame();
			break;

		case EVENT_LCD_SET_FMT_IND:
			ret = lcd_set_fmt(msg);
			break;

		case EVENT_PIPELINE_LCD_SCALE_IND:
			ret = lcd_scale_pipeline_open(msg);
			break;

		case EVENT_PIPELINE_DUMP_IND:
			decoder_mux_dump();
			BK_ASSERT_EX(0, "dump for debug\n");
			ret = 0;
			break;

#if CONFIG_MEDIA_UNIT_TEST
		case EVENT_PIPELINE_MEM_SHOW_IND:
			pipeline_mem_show();
			break;

		case EVENT_PIPELINE_MEM_LEAK_IND:
			pipeline_mem_leak();
			break;
#endif

		default:
			break;
	}

	msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);
}

bk_err_t uvc_pipeline_init(void)
{
	mux_sram_buffer = (mux_sram_buffer_t *)media_bt_share_buffer;

	if (mux_sram_buffer == NULL)
	{
#ifdef CONFIG_BT_REUSE_MEDIA_MEMORY
		LOGE("%s, BT_REUSE_MEDIA_MEMORY mux_sram_buffer failed\r\n", __func__);
#endif
		mux_sram_buffer = (mux_sram_buffer_t *)os_malloc(sizeof(mux_sram_buffer_t));

		if (mux_sram_buffer == NULL)
		{
			LOGE("%s, malloc mux_sram_buffer failed\r\n", __func__);
		}
	}

	LOGI("%s mux_sram_buffer_t: %d\n", __func__, sizeof(mux_sram_buffer_t));

	bk_jdec_pipeline_init();

#if SUPPORTED_IMAGE_MAX_720P
	bk_scale_pipeline_init();
#endif

	bk_rotate_pipeline_init();
	bk_h264_pipeline_init();

	return BK_OK;
}



