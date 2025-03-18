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

#include "camera_act.h"
#include "media_evt.h"
#include "media_app.h"

#include "yuv_encode.h"

#include <os/mem.h>

#include <driver/uvc_camera.h>

#include "frame_buffer.h"

#define TAG "uvc"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#if CONFIG_USB_UVC

static bool uvc_state_flag = false;
static uvc_camera_config_t *uvc_camera_config_st = NULL;


void uvc_device_connect_state_callback(uvc_state_t state)
{
	LOGI("%s, %d, %d\n", __func__, state, uvc_state_flag);

    if (state == UVC_DISCONNECT_ABNORMAL)
    {
#if CONFIG_MEDIA_PIPELINE
        jpeg_decode_restart();
#endif
    }
}

bk_err_t bk_uvc_camera_open(media_camera_device_t *device)
{
	int ret = BK_FAIL;

	if (uvc_camera_config_st == NULL)
	{
		uvc_camera_config_st = (uvc_camera_config_t *)os_malloc(sizeof(uvc_camera_config_t));
		if (uvc_camera_config_st == NULL)
		{
			LOGE("uvc_camera_config_st malloc failed\r\n");
			ret = kNoMemoryErr;
			return ret;
		}

		os_memset(uvc_camera_config_st, 0, sizeof(uvc_camera_config_t));
	}
// Modified by TUYA Start
#if CONFIG_TUYA_LOGIC_MODIFY
extern int tkl_vi_set_power_info(uint8_t io, uint8_t active);
    tkl_vi_set_power_info(device->ty_param[0], device->ty_param[1]);
#endif // CONFIG_TUYA_LOGIC_MODIFY
// Modified by TUYA End

	// first set uvc_stat_flag to ture, to avoid confusion caused by abnormal disconnection during the opening of uvc
	uvc_state_flag = true;
	os_memcpy(&uvc_camera_config_st->device, device, sizeof(media_camera_device_t));

	uvc_camera_config_st->jpeg_cb.init   = frame_buffer_fb_init;
	uvc_camera_config_st->jpeg_cb.deinit = frame_buffer_fb_deinit;
	uvc_camera_config_st->jpeg_cb.clear  = frame_buffer_fb_clear;
	uvc_camera_config_st->jpeg_cb.malloc = frame_buffer_fb_malloc;
	uvc_camera_config_st->jpeg_cb.push   = frame_buffer_fb_push;
	uvc_camera_config_st->jpeg_cb.pop    = NULL;
	uvc_camera_config_st->jpeg_cb.free   = frame_buffer_fb_direct_free;

    if (device->num_uvc_dev > 1)
    {
    	uvc_camera_config_st->h264_cb.init   = frame_buffer_fb_init;
    	uvc_camera_config_st->h264_cb.deinit = frame_buffer_fb_deinit;
    	uvc_camera_config_st->h264_cb.clear  = frame_buffer_fb_clear;
        uvc_camera_config_st->h264_cb.malloc = frame_buffer_fb_dual_malloc;
    	uvc_camera_config_st->h264_cb.push   = frame_buffer_fb_push;
    	uvc_camera_config_st->h264_cb.pop    = NULL;
    	uvc_camera_config_st->h264_cb.free   = frame_buffer_fb_direct_free;
    }

	uvc_camera_config_st->uvc_connect_state_change_cb = uvc_device_connect_state_callback;
	uvc_camera_config_st->user_cb = NULL;

	ret = bk_uvc_camera_driver_init(uvc_camera_config_st);

	// only judge the state of api calling
	if (ret != BK_OK)
	{
		LOGE("%s failed\r\n", __func__);
		uvc_state_flag = false;
	}

	LOGI("%s, complete, %d\r\n", __func__, uvc_state_flag);

	return ret;
}

bk_err_t bk_uvc_camera_close(void)
{
	if (uvc_state_flag)
	{
		uvc_state_flag = false;

		bk_uvc_camera_driver_deinit();

		if (uvc_camera_config_st)
		{
			os_free(uvc_camera_config_st);
			uvc_camera_config_st = NULL;
		}
	}

	return 0;
}

#endif
