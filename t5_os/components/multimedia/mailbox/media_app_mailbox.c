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

#include "FreeRTOS.h"
#include "event_groups.h"

#include "bk_list.h"
#include "media_core.h"
#include "media_evt.h"
#include <driver/mailbox_channel.h>
#include "transfer_act.h"
#include "media_mailbox_list_util.h"
#include <modules/pm.h>

#include <driver/pwr_clk.h>

#include "aud_tras_drv.h"
#include "aud_tras.h"
#include "audio_coprocess.h"
#include "storage_act.h"
#include "lcd_act.h"
#include "camera_driver.h"
#if (CONFIG_CACHE_ENABLE)
#include "cache.h"
#endif

#define TAG "media_app_mailbox"


#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static LIST_HEADER_T media_app_mailbox_msg_queue_req;
static LIST_HEADER_T media_app_mailbox_msg_queue_rsp;

static beken_queue_t media_app_mailbox_msg_queue;

static beken_thread_t media_app_mailbox_th_hd = NULL;
static beken_semaphore_t media_app_mailbox_sem = NULL;
static beken_semaphore_t media_app_mailbox_rsp_sem = NULL;
static beken_semaphore_t media_app_mailbox_init_sem = NULL;
static uint8_t media_app_mailbox_inited = 0;

static uint8_t cpu1_boot = 0;
static EventGroupHandle_t cpu1_evt_handle;
static beken_mutex_t cpu1_evt_lock;
static beken_mutex_t cpu1_boot_lock;
static beken2_timer_t cpu1_power_timer;
static uint32_t cpu1_boot_request = 0;

media_share_ptr_t *media_share_ptr = NULL;

static bk_err_t media_send_msg_to_queue(media_mailbox_msg_t *param, uint8_t ori);

static bk_err_t msg_send_to_media_app_mailbox_list(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_OK;

	ret = media_mailbox_list_push(msg, &media_app_mailbox_msg_queue_req);

	if (ret == BK_OK)
	{
		rtos_set_semaphore(&media_app_mailbox_sem);
	}

	return ret;
}

