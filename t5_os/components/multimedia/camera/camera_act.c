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
#include <components/log.h>

#include "media_core.h"
#include "camera_act.h"
#include "media_evt.h"
#include <driver/int.h>
#include <os/mem.h>

#include "camera.h"
#include "frame_buffer.h"


#include <driver/dvp_camera.h>
#include <driver/uvc_camera_types.h>
#include <driver/dvp_camera_types.h>
#include <driver/video_common_driver.h>
#include <driver/uvc_camera.h>
#include <driver/h264.h>

#define TAG "cam_act"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef void (*camera_connect_state_t)(uint8_t state);
extern bk_err_t bk_dvp_camera_init(void *data);
extern bk_err_t bk_dvp_camera_deinit();


camera_info_t camera_info = {0};
bool dvp_camera_reset_open_ind = false;
media_mailbox_msg_t camera_media_msg = {0};

void camera_uvc_device_info_notify_to_cp0(bk_uvc_device_brief_info_t *info, uvc_state_t state)
{
	camera_media_msg.event = EVENT_UVC_DEVICE_INFO_NOTIFY;
	camera_media_msg.param = (uint32_t)info;
	camera_media_msg.result = state;
	msg_send_notify_to_media_major_mailbox(&camera_media_msg, APP_MODULE);
}

static bk_err_t camera_dvp_reset_open_handle(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_OK;

#ifdef CONFIG_DVP_CAMERA

	if (dvp_camera_reset_open_ind)
	{
		bk_dvp_camera_close();
		dvp_camera_reset_open_ind = false;
	}

	ret = bk_dvp_camera_open(camera_info.device);
	if (ret != BK_OK)
	{
		dvp_camera_reset_open_ind = false;
	}
	else
	{
		dvp_camera_reset_open_ind = true;
	}
#endif

	os_free(msg);

	return ret;
}

static bk_err_t camera_dvp_open_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;

	LOGI("%s\n", __func__);
#ifdef CONFIG_DVP_CAMERA

	if (CAMERA_STATE_DISABLED != get_camera_state())
	{
		LOGI("%s already opened\n", __func__);
		goto end;
	}

	os_memcpy(camera_info.device, (media_camera_device_t *)msg->param, sizeof(media_camera_device_t));

	LOGI("%s, %d\r\n", __func__, __LINE__);

	ret = bk_dvp_camera_open(camera_info.device);

	if (ret != BK_OK)
	{
		LOGE("%s open failed\n", __func__);
		goto end;
	}

	//dvp_camera_reset_open_ind = true;

	set_camera_state(CAMERA_STATE_ENABLED);
#else
	LOGW("%s NOT SUPPORT DVP_CAMERA\n", __func__);
	goto end;
#endif

end:
	msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

	return ret;
}

static bk_err_t camera_dvp_close_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;

	LOGI("%s\n", __func__);

#ifdef CONFIG_DVP_CAMERA

	if (CAMERA_STATE_DISABLED == get_camera_state())
	{
		LOGI("%s already close\n", __func__);
		goto end;
	}

	bk_dvp_camera_close();

	set_camera_state(CAMERA_STATE_DISABLED);

	dvp_camera_reset_open_ind = false;
#else
	LOGW("%s NOT SUPPORT DVP_CAMERA\n", __func__);
	goto end;
#endif

end:
	msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

	return ret;
}

static bk_err_t camera_dvp_h264_reset_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_FAIL;

#ifdef CONFIG_DVP_CAMERA

	if (CAMERA_STATE_DISABLED == get_camera_state())
	{
		LOGI("%s already close\n", __func__);
		msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

		return ret;
	}

	ret = bk_dvp_camera_h264_regenerate_idr_frame();

#endif

	LOGI("%s complete\n", __func__);

	msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

	return ret;
}

bk_err_t camera_uvc_open_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;

	LOGI("%s\n", __func__);

#if CONFIG_USB_UVC

	if (CAMERA_STATE_DISABLED != get_camera_state())
	{
		LOGI("%s already opened\n", __func__);
		goto end;
	}

	os_memcpy(camera_info.device, (media_camera_device_t *)msg->param, sizeof(media_camera_device_t));

	ret = bk_uvc_camera_open(camera_info.device);

	if (ret != BK_OK)
	{
		LOGE("%s open failed\n", __func__);
	}
	else
	{
		set_camera_state(CAMERA_STATE_ENABLED);
	}
#else
	LOGW("%s NOT SUPPORT UVC_CAMERA\n", __func__);
	goto end;
#endif

end:
	msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

	return ret;
}

bk_err_t camera_uvc_close_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;

	LOGI("%s\n", __func__);

#ifdef CONFIG_USB_UVC

	if (CAMERA_STATE_DISABLED == get_camera_state())
	{
		LOGI("%s already close\n", __func__);
		goto end;
	}

	bk_uvc_camera_close();

	set_camera_state(CAMERA_STATE_DISABLED);

	LOGI("%s complete!\n", __func__);
#else
	LOGW("%s NOT SUPPORT UVC_CAMERA\n", __func__);
	goto end;
#endif

end:
	msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

	return ret;
}

bk_err_t camera_uvc_reset_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;
	LOGI("%s\n", __func__);

	msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

	LOGI("%s, complete!\n", __func__);

	return ret;
}

bk_err_t camera_net_open_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;

	LOGI("%s NOT SUPPORT\n", __func__);

#if 0//CONFIG_WIFI_TRANSFER

	if (CAMERA_STATE_DISABLED != get_camera_state())
	{
		LOGI("%s already opened\n", __func__);
		goto end;
	}

	os_memcpy(camera_info.device, (media_camera_device_t *)msg->param, sizeof(media_camera_device_t));

	ret = bk_net_camera_open(camera_info.device);

	if (ret != BK_OK)
	{
		LOGE("%s open failed\n", __func__);
		goto end;
	}

	set_camera_state(CAMERA_STATE_ENABLED);

