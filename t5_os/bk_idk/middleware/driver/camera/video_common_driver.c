// Copyright 2023-2024 Beken
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
#include <os/mem.h>
#include <os/str.h>

#include <driver/int.h>
#include <driver/jpeg_enc.h>
#include <driver/h264.h>
#include <driver/yuv_buf.h>
#include <driver/gpio.h>
#include <driver/video_common_driver.h>
#include <driver/dma.h>
#include "sys_driver.h"
#include "gpio_driver.h"
#include <modules/pm.h>
#include "bk_misc.h"

#define TAG "video_drv"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct {
	gpio_id_t gpio_id;
	gpio_dev_t dev;
} camera_gpio_map_t;

#define VIDEO_GPIO_PIN_NUMBER	12
#define VIDEO_GPIO_MAP \
{\
	{GPIO_27, GPIO_DEV_JPEG_MCLK},\
	{GPIO_29, GPIO_DEV_JPEG_PCLK},\
	{GPIO_30, GPIO_DEV_JPEG_HSYNC},\
	{GPIO_31, GPIO_DEV_JPEG_VSYNC},\
	{GPIO_32, GPIO_DEV_JPEG_PXDATA0},\
	{GPIO_33, GPIO_DEV_JPEG_PXDATA1},\
	{GPIO_34, GPIO_DEV_JPEG_PXDATA2},\
	{GPIO_35, GPIO_DEV_JPEG_PXDATA3},\
	{GPIO_36, GPIO_DEV_JPEG_PXDATA4},\
	{GPIO_37, GPIO_DEV_JPEG_PXDATA5},\
	{GPIO_38, GPIO_DEV_JPEG_PXDATA6},\
	{GPIO_39, GPIO_DEV_JPEG_PXDATA7},\
}

#define AUXS_CLK_CIS_ENABLE         1

extern uint32_t g_n_uvc_dev;

static uint8_t list_cnt = 0;
static camera_packet_t **camera_packet_list = NULL;
static camera_packet_t ** camera_packet_list_dual = NULL;

bk_err_t bk_video_camera_packet_list_deinit(void)
{
	LOGI("%s, %d\r\n", __func__, __LINE__);
	if (list_cnt == 0)
	{
		return BK_OK;
	}

    if(camera_packet_list != NULL)
    {
    	for (uint8_t j = 0; j < list_cnt; j++)
    	{
    		if (camera_packet_list[j] == NULL)
    			continue;

    		if (camera_packet_list[j]->data_buffer)
    		{
    			os_free(camera_packet_list[j]->data_buffer);
    			camera_packet_list[j]->data_buffer = NULL;
    		}

    		if (camera_packet_list[j]->state)
    		{
    			os_free(camera_packet_list[j]->state);
    			camera_packet_list[j]->state = NULL;
    		}

    		if (camera_packet_list[j]->num_byte)
    		{
    			os_free(camera_packet_list[j]->num_byte);
    			camera_packet_list[j]->num_byte = NULL;
    		}

    		if (camera_packet_list[j]->actual_num_byte)
    		{
    			os_free(camera_packet_list[j]->actual_num_byte);
    			camera_packet_list[j]->actual_num_byte = NULL;
    		}

    		os_free(camera_packet_list[j]);
    		camera_packet_list[j] = NULL;
    	}

    	os_free(camera_packet_list);
    	camera_packet_list = NULL;
    }

    if(camera_packet_list_dual != NULL)
    {
        for (uint8_t j = 0; j < list_cnt; j++)
        {
            if (camera_packet_list_dual[j] == NULL)
                continue;

            if (camera_packet_list_dual[j]->data_buffer)
            {
                os_free(camera_packet_list_dual[j]->data_buffer);
                camera_packet_list_dual[j]->data_buffer = NULL;
            }

            if (camera_packet_list_dual[j]->state)
            {
                os_free(camera_packet_list_dual[j]->state);
                camera_packet_list_dual[j]->state = NULL;
            }

            if (camera_packet_list_dual[j]->num_byte)
            {
                os_free(camera_packet_list_dual[j]->num_byte);
                camera_packet_list_dual[j]->num_byte = NULL;
            }

            if (camera_packet_list_dual[j]->actual_num_byte)
            {
                os_free(camera_packet_list_dual[j]->actual_num_byte);
                camera_packet_list_dual[j]->actual_num_byte = NULL;
            }

            os_free(camera_packet_list_dual[j]);
            camera_packet_list_dual[j] = NULL;
        }

        os_free(camera_packet_list_dual);
        camera_packet_list_dual = NULL;
    }

	list_cnt = 0;
	return BK_OK;
}