bk_err_t msg_send_req_to_media_app_mailbox_sync(uint32_t event, uint32_t in_param, uint32_t *out_param)
{
	bk_err_t ret = BK_OK;
	media_mailbox_msg_t msg = {0};

	if (media_app_mailbox_inited == 0)
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
	msg.src = APP_MODULE;
	msg.dest = MAJOR_MODULE;
	msg.type = MAILBOX_MSG_TYPE_REQ;
	msg.count = 0;

	ret = media_app_mailbox_check_cpu1_state();
	if (ret != BK_OK)
	{
		LOGE("%s cpu1 not open\n", __func__);
		goto out;
	}

	ret = msg_send_to_media_app_mailbox_list(&msg);
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

bk_err_t msg_send_rsp_to_media_app_mailbox(media_mailbox_msg_t *msg, uint32_t result)
{
	bk_err_t ret = BK_OK;
	msg->src = APP_MODULE;
	msg->dest = MAJOR_MODULE;
	msg->type = MAILBOX_MSG_TYPE_RSP;
	msg->result = result;
	msg->count = 0;
	if (media_app_mailbox_inited == 0)
	{
		return BK_ERR_NOT_INIT;
	}

	ret = msg_send_to_media_app_mailbox_list(msg);
	if (ret != kNoErr)
	{
		LOGE("%s add to list fail\n", __func__);
	}
	return ret;
}

bk_err_t msg_send_notify_to_media_app_mailbox(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_OK;
	msg->src = APP_MODULE;
	msg->dest = MAJOR_MODULE;
	msg->type = MAILBOX_MSG_TYPE_NOTIFY;
	msg->count = 0;
	if (media_app_mailbox_inited == 0)
	{
		return BK_ERR_NOT_INIT;
	}

	ret = msg_send_to_media_app_mailbox_list(msg);
	if (ret != kNoErr)
	{
		LOGE("%s add to list fail\n", __func__);
	}
	return ret;
}

static bk_err_t msg_send_back_to_media_app_mailbox(media_mailbox_msg_t *msg, uint32_t result)
{
	bk_err_t ret = BK_OK;
	msg->src = APP_MODULE;
	msg->dest = APP_MODULE;
	msg->type = MAILBOX_MSG_TYPE_RSP;
	msg->result = result;
	msg->count = 0;
	if (media_app_mailbox_inited == 0)
	{
		return BK_ERR_NOT_INIT;
	}

	ret = msg_send_to_media_app_mailbox_list(msg);
	if (ret != kNoErr)
	{
		LOGE("%s add to list fail\n", __func__);
	}
	return ret;
}

static bk_err_t media_app_mailbox_send_msg_to_media_major_mailbox(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_OK;
	bk_err_t send_ack_flag = false;
	mb_chnl_cmd_t mb_cmd;

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
		msg_send_back_to_media_app_mailbox(msg, BK_FAIL);
		LOGE("%s %d FAILED\n", __func__, __LINE__);
	}
	else
	{
		ret = rtos_get_semaphore(&media_app_mailbox_rsp_sem, 2000);
		if(ret != BK_OK)
		{
			LOGE("%s %d rtos_get_semaphore error type=%x event=%x\n", __func__, __LINE__, msg->type, msg->event);
			if (msg->type == MAILBOX_MSG_TYPE_REQ)
			{
				msg->count++;
				msg_send_to_media_app_mailbox_list(msg);
			}
		}
		else if (send_ack_flag && msg->ack_flag != MAILBOX_MESSAGE_ACK)
		{
			if (msg->type == MAILBOX_MSG_TYPE_REQ)
			{
				msg->count++;
				msg_send_to_media_app_mailbox_list(msg);
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

	if (media_app_mailbox_msg_queue)
	{
		ret = rtos_push_to_queue(&media_app_mailbox_msg_queue, &msg, BEKEN_NO_WAIT);

		if (BK_OK != ret)
		{
			LOGE("%s failed\n", __func__);
			return BK_FAIL;
		}

		return ret;
	}
	return ret;
}

static void media_app_mailbox_cpu1_timer_handle(void *arg1, void *arg2)
{
	rtos_lock_mutex(&cpu1_boot_lock);

	if (cpu1_boot_request == 0)
	{
		LOGI("%s, cpu1 power off\n", __func__);
		bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_MULTIMEDIA, PM_POWER_MODULE_STATE_OFF);
	}
	else
	{
		LOGI("%s, cpu1 power off ignore\n", __func__);
	}

	rtos_unlock_mutex(&cpu1_boot_lock);

}

bk_err_t media_app_mailbox_check_cpu1_state(void)
{
	rtos_lock_mutex(&cpu1_evt_lock);

	bk_err_t ret = BK_OK;
	uint8_t count = 5;

	xEventGroupClearBits(cpu1_evt_handle, 1 << 0);

	while (cpu1_boot == 0 && count-- > 0)
	{
		xEventGroupWaitBits(cpu1_evt_handle, 1, true, true, 500 / bk_get_ms_per_tick());
		if (cpu1_boot == 0)
		{
			LOGE("%s, wait timeout\n", __func__);
		}
	}

	if (count == 255)
	{
		ret = BK_FAIL;
	}
	rtos_unlock_mutex(&cpu1_evt_lock);

	return ret;
}

void media_app_mailbox_cpu1_startup_request(void)
{
	rtos_lock_mutex(&cpu1_boot_lock);

	if (cpu1_boot_request == 0)
	{
		//LOGI("%s, cpu1 power on\n", __func__);
		bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_MULTIMEDIA, PM_POWER_MODULE_STATE_ON);
	}

	if (rtos_is_oneshot_timer_running(&cpu1_power_timer))
	{
		rtos_stop_oneshot_timer(&cpu1_power_timer);
	}

	cpu1_boot_request++;

	media_app_mailbox_check_cpu1_state();

	rtos_unlock_mutex(&cpu1_boot_lock);
}

void media_app_mailbox_cpu1_shutdown_request(void)
{
	rtos_lock_mutex(&cpu1_boot_lock);

	cpu1_boot_request--;

	if (cpu1_boot_request == 0)
	{
		if (!rtos_is_oneshot_timer_running(&cpu1_power_timer))
		{
			rtos_start_oneshot_timer(&cpu1_power_timer);
		}
		else
		{
			rtos_oneshot_reload_timer(&cpu1_power_timer);
		}
	}
	else
	{
		if (rtos_is_oneshot_timer_running(&cpu1_power_timer))
		{
			rtos_stop_oneshot_timer(&cpu1_power_timer);
		}
	}

	rtos_unlock_mutex(&cpu1_boot_lock);
}

static void media_app_mailbox_rx_isr(void *param, mb_chnl_cmd_t *cmd_buf)
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
	rtos_set_semaphore(&media_app_mailbox_sem);
}


static void media_app_mailbox_tx_isr(void *param)
{
}

