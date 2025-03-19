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
#include "lcd_act.h"
#include "storage_act.h"
#include "media_evt.h"

#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>


#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>

#include "frame_buffer.h"

#define TAG "dvp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static media_mailbox_msg_t *rotate_msg = NULL;


bk_err_t bk_dvp_camera_frame_ops_cb(uint32_t param)
{
	int ret = BK_FAIL;

	if (rotate_msg)
	{
		rotate_msg->event = EVENT_YUV_ROTATE_NOTIFY;
		rotate_msg->param = (uint32_t)param;
		ret = msg_send_notify_to_media_major_mailbox(rotate_msg, MINOR_MODULE);
	}

	return ret;
}

bk_err_t bk_dvp_camera_open(media_camera_device_t *device)
{
	int ret = BK_OK;
	dvp_camera_config_t config = {0};

	config.fb_init = frame_buffer_fb_init;
	config.fb_deinit = frame_buffer_fb_deinit;
	config.fb_clear = frame_buffer_fb_clear;
	config.fb_malloc = frame_buffer_fb_malloc;
	config.fb_complete = frame_buffer_fb_push;
	config.fb_free = frame_buffer_fb_direct_free;
	config.frame_ops = bk_dvp_camera_frame_ops_cb;

	config.device = device;

	if (device->rot_angle != ROTATE_NONE)
	{
		rotate_msg = (media_mailbox_msg_t *)os_malloc(sizeof(media_mailbox_msg_t));
		if (rotate_msg == NULL)
		{
			LOGE("%s, malloc fail\n", __func__);
			ret = BK_ERR_NO_MEM;
			return ret;
		}
	}

// Modified by TUYA Start
#if CONFIG_TUYA_LOGIC_MODIFY
    extern int tkl_vi_set_dvp_i2c_pin(uint8_t clk, uint8_t sda);
    extern int tkl_vi_set_power_info(uint8_t device_type, uint8_t io, uint8_t active);
    tkl_vi_set_power_info(DVP_CAMERA, device->ty_param[2], device->ty_param[3]);
    tkl_vi_set_dvp_i2c_pin(device->ty_param[4], device->ty_param[5]);
#endif // CONFIG_TUYA_LOGIC_MODIFY
// Modified by TUYA End

	ret = bk_dvp_camera_driver_init(&config);

	return ret;
}

bk_err_t bk_dvp_camera_close(void)
{
	bk_dvp_camera_driver_deinit();

	if (rotate_msg)
	{
		os_free(rotate_msg);
		rotate_msg = NULL;
	}

	return 0;
}
