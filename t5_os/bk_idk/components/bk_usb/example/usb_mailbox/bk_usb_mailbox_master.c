#include "bk_usb_mailbox.h"

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
#if CONFIG_USB_MAILBOX_MASTER
#if CONFIG_USB_MAILBOX_MASTER_CPU1_TO_CPU0
#define USB_MAILBOX_CHNL MB_CHNL_USB
#elif CONFIG_USB_MAILBOX_MASTER_CPU1_TO_CPU2
#define USB_MAILBOX_CHNL CP2_MB_CHNL_USB
#else
#define USB_MAILBOX_CHNL MB_CHNL_USB
#endif


static beken_thread_t  s_usb_mailbox_master_thread_hdl = NULL;
static beken_queue_t s_usb_mailbox_master_msg_que = NULL;
static beken_queue_t s_usb_mailbox_master_rx_msg_que = NULL;
static beken_semaphore_t s_usb_mailbox_master_sem = NULL;
static beken_semaphore_t s_usb_mailbox_master_usb_driver_exit_sem = NULL;
static camera_packet_control_t *usb_mailbox_video_packet_ops = NULL;
static struct audio_packet_control_t *usb_mailbox_audio_packet_ops = NULL;

static bool s_usb_power_on_flag = 0;
static bool s_usb_open_flag = 0;
static uint32_t s_usb_dev_using_status = 0;
static uint32_t s_usb_dev_connect_status = 0;

static FUNCPTR s_usbh_mailbox_connect_cb[USB_DEVICE_MAX];
static FUNCPTR s_usbh_mailbox_disconnect_cb[USB_DEVICE_MAX];
static bk_uac_config_t s_uac_config;

#define USB_MAILBOX_MASTER_QITEM_COUNT 10
#define USB_MAILBOX_MASTER_RX_QITEM_COUNT 1
#define USB_MAILBOX_MASTER_TASK_PRIO 3
#define USB_MAILBOX_MASTER_TASK_SIZE 2048
#define USB_MAILBOX_MASTER_SEND_WAIT_MS 2
#define USB_RETURN_NOT_POWERED_ON() do {\
		if(!s_usb_power_on_flag) {\
			return BK_ERR_USB_NOT_POWER;\
		}\
	} while(0)


#define USB_RETURN_NOT_POWERED_DOWN() do {\
		if(s_usb_power_on_flag) {\
			return BK_ERR_USB_NOT_POWER;\
		}\
	} while(0)

#define USB_RETURN_NOT_OPEN() do {\
		if(!s_usb_open_flag) {\
			return BK_ERR_USB_NOT_OPEN;\
		}\
	} while(0)

#define USB_RETURN_NOT_OPEN_NULL() do {\
				if(!s_usb_open_flag) {\
					return;\
				}\
			} while(0)

#define USB_RETURN_NOT_CLOSE() do {\
		if(s_usb_open_flag) {\
			return BK_ERR_USB_NOT_CLOSE;\
		}\
	} while(0)

