// Copyright 2022-2023 Beken
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

#include <common/bk_include.h>
#include <components/system.h>
#include <driver/yuv_buf.h>
#include <driver/int.h>
#include <os/os.h>
#include <os/mem.h>
#include <modules/pm.h>
#include "sys_driver.h"
#include "yuv_buf_driver.h"
#include "yuv_buf_hal.h"
#include <driver/jpeg_enc.h>

typedef struct {
	yuv_buf_isr_t isr_handler;
	void *param;
} yuv_buf_isr_handler_t;

typedef struct {
	yuv_buf_hal_t hal;
	yuv_buf_isr_handler_t yuv_buf_isr_handler[YUV_BUF_ISR_MAX];
	yuv_mode_t cur_work_mode;
} yuv_buf_driver_t;

#define YUV_BUF_RETURN_ON_DRIVER_NOT_INIT() do {\
	if (!s_yuv_buf_driver_is_init) {\
		return BK_ERR_YUV_BUF_DRIVER_NOT_INIT;\
	}\
} while(0)

static yuv_buf_driver_t s_yuv_buf = {0};
static bool s_yuv_buf_driver_is_init = false;

static void yuv_buf_isr(void);

static void yuv_buf_init_common(void)
{
	/* 1) power on yuv_buf
	 * 2) enable yuv_buf system interrupt
	 */
	sys_drv_yuv_buf_pwr_up();
	sys_drv_int_group2_enable(YUV_BUF_INTERRUPT_CTRL_BIT);
	yuv_buf_hal_set_global_ctrl(&s_yuv_buf.hal);
}

static void yuv_buf_deinit_common(void)
{
	yuv_buf_hal_soft_reset(&s_yuv_buf.hal);
	/* 1) power off yuv_buf
	 * 2) disable yuv_buf system interrupt
	 * 3) unregister isr
	 */
	sys_drv_yuv_buf_pwr_down();
	sys_drv_int_group2_disable(YUV_BUF_INTERRUPT_CTRL_BIT);
	for (uint8_t i = 0; i < YUV_BUF_ISR_MAX; i++)
	{
		bk_yuv_buf_unregister_isr(i);
	}
}

bk_err_t bk_yuv_buf_driver_init(void)
{
	if (s_yuv_buf_driver_is_init) {
		return BK_OK;
	}

	os_memset(&s_yuv_buf, 0, sizeof(s_yuv_buf));
	bk_int_isr_register(INT_SRC_YUVB, yuv_buf_isr, NULL);
	yuv_buf_hal_init(&s_yuv_buf.hal);
	s_yuv_buf_driver_is_init = true;

	return BK_OK;
}

bk_err_t bk_yuv_buf_driver_deinit(void)
{
	if (!s_yuv_buf_driver_is_init) {
		return BK_OK;
	}
	bk_int_isr_unregister(INT_SRC_YUVB);
	yuv_buf_deinit_common();
	os_memset(&s_yuv_buf, 0, sizeof(s_yuv_buf));
	s_yuv_buf_driver_is_init = false;

	return BK_OK;
}

bk_err_t bk_yuv_buf_init(const yuv_buf_config_t *config)
{
	BK_RETURN_ON_NULL(config);
	YUV_BUF_RETURN_ON_DRIVER_NOT_INIT();

	// set cpu frequent to 320M
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_JPEG, PM_CPU_FRQ_480M);

	yuv_buf_init_common();

	switch (config->work_mode) {
		case JPEG_MODE:
			yuv_buf_hal_set_jpeg_mode_config(&s_yuv_buf.hal, config);
			break;
		case YUV_MODE:
			yuv_buf_hal_set_yuv_mode_config(&s_yuv_buf.hal, config);
			break;
		case H264_MODE:
			yuv_buf_hal_set_h264_mode_config(&s_yuv_buf.hal, config);
			break;
		default:
			break;
	}

	return BK_OK;
}

bk_err_t bk_yuv_buf_deinit(void)
{
	YUV_BUF_RETURN_ON_DRIVER_NOT_INIT();

	yuv_buf_deinit_common();

	// set cpu frequent to 320M
	bk_pm_module_vote_cpu_freq(PM_DEV_ID_JPEG, PM_CPU_FRQ_DEFAULT);

	return BK_OK;
}

bk_err_t bk_yuv_buf_set_frame_resolution(uint32_t width, uint32_t height)
{
	YUV_BUF_RETURN_ON_DRIVER_NOT_INIT();

	yuv_buf_set_frame_resolution(&s_yuv_buf.hal, width, height);

	return BK_OK;
}