end:
#endif
	msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

	return ret;
}

bk_err_t camera_net_close_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;

	LOGI("%s NOT SUPPORT\n", __func__);

#if 0//CONFIG_WIFI_TRANSFER

	if (CAMERA_STATE_DISABLED == get_camera_state())
	{
		LOGI("%s already close\n", __func__);
		goto end;
	}

	bk_net_camera_close();

	set_camera_state(CAMERA_STATE_DISABLED);

end:
#endif

	msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

	return ret;
}

static bk_err_t camera_rtsp_open_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;

	LOGI("%s\n", __func__);
#ifdef CONFIG_DVP_CAMERA

	if (CAMERA_STATE_DISABLED != get_camera_state())
	{
		LOGI("%s already opened\n", __func__);
		goto end;
	}

	LOGI("%s, %d\r\n", __func__, __LINE__);

	ret = bk_dvp_camera_init(msg);

	if (ret != BK_OK)
	{
		LOGE("%s open failed\n", __func__);
		goto end;
	}

	//dvp_camera_reset_open_ind = true;

	set_camera_state(CAMERA_STATE_ENABLED);
#else
	LOGW("%s NOT SUPPORT DVP_CAMERA\n", __func__);
	goto end;
#endif

end:
	msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

	return ret;
}

static bk_err_t camera_rtsp_close_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;

	LOGI("%s\n", __func__);

#ifdef CONFIG_DVP_CAMERA

	if (CAMERA_STATE_DISABLED == get_camera_state())
	{
		LOGI("%s already close\n", __func__);
		goto end;
	}

	bk_dvp_camera_deinit();

	set_camera_state(CAMERA_STATE_DISABLED);

	dvp_camera_reset_open_ind = false;

#else
	LOGW("%s NOT SUPPORT DVP_CAMERA\n", __func__);
	goto end;
#endif

end:
	msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

	return ret;
}

static bk_err_t camera_compression_ratio_config_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_FAIL;
	compress_ratio_t *ratio = (compress_ratio_t *)msg->param;

	LOGI("%s\n", __func__);

	ret = bk_video_compression_ratio_config(ratio);

	msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

	return ret;
}

static bk_err_t camera_uvc_register_device_info_cb_handle(media_mailbox_msg_t *msg)
{
	bk_uvc_camera_register_info_callback(camera_uvc_device_info_notify_to_cp0);

	msg_send_rsp_to_media_major_mailbox(msg, BK_OK, APP_MODULE);

	return BK_OK;
}

static bk_err_t camera_set_uvc_param_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;

	if (CAMERA_STATE_DISABLED == get_camera_state())
	{
		LOGI("%s uvc not open\n", __func__);
		goto end;
	}

	ret = bk_uvc_camera_set_config((bk_uvc_config_t *)msg->param);

end:
	msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

	return ret;
}

static bk_err_t camera_get_h264_encode_param_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;

	h264_base_config_t base_config = {0};

	ret = bk_h264_get_h264_base_config(&base_config);

	os_memcpy((h264_base_config_t *)msg->param, &base_config, sizeof(h264_base_config_t));

	msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);

	return ret;
}


bk_err_t camera_event_handle(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_FAIL;
	switch (msg->event)
	{
		case EVENT_CAM_DVP_OPEN_IND:
			ret = camera_dvp_open_handle(msg);
			break;

		case EVENT_CAM_DVP_CLOSE_IND:
			ret = camera_dvp_close_handle(msg);
			break;

		case EVENT_CAM_DVP_RESET_OPEN_IND:
			ret = camera_dvp_reset_open_handle(msg);
			break;

		case EVENT_CAM_H264_RESET_IND:
			ret = camera_dvp_h264_reset_handle(msg);
			break;

		case EVENT_CAM_UVC_OPEN_IND:
			ret = camera_uvc_open_handle(msg);
			break;

		case EVENT_CAM_UVC_CLOSE_IND:
			ret = camera_uvc_close_handle(msg);
			break;

		case EVENT_CAM_UVC_RESET_IND:
			ret = camera_uvc_reset_handle(msg);
			break;

		case EVENT_CAM_NET_OPEN_IND:
			camera_net_open_handle(msg);
			break;

		case EVENT_CAM_NET_CLOSE_IND:
			camera_net_close_handle(msg);
			break;

		case EVENT_CAM_RTSP_OPEN_IND:
			camera_rtsp_open_handle(msg);
			break;

		case EVENT_CAM_RTSP_CLOSE_IND:
			camera_rtsp_close_handle(msg);
			break;

		case EVENT_CAM_COMPRESS_IND:
			camera_compression_ratio_config_handle(msg);
			break;

		case EVENT_CAM_REG_UVC_INFO_CB_IND:
			camera_uvc_register_device_info_cb_handle(msg);
			break;

		case EVENT_CAM_SET_UVC_PARAM_IND:
			camera_set_uvc_param_handle(msg);
			break;

		case EVENT_CAM_GET_H264_INFO_IND:
			camera_get_h264_encode_param_handle(msg);
			break;

		default:
			break;
	}

	return ret;
}

media_camera_state_t get_camera_state(void)
{
	return camera_info.state;
}

void set_camera_state(media_camera_state_t state)
{
	camera_info.state = state;
}

void camera_init(void)
{
	camera_info.state = CAMERA_STATE_DISABLED;
	camera_info.debug = true;
	camera_info.device = (media_camera_device_t *)os_malloc(sizeof(media_camera_device_t));

	BK_ASSERT(camera_info.device != NULL);
}
