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

#include <common/bk_err.h>
#include <common/bk_include.h>

#include <os/os.h>
#include <os/mem.h>
#include <stdio.h>
#include <driver/uvc_camera_types.h>
#include <driver/uvc_camera.h>
#include <driver/media_types.h>
#include <driver/dma.h>
#include <driver/gpio.h>
#include <driver/video_common_driver.h>
#include "bk_general_dma.h"
#include <components/uvc_uac_api_common.h>
#include <components/usb.h>
#include "gpio_driver.h"
// Modified by TUYA Start
#include "gpio_map.h"
#include "tkl_video_in.h"
// Modified by TUYA End
#include "bk_misc.h"
#include <modules/pm.h>
#include <driver/timer.h>
#include <common/bk_assert.h>


#define UVC_PARAM_ERROR           (0xFF)

typedef struct
{
	uint8_t  eof : 1;
	uint8_t  frame_flag : 1;
	uint8_t  packet_err : 1;
	uint8_t  format_index;
	uint8_t  frame_num;
	uint8_t  frame_index;
	uint8_t  frame_fps;
	uint8_t  transfer_mode;
	uvc_state_t uvc_state;
	uint32_t total_packet_cnt;
	uint32_t ok_packet_cnt;
	uint32_t error_packet_cnt;
	uint32_t emp_packet_cnt;
	uint32_t frame_eof_not_ffd9_cnt;
	uint32_t frame_end_not_ffd9_cnt;
	uint32_t i_frame;
	uint32_t p_frame;
} uvc_driver_t;

typedef enum {
	UVC_SET_PARAM_IND,
	UVC_START_IND,
	UVC_STOP_IND,
	UVC_EOF_IND,
	UVC_CONNECTED_IND,
	UVC_DISCONNECTED_IND,
	UVC_EXIT_IND,
	UVC_PACKET_DEAL_IND,
	UVC_PACKET_DUAL_DEAL_IND,
	UVC_LOG_IND,
	UVC_UNKNOW,
}uvc_msg_type_t;

#define TAG "uvc_drv"

#define UVC_LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define UVC_LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define UVC_LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define UVC_LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

//#define UVC_DIAG_DEBUG

#ifdef UVC_DIAG_DEBUG

#define UVC_DIAG_DEBUG_INIT()                    \
	do {                                         \
		gpio_dev_unmap(GPIO_32);                 \
		bk_gpio_disable_pull(GPIO_32);           \
		bk_gpio_enable_output(GPIO_32);          \
		bk_gpio_set_output_low(GPIO_32);         \
		\
		gpio_dev_unmap(GPIO_33);                 \
		bk_gpio_disable_pull(GPIO_33);           \
		bk_gpio_enable_output(GPIO_33);          \
		bk_gpio_set_output_low(GPIO_33);         \
		\
		gpio_dev_unmap(GPIO_34);                 \
		bk_gpio_disable_pull(GPIO_34);           \
		bk_gpio_enable_output(GPIO_34);          \
		bk_gpio_set_output_low(GPIO_34);         \
		\
		gpio_dev_unmap(GPIO_35);                 \
		bk_gpio_disable_pull(GPIO_35);           \
		bk_gpio_enable_output(GPIO_35);          \
		bk_gpio_set_output_low(GPIO_35);         \
		\
		gpio_dev_unmap(GPIO_36);                 \
		bk_gpio_disable_pull(GPIO_36);           \
		bk_gpio_enable_output(GPIO_36);          \
		bk_gpio_set_output_low(GPIO_36);         \
		\
		gpio_dev_unmap(GPIO_37);                 \
		bk_gpio_disable_pull(GPIO_37);           \
		bk_gpio_enable_output(GPIO_37);          \
		bk_gpio_set_output_low(GPIO_37);         \
		\
	} while (0)

#define UVC_MALLOC_ENTRY()              bk_gpio_set_output_high(GPIO_32)
#define UVC_MALLOC_OUT()                bk_gpio_set_output_low(GPIO_32)

#define UVC_PUSH_ENTRY()                bk_gpio_set_output_high(GPIO_33)
#define UVC_PUSH_OUT()                  bk_gpio_set_output_low(GPIO_33)

#define UVC_PACKET_ENTRY()              bk_gpio_set_output_high(GPIO_34)
#define UVC_PACKET_OUT()                bk_gpio_set_output_low(GPIO_34)

#define UVC_PER_PACKET_ENTRY()          bk_gpio_set_output_high(GPIO_35)
#define UVC_PER_PACKET_OUT()            bk_gpio_set_output_low(GPIO_35)

#define UVC_PSRAM_DMA_ENTRY()           bk_gpio_set_output_high(GPIO_36)
#define UVC_PSRAM_DMA_OUT()             bk_gpio_set_output_low(GPIO_36)

#define UVC_JPEG_EOF_ENTRY()            bk_gpio_set_output_high(GPIO_37)
#define UVC_JPEG_EOF_OUT()              bk_gpio_set_output_low(GPIO_37)

#else

#define UVC_DIAG_DEBUG_INIT()

#define UVC_MALLOC_ENTRY()
#define UVC_MALLOC_OUT()

#define UVC_PUSH_ENTRY()
#define UVC_PUSH_OUT()

#define UVC_PACKET_ENTRY()
#define UVC_PACKET_OUT()

#define UVC_PER_PACKET_ENTRY()
#define UVC_PER_PACKET_OUT()

#define UVC_JPEG_EOF_ENTRY()
#define UVC_JPEG_EOF_OUT()

#define UVC_PSRAM_DMA_ENTRY()
#define UVC_PSRAM_DMA_OUT()

#endif

#define UVC_TASK_BLOCK_DEBUG

static beken_semaphore_t uvc_class_task_sem  = NULL;

extern media_debug_t *media_debug;

volatile uint32_t g_n_uvc_dev = 0;

static beken_thread_t uvc_class_task = NULL;
static beken_queue_t uvc_msg_queue = NULL;
static frame_buffer_t *curr_frame_buffer = NULL;
static frame_buffer_t *curr_frame_buffer_dual = NULL;

static uvc_driver_t *uvc_camera_drv[2] = {NULL, NULL};
static uvc_camera_config_t *uvc_camera_config_ptr = NULL;
static uint32_t uvc_frame_id[2] = {0, 0};
static bk_uvc_device_brief_info_t uvc_device_info[2];
static uvc_device_info_t device_info_cb = NULL;

beken_timer_t uvc_debug_timer = {0};

static bk_err_t uvc_send_msg(uint8_t type, uint32_t param)
{
	bk_err_t ret = kNoErr;
	uvc_msg_t msg;

#ifdef UVC_TASK_BLOCK_DEBUG
	static uint32_t uvc_send_msg_fail_count = 0;
#endif

	if (uvc_msg_queue)
	{
		msg.type = type;
		msg.data = param;

		ret = rtos_push_to_queue(&uvc_msg_queue, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret)
		{
			UVC_LOGE("uvc_msg_que failed, type:%d\r\n", type);
#ifdef UVC_TASK_BLOCK_DEBUG
		uvc_send_msg_fail_count++;
		if (uvc_send_msg_fail_count > 200)
		{
			UVC_LOGE("uvc_class task is block, dump for debug \n");
			BK_ASSERT(false);
		}
#endif
			return kNoResourcesErr;
		}
#ifdef UVC_TASK_BLOCK_DEBUG
		else
		{
			uvc_send_msg_fail_count = 0;
		}
#endif
		return ret;
	}
	return kGeneralErr;
}


