// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>
#include <components/log.h>
#include <common/bk_include.h>
#include <driver/int.h>

#include "bk_list.h"
#include "media_core.h"
#include "media_evt.h"
#include <driver/mailbox_channel.h>
#include "media_mailbox_list_util.h"

#include <driver/jpeg_dec.h>
#include <modules/jpeg_decode_sw.h>

#include "yuv_encode.h"

#include <modules/pm.h>

#if (CONFIG_CACHE_ENABLE)
#include "cache.h"
#endif

#define TAG "media_minor_mailbox"

//#define CONFIG_MINOR

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static LIST_HEADER_T media_minor_mailbox_msg_queue_req;
static LIST_HEADER_T media_minor_mailbox_msg_queue_rsp;

static beken_queue_t media_minor_mailbox_msg_queue;

static beken_thread_t media_minor_mailbox_th_hd = NULL;
static beken_semaphore_t media_minor_mailbox_sem = NULL;
static beken_semaphore_t media_minor_mailbox_rsp_sem = NULL;
static beken_semaphore_t media_minor_mailbox_init_sem = NULL;

static uint8_t media_minor_mailbox_inited = 0;

static bk_err_t media_send_msg_to_queue(media_mailbox_msg_t *param, uint8_t ori);

static bk_err_t msg_send_to_media_minor_mailbox_list(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_OK;
	ret = media_mailbox_list_push(msg, &media_minor_mailbox_msg_queue_req);

	if (ret == BK_OK)
	{
		rtos_set_semaphore(&media_minor_mailbox_sem);
	}
	return ret;
}

bk_err_t msg_send_req_to_media_minor_mailbox_sync(uint32_t event, uint32_t dest, uint32_t in_param, uint32_t *out_param)
{
	bk_err_t ret = BK_OK;
	media_mailbox_msg_t msg = {0};

	msg.count = 0;
	if (media_minor_mailbox_inited == 0)
	{
		return BK_ERR_NOT_INIT;
	}

	ret = rtos_init_semaphore_ex(&msg.sem, 1, 0);

	if (ret != BK_OK)
	{
		LOGE("%s init semaphore failed\n", __func__);
		goto out;
	}
	msg.event = event;
	msg.param = in_param;
	msg.src = MINOR_MODULE;
	msg.dest = dest;
	msg.type = MAILBOX_MSG_TYPE_REQ;

	ret = msg_send_to_media_minor_mailbox_list(&msg);
	if (ret != BK_OK)
	{
		LOGE("%s add to list fail\n", __func__);
		goto out;
	}

	ret = rtos_get_semaphore(&msg.sem, BEKEN_WAIT_FOREVER);

	if (ret != BK_OK)
	{
		LOGE("%s wait semaphore failed\n", __func__);
		goto out;
	}

	ret = msg.result;
	if (ret != BK_OK)
	{
		LOGE("%s failed 0x%x\n", __func__, ret);
		goto out;
	}

	if (out_param != NULL)
	{
		*out_param = msg.param;
	}
out:
	if(msg.sem)
	{
		rtos_deinit_semaphore(&msg.sem);
		msg.sem = NULL;
	}

	return ret;
}

bk_err_t msg_send_rsp_to_media_minor_mailbox(media_mailbox_msg_t *msg, uint32_t result, uint32_t dest)
{
	bk_err_t ret = BK_OK;
	msg->src = MINOR_MODULE;
	msg->dest = dest;
	msg->type = MAILBOX_MSG_TYPE_RSP;
	msg->result = result;
	msg->count = 0;

	if (media_minor_mailbox_inited == 0)
	{
		return BK_ERR_NOT_INIT;

}
	ret = msg_send_to_media_minor_mailbox_list(msg);
	if (ret != kNoErr)
	{
		LOGE("%s add to list fail\n", __func__);
	}
	return ret;
}

bk_err_t msg_send_notify_to_media_minor_mailbox(media_mailbox_msg_t *msg, uint32_t dest)
{
	bk_err_t ret = BK_OK;
	msg->src = MINOR_MODULE;
	msg->dest = dest;
	msg->type = MAILBOX_MSG_TYPE_NOTIFY;
	msg->count = 0;

	if (media_minor_mailbox_inited == 0)
	{
		return BK_ERR_NOT_INIT;
	}

	ret = msg_send_to_media_minor_mailbox_list(msg);
	if (ret != kNoErr)
	{
		LOGE("%s add to list fail\n", __func__);
	}
	return ret;
}