static void media_app_mailbox_tx_cmpl_isr(beken_semaphore_t msg, mb_chnl_ack_t *ack_buf)
{
#if (CONFIG_CACHE_ENABLE)
    flush_all_dcache();
#endif

	if (media_app_mailbox_rsp_sem)
	{
		rtos_set_semaphore(&media_app_mailbox_rsp_sem);
	}
}

static void media_app_mailbox_msg_send(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_OK;

	if(msg->type == MAILBOX_MSG_TYPE_REQ) //send req msg to cpu1 and add to rsp list
	{
		ret = media_app_mailbox_send_msg_to_media_major_mailbox(msg);
		if (ret != BK_OK)
		{
			LOGE("%s %d FAILED\n", __func__, __LINE__);
		}
	}
	else if(msg->type == MAILBOX_MSG_TYPE_RSP) //send rsp msg to cpu1
	{
		ret = media_app_mailbox_send_msg_to_media_major_mailbox(msg);
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
	}
	else
	{
		LOGE("%s unsupported type %x\n", __func__, msg->type);
	}
}

static void media_app_mailbox_msg_handle(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_OK;
	aud_intf_mic_param_t *mic_param;
	media_msg_t media_msg;

	if(msg->type == MAILBOX_MSG_TYPE_REQ) //send req msg to cpu0 other threads
	{
		switch (msg->event)
		{
			case EVENT_AUD_MIC_DATA_NOTIFY:
			case EVENT_AUD_SPK_DATA_NOTIFY:
			{
				msg_send_rsp_to_media_app_mailbox(msg, 160);
#if 0
				//TODO  modify after
				aud_tras_drv_mic_notify_t *mic_notify = (aud_tras_drv_mic_notify_t *)msg->param;
				extern int demo_doorbell_udp_voice_send_packet(unsigned char *data, unsigned int len);
				int size = demo_doorbell_udp_voice_send_packet((unsigned char *)(mic_notify->ptr_data), mic_notify->length);
				if (size < 0) {
					LOGE("%s send audio data fail\n", __func__, size);
				} else {
					LOGD("%s send audio data complete \n", __func__, size);
				}
#else
				aud_tras_send_msg(AUD_TRAS_TX, (void *)msg->param);
#endif
			}
			break;
			case EVENT_MEDIA_DATA_NOTIFY:
				transfer_app_event_handle(msg);
				break;

			case EVENT_JPEG_DEC_SAVE_IND:
				media_msg.event = msg->event;
				media_msg.param = (uint32_t)msg;
				media_app_send_msg(&media_msg);
				break;

			case EVENT_USB_DATA_NOTIFY:
				usb_app_event_handle(msg);
				break;

			case EVENT_VID_CAPTURE_NOTIFY:
			case EVENT_VID_SAVE_ALL_NOTIFY:
				storage_app_event_handle(msg);
				break;

			case EVENT_LCD_PICTURE_ECHO_NOTIFY:
				ret = read_storage_file_to_mem_handle(msg);
				break;

			case EVENT_MEDIA_CPU1_POWERUP_IND:
			{
				LOGI("cpu1 start up\n");
				cpu1_boot = 1;
				xEventGroupSetBits(cpu1_evt_handle, 1 << 0);
				media_share_ptr = (media_share_ptr_t*)msg->param;
				msg_send_rsp_to_media_app_mailbox(msg, BK_OK);
			}
			break;

			case EVENT_MEDIA_CPU1_POWEROFF_IND:
			{
				LOGI("cpu1 shut down\n");
				cpu1_boot = 0;
				xEventGroupSetBits(cpu1_evt_handle, 1 << 1);
				media_mailbox_list_clear(&media_app_mailbox_msg_queue_req, 1);
				media_mailbox_list_clear(&media_app_mailbox_msg_queue_rsp, 0);
				msg_send_rsp_to_media_app_mailbox(msg, BK_OK);
			}
			break;

			default:
				break;
		}
#if CONFIG_ASDF_WORK_CPU1
		/* audio message handle */
		if (msg->event >> MEDIA_EVT_BIT == AUD_EVENT)
		{
			audio_msg_t audio_msg;
			audio_msg.event = msg->event;
			audio_msg.param = (uint32_t)msg;
			ret = audio_coprocess_send_msg(&audio_msg);
			if (ret != BK_OK)
			{
				LOGE("%s send msg to audio coprocessor fail: %d\n", __func__, ret);
			}
		}
#endif
	}
	else if(msg->type == MAILBOX_MSG_TYPE_RSP) //set semaphore from cpu0 other threads and delete from rsp list
	{

		if (msg->sem)
		{
			media_mailbox_list_del_node(msg->sem, &media_app_mailbox_msg_queue_rsp);
			ret = rtos_set_semaphore(&msg->sem);

			if (ret != BK_OK)
			{
				LOGE("%s semaphore set failed: %d\n", __func__, ret);
			}
		}
		else
		{
			media_mailbox_list_del_node_by_event(msg->event, &media_app_mailbox_msg_queue_rsp);
		}
	}
	else if(msg->type == MAILBOX_MSG_TYPE_ABORT)
	{
	}
	else if(msg->type == MAILBOX_MSG_TYPE_NOTIFY)
	{
		switch (msg->event)
		{
			case EVENT_DVP_EOF_NOTIFY:
				bk_dvp_camera_eof_handler();
				break;

			case EVENT_DMA_RESTART_NOTIFY:
				dvp_camera_reset_dma();
				break;

			case EVENT_AUD_SPK_DATA_NOTIFY:
				aud_intf_send_msg(AUD_INTF_EVENT_SPK_RX, 0, msg->param);
				break;

			case EVENT_AUD_MIC_DATA_NOTIFY:
				mic_param = (aud_intf_mic_param_t *)msg->param;
				aud_intf_send_msg(AUD_INTF_EVENT_MIC_TX, mic_param->ptr_data, mic_param->length);
				break;

			case EVENT_UAC_CONNECT_STATE_NOTIFY:
				aud_intf_send_msg(AUD_INTF_EVENT_UAC_STATE, msg->param, 0);
				break;

			case EVENT_UVC_DEVICE_INFO_NOTIFY:
				media_msg.event = msg->event;
				media_msg.param = msg->param;
				media_app_send_msg(&media_msg);
				break;

			case EVENT_TRANS_LOG_NOTIFY:
				transfer_app_event_handle(msg);
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

static void media_app_mailbox_message_handle(void)
{
	bk_err_t ret_app_msg = BK_OK;
	media_mailbox_msg_t *node_msg = NULL;
	media_mailbox_msg_t *tmp_msg = NULL;
	media_msg_t media_msg;
	LOGI("%s\n", __func__);
	media_app_mailbox_inited = 1;

#if (CONFIG_ASDF_WORK_CPU1)
	audio_coprocess_task_init();
#endif
	rtos_set_semaphore(&media_app_mailbox_init_sem);

	while (1)
	{
		node_msg = media_mailbox_list_pop(&media_app_mailbox_msg_queue_req);

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
					msg_send_back_to_media_app_mailbox(node_msg, BK_FAIL);
				}
				continue;
			}
			switch (node_msg->dest)
			{
				case APP_MODULE:
					media_app_mailbox_msg_handle(node_msg);
					break;
				case MAJOR_MODULE:
					media_app_mailbox_msg_send(node_msg);
					break;

				default:
					break;
			}
		}
		else
		{
			if (media_app_mailbox_sem)
			{
				ret_app_msg = rtos_get_semaphore(&media_app_mailbox_sem, BEKEN_WAIT_FOREVER);
			}
		}

		while(!rtos_is_queue_empty(&media_app_mailbox_msg_queue))
		{
			ret_app_msg = rtos_pop_from_queue(&media_app_mailbox_msg_queue, &media_msg, 0);
			if (BK_OK == ret_app_msg)
			{
				tmp_msg = (media_mailbox_msg_t *)media_msg.param;
				if (media_msg.event == 0)
				{
					media_mailbox_list_push(tmp_msg, &media_app_mailbox_msg_queue_req);
				}
				else
				{
					media_mailbox_list_push(tmp_msg, &media_app_mailbox_msg_queue_rsp);
				}
			}
			else
			{
				break;
			}
		}
	}

exit:
	media_app_mailbox_deinit();

	/* delate task */
	media_app_mailbox_th_hd = NULL;

	LOGE("delete task complete\n");

	rtos_delete_thread(NULL);
}

