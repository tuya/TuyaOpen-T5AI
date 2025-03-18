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
#include <string.h>
#include <stdlib.h>

#include <os/os.h>
#include <components/log.h>

#include "media_core.h"
#include "media_evt.h"
#include "storage_act.h"
#include "lcd_act.h"
#include "frame_buffer.h"

#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>

#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>


#include <soc/mapping.h>

#include <driver/lcd.h>
#include <driver/dma.h>
#include <driver/gpio.h>
#include <driver/jpeg_dec.h>
#include <driver/dma2d.h>
#include <driver/jpeg_dec_types.h>
#include "modules/image_scale.h"
#include <driver/uvc_camera_types.h>
#include <driver/uvc_camera.h>

#include <driver/timer.h>
#include <driver/psram.h>
#include "gpio_driver.h"

#include "driver/flash.h"
#include "driver/flash_partition.h"
#include "beken_image.h"
#include <modules/jpeg_decode_sw.h>
#include <os/str.h>
#include "modules/image_scale.h"
#include <driver/dma2d.h>
#include "modules/lcd_font.h"
#include <driver/media_types.h>

#include <lcd_decode.h>
#include <lcd_rotate.h>
#include <lcd_scale.h>
#include <camera_act.h>

#include <driver/uvc_camera_types.h>
#include <driver/uvc_camera.h>
#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>

#if CONFIG_LCD_QSPI
#include <lcd_qspi_display_service.h>
#endif
#ifdef CONFIG_LVGL
#include "lv_vendor.h"
#include "lvgl.h"
#endif
#if CONFIG_CACHE_ENABLE
#include "cache.h"
#endif
#include "yuv_encode.h"

//#include "display_array.h"

#include "lcd_disp_hal.h"
#include "draw_blend.h"
#include "lcd_draw_blend.h"

#define LCD_RAM_WRITE          0x2c


#define TAG "lcd_act"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

//#define LCD_DIAG_DEBUG

#ifdef LCD_DIAG_DEBUG

#define LCD_DIAG_DEBUG_INIT()                   \
	do {                                        \
		gpio_dev_unmap(GPIO_2);                 \
		bk_gpio_disable_input((GPIO_2));        \
		bk_gpio_enable_output(GPIO_2);          \
		bk_gpio_set_output_low(GPIO_2);         \
		\
		gpio_dev_unmap(GPIO_3);                 \
		bk_gpio_disable_input((GPIO_3));        \
		bk_gpio_enable_output(GPIO_3);          \
		bk_gpio_set_output_low(GPIO_3);         \
		\
		gpio_dev_unmap(GPIO_4);                 \
		bk_gpio_disable_input((GPIO_4));        \
		bk_gpio_enable_output(GPIO_4);          \
		bk_gpio_set_output_low(GPIO_4);         \
		\
		gpio_dev_unmap(GPIO_5);                 \
		bk_gpio_disable_pull(GPIO_5);           \
		bk_gpio_enable_output(GPIO_5);          \
		bk_gpio_set_output_low(GPIO_5);         \
		\
		gpio_dev_unmap(GPIO_8);                 \
		bk_gpio_disable_input((GPIO_8));        \
		bk_gpio_enable_output(GPIO_8);          \
		bk_gpio_set_output_low(GPIO_8);         \
		\
	} while (0)

#define LCD_DECODER_START()                 bk_gpio_set_output_high(GPIO_2)
#define LCD_DECODER_END()                   bk_gpio_set_output_low(GPIO_2)

#define LCD_ROTATE_START()                  bk_gpio_set_output_high(GPIO_3)
#define LCD_ROTATE_END()                    bk_gpio_set_output_low(GPIO_3)

#define LCD_DISPLAY_START()                 bk_gpio_set_output_high(GPIO_4)
#define LCD_DISPLAY_END()                   bk_gpio_set_output_low(GPIO_4)

#define LCD_DISPLAY_ISR_ENTRY()             bk_gpio_set_output_high(GPIO_5)
#define LCD_DISPLAY_ISR_OUT()               bk_gpio_set_output_low(GPIO_5)

#define LCD_RESIZE_END()                    bk_gpio_set_output_high(GPIO_8)
#define LCD_RESIZE_START()                  bk_gpio_set_output_high(GPIO_8)
#else
#define LCD_DIAG_DEBUG_INIT()

#define LCD_DECODER_START()
#define LCD_DECODER_END()

#define LCD_ROTATE_START()
#define LCD_ROTATE_END()

#define LCD_DISPLAY_START()
#define LCD_DISPLAY_END()

#define LCD_DISPLAY_ISR_ENTRY()
#define LCD_DISPLAY_ISR_OUT()

#define LCD_RESIZE_END()
#define LCD_RESIZE_START()

#endif

#ifdef LCD_DIAG_DEBUG
void lcd_debug_en(void)
{
	gpio_dev_unmap(GPIO_2);
	BK_LOG_ON_ERR(bk_gpio_disable_input((GPIO_2)));
	bk_gpio_enable_output(GPIO_2);
	bk_gpio_set_output_low(GPIO_2);

	gpio_dev_unmap(GPIO_3);
	BK_LOG_ON_ERR(bk_gpio_disable_input((GPIO_3)));
	bk_gpio_enable_output(GPIO_3);
	bk_gpio_set_output_low(GPIO_3);

	gpio_dev_unmap(GPIO_4);
	BK_LOG_ON_ERR(bk_gpio_disable_input((GPIO_4)));
	bk_gpio_enable_output(GPIO_4);
	bk_gpio_set_output_low(GPIO_4);

	gpio_dev_unmap(GPIO_5);
	bk_gpio_disable_pull(GPIO_5);
	bk_gpio_enable_output(GPIO_5);
	bk_gpio_set_output_low(GPIO_5);

	gpio_dev_unmap(GPIO_8);
	BK_LOG_ON_ERR(bk_gpio_disable_input((GPIO_8)));
	bk_gpio_enable_output(GPIO_8);
	bk_gpio_set_output_low(GPIO_8);
}
#endif

#define LCD_DRIVER_FRAME_FREE(frame)			\
	do {										\
		if (frame								\
			&& lcd_info.lvgl_frame != frame)		\
		{										\
			lcd_info.fb_free(frame);		\
			frame = NULL;						\
		}										\
	} while (0)

extern media_debug_t *media_debug;
beken_semaphore_t step_sem;

/**< camera jpeg display */
beken_semaphore_t jpeg_decoder_sem;
bool jpeg_decoder_task_running = false;
static beken_thread_t jpeg_decoder_task = NULL;

/**< lcd jpeg display */
#define DISPLAY_PIPELINE_TASK
#ifdef DISPLAY_PIPELINE_TASK
beken_semaphore_t lcd_display_sem;
bool lcd_display_task_running = false;
static beken_thread_t lcd_display_task = NULL;
#endif

static frame_buffer_t *dbg_jpeg_frame = NULL;
static frame_buffer_t *dbg_display_frame = NULL;

lcd_info_t lcd_info = {0};
static lcd_config_t lcd_config = DEFAULT_LCD_CONFIG();

#if (CONFIG_LCD_QSPI && CONFIG_LVGL)

#else
u8 g_gui_need_to_wait = BK_FALSE;
#endif
static u8 g_dma2d_use_flag = 0;

static beken_semaphore_t dma2d_complete_sem = NULL;



static char *frame_suffix(pixel_format_t fmt)
{
	switch (fmt)
	{
		case PIXEL_FMT_UNKNOW:
			return ".unknow";
		case PIXEL_FMT_JPEG:
			return "_dvp.jpg";
		case PIXEL_FMT_H264:
			return ".h264";
		case PIXEL_FMT_RGB565_LE:
			return ".rgb565le";
		case PIXEL_FMT_YUYV:
			return ".yuyv";
		case PIXEL_FMT_UYVY:
			return ".uyvy";
		case PIXEL_FMT_YYUV:
			return ".yyuv";
		case PIXEL_FMT_UVYY:
			return ".uvyy";
		case PIXEL_FMT_VUYY:
			return ".vuyy";
		default:
			break;
	}

	return ".unknow";
}