static void uvc_camera_eof_handle(uint32_t idx_uvc)
{
	// frame_buffer complete
	UVC_JPEG_EOF_ENTRY();

	if (uvc_camera_drv[idx_uvc]->packet_err || curr_frame_buffer->length == 0)
	{
		uvc_camera_drv[idx_uvc]->packet_err = false;
		uvc_camera_drv[idx_uvc]->eof = false;
		curr_frame_buffer->length = 0;

		media_debug->uvc_error++;
		UVC_JPEG_EOF_OUT();
		return;
	}

	if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == JPEG_MODE)
	{
		media_debug->isr_jpeg++;
		media_debug->jpeg_length = curr_frame_buffer->length;
		media_debug->jpeg_kbps  += curr_frame_buffer->length;
	}
	else
	{
		media_debug->isr_h264++;
		media_debug->h264_length = curr_frame_buffer->length;
		media_debug->h264_kbps += curr_frame_buffer->length;
	}

	curr_frame_buffer->sequence = uvc_frame_id[idx_uvc]++;

	UVC_LOGD("%s, %02x-%02x, %02x-%02x, %d\r\n", __func__, curr_frame_buffer->frame[0], curr_frame_buffer->frame[1],
		curr_frame_buffer->frame[curr_frame_buffer->length - 2], curr_frame_buffer->frame[curr_frame_buffer->length - 1], curr_frame_buffer->length);
	uvc_camera_config_ptr->jpeg_cb.push(curr_frame_buffer);
	uvc_camera_drv[idx_uvc]->eof = false;

	UVC_JPEG_EOF_OUT();


	if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == JPEG_MODE)
	{
		curr_frame_buffer = uvc_camera_config_ptr->jpeg_cb.malloc(FB_INDEX_JPEG, CONFIG_JPEG_FRAME_SIZE);
	}
	else
	{
		curr_frame_buffer = uvc_camera_config_ptr->jpeg_cb.malloc(FB_INDEX_H264, CONFIG_H264_FRAME_SIZE);
	}

	if (curr_frame_buffer == NULL || curr_frame_buffer->frame == NULL)
	{
		UVC_LOGE("alloc frame error\n");
		return;
	}

	curr_frame_buffer->length = 0;
	curr_frame_buffer->type   = uvc_camera_config_ptr->device.type;
	curr_frame_buffer->width  = uvc_camera_config_ptr->device.uvc_device[idx_uvc].info.resolution.width;
	curr_frame_buffer->height = uvc_camera_config_ptr->device.uvc_device[idx_uvc].info.resolution.height;
	curr_frame_buffer->fmt    = uvc_camera_config_ptr->device.uvc_device[idx_uvc].fmt;
}

static void uvc_camera_process_payload(uint8_t *curptr, uint32_t newlen, uint32_t idx_uvc)
{
	uint8_t *data = NULL;
	uint8_t bmhead_info = 0;
	uint8_t head_len = 0;
	uint32_t data_len = newlen;
	int check_len = 0;
	bool bulk_trans = false;

#ifdef CONFIG_UVC_CHECK_BULK_JPEG_HEADER
	bulk_trans = uvc_camera_drv[idx_uvc]->transfer_mode;
#endif

	UVC_PER_PACKET_ENTRY();

	if (curr_frame_buffer == NULL
		|| curr_frame_buffer->frame == NULL)
	{
		UVC_LOGE("curr_frame_buffer NULL\n");
		UVC_PER_PACKET_OUT();
		return;
	}

	if (newlen == 0)
	{
		uvc_camera_drv[idx_uvc]->emp_packet_cnt++;
		UVC_PER_PACKET_OUT();
		return;
	}

	// step 2: judge there have uvc header
	// make sure this is a header, judge from header length and bit field
	// For SCR, PTS, some vendors not set bit, but also offer 12 Bytes header. so we just check SET condition
	if (newlen >= curptr[0]
			&& (curptr[0] == 12 || (curptr[0] == 2 && !(curptr[1] & 0x0C)) || (curptr[0] == 6 && !(curptr[1] & 0x08)))
			&& (curptr[1] & 0x80) && !(curptr[1] & 0x30)
#ifdef CONFIG_UVC_CHECK_BULK_JPEG_HEADER
			&& (!bulk_trans || ((curptr[curptr[0]] == 0xff) && (curptr[curptr[0] + 1] == 0xd8)))
#endif
		)
	{
		head_len    = curptr[0];
		data_len   -= head_len;
		bmhead_info = curptr[1];
	}

	// step 3: prase head
	if (bmhead_info)
	{
		// step 3.1: check this packet is ok
		if (bmhead_info & 0x40)  // bit6 = 1, payload error
		{
			uvc_camera_drv[idx_uvc]->packet_err = true;
			uvc_camera_drv[idx_uvc]->error_packet_cnt++;
			UVC_PER_PACKET_OUT();
			return;
		}

		// step 3.2: check is a new frame
		if (uvc_camera_drv[idx_uvc]->frame_flag != (bmhead_info & 0x01))   // bit0 fliped
		{
			uvc_camera_drv[idx_uvc]->frame_flag = (bmhead_info & 0x01);
			if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == JPEG_MODE)
			{
				if (curr_frame_buffer->length != 0)
				{
					check_len = bk_uvc_camera_jpeg_frame_check_eof(curr_frame_buffer->frame, curr_frame_buffer->length);
					if (check_len > 0)
					{
						curr_frame_buffer->length = check_len;
						uvc_camera_drv[idx_uvc]->eof = true;
					}
					else
					{
						UVC_LOGD("%s %d length:%d buffer not end with 0XFFD9\n", __func__, __LINE__, curr_frame_buffer->length);
						uvc_camera_drv[idx_uvc]->frame_end_not_ffd9_cnt++;
					}
				}
			}
			else
			{
				uvc_camera_drv[idx_uvc]->eof = true;
			}
		}
	}

	// step 4: check if current packet is new frame
	if (uvc_camera_drv[idx_uvc]->eof)
	{
		uvc_camera_eof_handle(idx_uvc);
	}

	// add protect
	if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == JPEG_MODE)
	{
		if ((curr_frame_buffer->length + data_len) > (CONFIG_JPEG_FRAME_SIZE - 1024))
		{
			uvc_camera_drv[idx_uvc]->packet_err = true;
		}
	}
	else
	{
		if ((curr_frame_buffer->length + data_len) > (CONFIG_H264_FRAME_SIZE - 1024))
		{
			uvc_camera_drv[idx_uvc]->packet_err = true;
		}
	}

	data = &curptr[head_len];

	if (data_len == 0)
	{
		uvc_camera_drv[idx_uvc]->emp_packet_cnt++;
		UVC_PER_PACKET_OUT();
		return;
	}

	UVC_LOGD("length:%d-%d %02x-%02x-%02x-%02x-%02x-%02x\r\n", data_len, curr_frame_buffer->length, data[0], data[1], data[2], data[3], data[4], data[5]);
	if (uvc_camera_drv[idx_uvc]->transfer_mode != USB_ENDPOINT_BULK_TRANSFER)
	{
		if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == JPEG_MODE)
		{
			if (data[0] == 0xFF && data[1] == 0xD8)
			{
				if (uvc_camera_drv[idx_uvc]->packet_err)
				{
					uvc_camera_drv[idx_uvc]->packet_err = false;
					media_debug->uvc_error++;
					uvc_camera_drv[idx_uvc]->error_packet_cnt++;
				}
				uvc_camera_drv[idx_uvc]->eof = false;
				curr_frame_buffer->length = 0;
			}
		}
		else
		{
			// for h264
			if (data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 1)
			{
				uvc_camera_drv[idx_uvc]->packet_err = false;
				uvc_camera_drv[idx_uvc]->eof = false;
				curr_frame_buffer->length = 0;
			}
		}
	}

	if (uvc_camera_drv[idx_uvc]->packet_err)
	{
		uvc_camera_drv[idx_uvc]->error_packet_cnt++;
		UVC_PER_PACKET_OUT();
		return;
	}

	// step 5: compy payload to frame_buffer
	if (data_len > 0)
	{
		UVC_PSRAM_DMA_ENTRY();
		os_memcpy(curr_frame_buffer->frame + curr_frame_buffer->length, data, data_len);
		curr_frame_buffer->length += data_len;
		UVC_PSRAM_DMA_OUT();
	}

	// step 3.2: check is a new frame
	if ((bmhead_info & 0x02) && !bulk_trans)   // bit1 is 1, eof bit is setted
	{
		if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == JPEG_MODE)
		{
			check_len = bk_uvc_camera_jpeg_frame_check_eof(curr_frame_buffer->frame, curr_frame_buffer->length);
			if (check_len > 0)
			{
				uvc_camera_drv[idx_uvc]->frame_flag = !(bmhead_info & 0x01);
				curr_frame_buffer->length = check_len;
				uvc_camera_drv[idx_uvc]->eof = true;
			}
			else
			{
				UVC_LOGD("%s %d buffer not end with 0XFFD9\n", __func__, __LINE__);
				uvc_camera_drv[idx_uvc]->frame_eof_not_ffd9_cnt++;
			}
		}
		else
		{
			uvc_camera_drv[idx_uvc]->frame_flag = !(bmhead_info & 0x01);
			// maybe first packet will satify this condition
			uvc_camera_drv[idx_uvc]->eof = true;
		}

		if (uvc_camera_drv[idx_uvc]->eof)
		{
			uvc_camera_eof_handle(idx_uvc);
		}
	}

	uvc_camera_drv[idx_uvc]->ok_packet_cnt++;

	UVC_PER_PACKET_OUT();
}