bk_err_t bk_video_camera_packet_list_init(mem_location_t locate, uint16_t MaxPacketSize, uint8_t max_packet_cnt, uint8_t cnt, uint32_t idx_uvc)
{
	uint8_t i = 0;
    uint32_t idx = idx_uvc;

    if (idx_uvc == 0)
    {
    	if (camera_packet_list != NULL)
    	{
    		bk_video_camera_packet_list_free();
    		return BK_OK;
    	}

    	list_cnt = cnt;

    	camera_packet_list = (camera_packet_t **)os_malloc(sizeof(camera_packet_t *) * cnt);
    	if (camera_packet_list == NULL)
    	{
    		bk_video_camera_packet_list_deinit();
    		return BK_ERR_NO_MEM;
    	}

    	for (i = 0; i < list_cnt; i++)
    	{
    		camera_packet_list[i] = (camera_packet_t *)os_malloc(sizeof(camera_packet_t));
            camera_packet_list[i]->uvc_dev_id = idx;

    		if (camera_packet_list[i] == NULL)
    		{
    			bk_video_camera_packet_list_deinit();
    			return BK_ERR_NO_MEM;
    		}

    		camera_packet_list[i]->locate = locate;

    	//  camera_packet_list[i]->state = (cam_stream_state_t *)os_malloc(sizeof(cam_stream_state_t) * max_packet_cnt);
    		camera_packet_list[i]->state = (uint8_t *)os_malloc(sizeof(uint8_t) * max_packet_cnt);
    		if (camera_packet_list[i]->state == NULL)
    		{
    			bk_video_camera_packet_list_deinit();
    			return BK_ERR_NO_MEM;
    		}

    		os_memset(camera_packet_list[i]->state, 1, max_packet_cnt);

            if (locate == CAMERA_MEM_IN_PSRAM)
            {
                camera_packet_list[i]->num_byte = (uint16_t *)psram_malloc(sizeof(uint16_t) * max_packet_cnt);
            }
            else
            {
                camera_packet_list[i]->num_byte = (uint16_t *)os_malloc(sizeof(uint16_t) * max_packet_cnt);
            }

    		if (camera_packet_list[i]->num_byte == NULL)
    		{
    			bk_video_camera_packet_list_deinit();
    			return BK_ERR_NO_MEM;
    		}

    		for (int j = 0; j < max_packet_cnt; j++)
    			camera_packet_list[i]->num_byte[j] = MaxPacketSize;

            if (locate == CAMERA_MEM_IN_PSRAM)
            {
                camera_packet_list[i]->actual_num_byte = (uint16_t *)psram_malloc(sizeof(uint16_t) * max_packet_cnt);
            }
            else
            {
                camera_packet_list[i]->actual_num_byte = (uint16_t *)os_malloc(sizeof(uint16_t) * max_packet_cnt);
            }

    		if (camera_packet_list[i]->actual_num_byte == NULL)
    		{
    			bk_video_camera_packet_list_deinit();
    			return BK_ERR_NO_MEM;
    		}
    		os_memset(camera_packet_list[i]->actual_num_byte, 0, sizeof(uint16_t) * max_packet_cnt);

    		camera_packet_list[i]->data_buffer_size = MaxPacketSize * max_packet_cnt;

    		if (locate == CAMERA_MEM_IN_PSRAM)
            {
                camera_packet_list[i]->data_buffer = (uint8_t *)psram_malloc(camera_packet_list[i]->data_buffer_size);
            }
    		else
            {
                camera_packet_list[i]->data_buffer = (uint8_t *)os_malloc(camera_packet_list[i]->data_buffer_size);
            }

    		if (camera_packet_list[i]->data_buffer == NULL)
    		{
    			bk_video_camera_packet_list_deinit();
    			return BK_ERR_NO_MEM;
    		}

    		LOGD("camera_packet_list[%d]->data_buffer:%p\r\n", i, camera_packet_list[i]->data_buffer);

    		camera_packet_list[i]->num_packets = max_packet_cnt;

    		camera_packet_list[i]->packet_state = CAM_STREAM_IDLE;
    	}
    }
    else if (idx_uvc == 1)
    {
    	if (camera_packet_list_dual != NULL)
    	{
    		bk_video_camera_packet_list_dual_free();
    		return BK_OK;
    	}

    	list_cnt = cnt;

    	camera_packet_list_dual = (camera_packet_t **)os_malloc(sizeof(camera_packet_t *) * cnt);
        LOGD("size of camera_packet_list_dual pointer : %d\n", (sizeof(camera_packet_t *) * cnt));
    	if (camera_packet_list_dual == NULL)
    	{
    		bk_video_camera_packet_list_deinit();
    		return BK_ERR_NO_MEM;
    	}

    	for (i = 0; i < list_cnt; i++)
    	{
    		camera_packet_list_dual[i] = (camera_packet_t *)os_malloc(sizeof(camera_packet_t));
            camera_packet_list_dual[i]->uvc_dev_id = idx;

    		if (camera_packet_list_dual[i] == NULL)
    		{
    			bk_video_camera_packet_list_deinit();
    			return BK_ERR_NO_MEM;
    		}

    		camera_packet_list_dual[i]->locate = locate;

            if (locate == CAMERA_MEM_IN_PSRAM)
            {
                camera_packet_list_dual[i]->state = (uint8_t *)psram_malloc(sizeof(uint8_t) * max_packet_cnt);
            }
            else
            {
                camera_packet_list_dual[i]->state = (uint8_t *)os_malloc(sizeof(uint8_t) * max_packet_cnt);
            }

    		if (camera_packet_list_dual[i]->state == NULL)
    		{
    			bk_video_camera_packet_list_deinit();
    			return BK_ERR_NO_MEM;
    		}

    		os_memset(camera_packet_list_dual[i]->state, 1, max_packet_cnt);

            if (locate == CAMERA_MEM_IN_PSRAM)
            {
                camera_packet_list_dual[i]->num_byte = (uint16_t *)psram_malloc(sizeof(uint16_t) * max_packet_cnt);
            }
            else
            {
                camera_packet_list_dual[i]->num_byte = (uint16_t *)os_malloc(sizeof(uint16_t) * max_packet_cnt);
            }

    		if (camera_packet_list_dual[i]->num_byte == NULL)
    		{
    			bk_video_camera_packet_list_deinit();
    			return BK_ERR_NO_MEM;
    		}

    		for (int j = 0; j < max_packet_cnt; j++)
    			camera_packet_list_dual[i]->num_byte[j] = MaxPacketSize;

            if (locate == CAMERA_MEM_IN_PSRAM)
            {
                camera_packet_list_dual[i]->actual_num_byte = (uint16_t *)psram_malloc(sizeof(uint16_t) * max_packet_cnt);
            }
            else
            {
                camera_packet_list_dual[i]->actual_num_byte = (uint16_t *)os_malloc(sizeof(uint16_t) * max_packet_cnt);
            }

    		if (camera_packet_list_dual[i]->actual_num_byte == NULL)
    		{
    			bk_video_camera_packet_list_deinit();
    			return BK_ERR_NO_MEM;
    		}
    		os_memset(camera_packet_list_dual[i]->actual_num_byte, 0, sizeof(uint16_t) * max_packet_cnt);

    		camera_packet_list_dual[i]->data_buffer_size = MaxPacketSize * max_packet_cnt;
    		if (locate == CAMERA_MEM_IN_PSRAM)
            {
                camera_packet_list_dual[i]->data_buffer = (uint8_t *)psram_malloc(camera_packet_list_dual[i]->data_buffer_size);
            }
            else
            {
                camera_packet_list_dual[i]->data_buffer = (uint8_t *)os_malloc(camera_packet_list_dual[i]->data_buffer_size);
            }

    		if (camera_packet_list_dual[i]->data_buffer == NULL)
    		{
    			bk_video_camera_packet_list_deinit();
    			return BK_ERR_NO_MEM;
    		}

			LOGD("camera_packet_list_dual[%d]->data_buffer:%p\r\n", i, camera_packet_list_dual[i]->data_buffer);

    		camera_packet_list_dual[i]->num_packets = max_packet_cnt;

    		camera_packet_list_dual[i]->packet_state = CAM_STREAM_IDLE;
    	}
    }
	return BK_OK;
}

