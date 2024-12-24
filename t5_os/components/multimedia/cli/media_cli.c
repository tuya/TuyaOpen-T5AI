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
#include "cli.h"
#include "media_cli.h"

#include "media_cli_comm.h"
#include "media_app.h"

#include <driver/dvp_camera.h>
#include <driver/jpeg_enc.h>
#include "lcd_act.h"
#include "draw_blend.h"

#include "storage_act.h"
#include <driver/audio_ring_buff.h>
#include "aud_intf.h"
#include <driver/uvc_camera.h>
#include <driver/h264_types.h>

#include "media_utils.h"

#define TAG "mcli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define UNKNOW_ERROR (-686)

#define CMD_CONTAIN(value) cmd_contain(argc, argv, value)
#define GET_PPI(value)     get_ppi_from_cmd(argc, argv, value)
#define GET_NAME(value)    get_name_from_cmd(argc, argv, value)
#define GET_ROTATE()       get_rotate_from_cmd(argc, argv)
#define GET_H26X_PPI()     get_h26x_ppi_from_cmd(argc, argv)

static bk_uvc_config_t uvc_config_param = {0};

void uvc_connect_state_callback(uint8_t state)
{
	LOGI("%s %d+++\n", __func__, state);
}

void media_read_frame_callback(frame_buffer_t *frame)
{
	LOGI("frame_id:%d, length:%d, h264_fmt:%x, frame_addr:%p\r\n", frame->sequence,
		frame->length, frame->h264_type,frame->frame);
}