static void uvc_camera_process_packet_handler(camera_packet_t *camera_packet)
{
	uint8_t *data = NULL;
	uint32_t idx_uvc = camera_packet->uvc_dev_id;
	UVC_PACKET_ENTRY();
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	if (camera_packet->packet_state == CAM_STREAM_ERROR)
	{
		uvc_camera_drv[idx_uvc]->packet_err = true;
		GLOBAL_INT_RESTORE();
		UVC_LOGD("%s, %p, %d\r\n", __func__, camera_packet->data_buffer, __LINE__);
		uvc_camera_drv[idx_uvc]->error_packet_cnt += 8;
		uvc_camera_drv[idx_uvc]->total_packet_cnt += 8;
		goto out;
	}

	//UVC_LOGI("%s, %p, %d\r\n", __func__, camera_packet->data_buffer, camera_packet->num_packets);

	camera_packet->packet_state = CAM_STREAM_BUSY;
	GLOBAL_INT_RESTORE();

	for (uint8_t i = 0; i < camera_packet->num_packets; i++)
	{
		uvc_camera_drv[idx_uvc]->total_packet_cnt++;
		if (camera_packet->state[i] == CAM_STREAM_READY)
		{
			data = camera_packet->data_buffer + i * camera_packet->num_byte[i];
			uvc_camera_process_payload(data, (uint32_t)camera_packet->actual_num_byte[i], idx_uvc);
		}
		else
		{
			uvc_camera_drv[idx_uvc]->packet_err = true;
			uvc_camera_drv[idx_uvc]->error_packet_cnt++;
		}
	}

out:
	bk_video_camera_packet_free(camera_packet);

	UVC_PACKET_OUT();
}

static void uvc_camera_eof_handle_dual(uint32_t idx_uvc)
{
	// frame_buffer complete
	UVC_JPEG_EOF_ENTRY();

	if (uvc_camera_drv[idx_uvc]->packet_err || curr_frame_buffer_dual->length == 0)
	{
		uvc_camera_drv[idx_uvc]->packet_err = false;
		uvc_camera_drv[idx_uvc]->eof        = false;
		curr_frame_buffer_dual->length      = 0;

		media_debug->uvc_h264_error++;
		UVC_JPEG_EOF_OUT();
		return;
	}

	if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == H264_MODE ||
			uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == H265_MODE )
	{

		media_debug->isr_h264++;
		media_debug->h264_length = curr_frame_buffer_dual->length;
		media_debug->h264_kbps  += curr_frame_buffer_dual->length;
	}

	curr_frame_buffer_dual->sequence = uvc_frame_id[idx_uvc]++;

	uvc_camera_config_ptr->h264_cb.push(curr_frame_buffer_dual);

	UVC_LOGD("H_len : %d\n", curr_frame_buffer_dual->length);
	uvc_camera_drv[idx_uvc]->eof = false;

	UVC_JPEG_EOF_OUT();

	if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == JPEG_MODE)
	{
		//curr_frame_buffer = uvc_camera_config_ptr->jpeg_cb.malloc(FB_INDEX_JPEG, CONFIG_JPEG_FRAME_SIZE);
	}
	else if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == H264_MODE)
	{
		curr_frame_buffer_dual = uvc_camera_config_ptr->h264_cb.malloc(FB_INDEX_H264, CONFIG_H264_FRAME_SIZE);
	}
	else if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == H265_MODE)
	{
		curr_frame_buffer_dual = uvc_camera_config_ptr->h264_cb.malloc(FB_INDEX_H265, CONFIG_H264_FRAME_SIZE);
	}

	if (curr_frame_buffer_dual == NULL || curr_frame_buffer_dual->frame == NULL)
	{
		UVC_LOGE("alloc frame error\n");
		return;
	}

	curr_frame_buffer_dual->length = 0;
	curr_frame_buffer_dual->type   = uvc_camera_config_ptr->device.type;
	curr_frame_buffer_dual->width  = uvc_camera_config_ptr->device.uvc_device[idx_uvc].info.resolution.width;
	curr_frame_buffer_dual->height = uvc_camera_config_ptr->device.uvc_device[idx_uvc].info.resolution.height;
	curr_frame_buffer_dual->fmt    = uvc_camera_config_ptr->device.uvc_device[idx_uvc].fmt;
	UVC_LOGD("w : %d, h : %d\n", curr_frame_buffer_dual->width, curr_frame_buffer_dual->height);
}

static void uvc_camera_process_payload_dual(uint8_t *curptr, uint32_t newlen, uint32_t idx_uvc)
{
	uint8_t *data = NULL;
	uint8_t bmhead_info = 0;
	uint8_t head_len = 0;
	uint32_t data_len = newlen;
	int check_len = 0;

	UVC_PER_PACKET_ENTRY();

	if (curr_frame_buffer_dual == NULL || curr_frame_buffer_dual->frame == NULL)
	{
		UVC_LOGE("curr_frame_buffer_dual NULL\n");
		UVC_PER_PACKET_OUT();
		return;
	}

	if (newlen == 0)
	{
		uvc_camera_drv[idx_uvc]->emp_packet_cnt++;
		UVC_PER_PACKET_OUT();
		return;
	}

	// step 2: judge there have uvc header
	// make sure this is a header, judge from header length and bit field
	// For SCR, PTS, some vendors not set bit, but also offer 12 Bytes header. so we just check SET condition
	if (newlen >= curptr[0]
			&& (curptr[0] == 12 || (curptr[0] == 2 && !(curptr[1] & 0x0C)) || (curptr[0] == 6 && !(curptr[1] & 0x08)))
			&& (curptr[1] & 0x80) && !(curptr[1] & 0x30))
	{
		head_len    = curptr[0];
		data_len   -= head_len;
		bmhead_info = curptr[1];
	}

	// step 3: prase head
	if (bmhead_info)
	{
		// step 3.1: check this packet is ok
		if (bmhead_info & 0x40)  // bit6 = 1, payload error
		{
			uvc_camera_drv[idx_uvc]->packet_err = true;
			uvc_camera_drv[idx_uvc]->error_packet_cnt++;
			UVC_PER_PACKET_OUT();
			return;
		}

		// step 3.2: check is a new frame
		if (uvc_camera_drv[idx_uvc]->frame_flag != (bmhead_info & 0x01))   // bit0 fliped
		{
			uvc_camera_drv[idx_uvc]->frame_flag = (bmhead_info & 0x01);
			if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == JPEG_MODE)
			{
				if (curr_frame_buffer->length != 0)
				{
					check_len = bk_uvc_camera_jpeg_frame_check_eof(curr_frame_buffer->frame, curr_frame_buffer->length);
					if (check_len > 0)
					{
						curr_frame_buffer->length = check_len;
						uvc_camera_drv[idx_uvc]->eof = true;
					}
					else
					{
						UVC_LOGD("%s %d buffer not end with 0XFFD9\n", __func__, __LINE__);
						uvc_camera_drv[idx_uvc]->frame_end_not_ffd9_cnt++;
					}
				}
			}
			else if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == H264_MODE ||
					uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == H265_MODE)
			{
				uvc_camera_drv[idx_uvc]->eof = true;
			}
		}
	}

	// step 4: check if current packet is new frame
	if (uvc_camera_drv[idx_uvc]->eof)
	{
		uvc_camera_eof_handle_dual(idx_uvc);
	}

	// add protect
	if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == JPEG_MODE)
	{
		if ((curr_frame_buffer->length + data_len) > (CONFIG_JPEG_FRAME_SIZE - 1024))
		{
			uvc_camera_drv[idx_uvc]->packet_err = true;
		}
	}
	else if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == H264_MODE ||
			uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == H265_MODE)
	{
		if ((curr_frame_buffer_dual->length + data_len) > (CONFIG_H264_FRAME_SIZE - 1024))
		{
			uvc_camera_drv[idx_uvc]->packet_err = true;
		}
	}

	data = &curptr[head_len];

	if (data_len == 0)
	{
		uvc_camera_drv[idx_uvc]->emp_packet_cnt++;
		UVC_PER_PACKET_OUT();
		return;
	}

	//UVC_LOGD("length:%d-%d %02x-%02x-%02x-%02x-%02x-%02x\r\n", data_len, curr_frame_buffer->length, data[0], data[1], data[2], data[3], data[4], data[5]);
	{
		if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == JPEG_MODE)
		{
			if (data[0] == 0xFF && data[1] == 0xD8)
			{
				if (uvc_camera_drv[idx_uvc]->packet_err)
				{
					uvc_camera_drv[idx_uvc]->packet_err = false;
					media_debug->uvc_error++;
					uvc_camera_drv[idx_uvc]->error_packet_cnt++;
				}
				uvc_camera_drv[idx_uvc]->eof = false;
				curr_frame_buffer->length = 0;
			}
		}
		else if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == H264_MODE ||
				uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == H265_MODE)
		{
			// for H264 and H265
			if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01)
			{
				if (data[4] == 0x27) {
					//uvc_camera_drv[idx_uvc]->i_frame++;
				}
				else if (data[4] == 0x21) {
					//uvc_camera_drv[idx_uvc]->p_frame++;
				}
				uvc_camera_drv[idx_uvc]->packet_err = false;
				uvc_camera_drv[idx_uvc]->eof        = false;
				curr_frame_buffer_dual->length      = 0;
			}
		}
	}

	if (uvc_camera_drv[idx_uvc]->packet_err)
	{
		uvc_camera_drv[idx_uvc]->error_packet_cnt++;
		UVC_PER_PACKET_OUT();
		return;
	}

	// step 5: compy payload to frame_buffer
	if (data_len > 0)
	{
		os_memcpy(curr_frame_buffer_dual->frame + curr_frame_buffer_dual->length, data, data_len);
		curr_frame_buffer_dual->length += data_len;
	}

	// step 3.2: check is a new frame
	if ((bmhead_info & 0x02))   // bit1 is 1, eof bit is setted
	{
		if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == JPEG_MODE)
		{
			check_len = bk_uvc_camera_jpeg_frame_check_eof(curr_frame_buffer->frame, curr_frame_buffer->length);
			if (check_len > 0)
			{
				uvc_camera_drv[idx_uvc]->frame_flag = !(bmhead_info & 0x01);
				// maybe first packet will satify this condition
				curr_frame_buffer->length = check_len;
				uvc_camera_drv[idx_uvc]->eof = true;
			}
			else
			{
				UVC_LOGD("%s %d buffer not end with 0XFFD9\n", __func__, __LINE__);
				uvc_camera_drv[idx_uvc]->frame_eof_not_ffd9_cnt++;
			}
		}
		else if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == H264_MODE ||
				uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == H265_MODE)
		{
			uvc_camera_drv[idx_uvc]->frame_flag = !(bmhead_info & 0x01);
			// maybe first packet will satify this condition
			uvc_camera_drv[idx_uvc]->eof = true;
		}

		if (uvc_camera_drv[idx_uvc]->eof)
		{
			uvc_camera_eof_handle_dual(idx_uvc);
		}
	}

	uvc_camera_drv[idx_uvc]->ok_packet_cnt++;

	UVC_PER_PACKET_OUT();
}



