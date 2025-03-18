#include "bk_usb_mailbox.h"

// Modified by TUYA Start
#include "gpio_map.h"
// Modified by TUYA End

#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif
#if (CONFIG_CPU_CNT > 2)
#include "components/system.h"
#endif
#if CONFIG_MAILBOX
/*******************************************************************************
*                      Function Declarations
*******************************************************************************/
#if CONFIG_USB_MAILBOX_SLAVE
#if CONFIG_USB_MAILBOX_SLAVE_CPU0_TO_CPU1
#define USB_MAILBOX_CHNL MB_CHNL_USB
#elif CONFIG_USB_MAILBOX_SLAVE_CPU2_TO_CPU1
#define USB_MAILBOX_CHNL CP1_MB_CHNL_USB
#else
#define USB_MAILBOX_CHNL MB_CHNL_USB
#endif

static beken_thread_t  s_usb_mailbox_slave_thread_hdl = NULL;
static beken_queue_t s_usb_mailbox_slave_msg_que = NULL;
static beken_queue_t s_usb_mailbox_slave_packet_msg_que = NULL;

static beken_queue_t s_usb_mailbox_slave_uvc_packet_msg_que = NULL;
static beken_queue_t s_usb_mailbox_slave_uvc_packet_dual_msg_que = NULL;

static beken_semaphore_t s_usb_mailbox_slave_uvc_push_sem = NULL;
static beken_semaphore_t s_usb_mailbox_slave_uvc_push_dual_sem = NULL;

static beken_semaphore_t s_usb_mailbox_slave_disconnect_sem = NULL;
static beken_semaphore_t s_usb_mailbox_slave_connect_sem = NULL;

#define USB_MAILBOOX_SLAVE_PACKET_WAIT_MS 10

#define USB_MAILBOX_SLAVE_PACKET_QITEM_COUNT 1
#define USB_MAILBOX_SLAVE_UVC_PACKET_QITEM_COUNT 1
#define USB_MAILBOX_SLAVE_UVC_PACKET_DUAL_QITEM_COUNT 1

#define USB_MAILBOX_SLAVE_QITEM_COUNT 10
#define USB_MAILBOX_SLAVE_TASK_PRIO 2
#define USB_MAILBOX_SLAVE_TASK_SIZE 2048

bk_err_t task_mb_chnl_write(uint8_t log_chnl, mb_chnl_cmd_t *cmd_buf)
{
       uint8_t   busy = 0;
       bk_err_t  ret_val = BK_FAIL;
       uint32_t retry_count = 1000;
       do
       {
              if(retry_count == 0) {
                  break;
              } else
                  retry_count--;

              ret_val = mb_chnl_ctrl(log_chnl, MB_CHNL_GET_STATUS, &busy);

              if(ret_val != BK_OK)
                     return ret_val;

              if(!busy)
              {
                     ret_val = mb_chnl_write(log_chnl, cmd_buf);
                     break;
              }
       } while(1);

       return ret_val;
}

static bk_err_t usb_mailbox_slave_set_semaphore(beken_semaphore_t *semaphore)
{
	if(*semaphore) {
		return rtos_set_semaphore(semaphore);
	} else {
		return BK_FAIL;
	}
}

static bk_err_t usb_mailbox_slave_get_semaphore(beken_semaphore_t *semaphore, uint32 timeout)
{
	if(*semaphore) {
		return rtos_get_semaphore(semaphore, timeout);
	} else {
		return BK_FAIL;
	}
}

camera_packet_t *bk_usb_mailbox_video_camera_packet_malloc(void)
{
	mb_chnl_cmd_t mb_cmd;

	mb_cmd.hdr.cmd = USB_DRV_UVC_TRANSFER_BUFFER_MALLOC;

	mb_cmd.param1 = 0;
	mb_cmd.param2 = 0;
	mb_cmd.param3 = 0;

	bk_err_t ret_m = BK_OK;
	ret_m = task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);
	if(BK_OK != ret_m) {
		LOGE("%s:line=%d:ret_m=%d\r\n", __func__, __LINE__, ret_m);
		return NULL;
	}

	bk_err_t ret = kNoErr;
	bk_usb_mailbox_msg_t msg;
	camera_packet_t *packet = NULL;

	ret = rtos_pop_from_queue(&s_usb_mailbox_slave_uvc_packet_msg_que, &msg, USB_MAILBOOX_SLAVE_PACKET_WAIT_MS);
	if (kNoErr == ret) {
		switch (msg.op) {
			case USB_DRV_UVC_TRANSFER_BUFFER_MALLOC:
				packet = (camera_packet_t *)(msg.param.param1);
				break;
			default:
				LOGE("[=]%s other op:%d\r\n",__func__, msg.op);
				break;
			}
	} else {
		LOGE("[=]%s WAIT TIMEOUT \r\n",__func__);
	}

	return packet;
}

void bk_usb_mailbox_video_camera_packet_free(camera_packet_t *camera_packet)
{
	static mb_chnl_cmd_t mb_cmd;

	mb_cmd.hdr.cmd = USB_DRV_UVC_TRANSFER_BUFFER_FREE;
	mb_cmd.param1 = (uint32_t)((uint32_t *)camera_packet);
	mb_cmd.param2 = 0;
	mb_cmd.param3 = 0;
	task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);