bk_err_t bk_video_camera_packet_list_free(void)
{
	for (uint8_t i = 0; i < list_cnt; i++)
	{
		if (camera_packet_list[i]->packet_state == CAM_STREAM_IDLE)
			continue;

		for (uint8_t j = 0; j < camera_packet_list[i]->num_packets; j++)
		{
			camera_packet_list[i]->state[j] = CAM_STREAM_IDLE;
			camera_packet_list[i]->actual_num_byte[j] = 0;
		//  camera_packet_list[i]->num_byte[j] = 0;
		}

		camera_packet_list[i]->packet_state = CAM_STREAM_IDLE;
	}

	return BK_OK;
}

bk_err_t bk_video_camera_packet_list_dual_free(void)
{
    if (camera_packet_list_dual != NULL)
    {
    	for (uint8_t i = 0; i < list_cnt; i++)
    	{
    		if (camera_packet_list_dual[i]->packet_state == CAM_STREAM_IDLE)
    			continue;

    		for (uint8_t j = 0; j < camera_packet_list_dual[i]->num_packets; j++)
    		{
    			camera_packet_list_dual[i]->state[j] = CAM_STREAM_IDLE;
    			camera_packet_list_dual[i]->actual_num_byte[j] = 0;
			//  camera_packet_list_dual[i]->num_byte[j] = 0;
    		}

    		camera_packet_list_dual[i]->packet_state = CAM_STREAM_IDLE;
    	}
    }
	return BK_OK;
}


camera_packet_t *bk_video_camera_packet_malloc(void)
{
	uint8_t i = 0;

	GLOBAL_INT_DECLARATION();

	for (i = 0; i < list_cnt; i++)
	{
		if (camera_packet_list[i]->packet_state == CAM_STREAM_IDLE)
		{
			break;
		}
	}

	GLOBAL_INT_DISABLE();

	if (i == list_cnt)
	{
		for (i = list_cnt; i > 0; i--)
		{
			if (camera_packet_list[i - 1]->packet_state == CAM_STREAM_READY)
			{
				LOGW("%s, index:%d [%d,%d,%d,%d]\r\n", __func__, i - 1,
						camera_packet_list[0]->packet_state,
						camera_packet_list[1]->packet_state,
						camera_packet_list[2]->packet_state,
						camera_packet_list[3]->packet_state);
				camera_packet_list[i - 1]->packet_state = CAM_STREAM_IDLE;
				i--;
				break;
			}
		}
	}

	for (uint8_t j = 0; j < camera_packet_list[i]->num_packets; j++)
	{
		camera_packet_list[i]->state[j] = CAM_STREAM_IDLE;
		camera_packet_list[i]->actual_num_byte[j] = 0;
		//camera_packet_list[i]->num_byte[j] = 0;
	}

	camera_packet_list[i]->packet_state = CAM_STREAM_BUSY;

	GLOBAL_INT_RESTORE();

	return camera_packet_list[i];
}

camera_packet_t *bk_video_camera_packet_dual_malloc(void)
{
	uint8_t i = 0;

	GLOBAL_INT_DECLARATION();

	for (i = 0; i < list_cnt; i++)
	{
		if (camera_packet_list_dual[i]->packet_state == CAM_STREAM_IDLE)
		{
			break;
		}
	}

	GLOBAL_INT_DISABLE();

	if (i == list_cnt)
	{
		for (i = list_cnt; i > 0; i--)
		{
			if (camera_packet_list_dual[i - 1]->packet_state == CAM_STREAM_READY)
			{
				LOGW("%s, index:%d [%d,%d,%d,%d]\r\n", __func__, i - 1,
						camera_packet_list_dual[0]->packet_state,
						camera_packet_list_dual[1]->packet_state,
						camera_packet_list_dual[2]->packet_state,
						camera_packet_list_dual[3]->packet_state);
				camera_packet_list_dual[i - 1]->packet_state = CAM_STREAM_IDLE;
				i--;
				break;
			}
		}
	}

	for (uint8_t j = 0; j < camera_packet_list_dual[i]->num_packets; j++)
	{
		camera_packet_list_dual[i]->state[j] = CAM_STREAM_IDLE;
		camera_packet_list_dual[i]->actual_num_byte[j] = 0;
	//  camera_packet_list_dual[i]->num_byte[j] = 0;
	}

	camera_packet_list_dual[i]->packet_state = CAM_STREAM_BUSY;

	GLOBAL_INT_RESTORE();

	return camera_packet_list_dual[i];
}