bk_err_t task_mb_chnl_write(uint8_t log_chnl, mb_chnl_cmd_t *cmd_buf)
{
       uint8_t   busy = 1;
       bk_err_t  ret_val = BK_FAIL;
       uint32_t retry_count = 1000;
       do
       {
              if(retry_count == 0) {
			  	  ret_val = BK_FAIL;
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

static bk_err_t usb_mailbox_master_send_msg(int op, void *param)
{
	bk_err_t ret;
	bk_usb_mailbox_msg_t msg;

	msg.op = op;
	if(param)
		msg.param = *(mb_chnl_cmd_t *)param;
	if (s_usb_mailbox_master_msg_que) {
		ret = rtos_push_to_queue(&s_usb_mailbox_master_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			LOGE("usb_driver_send_msg fail \r\n");
			return BK_FAIL;
		}
	}

	return ret;

}

static bk_err_t usb_mailbox_master_receive_msg(int op, void *param)
{
	bk_err_t ret;
	bk_usb_mailbox_msg_t msg;

	msg.op = op;
	if(param)
		msg.param = *(mb_chnl_cmd_t *)param;
	if (s_usb_mailbox_master_rx_msg_que) {
		ret = rtos_push_to_queue(&s_usb_mailbox_master_rx_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			LOGE("usb_driver_receive_msg fail \r\n");
			return BK_FAIL;
		}

		return ret;
	}
	return BK_OK;
}

static void usbh_mailbox_connect_device()
{
	s_usb_dev_connect_status |= (0x1 << USB_USER_CHECK_DEVICE);

	for(E_USB_DEVICE_T dev = USB_UVC_DEVICE; dev < USB_DEVICE_MAX; dev++)
	{
		if(s_usbh_mailbox_connect_cb[dev]) {
			if(s_usb_dev_connect_status & (0x1 << dev)) {
				LOGE("dev:%d IS CONNECT \r\n", dev);
			}else {
				s_usbh_mailbox_connect_cb[dev]();
				s_usb_dev_connect_status |= (0x1 << dev);
			}
		}
	}

}

static void usbh_mailbox_disconnect_device()
{
	s_usb_dev_connect_status = 0;

	for(E_USB_DEVICE_T dev = USB_UVC_DEVICE; dev < USB_DEVICE_MAX; dev++)
	{
		if(s_usbh_mailbox_disconnect_cb[dev]) {
			s_usbh_mailbox_disconnect_cb[dev]();
		}
	}
}

static bk_err_t usb_mailbox_master_set_semaphore(beken_semaphore_t *semaphore)
{
	if(*semaphore) {
		return rtos_set_semaphore(semaphore);
	} else {
		return BK_FAIL;
	}
}

static bk_err_t usb_mailbox_master_get_semaphore(beken_semaphore_t *semaphore, uint32 timeout)
{
	if(*semaphore) {
		return rtos_get_semaphore(semaphore, timeout);
	} else {
		return BK_FAIL;
	}
}

static void _mailbox_rx_isr(void *param, mb_chnl_cmd_t *cmd_buf)
{
#if CONFIG_CACHE_ENABLE
	flush_all_dcache();
#endif
	static mb_chnl_cmd_t mb_cmd;

	camera_packet_t *packet = NULL;
	mb_cmd.hdr.cmd = cmd_buf->hdr.cmd;
	audio_packet_t *uac_packet = NULL;
	mb_chnl_ack_t *ack_buf = (mb_chnl_ack_t *)cmd_buf;

	switch (cmd_buf->hdr.cmd) {
		case USB_DRV_UVC_TRANSFER_BUFFER_MALLOC:
			if(usb_mailbox_video_packet_ops) {
				packet = usb_mailbox_video_packet_ops->malloc();
				ack_buf->hdr.cmd = USB_DRV_UVC_TRANSFER_BUFFER_MALLOC;
				ack_buf->ack_data1 = (uint32_t)(uint32_t *)packet;
			}
			break;
		case USB_DRV_UVC_TRANSFER_BUFFER_PUSH:
			if(usb_mailbox_video_packet_ops->push) {
				packet = (camera_packet_t *)cmd_buf->param1;
				usb_mailbox_video_packet_ops->push(packet);
				ack_buf->hdr.cmd = USB_DRV_UVC_TRANSFER_BUFFER_PUSH;
			}
			break;
		case USB_DRV_UVC_TRANSFER_BUFFER_POP:
			break;
		case USB_DRV_UVC_TRANSFER_BUFFER_FREE:
			if(usb_mailbox_video_packet_ops->free) {
				packet = (camera_packet_t *)cmd_buf->param1;
				usb_mailbox_video_packet_ops->free(packet);
			}
			break;
		case USB_DRV_UVC_TRANSFER_DUAL_BUFFER_MALLOC:
			if(usb_mailbox_video_packet_ops) {
				packet = usb_mailbox_video_packet_ops->d_malloc();
				ack_buf->hdr.cmd = USB_DRV_UVC_TRANSFER_DUAL_BUFFER_MALLOC;
				ack_buf->ack_data1 = (uint32_t)(uint32_t *)packet;
			}
			break;
		case USB_DRV_UVC_TRANSFER_DUAL_BUFFER_PUSH:
			if(usb_mailbox_video_packet_ops->d_push) {
				packet = (camera_packet_t *)cmd_buf->param1;
				usb_mailbox_video_packet_ops->d_push(packet);
				ack_buf->hdr.cmd = USB_DRV_UVC_TRANSFER_DUAL_BUFFER_PUSH;
			}
			break;
		case USB_DRV_UVC_TRANSFER_DUAL_BUFFER_POP:
			break;
		case USB_DRV_UVC_TRANSFER_DUAL_BUFFER_FREE:
			if(usb_mailbox_video_packet_ops->d_free) {
				packet = (camera_packet_t *)cmd_buf->param1;
				usb_mailbox_video_packet_ops->d_free(packet);
			}
			break;
		case USB_DRV_UAC_TRANSFER_BUFFER_MALLOC:
			if(usb_mailbox_audio_packet_ops) {
				uac_packet = usb_mailbox_audio_packet_ops->_uac_malloc(cmd_buf->param2);
				ack_buf->hdr.cmd = USB_DRV_UAC_TRANSFER_BUFFER_MALLOC;
				ack_buf->ack_data1 = (uint32_t)(uint32_t *)uac_packet;
			}
			break;
		case USB_DRV_UAC_TRANSFER_BUFFER_PUSH:
			if(usb_mailbox_audio_packet_ops->_uac_push) {
				ack_buf->hdr.cmd = USB_DRV_UAC_TRANSFER_BUFFER_PUSH;
				uac_packet = (audio_packet_t *)cmd_buf->param1;
				usb_mailbox_audio_packet_ops->_uac_push(uac_packet);
			}
			break;
		case USB_DRV_UAC_TRANSFER_BUFFER_POP:
			break;
		case USB_DRV_UAC_TRANSFER_BUFFER_FREE:
			if(usb_mailbox_audio_packet_ops->_uac_free) {
				ack_buf->hdr.cmd = USB_DRV_UAC_TRANSFER_BUFFER_FREE;
				uac_packet = (audio_packet_t *)cmd_buf->param1;
				usb_mailbox_audio_packet_ops->_uac_free(uac_packet);
			}
			break;
		case USB_DRV_GET_DEV_CONNECT_STATUS:
			mb_cmd.hdr.cmd = USB_DRV_GET_DEV_CONNECT_STATUS;
			mb_cmd.param1 = cmd_buf->param1;
			usb_mailbox_master_send_msg(USB_DRV_GET_DEV_CONNECT_STATUS, (void *)&mb_cmd);
			break;
		case USB_DRV_CONNECT_HANDLE:
			//ack_buf->hdr.cmd = USB_DRV_CONNECT_HANDLE;
			//usbh_mailbox_connect_device();
			mb_cmd.hdr.cmd = USB_DRV_CONNECT_HANDLE;
			usb_mailbox_master_send_msg(USB_DRV_CONNECT_HANDLE, (void *)&mb_cmd);
			break;
		case USB_DRV_DISCONNECT_HANDLE:
			//ack_buf->hdr.cmd = USB_DRV_DISCONNECT_HANDLE;
			//usbh_mailbox_disconnect_device();
			mb_cmd.hdr.cmd = USB_DRV_DISCONNECT_HANDLE;
			usb_mailbox_master_send_msg(USB_DRV_DISCONNECT_HANDLE, (void *)&mb_cmd);
			break;
		case USB_DRV_EXIT:
			mb_cmd.hdr.cmd = USB_DRV_EXIT;
			usb_mailbox_master_send_msg(USB_DRV_EXIT, (void *)&mb_cmd);
			break;
		case USB_DRV_USB_INIT:
		case USB_DRV_USB_DEINIT:
		case USB_POWER_OPS:
		case USB_DRV_USB_OPEN:
		case USB_DRV_USB_CLOSE:
		case USB_DRV_SET_DEV_USE:
		case USB_DRV_UVC_REGISTER_CONNECT_CB:
		case USB_DRV_UVC_REGISTER_DISCONNECT_CB:
			usb_mailbox_master_set_semaphore(&s_usb_mailbox_master_sem);
			break;
		default:
			mb_cmd.param1 = cmd_buf->param1;
			mb_cmd.param2 = cmd_buf->param2;
			mb_cmd.param3 = cmd_buf->param3;
			usb_mailbox_master_receive_msg(mb_cmd.hdr.cmd, (void *)&mb_cmd);
			break;
	}


}

static void _mailbox_tx_isr(void *param)
{
}

static void _mailbox_tx_cmpl_isr(void *param, mb_chnl_ack_t *ack_buf)
{
	//usb_mailbox_master_set_semaphore(&s_usb_mailbox_master_sem);
}

static void usb_mailbox_master_task(beken_thread_arg_t param_data)
{
	bk_err_t ret = kNoErr;

    mb_chnl_cmd_t mb_cmd;

	while(1) {
		bk_usb_mailbox_msg_t msg;
		ret = rtos_pop_from_queue(&s_usb_mailbox_master_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case USB_DRV_EXIT:
					LOGI("USB_MB_MASTER_USB_DRV_EXIT!\r\n");
					usb_mailbox_master_set_semaphore(&s_usb_mailbox_master_usb_driver_exit_sem);
					break;
				case USB_DRV_USB_INIT:
					mb_cmd.hdr.cmd = USB_DRV_USB_INIT;
					task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);
					break;
				case USB_DRV_USB_DEINIT:
					mb_cmd.hdr.cmd = USB_DRV_USB_DEINIT;
					task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);
					break;
				case USB_POWER_OPS:
					LOGI("USB_MB_MASTER_USB_POWER_OPS!\r\n");
					task_mb_chnl_write(USB_MAILBOX_CHNL, (mb_chnl_cmd_t *)&msg.param);
					break;
				case USB_DRV_USB_OPEN:
					task_mb_chnl_write(USB_MAILBOX_CHNL, (mb_chnl_cmd_t *)&msg.param);
					break;
				case USB_DRV_USB_CLOSE:
					task_mb_chnl_write(USB_MAILBOX_CHNL, (mb_chnl_cmd_t *)&msg.param);
					break;
				case USB_DRV_SET_DEV_USE:
					task_mb_chnl_write(USB_MAILBOX_CHNL, (mb_chnl_cmd_t *)&msg.param);
					usb_mailbox_master_get_semaphore(&s_usb_mailbox_master_sem, USB_MAILBOX_MASTER_SEND_WAIT_MS);
					break;
				case USB_DRV_CONNECT_HANDLE:
					usbh_mailbox_connect_device();
					mb_cmd.hdr.cmd = USB_DRV_CONNECT_HANDLE;
					task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);
					break;
				case USB_DRV_DISCONNECT_HANDLE:
					usbh_mailbox_disconnect_device();
					mb_cmd.hdr.cmd = USB_DRV_DISCONNECT_HANDLE;
					task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);
					break;
				case USB_DRV_GET_DEV_CONNECT_STATUS:
					mb_cmd.hdr.cmd = USB_DRV_GET_DEV_CONNECT_STATUS;
					task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);
					usb_mailbox_master_get_semaphore(&s_usb_mailbox_master_sem, USB_MAILBOX_MASTER_SEND_WAIT_MS);
					break;
				case USB_DRV_CHECK_DEV_SUPPORT:
					task_mb_chnl_write(USB_MAILBOX_CHNL, (mb_chnl_cmd_t *)&msg.param);
					usb_mailbox_master_get_semaphore(&s_usb_mailbox_master_sem, USB_MAILBOX_MASTER_SEND_WAIT_MS);
					break;
				case USB_DRV_UVC_REGISTER_CONNECT_CB:
					break;
				case USB_DRV_UVC_REGISTER_DISCONNECT_CB:
					break;
				case USB_DRV_UVC_GET_PARAM:
					task_mb_chnl_write(USB_MAILBOX_CHNL, (mb_chnl_cmd_t *)&msg.param);
					usb_mailbox_master_get_semaphore(&s_usb_mailbox_master_sem, USB_MAILBOX_MASTER_SEND_WAIT_MS);
					break;
				case USB_DRV_UVC_SET_PARAM:
					task_mb_chnl_write(USB_MAILBOX_CHNL, (mb_chnl_cmd_t *)&msg.param);
					usb_mailbox_master_get_semaphore(&s_usb_mailbox_master_sem, USB_MAILBOX_MASTER_SEND_WAIT_MS);
					break;
				case USB_DRV_UVC_REGISTER_TRANSFER_OPS:
					task_mb_chnl_write(USB_MAILBOX_CHNL, (mb_chnl_cmd_t *)&msg.param);
					usb_mailbox_master_get_semaphore(&s_usb_mailbox_master_sem, USB_MAILBOX_MASTER_SEND_WAIT_MS);
					break;

				case USB_DRV_UVC_TRANSFER_BUFFER_MALLOC:
					break;
				case USB_DRV_UVC_TRANSFER_BUFFER_PUSH:
					break;
				case USB_DRV_UVC_TRANSFER_BUFFER_POP:
					break;
				case USB_DRV_UVC_TRANSFER_BUFFER_FREE:
					break;

				case USB_DRV_VIDEO_START:
					mb_cmd.hdr.cmd = USB_DRV_VIDEO_START;
					task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);
					break;
				case USB_DRV_VIDEO_DUAL_START:
					mb_cmd.hdr.cmd = USB_DRV_VIDEO_DUAL_START;
					task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);
					break;
				case USB_DRV_VIDEO_STOP:
					mb_cmd.hdr.cmd = USB_DRV_VIDEO_STOP;
					task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);
					break;

				case USB_DRV_UAC_REGISTER_CONNECT_CB:
					break;
				case USB_DRV_UAC_REGISTER_DISCONNECT_CB:
					break;
				case USB_DRV_UAC_GET_PARAM:
					task_mb_chnl_write(USB_MAILBOX_CHNL, (mb_chnl_cmd_t *)&msg.param);
					usb_mailbox_master_get_semaphore(&s_usb_mailbox_master_sem, USB_MAILBOX_MASTER_SEND_WAIT_MS);
					break;
				case USB_DRV_UAC_SET_PARAM:
					task_mb_chnl_write(USB_MAILBOX_CHNL, (mb_chnl_cmd_t *)&msg.param);
					usb_mailbox_master_get_semaphore(&s_usb_mailbox_master_sem, USB_MAILBOX_MASTER_SEND_WAIT_MS);
					break;
				case USB_DRV_UAC_REGISTER_TRANSFER_OPS:
					task_mb_chnl_write(USB_MAILBOX_CHNL, (mb_chnl_cmd_t *)&msg.param);
					usb_mailbox_master_get_semaphore(&s_usb_mailbox_master_sem, USB_MAILBOX_MASTER_SEND_WAIT_MS);
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
					mb_cmd.hdr.cmd = USB_DRV_AUDIO_MIC_START;
					task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);
					break;

				case USB_DRV_AUDIO_MIC_STOP:
					mb_cmd.hdr.cmd = USB_DRV_AUDIO_MIC_STOP;
					task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);
					break;
				case USB_DRV_AUDIO_SPK_START:
					mb_cmd.hdr.cmd = USB_DRV_AUDIO_SPK_START;
					task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);
					break;
				case USB_DRV_AUDIO_SPK_STOP:
					mb_cmd.hdr.cmd = USB_DRV_AUDIO_SPK_STOP;
					task_mb_chnl_write(USB_MAILBOX_CHNL, &mb_cmd);
					break;

				default:
					break;
			}
		}
	}

}