static bk_err_t msg_send_back_to_media_minor_mailbox(media_mailbox_msg_t *msg, uint32_t result)
{
	bk_err_t ret = BK_OK;
	msg->src = MINOR_MODULE;
	msg->dest = MINOR_MODULE;
	msg->type = MAILBOX_MSG_TYPE_RSP;
	msg->result = result;
	msg->count = 0;

	if (media_minor_mailbox_inited == 0)
	{
		return BK_ERR_NOT_INIT;
	}

	ret = msg_send_to_media_minor_mailbox_list(msg);
	if (ret != kNoErr)
	{
		LOGE("%s add to list fail\n", __func__);
	}
	return ret;
}

static bk_err_t media_minor_mailbox_send_msg_to_media_major_mailbox(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_OK;
	mb_chnl_cmd_t mb_cmd;
	bk_err_t send_ack_flag = false;

	msg->ack_flag = MAILBOX_MESSAGE_SEND;
	if (msg->type == MAILBOX_MSG_TYPE_REQ || msg->type == MAILBOX_MSG_TYPE_NOTIFY)
	{
		send_ack_flag = true;
	}

	mb_cmd.hdr.cmd = 1;
	mb_cmd.param1 = msg->event;
	mb_cmd.param2 = (uint32_t)msg;
	mb_cmd.param3 = (uint32_t)msg->sem;
	ret = mb_chnl_write(CP1_MB_CHNL_MEDIA, &mb_cmd);

	if (ret != BK_OK)
	{
		msg_send_back_to_media_minor_mailbox(msg, BK_FAIL);
		LOGE("%s %d FAILED\n", __func__, __LINE__);
	}
	else
	{
		ret = rtos_get_semaphore(&media_minor_mailbox_rsp_sem, 2000);
		if(ret != BK_OK)
		{
			LOGE("%s %d rtos_get_semaphore error type=%x event=%x\n", __func__, __LINE__, msg->type, msg->event);
			if (msg->type == MAILBOX_MSG_TYPE_REQ)
			{
				msg->count++;
				msg_send_to_media_minor_mailbox_list(msg);
			}
		}
		else if (send_ack_flag && msg->ack_flag != MAILBOX_MESSAGE_ACK)
		{
			if (msg->type == MAILBOX_MSG_TYPE_REQ)
			{
				msg->count++;
				msg_send_to_media_minor_mailbox_list(msg);
				LOGI("%s %d send request FAILED %x\n", __func__, __LINE__, msg->event);
			}
			else if (msg->type == MAILBOX_MSG_TYPE_NOTIFY)
			{
				LOGI("%s %d send notify FAILED %x\n", __func__, __LINE__, msg->event);
			}
		}
		else if (send_ack_flag && msg->ack_flag == MAILBOX_MESSAGE_ACK)
		{
			if (msg->type == MAILBOX_MSG_TYPE_REQ)
			{
				media_send_msg_to_queue(msg, 1);
			}
		}
	}
	return ret;
}

static bk_err_t media_send_msg_to_queue(media_mailbox_msg_t *param, uint8_t ori)
{
	bk_err_t ret = BK_OK;
	media_msg_t msg;
	msg.event = ori;
	msg.param = (uint32_t)param;

	if (media_minor_mailbox_msg_queue)
	{
		ret = rtos_push_to_queue(&media_minor_mailbox_msg_queue, &msg, BEKEN_NO_WAIT);

		if (BK_OK != ret)
		{
			LOGE("%s failed\n", __func__);
			return BK_FAIL;
		}

		return ret;
	}
	return ret;
}

static void media_minor_mailbox_mailbox_rx_isr(void *param, mb_chnl_cmd_t *cmd_buf)
{
	media_mailbox_msg_t *msg;

#if (CONFIG_CACHE_ENABLE)
	flush_all_dcache();
#endif

	msg = (media_mailbox_msg_t *)cmd_buf->param2;
	if (msg)
	{
		if (msg->ack_flag == MAILBOX_MESSAGE_SEND)
		{
			msg->ack_flag = MAILBOX_MESSAGE_ACK;
		}
		else
		{
			LOGE("%s %d ack flag error %d Event:%x\n", __func__, __LINE__, msg->ack_flag, msg->event);
			return;
		}
		media_send_msg_to_queue(msg, 0);
	}
	else
	{
		LOGE("%s %d msg is NULL\n", __func__, __LINE__);
	}

	rtos_set_semaphore(&media_minor_mailbox_sem);
}


static void media_minor_mailbox_mailbox_tx_isr(void *param)
{
}

static void media_minor_mailbox_mailbox_tx_cmpl_isr(void *param, mb_chnl_ack_t *ack_buf)
{
#if (CONFIG_CACHE_ENABLE)
    flush_all_dcache();
#endif

	if (media_minor_mailbox_rsp_sem)
	{
		rtos_set_semaphore(&media_minor_mailbox_rsp_sem);
	}
}