#if 0
	bk_err_t ret = kNoErr;
	bk_usb_mailbox_msg_t msg;

	LOGI("[=]%s wait free callback\r\n", __func__);
	ret = rtos_pop_from_queue(&s_usb_mailbox_slave_packet_msg_que, &msg, BEKEN_WAIT_FOREVER);
	if (kNoErr == ret) {
		switch (msg.op) {
			case USB_DRV_UVC_TRANSFER_BUFFER_FREE:
				break;
			default:
				break;
			}
	}
#endif
}

camera_packet_t *bk_usb_mailbox_video_camera_packet_pop(void)
{
	return NULL;
}

void bk_usb_mailbox_video_camera_packet_push(camera_packet_t *camera_packet)
{
	static mb_chnl_cmd_t mb_cmd;

#if CONFIG_CACHE_ENABLE
	flush_all_dcache();
#endif

	mb_cmd.hdr.cmd = USB_DRV_UVC_TRANSFER_BUFFER_PUSH;
	mb_cmd.param1 = (uint32_t)((uint32_t *)camera_packet);
	mb_cmd.param2 = 0;
	mb_cmd.param3 = 0;

	task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);

#if 1
	bk_err_t ret = kNoErr;
	//LOGI("[=]%s wait push callback\r\n", __func__);

	ret = usb_mailbox_slave_get_semaphore(&s_usb_mailbox_slave_uvc_push_sem, USB_MAILBOOX_SLAVE_PACKET_WAIT_MS);
	if(kNoErr != ret) {
		LOGE("[=]%s WAIT TIMEOUT \r\n",__func__);
	}

/*
	bk_usb_mailbox_msg_t msg;
	ret = rtos_pop_from_queue(&s_usb_mailbox_slave_uvc_packet_msg_que, &msg, USB_MAILBOOX_SLAVE_PACKET_WAIT_MS * 2);
	if(kNoErr == ret) {
		switch (msg.op) {
			case USB_DRV_UVC_TRANSFER_BUFFER_PUSH:
				break;
			default:
				break;
			}
	}	else {
		LOGE("[=]%s WAIT TIMEOUT \r\n",__func__);
	}
*/
#endif

}

camera_packet_t *bk_usb_mailbox_video_camera_packet_dual_malloc(void)
{
    LOGD("a_malloc\n");
	mb_chnl_cmd_t mb_cmd;

	mb_cmd.hdr.cmd = USB_DRV_UVC_TRANSFER_DUAL_BUFFER_MALLOC;

	mb_cmd.param1 = 0;
	mb_cmd.param2 = 0;//uvc_id;
	mb_cmd.param3 = 0;

	bk_err_t ret_m = BK_OK;
	ret_m = task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);
	if(BK_OK != ret_m) {
		LOGE("%s:line=%d:ret_m=%d\r\n", __func__, __LINE__, ret_m);
		return NULL;
	}

	bk_err_t ret = kNoErr;
	bk_usb_mailbox_msg_t msg;
	camera_packet_t *packet = NULL;

	ret = rtos_pop_from_queue(&s_usb_mailbox_slave_uvc_packet_dual_msg_que, &msg, USB_MAILBOOX_SLAVE_PACKET_WAIT_MS);
	if (kNoErr == ret) {
		switch (msg.op) {
			case USB_DRV_UVC_TRANSFER_DUAL_BUFFER_MALLOC:
				packet = (camera_packet_t *)(msg.param.param1);
				break;
			default:
				LOGE("[=]%s other op:%d\r\n",__func__, msg.op);
				break;
			}
	} else {
		LOGE("[=]%s WAIT TIMEOUT \r\n",__func__);
	}

	return packet;
}

void bk_usb_mailbox_video_camera_packet_dual_free(camera_packet_t *camera_packet)
{
	static mb_chnl_cmd_t mb_cmd;

	mb_cmd.hdr.cmd = USB_DRV_UVC_TRANSFER_DUAL_BUFFER_FREE;
	mb_cmd.param1 = (uint32_t)((uint32_t *)camera_packet);
	mb_cmd.param2 = 0;
	mb_cmd.param3 = 0;
	task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);

#if 0
	bk_err_t ret = kNoErr;
	bk_usb_mailbox_msg_t msg;

	LOGI("[=]%s wait free callback\r\n", __func__);
	ret = rtos_pop_from_queue(&s_usb_mailbox_slave_packet_msg_que, &msg, BEKEN_WAIT_FOREVER);
	if (kNoErr == ret) {
		switch (msg.op) {
			case USB_DRV_UVC_TRANSFER_BUFFER_FREE:
				break;
			default:
				break;
			}
	}
#endif
}

camera_packet_t *bk_usb_mailbox_video_camera_packet_dual_pop(void)
{
	return NULL;
}

void bk_usb_mailbox_video_camera_packet_dual_push(camera_packet_t *camera_packet)
{
	static mb_chnl_cmd_t mb_cmd;

#if CONFIG_CACHE_ENABLE
	flush_all_dcache();
#endif

	mb_cmd.hdr.cmd = USB_DRV_UVC_TRANSFER_DUAL_BUFFER_PUSH;
	mb_cmd.param1 = (uint32_t)((uint32_t *)camera_packet);
	mb_cmd.param2 = 0;
	mb_cmd.param3 = 0;

	task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);