void bk_usb_mailbox_sw_master_init(void)
{
	uint32_t ret = 0;
	memset(&s_uac_config, 0, sizeof(bk_uac_config_t));

	if ((!s_usb_mailbox_master_thread_hdl) && (!s_usb_mailbox_master_msg_que)) {

		do{
			if(!s_usb_mailbox_master_sem) {
				ret = rtos_init_semaphore(&s_usb_mailbox_master_sem, 1);
				if(ret != kNoErr)
					break;
			}

			if(!s_usb_mailbox_master_usb_driver_exit_sem) {
				ret = rtos_init_semaphore(&s_usb_mailbox_master_usb_driver_exit_sem, 1);
				if(ret != kNoErr)
					break;
			}

			if(!s_usb_mailbox_master_rx_msg_que) {
				ret = rtos_init_queue(&s_usb_mailbox_master_rx_msg_que,
									  "usb_mb_master_rx_q",
									  sizeof(bk_usb_mailbox_msg_t),
									  USB_MAILBOX_MASTER_RX_QITEM_COUNT);
				if(ret != kNoErr)
					break;
			}

			ret = rtos_init_queue(&s_usb_mailbox_master_msg_que,
								  "usb_mb_master_queue",
								  sizeof(bk_usb_mailbox_msg_t),
								  USB_MAILBOX_MASTER_QITEM_COUNT);
			if(ret != kNoErr)
				break;

			//create usb driver task
			ret = rtos_create_thread(&s_usb_mailbox_master_thread_hdl,
								 USB_MAILBOX_MASTER_TASK_PRIO,
								 "usb_mb_master",
								 (beken_thread_function_t)usb_mailbox_master_task,
								 USB_MAILBOX_MASTER_TASK_SIZE,
								 NULL);
			if(ret != kNoErr)
				break;
		}while(0);
		
		if (ret != kNoErr) {
			LOGE("create usb driver task fail \r\n");
			if(s_usb_mailbox_master_sem) {
				rtos_deinit_semaphore(&s_usb_mailbox_master_sem);
				s_usb_mailbox_master_sem = NULL;
			}
			
			if(s_usb_mailbox_master_usb_driver_exit_sem) {
				rtos_deinit_semaphore(&s_usb_mailbox_master_usb_driver_exit_sem);
				s_usb_mailbox_master_usb_driver_exit_sem = NULL;
			}

			if(s_usb_mailbox_master_rx_msg_que) {
				rtos_deinit_queue(&s_usb_mailbox_master_rx_msg_que);
				s_usb_mailbox_master_rx_msg_que = NULL;
			}

			if(s_usb_mailbox_master_msg_que) {
				rtos_deinit_queue(&s_usb_mailbox_master_msg_que);
				s_usb_mailbox_master_msg_que = NULL;
			}

			if (s_usb_mailbox_master_thread_hdl) {
			rtos_delete_thread(&s_usb_mailbox_master_thread_hdl);
			s_usb_mailbox_master_thread_hdl = NULL;
			}

			return;
		}

		/* Mailbox */
		if(mb_chnl_open(USB_MAILBOX_CHNL, NULL) != BK_OK) {
			LOGE("usb mailbox master mb_chnl_open open fail \r\n");
			return;
		}

		mb_chnl_ctrl(USB_MAILBOX_CHNL, MB_CHNL_SET_RX_ISR, _mailbox_rx_isr);
		mb_chnl_ctrl(USB_MAILBOX_CHNL, MB_CHNL_SET_TX_ISR, _mailbox_tx_isr);
		mb_chnl_ctrl(USB_MAILBOX_CHNL, MB_CHNL_SET_TX_CMPL_ISR, _mailbox_tx_cmpl_isr);

	} else {
		return;
	}
}