#ifdef DISPLAY_PIPELINE_TASK
static void lcd_display_task_entry(beken_thread_arg_t data)
{
	frame_buffer_t *frame = NULL;
	media_rotate_t rotate = (media_rotate_t)data;

	LOGI("%s, rotate: %d\n", __func__, rotate);

	rtos_set_semaphore(&lcd_display_sem);

	while (lcd_display_task_running)
	{
		rotate = lcd_info.rotate;
		frame = frame_buffer_fb_display_pop_wait();
		if (frame == NULL)
		{
			LOGD("read display frame NULL\n");
			continue;
		}

#if CONFIG_LCD_AUTO_FILL_FULL
		dma2d_fill_height_head_tail(frame, ((frame->width << 16) | frame->height), lcd_info.lcd_device->ppi, 0x00);
#endif
		if (lcd_info.rotate_en)
		{
			frame = lcd_driver_rotate_frame(frame, rotate);

			if (frame == NULL)
			{
				LOGD("rotate frame NULL\n");
				continue;
			}
		}
		if (lcd_info.scale_en)
		{
			frame = lcd_driver_scale_frame(frame, lcd_info.scale_ppi);

			if (frame == NULL)
			{
				LOGE("scale frame NULL\n");
				continue;
			}
		}
#if CONFIG_LCD_FONT_BLEND
			if(lcd_info.font_draw)
				lcd_font_handle(frame, lcd_info.lcd_width, lcd_info.lcd_height);
#endif
#if CONFIG_LCD_DMA2D_BLEND
			if(lcd_info.dma2d_blend)
				lcd_dma2d_handle(frame,  lcd_info.lcd_width, lcd_info.lcd_height);
#endif

		lcd_driver_display_frame(frame);
	}

	LOGI("lcd display task exit\n");

	lcd_display_task = NULL;
	rtos_set_semaphore(&lcd_display_sem);
	rtos_delete_thread(NULL);
}


void lcd_display_task_start(media_rotate_t rotate)
{
	bk_err_t ret;

	if (lcd_display_task != NULL)
	{
		LOGE("%s lcd_display_task_thread already running\n", __func__);
		return;
	}

	frame_buffer_fb_register(MODULE_LCD, FB_INDEX_DISPLAY);

	ret = rtos_init_semaphore_ex(&lcd_display_sem, 1, 0);

	if (BK_OK != ret)
	{
		LOGE("%s semaphore init failed\n", __func__);
		return;
	}

	lcd_display_task_running = true;

	ret = rtos_create_thread(&lcd_display_task,
	                         6,
	                         "lcd_display_thread",
	                         (beken_thread_function_t)lcd_display_task_entry,
	                         4 * 1024,
	                         (beken_thread_arg_t)rotate);

	if (BK_OK != ret)
	{
		LOGE("%slcd_display_thread init failed\n", __func__);
		return;
	}

	ret = rtos_get_semaphore(&lcd_display_sem, BEKEN_NEVER_TIMEOUT);

	if (BK_OK != ret)
	{
		LOGE("%s decoder_sem get failed\n", __func__);
	}
}

void lcd_display_task_stop(void)
{
	bk_err_t ret;

	if (lcd_display_task_running == false)
	{
		LOGI("%s already stop\n", __func__);
		return;
	}

	lcd_display_task_running = false;

	frame_buffer_fb_deregister(MODULE_LCD, FB_INDEX_DISPLAY);

	ret = rtos_get_semaphore(&lcd_display_sem, BEKEN_NEVER_TIMEOUT);

	if (BK_OK != ret)
	{
		LOGE("%s lcd_display_sem get failed\n", __func__);
	}

	LOGI("%s complete\n", __func__);

	ret = rtos_deinit_semaphore(&lcd_display_sem);

	if (BK_OK != ret)
	{
		LOGE("%s lcd_display_sem deinit failed\n", __func__);
	}
}

#endif

static void decoder_task_entry(beken_thread_arg_t data)
{
	frame_buffer_t *jpeg_frame = NULL;
	frame_buffer_t *dec_frame = NULL;
	media_rotate_t rotate = (media_rotate_t)data;

	rtos_set_semaphore(&jpeg_decoder_sem);

	while (jpeg_decoder_task_running)
	{
		if (lcd_info.step_mode == false)
		{
			/* Normal display workflow */
			jpeg_frame = frame_buffer_fb_read(MODULE_DECODER);
			if (jpeg_frame == NULL)
			{
				LOGD("%s read jpeg frame NULL\n", __func__);
				continue;
			}

			// if dvp work jpeg_yuv mode, use yuv frame display direct and jpeg data not decoder
			if (jpeg_frame->mix)
			{
				frame_buffer_fb_free(jpeg_frame, MODULE_DECODER);
				continue;
			}

			dec_frame = lcd_driver_decoder_frame(jpeg_frame, lcd_info.decode_mode);

			frame_buffer_fb_free(jpeg_frame, MODULE_DECODER);

			if (dec_frame == NULL)
			{
				LOGD("jpeg decoder frame NULL\n");
				continue;
			}

#ifdef  DISPLAY_PIPELINE_TASK
			frame_buffer_fb_push(dec_frame);
#else
			rotate = lcd_info.rotate;
			if (lcd_info.scale_en)
			{
				//dec_frame = lcd_driver_scale_frame(dec_frame, lcd_info.scale_ppi);

				if (dec_frame == NULL)
				{
					LOGD("scale frame NULL\n");
					continue;
				}
			}

			if (rotate != ROTATE_NONE)
			{
				dec_frame = lcd_driver_rotate_frame(dec_frame, rotate);

				if (dec_frame == NULL)
				{
					LOGD("rotate frame NULL\n");
					continue;
				}
			}

#if CONFIG_LCD_FONT_BLEND
			if(lcd_info.font_draw)
				lcd_font_handle(dec_frame, lcd_info.lcd_width, lcd_info.lcd_height);
#endif
#if CONFIG_LCD_DMA2D_BLEND
			if(lcd_info.dma2d_blend)
				lcd_dma2d_handle(dec_frame, lcd_info.lcd_width, lcd_info.lcd_height);
#endif

			lcd_driver_display_frame(dec_frame);
#endif
		}
		else
		{
			/* Debug display workflow */
			if (rtos_get_semaphore(&step_sem, BEKEN_NEVER_TIMEOUT))
			{
				LOGE("%s step_sem get failed\n", __func__);
			}

			if (jpeg_frame)
			{
				LOGI("free frame: %u\n", jpeg_frame->sequence);
				frame_buffer_fb_free(jpeg_frame, MODULE_DECODER);
				jpeg_frame = NULL;
			}

			jpeg_frame = frame_buffer_fb_read(MODULE_DECODER);

			if (jpeg_frame == NULL)
			{
				LOGD("read jpeg frame NULL\n");
				continue;
			}

			dbg_jpeg_frame = jpeg_frame;

			LOGI("Got jpeg frame seq: %u, ptr: %p, length: %u, fmt: %u\n",
			     dbg_jpeg_frame->sequence, dbg_jpeg_frame->frame, dbg_jpeg_frame->length, dbg_jpeg_frame->fmt);

			dec_frame = lcd_driver_decoder_frame(jpeg_frame, lcd_info.decode_mode);

			if (dec_frame == NULL)
			{
				LOGD("jpeg decoder frame NULL\n");
				continue;
			}

			if (lcd_info.scale_en == true)
			{
				//dec_frame = lcd_driver_scale_frame(dec_frame, lcd_info.scale_ppi);

				if (dec_frame == NULL)
				{
					LOGD("scale frame NULL\n");
					continue;
				}
			}

			if (rotate == true)
			{
				dec_frame = lcd_driver_rotate_frame(dec_frame, lcd_info.rotate);

				if (dec_frame == NULL)
				{
					LOGD("rotate frame NULL\n");
					continue;
				}
			}

			dbg_display_frame = dec_frame;

			LOGI("Got display frame seq: %u, ptr: %p, length: %u, fmt: %u\n",
			     dbg_display_frame->sequence, dbg_display_frame->frame, dbg_display_frame->length, dbg_display_frame->fmt);

			lcd_driver_display_frame(dec_frame);

		}
	}
	LOGI("camera display task exit\n");

	jpeg_decoder_task = NULL;
	rtos_set_semaphore(&jpeg_decoder_sem);

	rtos_delete_thread(NULL);
}