bk_err_t media_app_mailbox_deinit(void)
{
	if (rtos_is_oneshot_timer_running(&cpu1_power_timer))
	{
		rtos_stop_oneshot_timer(&cpu1_power_timer);
	}

	if (rtos_is_oneshot_timer_init(&cpu1_power_timer))
	{
		rtos_deinit_oneshot_timer(&cpu1_power_timer);
	}

	mb_chnl_close(MB_CHNL_MEDIA);

	media_mailbox_list_clear(&media_app_mailbox_msg_queue_req, 1);
	media_mailbox_list_clear(&media_app_mailbox_msg_queue_rsp, 0);
	if (media_app_mailbox_msg_queue)
	{
		rtos_deinit_queue(&media_app_mailbox_msg_queue);
		media_app_mailbox_msg_queue = NULL;
	}

	if (media_app_mailbox_init_sem)
	{
		rtos_deinit_semaphore(&media_app_mailbox_init_sem);
		media_app_mailbox_init_sem = NULL;
	}

	if (media_app_mailbox_rsp_sem)
	{
		rtos_deinit_semaphore(&media_app_mailbox_rsp_sem);
		media_app_mailbox_rsp_sem = NULL;
	}
	if (media_app_mailbox_sem)
	{
		LOGI("%s, %d\r\n", __func__, __LINE__);
		rtos_deinit_semaphore(&media_app_mailbox_sem);
		media_app_mailbox_sem = NULL;
	}
	media_app_mailbox_inited = 0;

	return BK_OK;
}