#if 1
	bk_err_t ret = kNoErr;
	//LOGI("[=]%s wait push callback\r\n", __func__);

	ret = usb_mailbox_slave_get_semaphore(&s_usb_mailbox_slave_uvc_push_dual_sem, USB_MAILBOOX_SLAVE_PACKET_WAIT_MS);
	if(kNoErr != ret) {
		LOGE("[=]%s WAIT TIMEOUT \r\n",__func__);
	}

/*
	bk_usb_mailbox_msg_t msg;
	ret = rtos_pop_from_queue(&s_usb_mailbox_slave_uvc_packet_dual_msg_que, &msg, USB_MAILBOOX_SLAVE_PACKET_WAIT_MS * 2);
	if(kNoErr == ret) {
		switch (msg.op) {
			case USB_DRV_UVC_TRANSFER_BUFFER_PUSH:
				break;
			default:
				break;
			}
	}	else {
		LOGE("[=]%s WAIT TIMEOUT \r\n",__func__);
	}
*/
#endif

}


const camera_packet_control_t usb_mailbox_video_camera_packet_ops = 
{
	.init   = NULL,
	.deinit = NULL,
	.malloc = bk_usb_mailbox_video_camera_packet_malloc,
	.push   = bk_usb_mailbox_video_camera_packet_push,
	.pop    = bk_usb_mailbox_video_camera_packet_pop,
	.free   = bk_usb_mailbox_video_camera_packet_free,
	.d_malloc = bk_usb_mailbox_video_camera_packet_dual_malloc,
	.d_push   = bk_usb_mailbox_video_camera_packet_dual_push,
	.d_pop    = bk_usb_mailbox_video_camera_packet_dual_pop,
	.d_free   = bk_usb_mailbox_video_camera_packet_dual_free,
};

static bk_err_t usb_mailbox_slave_packet_ops_msg(int op, void *param)
{
	bk_err_t ret;
	bk_usb_mailbox_msg_t msg;

	msg.op = op;
	if(param)
		msg.param = *(mb_chnl_cmd_t *)param;
	if (s_usb_mailbox_slave_packet_msg_que) {
		ret = rtos_push_to_queue(&s_usb_mailbox_slave_packet_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			LOGE("usb_mailbox_slave_packet_ops_msg fail \r\n");
			return BK_FAIL;
		}

		return ret;
	}
	return BK_OK;
}

static bk_err_t usb_mailbox_slave_uvc_packet_ops_msg(int op, void *param)
{
	bk_err_t ret;
	bk_usb_mailbox_msg_t msg;

	msg.op = op;
	if(param)
		msg.param = *(mb_chnl_cmd_t *)param;
	if (s_usb_mailbox_slave_uvc_packet_msg_que) {
		ret = rtos_push_to_queue(&s_usb_mailbox_slave_uvc_packet_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			LOGE("%s fail ret:%d op:%d\r\n", __func__, ret, op);
			return BK_FAIL;
		}

		return ret;
	}
	return BK_OK;
}
static bk_err_t usb_mailbox_slave_uvc_packet_ops_msg_dual(int op, void *param)
{
	bk_err_t ret;
	bk_usb_mailbox_msg_t msg;

	msg.op = op;
	if(param)
		msg.param = *(mb_chnl_cmd_t *)param;
	if (s_usb_mailbox_slave_uvc_packet_dual_msg_que) {
		ret = rtos_push_to_queue(&s_usb_mailbox_slave_uvc_packet_dual_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			LOGE("%s fail ret:%d op:%d\r\n", __func__, ret, op);
			return BK_FAIL;
		}

		return ret;
	}
	return BK_OK;
}

static audio_packet_t *bk_usb_mailbox_uac_packet_malloc(uint32_t dev)
{
	audio_packet_t *packet = NULL;
	mb_chnl_cmd_t mb_cmd;

	mb_cmd.hdr.cmd = USB_DRV_UAC_TRANSFER_BUFFER_MALLOC;
	mb_cmd.param1 = 0;
	mb_cmd.param2 = dev;
	mb_cmd.param3 = 0;
	task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);

	bk_err_t ret = kNoErr;
	bk_usb_mailbox_msg_t msg;

	ret = rtos_pop_from_queue(&s_usb_mailbox_slave_packet_msg_que, &msg, USB_MAILBOOX_SLAVE_PACKET_WAIT_MS);
	if (kNoErr == ret) {
		switch (msg.op) {
			case USB_DRV_UAC_TRANSFER_BUFFER_MALLOC:
				packet = (audio_packet_t *)(msg.param.param1);
				break;
			default:
				break;
			}
	} else {
		LOGE("[=]%s WAIT TIMEOUT\r\n",__func__);
	}

	return packet;

}

static void bk_usb_mailbox_uac_packet_push(audio_packet_t *packet)
{
	static mb_chnl_cmd_t mb_cmd;

#if CONFIG_CACHE_ENABLE
	flush_all_dcache();
#endif

	mb_cmd.hdr.cmd = USB_DRV_UAC_TRANSFER_BUFFER_PUSH;
	mb_cmd.param1 = (uint32_t)((uint32_t *)packet);
	mb_cmd.param2 = 0;
	mb_cmd.param3 = 0;
	task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);