bk_err_t bk_yuv_buf_set_resize(const yuv_buf_resize_config_t *config)
{
	YUV_BUF_RETURN_ON_DRIVER_NOT_INIT();

	yuv_buf_hal_set_resize_mode_config(&s_yuv_buf.hal, config);

	return BK_OK;
}

bk_err_t bk_yuv_buf_start(yuv_mode_t work_mode)
{
	s_yuv_buf.cur_work_mode = work_mode;
	switch (work_mode) {
	case YUV_MODE:
		yuv_buf_hal_start_yuv_mode(&s_yuv_buf.hal);
		break;
	case JPEG_MODE:
		yuv_buf_hal_start_jpeg_mode(&s_yuv_buf.hal);
		break;
	case H264_MODE:
		yuv_buf_hal_start_h264_mode(&s_yuv_buf.hal);
		break;
	default:
		break;
	}

	return BK_OK;
}

bk_err_t bk_yuv_buf_stop(yuv_mode_t work_mode)
{
	switch (work_mode) {
	case YUV_MODE:
		yuv_buf_hal_stop_yuv_mode(&s_yuv_buf.hal);
		break;
	case JPEG_MODE:
		yuv_buf_hal_stop_jpeg_mode(&s_yuv_buf.hal);
		break;
	case H264_MODE:
		yuv_buf_hal_stop_h264_mode(&s_yuv_buf.hal);
		break;
	default:
		break;
	}

	return BK_OK;
}

bk_err_t bk_yuv_buf_rencode_start(void)
{
	yuv_buf_hal_rencode_start(&s_yuv_buf.hal);

	return BK_OK;
}

bk_err_t bk_yuv_buf_enable_nosensor_encode_mode(void)
{
	yuv_buf_hal_enable_nosensor_encode_mode(&s_yuv_buf.hal);

	return BK_OK;
}

bk_err_t bk_yuv_buf_set_mclk_div(mclk_div_t div)
{
	YUV_BUF_RETURN_ON_DRIVER_NOT_INIT();

	yuv_buf_hal_set_mclk_div(&s_yuv_buf.hal, div);

	return BK_OK;
}

bk_err_t bk_yuv_buf_set_em_base_addr(uint32_t em_base_addr)
{
	YUV_BUF_RETURN_ON_DRIVER_NOT_INIT();
	yuv_buf_hal_set_em_base_addr(&s_yuv_buf.hal, em_base_addr);
	yuv_buf_hal_set_emr_base_addr(&s_yuv_buf.hal, em_base_addr);
	return BK_OK;
}

uint32_t bk_yuv_buf_get_em_base_addr(void)
{
	YUV_BUF_RETURN_ON_DRIVER_NOT_INIT();

	return yuv_buf_hal_get_em_base_addr(&s_yuv_buf.hal);
}

bk_err_t bk_yuv_buf_set_emr_base_addr(uint32_t emr_base_addr)
{
	YUV_BUF_RETURN_ON_DRIVER_NOT_INIT();
	yuv_buf_hal_set_emr_base_addr(&s_yuv_buf.hal, emr_base_addr);
	return BK_OK;
}

uint32_t bk_yuv_buf_get_emr_base_addr(void)
{
	YUV_BUF_RETURN_ON_DRIVER_NOT_INIT();

	return yuv_buf_hal_get_emr_base_addr(&s_yuv_buf.hal);
}


bk_err_t bk_yuv_buf_register_isr(yuv_buf_isr_type_t type_id, yuv_buf_isr_t isr, void *param)
{
	uint32_t int_level = rtos_disable_int();
	s_yuv_buf.yuv_buf_isr_handler[type_id].isr_handler = isr;
	s_yuv_buf.yuv_buf_isr_handler[type_id].param = param;
	rtos_enable_int(int_level);

	return BK_OK;
}

bk_err_t bk_yuv_buf_unregister_isr(yuv_buf_isr_type_t type_id)
{
	uint32_t int_level = rtos_disable_int();
	s_yuv_buf.yuv_buf_isr_handler[type_id].isr_handler = NULL;
	s_yuv_buf.yuv_buf_isr_handler[type_id].param = NULL;
	rtos_enable_int(int_level);

	return BK_OK;
}