bk_err_t media_app_mailbox_init(void)
{
	bk_err_t ret = BK_OK;

	if (media_app_mailbox_inited != 0)
	{
		LOGE("media_app_mailbox already init, exit!\n");
		goto exit;
	}

	rtos_init_mutex(&cpu1_evt_lock);
	rtos_init_mutex(&cpu1_boot_lock);

	cpu1_evt_handle = xEventGroupCreate();

	if (cpu1_evt_handle == NULL)
	{
		LOGE("media_app_mailbox cpu1_evt_handle null!\n");
		goto exit;
	}

	if (!rtos_is_oneshot_timer_init(&cpu1_power_timer))
	{
		ret = rtos_init_oneshot_timer(&cpu1_power_timer, 500, media_app_mailbox_cpu1_timer_handle, NULL, NULL);

		if (ret != BK_OK)
		{
			LOGE("create decoder timer failed\n");
		}
	}


	ret = media_app_init();

	if (ret != BK_OK)
	{
		LOGE("init media modules states failed\n");
		goto exit;
	}

	ret = rtos_init_semaphore(&media_app_mailbox_sem, 1);

	if (ret != BK_OK)
	{
		LOGE("create media_app_mailbox_sem failed\n");
		goto exit;
	}

	LOGI("%s sema ok %p\n", __func__, media_app_mailbox_sem);

	ret = rtos_init_semaphore(&media_app_mailbox_rsp_sem, 1);
	if (ret != BK_OK)
	{
		LOGE("create media_app_mailbox_rsp_sem failed\n");
		goto exit;
	}

	ret = rtos_init_semaphore(&media_app_mailbox_init_sem, 1);
	if (ret != BK_OK)
	{
		LOGE("create media_app_mailbox_init_sem failed\n");
		goto exit;
	}

	INIT_LIST_HEAD(&media_app_mailbox_msg_queue_req);
	INIT_LIST_HEAD(&media_app_mailbox_msg_queue_rsp);

	mb_chnl_open(MB_CHNL_MEDIA, NULL);
	mb_chnl_ctrl(MB_CHNL_MEDIA, MB_CHNL_SET_RX_ISR, media_app_mailbox_rx_isr);
	mb_chnl_ctrl(MB_CHNL_MEDIA, MB_CHNL_SET_TX_ISR, media_app_mailbox_tx_isr);
	mb_chnl_ctrl(MB_CHNL_MEDIA, MB_CHNL_SET_TX_CMPL_ISR, media_app_mailbox_tx_cmpl_isr);

	ret = rtos_init_queue(&media_app_mailbox_msg_queue,
							"media_app_mailbox_msg_queue",
							sizeof(media_msg_t),
							20);
	if (ret != BK_OK)
	{
		LOGE("create media_app_mailbox_msg_queue fail\n");
		goto exit;
	}

	ret = rtos_create_thread(&media_app_mailbox_th_hd,
							 4,
							 "media_app_mailbox_thread",
							 (beken_thread_function_t)media_app_mailbox_message_handle,
							 2048,
							 NULL);

	if (ret != BK_OK)
	{
		LOGE("create mailbox app thread fail\n");
		goto exit;
	}

	rtos_get_semaphore(&media_app_mailbox_init_sem, BEKEN_WAIT_FOREVER);

	LOGI("mailbox app thread startup complete\n");

	return BK_OK;

exit:
	media_app_mailbox_deinit();

	return ret;
}