void bk_video_camera_packet_free(camera_packet_t *camera_packet)
{
	if (camera_packet == NULL)
		return;
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	camera_packet->packet_state = CAM_STREAM_IDLE;
	GLOBAL_INT_RESTORE();
}

void bk_video_camera_packet_dual_free(camera_packet_t *camera_packet)
{
	if (camera_packet == NULL)
		return;
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	camera_packet->packet_state = CAM_STREAM_IDLE;
	GLOBAL_INT_RESTORE();
}

camera_packet_t *bk_video_camera_packet_pop(void)
{
	uint8_t count = 10;

	uint8_t i = 0;

	while (1)
	{
		for (i = 0; i < list_cnt; i++)
		{
			if (camera_packet_list[i]->packet_state == CAM_STREAM_READY)
			{
				break;
			}

			count--;
			rtos_delay_milliseconds(5);
			if (count == 0)
				break;
		}
	};

	if (count == 0 && camera_packet_list[i]->packet_state != CAM_STREAM_READY)
	{
		return NULL;
	}

	return camera_packet_list[i];
}

camera_packet_t *bk_video_camera_packet_dual_pop(void)
{
	uint8_t count = 10;

	uint8_t i = 0;

	while (1)
	{
		for (i = 0; i < list_cnt; i++)
		{
			if (camera_packet_list_dual[i]->packet_state == CAM_STREAM_READY)
			{
				break;
			}

			count--;
			rtos_delay_milliseconds(5);
			if (count == 0)
				break;
		}
	};

	if (count == 0 && camera_packet_list_dual[i]->packet_state != CAM_STREAM_READY)
	{
		return NULL;
	}

	return camera_packet_list_dual[i];
}

void bk_video_camera_packet_push(camera_packet_t *camera_packet)
{
	if (camera_packet == NULL)
		return;
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	camera_packet->packet_state = CAM_STREAM_READY;
	GLOBAL_INT_RESTORE();
}

void bk_video_camera_packet_dual_push(camera_packet_t *camera_packet)
{
	if (camera_packet == NULL)
		return;
	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();
	camera_packet->packet_state = CAM_STREAM_READY;
	GLOBAL_INT_RESTORE();
}

bk_err_t bk_video_power_on(uint8_t gpio, uint8_t activ_level)
{
#if (CONFIG_CAMERA_POWER_GPIO_CTRL)
	gpio_dev_unmap(gpio);
	bk_gpio_set_capacity(gpio, 0);
	BK_LOG_ON_ERR(bk_gpio_disable_input(gpio));
	BK_LOG_ON_ERR(bk_gpio_enable_output(gpio));

	if (activ_level)
		bk_gpio_set_output_high(gpio); // high active
	else
		bk_gpio_set_output_low(gpio); // low active

	delay_ms(5);

#endif

	return BK_OK;
}

bk_err_t bk_video_power_off(uint8_t gpio, uint8_t activ_level)
{
#if (CONFIG_CAMERA_POWER_GPIO_CTRL)

	if (activ_level)
		bk_gpio_set_output_low(gpio);
	else
		bk_gpio_set_output_high(gpio);

#endif //CONFIG_CAMERA_POWER_GPIO_CTRL

	return BK_OK;
}

bk_err_t bk_video_set_mclk(mclk_freq_t mclk)
{
	int ret = BK_OK;

#if (AUXS_CLK_CIS_ENABLE && CONFIG_SOC_BK7236XX)

	gpio_dev_unmap(GPIO_27);
	gpio_dev_map(GPIO_27, GPIO_DEV_CLK_AUXS_CIS);

	sys_hal_set_cis_auxs_clk_en(1);
	switch (mclk)
	{
		case MCLK_15M:
			sys_drv_set_auxs_cis(3, 31);
			break;

		case MCLK_16M:
			sys_drv_set_auxs_cis(3, 29);
			break;

		case MCLK_20M:
			sys_drv_set_auxs_cis(3, 23);
			break;

		case MCLK_24M:
			sys_drv_set_auxs_cis(3, 19);
			break;

		case MCLK_30M:
			sys_drv_set_auxs_cis(3, 15);
			break;

		case MCLK_32M:
			sys_drv_set_auxs_cis(3, 14);
			break;

		case MCLK_40M:
			sys_drv_set_auxs_cis(3, 11);
			break;

		case MCLK_48M:
			sys_drv_set_auxs_cis(3, 9);
			break;

		default:
			return BK_FAIL;
	}
	sys_drv_set_jpeg_clk_sel(VIDEO_SYS_CLK_480M);
#endif

#if (!CONFIG_SOC_BK7236XX)
	// for 7256xx
	switch (mclk)
	{
		case MCLK_16M:
			sys_drv_set_clk_div_mode1_clkdiv_jpeg(4);
			bk_jpeg_enc_set_mclk_div(YUV_MCLK_DIV_6);
			break;

		case MCLK_20M:
			sys_drv_set_clk_div_mode1_clkdiv_jpeg(3);
			bk_jpeg_enc_set_mclk_div(YUV_MCLK_DIV_6);
			break;

		case MCLK_24M:
			sys_drv_set_clk_div_mode1_clkdiv_jpeg(4);
			bk_jpeg_enc_set_mclk_div(YUV_MCLK_DIV_4);
			break;

		case MCLK_30M:
			sys_drv_set_clk_div_mode1_clkdiv_jpeg(3);
			bk_jpeg_enc_set_mclk_div(YUV_MCLK_DIV_4);
			break;

		case MCLK_32M:
			sys_drv_set_clk_div_mode1_clkdiv_jpeg(4);
			bk_jpeg_enc_set_mclk_div(YUV_MCLK_DIV_3);
			break;

		case MCLK_40M:
			sys_drv_set_clk_div_mode1_clkdiv_jpeg(2);
			bk_jpeg_enc_set_mclk_div(YUV_MCLK_DIV_4);
			break;

		case MCLK_48M:
			sys_drv_set_clk_div_mode1_clkdiv_jpeg(3);
			bk_jpeg_enc_set_mclk_div(YUV_MCLK_DIV_6);
			break;

		default:
			return BK_FAIL;
	}
#else
	// for bk7236xx, use auxs
	sys_drv_set_jpeg_clk_sel(VIDEO_SYS_CLK_480M);
	switch (mclk)
	{
		case MCLK_16M:
		case MCLK_20M:
		case MCLK_24M:
			sys_drv_set_clk_div_mode1_clkdiv_jpeg(1);
			bk_yuv_buf_set_mclk_div(YUV_MCLK_DIV_6);
			break;

		case MCLK_30M:
			sys_drv_set_clk_div_mode1_clkdiv_jpeg(1);
			bk_yuv_buf_set_mclk_div(YUV_MCLK_DIV_4);
			break;

		case MCLK_40M:
			sys_drv_set_clk_div_mode1_clkdiv_jpeg(1);
			bk_yuv_buf_set_mclk_div(YUV_MCLK_DIV_3);
			break;

		default:
			return BK_FAIL;
	}
#endif

	return ret;
}