void bk_usb_mailbox_sw_master_deinit(void)
{
	beken_queue_t usb_mailbox_master_msg_que_deinit = s_usb_mailbox_master_msg_que;
	beken_queue_t usb_mailbox_master_rx_msg_que_deinit = s_usb_mailbox_master_rx_msg_que;
	beken_semaphore_t usb_mailbox_master_sem_deinit = s_usb_mailbox_master_sem;
	beken_semaphore_t usb_mailbox_master_usb_driver_exit_sem_deinit = s_usb_mailbox_master_usb_driver_exit_sem;

	if(s_usb_mailbox_master_sem) {
		s_usb_mailbox_master_sem = NULL;
		rtos_deinit_semaphore(&usb_mailbox_master_sem_deinit);
	}

	if(s_usb_mailbox_master_usb_driver_exit_sem) {
		s_usb_mailbox_master_usb_driver_exit_sem = NULL;
		rtos_deinit_semaphore(&usb_mailbox_master_usb_driver_exit_sem_deinit);
	}

	if(s_usb_mailbox_master_rx_msg_que) {
		s_usb_mailbox_master_rx_msg_que = NULL;
		rtos_deinit_queue(&usb_mailbox_master_rx_msg_que_deinit);
	}

	if(s_usb_mailbox_master_msg_que) {
		s_usb_mailbox_master_msg_que = NULL;
		rtos_deinit_queue(&usb_mailbox_master_msg_que_deinit);
	}

	if (s_usb_mailbox_master_thread_hdl) {
		rtos_delete_thread(&s_usb_mailbox_master_thread_hdl);
		s_usb_mailbox_master_thread_hdl = NULL;
	}

}