#if 1
	bk_err_t ret = kNoErr;
	bk_usb_mailbox_msg_t msg;
	//LOGI("[=]%s wait push callback cmd:%d  param1:%d\r\n", __func__, mb_cmd.hdr.cmd, mb_cmd.param1);
	ret = rtos_pop_from_queue(&s_usb_mailbox_slave_packet_msg_que, &msg, USB_MAILBOOX_SLAVE_PACKET_WAIT_MS);
	if (kNoErr == ret) {
		switch (msg.op) {
			case USB_DRV_UAC_TRANSFER_BUFFER_PUSH:
				break;
			default:
				break;
			}
	} else {
		LOGE("[=]%s WAIT TIMEOUT\r\n",__func__);
	}
#endif

    return;
}

static void bk_usb_mailbox_uac_packet_free(audio_packet_t *packet)
{
	static mb_chnl_cmd_t mb_cmd;

	mb_cmd.hdr.cmd = USB_DRV_UAC_TRANSFER_BUFFER_FREE;
	mb_cmd.param1 = (uint32_t)((uint32_t *)packet);
	mb_cmd.param2 = 0;
	mb_cmd.param3 = 0;
	task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);


#if 1
		bk_err_t ret = kNoErr;
		bk_usb_mailbox_msg_t msg;
		//LOGI("[=]%s wait push callback cmd:%d  param1:%d\r\n", __func__, mb_cmd.hdr.cmd, mb_cmd.param1);
		ret = rtos_pop_from_queue(&s_usb_mailbox_slave_packet_msg_que, &msg, USB_MAILBOOX_SLAVE_PACKET_WAIT_MS);
		if (kNoErr == ret) {
			switch (msg.op) {
				case USB_DRV_UAC_TRANSFER_BUFFER_FREE:
					break;
				default:
					break;
				}
		} else {
		LOGE("[=]%s WAIT TIMEOUT\r\n",__func__);
	}
#endif

	return;
}


static const struct audio_packet_control_t usb_mailbox_uac_buffer_ops_funcs = {
    ._uac_malloc = bk_usb_mailbox_uac_packet_malloc,
    ._uac_push = bk_usb_mailbox_uac_packet_push,
    ._uac_pop = NULL,
    ._uac_free = bk_usb_mailbox_uac_packet_free,
};

static void usb_mailbox_disconnect_callback(void)
{
	mb_chnl_cmd_t mb_cmd;

	mb_cmd.hdr.cmd = USB_DRV_DISCONNECT_HANDLE;
	mb_cmd.param1 = 0;
	mb_cmd.param2 = 0;
	mb_cmd.param3 = 0;
	task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);

	bk_err_t ret = kNoErr;
	ret = usb_mailbox_slave_get_semaphore(&s_usb_mailbox_slave_disconnect_sem, USB_MAILBOOX_SLAVE_PACKET_WAIT_MS);
	if(kNoErr != ret) {
		LOGE("[=]%s WAIT TIMEOUT \r\n",__func__);
	}

}

static void usb_mailbox_connect_callback(void)
{
	mb_chnl_cmd_t mb_cmd;

	mb_cmd.hdr.cmd = USB_DRV_CONNECT_HANDLE;
	mb_cmd.param1 = 0;
	mb_cmd.param2 = 0;
	mb_cmd.param3 = 0;
	task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);

	bk_err_t ret = kNoErr;
	ret = usb_mailbox_slave_get_semaphore(&s_usb_mailbox_slave_connect_sem, USB_MAILBOOX_SLAVE_PACKET_WAIT_MS);
	if(kNoErr != ret) {
		LOGE("[=]%s WAIT TIMEOUT \r\n",__func__);
	}
}

bk_err_t usb_mailbox_slave_send_msg(int op, void *param)
{
	bk_err_t ret;
	bk_usb_mailbox_msg_t msg;

	msg.op = op;
	if(param)
		msg.param = *(mb_chnl_cmd_t *)param;
	if (s_usb_mailbox_slave_msg_que) {
		ret = rtos_push_to_queue(&s_usb_mailbox_slave_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			LOGE("usb_driver_send_msg fail \r\n");
			return BK_FAIL;
		}

		return ret;
	}
	return BK_OK;
}

void usb_mailbox_slave_usb_driver_exit(void)
{
	usb_mailbox_slave_send_msg(USB_DRV_EXIT, NULL);
}

