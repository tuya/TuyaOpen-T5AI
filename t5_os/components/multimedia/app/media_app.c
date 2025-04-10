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
#include <os/str.h>
#include <os/mem.h>
#include <components/log.h>
#include <common/bk_include.h>
#include <driver/int.h>
#include <driver/dvp_camera_types.h>
#include <driver/pwr_clk.h>

#include <components/video_transfer.h>

#include "media_core.h"
#include "media_evt.h"
#include "media_app.h"
#include "transfer_act.h"
#include "camera_act.h"
#include "lcd_act.h"
#include "draw_blend.h"

#include "storage_act.h"

#include "media_mailbox_list_util.h"
#include "camera_driver.h"

#ifdef CONFIG_BT_REUSE_MEDIA_MEMORY
#include "components/bluetooth/bk_ble.h"
#include "components/bluetooth/bk_dm_ble.h"
#include "components/bluetooth/bk_dm_bluetooth.h"
#endif

#define TAG "media_app"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static app_camera_type_t app_camera_type = APP_CAMERA_INVALIED;
static char *capture_name = NULL;
static media_modules_state_t *media_modules_state = NULL;

#if CONFIG_CHERRY_USB && CONFIG_USB_DEVICE
extern void usbd_video_h264_init();
extern void usbd_video_h264_deinit();
#endif

static beken_thread_t media_app_th_hd = NULL;
static beken_queue_t media_app_msg_queue = NULL;
static uvc_device_info_t uvc_device_info_cb = NULL;
static compress_ratio_t compress_factor = {0};

typedef struct {
	media_rotate_t rotate;
} jpeg_decode_pipeline_param_t;

static jpeg_decode_pipeline_param_t jpeg_decode_pipeline_param = {0};

#if CONFIG_TUYA_LOGIC_MODIFY
#include "tkl_mutex.h"
static TKL_MUTEX_HANDLE media_mutex = NULL;
static bk_err_t tuya_single_core_send_media_msg(uint32_t event, uint32_t param)
{
    int ret = 0;

    media_msg_t msg;
    media_mailbox_msg_t mb_msg;

    // TODO 防重入，加锁

    if (media_mutex == NULL) {
        ret = tkl_mutex_create_init(&media_mutex);
        if(ret) {
            return ret;
        }
    }

    tkl_mutex_lock(media_mutex);

    msg.param = (uint32_t)&mb_msg;
    msg.event = event;
    mb_msg.event = event;
    mb_msg.param = param;
    media_send_msg(&msg);

    tkl_mutex_unlock(media_mutex);

    return ret;
}
#endif // CONFIG_TUYA_LOGIC_MODIFY

bk_err_t media_send_msg_sync(uint32_t event, uint32_t param)
{
#if CONFIG_TUYA_LOGIC_MODIFY
    return tuya_single_core_send_media_msg(event, param);
#else
	int ret = BK_FAIL;
	ret = msg_send_req_to_media_app_mailbox_sync(event, param, NULL);
	if (ret != BK_OK)
	{
		LOGE("%s failed 0x%x\n", __func__, ret);
	}
	return ret;
#endif // CONFIG_TUYA_LOGIC_MODIFY
}

uint32_t media_send_msg_sync_return_param(uint32_t event, uint32_t in_param, uint32_t *out_param)
{
#if CONFIG_TUYA_LOGIC_MODIFY
    *(uint32_t *)out_param = 0; // TODO
    return tuya_single_core_send_media_msg(event, in_param);
#else
	int ret = BK_FAIL;

	ret = msg_send_req_to_media_app_mailbox_sync(event, in_param, out_param);
	if (ret != BK_OK)
	{
		LOGE("%s failed 0x%x\n", __func__, ret);
	}

	return ret;
#endif // CONFIG_TUYA_LOGIC_MODIFY
}