static void uvc_camera_process_packet_dual_handler(camera_packet_t *camera_packet)
{
	uint8_t *data = NULL;
	uint32_t idx_uvc = camera_packet->uvc_dev_id;
	UVC_PACKET_ENTRY();
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	if (camera_packet->packet_state == CAM_STREAM_ERROR)
	{
		uvc_camera_drv[idx_uvc]->packet_err = true;
		GLOBAL_INT_RESTORE();
		UVC_LOGD("%s, %p, %d\r\n", __func__, camera_packet->data_buffer, __LINE__);
		uvc_camera_drv[idx_uvc]->total_packet_cnt += 8;
		uvc_camera_drv[idx_uvc]->error_packet_cnt += 8;
		goto out;
	}

	UVC_LOGD("%s, %p, %d\r\n", __func__, camera_packet->data_buffer, camera_packet->num_packets);

	camera_packet->packet_state = CAM_STREAM_BUSY;
	GLOBAL_INT_RESTORE();

	for (uint8_t i = 0; i < camera_packet->num_packets; i++)
	{
		uvc_camera_drv[idx_uvc]->total_packet_cnt++;
		if (camera_packet->state[i] == CAM_STREAM_READY)
		{
			data = camera_packet->data_buffer + i * camera_packet->num_byte[i];
			UVC_LOGD("%s, %p, %d, length:%d\r\n", __func__, data, i, camera_packet->actual_num_byte[i]);
			UVC_LOGD("%s, %p-%d-%d\r\n", __func__, data, camera_packet->actual_num_byte[i], camera_packet->num_byte[i]);
			uvc_camera_process_payload_dual(data, (uint32_t)camera_packet->actual_num_byte[i], idx_uvc);
		}
		else
		{
			UVC_LOGD("%s, %d\r\n", __func__, __LINE__);
			uvc_camera_drv[idx_uvc]->packet_err = true;
			uvc_camera_drv[idx_uvc]->error_packet_cnt++;
		}
	}

out:
	bk_video_camera_packet_free(camera_packet);
	UVC_PACKET_OUT();
}

static void uvc_camera_connect(void)
{
	uvc_send_msg(UVC_CONNECTED_IND, 0);
}

static void uvc_camera_disconnect(void)
{
	uvc_send_msg(UVC_DISCONNECTED_IND, 0);
}

static void uvc_camera_timer_debug(timer_id_t timer_id)
{
	uvc_send_msg(UVC_LOG_IND, 0);
}

static void uvc_camera_power_on(void)
{
#if CONFIG_USB_MAILBOX_MASTER
	bk_usb_mailbox_sw_master_init();
#endif
    // Modified by TUYA Start
#if CONFIG_TUYA_LOGIC_MODIFY
    uint8_t usb_ldo, active_level;
    tkl_vi_get_power_info(UVC_CAMERA, &usb_ldo, &active_level);
	bk_usb_power_ops(usb_ldo, 1);
#else
	bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, 1);
#endif // CONFIG_TUYA_LOGIC_MODIFY
    // Modified by TUYA End

	void *parameter = (void *)uvc_camera_connect;
	bk_usb_uvc_register_connect_callback(parameter);
	parameter = (void *)uvc_camera_disconnect;
	bk_usb_uvc_register_disconnect_callback(parameter);
	bk_usb_open(USB_HOST_MODE);
	bk_usb_device_set_using_status(1, USB_UVC_DEVICE);
}