void lcd_decoder_task_start(media_rotate_t rotate)
{
	bk_err_t ret;

	if (jpeg_decoder_task != NULL)
	{
		LOGE("%s lcd decode thread already running\n", __func__);
		return;
	}

	frame_buffer_fb_register(MODULE_DECODER, FB_INDEX_JPEG);

	ret = rtos_init_semaphore_ex(&jpeg_decoder_sem, 1, 0);

	if (BK_OK != ret)
	{
		LOGE("%s semaphore init failed\n", __func__);
		return;
	}

	jpeg_decoder_task_running = true;

	ret = rtos_create_thread(&jpeg_decoder_task,
	                         6,
	                         "lcd_decoder_thread",
	                         (beken_thread_function_t)decoder_task_entry,
	                         4 * 1024,
	                         (beken_thread_arg_t)rotate);

	if (BK_OK != ret)
	{
		LOGE("%s lcd decoder task init failed\n", __func__);
		return;
	}

	ret = rtos_get_semaphore(&jpeg_decoder_sem, BEKEN_NEVER_TIMEOUT);

	if (BK_OK != ret)
	{
		LOGE("%s jpeg_decoder_sem get failed\n", __func__);
	}
}

void jpeg_decoder_task_stop(void)
{
	bk_err_t ret;
	if (jpeg_decoder_task_running == false)
	{
		LOGI("%s already stop\n", __func__);
		return;
	}
	jpeg_decoder_task_running = false;

	frame_buffer_fb_deregister(MODULE_DECODER, FB_INDEX_JPEG);
	ret = rtos_get_semaphore(&jpeg_decoder_sem, BEKEN_NEVER_TIMEOUT);

	if (BK_OK != ret)
	{
		LOGE("%s jpeg_decoder_sem get failed\n", __func__);
	}

	ret = rtos_deinit_semaphore(&jpeg_decoder_sem);

	if (BK_OK != ret)
	{
		LOGE("%s decoder_sem deinit failed\n", __func__);
	}

	LOGI("%s complete\n", __func__);
}

#if CONFIG_LV_ATTRIBUTE_FAST_MEM
static void lcd_driver_display_rgb_isr(void)
#else
__attribute__((section(".itcm_sec_code"))) static void lcd_driver_display_rgb_isr(void)
#endif
{
	LCD_DISPLAY_ISR_ENTRY();

	media_debug->isr_lcd++;

	if (lcd_info.enable == false)
	{
		LOGE("%s lcd_info.enable = false\n", __func__);
		return;
	}

	if (lcd_info.pingpong_frame != NULL)
	{
//		lcd_driver_ppi_set(lcd_info.pingpong_frame->width, lcd_info.pingpong_frame->height);
//		bk_lcd_set_yuv_mode(lcd_info.pingpong_frame->fmt);

		if (lcd_info.display_frame != NULL)
		{
			media_debug->fps_lcd++;

			LCD_DRIVER_FRAME_FREE(lcd_info.display_frame);

			lcd_info.display_frame = lcd_info.pingpong_frame;
			lcd_info.pingpong_frame = NULL;
			lcd_driver_set_display_base_addr((uint32_t)lcd_info.display_frame->frame);
			rtos_set_semaphore(&lcd_info.disp_sem);
		}
		else
		{
			lcd_info.display_frame = lcd_info.pingpong_frame;
			lcd_info.pingpong_frame = NULL;
			rtos_set_semaphore(&lcd_info.disp_sem);
		}
	}

	LCD_DISPLAY_ISR_OUT();
}

static void lcd_driver_display_mcu_isr(void)
{
	LCD_DISPLAY_ISR_ENTRY();

	media_debug->isr_lcd++;

	if (lcd_info.enable == false)
	{
		return;
	}

	if (lcd_info.pingpong_frame != NULL)
	{
		media_debug->fps_lcd++;
		LCD_DRIVER_FRAME_FREE(lcd_info.display_frame);

		lcd_info.display_frame = lcd_info.pingpong_frame;
		lcd_info.pingpong_frame = NULL;
		bk_lcd_8080_start_transfer(0);

		rtos_set_semaphore(&lcd_info.disp_sem);
	}
	else
	{
		bk_lcd_8080_start_transfer(0);
	}

	LCD_DISPLAY_ISR_OUT();
}

frame_buffer_t *lcd_driver_decoder_frame(frame_buffer_t *frame, media_decode_mode_t decode_mode)
{
	bk_err_t ret = BK_FAIL;
	frame_buffer_t *dec_frame = NULL;
	uint64_t before, after;

	if (lcd_info.enable == false)
	{
		return dec_frame;
	}

	rtos_lock_mutex(&lcd_info.dec_lock);

	if (lcd_info.decoder_en == false)
	{
		rtos_unlock_mutex(&lcd_info.dec_lock);
		return dec_frame;
	}

#if CONFIG_ARCH_RISCV
	before = riscv_get_mtimer();
#else
	before = 0;
#endif
	lcd_info.decoder_frame = lcd_info.fb_malloc(frame->width * frame->height * 2);

	if (lcd_info.decoder_frame == NULL)
	{
		LOGE("malloc decoder NULL\n");
		goto out;
	}
	LCD_DECODER_START();

	lcd_info.decoder_frame->sequence = frame->sequence;
    if (lcd_info.jpg_fmt_check == false)
    {
		lcd_info.jpg_fmt_check = true;
		yuv_enc_fmt_t yuv_fmt = bk_get_original_jpeg_encode_data_format(frame->frame, frame->length);
		if (yuv_fmt == YUV_422)
		{
			LOGI("%s, FMT:YUV422, use HARDWARE DECODE\r\n", __func__);
			lcd_info.decode_mode = JPEGDEC_HW_MODE;
		}
		else if (yuv_fmt == YUV_ERR)
		{
			LOGI("%s, FMT:ERR\r\n", __func__);
			lcd_info.decode_mode = NONE_DECODE;
			LCD_DRIVER_FRAME_FREE(lcd_info.decoder_frame);
			lcd_info.decoder_frame = NULL;
			goto out;
		}
		else
		{
			LOGI("%s, FMT:YUV420, use SOFTWARE DECODE\r\n", __func__);
			lcd_info.decode_mode = SOFTWARE_DECODING_MAJOR;
			bk_jpeg_dec_sw_init(NULL, 0);
		}
	}

	if (lcd_info.decode_mode == HARDWARE_DECODING)
	{
		lcd_info.decoder_frame->fmt = PIXEL_FMT_VUYY;
#if CONFIG_LCD_AUTO_FILL_FULL
		if (frame->height < lcd_info.lcd_height) //like jpeg 1280X720, LCD 480X854
			lcd_info.decoder_frame->frame += (frame->width * (lcd_info.lcd_height - frame->height));
#endif
		ret = lcd_hw_decode_start(frame, lcd_info.decoder_frame);
		if (ret != BK_OK)
		{
			LOGD("%s hw decoder error\n", __func__);
			LCD_DRIVER_FRAME_FREE(lcd_info.decoder_frame);
			lcd_info.decoder_frame = NULL;
			goto out;
		}
	}
	else
	{
		lcd_info.decoder_frame->fmt = PIXEL_FMT_YUYV;
#if CONFIG_LCD_SW_DECODE
		if (lcd_info.decode_mode == SOFTWARE_DECODING_MAJOR)
		{
#if CONFIG_LCD_AUTO_FILL_FULL
			if (frame->height < lcd_info.lcd_height) //like jpeg 1280X720, LCD 480X854
				lcd_info.decoder_frame->frame += (frame->width * (lcd_info.lcd_height - frame->height));
#endif
			ret = lcd_sw_jpegdec_start(frame, lcd_info.decoder_frame);
#if (CONFIG_TASK_WDT)
			extern void bk_task_wdt_feed(void);
			bk_task_wdt_feed();
#endif
			if (ret != BK_OK)
			{
				LOGE("%s sw decoder error\n", __func__);
				LCD_DRIVER_FRAME_FREE(lcd_info.decoder_frame);
				lcd_info.decoder_frame = NULL;
				goto out;
			}
		}
		else
		{
			//ret = lcd_sw_minor_jpegdec_start(frame, lcd_info.decoder_frame);

			if (ret != BK_OK)
			{
				LOGE("%s sw decoder error\n", __func__);
				LCD_DRIVER_FRAME_FREE(lcd_info.decoder_frame);
				lcd_info.decoder_frame = NULL;
				goto out;
			}
		}
#endif
	}
	LCD_DECODER_END();

out:

    if (lcd_info.decoder_frame == NULL)
    {
        LOGD("%s decoder failed\n", __func__);
        ret = BK_FAIL;
    }
    else
    {
#if CONFIG_LCD_AUTO_FILL_FULL
	if (frame->height < lcd_info.lcd_height) //like jpeg 1280X720, LCD 480X854
		lcd_info.decoder_frame->frame -= (frame->width * (lcd_info.lcd_height - frame->height));
#endif

        dec_frame = lcd_info.decoder_frame;
        lcd_info.decoder_frame = NULL;
    }

	rtos_unlock_mutex(&lcd_info.dec_lock);
#if CONFIG_ARCH_RISCV
	after = riscv_get_mtimer();
#else
	after = 0;
#endif
	LOGD("decoder time: %lu\n", (after - before) / 26000);

	return dec_frame;
}