static void _mailbox_rx_isr(void *param, mb_chnl_cmd_t *cmd_buf)
{
#if CONFIG_CACHE_ENABLE
	flush_all_dcache();
#endif

	static mb_chnl_cmd_t mb_cmd;

	memcpy(&mb_cmd, cmd_buf, sizeof(mb_chnl_cmd_t));

	switch (cmd_buf->hdr.cmd) {
		case USB_DRV_USB_INIT:
			bk_usb_driver_init();
			break;
		case USB_DRV_USB_DEINIT:
			bk_usb_driver_deinit();
			break;
		case USB_DRV_USB_CLOSE:
			bk_usb_close();
			break;
		case USB_DRV_SET_DEV_USE:
			bk_usb_device_set_using_status(mb_cmd.param1, mb_cmd.param2);
			break;
		case USB_DRV_DISCONNECT_HANDLE:
			usb_mailbox_slave_set_semaphore(&s_usb_mailbox_slave_disconnect_sem);
			break;
		case USB_DRV_CONNECT_HANDLE:
			usb_mailbox_slave_set_semaphore(&s_usb_mailbox_slave_connect_sem);
			break;
		case USB_DRV_UVC_REGISTER_CONNECT_CB:
			break;
		case USB_DRV_UVC_REGISTER_DISCONNECT_CB:
			break;
		case USB_DRV_UVC_SET_PARAM:
			bk_usb_uvc_set_param((bk_uvc_config_t *)(mb_cmd.param1));
			break;
		case USB_DRV_UVC_REGISTER_TRANSFER_OPS:
			break;
		case USB_DRV_VIDEO_START:
			break;
		case USB_DRV_VIDEO_STOP:
			break;
		case USB_DRV_UAC_REGISTER_TRANSFER_OPS:
			break;
		case USB_DRV_UAC_SET_PARAM:
			break;
		case USB_DRV_AUDIO_MIC_START:
			break;
		case USB_DRV_AUDIO_MIC_STOP:
			break;
		case USB_DRV_AUDIO_SPK_START:
			break;
		case USB_DRV_AUDIO_SPK_STOP:
			break;
		default:
			break;
	}

	usb_mailbox_slave_send_msg(cmd_buf->hdr.cmd, (void *)&mb_cmd);
}

static void _mailbox_tx_isr(void *param)
{
    LOGD("%s\n", __func__);
}

static void _mailbox_tx_cmpl_isr(void *param, mb_chnl_ack_t *ack_buf)
{
	switch (ack_buf->hdr.cmd) {
		case USB_DRV_UVC_TRANSFER_BUFFER_MALLOC:
			usb_mailbox_slave_uvc_packet_ops_msg(ack_buf->hdr.cmd, ack_buf);
			break;
		case USB_DRV_UVC_TRANSFER_BUFFER_PUSH:
			//usb_mailbox_slave_uvc_packet_ops_msg(ack_buf->hdr.cmd, ack_buf);
			usb_mailbox_slave_set_semaphore(&s_usb_mailbox_slave_uvc_push_sem);
			break;
		case USB_DRV_UVC_TRANSFER_DUAL_BUFFER_MALLOC:
			usb_mailbox_slave_uvc_packet_ops_msg_dual(ack_buf->hdr.cmd, ack_buf);
			break;
		case USB_DRV_UVC_TRANSFER_DUAL_BUFFER_PUSH:
			//usb_mailbox_slave_uvc_packet_ops_msg(ack_buf->hdr.cmd, ack_buf);
			usb_mailbox_slave_set_semaphore(&s_usb_mailbox_slave_uvc_push_dual_sem);
			break;

		case USB_DRV_UAC_TRANSFER_BUFFER_MALLOC:
			usb_mailbox_slave_packet_ops_msg(ack_buf->hdr.cmd, ack_buf);
			break;
		case USB_DRV_UAC_TRANSFER_BUFFER_PUSH:
			usb_mailbox_slave_packet_ops_msg(ack_buf->hdr.cmd, ack_buf);
			break;
		case USB_DRV_UAC_TRANSFER_BUFFER_FREE:
			usb_mailbox_slave_packet_ops_msg(ack_buf->hdr.cmd, ack_buf);
			break;

		default:
			break;
	}

}