static bk_err_t uvc_camera_get_device_info(void)
{
	int ret = BK_OK;
	uint8_t format_index = 0;
	uint8_t frame_num    = 0;
	uint8_t index        = 0;
	uint32_t n_uvc_dev   = g_n_uvc_dev;

	for (uint32_t k = 0; k < g_n_uvc_dev; k++)
	{
		uvc_device_info[k].n_uvc_dev = g_n_uvc_dev;
	}

	ret = bk_usb_uvc_get_param(&uvc_device_info[0]);
	if (ret != BK_OK)
	{
		UVC_LOGE("%s, get uvc param error\r\n", __func__);
		return ret;
	}

	for (uint32 idx = 0; idx < n_uvc_dev; idx++)
	{
		if (uvc_device_info[idx].ep_desc == NULL || uvc_device_info[idx].vendor_id == 0
			|| uvc_device_info[idx].product_id == 0)
		{
			UVC_LOGE("%s, get uvc ep_desc NULL\r\n", __func__);
			ret = BK_FAIL;
			return ret;
		}


		uvc_camera_drv[idx]->transfer_mode = ((uvc_device_info[idx].ep_desc->bmAttributes & 0x3) == USB_ENDPOINT_BULK_TRANSFER) ? true : false;
		UVC_LOGD("VID:0x%x\r\n", uvc_device_info[idx].vendor_id);
		UVC_LOGD("PID:0x%x\r\n", uvc_device_info[idx].product_id);

		format_index = uvc_device_info[idx].format_index.mjpeg_format_index;
		frame_num    = uvc_device_info[idx].all_frame.mjpeg_frame_num;

		if(format_index > 0){
		UVC_LOGD("MJPEG format_index:%d\r\n", format_index);

			if (uvc_camera_config_ptr->device.uvc_device[idx].mode == JPEG_MODE)
			{
				uvc_camera_drv[idx]->format_index = format_index;
				uvc_camera_drv[idx]->frame_num = frame_num;
			}

			for(index = 0; index < frame_num; index++)
			{
				UVC_LOGD("MJPEG width:%d heigth:%d index:%d\r\n",
							uvc_device_info[idx].all_frame.mjpeg_frame[index].width,
							uvc_device_info[idx].all_frame.mjpeg_frame[index].height,
							uvc_device_info[idx].all_frame.mjpeg_frame[index].index);

				if (uvc_device_info[idx].all_frame.mjpeg_frame[index].width == uvc_camera_config_ptr->device.uvc_device[idx].info.resolution.width
						&& uvc_device_info[idx].all_frame.mjpeg_frame[index].height == uvc_camera_config_ptr->device.uvc_device[idx].info.resolution.height)
				{
					uvc_camera_drv[idx]->frame_index = uvc_device_info[idx].all_frame.mjpeg_frame[index].index;
				}

				// iterate all support fps of current resolution
				for(int i = 0; i < uvc_device_info[idx].all_frame.mjpeg_frame[index].fps_num; i++)
				{
					UVC_LOGD("MJPEG fps:%d\r\n", uvc_device_info[idx].all_frame.mjpeg_frame[index].fps[i]);

					if (uvc_camera_drv[idx]->frame_index != UVC_PARAM_ERROR && uvc_camera_drv[idx]->frame_fps == UVC_PARAM_ERROR)
					{
						uvc_camera_drv[idx]->frame_fps = get_actual_value_of_fps(uvc_device_info[idx].all_frame.mjpeg_frame[index].fps[i], uvc_camera_config_ptr->device.uvc_device[idx].info.fps);
					}
				}

				// if not support this fps, should set default fps of uvc
				if (uvc_camera_drv[idx]->frame_index != UVC_PARAM_ERROR && uvc_camera_drv[idx]->frame_fps == UVC_PARAM_ERROR)
				{
					uvc_camera_drv[idx]->frame_fps = uvc_device_info[idx].all_frame.mjpeg_frame[index].fps[0];
				}
			}
		}

		format_index = uvc_device_info[idx].format_index.h264_format_index;
		frame_num    = uvc_device_info[idx].all_frame.h264_frame_num;
		if(format_index > 0){
			UVC_LOGD("H264 format_index:%d\r\n", format_index);

			if (uvc_camera_config_ptr->device.uvc_device[idx].mode == H264_MODE)
			{
				uvc_camera_drv[idx]->format_index = format_index;
				uvc_camera_drv[idx]->frame_num = frame_num;
			}

			for(index = 0; index < frame_num; index++)
			{
				UVC_LOGD("H264 width:%d heigth:%d index:%d\r\n",
							uvc_device_info[idx].all_frame.h264_frame[index].width,
							uvc_device_info[idx].all_frame.h264_frame[index].height,
							uvc_device_info[idx].all_frame.h264_frame[index].index);

				if (uvc_device_info[idx].all_frame.h264_frame[index].width == uvc_camera_config_ptr->device.uvc_device[idx].info.resolution.width
						&& uvc_device_info[idx].all_frame.h264_frame[index].height == uvc_camera_config_ptr->device.uvc_device[idx].info.resolution.height)
				{
					uvc_camera_drv[idx]->frame_index = uvc_device_info[idx].all_frame.h264_frame[index].index;
				}

				for(int i = 0; i < uvc_device_info[idx].all_frame.h264_frame[index].fps_num; i++)
				{
					UVC_LOGD("H264 fps:%d\r\n", uvc_device_info[idx].all_frame.h264_frame[index].fps[i]);

					if (uvc_camera_drv[idx]->frame_index != UVC_PARAM_ERROR && uvc_camera_drv[idx]->frame_fps == UVC_PARAM_ERROR)
					{
						uvc_camera_drv[idx]->frame_fps = get_actual_value_of_fps(uvc_device_info[idx].all_frame.h264_frame[index].fps[i], uvc_camera_config_ptr->device.uvc_device[idx].info.fps);
					}
				}

				// if not support this fps, should set default fps of uvc
				if (uvc_camera_drv[idx]->frame_index != UVC_PARAM_ERROR && uvc_camera_drv[idx]->frame_fps == UVC_PARAM_ERROR)
				{
					uvc_camera_drv[idx]->frame_fps = uvc_device_info[idx].all_frame.h264_frame[index].fps[0];
				}
			}
		}

		format_index = uvc_device_info[idx].format_index.h265_format_index;
		frame_num    = uvc_device_info[idx].all_frame.h265_frame_num;
		if(format_index > 0){
			UVC_LOGD("H265 format_index:%d\r\n", format_index);

			if (uvc_camera_config_ptr->device.uvc_device[idx].mode == H265_MODE)
			{
				uvc_camera_drv[idx]->format_index = format_index;
				uvc_camera_drv[idx]->frame_num = frame_num;
			}

			for(index = 0; index < frame_num; index++)
			{
				UVC_LOGD("H265 width:%d heigth:%d index:%d\r\n",
							uvc_device_info[idx].all_frame.h265_frame[index].width,
							uvc_device_info[idx].all_frame.h265_frame[index].height,
							uvc_device_info[idx].all_frame.h265_frame[index].index);

				if (uvc_device_info[idx].all_frame.h265_frame[index].width == uvc_camera_config_ptr->device.uvc_device[idx].info.resolution.width
						&& uvc_device_info[idx].all_frame.h265_frame[index].height == uvc_camera_config_ptr->device.uvc_device[idx].info.resolution.height)
				{
					uvc_camera_drv[idx]->frame_index = uvc_device_info[idx].all_frame.h265_frame[index].index;
				}

				for(int i = 0; i < uvc_device_info[idx].all_frame.h265_frame[index].fps_num; i++)
				{
					UVC_LOGD("H265 fps %d\r\n", uvc_device_info[idx].all_frame.h265_frame[index].fps[i]);

					if (uvc_camera_drv[idx]->frame_index != UVC_PARAM_ERROR && uvc_camera_drv[idx]->frame_fps == UVC_PARAM_ERROR)
					{
						uvc_camera_drv[idx]->frame_fps = get_actual_value_of_fps(uvc_device_info[idx].all_frame.h265_frame[index].fps[i], uvc_camera_config_ptr->device.uvc_device[idx].info.fps);
					}
				}

				// if not support this fps, should set default fps of uvc
				if (uvc_camera_drv[idx]->frame_index != UVC_PARAM_ERROR && uvc_camera_drv[idx]->frame_fps == UVC_PARAM_ERROR)
				{
					uvc_camera_drv[idx]->frame_fps = uvc_device_info[idx].all_frame.h265_frame[index].fps[0];
				}
			}
		}

		format_index = uvc_device_info[idx].format_index.yuv_format_index;
		frame_num    = uvc_device_info[idx].all_frame.yuv_frame_num;
		if(format_index > 0){
			UVC_LOGD("YUV format_index:%d\r\n", format_index);

			if (uvc_camera_config_ptr->device.uvc_device[idx].mode == YUV_MODE)
			{
				uvc_camera_drv[idx]->format_index = format_index;
				uvc_camera_drv[idx]->frame_num = frame_num;
			}

			for(index = 0; index < frame_num; index++)
			{
				UVC_LOGD("YUV width:%d heigth:%d index:%d\r\n",
							uvc_device_info[idx].all_frame.yuv_frame[index].width,
							uvc_device_info[idx].all_frame.yuv_frame[index].height,
							uvc_device_info[idx].all_frame.yuv_frame[index].index);

				if (uvc_device_info[idx].all_frame.yuv_frame[index].width == uvc_camera_config_ptr->device.uvc_device[idx].info.resolution.width
						&& uvc_device_info[idx].all_frame.yuv_frame[index].height == uvc_camera_config_ptr->device.uvc_device[idx].info.resolution.height)
				{
					uvc_camera_drv[idx]->frame_index = uvc_device_info[idx].all_frame.yuv_frame[index].index;
				}

				for(int i = 0; i < uvc_device_info[idx].all_frame.yuv_frame[index].fps_num; i++)
				{
					UVC_LOGD("YUV fps:%d\r\n", uvc_device_info[idx].all_frame.yuv_frame[index].fps[i]);

					if (uvc_camera_drv[idx]->frame_index != UVC_PARAM_ERROR && uvc_camera_drv[idx]->frame_fps == UVC_PARAM_ERROR)
					{
						uvc_camera_drv[idx]->frame_fps = get_actual_value_of_fps(uvc_device_info[idx].all_frame.yuv_frame[index].fps[i], uvc_camera_config_ptr->device.uvc_device[idx].info.fps);
					}
				}

				// if not support this fps, should set default fps of uvc
				if (uvc_camera_drv[idx]->frame_index != UVC_PARAM_ERROR && uvc_camera_drv[idx]->frame_fps == UVC_PARAM_ERROR)
				{
					uvc_camera_drv[idx]->frame_fps = uvc_device_info[idx].all_frame.yuv_frame[index].fps[0];
				}
			}
		}

#if 0
	for(int j = 0;j < uvc_device_info.endpoints_num; j++)
	{
		struct s_bk_usb_endpoint_descriptor *ep_desc = (struct s_bk_usb_endpoint_descriptor *)&uvc_device_info.ep_desc[j];
		UVC_LOGI("=========================================================================\r\n");
		UVC_LOGI("------------ Endpoint Descriptor -----------\r\n");
		UVC_LOGI("bLength                        : 0x%x (%d bytes)\r\n", ep_desc->bLength, ep_desc->bLength);
		UVC_LOGI("bDescriptorType                : 0x%x (Endpoint Descriptor)\r\n", ep_desc->bDescriptorType);
		if(ep_desc->bEndpointAddress & 0x80)
			UVC_LOGI("bEndpointAddress           : 0x%x (Direction=IN  EndpointID=%d)\r\n", ep_desc->bEndpointAddress, (ep_desc->bEndpointAddress & 0x0F));
		else
			UVC_LOGI("bEndpointAddress           : 0x%x (Direction=OUT  EndpointID=%d)\r\n", ep_desc->bEndpointAddress, (ep_desc->bEndpointAddress & 0x0F));
		UVC_LOGI("bmAttributes                   : 0x%x\r\n", ep_desc->bmAttributes);
		UVC_LOGI("wMaxPacketSize                 : 0x%x (%d bytes)\r\n", ep_desc->wMaxPacketSize, ep_desc->wMaxPacketSize);
		UVC_LOGI("bInterval                      : 0x%x (%d ms)\r\n", ep_desc->bInterval, ep_desc->bInterval);
	}
#endif
    }
	return ret;
}