void media_minor_mailbox_msg_handle(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_OK;
	if (msg->src == MAJOR_MODULE)
	{
		if(msg->type == MAILBOX_MSG_TYPE_REQ) //send req msg to cpu2 other threads
		{
			switch (msg->event)
			{
				case EVENT_JPEG_DEC_DEINIT_NOTIFY:
				{
					ret = jpeg_dec_task_close();
				}
				break;
				case EVENT_JPEG_DEC_INIT_NOTIFY:
				{
					ret = jpeg_dec_task_open(msg->param);
				}
				break;
				case EVENT_JPEG_DEC_SET_ROTATE_ANGLE_NOTIFY:
				{
					uint32_t rotate_angle = msg->param;
					if (rotate_angle <= ROTATE_270)
					{
						jpeg_dec_set_rotate_angle(rotate_angle);
					}
					else
					{
						ret = BK_ERR_PARAM;
					}
				}
				break;
				default:
					break;
			}
			msg_send_rsp_to_media_minor_mailbox(msg, ret, MAJOR_MODULE);
		}
		else if(msg->type == MAILBOX_MSG_TYPE_RSP) //set semaphore from cpu2 other threads and delete from rsp list
		{
		}
		else if(msg->type == MAILBOX_MSG_TYPE_ABORT)
		{

		}
		else if(msg->type == MAILBOX_MSG_TYPE_NOTIFY)
		{
			switch (msg->event)
			{
				case EVENT_JPEG_DEC_START_NOTIFY:
				{
					jpeg_dec_task_send_msg(msg->event, (uint32_t)msg);
				}
				break;

				case EVENT_YUV_ROTATE_NOTIFY:
				{
					yuv_frame_rotate_handle((uint32_t)msg);
				}
				break;

				default:
					break;
			}
		}
		else
		{
			LOGE("%s unsupported type %x\n", __func__, msg->type);
		}
	}
	else
	{
	}
}

void media_minor_mailbox_msg_send(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_OK;

	if(msg->type == MAILBOX_MSG_TYPE_REQ) //send req msg to cpu1 and add to rsp list
	{
		ret = media_minor_mailbox_send_msg_to_media_major_mailbox(msg);
		if (ret != BK_OK)
		{
			msg_send_back_to_media_minor_mailbox(msg, BK_FAIL);
			LOGE("%s FAILED 2\n", __func__);
		}
	}
	else if(msg->type == MAILBOX_MSG_TYPE_RSP) //send rsp msg to cpu1
	{
		ret = media_minor_mailbox_send_msg_to_media_major_mailbox(msg);
		if (ret != BK_OK)
		{
			LOGE("%s %d FAILED\n", __func__, __LINE__);
		}
	}
	else if(msg->type == MAILBOX_MSG_TYPE_ABORT)
	{

	}
	else if(msg->type == MAILBOX_MSG_TYPE_NOTIFY)
	{
		ret = media_minor_mailbox_send_msg_to_media_major_mailbox(msg);
		if (ret != BK_OK)
		{
			LOGE("%s %d FAILED\n", __func__, __LINE__);
		}
	}
	else
	{
		LOGE("%s unsupported type %x\n", __func__, msg->type);
	}
}

static void media_minor_mailbox_message_handle(void)
{
	bk_err_t ret_minor_msg = BK_OK;
	media_mailbox_msg_t *node_msg = NULL;
	media_mailbox_msg_t *tmp_msg = NULL;
	media_msg_t media_msg;
	media_mailbox_msg_t jpeg_dec_to_media_major_msg = {0};
	LOGI("%s\n", __func__);
	media_minor_mailbox_inited = 1;
	rtos_set_semaphore(&media_minor_mailbox_init_sem);
	jpeg_dec_to_media_major_msg.event = EVENT_JPEG_DEC_INIT_NOTIFY;
	msg_send_notify_to_media_minor_mailbox(&jpeg_dec_to_media_major_msg, MAJOR_MODULE);

	while (1)
	{
		node_msg = media_mailbox_list_pop(&media_minor_mailbox_msg_queue_req);
		if (node_msg != NULL)
		{
			if ((node_msg->event >> MEDIA_EVT_BIT) == EXIT_EVENT)
			{
				goto exit;
			}
			if (node_msg->count > 2)
			{
				LOGE("%s %d msg send failed %d\n", __func__, __LINE__, node_msg->event);
				if (node_msg->type == MAILBOX_MSG_TYPE_REQ)
				{
					msg_send_back_to_media_minor_mailbox(node_msg, BK_FAIL);
				}
				continue;
			}
			switch (node_msg->dest)
			{
				case MINOR_MODULE:
					media_minor_mailbox_msg_handle(node_msg);
					break;

				case MAJOR_MODULE:
					media_minor_mailbox_msg_send(node_msg);
					break;

				default:
					break;
			}
		}
		else
		{
			if (media_minor_mailbox_sem)
			{
				ret_minor_msg = rtos_get_semaphore(&media_minor_mailbox_sem, BEKEN_WAIT_FOREVER);
			}
		}

		while(!rtos_is_queue_empty(&media_minor_mailbox_msg_queue))
		{
			ret_minor_msg = rtos_pop_from_queue(&media_minor_mailbox_msg_queue, &media_msg, 0);
			if (BK_OK == ret_minor_msg)
			{
				tmp_msg = (media_mailbox_msg_t *)media_msg.param;
				if (media_msg.event == 0)
				{
					media_mailbox_list_push(tmp_msg, &media_minor_mailbox_msg_queue_req);
				}
				else
				{
					media_mailbox_list_push(tmp_msg, &media_minor_mailbox_msg_queue_rsp);
				}
			}
			else
			{
				break;
			}
		}
	}

exit:

	media_minor_mailbox_deinit();

	media_minor_mailbox_th_hd = NULL;
	LOGE("delete task complete\n");

	/* delate task */
	rtos_delete_thread(NULL);


}