bk_err_t bk_usb_driver_init(void)
{
	bk_usb_mailbox_sw_master_init();
	//usb_mailbox_master_send_msg(USB_DRV_USB_INIT, NULL);
	return BK_OK;
}

bk_err_t bk_usb_driver_deinit(void)
{
	usb_mailbox_master_send_msg(USB_DRV_USB_DEINIT, NULL);

	return BK_OK;
}

bk_err_t bk_usb_power_ops(uint32_t gpio_id, bool ops)
{
	mb_chnl_cmd_t mb_cmd;
	
	memset(&mb_cmd, 0, sizeof(mb_chnl_cmd_t));
	
	mb_cmd.hdr.cmd = USB_POWER_OPS;
	mb_cmd.param1 = gpio_id;
	mb_cmd.param2 = ops;

	if(ops) {
		USB_RETURN_NOT_POWERED_DOWN();


		gpio_dev_unprotect_unmap(gpio_id);
		bk_gpio_set_capacity(gpio_id, 0);
		BK_LOG_ON_ERR(bk_gpio_disable_input(gpio_id));
		BK_LOG_ON_ERR(bk_gpio_enable_output(gpio_id));

#if CONFIG_USB_MAILBOX_MASTER_CPU1_TO_CPU2
		if(CPU2_USER_USB == vote_start_cpu2_core(CPU2_USER_USB)) {//first owner start CPU2, so needs to wait sem
			if(usb_mailbox_master_get_semaphore(&s_usb_mailbox_master_sem, USB_MAILBOX_MASTER_SEND_WAIT_MS * 1000)) {
				LOGI("usb power on timeout\r\n");
			}
		}
#else 
		usb_mailbox_master_send_msg(USB_POWER_OPS, (void *)&mb_cmd);
		if(usb_mailbox_master_get_semaphore(&s_usb_mailbox_master_sem, USB_MAILBOX_MASTER_SEND_WAIT_MS * 1000)) {
			LOGI("usb power on timeout\r\n");
		}

#endif
		s_usb_power_on_flag = 1;
	} else {
		USB_RETURN_NOT_POWERED_ON();
		if(s_usb_dev_using_status)
			return BK_FAIL;

#if CONFIG_USB_MAILBOX_MASTER_CPU1_TO_CPU2
		vote_stop_cpu2_core(CPU2_USER_USB);
#else
		usb_mailbox_master_send_msg(USB_POWER_OPS, (void *)&mb_cmd);
		if(usb_mailbox_master_get_semaphore(&s_usb_mailbox_master_sem, USB_MAILBOX_MASTER_SEND_WAIT_MS * 1000)) {
			LOGI("usb power down timeout\r\n");
		}
#endif
		s_usb_power_on_flag = 0;
	}

	return 0;
}

bk_err_t bk_usb_open (uint32_t usb_mode)
{
	//rtos_lock_mutex((beken_mutex_t *)mutex);

#if 1
	if(s_usb_open_flag){
		if(s_usb_dev_connect_status)
			usbh_mailbox_connect_device();
	}
#endif

	USB_RETURN_NOT_POWERED_ON();
	USB_RETURN_NOT_CLOSE();

	bk_pm_module_vote_cpu_freq(PM_DEV_ID_USB_1, PM_CPU_FRQ_480M);
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_USB_1, 0x0, 0x0);

    mb_chnl_cmd_t mb_cmd;

	memset(&mb_cmd, 0, sizeof(mb_chnl_cmd_t));

	mb_cmd.hdr.cmd = USB_DRV_USB_OPEN;
	mb_cmd.param1 = usb_mode;
	usb_mailbox_master_send_msg(USB_DRV_USB_OPEN, (void *)&mb_cmd);

	if(usb_mailbox_master_get_semaphore(&s_usb_mailbox_master_sem, USB_MAILBOX_MASTER_SEND_WAIT_MS * 1000)) {
		LOGI("bk_usb_open timeout\r\n");
		return BK_FAIL;
	}

	s_usb_open_flag = 1;

	return BK_OK;
}