static bk_err_t uvc_camera_set_param(void)
{
	int ret = BK_FAIL;
	uint32_t n_uvc_dev = uvc_device_info[0].n_uvc_dev;

	bk_uvc_config_t uvc_config[2] = {{0},{0}};

	for (uint32_t i = 0; i < n_uvc_dev; i++)
	{
		uvc_config[i].vendor_id    = uvc_device_info[i].vendor_id;
		uvc_config[i].product_id   = uvc_device_info[i].product_id;
		uvc_config[i].width        = uvc_camera_config_ptr->device.uvc_device[i].info.resolution.width;
		uvc_config[i].height       = uvc_camera_config_ptr->device.uvc_device[i].info.resolution.height;
		uvc_config[i].fps          = uvc_camera_drv[i]->frame_fps;
		uvc_config[i].frame_index  = uvc_camera_drv[i]->frame_index;
		uvc_config[i].format_index = uvc_camera_drv[i]->format_index;
		uvc_config[i].ep_desc      = (struct s_bk_usb_endpoint_descriptor *)&uvc_device_info[i].ep_desc[0];

		UVC_LOGD("VID:0x%x\r\n", uvc_config[i].vendor_id);
		UVC_LOGD("PID:0x%x\r\n", uvc_config[i].product_id);
		UVC_LOGD("width:%d\r\n", uvc_config[i].width);
		UVC_LOGD("height:%d\r\n", uvc_config[i].height);
		UVC_LOGD("fps:%d\r\n", uvc_config[i].fps);
		UVC_LOGD("frame_index:%d\r\n", uvc_config[i].frame_index);
		UVC_LOGD("format_index:%d\r\n", uvc_config[i].format_index);
		UVC_LOGD("ep_desc:%x\r\n", uvc_config[i].ep_desc->bmAttributes);

		if (uvc_camera_drv[i]->format_index == UVC_PARAM_ERROR || uvc_camera_drv[i]->frame_index == UVC_PARAM_ERROR
				|| uvc_camera_drv[i]->frame_fps == UVC_PARAM_ERROR)
		{
			UVC_LOGE("%s, not support this format or resolution, please check!\r\n", __func__);
			return ret;
		}
	}
	ret = bk_usb_uvc_set_param(uvc_config);//(&uvc_config);
	return ret;
}

static void uvc_camera_packet_push(camera_packet_t * camera_packet)
{
	UVC_PUSH_ENTRY();

#if 0

	static uint32_t s_index = 0;

	for (uint8_t i = 0; i < camera_packet->num_packets; i++)
	{
		if (camera_packet->state[i] == 0)
		{
			uint8_t *buffer = camera_packet->data_buffer + i * camera_packet->num_byte[i];
			uint32_t len = camera_packet->actual_num_byte[i];

			if (len > 12)
			{
				os_printf("<%d, len:%d, %02x %02x> ", s_index++, len, buffer[12], buffer[13]);
			}
			else
			{
				os_printf("<%d, len:%d, %02x %02x> ", s_index++, len, buffer[0], buffer[1]);
			}
		}
	}

	os_printf("\n");
#endif

	uvc_send_msg(UVC_PACKET_DEAL_IND, (uint32_t)camera_packet);
	UVC_PUSH_OUT();
}

static void uvc_camera_packet_dual_push(camera_packet_t * camera_packet)
{
	UVC_PUSH_ENTRY();
	//uint32_t idx_uvc = camera_packet->uvc_dev_id;

	uvc_send_msg(UVC_PACKET_DUAL_DEAL_IND, (uint32_t)camera_packet);
	UVC_PUSH_OUT();
}

static camera_packet_t *uvc_camera_packet_malloc(void)
{
	camera_packet_t *packet = NULL;

	UVC_MALLOC_ENTRY();

	packet = bk_video_camera_packet_malloc();

	UVC_MALLOC_OUT();

	if (packet == NULL)
	{
		UVC_LOGW("%s, %d\n", __func__, __LINE__);
	}
	return packet;
}

static camera_packet_t *uvc_camera_packet_dual_malloc(void)
{
	camera_packet_t *packet = NULL;

	UVC_MALLOC_ENTRY();

	packet = bk_video_camera_packet_dual_malloc();

	UVC_MALLOC_OUT();

	if (packet == NULL)
	{
		UVC_LOGW("%s, %d\n", __func__, __LINE__);
	}
	return packet;
}

const camera_packet_control_t packet_ops =
{
	.init   = NULL,
	.deinit = NULL,
	.malloc = uvc_camera_packet_malloc,
	.push   = uvc_camera_packet_push,
	.pop    = bk_video_camera_packet_pop,
	.free   = bk_video_camera_packet_free,
	.d_malloc = uvc_camera_packet_dual_malloc,
	.d_push   = uvc_camera_packet_dual_push,
	.d_pop    = bk_video_camera_packet_dual_pop,
	.d_free   = bk_video_camera_packet_dual_free,
};


static void uvc_camera_connect_handler(void)
{
	int ret = BK_OK;
	uint32_t n_uvc_dev = 0; __maybe_unused_var(n_uvc_dev);
	uint32_t idx = 0; __maybe_unused_var(idx);

	//  uint8_t i = 0, temp = 0;
	//  uint16_t calibrate_fps = 0;
	if (uvc_camera_drv[0]->uvc_state == UVC_CONNECTED)
	{
		UVC_LOGE("%s, uvc have been connected!\r\n", __func__);
		return;
	}

	for (idx = 0; idx < g_n_uvc_dev; idx++)
	{
		uvc_camera_drv[idx]->uvc_state              = UVC_CONNECTED;
		uvc_camera_drv[idx]->ok_packet_cnt          = 0;
		uvc_camera_drv[idx]->error_packet_cnt       = 0;
		uvc_camera_drv[idx]->emp_packet_cnt         = 0;
		uvc_camera_drv[idx]->frame_eof_not_ffd9_cnt = 0;
		uvc_camera_drv[idx]->frame_end_not_ffd9_cnt = 0;
	}
	// step 1: get uvc descrption
	ret = uvc_camera_get_device_info();
	if (ret != BK_OK)
	{
		UVC_LOGE("%s, get param error:%d\r\n", __func__, ret);
		for (idx = 0; idx < g_n_uvc_dev; idx++) {
			uvc_camera_drv[idx]->uvc_state = UVC_DISCONNECT_ABNORMAL;
		}
		return;
	}

	// step 2: register camera_packet callback func
	bk_usb_uvc_register_transfer_buffer_ops((void *)&packet_ops);

	// step 3: init camera_packet_list
	for (idx = 0; idx < g_n_uvc_dev; idx++)
	{
		uint32_t maxpacketsize = 1024;//uvc_device_info[idx].ep_desc->wMaxPacketSize > 1024? 1024:uvc_device_info[idx].ep_desc->wMaxPacketSize;
#ifdef CONFIG_PSRAM_AS_SYS_MEMORY
		ret = bk_video_camera_packet_list_init(CAMERA_MEM_IN_PSRAM, maxpacketsize, 8, CONFIG_UVC_MAX_PACKET_CNT, idx);//uvc_device_info[0].ep_desc->wMaxPacketSize & 0x400, 8, 16);
#else
		ret = bk_video_camera_packet_list_init(CAMERA_MEM_IN_SRAM, maxpacketsize, 8, 4, idx);//uvc_device_info[0].ep_desc->wMaxPacketSize & 0x400, 8, 4);
#endif
		if (ret != BK_OK)
		{
			uvc_send_msg(UVC_EXIT_IND, 0);
			return;
		}
	}

	// step 4: set uvc params
	ret = uvc_camera_set_param();
	if (ret != BK_OK)
	{
		UVC_LOGE("%s, set param error:%d\r\n", __func__, ret);
		if (!device_info_cb)
			return;
	}

	// step 5: start video_stream
	if (!device_info_cb)
	{
		ret = bk_uvc_start();
		if (ret != BK_OK)
		{
			UVC_LOGE("%s, start error:%d\r\n", __func__, ret);
			return;
		}

		ret = bk_uvc_dual_start();
		if (ret != BK_OK)
		{
			UVC_LOGE("%s, start error:%d\r\n", __func__, ret);
			return;
		}
	}
	else
	{
		device_info_cb(&uvc_device_info[0], UVC_CONNECTED);
	}
	if (uvc_camera_config_ptr->uvc_connect_state_change_cb)
	{
		uvc_camera_config_ptr->uvc_connect_state_change_cb(UVC_CONNECTED);
	}
}