static void usb_mailbox_slave_task(beken_thread_arg_t param_data)
{
	bk_err_t ret = kNoErr;
    mb_chnl_cmd_t *mb_cmd;
	mb_chnl_cmd_t callback_mb_cmd;
	while(1) {
		bk_usb_mailbox_msg_t msg;
		ret = rtos_pop_from_queue(&s_usb_mailbox_slave_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case USB_DRV_EXIT:
					LOGI("USB_MB_SLAVE_USB_DRV_EXIT!\r\n");
					callback_mb_cmd.hdr.cmd = USB_DRV_EXIT;
					task_mb_chnl_write(USB_MAILBOX_CHNL, &callback_mb_cmd);
					rtos_reset_queue(&s_usb_mailbox_slave_msg_que);
					//goto usb_mailbox_slave_task_exit;
					break;
				case USB_DRV_USB_INIT:
					break;
				case USB_DRV_USB_DEINIT:
					goto usb_mailbox_slave_task_exit;
					break;
				case USB_POWER_OPS:
					mb_cmd = (mb_chnl_cmd_t *)(&msg.param);
					bk_usb_power_ops(mb_cmd->param1, mb_cmd->param2);
					callback_mb_cmd.hdr.cmd = USB_POWER_OPS;
					task_mb_chnl_write(USB_MAILBOX_CHNL, &callback_mb_cmd);
					break;
				case USB_DRV_USB_OPEN:
					//mb_cmd = (mb_chnl_cmd_t *)(&msg.param);
					bk_usb_uvc_register_connect_callback(usb_mailbox_connect_callback);
					bk_usb_uvc_register_disconnect_callback(usb_mailbox_disconnect_callback);
					bk_usb_open(0);
					callback_mb_cmd.hdr.cmd = USB_DRV_USB_OPEN;
					task_mb_chnl_write(USB_MAILBOX_CHNL, &callback_mb_cmd);
					break;
				case USB_DRV_USB_CLOSE:
					break;
				case USB_DRV_SET_DEV_USE:
					break;
				case USB_DRV_GET_DEV_CONNECT_STATUS:
					mb_cmd = (mb_chnl_cmd_t *)(&msg.param);
					mb_cmd->hdr.cmd = USB_DRV_GET_DEV_CONNECT_STATUS;
					mb_cmd->param1 = bk_usb_get_device_connect_status();
					task_mb_chnl_write(USB_MAILBOX_CHNL, mb_cmd);
					break;
				case USB_DRV_CHECK_DEV_SUPPORT:
					mb_cmd = (mb_chnl_cmd_t *)(&msg.param);
					mb_cmd->hdr.cmd = USB_DRV_CHECK_DEV_SUPPORT;
					mb_cmd->param2 = mb_cmd->param1;
					mb_cmd->param1 = bk_usb_check_device_supported(mb_cmd->param2);
					task_mb_chnl_write(USB_MAILBOX_CHNL, mb_cmd);
					break;
				case USB_DRV_UVC_REGISTER_CONNECT_CB:
					break;
				case USB_DRV_UVC_REGISTER_DISCONNECT_CB:
					break;
				case USB_DRV_UVC_GET_PARAM:
					mb_cmd = (mb_chnl_cmd_t *)(&msg.param);
					mb_cmd->hdr.cmd = USB_DRV_UVC_GET_PARAM;
					mb_cmd->param2 = bk_usb_uvc_get_param((bk_uvc_device_brief_info_t *)mb_cmd->param1);
					task_mb_chnl_write(USB_MAILBOX_CHNL, mb_cmd);
					break;
				case USB_DRV_UVC_SET_PARAM:
					break;
				case USB_DRV_UVC_REGISTER_TRANSFER_OPS:
					bk_usb_uvc_register_transfer_buffer_ops((void *)&usb_mailbox_video_camera_packet_ops);
					break;

				case USB_DRV_UVC_TRANSFER_BUFFER_MALLOC:
					break;
				case USB_DRV_UVC_TRANSFER_BUFFER_PUSH:
					break;
				case USB_DRV_UVC_TRANSFER_BUFFER_POP:
					break;
				case USB_DRV_UVC_TRANSFER_BUFFER_FREE:
					break;
				case USB_DRV_UVC_TRANSFER_DUAL_BUFFER_MALLOC:
					break;
				case USB_DRV_UVC_TRANSFER_DUAL_BUFFER_PUSH:
					break;
				case USB_DRV_UVC_TRANSFER_DUAL_BUFFER_POP:
					break;
				case USB_DRV_UVC_TRANSFER_DUAL_BUFFER_FREE:
					break;
                case USB_DRV_VIDEO_START:
                    bk_uvc_start();
					break;
				case USB_DRV_VIDEO_DUAL_START:
                    bk_uvc_dual_start();
					break;
				case USB_DRV_VIDEO_DUAL_STOP:
				case USB_DRV_VIDEO_STOP:
					bk_uvc_stop();
					break;

				case USB_DRV_UAC_REGISTER_CONNECT_CB:
					break;
				case USB_DRV_UAC_REGISTER_DISCONNECT_CB:
					break;
				case USB_DRV_UAC_GET_PARAM:
					mb_cmd = (mb_chnl_cmd_t *)(&msg.param);
					mb_cmd->hdr.cmd = USB_DRV_UAC_GET_PARAM;
					mb_cmd->param2 = bk_usb_uac_get_param((bk_uac_device_brief_info_t *)mb_cmd->param1);
					task_mb_chnl_write(USB_MAILBOX_CHNL, mb_cmd);
					break;
				case USB_DRV_UAC_SET_PARAM:
					mb_cmd = (mb_chnl_cmd_t *)(&msg.param);
					bk_usb_uac_set_param((bk_uac_config_t *)(mb_cmd->param1));
					mb_cmd->hdr.cmd = USB_DRV_UAC_SET_PARAM;
					task_mb_chnl_write(USB_MAILBOX_CHNL, mb_cmd);
					break;
				case USB_DRV_UAC_REGISTER_TRANSFER_OPS:
					bk_usb_uac_register_transfer_buffer_ops((void *)&usb_mailbox_uac_buffer_ops_funcs);
					break;

				case USB_DRV_UAC_TRANSFER_BUFFER_MALLOC:
					break;
				case USB_DRV_UAC_TRANSFER_BUFFER_PUSH:
					break;
				case USB_DRV_UAC_TRANSFER_BUFFER_POP:
					break;
				case USB_DRV_UAC_TRANSFER_BUFFER_FREE:
					break;

				case USB_DRV_AUDIO_MIC_START:
					bk_uac_start_mic();
					break;
				case USB_DRV_AUDIO_MIC_STOP:
					bk_uac_stop_mic();
					break;
				case USB_DRV_AUDIO_SPK_START:
					bk_uac_start_speaker();
					break;
				case USB_DRV_AUDIO_SPK_STOP:
					bk_uac_stop_speaker();
					break;


				default:
					break;
			}
		}
	}

usb_mailbox_slave_task_exit:

	bk_usb_mailbox_sw_slave_deinit();

}

