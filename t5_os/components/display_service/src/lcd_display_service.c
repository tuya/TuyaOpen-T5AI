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
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <driver/lcd.h>
#include <driver/flash.h>

#include "display_service.h"

#include "frame_buffer.h"
#include "lcd_decode.h"
#include "yuv_encode.h"
#if CONFIG_LCD_QSPI
#include <lcd_qspi_display_service.h>
#endif
#include "mux_pipeline.h"
#include <media_mailbox_list_util.h>
#include "draw_blend.h"
#include "lcd_draw_blend.h"

#if CONFIG_LCD_SPI_DISPLAY
#include <lcd_spi_display_service.h>
#endif


#define TAG "lcd_pip"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

// Modified by TUYA Start
#define DISPLAY_LOGO_IMGE 1
// Modified by TUYA End

#ifdef DISP_DIAG_DEBUG
#define DISPLAY_START()			do { GPIO_UP(GPIO_DVP_D6); } while (0)
#define DISPLAY_END()			do { GPIO_DOWN(GPIO_DVP_D6); } while (0)
#define DISPLAY_ISR_START()			do { GPIO_UP(GPIO_DVP_D7); } while (0)
#define DISPLAY_ISR_END()			do { GPIO_DOWN(GPIO_DVP_D7); } while (0)
#else
#define DISPLAY_START()
#define DISPLAY_END()
#define DISPLAY_ISR_START()
#define DISPLAY_ISR_END()

#endif

typedef struct
{
	uint32_t event;
	uint32_t param;
} display_msg_t;


typedef struct {
	uint8_t disp_task_running : 1;
	uint8_t lcd_type;
	uint16_t lcd_width;
	uint16_t lcd_height;
	frame_buffer_t *pingpong_frame;
	frame_buffer_t *display_frame;
	beken_semaphore_t disp_sem;
	beken_semaphore_t disp_task_sem;
	beken_thread_t disp_task;
	beken_queue_t queue;
} lcd_disp_config_t;

typedef struct {
	beken_mutex_t lock;
} display_service_info_t;


extern media_debug_t *media_debug;
extern uint32_t  platform_is_in_interrupt_context(void);


static lcd_disp_config_t *lcd_disp_config = NULL;
static display_service_info_t *service_info = NULL;

typedef enum {
	DISPLAY_FRAME_REQUEST,
	DISPLAY_FRAME_FREE,
	DISPLAY_FRAME_EXTI,
// Modified by TUYA Start
#if (DISPLAY_LOGO_IMGE == 1)
	DISPLAY_LOGE_FRAME_EXTI,
#endif
// Modified by TUYA End
} lcd_display_msg_type_t;

bk_err_t lcd_display_task_send_msg(uint8_t type, uint32_t param);

static void lcd_driver_display_mcu_isr(void)
{
	media_debug->isr_lcd++;
	GLOBAL_INT_DECLARATION();

	if (lcd_disp_config->pingpong_frame != NULL)
	{
		media_debug->fps_lcd++;
		frame_buffer_display_free(lcd_disp_config->display_frame);

		GLOBAL_INT_DISABLE();
		lcd_disp_config->display_frame = lcd_disp_config->pingpong_frame;
		lcd_disp_config->pingpong_frame = NULL;
		GLOBAL_INT_RESTORE();
		bk_lcd_8080_start_transfer(0);

		rtos_set_semaphore(&lcd_disp_config->disp_sem);
	}
}