frame_buffer_t *lcd_driver_scale_frame(frame_buffer_t *frame, media_ppi_t ppi)
{
	frame_buffer_t *scale_frame = NULL;
#if CONFIG_MEDIA_SCALE
	bk_err_t ret = BK_FAIL;
	uint64_t before, after;

	if (lcd_info.enable == false)
	{
		LOGE("%s lcd_info.enable == false\r\n", __func__);
		LCD_DRIVER_FRAME_FREE(frame);
		return scale_frame;
	}
	if (lcd_info.scale_en == false)
	{
		LOGE("%s lcd_info.scale_en == false\r\n", __func__);
		rtos_unlock_mutex(&lcd_info.scale_lock);
		return frame;
	}

	LCD_RESIZE_START();
#if CONFIG_ARCH_RISCV
		before = riscv_get_mtimer();
#else
		before = 0;
#endif

	uint32_t scale_length = (lcd_info.scale_ppi >> 16) * (lcd_info.scale_ppi & 0xFFFF) * 2;
	if (lcd_info.scale_frame == NULL)
	{
		//lcd_info.scale_frame = lcd_info.fb_malloc(scale_length);
		lcd_info.scale_frame = lcd_info.fb_malloc(frame->width * frame->height * 2);
	}
	if (lcd_info.scale_frame == NULL)
	{
		LOGE("%s, malloc scale_frame NULL\n", __func__);
		goto out;
	}
	LOGD("scale_ppi: width height %d %d\n", (lcd_info.scale_ppi >> 16), (lcd_info.scale_ppi & 0xFFFF));

	lcd_info.scale_frame->width = (lcd_info.scale_ppi >> 16);
	lcd_info.scale_frame->height = (lcd_info.scale_ppi & 0xFFFF);
	lcd_info.scale_frame->fmt = frame->fmt;
	lcd_info.scale_frame->sequence = frame->sequence;
	lcd_info.scale_frame->length = scale_length;

	ret = lcd_hw_scale(frame, lcd_info.scale_frame);
	if (ret != BK_OK)
	{
		LOGE("%s scale error: %d\n", __func__, ret);
		LCD_DRIVER_FRAME_FREE(lcd_info.scale_frame);
		goto out;
	}

#if CONFIG_ARCH_RISCV
	after = riscv_get_mtimer();
#else
	after = 0;
#endif
	LOGD("rotate time: %lu\n", (after - before) / 26000);

	LCD_RESIZE_END();

	scale_frame = lcd_info.scale_frame;
	lcd_info.scale_frame = NULL;

out:

	LCD_DRIVER_FRAME_FREE(frame);
	rtos_unlock_mutex(&lcd_info.scale_lock);
#endif
	return scale_frame;
}

frame_buffer_t *lcd_driver_rotate_frame(frame_buffer_t *frame, media_rotate_t rotate)
{
	frame_buffer_t *rot_frame = NULL;
	uint64_t before, after;
	bk_err_t ret = BK_FAIL;

	if (rotate == ROTATE_NONE)
	{
		LOGD("%s rotate 0 \n",__func__);
		return frame;
	}
	LCD_ROTATE_START();

	if (lcd_info.enable == false)
	{
		LOGE("%s lcd_info.enable == false\r\n", __func__);
		LCD_DRIVER_FRAME_FREE(frame);
		return rot_frame;
	}

	rtos_lock_mutex(&lcd_info.rot_lock);

	if (lcd_info.rotate_en == false)
	{
		rtos_unlock_mutex(&lcd_info.rot_lock);
		return frame;
	}
#if CONFIG_ARCH_RISCV
	before = riscv_get_mtimer();
#else
	before = 0;
#endif


	if (lcd_info.rotate_frame == NULL)
	{
		lcd_info.rotate_frame = lcd_info.fb_malloc(frame->width * frame->height * 2);
	}
	if (lcd_info.rotate_frame == NULL)
	{
		LOGE("%s, malloc rotate_frame NULL\n", __func__);
		goto out;
	}

	if (rotate == ROTATE_180)
	{
		lcd_info.rotate_frame->height = frame->height;
		lcd_info.rotate_frame->width = frame->width;
	}
	else
	{
		lcd_info.rotate_frame->height = frame->width;
		lcd_info.rotate_frame->width = frame->height;
	}

	lcd_info.rotate_frame->fmt = frame->fmt;
	lcd_info.rotate_frame->sequence = frame->sequence;
	lcd_info.rotate_frame->length = frame->width * frame->height * 2;

	if ((lcd_info.rotate_mode == HW_ROTATE))
	{
#if CONFIG_HW_ROTATE_PFC
		ret = lcd_hw_rotate_yuv2rgb565(frame, lcd_info.rotate_frame, rotate);
#endif
	}
	else
	{
		ret = lcd_sw_rotate(frame, lcd_info.rotate_frame, rotate);
	}
	if (ret != BK_OK)
	{
		LOGE("%s rotate error: %d\n", __func__, ret);
		LCD_DRIVER_FRAME_FREE(lcd_info.rotate_frame);
		goto out;
	}

#if CONFIG_ARCH_RISCV
	after = riscv_get_mtimer();
#else
	after = 0;
#endif
	LOGD("rotate time: %lu\n", (after - before) / 26000);

	LCD_ROTATE_END();

out:
	if(lcd_info.rotate_frame == NULL)
	{
		media_debug->err_rot++;
	}
	else
	{
		rot_frame = lcd_info.rotate_frame;
		lcd_info.rotate_frame = NULL;
	}
	LCD_DRIVER_FRAME_FREE(frame);
	rtos_unlock_mutex(&lcd_info.rot_lock);

	return rot_frame;
}


bk_err_t lcd_driver_display_frame_wait(uint32_t timeout_ms)
{
	bk_err_t ret = rtos_get_semaphore(&lcd_info.disp_sem, timeout_ms);

	if (ret != BK_OK)
	{
		LOGE("%s semaphore get failed: %d\n", __func__, ret);
	}

	return ret;
}

