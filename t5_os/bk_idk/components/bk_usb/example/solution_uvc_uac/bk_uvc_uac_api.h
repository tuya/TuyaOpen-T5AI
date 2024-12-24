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

#pragma once

#include <components/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UVC_UAC_TAG "UVC_UAC"
#define UVC_UAC_LOGI(...) BK_LOGI(UVC_UAC_TAG, ##__VA_ARGS__)
#define UVC_UAC_LOGW(...) BK_LOGW(UVC_UAC_TAG, ##__VA_ARGS__)
#define UVC_UAC_LOGE(...) BK_LOGE(UVC_UAC_TAG, ##__VA_ARGS__)
#define UVC_UAC_LOGD(...) BK_LOGD(UVC_UAC_TAG, ##__VA_ARGS__)


#define UVC_MAX_FPS 30
#define UVC_CALCULATE_FRAME_RATE_VALUE_NUM 10000000

bk_err_t bk_usb_uvc_uac_sw_init(void *set_config);
bk_err_t bk_usb_uvc_uac_sw_deinit();

#if CONFIG_CHERRY_USB
#define UVC_UAC_TRIGGER_ERROR_COUNT 5
#define UVC_UAC_TRANSFER_IRQ_ERROR_COUNT 5

void bk_usb_uvc_uac_free_enumerate_resources();
void bk_usb_updata_video_interface(void *hport, uint8_t bInterfaceNumber, uint8_t interface_sub_class);

void bk_usbh_video_start_handle(struct usbh_video *uvc_device);
void bk_usbh_video_start_dual_handle(struct usbh_video *uvc_device);

bk_err_t bk_uvc_trigger_video_stream_rx(void *uvc_device);
bk_err_t bk_uvc_trigger_video_stream_dual_rx(void *uvc_device);

void bk_usbh_video_stop_handle(struct usbh_video *uvc_device);

void bk_usb_updata_audio_interface(void *hport, uint8_t bInterfaceNumber, uint8_t interface_sub_class);

void bk_uac_start_mic_handle(struct usbh_audio *uac_device);
bk_err_t bk_uac_trigger_audio_stream_rx(void *uac_device);
void bk_uac_stop_mic_handle(struct usbh_audio *uac_device);

void bk_uac_start_speaker_handle(struct usbh_audio *uac_device);
bk_err_t bk_uac_trigger_audio_stream_tx(void *uac_device);
void bk_uac_stop_speaker_handle(struct usbh_audio *uac_device);

#endif

#ifdef __cplusplus
}
#endif
