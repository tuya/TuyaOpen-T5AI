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

#include "lcd_act.h"

#include "bk_list.h"
#include "media_core.h"
#include "media_evt.h"
#include <driver/dvp_camera.h>
#include <driver/mailbox_channel.h>
#include "media_mailbox_list_util.h"


#include "lcd_act.h"
#include "transfer_act.h"
#include "storage_act.h"
#include "camera_act.h"
#include "frame_buffer.h"
#include <modules/pm.h>
#include "yuv_encode.h"

#include "aud_tras_drv.h"
#include "aud_intf.h"
#include "aud_intf_types.h"
#if (CONFIG_CACHE_ENABLE)
#include "cache.h"
#endif

#define TAG "media_major_mailbox"

//#define CONFIG_MINOR

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static LIST_HEADER_T media_major_mailbox_msg_queue_req;
static LIST_HEADER_T media_major_mailbox_msg_queue_rsp;

static beken_queue_t media_major_mailbox_msg_queue;

static beken_thread_t media_major_mailbox_th_hd = NULL;
static beken_semaphore_t media_major_mailbox_sem = NULL;
static beken_semaphore_t media_major_mailbox_rsp_sem = NULL;
static beken_semaphore_t media_major_mailbox_init_sem = NULL;

static uint8_t media_major_mailbox_inited = 0;

static bk_err_t media_send_msg_to_queue(media_mailbox_msg_t *param, uint8_t ori);

static bk_err_t msg_send_to_media_major_mailbox_list(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_OK;
	ret = media_mailbox_list_push(msg, &media_major_mailbox_msg_queue_req);

	if (ret == BK_OK)
	{
		rtos_set_semaphore(&media_major_mailbox_sem);
	}
	return ret;
}

