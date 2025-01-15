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

#include <common/bk_include.h>
#include "media_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MEDIA_EVT_RETURN(res, val) \
	do { \
		if (res != NULL) { \
			res->ret = val; \
			rtos_set_semaphore(&res->sem); \
		} \
	} while(0)


/*
*   Message Type:
*       CMD <-> EVT     for     CPU0 <-> CPU1
*       REQ <-> EVT     for     modules
*       IND             for     unicast instruction
*    _______________________________________                   _______________________________________
*   |                                       |                 |                                       |
*   |             Major CPU(CPU0)           |                 |             Minor CPU(CPU1)           |
*   |_______________________________________|                 |_______________________________________|
*   |                   |                   |     CMD1 ==>    |                   |                   |
*   |    Moudle A       |    Moudle B       |     <== EVT1    |    Moudle C       |    Moudle D       |
*   |  _____________    |    _____________  |                 |  _____________    |    _____________  |
*   | |             |   |   |             | |     CMD2 ==>    | |             |   |   |             | |
*   | | EVENT_REQ1  | <-|-> | EVENT_RES1  | |     <== EVT2    | | EVENT_REQ1  | <-|-> | EVENT_RES1  | |
*   | | EVENT_REQ2  | <-|-> | EVENT_RES2  | |                 | | EVENT_REQ2  | <-|-> | EVENT_RES2  | |
*   | | EVENT_RES1  | <-|-> | EVENT_REQ1  | |     CMD3 ==>    | | EVENT_RES1  | <-|-> | EVENT_REQ1  | |
*   | | EVENT_RES2  | <-|-> | EVENT_REQ2  | |     <== EVT3    | | EVENT_RES2  | <-|-> | EVENT_REQ2  | |
*   | |_____________|   |   |_____________| |                 | |_____________|   |   |_____________| |
*   |___________________|___________________|                 |___________________|___________________|
*
*/


typedef enum
{
	COM_EVENT = 1,
	CAM_EVENT,
	AUD_EVENT,
	LCD_EVENT,
	LVGL_EVENT,
	STORAGE_EVENT,
	TRS_EVENT,
	USB_TRS_EVENT,
	UVC_PIPELINE_EVENT,
	EXIT_EVENT,
	QUEUE_EVENT,
	MAILBOX_CMD,
	MAILBOX_EVT,
	MAILBOX_REQ,
	MAILBOX_RSP,
	MAILBOX_NOTIFY,
	AUD_NOTIFY,
	BT_EVENT,
	MAJOR_COMM_EVENT,
	FRAME_BUFFER_EVENT,
} media_mask_t;