#if CONFIG_LV_ATTRIBUTE_FAST_MEM
static void lcd_driver_display_rgb_isr(void)
#else
__attribute__((section(".itcm_sec_code"))) static void lcd_driver_display_rgb_isr(void)
#endif
{
    DISPLAY_ISR_START();
	flash_op_status_t flash_status = FLASH_OP_IDLE;
	flash_status = bk_flash_get_operate_status();
	media_debug->isr_lcd++;
if (flash_status == FLASH_OP_IDLE)
{
	GLOBAL_INT_DECLARATION();
	if (lcd_disp_config->pingpong_frame != NULL)
	{
		if (lcd_disp_config->display_frame != NULL)
		{
			frame_buffer_t *temp_buffer = NULL;
			bk_err_t ret = BK_OK;
			media_debug->fps_lcd++;

			GLOBAL_INT_DISABLE();

			if (lcd_disp_config->pingpong_frame != lcd_disp_config->display_frame) {
				if (lcd_disp_config->display_frame->width != lcd_disp_config->pingpong_frame->width
					|| lcd_disp_config->display_frame->height != lcd_disp_config->pingpong_frame->height)
				{
					lcd_driver_ppi_set(lcd_disp_config->pingpong_frame->width, lcd_disp_config->pingpong_frame->height);
				}
				if (lcd_disp_config->display_frame->fmt != lcd_disp_config->pingpong_frame->fmt)
				{
					bk_lcd_set_yuv_mode(lcd_disp_config->pingpong_frame->fmt);
				}
                if (lcd_disp_config->display_frame->cb != NULL
                    && lcd_disp_config->display_frame->cb->free != NULL) {
                    lcd_disp_config->display_frame->cb->free(lcd_disp_config->display_frame);
                } else {
                    temp_buffer = lcd_disp_config->display_frame;
                    lcd_disp_config->display_frame = NULL;
                }
			}
			lcd_disp_config->display_frame = lcd_disp_config->pingpong_frame;
			lcd_disp_config->pingpong_frame = NULL;

			lcd_driver_set_display_base_addr((uint32_t)lcd_disp_config->display_frame->frame);

			if (temp_buffer != NULL)
			{
				ret = lcd_display_task_send_msg(DISPLAY_FRAME_FREE, (uint32_t)temp_buffer);
				if (ret != BK_OK)
				{
					frame_buffer_display_free(temp_buffer);
				}
			}
			GLOBAL_INT_RESTORE();
			rtos_set_semaphore(&lcd_disp_config->disp_sem);
		}
		else
		{
			GLOBAL_INT_DISABLE();
			lcd_disp_config->display_frame = lcd_disp_config->pingpong_frame;
			lcd_disp_config->pingpong_frame = NULL;
			GLOBAL_INT_RESTORE();
			rtos_set_semaphore(&lcd_disp_config->disp_sem);
		}
	}
}
    DISPLAY_ISR_END();
}

static bk_err_t lcd_display_frame(frame_buffer_t *frame)
{
	bk_err_t ret = BK_FAIL;
	DISPLAY_START();

	if (lcd_disp_config->display_frame == NULL)
	{
		lcd_driver_ppi_set(frame->width, frame->height);

		bk_lcd_set_yuv_mode(frame->fmt);
		lcd_disp_config->pingpong_frame = frame;

		lcd_driver_set_display_base_addr((uint32_t)frame->frame);
		lcd_driver_display_enable();
		LOGI("display start, frame width, height %d, %d\n", frame->width, frame->height);
	}
	else
	{
		if (lcd_disp_config->pingpong_frame != NULL)
		{
			frame_buffer_display_free(lcd_disp_config->pingpong_frame);
			lcd_disp_config->pingpong_frame = NULL;
		}

		lcd_disp_config->pingpong_frame = frame;

		if (lcd_disp_config->lcd_type == LCD_TYPE_MCU8080)
		{
			lcd_driver_ppi_set(frame->width, frame->height);
			bk_lcd_set_yuv_mode(frame->fmt);
			lcd_driver_set_display_base_addr((uint32_t)frame->frame);

			lcd_driver_display_continue();
		}
	}

	ret = rtos_get_semaphore(&lcd_disp_config->disp_sem, BEKEN_NEVER_TIMEOUT);

	if (ret != BK_OK)
	{
		LOGE("%s semaphore get failed: %d\n", __func__, ret);
	}
	DISPLAY_END();

	return ret;
}



bk_err_t lcd_display_task_send_msg(uint8_t type, uint32_t param)
{
	int ret = BK_FAIL;
	display_msg_t msg;
	uint32_t isr_context = platform_is_in_interrupt_context();

	if (lcd_disp_config)
	{
		msg.event = type;
		msg.param = param;

		if (!isr_context)
		{
			rtos_lock_mutex(&service_info->lock);
		}

		if (lcd_disp_config->disp_task_running)
		{
			ret = rtos_push_to_queue(&lcd_disp_config->queue, &msg, BEKEN_WAIT_FOREVER);

			if (ret != BK_OK)
			{
				LOGE("%s push failed\n", __func__);
			}
		}

		if (!isr_context)
		{
			rtos_unlock_mutex(&service_info->lock);
		}

	}

	return ret;
}

bk_err_t lcd_display_frame_request(frame_buffer_t *frame)
{
	return lcd_display_task_send_msg(DISPLAY_FRAME_REQUEST, (uint32_t)frame);
}