bk_err_t lcd_driver_display_frame_sync(frame_buffer_t *frame, bool wait)
{
	bk_err_t ret = BK_FAIL;
	uint64_t before, after;

	if (lcd_info.enable == false)
	{
		LCD_DRIVER_FRAME_FREE(frame);
		return ret;
	}

	LCD_DISPLAY_START();
#if CONFIG_ARCH_RISCV
	before = riscv_get_mtimer();
#else
	before = 0;
#endif
	rtos_lock_mutex(&lcd_info.disp_lock);

	if (lcd_info.display_en == false)
	{
		LCD_DRIVER_FRAME_FREE(frame);
		rtos_unlock_mutex(&lcd_info.disp_lock);
		return ret;
	}

	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();

	if (lcd_info.display_frame == NULL)
	{
#if CONFIG_LCD_AUTO_FILL_FULL
		if (frame->height < lcd_info.lcd_height)
			lcd_driver_ppi_set(frame->width, lcd_info.lcd_height);
		else
#endif
		lcd_driver_ppi_set(frame->width, frame->height);

		bk_lcd_set_yuv_mode(frame->fmt);
        if ((frame->fmt == PIXEL_FMT_RGB565_LE) || (frame->fmt == PIXEL_FMT_RGB565))
        {
            lcd_hal_rgb_set_in_out_format(frame->fmt, PIXEL_FMT_RGB565);
        }
		lcd_info.pingpong_frame = frame;

		lcd_driver_set_display_base_addr((uint32_t)frame->frame);
		lcd_driver_display_enable();
		LOGI("display start, frame W x H %d x %d, data_fmt %d\n", frame->width, frame->height, frame->fmt);
	}
	else
	{
		if (lcd_info.pingpong_frame != NULL)
		{
			LCD_DRIVER_FRAME_FREE(lcd_info.pingpong_frame);
		}

		lcd_info.pingpong_frame = frame;

		if (lcd_info.lcd_device->type == LCD_TYPE_MCU8080)
		{
			lcd_driver_ppi_set(frame->width, frame->height);
			bk_lcd_set_yuv_mode(frame->fmt);
			lcd_driver_set_display_base_addr((uint32_t)frame->frame);

			lcd_driver_display_continue();
		}
	}

	GLOBAL_INT_RESTORE();

	if(wait == BK_TRUE)
	{
		ret = lcd_driver_display_frame_wait(BEKEN_NEVER_TIMEOUT);
	}

	rtos_unlock_mutex(&lcd_info.disp_lock);
#if CONFIG_ARCH_RISCV
	after = riscv_get_mtimer();
#else
	after = 0;
#endif
	LOGD("display time: %lu\n", (after - before) / 26000);

	LCD_DISPLAY_END();

	return ret;
}

static void dma2d_transfer_complete(void)
{
    rtos_set_semaphore(&dma2d_complete_sem);
}

void dma2d_memcpy_init(void)
{
    bk_dma2d_driver_init();
    rtos_init_semaphore_ex(&dma2d_complete_sem, 1, 0);
    bk_dma2d_int_enable( DMA2D_TRANS_COMPLETE,1);
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_COMPLETE_ISR, dma2d_transfer_complete);
}


void dma2d_memcpy_psram_wait_last_transform_is_finish(void)
{
	if(dma2d_complete_sem && 1 == g_dma2d_use_flag)
	{
		rtos_get_semaphore(&dma2d_complete_sem, 1000);
		g_dma2d_use_flag = 0;
	}
}

void dma2d_memcpy_psram_wait_last_transform(void *Psrc, void *Pdst, uint32_t xsize, uint32_t ysize, uint32_t src_offline, uint32_t dest_offline)
{
	dma2d_memcpy_psram_wait_last_transform_is_finish();
	dma2d_memcpy_psram(Psrc, Pdst, xsize, ysize, src_offline, dest_offline);
	g_dma2d_use_flag = 1;
}

void dma2d_memcpy_psram_wait_last_transform_for_lvgl
                                (void *Psrc, uint32_t src_xsize, uint32_t src_ysize,
                                   void *Pdst, uint32_t dst_xpos, uint32_t dst_ypos)
{
    dma2d_memcpy_psram_wait_last_transform_is_finish();
    dma2d_memcpy_psram_for_lvgl(Psrc, src_xsize, src_ysize,
                            Pdst, lcd_info.lcd_width, lcd_info.lcd_height,
                            0, 0,
                            dst_xpos, dst_ypos
                            );
    g_dma2d_use_flag = 1;
}



int __attribute__((weak)) lv_vendor_display_frame_cnt(void)
{
    return 0;
}




void lcd_driver_display_frame_with_gui(void *buffer, int width, int height)
{
#if (CONFIG_LCD_QSPI && CONFIG_LVGL)
    bk_lcd_qspi_display((uint32_t)buffer);
#else

    if(g_gui_need_to_wait)
	{
		lcd_driver_display_frame_wait(2000);

		g_gui_need_to_wait = BK_FALSE;
	}

    if(lcd_info.lvgl_frame){
       lcd_info.lvgl_frame->fmt = PIXEL_FMT_RGB565;
        lcd_info.lvgl_frame->frame = buffer;
        lcd_info.lvgl_frame->width = width;
        lcd_info.lvgl_frame->height = height;

        if (lcd_info.lcd_device->type == LCD_TYPE_RGB565)
        {
            if(2 == lv_vendor_display_frame_cnt())
            {
                lcd_driver_display_frame_sync(lcd_info.lvgl_frame, BK_TRUE);
                g_gui_need_to_wait = BK_FALSE;
            }
            else
            {
                lcd_driver_display_frame_sync(lcd_info.lvgl_frame, BK_FALSE);
                g_gui_need_to_wait = BK_TRUE;
            }
        }
        else
        {
            lcd_driver_display_frame_sync(lcd_info.lvgl_frame, BK_TRUE);
            g_gui_need_to_wait = BK_FALSE;
        }
    }
    else{
        LOGI("[%s][%d] fb malloc fail\r\n", __FUNCTION__, __LINE__);
    }
#endif
}

bk_err_t lcd_driver_display_frame(frame_buffer_t *frame)
{
	return lcd_driver_display_frame_sync(frame, BK_TRUE);
}



bk_err_t lcd_blend_open_handler(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;
	if (msg->param == 0)
	{
#if (CONFIG_LCD_DMA2D_BLEND || CONFIG_LCD_FONT_BLEND)
		if (lcd_info.dma2d_blend == true)
		{
			lcd_dma2d_blend_deinit();
			lcd_info.font_draw = false;
			lcd_info.dma2d_blend = false;
			LOGI("%s lcd dma2d blend deinit \r\n", __func__);
		}
#endif
	}
	else
	{
#if CONFIG_LCD_DMA2D_BLEND || ONFIG_LCD_FONT_BLEND
		if ((lcd_info.dma2d_blend != true) && (lcd_info.font_draw != true))
		{
			lcd_info.dma2d_blend = true;
			lcd_info.font_draw = true;

			ret = lcd_blend_malloc_buffer();
			if (ret != BK_OK) {
				LOGE("lcd blend malloc error, lcd blend open fail\r\n");
				lcd_info.dma2d_blend = false;
				lcd_info.font_draw = false;
			}

			if(lcd_info.dma2d_blend)
			{
				ret= lcd_dma2d_blend_init();
					if (ret != BK_OK) {
					LOGE("lcd blend malloc error, lcd blend open fail\r\n");
					lcd_info.dma2d_blend = false;
					lcd_info.font_draw = false;
				}
			}
		}
		else
		{
			LOGI("lcd blend already open, dma2d_blend or font_draw is true\r\n");
		}
#endif
	}

	return ret;
}