bk_err_t bk_usb_close (void)
{
	USB_RETURN_NOT_POWERED_ON();
	USB_RETURN_NOT_OPEN();
	if(s_usb_dev_using_status) {
		s_usb_dev_connect_status &= s_usb_dev_using_status;
		s_usb_dev_connect_status |= (0x1 << USB_USER_CHECK_DEVICE);
		return BK_FAIL;
	}

    mb_chnl_cmd_t mb_cmd;

	memset(&mb_cmd, 0, sizeof(mb_chnl_cmd_t));

	mb_cmd.hdr.cmd = USB_DRV_USB_CLOSE;

	usb_mailbox_master_send_msg(USB_DRV_USB_CLOSE, (void *)&mb_cmd);
	s_usb_open_flag = 0;

	/* 1000ms, There are up to 10packets(10ms) of data in the queue, and you need to wait for the
	 * data in the queue to be copied before performing the shutdown operation.
	 */
	if(usb_mailbox_master_get_semaphore(&s_usb_mailbox_master_usb_driver_exit_sem, USB_MAILBOX_MASTER_SEND_WAIT_MS * 500))
		LOGI("bk_usb_close timeout\r\n");
#if 0
	rtos_delay_milliseconds(400);
	bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU2, PM_POWER_MODULE_STATE_OFF);
	extern void stop_cpu2_core(void);
	stop_cpu2_core();
#endif
	s_usb_dev_connect_status = 0;

	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_USB_1, 0x1, 0x0);

	return 0;
}

void bk_usb_device_set_using_status(bool use_or_no, E_USB_DEVICE_T dev)
{
	USB_RETURN_NOT_OPEN_NULL();

	if(use_or_no)
		s_usb_dev_using_status |= (0x1 << dev);
	else
		s_usb_dev_using_status &= ~(0x1 << dev);

/*
    mb_chnl_cmd_t mb_cmd;

	memset(&mb_cmd, 0, sizeof(mb_chnl_cmd_t));

	mb_cmd.hdr.cmd = USB_DRV_SET_DEV_USE;
	mb_cmd.param1 = use_or_no;
	mb_cmd.param2 = dev;

	usb_mailbox_master_send_msg(USB_DRV_SET_DEV_USE, (void *)&mb_cmd);
*/
	return;
}

bool bk_usb_get_device_connect_status(void)
{
	USB_RETURN_NOT_OPEN();
	if(s_usb_dev_connect_status)
		return true;
	else
		return false;
/*
	usb_mailbox_master_send_msg(USB_DRV_GET_DEV_CONNECT_STATUS, NULL);

	bool connect_status = 0;
	bk_err_t ret = kNoErr;
    mb_chnl_cmd_t mb_cmd;
	bk_usb_mailbox_msg_t msg;

	ret = rtos_pop_from_queue(&s_usb_mailbox_master_rx_msg_que, &msg, BEKEN_WAIT_FOREVER);
	if (kNoErr == ret) {
		switch (msg.op) {
			case USB_DRV_GET_DEV_CONNECT_STATUS:
				memcpy(&mb_cmd, &msg.param, sizeof(mb_chnl_cmd_t));
				connect_status = mb_cmd.param1;
				break;
			default:
				break;
			}
	}

	return connect_status;
*/
}

bk_err_t bk_usb_check_device_supported(E_USB_DEVICE_T usb_dev)
{
	USB_RETURN_NOT_POWERED_ON();
	USB_RETURN_NOT_OPEN();
    mb_chnl_cmd_t smb_cmd;

	memset(&smb_cmd, 0, sizeof(mb_chnl_cmd_t));

	smb_cmd.hdr.cmd = USB_DRV_CHECK_DEV_SUPPORT;
	smb_cmd.param1 = usb_dev;

	usb_mailbox_master_send_msg(USB_DRV_CHECK_DEV_SUPPORT, (void *)&smb_cmd);

	bool check_status = 0;
	bk_err_t ret = kNoErr;
    mb_chnl_cmd_t mb_cmd;
	bk_usb_mailbox_msg_t msg;

	ret = rtos_pop_from_queue(&s_usb_mailbox_master_rx_msg_que, &msg, USB_MAILBOX_MASTER_SEND_WAIT_MS * 1000);
	if (kNoErr == ret) {
		switch (msg.op) {
			case USB_DRV_CHECK_DEV_SUPPORT:
				memcpy(&mb_cmd, &msg.param, sizeof(mb_chnl_cmd_t));
				check_status = mb_cmd.param1;
				break;
			default:
				break;
			}
	}

	return check_status;
}

bk_err_t bk_uvc_uac_register_callback(bk_uvc_uac_app_callback *uvc_uac_all_cb)
{
	USB_RETURN_NOT_POWERED_ON();
	USB_RETURN_NOT_OPEN();
	return 0;
}

bk_err_t bk_usb_uvc_register_connect_callback(void *param)
{
	s_usbh_mailbox_connect_cb[USB_UVC_DEVICE] = (FUNCPTR)param;

#if 0
	USB_RETURN_NOT_POWERED_ON();
    mb_chnl_cmd_t mb_cmd;

	memset(&mb_cmd, 0, sizeof(mb_chnl_cmd_t));

	mb_cmd.hdr.cmd = USB_DRV_UVC_REGISTER_CONNECT_CB;
	mb_cmd.param1 = (uint32_t)((uint32_t *)param);

	usb_mailbox_master_send_msg(USB_DRV_UVC_REGISTER_CONNECT_CB, (void *)&mb_cmd);
#endif

	return 0;
}