// Modified by TUYA Start
#if (DISPLAY_LOGO_IMGE == 1)
#include "frame_buffer.h"
#include "modules/image_scale.h"
#include "flash_driver.h"
#include <driver/flash.h>
#include <driver/flash_partition.h>
extern uint8_t uvc_ok_flag; // flag for uvc connect or disconnect status
// define by lcd, also can according to lcd_device
#define DISPLAY_WIDTH 480
#define DISPLAY_HEIGTH 854
// #define DISPLAY_COLOR (0xF800) // RGB565_LE
static frame_buffer_t *startup_frame = NULL;
//static uint8_t fram_read_tag = 0;
beken_timer_t test_timer={0};
#define DISPLAY_LENGTH (DISPLAY_WIDTH * DISPLAY_HEIGTH)
void  read_rgb_data_from_flash(uint32_t address, uint8_t *dst_buffer)
{
    os_printf("%s %d\n\r", __func__, __LINE__);
    for (uint32_t addr = address; addr < (address + DISPLAY_LENGTH * 2); addr += FLASH_PAGE_SIZE) // flash addr according to img data bin download addr
    {
        bk_flash_read_bytes(addr, (uint8_t*)dst_buffer, FLASH_PAGE_SIZE);
        dst_buffer += FLASH_PAGE_SIZE;
        if (addr == address + DISPLAY_LENGTH)
        {
            os_printf("%s %d\n\r", __func__, __LINE__);
            extern void bk_wdt_force_feed(void);
            bk_wdt_force_feed();
        }
    }
}

static void __set_fill_sf_buffer(void *param)
{
    uint32_t address = (uint32_t) param;

    os_printf("startup image address: %x\r\n", address);

    if (lcd_disp_config != NULL) {
        os_printf("lcd display task state: %d\r\n", lcd_disp_config->disp_task_running);
    } else {
        os_printf("lcd display task not init\r\n");
    }
    if(startup_frame == NULL)
    {
        startup_frame = frame_buffer_display_malloc(DISPLAY_WIDTH*DISPLAY_HEIGTH*2);
        if(startup_frame == NULL) {
            os_printf("====malloc error===\r\n");
            return;
        }

        uint8_t *tmp = (uint8_t *)startup_frame->frame;
        startup_frame->fmt = PIXEL_FMT_RGB565_LE;
        startup_frame->width = DISPLAY_WIDTH;
        startup_frame->height = DISPLAY_HEIGTH;
        read_rgb_data_from_flash(address, tmp);
    }
}

static void __set_clean_sf_buffer(void *param)
{
    if (startup_frame)
        frame_buffer_display_free(startup_frame);
    startup_frame = NULL;
}

static void __logo_timer_callback(void *param)
{
    if (lcd_disp_config != NULL) {
        if (lcd_disp_config->disp_task_running) {
            if (lcd_display_frame_request(startup_frame) != BK_OK) {
                os_printf("frame_buffer_display_free\n\r");
                frame_buffer_display_free(startup_frame);
            }
        }
    }
}

void lcd_startup_frame_open(void *config)
{
    __set_fill_sf_buffer(config);
    rtos_init_timer(&test_timer, 100, __logo_timer_callback, NULL);
    rtos_start_timer(&test_timer);
}

static bk_err_t lcd_display_task_stop(void);
void lcd_startup_frame_close(void *config)
{
    rtos_stop_timer(&test_timer);
    lcd_display_task_stop();
    rtos_deinit_timer(&test_timer);
    __set_clean_sf_buffer(NULL);
}
#endif
// Modified by TUYA End

static void lcd_display_task_entry(beken_thread_arg_t data)
{
	lcd_disp_config->disp_task_running = true;

	rtos_set_semaphore(&lcd_disp_config->disp_task_sem);

	while (lcd_disp_config->disp_task_running)
	{
		display_msg_t msg;
		int ret = rtos_pop_from_queue(&lcd_disp_config->queue, &msg, BEKEN_WAIT_FOREVER);
		if (ret == BK_OK)
		{
			switch (msg.event)
			{
				case DISPLAY_FRAME_REQUEST:
#if (CONFIG_LCD_FONT_BLEND || CONFIG_LCD_DMA2D_BLEND)
                    lcd_font_handle((frame_buffer_t*)msg.param, lcd_disp_config->lcd_width, lcd_disp_config->lcd_height);
#endif
					lcd_display_frame((frame_buffer_t*)msg.param);
					break;
				case DISPLAY_FRAME_FREE:
					frame_buffer_display_free((frame_buffer_t*)msg.param);
					break;
				case DISPLAY_FRAME_EXTI:
				{
					rtos_lock_mutex(&service_info->lock);

					GLOBAL_INT_DECLARATION();
					GLOBAL_INT_DISABLE();
					lcd_disp_config->disp_task_running = false;
					GLOBAL_INT_RESTORE();

					do {
						ret = rtos_pop_from_queue(&lcd_disp_config->queue, &msg, BEKEN_NO_WAIT);

						if (ret == BK_OK)
						{
							if (msg.event == DISPLAY_FRAME_REQUEST || msg.event == DISPLAY_FRAME_FREE)
							{
								frame_buffer_display_free((frame_buffer_t*)msg.param);
							}
						}
					} while (ret == BK_OK);

					rtos_unlock_mutex(&service_info->lock);
				}
				goto exit;
				break;
			}
		}
	}

exit:

	lcd_disp_config->disp_task = NULL;
	rtos_set_semaphore(&lcd_disp_config->disp_task_sem);
	rtos_delete_thread(NULL);
}