static void uvc_camera_disconnect_handler(void)
{
	UVC_LOGI("%s\r\n", __func__);
	for (uint32_t idx_uvc = 0; idx_uvc < g_n_uvc_dev; idx_uvc++)
	{
		uvc_camera_drv[idx_uvc]->uvc_state = UVC_DISCONNECT_ABNORMAL;
	}

	if (device_info_cb)
	{
		device_info_cb(NULL, UVC_DISCONNECT_ABNORMAL);
	}
	if (uvc_camera_config_ptr->uvc_connect_state_change_cb)
	{
		uvc_camera_config_ptr->uvc_connect_state_change_cb(UVC_DISCONNECT_ABNORMAL);
	}
}

static void uvc_camera_debug_log_handler(uint32_t idx_uvc)
{
	UVC_LOGI("seq:%d, total:%d, ok:%d, error:%d, emp:%d, eof_not_ffd9:%d, end_not_ffd9:%d\n", uvc_frame_id[idx_uvc],
		uvc_camera_drv[idx_uvc]->total_packet_cnt, uvc_camera_drv[idx_uvc]->ok_packet_cnt,
		uvc_camera_drv[idx_uvc]->error_packet_cnt, uvc_camera_drv[idx_uvc]->emp_packet_cnt,
		uvc_camera_drv[idx_uvc]->frame_eof_not_ffd9_cnt, uvc_camera_drv[idx_uvc]->frame_end_not_ffd9_cnt);
}

static void uvc_camera_deinit(void)
{
	uint32_t idx_uvc = 0;

	// step 1: free dma
	for (idx_uvc = 0; idx_uvc < g_n_uvc_dev; idx_uvc++)
	{
		UVC_LOGI("%s, total:%d, ok:%d, error:%d, empty:%d\n", __func__,
			uvc_camera_drv[idx_uvc]->total_packet_cnt, uvc_camera_drv[idx_uvc]->ok_packet_cnt,
			uvc_camera_drv[idx_uvc]->error_packet_cnt, uvc_camera_drv[idx_uvc]->emp_packet_cnt);

		if (uvc_camera_drv[idx_uvc])
		{
			os_free(uvc_camera_drv[idx_uvc]);
			uvc_camera_drv[idx_uvc] = NULL;
		}
	}

	if (uvc_class_task_sem)
	{
		rtos_deinit_semaphore(&uvc_class_task_sem);
		uvc_class_task_sem = NULL;
	}

	// step 2: free frame_buffer and deinit
	bk_video_camera_packet_list_free();
	bk_video_camera_packet_list_dual_free();
	bk_video_camera_packet_list_deinit();

	if (curr_frame_buffer)
	{
		uvc_camera_config_ptr->jpeg_cb.free(curr_frame_buffer);
		curr_frame_buffer = NULL;
		UVC_LOGI("%s, curr_frame_buffer = 0x%x\n", __func__, curr_frame_buffer);
	}

	if (curr_frame_buffer_dual)
	{
		uvc_camera_config_ptr->h264_cb.free(curr_frame_buffer_dual);
		curr_frame_buffer_dual = NULL;
		UVC_LOGI("%s, curr_frame_buffer_dual = 0x%x\n", __func__, curr_frame_buffer_dual);
	}

	for (idx_uvc = 0; idx_uvc < g_n_uvc_dev; idx_uvc++)
	{
		if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == JPEG_MODE) {
			uvc_camera_config_ptr->jpeg_cb.clear(FB_INDEX_JPEG);
		}
		else if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == H264_MODE) {
			uvc_camera_config_ptr->h264_cb.clear(FB_INDEX_H264);
		}
		else if (uvc_camera_config_ptr->device.uvc_device[idx_uvc].mode == H265_MODE) {
			uvc_camera_config_ptr->h264_cb.clear(FB_INDEX_H265);
		}
	}
	g_n_uvc_dev = 0;

	//  step 3: free config
	uvc_camera_config_ptr = NULL;
	device_info_cb        = NULL;

	// uvc_frame_id = 0;
	// uvc_h264_frame_id = 0;
	os_memset(uvc_frame_id, 0x00, sizeof(uvc_frame_id)*sizeof(uint32_t));

    // Modified by TUYA Start
#if CONFIG_TUYA_LOGIC_MODIFY
    uint8_t usb_ldo, active_level;
    tkl_vi_get_power_info(UVC_CAMERA, &usb_ldo, &active_level);
	bk_usb_power_ops(usb_ldo, 0);
#else
	bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, 0);
#endif // CONFIG_TUYA_LOGIC_MODIFY
    // Modified by TUYA End

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_USB_1, PM_CPU_FRQ_DEFAULT);
}

static void uvc_camera_main(beken_thread_arg_t data)
{
	int ret = BK_OK;
	uint32_t idx = 0; __maybe_unused_var(idx);
	rtos_init_timer(&uvc_debug_timer, 6 * 1000, (timer_handler_t)uvc_camera_timer_debug, NULL);
	rtos_start_timer(&uvc_debug_timer);

	rtos_set_semaphore(&uvc_class_task_sem);

	uvc_camera_power_on();

	while (1)
	{
		uvc_msg_t msg;
		ret = rtos_pop_from_queue(&uvc_msg_queue, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret)
		{
			switch (msg.type)
			{
				case UVC_CONNECTED_IND:
					uvc_camera_connect_handler();
					break;

				case UVC_DISCONNECTED_IND:
					uvc_camera_disconnect_handler();
					break;

				case UVC_PACKET_DEAL_IND:
					uvc_camera_process_packet_handler((camera_packet_t *)(msg.data));
					break;
				case UVC_PACKET_DUAL_DEAL_IND:
					uvc_camera_process_packet_dual_handler((camera_packet_t *)(msg.data));

					break;
				case UVC_LOG_IND:
					uvc_camera_debug_log_handler(0);
					break;

				case UVC_EXIT_IND:
					goto exit;

				default:
					break;
			}
		}
	}

exit:

	rtos_stop_timer(&uvc_debug_timer);
	rtos_deinit_timer(&uvc_debug_timer);

	// step 1: stop uvc stream and close uvc
	bk_uvc_stop();

	bk_usb_device_set_using_status(0, USB_UVC_DEVICE);

	// unregister connect & disconnect
	bk_usb_uvc_register_disconnect_callback(NULL);
	bk_usb_uvc_register_connect_callback(NULL);

	for (idx = 0; idx < g_n_uvc_dev; idx++) {
		uvc_camera_drv[idx]->uvc_state = UVC_DISCONNECT_NORMAL;
	}
	ret = bk_usb_close();

	// step 5: free que and delete task
	rtos_deinit_queue(&uvc_msg_queue);
	uvc_msg_queue = NULL;

	uvc_class_task = NULL;
	rtos_set_semaphore(&uvc_class_task_sem);
	rtos_delete_thread(NULL);
}

bool check_uvc_status(void)
{
	if (uvc_camera_drv[0] == NULL)
	{
		return false;
	}
	return (uvc_camera_drv[0]->uvc_state == UVC_CONNECTED) ? 1 : 0;
}

int bk_uvc_camera_jpeg_frame_check_eof(uint8_t *data, uint32_t length)
{
	int ret = BK_FAIL;

	uint32_t max_length = 50;

#if (CONFIG_JPEG_FRAME_CHECK_LENGTH)
	max_length = CONFIG_JPEG_FRAME_CHECK_LENGTH;
#endif

	if (length <= max_length)
	{
		return ret;
	}

	for (uint32_t i = length - 1; i > 1 && max_length > 0; i++)
	{
		if (data[i] == 0xD9 && data[i - 1] == 0xFF)
		{
			ret = i + 1;
			break;
		}

		max_length--;
	}

	return ret;
}