typedef enum
{
	EVENT_COM_DEFAULT_IND = (COM_EVENT << MEDIA_EVT_BIT),
	EVENT_COM_FRAME_WIFI_FREE_IND,
	EVENT_COM_FRAME_DECODER_FREE_IND,
	EVENT_COM_FRAME_CAPTURE_FREE_IND,
	EVENT_COM_FRAME_REQUEST_IND,


	EVENT_AUD_INIT_REQ = (AUD_EVENT << MEDIA_EVT_BIT),
	EVENT_AUD_DEINIT_REQ,		/**< deinit aud tras drv */
	EVENT_AUD_SET_MODE_REQ,		/**< set work mode */
	/* mic op */
	EVENT_AUD_MIC_INIT_REQ,		/**< init mic */
	EVENT_AUD_MIC_DEINIT_REQ,	/**< deinit mic */
	EVENT_AUD_MIC_START_REQ, 	/**< start mic */
	EVENT_AUD_MIC_PAUSE_REQ, 	/**< pause mic */
	EVENT_AUD_MIC_STOP_REQ,		/**< stop mic */
	EVENT_AUD_MIC_SET_CHL_REQ,	/**< set mic channel */
	EVENT_AUD_MIC_SET_SAMP_RATE_REQ, 	/**< set adc sample rate */
	EVENT_AUD_MIC_SET_GAIN_REQ,			/**< set adc gain */
	/* spk op */
	EVENT_AUD_SPK_INIT_REQ,		/**< init spk */
	EVENT_AUD_SPK_DEINIT_REQ,	/**< deinit spk */
	EVENT_AUD_SPK_START_REQ, 	/**< start spk */
	EVENT_AUD_SPK_PAUSE_REQ, 	/**< pause spk */
	EVENT_AUD_SPK_STOP_REQ,		/**< stop spk */
	EVENT_AUD_SPK_SET_CHL_REQ,	/**< set speaker channel */
	EVENT_AUD_SPK_SET_SAMP_RATE_REQ, 	/**< set dac sample rate */
	EVENT_AUD_SPK_SET_GAIN_REQ,			/**< set dac gain */
	/* voc op */
	EVENT_AUD_VOC_INIT_REQ,		/**< init voc */
	EVENT_AUD_VOC_DEINIT_REQ,	/**< deinit voc */
	EVENT_AUD_VOC_START_REQ, 	/**< start voc */
	EVENT_AUD_VOC_STOP_REQ,		/**< stop voc */
	EVENT_AUD_VOC_CTRL_MIC_REQ,	/**< set voc mic enable */
	EVENT_AUD_VOC_CTRL_SPK_REQ,	/**< set voc spk enable */
	EVENT_AUD_VOC_CTRL_AEC_REQ,	/**< set voc aec enable */
	EVENT_AUD_VOC_SET_MIC_GAIN_REQ,		/**< set audio adc gain */
	EVENT_AUD_VOC_SET_SPK_GAIN_REQ,		/**< set audio dac gain */
	EVENT_AUD_VOC_SET_AEC_PARA_REQ,		/**< set AEC parameters */
	EVENT_AUD_VOC_GET_AEC_PARA_REQ,		/**< get AEC parameters */
	EVENT_AUD_VOC_TX_DEBUG_REQ,			/**< dump tx data */
	EVENT_AUD_VOC_RX_DEBUG_REQ,			/**< dump rx data */
	EVENT_AUD_VOC_AEC_DEBUG_REQ, 		/**< dump aec data */
	/* UAC op */
	EVENT_AUD_UAC_REGIS_CONT_STATE_CB_REQ,		/**< register uac mic and speaker connect state callback */
	EVENT_AUD_UAC_CONT_REQ,						/**< recover uac status after uac automatically connect */
	EVENT_AUD_UAC_DISCONT_REQ,					/**< uac abnormal disconnect */
	EVENT_AUD_UAC_AUTO_CONT_CTRL_REQ,			/**< uac automatically connect enable control*/

	EVENT_CAM_DVP_OPEN_IND = (CAM_EVENT << MEDIA_EVT_BIT),
	EVENT_CAM_DVP_CLOSE_IND,
	EVENT_CAM_DVP_RESET_OPEN_IND,
	EVENT_CAM_UVC_OPEN_IND,
	EVENT_CAM_UVC_CLOSE_IND,
	EVENT_CAM_UVC_RESET_IND,
	EVENT_CAM_NET_OPEN_IND,
	EVENT_CAM_NET_CLOSE_IND,
	EVENT_CAM_RTSP_OPEN_IND,
	EVENT_CAM_RTSP_CLOSE_IND,
	EVENT_CAM_COMPRESS_IND,
	EVENT_CAM_SET_UVC_PARAM_IND,
	EVENT_CAM_REG_UVC_INFO_CB_IND,
	EVENT_CAM_GET_H264_INFO_IND,
	EVENT_CAM_H264_RESET_IND,

	EVENT_LCD_OPEN_IND = (LCD_EVENT << MEDIA_EVT_BIT),
	EVENT_LCD_DVP_REG_CAM_INIT_RES,
	EVENT_LCD_FRAME_COMPLETE_IND,
	EVENT_LCD_RESIZE_IND,
	EVENT_LCD_CLOSE_IND,
	EVENT_LCD_SET_BACKLIGHT_IND,
	EVENT_LCD_ROTATE_ENABLE_IND,
	EVENT_LCD_FRAME_LOCAL_ROTATE_IND,
	EVENT_LCD_DUMP_DECODER_IND,
	EVENT_LCD_DUMP_JPEG_IND,
	EVENT_LCD_DUMP_DISPLAY_IND,
	EVENT_LCD_STEP_MODE_IND,
	EVENT_LCD_STEP_TRIGGER_IND,
	EVENT_LCD_DISPLAY_IND,
	EVENT_LCD_BEKEN_LOGO_DISPLAY,
	EVENT_LCD_DISPLAY_FILE_IND,
	EVENT_LCD_BLEND_IND,
	EVENT_LCD_DECODE_MODE_IND,
	EVENT_LCD_BLEND_OPEN_IND,
	EVENT_LCD_GET_DEVICES_NUM_IND,
	EVENT_LCD_GET_DEVICES_LIST_IND,
	EVENT_LCD_GET_DEVICES_IND,
	EVENT_LCD_SCALE_IND,
	EVENT_LCD_GET_STATUS_IND,
	EVENT_GET_UVC_STATUS_IND,
	EVENT_JPEG_DEC_SAVE_IND,
    // Modified by TUYA Start
    EVENT_GET_USB_STATUS_IND,
    // Modified by TUYA End

	EVENT_PIPELINE_LCD_DISP_OPEN_IND = (UVC_PIPELINE_EVENT << MEDIA_EVT_BIT),
	EVENT_PIPELINE_LCD_DISP_CLOSE_IND,
	EVENT_PIPELINE_LCD_JDEC_OPEN_IND,
	EVENT_PIPELINE_LCD_JDEC_CLOSE_IND,
	EVENT_PIPELINE_SET_ROTATE_IND,
	EVENT_PIPELINE_H264_OPEN_IND,
	EVENT_PIPELINE_H264_CLOSE_IND,
	EVENT_PIPELINE_H264_RESET_IND,
	EVENT_LCD_SET_FMT_IND,
	EVENT_PIPELINE_LCD_SCALE_IND,
	EVENT_PIPELINE_MEM_SHOW_IND,
	EVENT_PIPELINE_MEM_LEAK_IND,
	EVENT_PIPELINE_DUMP_IND,
	//Modified by TUYA Start
	EVENT_PIPELINE_STARTUP_OPEN_IND,
	EVENT_PIPELINE_STARTUP_CLOSE_IND,
	//Modified by TUYA End

	EVENT_MEDIA_APP_EXIT_IND = (EXIT_EVENT << MEDIA_EVT_BIT),

	EVENT_LVGL_OPEN_IND = (LVGL_EVENT << MEDIA_EVT_BIT),
	EVENT_LVGL_CLOSE_IND,
	EVENT_LVCAM_LVGL_OPEN_IND,
	EVENT_LVCAM_LVGL_CLOSE_IND,

	EVENT_STORAGE_OPEN_IND = (STORAGE_EVENT << MEDIA_EVT_BIT),
	EVENT_STORAGE_CLOSE_IND,
	EVENT_STORAGE_CAPTURE_IND,
	EVENT_STORAGE_SAVE_START_IND,
	EVENT_STORAGE_SAVE_STOP_IND,

	EVENT_TRANSFER_OPEN_IND = (TRS_EVENT << MEDIA_EVT_BIT),
	EVENT_TRANSFER_CLOSE_IND,
	EVENT_TRANSFER_START_IND,
	EVENT_TRANSFER_PAUSE_IND,

	EVENT_AVI_OPEN_IND,
	EVENT_AVI_CLOSE_IND,

	EVENT_TRANSFER_USB_OPEN_IND = (USB_TRS_EVENT << MEDIA_EVT_BIT),
	EVENT_TRANSFER_USB_CLOSE_IND,

	EVENT_LCD_DEFAULT_CMD = (MAILBOX_CMD << MEDIA_EVT_BIT),
	EVENT_LCD_ROTATE_RIGHT_CMD,
	EVENT_LCD_RESIZE_CMD,
	EVENT_LCD_DEC_SW_CMD,
	EVENT_LCD_DEC_SW_OPEN_CMD,


	EVENT_LVGL_DRAW_CMD,

	EVENT_LCD_DEFAULT_EVT = (MAILBOX_EVT << MEDIA_EVT_BIT),
	EVENT_LCD_ROTATE_RIGHT_COMP_EVT,

	EVENT_LCD_OPEN_REQ = (MAILBOX_REQ << MEDIA_EVT_BIT),
	EVENT_LCD_DEC_SW_REQ,
	EVENT_TRANSFER_FRAME_REQ,
	EVENT_TRANSFER_DEINIT_REQ,

	EVENT_LCD_OPEN_RSP = (MAILBOX_RSP << MEDIA_EVT_BIT),
	EVENT_LCD_DEC_SW_RSP,
	EVENT_TRANSFER_FRAME_RSP,
	EVENT_TRANSFER_DEINIT_RSP,

	/* notify op */
	EVENT_AUD_MIC_DATA_NOTIFY = (MAILBOX_NOTIFY << MEDIA_EVT_BIT),
	EVENT_AUD_SPK_DATA_NOTIFY,
	EVENT_MEDIA_DATA_NOTIFY,
	EVENT_USB_DATA_NOTIFY,
	EVENT_VID_CAPTURE_NOTIFY,
	EVENT_VID_SAVE_ALL_NOTIFY,
	EVENT_LCD_PICTURE_ECHO_NOTIFY,
	EVENT_DMA_RESTART_NOTIFY,
	EVENT_DVP_EOF_NOTIFY,
	EVENT_JPEG_DEC_INIT_NOTIFY,
	EVENT_JPEG_DEC_DEINIT_NOTIFY,
	EVENT_JPEG_DEC_START_NOTIFY,
	EVENT_JPEG_DEC_SET_ROTATE_ANGLE_NOTIFY,
	EVENT_JPEG_DEC_START_COMPLETE_NOTIFY,
	EVENT_JPEG_DEC_LINE_COMPLETE_NOTIFY,
	EVENT_TRANS_LOG_NOTIFY,
	EVENT_UVC_DEVICE_INFO_NOTIFY,
	EVENT_UAC_CONNECT_STATE_NOTIFY,
	EVENT_YUV_ROTATE_NOTIFY,

	EVENT_BT_AUDIO_INIT_REQ = (BT_EVENT << MEDIA_EVT_BIT),
	EVENT_BT_AUDIO_DEINIT_REQ,
	EVENT_BT_PCM_RESAMPLE_INIT_REQ,
	EVENT_BT_PCM_RESAMPLE_DEINIT_REQ,
	EVENT_BT_PCM_RESAMPLE_REQ,
	EVENT_BT_PCM_ENCODE_INIT_REQ,
	EVENT_BT_PCM_ENCODE_DEINIT_REQ,
	EVENT_BT_PCM_ENCODE_REQ,

	EVENT_MEDIA_DEBUG_IND = (MAJOR_COMM_EVENT << MEDIA_EVT_BIT),
	EVENT_MEDIA_CPU1_POWERUP_IND,
	EVENT_MEDIA_CPU1_POWEROFF_IND,

	EVENT_FRAME_BUFFER_INIT_IND = (FRAME_BUFFER_EVENT << MEDIA_EVT_BIT),
	EVENT_FRAME_BUFFER_JPEG_MALLOC_IND,
	EVENT_FRAME_BUFFER_H264_MALLOC_IND,
	EVENT_FRAME_BUFFER_FREE_IND,
	EVENT_FRAME_BUFFER_PUSH_IND,
} media_event_t;

typedef struct
{
	uint32_t event;
	uint32_t param;
} media_msg_t;

typedef struct
{
	uint32_t event;
	uint32_t param;
	uint32_t param1;
} jpeg_msg_t;

typedef struct
{
	beken_semaphore_t sem;
	uint32_t param;
	int ret;
} param_pak_t;

typedef struct
{
	void *media_debug;
} media_share_ptr_t;


bk_err_t media_send_msg(media_msg_t *msg);
bk_err_t media_app_send_msg(media_msg_t *msg);

#ifdef __cplusplus
}
#endif