bk_err_t msg_send_req_to_media_major_mailbox_sync(uint32_t event, uint32_t dest, uint32_t in_param, uint32_t *out_param)
{
	bk_err_t ret = BK_OK;
	media_mailbox_msg_t msg = {0};

	if (media_major_mailbox_inited == 0)
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
	msg.src = MAJOR_MODULE;
	msg.dest = dest;
	msg.type = MAILBOX_MSG_TYPE_REQ;
	msg.count = 0;

	ret = msg_send_to_media_major_mailbox_list(&msg);
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

bk_err_t msg_send_rsp_to_media_major_mailbox(media_mailbox_msg_t *msg, uint32_t result, uint32_t dest)
{
#if CONFIG_TUYA_LOGIC_MODIFY
    return BK_OK;
#endif // CONFIG_TUYA_LOGIC_MODIFY

	bk_err_t ret = BK_OK;
	msg->src = MAJOR_MODULE;
	msg->dest = dest;
	msg->type = MAILBOX_MSG_TYPE_RSP;
	msg->result = result;
	msg->count = 0;

	if (media_major_mailbox_inited == 0)
	{
		return BK_ERR_NOT_INIT;

}
	ret = msg_send_to_media_major_mailbox_list(msg);
	if (ret != kNoErr)
	{
		LOGE("%s add to list fail\n", __func__);
	}
	return ret;
}

bk_err_t msg_send_notify_to_media_major_mailbox(media_mailbox_msg_t *msg, uint32_t dest)
{
	bk_err_t ret = BK_OK;
	msg->src = MAJOR_MODULE;
	msg->dest = dest;
	msg->type = MAILBOX_MSG_TYPE_NOTIFY;
	msg->count = 0;

	if (media_major_mailbox_inited == 0)
	{
		return BK_ERR_NOT_INIT;
	}

	ret = msg_send_to_media_major_mailbox_list(msg);
	if (ret != kNoErr)
	{
		LOGE("%s add to list fail\n", __func__);
	}
	return ret;
}


static bk_err_t msg_send_back_to_media_major_mailbox(media_mailbox_msg_t *msg, uint32_t result)
{
	bk_err_t ret = BK_OK;

	if (media_major_mailbox_inited == 0)
	{
		return BK_ERR_NOT_INIT;
	}

	// maybe need send again
#if 0
	msg->src = MAJOR_MODULE;
	msg->dest = MAJOR_MODULE;
	msg->type = MAILBOX_MSG_TYPE_RSP;
	msg->result = result;
#endif
	ret = msg_send_to_media_major_mailbox_list(msg);
	if (ret != kNoErr)
	{
		LOGE("%s add to list fail\n", __func__);
	}

	return ret;
}

static bk_err_t media_major_mailbox_send_msg_to_media_app_mailbox(media_mailbox_msg_t *msg)
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
	ret = mb_chnl_write(MB_CHNL_MEDIA, &mb_cmd);

	if (ret != BK_OK)
	{
		msg_send_back_to_media_major_mailbox(msg, BK_FAIL);
		LOGE("%s %d FAILED\n", __func__, __LINE__);
	}
	else
	{
		ret = rtos_get_semaphore(&media_major_mailbox_rsp_sem, 2000);
		if(ret != BK_OK)
		{
			LOGE("%s %d rtos_get_semaphore error type=%x event=%x\n", __func__, __LINE__, msg->type, msg->event);
			if (msg->type == MAILBOX_MSG_TYPE_REQ)
			{
				msg->count++;
				msg_send_to_media_major_mailbox_list(msg);
			}
		}
		else if (send_ack_flag && msg->ack_flag != MAILBOX_MESSAGE_ACK)
		{
			if (msg->type == MAILBOX_MSG_TYPE_REQ)
			{
				msg->count++;
				msg_send_to_media_major_mailbox_list(msg);
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

static bk_err_t media_major_mailbox_send_msg_to_media_minor_mailbox(media_mailbox_msg_t *msg)
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
	ret = mb_chnl_write(CP2_MB_CHNL_MEDIA, &mb_cmd);

	if (ret != BK_OK)
	{
		msg_send_back_to_media_major_mailbox(msg, BK_FAIL);
		LOGE("%s %d FAILED\n", __func__, __LINE__);
	}
	else
	{
		ret = rtos_get_semaphore(&media_major_mailbox_rsp_sem, 2000);
		if(ret != BK_OK)
		{
			LOGE("%s %d rtos_get_semaphore error type=%x event=%x\n", __func__, __LINE__, msg->type, msg->event);
			if (msg->type == MAILBOX_MSG_TYPE_REQ)
			{
				msg->count++;
				msg_send_to_media_major_mailbox_list(msg);
			}
		}
		else if (send_ack_flag && msg->ack_flag != MAILBOX_MESSAGE_ACK)
		{
			if (msg->type == MAILBOX_MSG_TYPE_REQ)
			{
				msg->count++;
				msg_send_to_media_major_mailbox_list(msg);
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

	if (media_major_mailbox_msg_queue)
	{
		ret = rtos_push_to_queue(&media_major_mailbox_msg_queue, &msg, BEKEN_NO_WAIT);

		if (BK_OK != ret)
		{
			LOGE("%s failed\n", __func__);
			return BK_FAIL;
		}

		return ret;
	}
	return ret;
}

static void media_major_mailbox_mailbox_rx_isr(void *param, mb_chnl_cmd_t *cmd_buf)
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
		msg->count = 0;
		media_send_msg_to_queue(msg, 0);
	}
	else
	{
		LOGE("%s %d msg is NULL\n", __func__, __LINE__);
	}

	rtos_set_semaphore(&media_major_mailbox_sem);
}

static void media_major_mailbox_mailbox_from_cp0_rx_isr(void *param, mb_chnl_cmd_t *cmd_buf)
{
	media_major_mailbox_mailbox_rx_isr(param, cmd_buf);
}

static void media_major_mailbox_mailbox_from_cp2_rx_isr(void *param, mb_chnl_cmd_t *cmd_buf)
{
	media_major_mailbox_mailbox_rx_isr(param, cmd_buf);
}

static void media_major_mailbox_mailbox_tx_isr(void *param)
{
}

static void media_major_mailbox_mailbox_to_cp0_tx_isr(void *param)
{
	media_major_mailbox_mailbox_tx_isr(param);
}

static void media_major_mailbox_mailbox_to_cp2_tx_isr(void *param)
{
	media_major_mailbox_mailbox_tx_isr(param);
}

static void media_major_mailbox_mailbox_tx_cmpl_isr(void *param, mb_chnl_ack_t *ack_buf)
{
#if (CONFIG_CACHE_ENABLE)
    flush_all_dcache();
#endif

	if (media_major_mailbox_rsp_sem)
	{
		rtos_set_semaphore(&media_major_mailbox_rsp_sem);
	}
}

static void media_major_mailbox_mailbox_to_cp0_tx_cmpl_isr(void *param, mb_chnl_ack_t *ack_buf)
{
	media_major_mailbox_mailbox_tx_cmpl_isr(param, ack_buf);
}

static void media_major_mailbox_mailbox_to_cp2_tx_cmpl_isr(void *param, mb_chnl_ack_t *ack_buf)
{
	media_major_mailbox_mailbox_tx_cmpl_isr(param, ack_buf);
}

void media_major_mailbox_msg_handle(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_OK;
	media_msg_t media_msg;
	if (msg->src == APP_MODULE)
	{
		if(msg->type == MAILBOX_MSG_TYPE_REQ) //send req msg to cpu1 other threads
		{
			switch (msg->event >> MEDIA_EVT_BIT)
			{
				case LCD_EVENT:
					media_msg.event = msg->event;
					media_msg.param = (uint32_t)msg;
					media_send_msg(&media_msg);
					break;

				case UVC_PIPELINE_EVENT:
					media_msg.event = msg->event;
					media_msg.param = (uint32_t)msg;
					media_send_msg(&media_msg);
					break;

				case LVGL_EVENT:
					media_msg.event = msg->event;
					media_msg.param = (uint32_t)msg;
					media_send_msg(&media_msg);
					break;

				case CAM_EVENT:
					media_msg.event = msg->event;
					media_msg.param = (uint32_t)msg;
					media_send_msg(&media_msg);
					break;

				case TRS_EVENT:
					media_msg.event = msg->event;
					media_msg.param = (uint32_t)msg;
					media_send_msg(&media_msg);
					break;

				case USB_TRS_EVENT:
					media_msg.event = msg->event;
					media_msg.param = (uint32_t)msg;
					media_send_msg(&media_msg);
					break;

				case STORAGE_EVENT:
					media_msg.event = msg->event;
					media_msg.param = (uint32_t)msg;
					media_send_msg(&media_msg);
					break;

				case AUD_EVENT:
				case AUD_NOTIFY:
					audio_event_handle(msg);
					break;

				case BT_EVENT:
                    media_msg.event = msg->event;
                    media_msg.param = (uint32_t)msg;
                    media_send_msg(&media_msg);
				    break;

//				case MAILBOX_NOTIFY:
//					switch (msg->event)
//					{
//						case EVENT_MEDIA_DATA_NOTIFY:
//						case EVENT_VID_CAPTURE_NOTIFY:
//						case EVENT_VID_SAVE_ALL_NOTIFY:
//							media_major_mailbox_rsp_handle_ext(msg);
//							break;
//
//						case EVENT_AUD_MIC_DATA_NOTIFY:
//						case EVENT_AUD_SPK_DATA_NOTIFY:
//							audio_event_handle(msg);
//							break;
//
//						case EVENT_LCD_PICTURE_ECHO_NOTIFY:
//							lcd_display_echo_event_handle(msg);
//							break;
//
//						default:
//							break;
//					}
//					break;
				case FRAME_BUFFER_EVENT:
					media_msg.event = msg->event;
					media_msg.param = (uint32_t)msg;
					media_send_msg(&media_msg);
					break;

				default:
					break;
			}
		}
		else if(msg->type == MAILBOX_MSG_TYPE_RSP) //set semaphore from cpu1 other threads and delete from rsp list
		{
			if( msg->event == EVENT_LCD_PICTURE_ECHO_NOTIFY)
			{
				media_mailbox_list_del_node(msg->sem, &media_major_mailbox_msg_queue_rsp);
				lcd_display_echo_event_handle(msg);
				return;
			}

			if (msg->sem)
			{
				media_mailbox_list_del_node(msg->sem, &media_major_mailbox_msg_queue_rsp);
				ret = rtos_set_semaphore(&msg->sem);

				if (ret != BK_OK)
				{
					LOGE("%s semaphore set failed: %d, %x\n", __func__, ret, msg->event);
				}
			}
			else
			{
				media_mailbox_list_del_node_by_event(msg->event, &media_major_mailbox_msg_queue_rsp);
			}
		}
		else if(msg->type == MAILBOX_MSG_TYPE_ABORT)
		{

		}
		else if(msg->type == MAILBOX_MSG_TYPE_NOTIFY)
		{

		}
		else
		{
			LOGE("%s unsupported type %x\n", __func__, msg->type);
		}
	}
	else if (msg->src == MINOR_MODULE)
	{
        if(msg->type == MAILBOX_MSG_TYPE_RSP) //set semaphore from cpu1 other threads and delete from rsp list
        {
            if (msg->sem)
            {
                media_mailbox_list_del_node(msg->sem, &media_major_mailbox_msg_queue_rsp);
                ret = rtos_set_semaphore(&msg->sem);

                if (ret != BK_OK)
                {
                    LOGE("%s semaphore set failed: %d\n", __func__, ret);
                }
            }
            else
            {
                media_mailbox_list_del_node_by_event(msg->event, &media_major_mailbox_msg_queue_rsp);
            }
        }
		else if(msg->type == MAILBOX_MSG_TYPE_NOTIFY)
		{
			switch (msg->event)
			{
				case EVENT_JPEG_DEC_START_COMPLETE_NOTIFY:
				{
				#if CONFIG_MEDIA_PIPELINE
					jpeg_decode_task_send_msg(JPEGDEC_FRAME_CP2_FINISH, msg->result);
				#endif
				}
				break;
				case EVENT_JPEG_DEC_INIT_NOTIFY:
				{
				#if CONFIG_MEDIA_PIPELINE
					jpeg_decode_cp2_init_notify();
				#endif
				}
				break;

				case EVENT_YUV_ROTATE_NOTIFY:
				#if CONFIG_DVP_CAMERA
					bk_dvp_camera_cp2_rotate_finish();
				#endif
				break;

				default:
					break;
			}
		
}
	}
	else
	{
		if(msg->type == MAILBOX_MSG_TYPE_RSP) //set semaphore from cpu1 other threads and delete from rsp list
		{
			if (msg->sem)
			{
				media_mailbox_list_del_node(msg->sem, &media_major_mailbox_msg_queue_rsp);
				ret = rtos_set_semaphore(&msg->sem);

				if (ret != BK_OK)
				{
					LOGE("%s semaphore set failed: %d\n", __func__, ret);
				}
			}
			else
			{
				media_mailbox_list_del_node_by_event(msg->event, &media_major_mailbox_msg_queue_rsp);
			}
		}
	}
}

void media_major_mailbox_msg_send_to_app(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_OK;


	if(msg->type == MAILBOX_MSG_TYPE_REQ) //send req msg to cpu0 and add to rsp list
	{
		ret = media_major_mailbox_send_msg_to_media_app_mailbox(msg);
		if (ret != BK_OK)
		{
			LOGE("%s(%d) FAILED \n", __func__, __LINE__);
		}
	}
	else if(msg->type == MAILBOX_MSG_TYPE_RSP) //send rsp msg to cpu0
	{
		ret = media_major_mailbox_send_msg_to_media_app_mailbox(msg);
		if (ret != BK_OK)
		{
			LOGE("%s(%d) FAILED \n", __func__, __LINE__);
		}
	}
	else if(msg->type == MAILBOX_MSG_TYPE_ABORT)
	{

	}
	else if(msg->type == MAILBOX_MSG_TYPE_NOTIFY)
	{
		ret = media_major_mailbox_send_msg_to_media_app_mailbox(msg);
		if (ret != BK_OK)
		{
			LOGE("%s(%d) FAILED \n", __func__, __LINE__);
		}
	}
	else
	{
		LOGE("%s unsupported type %x\n", __func__, msg->type);
	}
}

void media_major_mailbox_msg_send_to_minor(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_OK;
	if(msg->type == MAILBOX_MSG_TYPE_REQ) //send req msg to cpu2 and add to rsp list
	{
		ret = media_major_mailbox_send_msg_to_media_minor_mailbox(msg);
		if (ret != BK_OK)
		{
			LOGE("%s(%d) FAILED \n", __func__, __LINE__);
		}

	}
	else if(msg->type == MAILBOX_MSG_TYPE_RSP) //send rsp msg to cpu0
	{
	}
	else if(msg->type == MAILBOX_MSG_TYPE_ABORT)
	{

	}
	else if(msg->type == MAILBOX_MSG_TYPE_NOTIFY)
	{
		ret = media_major_mailbox_send_msg_to_media_minor_mailbox(msg);
		if(ret != BK_OK)
		{
			LOGE("%s %d send msg error type=%x event=%x\n", __func__, __LINE__, msg->type, msg->event);
		}
	}
	else
	{
		LOGE("%s unsupported type %x\n", __func__, msg->type);
	}
}

static void media_major_mailbox_message_handle(void)
{
	bk_err_t ret_major_msg = BK_OK;
	media_mailbox_msg_t *node_msg = NULL;
	media_mailbox_msg_t *tmp_msg = NULL;
	media_msg_t media_msg;
	LOGI("%s\n", __func__);
	media_major_mailbox_inited = 1;
	media_ui_init();
	rtos_set_semaphore(&media_major_mailbox_init_sem);

	while (1)
	{
		node_msg = media_mailbox_list_pop(&media_major_mailbox_msg_queue_req);
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
					msg_send_back_to_media_major_mailbox(node_msg, BK_FAIL);
				}
				continue;
			}
			switch (node_msg->dest)
			{
				case MAJOR_MODULE:
					media_major_mailbox_msg_handle(node_msg);
					break;

				case APP_MODULE:
					media_major_mailbox_msg_send_to_app(node_msg);
					break;

				case MINOR_MODULE:
					media_major_mailbox_msg_send_to_minor(node_msg);
					break;

				default:
					break;
			}
		}
		else
		{
			if (media_major_mailbox_sem)
			{
				ret_major_msg = rtos_get_semaphore(&media_major_mailbox_sem, BEKEN_WAIT_FOREVER);
			}
		}

		while(!rtos_is_queue_empty(&media_major_mailbox_msg_queue))
		{
			ret_major_msg = rtos_pop_from_queue(&media_major_mailbox_msg_queue, &media_msg, 0);
			if (BK_OK == ret_major_msg)
			{
				tmp_msg = (media_mailbox_msg_t *)media_msg.param;
				if (media_msg.event == 0)
				{
					media_mailbox_list_push(tmp_msg, &media_major_mailbox_msg_queue_req);
				}
				else
				{
					media_mailbox_list_push(tmp_msg, &media_major_mailbox_msg_queue_rsp);
				}
			}
			else
			{
				break;
			}
		}
	}

exit:

	media_ui_deinit();

	media_major_mailbox_deinit();

	media_major_mailbox_th_hd = NULL;

	LOGE("delete task complete\n");
	/* delate task */
	rtos_delete_thread(NULL);

}

bk_err_t media_major_mailbox_deinit(void)
{
	mb_chnl_close(MB_CHNL_MEDIA);
	mb_chnl_close(CP2_MB_CHNL_MEDIA);

	media_mailbox_list_clear(&media_major_mailbox_msg_queue_req, 1);
	media_mailbox_list_clear(&media_major_mailbox_msg_queue_rsp, 0);

	if (media_major_mailbox_msg_queue)
	{
		rtos_deinit_queue(&media_major_mailbox_msg_queue);
		media_major_mailbox_msg_queue = NULL;
	}

	if (media_major_mailbox_init_sem)
	{
		rtos_deinit_semaphore(&media_major_mailbox_init_sem);
		media_major_mailbox_init_sem = NULL;
	}

	if (media_major_mailbox_rsp_sem)
	{
		rtos_deinit_semaphore(&media_major_mailbox_rsp_sem);
		media_major_mailbox_rsp_sem = NULL;
	}

	if (media_major_mailbox_sem)
	{
		rtos_deinit_semaphore(&media_major_mailbox_sem);
		media_major_mailbox_sem = NULL;
	}
	media_major_mailbox_inited = 0;
	return BK_OK;

}

bk_err_t media_major_mailbox_init(void)
{
	bk_err_t ret = BK_OK;
	LOGI("%s\n", __func__);

	if (media_major_mailbox_inited != 0)
	{
		LOGE("media_major_mailbox already init, exit!\n");
		return ret;
	}

	ret = rtos_init_semaphore(&media_major_mailbox_sem, 1);
	if (ret != BK_OK)
	{
		LOGE("create media_major_mailbox_sem failed\n");
		goto exit;
	}

	ret = rtos_init_semaphore(&media_major_mailbox_rsp_sem, 1);
	if (ret != BK_OK)
	{
		LOGE("create media_major_mailbox_rsp_sem failed\n");
		goto exit;
	}

	ret = rtos_init_semaphore(&media_major_mailbox_init_sem, 1);
	if (ret != BK_OK)
	{
		LOGE("create media_major_mailbox_init_sem failed\n");
		goto exit;
	}

	INIT_LIST_HEAD(&media_major_mailbox_msg_queue_req);
	INIT_LIST_HEAD(&media_major_mailbox_msg_queue_rsp);

	mb_chnl_open(MB_CHNL_MEDIA, NULL);
	mb_chnl_ctrl(MB_CHNL_MEDIA, MB_CHNL_SET_RX_ISR, media_major_mailbox_mailbox_from_cp0_rx_isr);
	mb_chnl_ctrl(MB_CHNL_MEDIA, MB_CHNL_SET_TX_ISR, media_major_mailbox_mailbox_to_cp0_tx_isr);
	mb_chnl_ctrl(MB_CHNL_MEDIA, MB_CHNL_SET_TX_CMPL_ISR, media_major_mailbox_mailbox_to_cp0_tx_cmpl_isr);

	mb_chnl_open(CP2_MB_CHNL_MEDIA, NULL);
	mb_chnl_ctrl(CP2_MB_CHNL_MEDIA, MB_CHNL_SET_RX_ISR, media_major_mailbox_mailbox_from_cp2_rx_isr);
	mb_chnl_ctrl(CP2_MB_CHNL_MEDIA, MB_CHNL_SET_TX_ISR, media_major_mailbox_mailbox_to_cp2_tx_isr);
	mb_chnl_ctrl(CP2_MB_CHNL_MEDIA, MB_CHNL_SET_TX_CMPL_ISR, media_major_mailbox_mailbox_to_cp2_tx_cmpl_isr);

	ret = rtos_init_queue(&media_major_mailbox_msg_queue,
							"media_major_mailbox_msg_queue",
							sizeof(media_msg_t),
							20);
	if (ret != BK_OK)
	{
		LOGE("create media_major_mailbox_msg_queue fail\n");
		goto exit;
	}

	ret = rtos_create_thread(&media_major_mailbox_th_hd,
							 4,
							 "media_major_mailbox_thread",
							 (beken_thread_function_t)media_major_mailbox_message_handle,
							 2048,
							 NULL);

	if (ret != BK_OK)
	{
		LOGE("create mailbox major thread fail\n");
		goto exit;
	}

	rtos_get_semaphore(&media_major_mailbox_init_sem, BEKEN_WAIT_FOREVER);

	LOGI("mailboxs major thread startup complete\n");

	return BK_OK;

exit:
	media_major_mailbox_deinit();

	return ret;
}