bk_err_t bk_video_dvp_mclk_enable(yuv_mode_t mode)
{
	// step 1: vid_power on, current only have jpeg_enc vote, need add yuv_buf/h264 .etc
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_VIDP_JPEG_EN, PM_POWER_MODULE_STATE_ON);

	// step 2: clk enable
	bk_pm_clock_ctrl(PM_CLK_ID_JPEG, CLK_PWR_CTRL_PWR_UP); //jpeg_clk
#if (CONFIG_SOC_BK7236XX)
	sys_drv_yuv_buf_pwr_up(); // yuv_buf clk enable
	sys_drv_h264_pwr_up();    // h264 clk enable
	bk_yuv_buf_soft_reset();
#endif

	// step 3: config mclk
	bk_video_set_mclk(MCLK_24M);

	// step 4: enable encode
	bk_video_encode_start(mode);

	return BK_OK;
}

bk_err_t bk_video_dvp_mclk_disable(void)
{
	bk_pm_clock_ctrl(PM_CLK_ID_JPEG, CLK_PWR_CTRL_PWR_DOWN); //jpeg_clk
#if (CONFIG_SOC_BK7236XX)
	sys_drv_yuv_buf_pwr_down(); // yuv_buf clk enable
	sys_drv_h264_pwr_down();    // h264 clk enable
#endif

#if (AUXS_CLK_CIS_ENABLE)
	sys_drv_set_cis_auxs_clk_en(0); // ausx_clk disable
#endif

	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_VIDP_JPEG_EN, PM_POWER_MODULE_STATE_OFF);

	return BK_OK;
}

bk_err_t bk_video_encode_start(yuv_mode_t mode)
{
	switch (mode)
	{
		case YUV_MODE:
		case GRAY_MODE:
			#if (CONFIG_SOC_BK7236XX)
			bk_yuv_buf_start(YUV_MODE);
			#else
			bk_jpeg_enc_start(YUV_MODE);
			#endif
			break;

		case JPEG_MODE:
		case JPEG_YUV_MODE:
			#if (CONFIG_SOC_BK7236XX)
			bk_yuv_buf_start(JPEG_MODE);
			#else
			bk_jpeg_enc_start(JPEG_MODE);
			#endif
			break;

		case H264_MODE:
		case H264_YUV_MODE:
			#if (CONFIG_SOC_BK7236XX)
			//BK_LOG_ON_ERR(bk_gpio_enable_input(CAMERA_DVP_VSYNC_PIN));
			bk_yuv_buf_start(H264_MODE);
			bk_h264_encode_enable();
			#endif
			break;

		default:
			break;
	}

	return BK_OK;
}

bk_err_t bk_video_encode_stop(yuv_mode_t mode)
{
	switch (mode)
	{
		case YUV_MODE:
		case GRAY_MODE:
			#if (CONFIG_SOC_BK7236XX)
			bk_yuv_buf_stop(YUV_MODE);
			#else
			bk_jpeg_enc_stop(YUV_MODE);
			#endif
			break;

		case JPEG_MODE:
		case JPEG_YUV_MODE:
			#if (CONFIG_SOC_BK7236XX)
			bk_yuv_buf_stop(JPEG_MODE);
			#else
			bk_jpeg_enc_stop(JPEG_MODE);
			#endif
			break;

		case H264_MODE:
		case H264_YUV_MODE:
			#if (CONFIG_SOC_BK7236XX)
			bk_yuv_buf_stop(H264_MODE);
			#endif
			break;

		default:
			break;
	}

	return BK_OK;
}

bk_err_t bk_video_gpio_init(dvp_gpio_mode_t mode)
{
	camera_gpio_map_t camera_gpio_map_table[] = VIDEO_GPIO_MAP;

	if (mode == DVP_GPIO_CLK)
	{
		for (uint32_t i = 0; i < 2; i++) {
			gpio_dev_unmap(camera_gpio_map_table[i].gpio_id);
			gpio_dev_map(camera_gpio_map_table[i].gpio_id, camera_gpio_map_table[i].dev);
		}
	}
	else if (mode == DVP_GPIO_DATA)
	{
		for (uint32_t i = 2; i < VIDEO_GPIO_PIN_NUMBER; i++) {
			gpio_dev_unmap(camera_gpio_map_table[i].gpio_id);
			gpio_dev_map(camera_gpio_map_table[i].gpio_id, camera_gpio_map_table[i].dev);
		}
	}
	else if (mode == DVP_GPIO_HSYNC_DATA)
	{
		gpio_dev_unmap(camera_gpio_map_table[2].gpio_id);
		gpio_dev_map(camera_gpio_map_table[2].gpio_id, camera_gpio_map_table[2].dev);
		for (uint32_t i = 4; i < VIDEO_GPIO_PIN_NUMBER; i++) {
			gpio_dev_unmap(camera_gpio_map_table[i].gpio_id);
			gpio_dev_map(camera_gpio_map_table[i].gpio_id, camera_gpio_map_table[i].dev);
		}
	}
	else // DVP_GPIO_ALL
	{
		for (uint32_t i = 0; i < VIDEO_GPIO_PIN_NUMBER; i++) {
			gpio_dev_unmap(camera_gpio_map_table[i].gpio_id);
			gpio_dev_map(camera_gpio_map_table[i].gpio_id, camera_gpio_map_table[i].dev);
		}
	}

	return BK_OK;
}

