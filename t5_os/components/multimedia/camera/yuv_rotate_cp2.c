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
#include <modules/image_scale.h>

#include "media_mailbox_list_util.h"
#include "media_evt.h"
#include "frame_buffer.h"

#define TAG "rot_cp2"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define YUV_ROTATE_IN_SRAM_ENABLE (1)
#define MAX_BLOCK_WIDTH     (80)
#define MAX_BLOCK_HEIGHT    (40)
#define DVP_YUV_ROTATE_BLOCK_SIZE (MAX_BLOCK_WIDTH * MAX_BLOCK_HEIGHT * 2)
extern uint8_t *media_dtcm_share_buff;

typedef struct
{
	uint8_t *rx_buf;
	uint8_t *tx_buf;
} rot_buf_t;

static rot_buf_t *s_rot_buf = NULL;

void memcpy_word(uint32_t *dst, uint32_t *src, uint32_t size)
{
	uint32_t i = 0;

	for (i = 0; i < size; i++)
	{
		dst[i] = src[i];
	}
}

bk_err_t yuv_frame_rotate_handle(uint32_t param)
{
	int ret = BK_OK;
	media_mailbox_msg_t *msg = (media_mailbox_msg_t *)param;
	rotate_nofity_t *rot_notify = (rotate_nofity_t *)msg->param;

	frame_buffer_t *src_yuv = rot_notify->src_yuv;
	frame_buffer_t *rotate_yuv = rot_notify->dst_yuv;

	// start rotate
	int (*func)(unsigned char *vuyy, unsigned char *rotatedVuyy, int width, int height);

	func = NULL;
	switch (rot_notify->rot_angle)
	{
		case ROTATE_90:
			func = yuyv_rotate_degree90_to_yuyv;
			rotate_yuv->width = src_yuv->height;
			rotate_yuv->height = src_yuv->width;
			break;

		case ROTATE_270:
			func = yuyv_rotate_degree270_to_yuyv;
			rotate_yuv->width = src_yuv->height;
			rotate_yuv->height = src_yuv->width;
			break;

		case ROTATE_180:
			func = yuyv_rotate_degree180_to_yuyv;
			rotate_yuv->width = src_yuv->width;
			rotate_yuv->height = src_yuv->height;
			break;

		default:
			ret = BK_FAIL;
			break;
	}

#if YUV_ROTATE_IN_SRAM_ENABLE

		if (media_dtcm_share_buff == NULL)
		{
			if (s_rot_buf == NULL)
			{
				s_rot_buf = (rot_buf_t *)os_malloc(DVP_YUV_ROTATE_BLOCK_SIZE * 2 + sizeof(rot_buf_t));
				if (s_rot_buf == NULL)
				{
					ret = BK_ERR_NO_MEM;
					goto out;
				}

				s_rot_buf->rx_buf = (uint8_t *)s_rot_buf + sizeof(rot_buf_t);
				s_rot_buf->tx_buf = s_rot_buf->rx_buf + DVP_YUV_ROTATE_BLOCK_SIZE;
			}
		}
		else
		{
			if (s_rot_buf == NULL)
			{
				s_rot_buf = (rot_buf_t *)os_malloc(sizeof(rot_buf_t));
				if (s_rot_buf == NULL)
				{
					ret = BK_ERR_NO_MEM;
					goto out;
				}
			}
			s_rot_buf->rx_buf = media_dtcm_share_buff;
			s_rot_buf->tx_buf = s_rot_buf->rx_buf + 5 * 1024;
		}

		int i = 0, j = 0, k = 0;
		int src_width = 864, src_height = 480;
		int block_width = 54, block_height = 40;
		uint8_t *rx_block = s_rot_buf->rx_buf;
		uint8_t *tx_block = s_rot_buf->tx_buf;
		register uint8_t *cp_ptr = NULL;
		uint8_t *src_frame_temp = src_yuv->frame;
		uint8_t *dst_frame_temp = rotate_yuv->frame;
		for (j = 0; j < (src_height / block_height); j++)
		{

			for (i = 0; i < (src_width / block_width); i++)
			{
				for (k = 0; k < block_height; k++)
				{
					cp_ptr = src_frame_temp + i * block_width * 2 + j * block_height * src_width * 2 + k * src_width * 2;
					memcpy_word((uint32_t *)(rx_block + block_width * 2 * k), (uint32_t *)cp_ptr, block_width * 2 / 4);
				}

				func(rx_block, tx_block, block_width, block_height);

				for (k = 0; k < block_width; k++)
				{
					if (rot_notify->rot_angle == ROTATE_90)
					{
						cp_ptr = dst_frame_temp + (src_height / block_height - j - 1) * block_height * 2 + (i) * block_width * src_height * 2 + k * src_height * 2;
						memcpy_word((uint32_t *)cp_ptr, (uint32_t *)(tx_block + block_height * 2 * k), block_height * 2 / 4);
					}
					else //270
					{
						cp_ptr = dst_frame_temp + (src_width / block_width - 1 - i) * block_width * src_height * 2 + block_height * j * 2 + k * src_height * 2;
						memcpy_word((uint32_t *)cp_ptr, (uint32_t *)(tx_block + block_height * 2 * k), block_height * 2 / 4);
					}
				}
			}
		}

#else
		func(src_yuv->frame, rotate_yuv->frame, src_yuv->width, src_yuv->height);
#endif

out:
	msg->result = ret;
	ret = msg_send_notify_to_media_minor_mailbox(msg, MAJOR_MODULE);

	if (ret != BK_OK)
	{
		LOGE("%s, %d\n", __func__, __LINE__);
	}

	return ret;
}