void bk_usb_mailbox_sw_slave_init(void)
{
	uint32_t ret = 0;

	if ((!s_usb_mailbox_slave_thread_hdl) && (!s_usb_mailbox_slave_msg_que)) {

		do{
			if(!s_usb_mailbox_slave_packet_msg_que) {
				ret = rtos_init_queue(&s_usb_mailbox_slave_packet_msg_que,
									  "usb_mb_slave_packet_q",
									  sizeof(bk_usb_mailbox_msg_t),
									  USB_MAILBOX_SLAVE_PACKET_QITEM_COUNT);
				if(ret != kNoErr) {
					break;
				}
			}

			if(!s_usb_mailbox_slave_uvc_packet_msg_que) {
				ret = rtos_init_queue(&s_usb_mailbox_slave_uvc_packet_msg_que,
									  "usb_mb_slave_uvc_packet_q",
									  sizeof(bk_usb_mailbox_msg_t),
									  USB_MAILBOX_SLAVE_UVC_PACKET_QITEM_COUNT);
				if(ret != kNoErr) {
					break;
				}
			}
			if(!s_usb_mailbox_slave_uvc_packet_dual_msg_que) {
				ret = rtos_init_queue(&s_usb_mailbox_slave_uvc_packet_dual_msg_que,
									  "usb_mb_slave_uvc_packet_q_dual",
									  sizeof(bk_usb_mailbox_msg_t),
									  USB_MAILBOX_SLAVE_UVC_PACKET_DUAL_QITEM_COUNT);
				if(ret != kNoErr) {
					break;
				}
			}
			if(!s_usb_mailbox_slave_uvc_push_sem) {
				ret = rtos_init_semaphore(&s_usb_mailbox_slave_uvc_push_sem, 1);
				if(ret != kNoErr) {
					break;
				}
			}
			if(!s_usb_mailbox_slave_uvc_push_dual_sem) {
				ret = rtos_init_semaphore(&s_usb_mailbox_slave_uvc_push_dual_sem, 1);
				if(ret != kNoErr) {
					break;
				}
			}
			if(!s_usb_mailbox_slave_disconnect_sem) {
				ret = rtos_init_semaphore(&s_usb_mailbox_slave_disconnect_sem, 1);
				if(ret != kNoErr) {
					break;
				}
			}

			if(!s_usb_mailbox_slave_connect_sem) {
				ret = rtos_init_semaphore(&s_usb_mailbox_slave_connect_sem, 1);
				if(ret != kNoErr) {
					break;
				}
			}

			ret = rtos_init_queue(&s_usb_mailbox_slave_msg_que,
								  "usb_mb_slave_queue",
								  sizeof(bk_usb_mailbox_msg_t),
								  USB_MAILBOX_SLAVE_QITEM_COUNT);

			if(ret != kNoErr) {
				break;
			}

			//create usb driver task
			ret = rtos_create_thread(&s_usb_mailbox_slave_thread_hdl,
								 USB_MAILBOX_SLAVE_TASK_PRIO,
								 "usb_mb_slave",
								 (beken_thread_function_t)usb_mailbox_slave_task,
								 USB_MAILBOX_SLAVE_TASK_SIZE,
								 NULL);
			if(ret != kNoErr) {
				break;
			}
		}while(0);
		if (ret != kNoErr) {
			LOGE("create usb driver task fail \r\n");
			if(s_usb_mailbox_slave_packet_msg_que) {
				rtos_deinit_queue(&s_usb_mailbox_slave_packet_msg_que);
				s_usb_mailbox_slave_packet_msg_que = NULL;
			}

			if(s_usb_mailbox_slave_uvc_packet_msg_que) {
				rtos_deinit_queue(&s_usb_mailbox_slave_uvc_packet_msg_que);
				s_usb_mailbox_slave_uvc_packet_msg_que = NULL;
			}
			if(s_usb_mailbox_slave_uvc_packet_dual_msg_que) {
				rtos_deinit_queue(&s_usb_mailbox_slave_uvc_packet_dual_msg_que);
				s_usb_mailbox_slave_uvc_packet_dual_msg_que = NULL;
			}
			if(s_usb_mailbox_slave_uvc_push_sem) {
				rtos_deinit_semaphore(&s_usb_mailbox_slave_uvc_push_sem);
				s_usb_mailbox_slave_uvc_push_sem = NULL;
			}			
			if(s_usb_mailbox_slave_uvc_push_dual_sem) {
				rtos_deinit_semaphore(&s_usb_mailbox_slave_uvc_push_dual_sem);
				s_usb_mailbox_slave_uvc_push_dual_sem = NULL;
			}

			if(s_usb_mailbox_slave_disconnect_sem) {
				rtos_deinit_semaphore(&s_usb_mailbox_slave_disconnect_sem);
				s_usb_mailbox_slave_disconnect_sem = NULL;
			}

			if(s_usb_mailbox_slave_connect_sem) {
				rtos_deinit_semaphore(&s_usb_mailbox_slave_connect_sem);
				s_usb_mailbox_slave_connect_sem = NULL;
			}

			if(s_usb_mailbox_slave_msg_que) {
				rtos_deinit_queue(&s_usb_mailbox_slave_msg_que);
				s_usb_mailbox_slave_msg_que = NULL;
			}

			if (s_usb_mailbox_slave_thread_hdl) {
				rtos_delete_thread(&s_usb_mailbox_slave_thread_hdl);
				s_usb_mailbox_slave_thread_hdl = NULL;
			}
			return;
		}

		if(mb_chnl_open(USB_MAILBOX_CHNL, NULL) != BK_OK) {
			LOGE("usb mailbox slave mb_chnl_open open fail \r\n");
			return;
		}

		mb_chnl_ctrl(USB_MAILBOX_CHNL, MB_CHNL_SET_RX_ISR, _mailbox_rx_isr);
		mb_chnl_ctrl(USB_MAILBOX_CHNL, MB_CHNL_SET_TX_ISR, _mailbox_tx_isr);
		mb_chnl_ctrl(USB_MAILBOX_CHNL, MB_CHNL_SET_TX_CMPL_ISR, _mailbox_tx_cmpl_isr);
		LOGD("create usb mailbox master task complete \r\n");

#if CONFIG_USB_MAILBOX_SLAVE_CPU2_TO_CPU1
		mb_chnl_cmd_t callback_mb_cmd;
		callback_mb_cmd.hdr.cmd = USB_POWER_OPS;
    // Modified by TUYA Start
#if CONFIG_TUYA_LOGIC_MODIFY
        uint8_t usb_ldo, active_level;
        tkl_vi_get_power_info(UVC_CAMERA, &usb_ldo, &active_level);
		bk_usb_power_ops(usb_ldo, 1);
#else
		bk_usb_power_ops(CONFIG_USB_VBAT_CONTROL_GPIO_ID, 1);
#endif // CONFIG_TUYA_LOGIC_MODIFY
    // Modified by TUYA End
		task_mb_chnl_write(USB_MAILBOX_CHNL, &callback_mb_cmd);
#endif

	} else {
		return;
	}
}