bk_err_t lcd_display_echo_event_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;
	LOGI("%s  lcd_info.decode_mode = %d\n", __func__,  lcd_info.decode_mode);

	frame_buffer_t *jpeg_frame = (frame_buffer_t *)msg->param;
	jpeg_frame->fmt = PIXEL_FMT_JPEG;
	if(msg->result != BK_OK)
	{
		frame_buffer_fb_direct_free(jpeg_frame);
		ret = BK_FAIL;
		goto error;
	}

	if (jpeg_frame == NULL)
	{
		LOGW("%s:jpeg_frame == NULL\n", __func__);
		ret = BK_FAIL;
		goto error;
	}

	frame_buffer_t *dec_frame = lcd_driver_decoder_frame(jpeg_frame, lcd_info.decode_mode);
	lcd_info.picture_echo = false;

	if (dec_frame == NULL)
	{
		frame_buffer_fb_direct_free(jpeg_frame);
		LOGE("display jpeg decoder error\n");
		ret = BK_FAIL;
		goto error;
	}

	frame_buffer_fb_direct_free(jpeg_frame);
	frame_buffer_fb_push(dec_frame);

	LOGI("%s complete\n", __func__);
	msg->result = ret;
	ret = rtos_set_semaphore(&msg->sem);
	if (ret != BK_OK)
	{
		LOGE("%s semaphore set failed: %d\n", __func__, ret);
	}
	return ret;

error:
	lcd_info.picture_echo = false;
	lcd_decoder_task_start(lcd_info.rotate);
	set_lcd_state(LCD_STATE_ENABLED);

	msg->result = ret;
	LOGI("%s: open jpeg_decoder_task \n", __func__);

	ret = rtos_set_semaphore(&msg->sem);
	if (ret != BK_OK)
	{
		LOGE("%s semaphore set failed: %d\n", __func__, ret);
	}
	return BK_FAIL;
}

bk_err_t lcd_display_file_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;

	jpeg_decoder_task_stop();
	set_lcd_state(LCD_STATE_DISPLAY);

	if (lcd_info.picture_echo == true)
	{
		LOGE("%s display echo is on going\r\n", __func__);
		msg->result = BK_FAIL;
		return BK_FAIL;
	}

	frame_buffer_fb_init(FB_INDEX_JPEG);
	frame_buffer_fb_register(MODULE_DECODER, FB_INDEX_JPEG);
	frame_buffer_t *frame = frame_buffer_fb_malloc(FB_INDEX_JPEG, 200 * 1024);

	if (frame == NULL)
	{
		LOGE("%s, read jpeg frame NULL\n", __func__);

		msg->result = BK_FAIL;
		return BK_FAIL;
	}

	lcd_info.picture_echo = true;
    bk_printf("%s %d\r\n", __func__, __LINE__);
	ret = msg_send_req_to_media_major_mailbox_sync(EVENT_LCD_PICTURE_ECHO_NOTIFY, APP_MODULE, (uint32_t)frame, NULL);

	msg->result = ret;
	return ret;

}


bk_err_t lcd_open_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;

	if (LCD_STATE_DISPLAY == get_lcd_state())
	{
		lcd_decoder_task_start(lcd_info.rotate); 
		set_lcd_state(LCD_STATE_ENABLED);
		LOGI("%s: open jpeg_decoder_task \n", __func__);
	}

	if (LCD_STATE_ENABLED == get_lcd_state())
	{
		LOGW("%s already open\n", __func__);
		return ret;
	}

    bk_printf("%s %x\n", __func__, (uint32_t *)msg->param);

	//os_memcpy(&lcd_info.lcd_device, lcd_config.lcd_device, sizeof(lcd_device_t));
	lcd_open_t *lcd_open = (lcd_open_t *)msg->param;
	os_memcpy(&lcd_config.lcd_open, lcd_open, sizeof(lcd_open_t));

	if (lcd_config.lcd_open.device_name != NULL)
		lcd_info.lcd_device = get_lcd_device_by_name(lcd_config.lcd_open.device_name);

	if (lcd_info.lcd_device == NULL)
	{
		lcd_info.lcd_device = get_lcd_device_by_ppi(lcd_config.lcd_open.device_ppi);
	}

	if (lcd_info.lcd_device == NULL)
	{
		LOGE("%s lcd device not found\n", __func__);
		goto out;
	}

	LOGI("%s, lcd ppi: %dX%d %s\n", __func__, lcd_info.lcd_device->ppi >> 16, lcd_info.lcd_device->ppi & 0xFFFF, lcd_info.lcd_device->name);

	lcd_info.lcd_width = ppi_to_pixel_x(lcd_info.lcd_device->ppi);
	lcd_info.lcd_height = ppi_to_pixel_y(lcd_info.lcd_device->ppi);
	lcd_info.fb_free  = frame_buffer_fb_direct_free;
	lcd_info.fb_malloc = frame_buffer_fb_display_malloc_wait;

	frame_buffer_fb_init(FB_INDEX_DISPLAY);

	ret = lcd_driver_init(lcd_info.lcd_device);
	if(lcd_info.lcd_device->type == LCD_TYPE_MCU8080)
		bk_lcd_isr_register(I8080_OUTPUT_EOF, lcd_driver_display_mcu_isr);
	else
		bk_lcd_isr_register(RGB_OUTPUT_EOF, lcd_driver_display_rgb_isr);

#if (CONFIG_LCD_FONT_BLEND || CONFIG_LCD_DMA2D_BLEND)
	lcd_info.dma2d_blend = true;
	lcd_info.font_draw = true;
	ret = lcd_blend_malloc_buffer();
	if (ret != BK_OK) {
		LOGE("lcd blend malloc error, lcd blend open fail\r\n");
		lcd_info.dma2d_blend = false;
		lcd_info.font_draw = false;
	}

	if (lcd_info.dma2d_blend)
	{
		ret = lcd_dma2d_blend_init();
			if (ret != BK_OK)
			{
				LOGE("lcd blend malloc error, lcd blend open fail\r\n");
				lcd_info.dma2d_blend = false;
		}
	}
#endif

#if CONFIG_LCD_ROTATE
	lcd_info.hw_yuv2rgb = lcd_config.hw_yuv2rgb;
	if((lcd_info.rotate_en == 1) || (lcd_config.rotate_open == 1) || (lcd_info.hw_yuv2rgb == 1))
	{
		if(!lcd_info.rotate)
			lcd_info.rotate = lcd_config.rotate_angle;
		lcd_info.rotate_en = true;
		lcd_info.rotate_mode = lcd_config.rotate_mode;//in lcd_config set default hw rotate
		ret = lcd_rotate_init(lcd_info.rotate_mode);  //hw no rotate
		if (ret != BK_OK)
			LOGE("%s, lcd_rotate_init fail\r\n", __func__);

		LOGI("rotate mode(1-sw, 2-hw) = %d, rotate angle(0/1/2/3 -->0/90/180/270) = %d\r\n", lcd_info.rotate_mode, lcd_info.rotate);
	}
	else
	{
		LOGI("%s, lcd rotate not open\r\n", __func__);
		lcd_info.rotate_en = false;
	}
#endif
	if (lcd_info.decode_mode == NONE_DECODE)
		lcd_info.decode_mode = lcd_config.decode_mode;
	if(lcd_info.decode_mode == HARDWARE_DECODING)
	{
		lcd_info.decoder_en = true;
#if CONFIG_LCD_HW_DECODE
		lcd_hw_decode_init();
		LOGI("%s, hw decode init ok\r\n", __func__);
#endif
	}
	else
	{
		lcd_info.decoder_en = true;
#if CONFIG_LCD_SW_DECODE
		lcd_sw_decode_init(lcd_info.decode_mode);
		LOGI("%s, lcd SW decode init ok\r\n", __func__);
#endif
	}

#if CONFIG_MEDIA_SCALE
	if (lcd_info.scale_en)
	{
		ret = lcd_scale_init();
		if (ret != BK_OK)
			LOGE("%s, bk_hw_scale_driver_init fail\r\n", __func__);
        lcd_info.scale_ppi = lcd_info.lcd_device->ppi;

        LOGI("%s, ===>>>scale ppi: %dX%d %s\n", __func__, lcd_info.scale_ppi >> 16, lcd_info.scale_ppi & 0xFFFF, lcd_info.lcd_device->name);
	}