bk_err_t bk_video_gpio_deinit(dvp_gpio_mode_t mode)
{
	camera_gpio_map_t camera_gpio_map_table[] = VIDEO_GPIO_MAP;

	if (mode == DVP_GPIO_CLK)
	{
		for (uint32_t i = 0; i < 2; i++) {
			gpio_dev_unmap(camera_gpio_map_table[i].gpio_id);
		}
	}
	else if (mode == DVP_GPIO_DATA)
	{
		for (uint32_t i = 2; i < VIDEO_GPIO_PIN_NUMBER; i++) {
			gpio_dev_unmap(camera_gpio_map_table[i].gpio_id);
		}
	}
	else if (mode == DVP_GPIO_HSYNC_DATA)
	{
		gpio_dev_unmap(camera_gpio_map_table[2].gpio_id);
		for (uint32_t i = 4; i < VIDEO_GPIO_PIN_NUMBER; i++) {
			gpio_dev_unmap(camera_gpio_map_table[i].gpio_id);
		}
	}
	else //DVP_GPIO_ALL
	{
		for (uint32_t i = 0; i < VIDEO_GPIO_PIN_NUMBER; i++) {
			gpio_dev_unmap(camera_gpio_map_table[i].gpio_id);
		}
	}

	return BK_OK;
}

typedef struct
{
	uint8_t encode_line_odd; // encode line flag
	uint8_t *src_buffer;
	uint8_t dma_channel;
	beken_semaphore_t sem; // wait encode success
	yuv_mode_t mode; // encode mode:h264/jpeg
	pixel_format_t fmt; // input data format
	frame_resl_t resolution; // image size
	frame_buffer_t *src_frame;
	frame_buffer_t *dst_frame;
	uint32_t node_length;
	uint32_t rx_encode_length;
	uint32_t rx_read_length;
} yuv_encode_config_t;

static yuv_encode_config_t *yuv_encode_config = NULL;

static bk_err_t yuv_encode_memcpy_by_chnl(void *out, const void *in, uint32_t len, dma_id_t cpy_chnl)
{
	dma_config_t dma_config = {0};

	os_memset(&dma_config, 0, sizeof(dma_config_t));

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;

	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.src.start_addr = (uint32_t)in;
	dma_config.src.end_addr = (uint32_t)(in + len);

	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.start_addr = (uint32_t)out;
	dma_config.dst.end_addr = (uint32_t)(out + len);


	bk_dma_init(cpy_chnl, &dma_config);
	bk_dma_set_transfer_len(cpy_chnl, len);
#if (CONFIG_SPE)

	bk_dma_set_src_sec_attr(cpy_chnl, DMA_ATTR_SEC);
	bk_dma_set_dest_sec_attr(cpy_chnl, DMA_ATTR_SEC);
#endif
	bk_dma_start(cpy_chnl);

	BK_WHILE(bk_dma_get_enable_status(cpy_chnl));

	return BK_OK;
}

static void yuv_encode_line_done_handler(jpeg_unit_t id, void *param)
{
	if (yuv_encode_config->encode_line_odd)
	{
		yuv_encode_config->encode_line_odd = 0;
		yuv_encode_config->rx_read_length += yuv_encode_config->rx_encode_length;
		bk_yuv_buf_set_em_base_addr((uint32_t)(yuv_encode_config->src_frame->frame + yuv_encode_config->rx_read_length));
	}
	else
	{
		yuv_encode_config->encode_line_odd = 1;
	}

	bk_yuv_buf_rencode_start();
}

static void yuv_encode_final_out_handler(jpeg_unit_t id, void *param)
{
	uint32_t real_length = 0;
	uint32_t remain_length = 0;

	bk_dma_flush_src_buffer(yuv_encode_config->dma_channel);
	bk_dma_stop(yuv_encode_config->dma_channel);

	remain_length = yuv_encode_config->node_length - bk_dma_get_remain_len(yuv_encode_config->dma_channel);

	if (yuv_encode_config->mode == JPEG_MODE)
	{
		real_length = bk_jpeg_enc_get_frame_size();
		if (yuv_encode_config->dst_frame->length + remain_length - JPEG_CRC_SIZE != real_length)
		{
			LOGW("size no match: %u:%u\n", yuv_encode_config->dst_frame->length + remain_length - JPEG_CRC_SIZE, real_length);
		}
		yuv_encode_config->dst_frame->fmt = PIXEL_FMT_JPEG;
	}
	else
	{
		real_length = bk_h264_get_encode_count() * 4;
		if (yuv_encode_config->dst_frame->length + remain_length != real_length)
		{
			LOGW("size no match: %u:%u\n", yuv_encode_config->dst_frame->length + remain_length, real_length);
		}
		yuv_encode_config->dst_frame->fmt = PIXEL_FMT_H264;
	}

	bk_video_encode_stop(yuv_encode_config->mode);

	//yuv_encode_config->dst_frame->length += remain_length;
	yuv_encode_config->dst_frame->length = real_length;
	yuv_encode_config->dst_frame->sequence = yuv_encode_config->src_frame->sequence;
	yuv_encode_config->dst_frame->width = yuv_encode_config->resolution.width;
	yuv_encode_config->dst_frame->height = yuv_encode_config->resolution.height;

	rtos_set_semaphore(&yuv_encode_config->sem);
}