void bk_usb_mailbox_sw_slave_deinit(void)
{
	beken_queue_t usb_mailbox_slave_msg_que_deinit = s_usb_mailbox_slave_packet_msg_que;
	beken_queue_t usb_mailbox_slave_packet_msg_que_deinit = s_usb_mailbox_slave_msg_que;
	beken_queue_t usb_mailbox_slave_uvc_packet_msg_que_deinit = s_usb_mailbox_slave_uvc_packet_msg_que;
	beken_queue_t usb_mailbox_slave_uvc_packet_msg_que_dual_deinit = s_usb_mailbox_slave_uvc_packet_dual_msg_que;

	beken_semaphore_t usb_mailbox_slave_uvc_push_sem = s_usb_mailbox_slave_uvc_push_sem;
	beken_semaphore_t usb_mailbox_slave_uvc_push_sem_dual = s_usb_mailbox_slave_uvc_push_dual_sem;
	beken_semaphore_t usb_mailbox_slave_disconnect_sem = s_usb_mailbox_slave_disconnect_sem;
	beken_semaphore_t usb_mailbox_slave_connect_sem = s_usb_mailbox_slave_connect_sem;

	if(s_usb_mailbox_slave_packet_msg_que) {
		s_usb_mailbox_slave_packet_msg_que = NULL;
		rtos_deinit_queue(&usb_mailbox_slave_msg_que_deinit);
	}

	if(s_usb_mailbox_slave_uvc_packet_msg_que) {
		s_usb_mailbox_slave_uvc_packet_msg_que = NULL;
		rtos_deinit_queue(&usb_mailbox_slave_uvc_packet_msg_que_deinit);
	}
	if(s_usb_mailbox_slave_uvc_packet_msg_que) {
		s_usb_mailbox_slave_uvc_packet_dual_msg_que = NULL;
		rtos_deinit_queue(&usb_mailbox_slave_uvc_packet_msg_que_dual_deinit);
	}

	if(s_usb_mailbox_slave_uvc_push_sem) {
		s_usb_mailbox_slave_uvc_push_sem = NULL;
		rtos_deinit_semaphore(&usb_mailbox_slave_uvc_push_sem);
	}	
    if(s_usb_mailbox_slave_uvc_push_dual_sem) {
		s_usb_mailbox_slave_uvc_push_dual_sem = NULL;
		rtos_deinit_semaphore(&usb_mailbox_slave_uvc_push_sem_dual);
	}

	if(s_usb_mailbox_slave_disconnect_sem) {
		s_usb_mailbox_slave_disconnect_sem = NULL;
		rtos_deinit_semaphore(&usb_mailbox_slave_disconnect_sem);
	}

	if(s_usb_mailbox_slave_connect_sem) {
		s_usb_mailbox_slave_connect_sem = NULL;
		rtos_deinit_semaphore(&usb_mailbox_slave_connect_sem);
	}

	if(s_usb_mailbox_slave_msg_que) {
		s_usb_mailbox_slave_msg_que = NULL;
		rtos_deinit_queue(&usb_mailbox_slave_packet_msg_que_deinit);
	}

	if (s_usb_mailbox_slave_thread_hdl) {
		rtos_delete_thread(&s_usb_mailbox_slave_thread_hdl);
		s_usb_mailbox_slave_thread_hdl = NULL;
	}

}

#endif

#endif //CONFIG_MAILBOX