bk_err_t media_minor_mailbox_deinit(void)
{
	mb_chnl_close(CP1_MB_CHNL_MEDIA);

	media_mailbox_list_clear(&media_minor_mailbox_msg_queue_req, 1);
	media_mailbox_list_clear(&media_minor_mailbox_msg_queue_rsp, 0);

	if (media_minor_mailbox_msg_queue)
	{
		rtos_deinit_queue(&media_minor_mailbox_msg_queue);
		media_minor_mailbox_msg_queue = NULL;
	}

	if (media_minor_mailbox_init_sem)
	{
		rtos_deinit_semaphore(&media_minor_mailbox_init_sem);
		media_minor_mailbox_init_sem = NULL;
	}

	if (media_minor_mailbox_rsp_sem)
	{
		rtos_deinit_semaphore(&media_minor_mailbox_rsp_sem);
		media_minor_mailbox_rsp_sem = NULL;
	}

	if (media_minor_mailbox_sem)
	{
		rtos_deinit_semaphore(&media_minor_mailbox_sem);
		media_minor_mailbox_sem = NULL;
	}
	media_minor_mailbox_inited = 0;
	return BK_OK;

}

bk_err_t media_minor_mailbox_init(void)
{
	bk_err_t ret = BK_OK;
	LOGE("%s\n", __func__);

	if (media_minor_mailbox_inited != 0)
	{
		LOGE("media_minor_mailbox already init, exit!\n");
		return ret;
	}

	ret = rtos_init_semaphore(&media_minor_mailbox_sem, 1);
	if (ret != BK_OK)
	{
		LOGE("create media_minor_mailbox_sem failed\n");
		goto exit;
	}

	ret = rtos_init_semaphore(&media_minor_mailbox_rsp_sem, 1);
	if (ret != BK_OK)
	{
		LOGE("create media_minor_mailbox_rsp_sem failed\n");
		goto exit;
	}

	ret = rtos_init_semaphore(&media_minor_mailbox_init_sem, 1);
	if (ret != BK_OK)
	{
		LOGE("create media_minor_mailbox_init_sem failed\n");
		goto exit;
	}

	INIT_LIST_HEAD(&media_minor_mailbox_msg_queue_req);
	INIT_LIST_HEAD(&media_minor_mailbox_msg_queue_rsp);

	mb_chnl_open(CP1_MB_CHNL_MEDIA, NULL);
	mb_chnl_ctrl(CP1_MB_CHNL_MEDIA, MB_CHNL_SET_RX_ISR, media_minor_mailbox_mailbox_rx_isr);
	mb_chnl_ctrl(CP1_MB_CHNL_MEDIA, MB_CHNL_SET_TX_ISR, media_minor_mailbox_mailbox_tx_isr);
	mb_chnl_ctrl(CP1_MB_CHNL_MEDIA, MB_CHNL_SET_TX_CMPL_ISR, media_minor_mailbox_mailbox_tx_cmpl_isr);

	ret = rtos_init_queue(&media_minor_mailbox_msg_queue,
							"media_minor_mailbox_msg_queue",
							sizeof(media_msg_t),
							15);
	if (ret != BK_OK)
	{
		LOGE("create media_minor_mailbox_msg_queue fail\n");
		goto exit;
	}

	ret = rtos_create_thread(&media_minor_mailbox_th_hd,
							4,
							"media_minor_mailbox_thread",
							(beken_thread_function_t)media_minor_mailbox_message_handle,
							1024,
							NULL);

	if (ret != BK_OK)
	{
		LOGE("create mailbox minor thread fail\n");
		goto exit;
	}

	rtos_get_semaphore(&media_minor_mailbox_init_sem, BEKEN_WAIT_FOREVER);

	LOGI("mailboxs minor thread startup complete\n");

	return BK_OK;

exit:
	media_minor_mailbox_deinit();

	return ret;
}