bk_err_t bk_usb_uvc_register_disconnect_callback(void *param)
{
	s_usbh_mailbox_disconnect_cb[USB_UVC_DEVICE] = (FUNCPTR)param;

#if 0
	USB_RETURN_NOT_POWERED_ON();
    mb_chnl_cmd_t mb_cmd;

	memset(&mb_cmd, 0, sizeof(mb_chnl_cmd_t));

	mb_cmd.hdr.cmd = USB_DRV_UVC_REGISTER_DISCONNECT_CB;
	mb_cmd.param1 = (uint32_t)((uint32_t *)param);

	usb_mailbox_master_send_msg(USB_DRV_UVC_REGISTER_DISCONNECT_CB, (void *)&mb_cmd);
#endif
	return 0;
}

bk_err_t bk_usb_uvc_get_param(bk_uvc_device_brief_info_t *uvc_device_info)
{
	USB_RETURN_NOT_POWERED_ON();
	USB_RETURN_NOT_OPEN();
    mb_chnl_cmd_t tmb_cmd;

	memset(&tmb_cmd, 0, sizeof(mb_chnl_cmd_t));

	tmb_cmd.hdr.cmd = USB_DRV_UVC_GET_PARAM;
	tmb_cmd.param1 = (uint32_t)((uint32_t *)uvc_device_info);

	usb_mailbox_master_send_msg(USB_DRV_UVC_GET_PARAM, (void *)&tmb_cmd);

	bk_err_t ret = kNoErr;
    mb_chnl_cmd_t mb_cmd;
	bk_usb_mailbox_msg_t msg;

	ret = rtos_pop_from_queue(&s_usb_mailbox_master_rx_msg_que, &msg, USB_MAILBOX_MASTER_SEND_WAIT_MS * 1000);
	if (kNoErr == ret) {
		switch (msg.op) {
			case USB_DRV_UVC_GET_PARAM:
				memcpy(&mb_cmd, &msg.param, sizeof(mb_chnl_cmd_t));
				ret = mb_cmd.param2;
				break;
			default:
				break;
			}
	} else {
			LOGE("[=]%s WAIT TIMEOUT\r\n",__func__);
	}

	return ret;
}

bk_err_t bk_usb_uvc_set_param(bk_uvc_config_t *uvc_config)
{
	USB_RETURN_NOT_POWERED_ON();
	USB_RETURN_NOT_OPEN();
    mb_chnl_cmd_t mb_cmd;

	memset(&mb_cmd, 0, sizeof(mb_chnl_cmd_t));

	mb_cmd.hdr.cmd = USB_DRV_UVC_SET_PARAM;
	mb_cmd.param1 = (uint32_t)((uint32_t *)uvc_config);

	usb_mailbox_master_send_msg(USB_DRV_UVC_SET_PARAM, (void *)&mb_cmd);

	return 0;
}


bk_err_t bk_usb_uvc_register_transfer_buffer_ops(void *ops)
{
	USB_RETURN_NOT_POWERED_ON();
    mb_chnl_cmd_t mb_cmd;

	memset(&mb_cmd, 0, sizeof(mb_chnl_cmd_t));

	mb_cmd.hdr.cmd = USB_DRV_UVC_REGISTER_TRANSFER_OPS;
	mb_cmd.param1 = (uint32_t)((uint32_t *)ops);

	usb_mailbox_video_packet_ops = (camera_packet_control_t *)ops;

	usb_mailbox_master_send_msg(USB_DRV_UVC_REGISTER_TRANSFER_OPS, (void *)&mb_cmd);

	return 0;
}

bk_err_t bk_uvc_start(void)
{
	USB_RETURN_NOT_OPEN();

	usb_mailbox_master_send_msg(USB_DRV_VIDEO_START, NULL);
	rtos_delay_milliseconds(10);

	return 0;
}

bk_err_t bk_uvc_dual_start(void)
{
	USB_RETURN_NOT_OPEN();
	usb_mailbox_master_send_msg(USB_DRV_VIDEO_DUAL_START, NULL);

	rtos_delay_milliseconds(10);
	return 0;
}

bk_err_t bk_uvc_stop(void)
{
	USB_RETURN_NOT_OPEN();

	usb_mailbox_master_send_msg(USB_DRV_VIDEO_STOP, NULL);
	rtos_delay_milliseconds(10);

	return 0;
}

bk_err_t bk_usb_uvc_check_support_attribute(uint32_t attribute)
{
	return 0;
}

bk_err_t bk_usb_uvc_attribute_op(E_USB_ATTRIBUTE_OP ops, uint32_t attribute, uint32_t *param)
{
	return 0;
}

bk_err_t bk_usb_uac_register_connect_callback(void *param)
{
	s_usbh_mailbox_connect_cb[USB_UAC_MIC_DEVICE] = (FUNCPTR)param;

#if 0
	USB_RETURN_NOT_POWERED_ON();

    mb_chnl_cmd_t mb_cmd;

	memset(&mb_cmd, 0, sizeof(mb_chnl_cmd_t));

	mb_cmd.hdr.cmd = USB_DRV_UAC_REGISTER_CONNECT_CB;
	mb_cmd.param1 = (uint32_t)((uint32_t *)param);

	usb_mailbox_master_send_msg(USB_DRV_UAC_REGISTER_CONNECT_CB, (void *)&mb_cmd);
#endif
	return 0;
}

bk_err_t bk_usb_uac_register_disconnect_callback(void *param)
{
	s_usbh_mailbox_disconnect_cb[USB_UAC_MIC_DEVICE] = (FUNCPTR)param;

#if 0
	USB_RETURN_NOT_POWERED_ON();

    mb_chnl_cmd_t mb_cmd;

	memset(&mb_cmd, 0, sizeof(mb_chnl_cmd_t));

	mb_cmd.hdr.cmd = USB_DRV_UAC_REGISTER_DISCONNECT_CB;
	mb_cmd.param1 = (uint32_t)((uint32_t *)param);

	usb_mailbox_master_send_msg(USB_DRV_UAC_REGISTER_DISCONNECT_CB, (void *)&mb_cmd);
#endif
	return 0;
}