bk_err_t media_app_camera_open(media_camera_device_t *device)
{
	int ret = BK_FAIL;

	if (CAMERA_STATE_DISABLED != media_modules_state->cam_state)
	{
		LOGI("%s already opened\n", __func__);
		return BK_OK;
	}
	if (device->dualstream == 1)
	{
		device->num_uvc_dev = 2;
	}
	else
	{
		device->num_uvc_dev = 1;
	}

	{
		device->uvc_device[0].mode                   = device->mode;
		device->uvc_device[0].fmt                    = device->fmt;
		device->uvc_device[0].info.fps               = device->info.fps;
		device->uvc_device[0].info.resolution.width  = device->info.resolution.width;
		device->uvc_device[0].info.resolution.height = device->info.resolution.height;
	}

	if (device->num_uvc_dev == 1) {
		LOGI("%s, %d-%d, mode:%d, type:%d\r\n", __func__, device->uvc_device[0].info.resolution.width, device->uvc_device[0].info.resolution.height,
		device->uvc_device[0].mode, device->type);
	} else {
        device->uvc_device[1].mode                   = device->d_mode;
        device->uvc_device[1].fmt                    = device->d_fmt;
        device->uvc_device[1].info.fps               = device->d_info.fps;
        device->uvc_device[1].info.resolution.width  = device->d_info.resolution.width;
        device->uvc_device[1].info.resolution.height = device->d_info.resolution.height;

        LOGI("%s, %d-%d, mode:%d, type:%d\r\n", __func__, device->uvc_device[0].info.resolution.width, device->uvc_device[0].info.resolution.height,
                device->uvc_device[0].mode, device->type);
        LOGI("%s, %d-%d, mode:%d, type:%d\r\n", __func__, device->uvc_device[1].info.resolution.width, device->uvc_device[1].info.resolution.height,
                device->uvc_device[1].mode, device->type);
    }

#ifdef CONFIG_BT_REUSE_MEDIA_MEMORY
#if CONFIG_BLUETOOTH
	bk_bluetooth_deinit();
#endif
#endif

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_JPEG_EN, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	if (compress_factor.enable)
	{
		ret = media_send_msg_sync(EVENT_CAM_COMPRESS_IND, (uint32_t)&compress_factor);
		if (ret != BK_OK)
		{
			LOGI("%s set error, %d\r\n", __func__, __LINE__);
		}
	}

	switch (device->type)
	{
		case DVP_CAMERA:
			ret = media_send_msg_sync(EVENT_CAM_DVP_OPEN_IND, (uint32_t)device);
			break;

		case UVC_CAMERA:
			ret = media_send_msg_sync(EVENT_CAM_UVC_OPEN_IND, (uint32_t)device);
			break;

		case NET_CAMERA:
			ret = media_send_msg_sync(EVENT_CAM_NET_OPEN_IND, (uint32_t)device);
			break;

		default:
			break;
	}

	switch (device->mode)
	{
		case JPEG_MODE:
		case JPEG_YUV_MODE:
			app_camera_type = APP_CAMERA_DVP_JPEG;
			break;

		case H264_MODE:
		case H264_YUV_MODE:
		case H265_MODE:
			app_camera_type = APP_CAMERA_DVP_H264_ENC_LCD;
			break;

		default:
			break;
	}

	if (ret == BK_OK)
	{
		media_modules_state->cam_state = CAMERA_STATE_ENABLED;
	}
	else
	{
#if !CONFIG_TUYA_LOGIC_MODIFY
		bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_JPEG_EN, PM_POWER_MODULE_STATE_OFF);
#endif // !CONFIG_TUYA_LOGIC_MODIFY
	}

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_camera_close(camera_type_t type)
{
	int ret = BK_FAIL;

	if (CAMERA_STATE_ENABLED != media_modules_state->cam_state)
	{
		LOGI("%s already closed\n", __func__);
		return BK_OK;
	}

	switch (type)
	{
		case DVP_CAMERA:
			ret = media_send_msg_sync(EVENT_CAM_DVP_CLOSE_IND, 0);
			break;

		case UVC_CAMERA:
			ret = media_send_msg_sync(EVENT_CAM_UVC_CLOSE_IND, 0);
			break;

		case NET_CAMERA:
			ret = media_send_msg_sync(EVENT_CAM_NET_CLOSE_IND, 0);
			break;

		default:
			break;
	}

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_JPEG_EN, PM_POWER_MODULE_STATE_OFF);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	if (ret == BK_OK)
	{
		media_modules_state->cam_state = CAMERA_STATE_DISABLED;
	}

	os_memset(&compress_factor, 0, sizeof(compress_ratio_t));

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_get_h264_encode_config(h264_base_config_t *config)
{
	int ret = BK_FAIL;

	if (CAMERA_STATE_ENABLED != media_modules_state->cam_state)
	{
		LOGI("%s h264 function not open!\n", __func__);
		return ret;
	}

	ret = media_send_msg_sync(EVENT_CAM_GET_H264_INFO_IND, (uint32_t)config);

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_uvc_register_info_notify_cb(uvc_device_info_t cb)
{
	int ret = BK_FAIL;

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_JPEG_EN, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	uvc_device_info_cb = cb;

	ret = media_send_msg_sync(EVENT_CAM_REG_UVC_INFO_CB_IND, 0);

	return ret;
}

bk_err_t media_app_set_uvc_device_param(bk_uvc_config_t *config)
{
	int ret = BK_FAIL;

	if (config == NULL)
	{
		LOGI("%s param not init\n", __func__);
		return ret;
	}

	if (CAMERA_STATE_ENABLED != media_modules_state->cam_state)
	{
		LOGE("%s uvc not open\n", __func__);
		return ret;
	}

	ret = media_send_msg_sync(EVENT_CAM_SET_UVC_PARAM_IND, (uint32_t)config);

	return ret;
}

bk_err_t media_app_set_compression_ratio(compress_ratio_t *ratio)
{
	int ret = BK_FAIL;

	os_memcpy(&compress_factor, ratio, sizeof(compress_ratio_t));

	if (CAMERA_STATE_DISABLED == media_modules_state->cam_state)
	{
		LOGI("%s cam not opened\n", __func__);
		return BK_OK;
	}

	ret = media_send_msg_sync(EVENT_CAM_COMPRESS_IND, (uint32_t)ratio);

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_rtsp_open(video_config_t *config)
{
	int ret = BK_FAIL;

	if (CAMERA_STATE_DISABLED != media_modules_state->cam_state)
	{
		LOGI("%s already opened\n", __func__);
		return BK_OK;
	}

	LOGI("%s, %d-%d, mode:%d, type:%d\r\n", __func__, config->device->info.resolution.width, config->device->info.resolution.height,
			config->device->mode, config->device->type);
	dvp_camera_dma_config(config);

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_JPEG_EN, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	ret = media_send_msg_sync(EVENT_CAM_RTSP_OPEN_IND, (uint32_t)config->device);

	switch (config->device->mode)
	{
		case JPEG_MODE:
		case JPEG_YUV_MODE:
			app_camera_type = APP_CAMERA_DVP_JPEG;
			break;

		case H264_MODE:
		case H264_YUV_MODE:
			app_camera_type = APP_CAMERA_DVP_H264_ENC_LCD;
			break;

		default:
			break;
	}

	if (ret == BK_OK)
	{
		media_modules_state->cam_state = CAMERA_STATE_ENABLED;
	}

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_rtsp_close()
{
	int ret = BK_FAIL;

	if (CAMERA_STATE_ENABLED != media_modules_state->cam_state)
	{
		LOGI("%s already closed\n", __func__);
		return BK_OK;
	}
	bk_dvp_dma_deinit();

	ret = media_send_msg_sync(EVENT_CAM_RTSP_CLOSE_IND, 0);

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_JPEG_EN, PM_POWER_MODULE_STATE_OFF);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	if (ret == BK_OK)
	{
		media_modules_state->cam_state = CAMERA_STATE_DISABLED;
	}

	LOGI("%s complete\n", __func__);

	return ret;
}


bk_err_t media_app_h264_pipeline_open(void)
{
	int ret = BK_OK;

#ifdef CONFIG_BT_REUSE_MEDIA_MEMORY
#if CONFIG_BLUETOOTH
	bk_bluetooth_deinit();
#endif
#endif

	ret = media_send_msg_sync(EVENT_PIPELINE_SET_ROTATE_IND, jpeg_decode_pipeline_param.rotate);

	LOGI("%s set rotate %x\n", __func__, ret);

	ret = media_send_msg_sync(EVENT_PIPELINE_H264_OPEN_IND, 0);

	LOGI("%s complete %x\n", __func__, ret);

	return ret;
}

bk_err_t media_app_h264_pipeline_close(void)
{
	int ret = BK_OK;
	ret = media_send_msg_sync(EVENT_PIPELINE_H264_CLOSE_IND, 0);

	LOGI("%s complete %x\n", __func__, ret);

	return ret;
}

bk_err_t media_app_h264_regenerate_idr(camera_type_t type)
{
	int ret = BK_FAIL;

	if (CAMERA_STATE_DISABLED == media_modules_state->cam_state)
	{
		LOGI("%s camera not open\n", __func__);
		return ret;
	}

	if (type == UVC_CAMERA)
	{
		ret = media_send_msg_sync(EVENT_PIPELINE_H264_RESET_IND, 0);
	}
	else
	{
		ret = media_send_msg_sync(EVENT_CAM_H264_RESET_IND, 0);
	}

	LOGI("%s complete %x\n", __func__, ret);

	return ret;
}

bk_err_t media_app_lcd_fmt(pixel_format_t fmt)
{
	bk_err_t ret;

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY
	ret = media_send_msg_sync(EVENT_LCD_SET_FMT_IND, fmt);

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_pipline_set_rotate(media_rotate_t rotate)
{
	int ret = BK_OK;

	jpeg_decode_pipeline_param.rotate = rotate;
	LOGE("%s %d %d\n", __func__, __LINE__, rotate);
	return ret;
}

bk_err_t media_app_lcd_pipeline_open(void *config)
{
    int ret = BK_OK;

    ret = media_app_lcd_pipeline_disp_open(config);
    if (ret != BK_OK) {
        LOGE("%s media_app_lcd_pipeline_disp_open fail\n", __func__);
        return ret;
    }

    ret = media_app_lcd_pipeline_jdec_open();

    return ret;
}

bk_err_t media_app_lcd_pipeline_close(void)
{
    int ret = BK_OK;

    ret = media_app_lcd_pipeline_jdec_close();
    if (ret != BK_OK) {
        LOGE("%s media_app_lcd_pipeline_jdec_close fail\n", __func__);
        return ret;
    }

    ret = media_app_lcd_pipeline_disp_close();

    return ret;
}

bk_err_t media_app_lcd_pipeline_disp_open(void *config)
{
	int ret = BK_OK;

	if (config == NULL) {
		LOGE("malloc lcd_open_t failed\r\n");
		return BK_FAIL;
	}

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	ret = media_send_msg_sync(EVENT_PIPELINE_LCD_DISP_OPEN_IND, (uint32_t)config);
	LOGI("%s complete %x\n", __func__, ret);

	return ret;
}

bk_err_t media_app_lcd_pipeline_disp_close(void)
{
	int ret = BK_OK;

	ret = media_send_msg_sync(EVENT_PIPELINE_LCD_DISP_CLOSE_IND, 0);

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_OFF);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	LOGI("%s complete %x\n", __func__, ret);

	return ret;
}

bk_err_t media_app_lcd_pipeline_jdec_open(void)
{
    int ret = BK_OK;

#ifdef CONFIG_BT_REUSE_MEDIA_MEMORY
#if (CONFIG_BLUETOOTH)
    bk_bluetooth_deinit();
#endif
#endif

#if !CONFIG_TUYA_LOGIC_MODIFY
    bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_JPEG_DE, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

    ret = media_send_msg_sync(EVENT_PIPELINE_SET_ROTATE_IND, jpeg_decode_pipeline_param.rotate);
    LOGI("%s set rotate %x\n", __func__, ret);

    ret = media_send_msg_sync(EVENT_PIPELINE_LCD_JDEC_OPEN_IND, 0);
    LOGI("%s complete %x\n", __func__, ret);

    return ret;
}

//Modified by TUYA Start
bk_err_t media_app_lcd_startup_frame_open(void *config)
{
	int ret = BK_OK;
#ifdef CONFIG_BT_REUSE_MEDIA_MEMORY
#if CONFIG_BLUETOOTH
	bk_bluetooth_deinit();
#endif
#endif

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_APP, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	ret = media_send_msg_sync(EVENT_PIPELINE_STARTUP_OPEN_IND, (uint32_t)config);
	LOGI("%s complete %x\n", __func__, ret);
    return ret;
}


bk_err_t media_app_lcd_startup_frame_close(void)
{
	int ret = BK_OK;

	ret = media_send_msg_sync(EVENT_PIPELINE_STARTUP_CLOSE_IND, 0);

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_APP, PM_POWER_MODULE_STATE_OFF);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	LOGI("%s complete %x\n", __func__, ret);

	return ret;
}
//Modified by TUYA End

bk_err_t media_app_lcd_pipeline_jdec_close(void)
{
	int ret = BK_OK;

	ret = media_send_msg_sync(EVENT_PIPELINE_LCD_JDEC_CLOSE_IND, 0);

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_JPEG_DE, PM_POWER_MODULE_STATE_OFF);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	LOGI("%s complete %x\n", __func__, ret);

	return ret;
}