bk_err_t bk_uvc_camera_driver_init(uvc_camera_config_t *config)
{
	int ret = kNoErr;

	if (uvc_class_task != NULL || uvc_msg_queue != NULL)
	{
		UVC_LOGE("uvc_calss_task already init\r\n");
		return ret;
	}
	UVC_DIAG_DEBUG_INIT();

	uvc_camera_config_ptr = config;
	uint32_t idx = 0;
	uint32_t num_uvc_dev = config->device.num_uvc_dev;

	BK_ASSERT(config->jpeg_cb.malloc != NULL);
	BK_ASSERT(config->jpeg_cb.push != NULL);

	if (num_uvc_dev > 1)
	{
		BK_ASSERT(config->h264_cb.malloc != NULL);
		BK_ASSERT(config->h264_cb.push != NULL);
	}
	g_n_uvc_dev = num_uvc_dev;

	for (idx = 0; idx < num_uvc_dev; idx++)
	{
		if (uvc_camera_drv[idx] == NULL)
		{
			uvc_camera_drv[idx] = (uvc_driver_t *)os_malloc(sizeof(uvc_driver_t));

			if (uvc_camera_drv[idx] == NULL)
			{
				UVC_LOGE("uvc_camera_drv[idx] malloc failed\n");
				ret = BK_UVC_NO_MEMORY;
				return ret;
			}

			os_memset(uvc_camera_drv[idx], 0, sizeof(uvc_driver_t));

			uvc_camera_drv[idx]->format_index = UVC_PARAM_ERROR;
			uvc_camera_drv[idx]->frame_index  = UVC_PARAM_ERROR;
			uvc_camera_drv[idx]->frame_fps    = UVC_PARAM_ERROR;
		}

		// step 2: malloc frame_buffer

		if (uvc_camera_config_ptr->device.uvc_device[idx].mode == JPEG_MODE)
		{
			UVC_LOGD("mode : JPEG_MODE\n");
			uvc_camera_config_ptr->jpeg_cb.init(FB_INDEX_JPEG);
			curr_frame_buffer = uvc_camera_config_ptr->jpeg_cb.malloc(FB_INDEX_JPEG, CONFIG_JPEG_FRAME_SIZE);
			if (curr_frame_buffer == NULL)
			{
				UVC_LOGE("malloc frame fail \r\n");
				ret = BK_UVC_NO_MEMORY;
				goto error;
			}

			curr_frame_buffer->type   = uvc_camera_config_ptr->device.type;
			curr_frame_buffer->width  = uvc_camera_config_ptr->device.uvc_device[idx].info.resolution.width;
			curr_frame_buffer->height = uvc_camera_config_ptr->device.uvc_device[idx].info.resolution.height;
			curr_frame_buffer->fmt    = uvc_camera_config_ptr->device.uvc_device[idx].fmt;
		}
		else if (uvc_camera_config_ptr->device.uvc_device[idx].mode == H264_MODE)
		{
			UVC_LOGD("mode : H264_MODE\n");
			uvc_camera_config_ptr->h264_cb.init(FB_INDEX_H264);
			curr_frame_buffer_dual = uvc_camera_config_ptr->h264_cb.malloc(FB_INDEX_H264, CONFIG_H264_FRAME_SIZE);
			if (curr_frame_buffer_dual == NULL)
			{
				UVC_LOGE("malloc frame fail \r\n");
				ret = BK_UVC_NO_MEMORY;
				goto error;
			}

			curr_frame_buffer_dual->type   = uvc_camera_config_ptr->device.type;
			curr_frame_buffer_dual->width  = uvc_camera_config_ptr->device.uvc_device[idx].info.resolution.width;
			curr_frame_buffer_dual->height = uvc_camera_config_ptr->device.uvc_device[idx].info.resolution.height;
			curr_frame_buffer_dual->fmt    = uvc_camera_config_ptr->device.uvc_device[idx].fmt;
		}
		else if (uvc_camera_config_ptr->device.uvc_device[idx].mode == H265_MODE)
		{
			UVC_LOGD("mode : H265_MODE\n");
			uvc_camera_config_ptr->h264_cb.init(FB_INDEX_H265);
			curr_frame_buffer_dual = uvc_camera_config_ptr->h264_cb.malloc(FB_INDEX_H265, CONFIG_H264_FRAME_SIZE);
			if (curr_frame_buffer_dual == NULL)
			{
				UVC_LOGE("malloc frame fail \r\n");
				ret = BK_UVC_NO_MEMORY;
				goto error;
			}

			curr_frame_buffer_dual->type   = uvc_camera_config_ptr->device.type;
			curr_frame_buffer_dual->width  = uvc_camera_config_ptr->device.uvc_device[idx].info.resolution.width;
			curr_frame_buffer_dual->height = uvc_camera_config_ptr->device.uvc_device[idx].info.resolution.height;
			curr_frame_buffer_dual->fmt    = uvc_camera_config_ptr->device.uvc_device[idx].fmt;
		}
	}

	ret = rtos_init_semaphore(&uvc_class_task_sem, 1);
	if (ret != kNoErr)
	{
		goto error;
	}

	// step 4: init uvc_task
	ret = rtos_init_queue(&uvc_msg_queue, "uvc_class_queue", sizeof(uvc_msg_t), 20);
	if (ret != kNoErr)
	{
		UVC_LOGE("init uvc_class_queue failed\r\n");
		goto error;
	}

	ret = rtos_create_thread(&uvc_class_task,
							4,
							"uvc_class_task",
							(beken_thread_function_t)uvc_camera_main,
							1024 * 2,
							NULL);

	if (ret == kNoErr)
	{
		rtos_get_semaphore(&uvc_class_task_sem, BEKEN_NEVER_TIMEOUT);
		bk_pm_module_vote_cpu_freq(PM_DEV_ID_USB_1, PM_CPU_FRQ_480M);
		return ret;
	}

	media_debug->isr_jpeg    = 0;
	media_debug->isr_h264    = 0;
	media_debug->uvc_error   = 0;
	media_debug->jpeg_length = 0;
	media_debug->h264_length = 0;
	media_debug->jpeg_kbps   = 0;
	media_debug->h264_kbps   = 0;

error:
	UVC_LOGE("%s failed\r\n", __func__);

	for (idx = 0; idx < num_uvc_dev; idx++)
	{
		if (uvc_camera_drv[idx])
		{
		os_free(uvc_camera_drv[idx]);
		uvc_camera_drv[idx] = NULL;
		}
	}

	if (uvc_class_task_sem)
	{
		rtos_deinit_semaphore(&uvc_class_task_sem);
		uvc_class_task_sem = NULL;
	}

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_USB_1, PM_CPU_FRQ_DEFAULT);

	uvc_camera_config_ptr = NULL;
	device_info_cb = NULL;

	if (uvc_msg_queue)
	{
		rtos_deinit_queue(&uvc_msg_queue);
		uvc_msg_queue = NULL;
	}

	return ret;
}


bk_err_t bk_uvc_camera_driver_deinit(void)
{
	if (uvc_msg_queue)
	{
		uvc_send_msg(UVC_EXIT_IND, 0);

		rtos_get_semaphore(&uvc_class_task_sem, BEKEN_NEVER_TIMEOUT);

		uvc_camera_deinit();
	}

	UVC_LOGI("%s, complete\r\n", __func__);

	return BK_OK;
}

media_camera_device_t *bk_uvc_camera_get_device(void)
{
	if (uvc_camera_config_ptr == NULL)
	{
		return NULL;
	}

	return &uvc_camera_config_ptr->device;
}

bk_err_t bk_uvc_camera_get_config(uvc_camera_device_t *param, uint16_t count)
{
	if (param == NULL)
	{
		return BK_UVC_NO_MEMORY;
	}

	if (uvc_camera_drv[0] == NULL)
	{
		return BK_UVC_NOT_PERMIT;
	}

//  bk_uvc_get_resolution_framerate((void *)param, count);

	return BK_OK;
}

bk_err_t bk_uvc_camera_set_config(bk_uvc_config_t *config)
{
	if (uvc_camera_drv[0]->uvc_state != UVC_CONNECTED)
	{
		UVC_LOGE("%s, uvc not connect success\r\n", __func__);
		return BK_FAIL;
	}

	if (config->product_id == 0 || config->product_id == 0)
	{
		config->product_id = uvc_device_info[0].product_id;
		config->vendor_id  = uvc_device_info[0].vendor_id;
	}

	UVC_LOGI("%s VID:0x%x\r\n",__func__, config->vendor_id);
	UVC_LOGI("%s PID:0x%x\r\n",__func__, config->product_id);
	UVC_LOGI("%s width:%d\r\n",__func__, config->width);
	UVC_LOGI("%s height:%d\r\n",__func__, config->height);
	UVC_LOGI("%s fps:%d\r\n",__func__, config->fps);
	UVC_LOGI("%s frame_index:%d\r\n",__func__, config->frame_index);
	UVC_LOGI("%s format_index:%d\r\n",__func__, config->format_index);

	config->ep_desc = (struct s_bk_usb_endpoint_descriptor *)&uvc_device_info[0].ep_desc[0];

	if (bk_usb_uvc_set_param(config) != BK_OK)
	{
		UVC_LOGE("%s, param error\r\n", __func__);
		return BK_FAIL;
	}

	uvc_camera_config_ptr->device.uvc_device[0].info.resolution.width = config->width;
	uvc_camera_config_ptr->device.uvc_device[0].info.resolution.height = config->height;

	if (bk_uvc_start() != BK_OK)
	{
		UVC_LOGE("%s, start error\r\n", __func__);
		return BK_FAIL;
	}

	return BK_OK;
}

bk_err_t bk_uvc_camera_register_info_callback(uvc_device_info_t cb)
{
	device_info_cb = cb;
	return BK_OK;
}