bk_err_t bk_yuv_buf_init_partial_display(const yuv_buf_partial_offset_config_t *offset_config)
{
	YUV_BUF_RETURN_ON_DRIVER_NOT_INIT();
	yuv_buf_hal_set_partial_display_offset_config(&s_yuv_buf.hal, offset_config);
	yuv_buf_hal_enable_partial_display(&s_yuv_buf.hal);

	return BK_OK;
}

bk_err_t bk_yuv_buf_deinit_partial_display(void)
{
	YUV_BUF_RETURN_ON_DRIVER_NOT_INIT();
	yuv_buf_hal_disable_partial_display(&s_yuv_buf.hal);
	return BK_OK;
}

bk_err_t bk_yuv_buf_soft_reset(void)
{
	YUV_BUF_LOGD("yuv soft reset \r\n");
	YUV_BUF_RETURN_ON_DRIVER_NOT_INIT();

	uint32_t int_level = rtos_disable_int();

	yuv_buf_hal_soft_reset(&s_yuv_buf.hal);

	rtos_enable_int(int_level);

	return BK_OK;
}

static void yuv_buf_isr(void)
{
	yuv_buf_hal_t *hal = &s_yuv_buf.hal;
	uint32_t int_status = yuv_buf_hal_get_interrupt_status(hal);

	YUV_BUF_LOGD("[yuv_buf_isr] int_status:%x\r\n", int_status);
	yuv_buf_hal_clear_interrupt_status(hal, int_status);

	if (yuv_buf_hal_is_vsync_negedge_int_triggered(hal, int_status)) {
		/* MPW2 yuv_arv_int cannot work, regard the VSYNC_NEGEDGE_INT as the YUV_ARV_INT.
		 * need to reopen yuv_mode
		 */
		if (s_yuv_buf.cur_work_mode == YUV_MODE) {
			yuv_buf_hal_disable_yuv_buf_mode(hal);
			yuv_buf_hal_enable_yuv_buf_mode(hal);
		}
		if (s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_VSYNC_NEGEDGE].isr_handler) {
			s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_VSYNC_NEGEDGE].isr_handler(0, s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_VSYNC_NEGEDGE].param);
		}
	}

	if (yuv_buf_hal_is_yuv_arv_int_triggered(hal, int_status)) {
		if (s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_YUV_ARV].isr_handler) {
			s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_YUV_ARV].isr_handler(0, s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_YUV_ARV].param);
		}
	}

	if (yuv_buf_hal_is_sm0_wr_int_triggered(hal, int_status)) {
		if (s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_SM0_WR].isr_handler) {
			s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_SM0_WR].isr_handler(0, s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_SM0_WR].param);
		}
	}

	if (yuv_buf_hal_is_sm1_wr_int_triggered(hal, int_status)) {
		if (s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_SM1_WR].isr_handler) {
			s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_SM1_WR].isr_handler(0, s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_SM1_WR].param);
		}
	}

	if (yuv_buf_hal_is_fifo_full_int_triggered(hal, int_status)) {
		if (s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_FULL].isr_handler) {
			YUV_BUF_LOGE("sensor fifo is full\r\n");
			s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_FULL].isr_handler(0, s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_FULL].param);
		}
	}

	if (yuv_buf_hal_is_enc_line_done_int_triggered(hal, int_status)) {
		if (s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_ENC_LINE].isr_handler) {
			s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_ENC_LINE].isr_handler(0, s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_ENC_LINE].param);
		}
	}

	if (yuv_buf_hal_is_sensor_resolution_err_int_triggered(hal, int_status)) {
		YUV_BUF_LOGE("sensor's yuyv data resolution is not right\r\n");
		if (s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_SEN_RESL].isr_handler) {
			s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_SEN_RESL].isr_handler(0, s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_SEN_RESL].param);
		}
	}

	if (yuv_buf_hal_is_h264_err_int_triggered(hal, int_status)) {
		if (s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_H264_ERR].isr_handler) {
			YUV_BUF_LOGE("h264 encode error\r\n");
			s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_H264_ERR].isr_handler(0, s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_H264_ERR].param);
		}
	}

	if (yuv_buf_hal_is_enc_slow_int_triggered(hal, int_status)) {
		if (s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_ENC_SLOW].isr_handler) {
			YUV_BUF_LOGE("jpeg encode rate is slow than sensor's data rate\r\n");
			s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_ENC_SLOW].isr_handler(0, s_yuv_buf.yuv_buf_isr_handler[YUV_BUF_ENC_SLOW].param);
		}
	}
}