bk_err_t media_app_lcd_pipline_scale_open(void *config)
{
	int ret = BK_OK;

#ifdef CONFIG_BT_REUSE_MEDIA_MEMORY
#if CONFIG_BLUETOOTH
	bk_bluetooth_deinit();
#endif
#endif

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_SCALE, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	ret = media_send_msg_sync(EVENT_PIPELINE_LCD_SCALE_IND, (uint32_t)config);

	LOGI("%s complete %x\n", __func__, ret);

	return ret;
}

bk_err_t media_app_lcd_pipline_scale_close(void)
{
	int ret = BK_OK;

	ret = media_send_msg_sync(EVENT_PIPELINE_LCD_SCALE_IND, 0);

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_SCALE, PM_POWER_MODULE_STATE_OFF);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	LOGI("%s complete %x\n", __func__, ret);

	return ret;
}

bk_err_t media_app_mailbox_test(void)
{
	bk_err_t ret = BK_FAIL;
	uint32_t param = 0x88888888;

	LOGI("%s +++\n", __func__);

	ret = media_send_msg_sync(EVENT_LCD_DEFAULT_CMD, param);

	LOGI("%s ---\n", __func__);

	return ret;
}

bk_err_t media_app_register_read_frame_callback(pixel_format_t fmt, frame_cb_t cb)
{
	int ret = BK_OK;

	LOGI("%s\n", __func__);

	if (media_modules_state->trs_state == TRS_STATE_ENABLED)
	{
		LOGI("%s, transfer have been opened!\r\n", __func__);
		return ret;
	}

	ret = transfer_app_task_init(cb);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = media_send_msg_sync(EVENT_TRANSFER_OPEN_IND, (uint32_t)fmt);

	if (ret == BK_OK)
	{
		media_modules_state->trs_state = TRS_STATE_ENABLED;
	}

	LOGI("%s complete\n", __func__);

	return ret;
}