bk_err_t bk_usb_uac_get_param(bk_uac_device_brief_info_t *uac_device_info)
{
	USB_RETURN_NOT_POWERED_ON();
	USB_RETURN_NOT_OPEN();
    mb_chnl_cmd_t tmb_cmd;

	memset(&tmb_cmd, 0, sizeof(mb_chnl_cmd_t));

	tmb_cmd.hdr.cmd = USB_DRV_UAC_GET_PARAM;
	tmb_cmd.param1 = (uint32_t)((uint32_t *)uac_device_info);

	usb_mailbox_master_send_msg(USB_DRV_UAC_GET_PARAM, (void *)&tmb_cmd);

	bk_err_t ret = kNoErr;
    mb_chnl_cmd_t mb_cmd;
	bk_usb_mailbox_msg_t msg;

	ret = rtos_pop_from_queue(&s_usb_mailbox_master_rx_msg_que, &msg, USB_MAILBOX_MASTER_SEND_WAIT_MS * 1000);
	if (kNoErr == ret) {
		switch (msg.op) {
			case USB_DRV_UAC_GET_PARAM:
				memcpy(&mb_cmd, &msg.param, sizeof(mb_chnl_cmd_t));
				ret = mb_cmd.param2;
				break;
			default:
				break;
			}
	} else {
			LOGE("[=]%s WAIT TIMEOUT\r\n",__func__);
	}

	return ret;

}

bk_err_t bk_usb_uac_set_param(bk_uac_config_t *uac_config)
{
	USB_RETURN_NOT_POWERED_ON();
	USB_RETURN_NOT_OPEN();

    mb_chnl_cmd_t tmb_cmd;

	memcpy(&s_uac_config, uac_config, sizeof(bk_uac_config_t));

	memset(&tmb_cmd, 0, sizeof(mb_chnl_cmd_t));

	tmb_cmd.hdr.cmd = USB_DRV_UAC_SET_PARAM;
	tmb_cmd.param1 = (uint32_t)(&s_uac_config);

	usb_mailbox_master_send_msg(USB_DRV_UAC_SET_PARAM, (void *)&tmb_cmd);

	bk_err_t ret = kNoErr;
    mb_chnl_cmd_t mb_cmd;
	bk_usb_mailbox_msg_t msg;

	ret = rtos_pop_from_queue(&s_usb_mailbox_master_rx_msg_que, &msg, USB_MAILBOX_MASTER_SEND_WAIT_MS * 1000);
	if (kNoErr == ret) {
		switch (msg.op) {
			case USB_DRV_UAC_SET_PARAM:
				memcpy(&mb_cmd, &msg.param, sizeof(mb_chnl_cmd_t));
				ret = mb_cmd.param2;
				break;
			default:
				break;
			}
	} else {
			LOGE("[=]%s WAIT TIMEOUT\r\n",__func__);
	}

	return 0;

}

bk_err_t bk_usb_uac_set_mic_param(bk_uac_mic_config_t *uac_mic_config)
{
    s_uac_config.mic_format_tag = uac_mic_config->mic_format_tag;
    s_uac_config.mic_samples_frequence = uac_mic_config->mic_samples_frequence;
    s_uac_config.mic_ep_desc = uac_mic_config->mic_ep_desc;
    return bk_usb_uac_set_param(&s_uac_config);
}

bk_err_t bk_usb_uac_set_spk_param(bk_uac_spk_config_t *uac_spk_config)
{
    s_uac_config.spk_format_tag = uac_spk_config->spk_format_tag;
    s_uac_config.spk_samples_frequence = uac_spk_config->spk_samples_frequence;
    s_uac_config.spk_ep_desc = uac_spk_config->spk_ep_desc;
    return bk_usb_uac_set_param(&s_uac_config);
}

bk_err_t bk_usb_uac_register_transfer_buffer_ops(void *ops)
{
	USB_RETURN_NOT_POWERED_ON();
    mb_chnl_cmd_t mb_cmd;

	memset(&mb_cmd, 0, sizeof(mb_chnl_cmd_t));

	mb_cmd.hdr.cmd = USB_DRV_UAC_REGISTER_TRANSFER_OPS;
	mb_cmd.param1 = (uint32_t)((uint32_t *)ops);

	usb_mailbox_audio_packet_ops = (struct audio_packet_control_t *)ops;

	usb_mailbox_master_send_msg(USB_DRV_UAC_REGISTER_TRANSFER_OPS, (void *)&mb_cmd);

	return 0;
}

bk_err_t bk_uac_start_mic(void)
{
	USB_RETURN_NOT_OPEN();

	usb_mailbox_master_send_msg(USB_DRV_AUDIO_MIC_START, NULL);
	rtos_delay_milliseconds(10);

	return 0;
}

bk_err_t bk_uac_stop_mic(void)
{
	USB_RETURN_NOT_OPEN();

	usb_mailbox_master_send_msg(USB_DRV_AUDIO_MIC_STOP, NULL);
	rtos_delay_milliseconds(10);

	return 0;
}

bk_err_t bk_uac_start_speaker(void)
{
	USB_RETURN_NOT_OPEN();

	usb_mailbox_master_send_msg(USB_DRV_AUDIO_SPK_START, NULL);
	rtos_delay_milliseconds(10);

	return 0;
}

bk_err_t bk_uac_stop_speaker(void)
{
	USB_RETURN_NOT_OPEN();

	usb_mailbox_master_send_msg(USB_DRV_AUDIO_SPK_STOP, NULL);
	rtos_delay_milliseconds(10);

	return 0;
}

bk_err_t bk_usb_uac_check_support_attribute(uint32_t attribute)
{
	return 0;
}

bk_err_t bk_usb_uac_attribute_op(E_USB_ATTRIBUTE_OP ops, uint32_t attribute, uint32_t *param)
{
	return 0;
}

#endif //CONFIG_USB_MAILBOX_MASTER

#endif //CONFIG_MAILBOX