static bk_err_t lcd_display_task_start(void)
{
	int ret = BK_OK;

	ret = rtos_init_queue(&lcd_disp_config->queue,
							"display_queue",
							sizeof(display_msg_t),
							15);

	if (ret != BK_OK)
	{
		LOGE("%s, init display_queue failed\r\n", __func__);
		return ret;
	}

	ret = rtos_create_thread(&lcd_disp_config->disp_task,
							BEKEN_DEFAULT_WORKER_PRIORITY,
							"display_thread",
							(beken_thread_function_t)lcd_display_task_entry,
							1024 * 2,
							(beken_thread_arg_t)NULL);

	if (BK_OK != ret)
	{
		LOGE("%s lcd_display_thread init failed\n", __func__);
		return ret;
	}

	ret = rtos_get_semaphore(&lcd_disp_config->disp_task_sem, BEKEN_NEVER_TIMEOUT);

	if (BK_OK != ret)
	{
		LOGE("%s decoder_sem get failed\n", __func__);
	}

	return ret;
}

static bk_err_t lcd_display_task_stop(void)
{
	bk_err_t ret = BK_OK;
	if (!lcd_disp_config || lcd_disp_config->disp_task_running == false)
	{
		LOGI("%s already stop\n", __func__);
		return ret;
	}

	lcd_display_task_send_msg(DISPLAY_FRAME_EXTI, 0);

	ret = rtos_get_semaphore(&lcd_disp_config->disp_task_sem, BEKEN_NEVER_TIMEOUT);

	if (BK_OK != ret)
	{
		LOGE("%s jpeg_display_sem get failed\n", __func__);
	}

	if (lcd_disp_config->queue)
	{
		rtos_deinit_queue(&lcd_disp_config->queue);
		lcd_disp_config->queue = NULL;
	}

	LOGI("%s complete\n", __func__);

	return ret;
}


bk_err_t lcd_display_config_free(void)
{
	int ret = BK_OK;

	if (lcd_disp_config)
	{
		if (lcd_disp_config->disp_task_sem)
		{
			rtos_deinit_semaphore(&lcd_disp_config->disp_task_sem);
			lcd_disp_config->disp_task_sem = NULL;
		}

		lcd_driver_deinit();

		if (lcd_disp_config->disp_sem)
		{
			rtos_deinit_semaphore(&lcd_disp_config->disp_sem);
			lcd_disp_config->disp_sem = NULL;
		}

		if (lcd_disp_config->pingpong_frame)
		{
			LOGI("%s pingpong_frame free\n", __func__);
			frame_buffer_display_free(lcd_disp_config->pingpong_frame);
			lcd_disp_config->pingpong_frame = NULL;
		}

		if (lcd_disp_config->display_frame)
		{
			LOGI("%s display_frame free\n", __func__);
			frame_buffer_display_free(lcd_disp_config->display_frame);
			lcd_disp_config->display_frame = NULL;
		}

		if (lcd_disp_config)
		{
			os_free(lcd_disp_config);
			lcd_disp_config = NULL;
		}
	}
	LOGD("%s %d\n", __func__, __LINE__);
	return ret;
}


bool check_lcd_task_is_open(void)
{
	if (lcd_disp_config == NULL)
	{
		return false;
	}
	else
	{
		return lcd_disp_config->disp_task_running;
	}
}