void media_checkout_uvc_device_info(bk_uvc_device_brief_info_t *info, uvc_state_t state)
{
	if (state == UVC_CONNECTED)
	{
		uint8_t format_index = 0;
		uint8_t frame_num = 0;
		uint8_t index = 0, i = 0;
		uvc_config_param.vendor_id = info->vendor_id;
		uvc_config_param.product_id = info->product_id;
		LOGI("%s uvc_get_param VID:0x%x\r\n",__func__, info->vendor_id);
		LOGI("%s uvc_get_param PID:0x%x\r\n",__func__, info->product_id);

		format_index = info->format_index.mjpeg_format_index;
		frame_num = info->all_frame.mjpeg_frame_num;
		if(format_index > 0){
			LOGI("%s uvc_get_param MJPEG format_index:%d\r\n",__func__, format_index);
			for(index = 0; index < frame_num; index++)
			{
				LOGI("uvc_get_param MJPEG width:%d heigth:%d index:%d\r\n",
							info->all_frame.mjpeg_frame[index].width,
							info->all_frame.mjpeg_frame[index].height,
							info->all_frame.mjpeg_frame[index].index);
				for(i = 0; i < info->all_frame.mjpeg_frame[index].fps_num; i++)
				{
					LOGI("uvc_get_param MJPEG fps:%d\r\n", info->all_frame.mjpeg_frame[index].fps[i]);
				}

				if (info->all_frame.mjpeg_frame[index].width == 800 && info->all_frame.mjpeg_frame[index].height == 480)
				{
					uvc_config_param.frame_index = info->all_frame.mjpeg_frame[index].index;
					uvc_config_param.fps = info->all_frame.mjpeg_frame[index].fps[0];
					uvc_config_param.width = 800;
					uvc_config_param.height = 480;
				}
			}
		}

		uvc_config_param.format_index = format_index;

		if (media_app_set_uvc_device_param(&uvc_config_param) != BK_OK)
		{
			LOGE("%s, failed\r\n", __func__);
		}

#if 0
		format_index = info->format_index.h264_format_index;
		frame_num = info->all_frame.h264_frame_num;
		if(format_index > 0){
			LOGI("uvc_get_param H264 format_index:%d\r\n", format_index);
			for(index = 0; index < frame_num; index++)
			{
				LOGI("uvc_get_param H264 width:%d heigth:%d index:%d\r\n",
							info->all_frame.h264_frame[index].width,
							info->all_frame.h264_frame[index].height,
							info->all_frame.h264_frame[index].index);
				for(i = 0; i < info->all_frame.h264_frame[index].fps_num; i++)
				{
					LOGI("uvc_get_param H264 fps:%d\r\n", info->all_frame.h264_frame[index].fps[i]);
				}
			}
		}

		format_index = info->format_index.h265_format_index;
		frame_num = info->all_frame.h265_frame_num;
		if(format_index > 0){
			LOGI("uvc_get_param H265 format_index:%d\r\n", format_index);
			for(index = 0; index < frame_num; index++)
			{
				LOGI("uvc_get_param H265 width:%d heigth:%d index:%d\r\n",
							info->all_frame.h265_frame[index].width,
							info->all_frame.h265_frame[index].height,
							info->all_frame.h265_frame[index].index);
				for(i = 0; i < info->all_frame.h265_frame[index].fps_num; i++)
				{
					LOGI("uvc_get_param H265 fps %d\r\n", info->all_frame.h265_frame[index].fps[i]);
				}
			}
		}

		format_index = info->format_index.yuv_format_index;
		frame_num = info->all_frame.yuv_frame_num;
		if(format_index > 0){
			LOGI("uvc_get_param YUV format_index:%d\r\n", format_index);
			for(index = 0; index < frame_num; index++)
			{
				LOGI("uvc_get_param YUV width:%d heigth:%d index:%d\r\n",
							info->all_frame.yuv_frame[index].width,
							info->all_frame.yuv_frame[index].height,
							info->all_frame.yuv_frame[index].index);
				for(i = 0; i < info->all_frame.yuv_frame[index].fps_num; i++)
				{
					LOGI("uvc_get_param YUV fps:%d\r\n", info->all_frame.yuv_frame[index].fps[i]);
				}
			}
		}

		for(int j = 0;j < info->endpoints_num; j++)
		{
			struct s_bk_usb_endpoint_descriptor *ep_desc = (struct s_bk_usb_endpoint_descriptor *)&info->ep_desc[j];
			LOGI("=========================================================================\r\n");
			LOGI("	  ------------ Endpoint Descriptor -----------	\r\n");
			LOGI("bLength					: 0x%x (%d bytes)\r\n", ep_desc->bLength, ep_desc->bLength);
			LOGI("bDescriptorType			: 0x%x (Endpoint Descriptor)\r\n", ep_desc->bDescriptorType);
			LOGI("bEndpointAddress			: 0x%x (Direction=IN  EndpointID=%d)\r\n", ep_desc->bEndpointAddress, (ep_desc->bEndpointAddress & 0x0F));
			LOGI("bmAttributes			: 0x%x\r\n", ep_desc->bmAttributes);
			LOGI("wMaxPacketSize 			: 0x%x (%d bytes)\r\n", ep_desc->wMaxPacketSize, ep_desc->wMaxPacketSize);
			LOGI("bInterval				: 0x%x (%d ms)\r\n", ep_desc->bInterval, ep_desc->bInterval);
		}
#endif
	}
	else
	{
		LOGI("%s, %d\r\n", __func__, state);
	}
}


uint32_t get_ppi_from_cmd(int argc, char **argv, uint32_t pre)
{
	int i;
	uint32_t value = pre;

	for (i = 0; i < argc; i++)
	{
		value = get_string_to_ppi(argv[i]);

		if (value != PPI_DEFAULT)
		{
			break;
		}
	}

	if (value == PPI_DEFAULT)
	{
		value = pre;
	}

	LOGD("%s %d-%d+++\n", __func__, value >> 16, value & 0xFFFF);

	return value;
}

uint32_t get_h26x_ppi_from_cmd(int argc, char **argv)
{
	int i;
	uint32_t value = PPI_DEFAULT;

	for (i = 5; i < argc; i++)
	{
		value = get_string_to_ppi(argv[i]);

		if (value != PPI_DEFAULT)
		{
			break;
		}
	}
	LOGD("%s %d-%d+++\n", __func__, value >> 16, value & 0xFFFF);
	return value;
}