#endif

	media_debug->fps_lcd= 0;
	media_debug->isr_lcd = 0;

	rtos_init_mutex(&lcd_info.dec_lock);
	rtos_init_mutex(&lcd_info.rot_lock);
	rtos_init_mutex(&lcd_info.scale_lock);
	rtos_init_mutex(&lcd_info.disp_lock);

	lcd_info.display_en = true;

	lcd_info.lvgl_frame = os_malloc(sizeof(frame_buffer_t));
	os_memset(lcd_info.lvgl_frame, 0, sizeof(frame_buffer_t));

	ret = rtos_init_semaphore_ex(&lcd_info.disp_sem, 1, 0);

	if (ret != BK_OK)
	{
		LOGE("%s disp_sem init failed: %d\n", __func__, ret);
		return ret;
	}

	lcd_decoder_task_start(lcd_info.rotate);

#if CONFIG_LCD_QSPI
    bk_lcd_qspi_disp_task_start(lcd_info.lcd_device);
#else
#ifdef DISPLAY_PIPELINE_TASK
	lcd_display_task_start(lcd_info.rotate);
#endif
#endif

	set_lcd_state(LCD_STATE_ENABLED);
	lcd_info.enable = true;
	lcd_driver_backlight_open();

	LOGI("%s complete\n", __func__);

out:
	return ret;

}

bk_err_t lcd_display_fram_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;
	//static uint8_t display_type_init = 0;
	lcd_display_t *lcd_mcu_display = (lcd_display_t *)msg->param;
#if 0
	if (lcd_mcu_display->display_type == 1 && display_type_init != lcd_mcu_display->display_type)
	{
		LOGI("set new ppi 320 * 80\n\r");
		lcd_driver_ppi_set(320, 240);
		display_type_init = lcd_mcu_display->display_type;
	}
	else if (lcd_mcu_display->display_type == 2 && display_type_init != lcd_mcu_display->display_type)
	{
		LOGI("set new ppi 160 * 240\n\r");
		lcd_driver_ppi_set(160, 240);
		display_type_init = lcd_mcu_display->display_type;
	}
	else if (lcd_mcu_display->display_type == 0 && display_type_init != lcd_mcu_display->display_type)
	{
		LOGI("set new ppi 320 * 480\n\r");
		lcd_driver_ppi_set(320, 480);
		display_type_init = lcd_mcu_display->display_type;
	}
#endif
	if(lcd_info.lcd_device->type == LCD_TYPE_MCU8080)
	{
		// LOGI("%s image_addr: 0x%x, x_start: %d, y_start: %d, x_end: %d, y_end: %d\n", __func__, lcd_mcu_display->image_addr, lcd_mcu_display->x_start, lcd_mcu_display->y_start, lcd_mcu_display->x_end, lcd_mcu_display->y_end);
		//lcd_driver_set_display_mem_area(lcd_mcu_display->x_start, lcd_mcu_display->x_end, lcd_mcu_display->y_start, lcd_mcu_display->y_end);
		lcd_info.lcd_device->mcu->set_display_area(lcd_mcu_display->x_start, lcd_mcu_display->x_end, lcd_mcu_display->y_start, lcd_mcu_display->y_end);
		lcd_driver_set_display_base_addr((uint32_t)lcd_mcu_display->image_addr);
		bk_lcd_8080_start_transfer(1);
		lcd_hal_8080_ram_write(LCD_RAM_WRITE);
	}
	if(lcd_info.lcd_device->type == LCD_TYPE_RGB565)
	{
		lcd_hal_rgb_display_en(0);
		lcd_hal_set_display_read_base_addr((uint32_t)lcd_mcu_display->image_addr);
		lcd_driver_display_enable();
	}


	return ret;
}


bk_err_t lcd_close_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;

	LOGI("%s\n", __func__);
	if (step_sem != NULL)
	{
		if (rtos_set_semaphore(&step_sem))
		{
			LOGE("%s step_sem set failed\n", __func__);
		}

		if (rtos_deinit_semaphore(&step_sem))
		{
			LOGE("%s step_sem deinit failed\n", __func__);
		}
		step_sem = NULL;
	}

	if (LCD_STATE_DISABLED == get_lcd_state())
	{
		LOGW("%s already close\n", __func__);
		goto out;
	}

	lcd_driver_backlight_close();

	jpeg_decoder_task_stop();

#if CONFIG_LCD_QSPI
    bk_lcd_qspi_disp_task_stop();
#else
#ifdef DISPLAY_PIPELINE_TASK
	lcd_display_task_stop();
#endif
#endif

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	lcd_info.enable = false;
	GLOBAL_INT_RESTORE();

	rtos_lock_mutex(&lcd_info.dec_lock);
	lcd_info.decoder_en = false;
	rtos_unlock_mutex(&lcd_info.dec_lock);

	rtos_lock_mutex(&lcd_info.rot_lock);
	lcd_info.rotate_en = false;
	rtos_unlock_mutex(&lcd_info.rot_lock);

	rtos_lock_mutex(&lcd_info.scale_lock);
	lcd_info.scale_en = false;
	rtos_unlock_mutex(&lcd_info.scale_lock);

	rtos_lock_mutex(&lcd_info.disp_lock);
	lcd_info.display_en = false;
	rtos_unlock_mutex(&lcd_info.disp_lock);

	lcd_driver_deinit();

	if(lcd_info.decode_mode == HARDWARE_DECODING)
	{
#if CONFIG_LCD_HW_DECODE
		lcd_hw_decode_deinit();
		LOGW("%s lcd_hw_decode_deinit\n", __func__);
#endif
	}
	else
	{
#if CONFIG_LCD_SW_DECODE
		if(lcd_info.decode_mode != NONE_DECODE)
			lcd_sw_decode_deinit(lcd_info.decode_mode);
#endif
	}
#if CONFIG_LCD_ROTATE
	lcd_rotate_deinit();
#endif

#if (CONFIG_LCD_DMA2D_BLEND || CONFIG_LCD_FONT_BLEND)
	if (lcd_info.dma2d_blend || lcd_info.font_draw)
		lcd_blend_free_buffer();
	if (lcd_info.dma2d_blend)
		lcd_dma2d_blend_deinit();
	lcd_info.dma2d_blend = false;
	lcd_info.font_draw = false;
#endif

	if (lcd_info.rotate_frame)
	{
		lcd_info.fb_free(lcd_info.rotate_frame);
		lcd_info.rotate_frame = NULL;
	}

	if (lcd_info.decoder_frame)
	{
		lcd_info.fb_free(lcd_info.decoder_frame);
		lcd_info.decoder_frame = NULL;
	}

	if (lcd_info.pingpong_frame)
	{
		lcd_info.fb_free(lcd_info.pingpong_frame);
		lcd_info.pingpong_frame = NULL;
	}

	if (lcd_info.display_frame)
	{
		if (lcd_info.display_frame != lcd_info.lvgl_frame)
		{
			lcd_info.fb_free(lcd_info.display_frame);
		}
		lcd_info.display_frame = NULL;
	}

	if (lcd_info.lvgl_frame)
	{
		os_free(lcd_info.lvgl_frame);
		lcd_info.lvgl_frame = NULL;
	}

//#if (CONFIG_LCD_QSPI && CONFIG_LVGL)
//#else
//	g_gui_need_to_wait = BK_FALSE;
//#endif


	ret = rtos_deinit_semaphore(&lcd_info.disp_sem);

	if (ret != BK_OK)
	{
		LOGE("%s disp_sem deinit failed: %d\n", __func__, ret);
		return ret;
	}

	rtos_deinit_mutex(&lcd_info.dec_lock);
	rtos_deinit_mutex(&lcd_info.rot_lock);
	rtos_deinit_mutex(&lcd_info.scale_lock);
	rtos_deinit_mutex(&lcd_info.disp_lock);

	lcd_info.rotate = ROTATE_NONE;
	lcd_info.scale_en = false;
	lcd_info.decoder_en = false;
	lcd_info.rotate_en = false;
	lcd_info.display_en = false;
	lcd_info.enable = false;

	set_lcd_state(LCD_STATE_DISABLED);

	LOGI("%s complete\n", __func__);