static void yuv_encode_head_output_handler(jpeg_unit_t id, void *param)
{
	bk_yuv_buf_rencode_start();
}

static void yuv_encode_dma_finish_callback(dma_id_t id)
{
	yuv_encode_config->dst_frame->length += yuv_encode_config->node_length;
}

static bk_err_t yuv_encode_dma_config(yuv_mode_t mode)
{
	uint32_t encode_fifo_addr;

	if (mode == H264_MODE)
	{
		bk_h264_get_fifo_addr(&encode_fifo_addr);
		yuv_encode_config->dma_channel = bk_dma_alloc(DMA_DEV_H264);
	}
	else
	{
		bk_jpeg_enc_get_fifo_addr(&encode_fifo_addr);
		yuv_encode_config->dma_channel = bk_dma_alloc(DMA_DEV_JPEG);
	}

	LOGI("dma for encode is %x \r\n", yuv_encode_config->dma_channel);

	dma_config_t dma_config = {0};
	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_H264;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = encode_fifo_addr;

	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.start_addr = 0x60000000;//(uint32_t)yuv_encode_config->frame->frame;
	dma_config.dst.end_addr = 0x60800000;//(uint32_t)(yuv_encode_config->frame->frame + yuv_encode_config->frame->size);

	BK_LOG_ON_ERR(bk_dma_init(yuv_encode_config->dma_channel, &dma_config));
	BK_LOG_ON_ERR(bk_dma_register_isr(yuv_encode_config->dma_channel, NULL, yuv_encode_dma_finish_callback));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(yuv_encode_config->dma_channel));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(yuv_encode_config->dma_channel, yuv_encode_config->node_length));
#if (CONFIG_SPE)
	BK_LOG_ON_ERR(bk_dma_set_src_burst_len(yuv_encode_config->dma_channel, BURST_LEN_SINGLE));
	BK_LOG_ON_ERR(bk_dma_set_dest_burst_len(yuv_encode_config->dma_channel, BURST_LEN_INC16));
	BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(yuv_encode_config->dma_channel, DMA_ATTR_SEC));
	BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(yuv_encode_config->dma_channel, DMA_ATTR_SEC));
#endif

	return BK_OK;
}

bk_err_t bk_video_yuv_encode_init(media_camera_device_t *device)
{
	int ret = BK_OK;
	uint32_t encode_buffer_length = 0;
	yuv_buf_config_t yuv_config = {0};
	jpeg_config_t  jpeg_config = {0};

	// encode_clk 120M
	sys_drv_set_jpeg_clk_sel(VIDEO_SYS_CLK_480M);
	sys_drv_set_clk_div_mode1_clkdiv_jpeg(1);

	if (device->info.resolution.width * device->info.resolution.height > 1280 * 720)
	{
		LOGE("%s, not support more than 1280X720 resolution\r\n", __func__);
		ret = BK_FAIL;
		goto error;
	}

	if (device->mode == JPEG_MODE)
	{
		encode_buffer_length = device->info.resolution.width * 2 * 8 * 2;
	}
	else if (device->mode == H264_MODE)
	{
		encode_buffer_length = device->info.resolution.width * 2 * 16 * 2;
	}
	else
	{
		LOGE("%s, not support this mode:%d!\r\n", __func__, device->mode);
		ret = BK_FAIL;
		goto error;
	}

	if (yuv_encode_config == NULL)
	{
		yuv_encode_config = (yuv_encode_config_t *)os_malloc(sizeof(yuv_encode_config_t));
		if (yuv_encode_config == NULL)
		{
			LOGE("%s, malloc yuv_encode_config failed!\r\n", __func__);
			ret = BK_FAIL;
			goto error;
		}

		os_memset(yuv_encode_config, 0, sizeof(yuv_encode_config_t));

		ret = rtos_init_semaphore(&yuv_encode_config->sem, 1);
		if (yuv_encode_config->sem == NULL)
		{
			LOGE("%s, malloc yuv_encode_config->sem failed!\r\n", __func__);
			ret = BK_FAIL;
			goto error;
		}
	}

	yuv_encode_config->resolution.width = device->info.resolution.width;
	yuv_encode_config->resolution.height = device->info.resolution.height;
	yuv_encode_config->mode = device->mode;
	yuv_encode_config->fmt = device->fmt;

	yuv_config.x_pixel = yuv_encode_config->resolution.width / 8;
	yuv_config.y_pixel = yuv_encode_config->resolution.height / 8;
	yuv_config.work_mode = yuv_encode_config->mode;
	yuv_config.base_addr = NULL;
	yuv_config.yuv_mode_cfg.yuv_format = YUV_FORMAT_YUYV;

	ret = bk_yuv_buf_init(&yuv_config);
	if (ret != BK_OK)
	{
		LOGE("%s, init yuv_buf error\r\n", __func__);
		goto error;
	}

	bk_yuv_buf_enable_nosensor_encode_mode();

	if (yuv_encode_config->mode == JPEG_MODE)
	{
		jpeg_config.x_pixel = yuv_config.x_pixel;
		jpeg_config.y_pixel = yuv_config.y_pixel;
		jpeg_config.mode = yuv_config.work_mode;

		ret = bk_jpeg_enc_init(&jpeg_config);

		bk_jpeg_enc_register_isr(JPEG_LINE_CLEAR, yuv_encode_line_done_handler, NULL);
		bk_jpeg_enc_register_isr(JPEG_EOF, yuv_encode_final_out_handler, NULL);
		bk_jpeg_enc_register_isr(JPEG_HEAD_OUTPUT, yuv_encode_head_output_handler, NULL);
	}
	else
	{
		ret = bk_h264_init((yuv_encode_config->resolution.width << 16) | yuv_encode_config->resolution.height);

		/* register h264 callback */
		bk_h264_register_isr(H264_LINE_DONE, yuv_encode_line_done_handler, 0);
		bk_h264_register_isr(H264_FINAL_OUT, yuv_encode_final_out_handler, 0);
	}

	if (ret != BK_OK)
	{
		LOGE("%s, init encode module error\r\n", __func__);
		goto error;
	}

	yuv_encode_config->node_length = 1024 * 5;
	yuv_encode_config->rx_encode_length = encode_buffer_length;
	yuv_encode_config->rx_read_length = 0;

	ret = yuv_encode_dma_config(yuv_encode_config->mode);
	if (ret != BK_OK)
	{
		LOGE("%s, init dma error\r\n", __func__);
		goto error;
	}

	LOGI("%s complete\n", __func__);

	return ret;

error:

	bk_video_yuv_encode_deinit();

	return ret;
}