char * get_name_from_cmd(int argc, char **argv, char * pre)
{
	int i;
	char* value = pre;

	for (i = 3; i < argc; i++)
	{
		value = get_string_to_lcd_name(argv[i]);

		if (value != NULL)
		{
			break;
		}
	}

	return value;
}

media_rotate_t get_rotate_from_cmd(int argc, char **argv)
{
	int i;
	media_rotate_t value = ROTATE_90;

	for (i = 3; i < argc; i++)
	{
		value = get_string_to_angle(argv[i]);
	}

	return value;
}

bool cmd_contain(int argc, char **argv, char *string)
{
	int i;
	bool ret = false;

	for (i = 0; i < argc; i++)
	{
		if (os_strcmp(argv[i], string) == 0)
		{
			ret = true;
		}
	}

	return ret;
}

void media_cli_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = UNKNOW_ERROR;
	char *msg = NULL;

	LOGI("%s +++\n", __func__);

	if (argc <= 2)
	{
		ret = kParamErr;
		goto output;
	}
	else
	{
		if (os_strcmp(argv[1], "dvp") == 0)
		{
			media_ppi_t ppi = GET_PPI(PPI_640X480);
			media_camera_device_t device = {0};
			device.type = DVP_CAMERA;
			device.mode = JPEG_MODE;
			device.fmt = PIXEL_FMT_JPEG;
			device.info.resolution.width = ppi >> 16;
			device.info.resolution.height = ppi & 0xFFFF;
			device.info.fps = FPS25;

			if (CMD_CONTAIN("yuv"))
			{
				device.mode = YUV_MODE;
				device.fmt = PIXEL_FMT_YUYV;
			}

			if (CMD_CONTAIN("jpeg"))
			{
				if (CMD_CONTAIN("enc_yuv"))
				{
					device.mode = JPEG_YUV_MODE;
				}
				else
				{
					device.mode = JPEG_MODE;
				}

				device.fmt = PIXEL_FMT_JPEG;
			}

			if (CMD_CONTAIN("h264"))
			{
				if (CMD_CONTAIN("enc_yuv"))
				{
					device.mode = H264_YUV_MODE;
				}
				else
				{
					device.mode = H264_MODE;
				}

				device.fmt = PIXEL_FMT_H264;
			}

			if (CMD_CONTAIN("rotate"))
			{
				if (CMD_CONTAIN("90"))
				{
					device.rot_angle = ROTATE_90;
				}
				else if (CMD_CONTAIN("180"))
				{
					device.rot_angle = ROTATE_180;
				}
				else if (CMD_CONTAIN("270"))
				{
					device.rot_angle = ROTATE_270;
				}
				else
				{
					device.rot_angle = ROTATE_NONE;
				}
			}

			if (os_strcmp(argv[2], "open") == 0)
			{
				ret = media_app_camera_open(&device);
			}

			if (os_strcmp(argv[2], "close") == 0)
			{
				ret = media_app_camera_close(DVP_CAMERA);
			}

			if (os_strcmp(argv[2], "free") == 0)
			{
				ret = media_app_camera_close(UNKNOW_CAMERA);
			}
		}

		if (os_strcmp(argv[1], "compress") == 0)
		{
			compress_ratio_t compress = {0};

			if (os_strcmp(argv[2], "h264") == 0)
			{
				if (argc >= 8)
				{
					compress.mode = H264_MODE;
					compress.enable = 1;
					compress.qp.init_qp = os_strtoul(argv[3], NULL, 10);
					compress.qp.i_max_qp = os_strtoul(argv[4], NULL, 10);
					compress.qp.p_max_qp = os_strtoul(argv[5], NULL, 10);
					compress.imb_bits = os_strtoul(argv[6], NULL, 10);
					compress.pmb_bits = os_strtoul(argv[7], NULL, 10);

					ret = media_app_set_compression_ratio(&compress);
				}
			}
			else if (os_strcmp(argv[2], "jpeg") == 0)
			{
				if (argc >= 6)
				{
					compress.mode = JPEG_MODE;
					compress.enable = os_strtoul(argv[3], NULL, 10);
					compress.jpeg_up = 1024 * os_strtoul(argv[4], NULL, 10);
					compress.jpeg_low = 1024 * os_strtoul(argv[5], NULL, 10);

					ret = media_app_set_compression_ratio(&compress);
				}
			}
		}

		if (os_strcmp(argv[1], "register") == 0)
		{
			if (os_strcmp(argv[2], "get_config") == 0)
			{
				ret = media_app_uvc_register_info_notify_cb(media_checkout_uvc_device_info);
			}

			if (os_strcmp(argv[2], "set_config") == 0)
			{
				if (argc >= 8)
				{
					uvc_config_param.width = os_strtoul(argv[3], NULL, 10);
					uvc_config_param.height = os_strtoul(argv[4], NULL, 10);
					uvc_config_param.format_index = os_strtoul(argv[5], NULL, 10);
					uvc_config_param.frame_index = os_strtoul(argv[6], NULL, 10);
					uvc_config_param.fps = os_strtoul(argv[7], NULL, 10);
					ret = media_app_set_uvc_device_param(&uvc_config_param);
				}
				else
					ret = kParamErr;
			}
		}

		if (os_strcmp(argv[1], "storage") == 0)
		{
			app_camera_type_t camera_type = APP_CAMERA_DVP_JPEG;

			if (CMD_CONTAIN("h264"))
			{
				camera_type = APP_CAMERA_DVP_H264_LOCAL;
			}

			if (os_strcmp(argv[2], "open") == 0)
			{
				ret = media_app_storage_enable(camera_type, 1);
			}
			else
			{
				ret = media_app_storage_enable(camera_type, 0);
			}
		}

		if (os_strcmp(argv[1], "capture") == 0)
		{
			if (argc >= 3)
			{
				ret = media_app_capture(argv[2]);
			}
			else
			{
				ret = media_app_capture("unknow.jpg");
			}
		}

		if (os_strcmp(argv[1], "save_start") == 0)
		{
			if (argc >= 3)
			{
				ret = media_app_save_start(argv[2]);
			}
			else
			{
				ret = media_app_save_start("unknow.264");
			}
		}

		if (os_strcmp(argv[1], "save_stop") == 0)
		{
			ret = media_app_save_stop();
		}

		if (os_strcmp(argv[1], "read_stop") == 0)
		{
			ret = media_app_save_stop();
		}

		if (os_strcmp(argv[1], "lcd") == 0)
		{
			media_ppi_t ppi = PPI_480X272;
			char *name = "NULL";
			media_rotate_t rotate = ROTATE_NONE;
			//media_rotate_t yuv2rgb = ROTATE_NONE;
			ppi = GET_PPI(PPI_480X272);
			name = GET_NAME(name);

			if (CMD_CONTAIN("rotate"))
			{
				rotate = GET_ROTATE();
				ret = media_app_lcd_rotate(rotate);
			}
			if (CMD_CONTAIN("scale"))
			{
				ret = media_app_lcd_scale();
			}
			if (os_strcmp(argv[2], "open") == 0)
			{
				if (argc >= 4)
				{
					if(os_strcmp(argv[3], "dec_major") == 0)
					{
						media_app_lcd_decode(SOFTWARE_DECODING_MAJOR);
					}
					else if(os_strcmp(argv[3], "dec_minor") == 0)
					{
						media_app_lcd_decode(SOFTWARE_DECODING_MINOR);
					}
					else
					{
						media_app_lcd_decode(HARDWARE_DECODING);
					}
				}
				lcd_open_t lcd_open;
				lcd_open.device_ppi = ppi;
				lcd_open.device_name = name;
				ret = media_app_lcd_open(&lcd_open);
			}

			if (os_strcmp(argv[2], "close") == 0)
			{
				ret = media_app_lcd_close();
			}
			if (os_strcmp(argv[2], "backlight") == 0)
			{
				uint8_t level = os_strtoul(argv[3], NULL, 10) & 0xFF;
				ret = media_app_lcd_set_backlight(level);
			}
			if (os_strcmp(argv[2], "display") == 0)
			{
				if (argc >= 4)
				{
					ret = media_app_lcd_display_file(argv[3]);
				}
			}
			if (os_strcmp(argv[2], "dma2d_blend") == 0)
			{
				if (argc < 4)
				{
					ret = kParamErr;
					goto output;
				}
				
				lcd_blend_msg_t blend = {0} ;
				if(os_strcmp(argv[3], "open") == 0)
				{
					ret = media_app_lcd_blend_open(1);
				}
				else if (os_memcmp (argv[3], "wifi", 4) == 0)
				{
					if (argc < 5)
					{
						ret = kParamErr;
						goto output;
					}

					blend.blend_on = 1;
					blend.lcd_blend_type = LCD_BLEND_WIFI;
					blend.data[0] =  os_strtoul(argv[4], NULL, 10) & 0xFFFF;
					LOGI("wifi dma2d blend data = %d\r\n", blend.data[0]);
				}
				else if (os_memcmp (argv[3], "clock", 4) == 0)
				{
					if (argc < 5)
					{
						ret = kParamErr;
						goto output;
					}

					blend.blend_on = 1;
					blend.lcd_blend_type = LCD_BLEND_TIME;
					os_memcpy(blend.data, argv[4], 5);
					LOGI("time dma2d blend data = %s\r\n", blend.data);
				}
				else if (os_memcmp (argv[3], "data", 4) == 0)
				{
					blend.blend_on = 1;
					blend.lcd_blend_type = LCD_BLEND_DATA;
					//os_memcpy(blend.data, argv[4], 5);
				}
				else if (os_memcmp (argv[3], "ver", 3) == 0)
				{
					uint8_t version[] = "VL4 V1.23.34";
					blend.blend_on = 1;
					blend.lcd_blend_type = LCD_BLEND_VERSION;
					os_memcpy(blend.data, version, sizeof(version));
					LOGD("ver dma2d blend data = %s\r\n", blend.data);
				}
				else if (os_strcmp(argv[3], "close") == 0)
				{
					if (argc < 5)
					{
						blend.blend_on = 0;
						blend.lcd_blend_type = LCD_BLEND_VERSION | LCD_BLEND_TIME | LCD_BLEND_WIFI | LCD_BLEND_DATA;
						ret = media_app_lcd_blend_open(0);
						LOGI(" dma2d blend close deinit\r\n");
					}
					else
					{
						if (os_strcmp(argv[4], "wifi") == 0)
						{
							blend.blend_on = 0;
							blend.lcd_blend_type = LCD_BLEND_WIFI;
						}
						else if (os_strcmp(argv[4], "clock") == 0)
						{
							blend.blend_on = 0;
							blend.lcd_blend_type = LCD_BLEND_TIME;
						}
						else if (os_strcmp(argv[4], "ver") == 0)
						{
							blend.blend_on = 0;
							blend.lcd_blend_type = LCD_BLEND_VERSION;
						}
						else if (os_strcmp(argv[4], "data") == 0)
						{
							blend.blend_on = 0;
							blend.lcd_blend_type = LCD_BLEND_DATA;
						}
						else
						{
							blend.blend_on = 0;
							blend.lcd_blend_type = LCD_BLEND_VERSION | LCD_BLEND_TIME | LCD_BLEND_WIFI | LCD_BLEND_DATA;
						}
					}
				}
				else
				{
					LOGI("cmd not support \r\n");
				}
				ret = media_app_lcd_blend(&blend);
			}
			if (os_strcmp(argv[2], "status") == 0)
			{
				uint32_t lcd_status = 0;
				lcd_status = media_app_get_lcd_status();
				LOGI("lcd status %d\r\n", lcd_status);
				ret = BK_OK;
			}
		}

		if (os_strcmp(argv[1], "uvc") == 0)
		{
			media_ppi_t ppi = GET_PPI(PPI_640X480);
			media_camera_device_t device = {0};
			device.type = UVC_CAMERA;
			device.mode = JPEG_MODE;
			device.fmt = PIXEL_FMT_JPEG;
			device.info.fps = FPS25;
            device.num_uvc_dev = 1;

			if (CMD_CONTAIN("h264"))
			{
				device.mode = H264_MODE;
				device.fmt = PIXEL_FMT_H264;
			}

			if (os_strcmp(argv[2], "open") == 0)
			{
				if (ppi == 0)
				{
					LOGI("resolution not support\r\n");
					ret = BK_FAIL;
				}
				else
				{
					device.info.resolution.width  = ppi >> 16;
					device.info.resolution.height = ppi & 0xFFFF;
					//media_app_register_uvc_connect_state_cb(uvc_connect_state_callback);

					if (CMD_CONTAIN("dual")) {
						media_ppi_t h26x_ppi = 0;
						device.dualstream = 1;
						if (CMD_CONTAIN("H264")) {
							device.d_fmt  = PIXEL_FMT_H264;
							device.d_mode = H264_MODE;
						} else if (CMD_CONTAIN("H265"))
						{
							device.d_fmt  = PIXEL_FMT_H265;
							device.d_mode = H265_MODE;
						}
						h26x_ppi = GET_H26X_PPI();
						device.num_uvc_dev = 2;
						device.d_info.resolution.width	= h26x_ppi >> 16;
						device.d_info.resolution.height = h26x_ppi & 0xFFFF;
						device.d_info.fps = FPS30;
						LOGI("Enter Second uvc device H26X Config.\n");
					}
					ret = media_app_camera_open(&device);
				}
			}

			if (os_strcmp(argv[2], "close") == 0)
			{
				ret = media_app_camera_close(UVC_CAMERA);
			}

			if (os_strcmp(argv[2], "status") == 0)
			{
				uint32_t camera_status = 0;
				camera_status = media_app_get_uvc_camera_status();
				LOGI("camera status %d\r\n", camera_status);
				ret = BK_OK;
			}
		}

		if (os_strcmp(argv[1], "pipeline") == 0)
		{
			if (os_strcmp(argv[2], "lcd_open") == 0)
			{
				lcd_open_t lcd_open;
				char *name = "st7792";
				name = GET_NAME(name);
				lcd_open.device_ppi = GET_PPI(PPI_480X272);
				lcd_open.device_name = name;

                media_rotate_t rotate = ROTATE_NONE;
                if (CMD_CONTAIN("rotate"))
                {
                    
                    if (CMD_CONTAIN("sw"))
                    {
                        media_app_lcd_fmt(PIXEL_FMT_RGB888);
                    }
                    else
                    {
                        media_app_lcd_fmt(PIXEL_FMT_RGB565_LE);
                    }
                    rotate = GET_ROTATE();
                    ret = media_app_pipline_set_rotate(rotate);
                }

				if (CMD_CONTAIN("scale"))
				{
					lcd_scale_t lcd_scale;
					lcd_scale.src_ppi = PPI_480X864; 
                    lcd_scale.dst_ppi = PPI_480X854;
                    if (argv[5] && argv[6])
                    {
                        uint32_t width = strtoul(argv[5], NULL, 10);
                        uint32_t height = strtoul(argv[6], NULL, 10);
                        lcd_scale.dst_ppi = width << 16 | height;
                    }
                    //default by 480X864, acturally by formal param, such as rotated or decoded
					ret = media_app_lcd_pipline_scale_open(&lcd_scale);
				}

				ret = media_app_lcd_pipeline_open(&lcd_open);
			}

			if (os_strcmp(argv[2], "lcd_close") == 0)
			{
				ret = media_app_lcd_pipeline_close();
				if (CMD_CONTAIN("scale"))
				{
				    media_app_lcd_pipline_scale_close();
				}
			}
			if (os_strcmp(argv[2], "h264_open") == 0)
			{
				ret = media_app_h264_pipeline_open();
			}

			if (os_strcmp(argv[2], "h264_close") == 0)
			{
				ret = media_app_h264_pipeline_close();
			}

			if (os_strcmp(argv[2], "dump") == 0)
			{
				ret = media_app_pipeline_dump();
			}

		}

		if (os_strcmp(argv[1], "read_frame") == 0)
		{
			uint32_t fmt = PIXEL_FMT_JPEG;
			if (os_strcmp(argv[2], "open") == 0)
			{
				if (CMD_CONTAIN("h264"))
				{
					fmt = PIXEL_FMT_H264;
				}
				else
				{
					fmt = PIXEL_FMT_JPEG;
				}

				ret = media_app_register_read_frame_callback(fmt, media_read_frame_callback);
			}
			else // close
			{
				ret = media_app_unregister_read_frame_callback();
			}
		}

		if (os_strcmp(argv[1], "h264") == 0)
		{
			if (os_strcmp(argv[2], "get_config") == 0)
			{
				h264_base_config_t config;
				ret = media_app_get_h264_encode_config(&config);
				if (ret == BK_OK)
				{
					LOGI("h264_encode_state:%d\r\n", config.h264_state);
					LOGI("p_frame_cnt      :%d\r\n", config.p_frame_cnt);
					LOGI("profile_id       :%d\r\n", config.profile_id);
					LOGI("qp               :%d\r\n", config.qp);
					LOGI("num_imb_bits     :%d\r\n", config.num_imb_bits);
					LOGI("num_pmb_bits     :%d\r\n", config.num_pmb_bits);
					LOGI("width            :%d\r\n", config.width);
					LOGI("height           :%d\r\n", config.height);
				}
			}

			if (os_strcmp(argv[2], "reset") == 0)
			{
				if (os_strcmp(argv[3], "dvp") == 0)
					ret = media_app_h264_regenerate_idr(DVP_CAMERA);
				else
					ret = media_app_h264_regenerate_idr(UVC_CAMERA);
			}
		}

		if (os_strcmp(argv[1], "fb") == 0)
		{
			static frame_buffer_t *new_frame = NULL;
			fb_type_t type = FB_INDEX_JPEG;

			if (os_strcmp(argv[2], "init") == 0)
			{
				if (os_strcmp(argv[3], "jpeg") == 0)
				{
					type = FB_INDEX_JPEG;
				}
				else if (os_strcmp(argv[3], "h264") == 0)
				{
					type = FB_INDEX_H264;
				}
				else
				{
					type = FB_INDEX_DISPLAY;
				}

				ret = media_app_frame_buffer_init(type);
			}

			if (os_strcmp(argv[2], "malloc") == 0)
			{
				if (os_strcmp(argv[3], "jpeg") == 0)
				{
					new_frame = media_app_frame_buffer_jpeg_malloc();
					ret = BK_OK;
				}
				else if (os_strcmp(argv[3], "h264") == 0)
				{
					new_frame = media_app_frame_buffer_h264_malloc();
					ret = BK_OK;
				}
				else
				{
					//
					ret = kParamErr;
				}
			}

			if (os_strcmp(argv[2], "push") == 0)
			{
				ret = media_app_frame_buffer_push(new_frame);
			}

			if (os_strcmp(argv[2], "clear") == 0)
			{
				ret = media_app_frame_buffer_clear(new_frame);
			}
		}
	}

output:

	if (ret == UNKNOW_ERROR)
	{
		LOGE("%s unknow cmd\n", __func__);
	}

	if (ret == kParamErr)
	{
		LOGE("%s param error cmd\n", __func__);
	}

	if (ret != BK_OK)
	{
		msg = CLI_CMD_RSP_ERROR;
	}
	else
	{
		msg = CLI_CMD_RSP_SUCCEED;
	}

	LOGI("%s ---complete\n", __func__);

	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#define MEDIA_CMD_CNT   (sizeof(s_media_commands) / sizeof(struct cli_command))

static const struct cli_command s_media_commands[] =
{
	{"media", "media...", media_cli_test_cmd},
};

int media_cli_init(void)
{
	return cli_register_commands(s_media_commands, MEDIA_CMD_CNT);
}