bk_err_t lcd_display_open(lcd_open_t *lcd_config)
{
	int ret = BK_OK;
	const lcd_device_t *lcd_device = NULL;

	if (lcd_disp_config && lcd_disp_config->disp_task_running)
	{
		LOGE("%s lcd display task is running!\r\n", __func__);
		return ret;
	}

	lcd_disp_config = (lcd_disp_config_t *)os_malloc(sizeof(lcd_disp_config_t));
	if (lcd_disp_config == NULL)
	{
		LOGE("%s, malloc lcd_disp_config fail!\r\n", __func__);
		ret = BK_FAIL;
		return ret;
	}

	os_memset(lcd_disp_config, 0, sizeof(lcd_disp_config_t));

	ret = rtos_init_semaphore(&lcd_disp_config->disp_task_sem, 1);
	if (ret != BK_OK)
	{
		LOGE("%s disp_task_sem init failed: %d\n", __func__, ret);
		goto out;
	}

	ret = rtos_init_semaphore(&lcd_disp_config->disp_sem, 1);
	if (ret != BK_OK)
	{
		LOGE("%s disp_sem init failed: %d\n", __func__, ret);
		goto out;
	}

	if (lcd_config->device_name != NULL)
		lcd_device = get_lcd_device_by_name(lcd_config->device_name);

	if (lcd_device == NULL)
	{
		lcd_device = get_lcd_device_by_ppi(lcd_config->device_ppi);
	}

	if (lcd_device == NULL)
	{
		LOGE("%s lcd device not found\n", __func__);
		goto out;
	}
    lcd_disp_config->lcd_width = lcd_device->ppi >> 16;
    lcd_disp_config->lcd_height = lcd_device->ppi & 0xFFFF;

	// step 4: init frame buffer
	LOGI("%s %d lcd ppi:%d %d\n", __func__, __LINE__, lcd_disp_config->lcd_width, lcd_disp_config->lcd_height);

	// step 5: init lcd display
	ret = lcd_driver_init(lcd_device);
	if (ret != BK_OK)
	{
		LOGE("%s, lcd_driver_init fail\r\n", __func__);
		goto out;
	}

	if(lcd_device->type == LCD_TYPE_MCU8080)
		bk_lcd_isr_register(I8080_OUTPUT_EOF, lcd_driver_display_mcu_isr);
	else
		bk_lcd_isr_register(RGB_OUTPUT_EOF, lcd_driver_display_rgb_isr);

	media_debug->fps_lcd= 0;
	media_debug->isr_lcd = 0;

#if (CONFIG_LCD_FONT_BLEND || CONFIG_LCD_DMA2D_BLEND)
    ret = lcd_blend_malloc_buffer();
    if (ret != BK_OK) {
        LOGE("lcd blend malloc error, lcd blend open fail\r\n");
        goto out;
    }
    ret = lcd_dma2d_blend_init();
    if (ret != BK_OK)
    {
        LOGE("lcd blend malloc error, lcd blend open fail\r\n");
        goto out;
    }
#endif

    if (lcd_device->type == LCD_TYPE_SPI) {
    #if CONFIG_LCD_SPI_DISPLAY
        lcd_spi_init(lcd_device);
    #endif
    } else if (lcd_device->type == LCD_TYPE_QSPI) {
    #if CONFIG_LCD_QSPI
        bk_lcd_qspi_disp_task_start(lcd_device);
        lcd_disp_config->disp_task_running = true;
    #endif
    } else {
        ret = lcd_display_task_start();
        if (ret != BK_OK)
        {
            LOGE("%s lcd_display_task_start failed: %d\n", __func__, ret);
            goto out;
        }
    }

    lcd_driver_backlight_open();

	LOGI("%s %d complete\n", __func__, __LINE__);

	return ret;

out:

	LOGE("%s failed\r\n", __func__);
	lcd_display_config_free();

	return ret;
}

bk_err_t lcd_display_close(void)
{
	LOGI("%s, %d\n", __func__, __LINE__);

	if (lcd_disp_config == NULL)
	{
		LOGE("%s, have been closed!\r\n", __func__);
		return BK_OK;
	}

	lcd_driver_backlight_close();

#if CONFIG_LCD_QSPI
    bk_lcd_qspi_disp_task_stop();
    lcd_disp_config->disp_task_running = false;
#else
	lcd_display_task_stop();
#endif

	lcd_display_config_free();
#if (CONFIG_LCD_DMA2D_BLEND || CONFIG_LCD_FONT_BLEND)
    lcd_blend_free_buffer();
    lcd_dma2d_blend_deinit();
#endif
	LOGI("%s complete, %d\n", __func__, __LINE__);

	return BK_OK;
}

bk_err_t lcd_display_service_init(void)
{
	bk_err_t ret =  BK_FAIL;

	service_info = os_malloc(sizeof(display_service_info_t));

	if (service_info == NULL)
	{
		LOGE("%s, malloc service_info failed\n", __func__);
		goto error;
	}

	os_memset(service_info, 0, sizeof(display_service_info_t));

	ret = rtos_init_mutex(&service_info->lock);

	if (ret != BK_OK)
	{
		LOGE("%s, init mutex failed\n", __func__);
		goto error;
	}

	return BK_OK;

error:

	return BK_FAIL;
}