bk_err_t bk_video_yuv_encode_deinit(void)
{
	if (yuv_encode_config == NULL)
	{
		return BK_OK;
	}

	// step 1: stop encode
	bk_video_encode_stop(yuv_encode_config->mode);

	bk_h264_deinit();

	bk_jpeg_enc_deinit();

	if (yuv_encode_config)
	{
		if (yuv_encode_config->dma_channel < DMA_ID_MAX)
		{
			bk_dma_stop(yuv_encode_config->dma_channel);
			bk_dma_deinit(yuv_encode_config->dma_channel);

			if (yuv_encode_config->mode == JPEG_MODE)
				bk_dma_free(DMA_DEV_JPEG, yuv_encode_config->dma_channel);
			else
				bk_dma_free(DMA_DEV_H264, yuv_encode_config->dma_channel);
		}

		if (yuv_encode_config->sem)
		{
			rtos_deinit_semaphore(&yuv_encode_config->sem);
			yuv_encode_config->sem = NULL;
		}

		os_free(yuv_encode_config);
		yuv_encode_config = NULL;
	}

	return BK_OK;
}

bk_err_t bk_video_yuv_encode_start(frame_buffer_t *src_frame, frame_buffer_t *dst_frame)
{
	if (yuv_encode_config == NULL)
	{
		return BK_FAIL;
	}

	yuv_encode_config->dst_frame = dst_frame;
	yuv_encode_config->src_frame = src_frame;
	yuv_encode_config->rx_read_length = 0;
	yuv_encode_config->encode_line_odd = 0;

	bk_yuv_buf_set_em_base_addr((uint32_t)src_frame->frame);

	bk_dma_set_dest_addr(yuv_encode_config->dma_channel, (uint32_t)dst_frame->frame, (uint32_t)(dst_frame->frame + dst_frame->size));

	bk_dma_start(yuv_encode_config->dma_channel);

	bk_video_encode_start(yuv_encode_config->mode);

	bk_yuv_buf_rencode_start();

	if (rtos_get_semaphore(&yuv_encode_config->sem, 500) != BK_OK)//BEKEN_WAIT_FOREVER
	{
		bk_video_encode_stop(yuv_encode_config->mode);
		yuv_encode_config->encode_line_odd = 0;
		bk_dma_stop(yuv_encode_config->dma_channel);
		return BK_FAIL;
	}

	yuv_encode_config->encode_line_odd = 0;
	return BK_OK;
}

uint8_t *dvp_camera_yuv_base_addr_init(frame_resl_t resolution, yuv_mode_t mode)
{
	uint8_t *encode_buffer = NULL;

#if CONFIG_ENCODE_BUF_DYNAMIC
	if (encode_buffer == NULL)
	{
		if (mode == H264_MODE)
		{
			encode_buffer = (uint8_t *)os_malloc(resolution.width * 32 * 2);
		}
		else
			encode_buffer = (uint8_t *)os_malloc(resolution.width * 16 * 2);


		if (encode_buffer == NULL)
		{
			LOGE("yuv_buf base addr init failed!\n");
		}
	}
#endif

	return encode_buffer;
}

uint32_t bk_video_identify_h264_nal_fmt(uint8_t *buf, uint32_t size)
{
	uint32_t h264_type = 0;
	uint8_t nal_type = 0;
	uint8_t i = 0;
	uint8_t nal_ref_idc = 0;
	if (size < 128)
		return h264_type;

	for (i = 0; i < 128; i++)
	{
		if (buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 0 && buf[i + 3] == 1)
		{
			i += 4;
			nal_ref_idc = (buf[i] >> 5) & 0x3;
			nal_type = buf[i] & 0x1f;
			h264_type |= 0x1 << nal_type;
			if (nal_type == 1)
			{
				if (nal_ref_idc > 0)
					h264_type |= (0x1 << H264_NAL_I_FRAME);
				else
					h264_type |= (0x1 << H264_NAL_P_FRAME);
			}
		}
	}

	return h264_type;
}

bk_err_t bk_video_compression_ratio_config(compress_ratio_t *config)
{
	int ret = BK_FAIL;
	if (!config)
	{
		LOGE("%s, param error\n", __func__);
		return ret;
	}

	if (config->mode == JPEG_MODE)
	{
		ret = bk_jpeg_enc_encode_config(config->enable, config->jpeg_up, config->jpeg_low);
	}
	else if (config->mode == H264_MODE)
	{
		ret = bk_h264_set_base_config(config);
	}
	else
	{
		LOGE("%s, mode:%d error\n", __func__, config->mode);
		return BK_FAIL;
	}

	return ret;
}