out:
	return ret;
}



void lcd_set_backligth_handle(param_pak_t *param)
{
	int ret = BK_OK;

	LOGI("%s, levle: %d\n", __func__, param->param);
	lcd_driver_backlight_set(param->param);

	MEDIA_EVT_RETURN(param, ret);
}

void lcd_act_dump_decoder_frame(void)
{
#if (CONFIG_IMAGE_STORAGE && CONFIG_SYS_CPU0)
	//storage_frame_buffer_dump(lcd_info.decoder_frame, "decoder_vuyy.yuv");
#endif
}

void lcd_act_dump_jpeg_frame(void)
{
	if (dbg_jpeg_frame == NULL)
	{
		LOGE("dbg_jpeg_frame was NULL\n");
		return;
	}

	LOGI("Dump jpeg frame seq: %u, ptr: %p, length: %u, fmt: %u\n",
	     dbg_jpeg_frame->sequence, dbg_jpeg_frame->frame, dbg_jpeg_frame->length, dbg_jpeg_frame->fmt);

#if (CONFIG_IMAGE_STORAGE && CONFIG_SYS_CPU0)
	storage_frame_buffer_dump(dbg_jpeg_frame, frame_suffix(dbg_jpeg_frame->fmt));
#endif
}

void lcd_act_dump_display_frame(void)
{
	if (dbg_display_frame == NULL)
	{
		LOGE("dbg_display_frame was NULL\n");
		return;
	}

	LOGI("Dump display frame seq: %u, ptr: %p, length: %u, fmt: %u\n",
	     dbg_display_frame->sequence, dbg_display_frame->frame, dbg_display_frame->length, dbg_display_frame->fmt);

#if (CONFIG_IMAGE_STORAGE && CONFIG_SYS_CPU0)
	storage_frame_buffer_dump(dbg_display_frame, frame_suffix(dbg_display_frame->fmt));
#endif
}
bk_err_t lcd_set_decode_mode_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;
	lcd_info.decoder_en = true;
	lcd_info.decode_mode = (media_decode_mode_t)msg->param;
	LOGI("%s =%d (1-major_dec,2-minor_dec,3-hw dec)\n", __func__, lcd_info.decode_mode);
	return ret;
}

bk_err_t lcd_rotate_handle(media_mailbox_msg_t *msg)
{
	int ret = BK_OK;
	lcd_info.rotate_en = true;
	lcd_info.rotate = msg->param;
	return ret;
}

//Modified by TUYA Start
#include "usbh_core.h"
static uint32_t usb_device_status = 0;
static void usbh_device_info(uint32_t vid, uint32_t pid, uint32_t bcd)
{
    bk_printf("vid: %04x, pid: %04x, bcd: %04x\r\n", vid, pid, bcd);
    if ((vid != 0) && (pid != 0) && (bcd != 0)) {
        usb_device_status = 1;
    } else {
        usb_device_status = 0;
    }
}
//Modified by TUYA End

void lcd_event_handle(media_mailbox_msg_t *msg)
{
	bk_err_t ret = BK_OK;

	switch (msg->event)
	{

		case EVENT_LCD_OPEN_IND:
			LOGI("%s EVENT_LCD_OPEN_IND \n", __func__);
			ret = lcd_open_handle(msg);
			break;
		case EVENT_LCD_CLOSE_IND:
			LOGI(" %s EVENT_LCD_CLOSE_IND \n", __func__);
			ret = lcd_close_handle(msg);
			break;

		case EVENT_LCD_ROTATE_ENABLE_IND:
		{
			LOGI("%s EVENT_LCD_ROTATE_ENABLE_IND\n", __func__);
			ret = lcd_rotate_handle(msg);
		}
			break;
		case EVENT_LCD_DECODE_MODE_IND:
		{
			LOGI("%s EVENT_LCD_DECODE_MODE_IND\n", __func__);
			ret = lcd_set_decode_mode_handle(msg);
		}
			break;

		case EVENT_LCD_BLEND_IND:
		{
#if CONFIG_LCD_DMA2D_BLEND || CONFIG_LCD_FONT_BLEND
			ret = lcd_blend_font_handle(msg);
#endif
		}
			break;
		case EVENT_LCD_BLEND_OPEN_IND:
		{
#if CONFIG_LCD_DMA2D_BLEND || CONFIG_LCD_FONT_BLEND
			ret = lcd_blend_open_handler(msg);
#endif
		}
			break;

		case EVENT_LCD_DISPLAY_FILE_IND:
		{
			LOGI("%s EVENT_LCD_DISPLAY_FILE_IND\n", __func__);
			ret = lcd_display_file_handle(msg);
		}
			break;
		case EVENT_LCD_GET_DEVICES_NUM_IND:
		{
			uint32_t device_num = get_lcd_devices_num();
			msg->param = device_num;
			break;
		}
		case EVENT_LCD_GET_DEVICES_LIST_IND:
		{
			const lcd_device_t **device_addr = get_lcd_devices_list();
			LOGI("%s, lcd device addr = %p\n", __func__, device_addr);
			msg->param =(uint32_t)(device_addr);
			break;
		}
		case EVENT_LCD_GET_DEVICES_IND:
		{
			const lcd_device_t *device = get_lcd_device_by_id(msg->param);
			if (device == NULL)
			{
				LOGE("%s, lcd device not exist id:%d\n", __func__, msg->param);
				ret = BK_ERR_NOT_SUPPORT;
			}
			msg->param =(uint32_t)(device);
			break;
		}
		case EVENT_LCD_SCALE_IND:
			LOGI(" %s, EVENT_LCD_SCALE_IND \n", __func__);
			lcd_info.scale_en = true;
			break;
		case EVENT_LCD_DISPLAY_IND:
			ret = lcd_display_fram_handle(msg);
			break;
		case EVENT_LCD_GET_STATUS_IND:
		{
			bool lcd_status = false;
#if CONFIG_MEDIA_PIPELINE
			lcd_status = check_lcd_task_is_open();
#else
			media_lcd_state_t lcd_state = get_lcd_state();
			lcd_status = (lcd_state == LCD_STATE_DISABLED ? 0 : 1);
#endif
			msg->param =(uint32_t)(lcd_status);
			break;
		}
		case EVENT_GET_UVC_STATUS_IND:
		{
			bool camera_status = check_uvc_status();
			msg->param =(uint32_t)(camera_status);
			break;
		}
        // Modified by TUYA Start
		case EVENT_GET_USB_STATUS_IND:
		{
            extern uint8_t is_usb_opened;
            uint8_t usb_need_open_close = 0;

            usbh_enumerate_register_cb(usbh_device_info);

            if (!is_usb_opened)
                usb_need_open_close = 1;

            if (usb_need_open_close)
                bk_usb_open(0);

            extern bk_err_t rtos_delay_milliseconds(uint32_t num_ms);
            rtos_delay_milliseconds(1000);
	        //bk_usb_device_set_using_status(1, USB_UVC_DEVICE);
			msg->param = (uint32_t)(usb_device_status);

            if (usb_need_open_close)
                bk_usb_close();
			break;
		}
        // Modified by TUYA End
		default:
			break;
	}

	msg_send_rsp_to_media_major_mailbox(msg, ret, APP_MODULE);
}

media_lcd_state_t get_lcd_state(void)
{
	return lcd_info.state;
}

void set_lcd_state(media_lcd_state_t state)
{
	lcd_info.state = state;
}

void lcd_init(void)
{
	os_memset(&lcd_info, 0, sizeof(lcd_info_t));
	lcd_info.state = LCD_STATE_DISABLED;
	lcd_info.debug = false;
	lcd_info.rotate = ROTATE_NONE;
	lcd_info.scale_en = false;
	//lcd_info.scale_ppi= PPI_480X854;
	lcd_info.scale_ppi= PPI_854X480;
}


media_decode_mode_t lcd_get_decode_mode_handle(media_mailbox_msg_t *msg)
{
	return lcd_info.decode_mode;
}