bk_err_t media_app_transfer_pause(bool pause)
{
	int ret = kNoErr;

	ret = media_send_msg_sync(EVENT_TRANSFER_PAUSE_IND, pause);

	return ret;
}

bk_err_t media_app_unregister_read_frame_callback(void)
{
	bk_err_t ret = BK_OK;

	media_mailbox_msg_t mb_msg;

	if (media_modules_state->trs_state == TRS_STATE_DISABLED)
	{
		LOGI("%s, transfer have been closed!\r\n", __func__);
		return ret;
	}

	LOGI("%s\n", __func__);

	ret = media_send_msg_sync(EVENT_TRANSFER_CLOSE_IND, 0);

	mb_msg.event = EVENT_TRANSFER_CLOSE_IND;
	transfer_app_event_handle(&mb_msg);

	if (ret == BK_OK)
	{
		media_modules_state->trs_state = TRS_STATE_DISABLED;
	}

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_usb_open(video_setup_t *setup_cfg)
{
	int ret = kNoErr;
#if CONFIG_CHERRY_USB && CONFIG_USB_DEVICE

	uint32_t param_fmt = FB_INDEX_H264;

	app_camera_type_t type = setup_cfg->open_type;

	usbd_video_h264_init();

	ret = usb_app_task_init((video_setup_t *)setup_cfg);
	if (ret != kNoErr)
	{
		LOGE("usb_app_task_init failed\r\n");
		return ret;
	}

	switch (type)
	{
		case APP_CAMERA_DVP_H264_TRANSFER:
			param_fmt = FB_INDEX_H264;
			break;
		case APP_CAMERA_DVP_JPEG:
			param_fmt = FB_INDEX_JPEG;

		default:
			LOGE("unsupported format! \r\n");
			break;
	}

	LOGI("%s, %d %d\n", __func__, type, param_fmt);

	ret = media_send_msg_sync(EVENT_TRANSFER_USB_OPEN_IND, param_fmt);

	LOGI("%s complete\n", __func__);
#endif

	return ret;
}

bk_err_t media_app_usb_close(void)
{
	bk_err_t ret = BK_OK;

#if CONFIG_CHERRY_USB && CONFIG_USB_DEVICE
	media_mailbox_msg_t mb_msg;

	LOGI("%s\n", __func__);

	usbd_video_h264_deinit();

	mb_msg.event = EVENT_TRANSFER_USB_CLOSE_IND;
	usb_app_event_handle(&mb_msg);

	ret = media_send_msg_sync(EVENT_TRANSFER_USB_CLOSE_IND, 0);
#endif

	return ret;
}

bk_err_t media_app_lcd_rotate(media_rotate_t rotate)
{
	bk_err_t ret;

	LOGI("%s\n", __func__);
#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY
	ret = media_send_msg_sync(EVENT_LCD_ROTATE_ENABLE_IND, rotate);

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_lcd_decode(media_decode_mode_t decode_mode)
{
	bk_err_t ret;

	LOGI("%s\n", __func__);
#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	ret = media_send_msg_sync(EVENT_LCD_DECODE_MODE_IND, decode_mode);

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_lcd_display_beken(void *lcd_display)
{
	bk_err_t ret;

	lcd_display_t *ptr = NULL;
	ptr = (lcd_display_t *)os_malloc(sizeof(lcd_display_t));
	os_memcpy(ptr, (lcd_display_t *)lcd_display, sizeof(lcd_display_t));

	ret = media_send_msg_sync(EVENT_LCD_BEKEN_LOGO_DISPLAY, (uint32_t)ptr);
	LOGI("%s complete\n", __func__);
	os_free(ptr);
	ptr =NULL;

	return ret;
}

// step 1
uint32_t media_app_get_lcd_devices_num(void)
{
	uint32_t num;
	bk_err_t ret;
#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_GET_MEDIA_MSG, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY
	ret = media_send_msg_sync_return_param(EVENT_LCD_GET_DEVICES_NUM_IND, 0, &num);
	if (ret != BK_OK)
	{
		LOGE("%s error\n", __func__);
	}
#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_GET_MEDIA_MSG, PM_POWER_MODULE_STATE_OFF);
#endif // !CONFIG_TUYA_LOGIC_MODIFY
	return num;
}

// step 2
uint32_t media_app_get_lcd_devices_list(void)
{
	uint32_t device_addr = 0;
	bk_err_t ret;
#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_GET_MEDIA_MSG, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY
	ret = media_send_msg_sync_return_param(EVENT_LCD_GET_DEVICES_LIST_IND, 0, &device_addr);
	if (ret != BK_OK)
	{
		LOGE("%s error\n", __func__);
	}
#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_GET_MEDIA_MSG, PM_POWER_MODULE_STATE_OFF);
#endif // !CONFIG_TUYA_LOGIC_MODIFY
	return device_addr;
}

uint32_t media_app_get_lcd_device_by_id(uint32_t id)
{
	uint32_t lcd_device = 0;
	bk_err_t ret;
#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_GET_MEDIA_MSG, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY
	ret = media_send_msg_sync_return_param(EVENT_LCD_GET_DEVICES_IND, id, &lcd_device);
	if (ret != BK_OK)
	{
		LOGE("%s error\n", __func__);
	}
#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_GET_MEDIA_MSG, PM_POWER_MODULE_STATE_OFF);
#endif // !CONFIG_TUYA_LOGIC_MODIFY
	return lcd_device;
}

bk_err_t media_app_get_lcd_status(void)
{
	uint32_t lcd_status = 0;
	bk_err_t ret;
#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_GET_MEDIA_MSG, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY
	ret = media_send_msg_sync_return_param(EVENT_LCD_GET_STATUS_IND, 0, &lcd_status);
	if (ret != BK_OK)
	{
		LOGE("%s error\n", __func__);
	}
#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_GET_MEDIA_MSG, PM_POWER_MODULE_STATE_OFF);
#endif // !CONFIG_TUYA_LOGIC_MODIFY
	return lcd_status;
}

bk_err_t media_app_get_uvc_camera_status(void)
{
	uint32_t camera_status = 0;
	bk_err_t ret;
#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_GET_MEDIA_MSG, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY
	ret = media_send_msg_sync_return_param(EVENT_GET_UVC_STATUS_IND, 0, &camera_status);
	if (ret != BK_OK)
	{
		LOGE("%s error\n", __func__);
	}
#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_GET_MEDIA_MSG, PM_POWER_MODULE_STATE_OFF);
#endif // !CONFIG_TUYA_LOGIC_MODIFY
	return camera_status;
}

// Modified by TUYA Start
bk_err_t media_app_get_usb_connect_status(void)
{
	uint32_t camera_status = 0;
	bk_err_t ret;
#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_GET_MEDIA_MSG, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY
	ret = media_send_msg_sync_return_param(EVENT_GET_USB_STATUS_IND, 0, &camera_status);
	if (ret != BK_OK)
	{
		LOGE("%s error\n", __func__);
	}
#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_GET_MEDIA_MSG, PM_POWER_MODULE_STATE_OFF);
#endif // !CONFIG_TUYA_LOGIC_MODIFY
	return camera_status;
}
// Modified by TUYA End

bk_err_t media_app_lcd_open(void *lcd_open)
{
	int ret = kNoErr;
// Modified by TUYA Start
#if 0
	lcd_open_t *ptr = NULL;

	ptr = (lcd_open_t *)os_malloc(sizeof(lcd_open_t));
	if (ptr == NULL) {
		LOGE("malloc lcd_open_t failed\r\n");
		return kGeneralErr;
	}
	os_memcpy(ptr, (lcd_open_t *)lcd_open, sizeof(lcd_open_t));
#endif

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	// ret = media_send_msg_sync(EVENT_LCD_OPEN_IND, (uint32_t)ptr);
	ret = media_send_msg_sync(EVENT_LCD_OPEN_IND, (uint32_t)lcd_open);

#if 0
	if (ptr) {
		os_free(ptr);
		ptr =NULL;
	}
#endif
// Modified by TUYA End

	LOGI("%s complete %x\n", __func__, ret);

	return ret;
}

bk_err_t media_app_lcd_display_file(char *file_name)
{
	int ret;

	LOGI("%s ,%s\n", __func__,file_name);

	if (file_name != NULL)
	{
		uint32_t len = os_strlen(file_name) + 1;

		if (len > 31)
		{
			len = 31;
		}

		if (capture_name == NULL)
		{
			capture_name = (char *)os_malloc(len);
		}
		else
		{
			os_free(capture_name);
			capture_name = NULL;
			capture_name = (char *)os_malloc(len);
		}
		os_memset(capture_name, 0, len);
		os_memcpy(capture_name, file_name, len);
		capture_name[len - 1] = '\0';
	}

	ret = media_send_msg_sync(EVENT_LCD_DISPLAY_FILE_IND, 0);

	return ret;

}

bk_err_t read_storage_file_to_mem_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;
	LOGI("%s\n", __func__);

	media_mailbox_msg_t *resd_storage_node = msg;
	frame_buffer_t *frame = (frame_buffer_t  *)msg->param;
	if (capture_name == NULL)
	{
		LOGE("%s  display file is none \n", __func__);
		return BK_FAIL;
	}

#if (CONFIG_FATFS)
	ret = sdcard_read_to_mem((char *)capture_name, (uint32_t *)frame->frame, &frame->length );
#endif

	os_free(capture_name);
	capture_name = NULL;

	resd_storage_node->param = (uint32_t)frame;
	resd_storage_node->event = EVENT_LCD_PICTURE_ECHO_NOTIFY;
	resd_storage_node->result = ret;
	ret = msg_send_rsp_to_media_app_mailbox(resd_storage_node, ret);

	return ret;
}

bk_err_t media_app_lcd_display(void *lcd_display)
{
	bk_err_t ret;
	lcd_display_t *ptr = NULL;
	ptr = (lcd_display_t *)os_malloc(sizeof(lcd_display_t));
	os_memcpy(ptr, (lcd_display_t *)lcd_display, sizeof(lcd_display_t));

	ret = media_send_msg_sync(EVENT_LCD_DISPLAY_IND, (uint32_t)ptr);
	LOGI("%s complete\n", __func__);

	os_free(ptr);
	return ret;
}



bk_err_t media_app_lcd_blend_open(bool en)
{
	bk_err_t ret = 0;
	ret = media_send_msg_sync(EVENT_LCD_BLEND_OPEN_IND, en);

	LOGI("%s complete\n", __func__);
	return ret;
}

bk_err_t media_app_lcd_blend(void *param)
{
	bk_err_t ret;
	lcd_blend_msg_t *ptr = NULL;
	ptr = (lcd_blend_msg_t *)os_malloc(sizeof(lcd_blend_msg_t));
	os_memcpy(ptr, (lcd_blend_msg_t *)param, sizeof(lcd_blend_msg_t));

	ret = media_send_msg_sync(EVENT_LCD_BLEND_IND, (uint32_t)ptr);

	os_free(ptr);
	return ret;
}

bk_err_t media_app_lcd_scale(void)
{
	bk_err_t ret;

	ret = media_send_msg_sync(EVENT_LCD_SCALE_IND, 0);
	return ret;
}

bk_err_t media_app_lcd_close(void)
{
	bk_err_t ret = 0;

	LOGI("%s\n", __func__);

	ret = media_send_msg_sync(EVENT_LCD_CLOSE_IND, 0);

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_OFF);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	LOGI("%s complete\n", __func__);

	return ret;
}


bk_err_t media_app_lcd_set_backlight(uint8_t level)
{
	bk_err_t ret = BK_OK;

	LOGI("%s\n", __func__);

	ret = media_send_msg_sync(EVENT_LCD_SET_BACKLIGHT_IND, level);

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_storage_enable(app_camera_type_t type, uint8_t enable)
{
	int ret = BK_OK;
	uint32_t param = FB_INDEX_JPEG;
	media_mailbox_msg_t msg = {0};

	if (enable)
	{
		if (media_modules_state->stor_state == STORAGE_STATE_ENABLED)
		{
			LOGD("%s, storage have been opened!\r\n", __func__);
			return ret;
		}

#if !CONFIG_TUYA_LOGIC_MODIFY
		bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_VIDP_JPEG_EN, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

		msg.event = EVENT_STORAGE_OPEN_IND;
		ret = storage_app_event_handle(&msg);
		if (ret != BK_OK)
		{
			return ret;
		}

		if (type == APP_CAMERA_DVP_H264_LOCAL)
			param = FB_INDEX_H264;

		ret = media_send_msg_sync(EVENT_STORAGE_OPEN_IND, param);

		if (ret == BK_OK)
		{
			media_modules_state->stor_state = STORAGE_STATE_ENABLED;
		}
	}
	else
	{
		if (media_modules_state->stor_state == STORAGE_STATE_DISABLED)
		{
			LOGI("%s, storage have been closed!\r\n", __func__);
			return ret;
		}

		ret = media_send_msg_sync(EVENT_STORAGE_CLOSE_IND, 0);
		if (ret != BK_OK)
		{
			LOGE("storage_major_task deinit failed\r\n");
		}

		msg.event = EVENT_STORAGE_CLOSE_IND;
		ret = storage_app_event_handle(&msg);

		if (ret == BK_OK)
		{
			media_modules_state->stor_state = STORAGE_STATE_DISABLED;
		}
	}

	return ret;
}

bk_err_t media_app_capture(char *name)
{
	int ret = BK_OK;

	if (name == NULL)
	{
		return ret;
	}

	media_app_storage_enable(app_camera_type, 1);

	ret = storage_app_set_frame_name(name);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = media_send_msg_sync(EVENT_STORAGE_CAPTURE_IND, 0);

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_save_start(char *name)
{
	int ret = BK_OK;

	if (name == NULL)
	{
		return ret;
	}

	media_app_storage_enable(app_camera_type, 1);

	ret = storage_app_set_frame_name(name);
	if (ret != BK_OK)
	{
		return ret;
	}

	ret = media_send_msg_sync(EVENT_STORAGE_SAVE_START_IND, 0);

	LOGI("%s complete\n", __func__);

	return ret;
}


bk_err_t media_app_save_stop(void)
{
	int ret = BK_OK;
	media_mailbox_msg_t msg = {0};

	if (media_modules_state->stor_state == STORAGE_STATE_DISABLED)
	{
		LOGI("%s storage function not init\n", __func__);
		return ret;
	}

	ret = media_send_msg_sync(EVENT_STORAGE_SAVE_STOP_IND, 0);
	if (ret != BK_OK)
	{
		LOGE("storage_major_task stop save video failed\r\n");
	}

	msg.event = EVENT_STORAGE_SAVE_STOP_IND;
	ret = storage_app_event_handle(&msg);

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_dump_display_frame(void)
{
	return media_send_msg_sync(EVENT_LCD_DUMP_DISPLAY_IND, 0);
}

bk_err_t media_app_dump_decoder_frame(void)
{
	return media_send_msg_sync(EVENT_LCD_DUMP_DECODER_IND, 0);
}

bk_err_t media_app_dump_jpeg_frame(void)
{
	return media_send_msg_sync(EVENT_LCD_DUMP_JPEG_IND, 0);
}

bk_err_t media_app_lcd_step_mode(bool enable)
{
	return media_send_msg_sync(EVENT_LCD_STEP_MODE_IND, enable);
}

bk_err_t media_app_lcd_step_trigger(void)
{
	return media_send_msg_sync(EVENT_LCD_STEP_TRIGGER_IND, 0);
}

#if CONFIG_VIDEO_AVI
bk_err_t media_app_avi_open(void)
{
	return media_send_msg_sync(EVENT_AVI_OPEN_IND, 1);
}

bk_err_t media_app_avi_close(void)
{
	return media_send_msg_sync(EVENT_AVI_CLOSE_IND, 0);
}
#endif

bk_err_t media_app_lvgl_open(void *lcd_open)
{
	int ret = BK_OK;
	lcd_open_t *ptr = NULL;

	ptr = (lcd_open_t *)os_malloc(sizeof(lcd_open_t));
	if (ptr == NULL) {
		LOGE("malloc lcd_open_t failed\r\n");
		return BK_ERR_NO_MEM;
	}
	os_memcpy(ptr, (lcd_open_t *)lcd_open, sizeof(lcd_open_t));

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_LVGL, PM_POWER_MODULE_STATE_ON);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	ret = media_send_msg_sync(EVENT_LVGL_OPEN_IND, (uint32_t)ptr);

	if (ptr) {
		os_free(ptr);
		ptr = NULL;
	}

	LOGI("%s complete %x\n", __func__, ret);

	return ret;
}

bk_err_t media_app_lvgl_close(void)
{
	int ret = BK_OK;

	LOGI("%s\n", __func__);

	ret = media_send_msg_sync(EVENT_LVGL_CLOSE_IND, 0);

#if !CONFIG_TUYA_LOGIC_MODIFY
	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_LVGL, PM_POWER_MODULE_STATE_OFF);
#endif // !CONFIG_TUYA_LOGIC_MODIFY

	LOGI("%s complete %x\n", __func__, ret);

	return ret;
}

bk_err_t media_app_lvcam_lvgl_open(void *lcd_open)
{
    bk_err_t ret = 0;
    lcd_open_t *ptr = NULL;

    ptr = (lcd_open_t *)os_malloc(sizeof(lcd_open_t));
    if (ptr == NULL) {
        LOGE("malloc lcd_open_t failed\r\n");
        return BK_ERR_NO_MEM;
    }
    os_memcpy(ptr, (lcd_open_t *)lcd_open, sizeof(lcd_open_t));

    ret =  media_send_msg_sync(EVENT_LVCAM_LVGL_OPEN_IND, (uint32_t)ptr);

    if (ptr) {
        os_free(ptr);
        ptr = NULL;
    }

    LOGI("%s complete\n", __func__);

    return ret;
}

bk_err_t media_app_lvcam_lvgl_close(void)
{
    bk_err_t ret = 0;

    ret =  media_send_msg_sync(EVENT_LVCAM_LVGL_CLOSE_IND, 0);

    LOGI("%s complete\n", __func__);

    return ret;
}

bk_err_t media_app_pipeline_dump(void)
{
	int ret = BK_OK;

	ret = media_send_msg_sync(EVENT_PIPELINE_DUMP_IND, 0);

	LOGI("%s complete %x\n", __func__, ret);

	return ret;
}

bk_err_t media_app_frame_buffer_init(fb_type_t type)
{
	int ret = BK_FAIL;

	if (CAMERA_STATE_DISABLED == media_modules_state->cam_state)
	{
		LOGI("%s camera not opened\n", __func__);
		return ret;
	}

	ret = media_send_msg_sync(EVENT_FRAME_BUFFER_INIT_IND, (uint32_t)type);

	if (ret != BK_OK)
	{
		LOGE("%s, malloc fail\r\n", __func__);
	}

	return ret;
}

frame_buffer_t *media_app_frame_buffer_jpeg_malloc(void)
{
	int ret = BK_FAIL;
	frame_buffer_t *frame = NULL;

	if (CAMERA_STATE_DISABLED == media_modules_state->cam_state)
	{
		LOGI("%s camera not opened!\n", __func__);
		return frame;
	}

	ret = media_send_msg_sync(EVENT_FRAME_BUFFER_JPEG_MALLOC_IND, (uint32_t)&frame);

	if (ret != BK_OK)
	{
		LOGE("%s, malloc fail\r\n", __func__);
	}

	LOGD("%s, %p\r\n", __func__, frame);

	if (frame)
		frame->fmt = PIXEL_FMT_JPEG;

	return frame;
}

frame_buffer_t *media_app_frame_buffer_h264_malloc(void)
{
	int ret = BK_FAIL;
	frame_buffer_t *frame = NULL;

	if (CAMERA_STATE_DISABLED == media_modules_state->cam_state)
	{
		LOGI("%s camera not opened!\n", __func__);
		return frame;
	}

	ret = media_send_msg_sync(EVENT_FRAME_BUFFER_H264_MALLOC_IND, (uint32_t)&frame);

	if (ret != BK_OK)
	{
		LOGE("%s, malloc fail\r\n", __func__);
	}

	if (frame)
		frame->fmt = PIXEL_FMT_H264;

	return frame;
}

bk_err_t media_app_frame_buffer_push(frame_buffer_t *frame)
{
	int ret = BK_FAIL;

	if (CAMERA_STATE_DISABLED == media_modules_state->cam_state)
	{
		LOGI("%s camera not opened!\n", __func__);
		return ret;
	}

	ret = media_send_msg_sync(EVENT_FRAME_BUFFER_PUSH_IND, (uint32_t)frame);

	return ret;
}

bk_err_t media_app_frame_buffer_clear(frame_buffer_t *frame)
{
	int ret = BK_FAIL;

	if (CAMERA_STATE_DISABLED == media_modules_state->cam_state)
	{
		LOGI("%s camera not opened!\n", __func__);
		return ret;
	}

	ret = media_send_msg_sync(EVENT_FRAME_BUFFER_FREE_IND, (uint32_t)frame);

	return ret;
}



#if CONFIG_MEDIA_UNIT_TEST
bk_err_t media_app_pipeline_mem_show(void)
{
	int ret = BK_OK;

	ret = media_send_msg_sync(EVENT_PIPELINE_MEM_SHOW_IND, 0);

	LOGI("%s complete %x\n", __func__, ret);

	return ret;
}

bk_err_t media_app_pipeline_mem_leak(void)
{
	int ret = BK_OK;

	ret = media_send_msg_sync(EVENT_PIPELINE_MEM_LEAK_IND, 0);

	LOGI("%s complete %x\n", __func__, ret);

	return ret;
}
#endif

void media_app_save_yuv_data_handle(uint32_t param)
{
	int ret = BK_OK;
	media_mailbox_msg_t *msg = (media_mailbox_msg_t *)param;

	frame_buffer_t *yuv = (frame_buffer_t *)msg->param;
	char file_name[20] = {0};

	if (yuv != NULL)
	{
		os_snprintf(file_name, 20, "%d.yuv", yuv->sequence);

		storage_mem_to_sdcard(file_name, yuv->frame, yuv->size);
	}

	msg_send_rsp_to_media_app_mailbox(msg, ret);
}

bk_err_t media_app_send_msg(media_msg_t *msg)
{
	bk_err_t ret;

	if (media_app_msg_queue)
	{
		ret = rtos_push_to_queue(&media_app_msg_queue, msg, BEKEN_NO_WAIT);

		if (kNoErr != ret)
		{
			LOGE("%s failed\n", __func__);
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

static void media_app_message_handle(void)
{
	bk_err_t ret = BK_OK;
	media_msg_t msg;

	while (1)
	{
		ret = rtos_pop_from_queue(&media_app_msg_queue, &msg, BEKEN_WAIT_FOREVER);

		if (BK_OK == ret)
		{
			switch (msg.event)
			{
				case EVENT_UVC_DEVICE_INFO_NOTIFY:
					if (uvc_device_info_cb)
					{
						if (msg.param != 0)
							uvc_device_info_cb((bk_uvc_device_brief_info_t *)msg.param, UVC_CONNECTED);
						else
							uvc_device_info_cb((bk_uvc_device_brief_info_t *)msg.param, UVC_DISCONNECT_ABNORMAL);
					}
					break;

				case EVENT_JPEG_DEC_SAVE_IND:
					media_app_save_yuv_data_handle(msg.param);
					break;

				case EVENT_MEDIA_APP_EXIT_IND:
					goto exit;
					break;

				default:
					break;
			}
		}
	}

exit:

	/* delate msg queue */
	ret = rtos_deinit_queue(&media_app_msg_queue);

	if (ret != BK_OK)
	{
		LOGE("delate message queue fail\n");
	}

	media_app_msg_queue = NULL;

	LOGE("delate message queue complete\n");

	/* delate task */
	media_app_th_hd = NULL;
	rtos_delete_thread(NULL);

}

bk_err_t media_app_init(void)
{
	bk_err_t ret = BK_OK;

	if (media_app_msg_queue != NULL)
	{
		LOGE("%s, media_app_msg_queue already init, exit!\n", __func__);
		goto error;
	}

	if (media_app_th_hd != NULL)
	{
		LOGE("%s, media_app_th_hd already init, exit!\n", __func__);
		goto error;
	}

	if (media_modules_state == NULL)
	{
		media_modules_state = (media_modules_state_t *)os_malloc(sizeof(media_modules_state_t));
		if (media_modules_state == NULL)
		{
			LOGE("%s, media_modules_state malloc failed!\n", __func__);
			return BK_ERR_NO_MEM;
		}
	}

	media_modules_state->aud_state = AUDIO_STATE_DISABLED;
	media_modules_state->cam_state = CAMERA_STATE_DISABLED;
	media_modules_state->lcd_state = LCD_STATE_DISABLED;
	media_modules_state->stor_state = STORAGE_STATE_DISABLED;
	media_modules_state->trs_state = TRS_STATE_DISABLED;

	ret = rtos_init_queue(&media_app_msg_queue,
	                      "media_app_msg_queue",
	                      sizeof(media_msg_t),
	                      20);

	if (ret != kNoErr)
	{
		LOGE("%s, create media minor message queue failed\n", __func__);
		goto error;
	}

	ret = rtos_create_thread(&media_app_th_hd,
	                         BEKEN_DEFAULT_WORKER_PRIORITY,
	                         "media_app_thread",
	                         (beken_thread_function_t)media_app_message_handle,
	                         2048,
	                         NULL);

	if (ret != kNoErr)
	{
		LOGE("create media app thread fail\n");
		goto error;
	}

	LOGI("media app thread startup complete\n");

	return ret;

error:

	if (media_app_msg_queue)
	{
		rtos_deinit_queue(&media_app_msg_queue);
		media_app_msg_queue = NULL;
	}

	return ret;

}


