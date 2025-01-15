// Copyright 2022-2023 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <driver/dma.h>
#include <driver/int.h>
#include <os/mem.h>
#include <driver/h264.h>
#include <driver/h264_types.h>
#include <driver/gpio.h>
#include "clock_driver.h"
#include "gpio_driver.h"
#include "sys_driver.h"
#include "sys_hal.h"
#include "h264_hal.h"
#include "h264_driver.h"
#include "h264_default_config.h"

typedef struct {
	h264_isr_t isr_handler;
	void *param;
} h264_isr_handler_t;

typedef struct {
	h264_hal_t hal;
	h264_isr_handler_t h264_isr_handler[H264_ISR_MAX];
} h264_driver_t;

#define H264_QUALITY_LOW         (1)
#define H264_QUALITY_MIDDLE      (2)
#define H264_QUALITY_HIGH        (3)
#define H264_RETURN_ON_DRIVER_NOT_INIT() do {\
		if (!s_h264_driver_is_init) {\
			return BK_ERR_H264_DRIVER_NOT_INIT;\
		}\
	} while (0)

extern void bk_delay(int num);

static h264_driver_t s_h264 = {0};
static uint8_t h264_dma_rx_id = 0;
static bool s_h264_driver_is_init = false;
static uint32_t h264_int_count[H264_ISR_MAX] = {0};
static compress_ratio_t h264_compress = {0};

static void h264_isr(void);

bk_err_t h264_int_enable(void)
{
	uint32_t int_level = rtos_disable_int();
	sys_drv_int_group2_enable(H264_INTERRUPT_CTRL_BIT);
	h264_hal_int_config(&s_h264.hal, H264_INT_ENABLE);
	rtos_enable_int(int_level);
	return BK_OK;
}

static void h264_init_common(void)
{
	/* h264 clock set */
	sys_drv_set_clk_div_mode1_clkdiv_h264(1);
	sys_drv_set_h264_clk_sel(H264_CLK_SEL_480M);
	sys_drv_set_jpeg_clk_en(1);

	/* h264 AHB bus clock config */
	sys_drv_set_h264_clk_en();
	//sys_drv_mclk_mux_set(H264_AHB_CLK_SEL);
	//sys_drv_h264_set_mclk_div(H264_AHB_CLK_DIV);

	/* h264 global ctrl set */
	h264_hal_set_global_ctrl(&s_h264.hal);
	h264_hal_enable_external_clk_en(&s_h264.hal);
}

bk_err_t bk_h264_pingpong_in_psram_clk_set(void)
{
	h264_hal_enable_external_clk_en(&s_h264.hal);
	return BK_OK;
}

bk_err_t bk_h264_dma_rx_init(h264_dma_config_t *config)
{
	if((config->rx_buf == NULL) || (config->transfer_len <= 0)) {
		return BK_ERR_DMA_INVALID_ADDR;
	}
	dma_config_t dma_config = {0};

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_H264;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = H264_R_RX_FIFO;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
	dma_config.dst.start_addr = (uint32_t)config->rx_buf;

	h264_dma_rx_id = bk_dma_alloc(DMA_DEV_H264);
	if(h264_dma_rx_id >= DMA_ID_MAX) {
		return BK_ERR_DMA_ID;
	}
	BK_LOG_ON_ERR(bk_dma_init(h264_dma_rx_id, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(h264_dma_rx_id, config->transfer_len));
	#if (CONFIG_SPE)
	BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(h264_dma_rx_id, DMA_ATTR_SEC));
	BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(h264_dma_rx_id, DMA_ATTR_SEC));
	#endif
	BK_LOG_ON_ERR(bk_dma_start(h264_dma_rx_id));

	return BK_OK;
}

bk_err_t bk_h264_enc_lcd_dma_cpy(void *out, const void *in, uint32_t len, dma_id_t cpy_chnl)
{
	dma_config_t dma_config = {0};
	os_memset(&dma_config, 0, sizeof(dma_config_t));

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 1;

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

	BK_LOG_ON_ERR(bk_dma_init(cpy_chnl, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(cpy_chnl, len));
#if (CONFIG_SPE && CONFIG_GDMA_HW_V2PX)
	BK_LOG_ON_ERR(bk_dma_set_src_burst_len(cpy_chnl, 3));
	BK_LOG_ON_ERR(bk_dma_set_dest_burst_len(cpy_chnl, 3));
	BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(cpy_chnl, DMA_ATTR_SEC));
	BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(cpy_chnl, DMA_ATTR_SEC));
#endif

	return BK_OK;
}

bk_err_t bk_h264_dma_rx_deinit(void)
{
	BK_LOG_ON_ERR(bk_dma_stop(h264_dma_rx_id));
	BK_LOG_ON_ERR(bk_dma_deinit(h264_dma_rx_id));
	BK_LOG_ON_ERR(bk_dma_free(DMA_DEV_H264, h264_dma_rx_id));
	h264_dma_rx_id = 0;
	return BK_OK;
}

bk_err_t h264_int_disable(void)
{
	uint32_t int_level = rtos_disable_int();
	h264_hal_int_config(&s_h264.hal, H264_CPU_INT_DISABLE);
	sys_drv_int_group2_disable(H264_INTERRUPT_CTRL_BIT);
	rtos_enable_int(int_level);
	return BK_OK;
}

bk_err_t bk_h264_driver_init(void)
{
	if (s_h264_driver_is_init) {
		return BK_OK;
	}

	os_memset(&s_h264, 0, sizeof(s_h264));
	bk_int_isr_register(INT_SRC_H264, h264_isr, NULL);
	h264_hal_init(&s_h264.hal);
	s_h264_driver_is_init = true;

	return BK_OK;
}

bk_err_t bk_h264_driver_deinit(void)
{
	if (!s_h264_driver_is_init)
	{
		return BK_OK;
	}

	bk_int_isr_unregister(INT_SRC_H264);
	h264_int_disable();
	os_memset(&s_h264, 0, sizeof(s_h264));
	s_h264_driver_is_init = false;

	return BK_OK;
}

bk_err_t bk_h264_config_init(const h264_config_t *config, uint16_t media_width, uint16_t media_height)
{
	if(config == NULL) {
		return BK_ERR_H264_INVALID_CONFIG_PARAM;
	}
	h264_init_common();
	if(h264_hal_set_img_width(&s_h264.hal, media_width, media_height)) {
		return BK_ERR_H264_INVALID_RESOLUTION_TYPE;
	}
	h264_hal_gop_config(&s_h264.hal, config);
	h264_hal_qp_config(&s_h264.hal, config);
	h264_hal_idc_config(&s_h264.hal, config);
	h264_hal_mb_config(&s_h264.hal, config);
	h264_hal_scale_factor_config(&s_h264.hal);
	h264_hal_vui_config(&s_h264.hal, config);
	h264_hal_cpb_vui_config(&s_h264.hal, config);
	h264_hal_frame_cbr_config(&s_h264.hal, config);
	h264_int_enable();

	return BK_OK;
}

bk_err_t bk_h264_init(media_ppi_t media_ppi)
{
	int ret = BK_OK;
	H264_RETURN_ON_DRIVER_NOT_INIT();
	uint16_t media_width = ppi_to_pixel_x(media_ppi);
	uint16_t media_height = ppi_to_pixel_y(media_ppi);
	if (media_height % 16 != 0) {
		H264_LOGE("h264 input pixel height is not n times of 16!! \r\n");
		return BK_ERR_H264_INVALID_PIXEL_HEIGHT;
	}

	ret = bk_h264_config_init(&h264_commom_config, media_width, media_height);
	if (ret != BK_OK)
	{
		H264_LOGE("%s, set commom config error!\r\n", __func__);
		return ret;
	}

	if (CONFIG_H264_QUALITY_LEVEL == H264_QUALITY_HIGH)
	{
		h264_compress.qp.init_qp = 25;
		h264_compress.qp.i_max_qp = 25;
		h264_compress.qp.p_max_qp = 25;
		h264_compress.imb_bits = 200;
		h264_compress.pmb_bits = 80;
		h264_compress.enable = true;
	}
	else if (CONFIG_H264_QUALITY_LEVEL == H264_QUALITY_MIDDLE)
	{
		h264_compress.qp.init_qp = 40;
		h264_compress.qp.i_max_qp = 40;
		h264_compress.qp.p_max_qp = 25;
		h264_compress.imb_bits = 160;
		h264_compress.pmb_bits = 60;
		h264_compress.enable = true;
	}
	else if (CONFIG_H264_QUALITY_LEVEL == H264_QUALITY_LOW)
	{
		// Modified by TUYA Start
		//h264_compress.qp.init_qp = 40;
		//h264_compress.qp.i_max_qp = 40;
		//h264_compress.qp.p_max_qp = 25;
		//h264_compress.imb_bits = 130;
		//h264_compress.pmb_bits = 50;
		h264_compress.qp.init_qp = 30;
		h264_compress.qp.i_max_qp = 30;
		h264_compress.qp.p_max_qp = 10;
		h264_compress.imb_bits = 240;
		h264_compress.pmb_bits = 40;
		// Modified by TUYA End
		h264_compress.enable = true;
	}

	if (h264_compress.enable)
	{
		ret = bk_h264_set_qp(&h264_compress.qp);
		if (ret != BK_OK)
		{
			H264_LOGE("%s, set qp config error!\r\n", __func__);
			return ret;
		}

		bk_h264_set_quality(h264_compress.imb_bits, h264_compress.pmb_bits);

		h264_compress.enable = false;
	}

	return ret;
}

bk_err_t bk_h264_set_base_config(compress_ratio_t *config)
{
	int ret = BK_OK;

	if (h264_compress.enable)
	{
		H264_LOGE("%s, already enable!!!\r\n", __func__);
		ret = BK_FAIL;
		goto error;
	}

	if(config->imb_bits > MAX_IFRAME_BITS || config->imb_bits < MIN_FRAME_BITS)
	{
		ret = BK_ERR_H264_INVALID_IMB_BITS;
		goto error;
	}

	if(config->pmb_bits > MAX_PFRAME_BITS || config->pmb_bits < MIN_FRAME_BITS)
	{
		ret = BK_ERR_H264_INVALID_PMB_BITS;
		goto error;
	}

	if(config->qp.init_qp > MAX_QP || config->qp.init_qp < MIN_QP)
	{
		ret = BK_ERR_H264_INVALID_QP;
		goto error;
	}

	if (config->qp.i_min_qp >= config->qp.i_max_qp || config->qp.p_min_qp >= config->qp.p_max_qp)
	{
		ret =  BK_ERR_H264_INVALID_QP;
		goto error;
	}

	if (config->qp.i_max_qp > MAX_QP || config->qp.p_max_qp > MAX_QP)
	{
		ret =  BK_ERR_H264_INVALID_QP;
		goto error;
	}

	os_memcpy(&h264_compress, config, sizeof(compress_ratio_t));

	return ret;

error:
	H264_LOGE("%s, %d\r\n", __func__, ret);
	return ret;

}

bk_err_t bk_h264_deinit(void)
{
	H264_RETURN_ON_DRIVER_NOT_INIT();

	h264_int_disable();
	bk_h264_soft_reset();
	h264_hal_reset(&s_h264.hal);
	sys_drv_h264_pwr_down();
	os_memset(&h264_compress, 0, sizeof(compress_ratio_t));
	for (uint8_t i = 0; i < H264_ISR_MAX; i++)
	{
		bk_h264_unregister_isr(i);
	}

	return BK_OK;
}

bk_err_t bk_h264_set_pframe_num(uint32_t pframe_number)
{
	H264_RETURN_ON_DRIVER_NOT_INIT();
	if(pframe_number > MAX_PFRAME || pframe_number < MIN_PFRAME) {
		return BK_ERR_H264_INVALID_PFRAME_NUMBER;
	}
	h264_config_t param_set = {0};
	param_set.skip_mode = DEFAULT_SKIP_MODE;
	param_set.num_pframe = pframe_number;

	const h264_config_t *para = &param_set;
	h264_hal_gop_config(&s_h264.hal, para);

	return BK_OK;
}

bk_err_t bk_h264_set_qp(h264_qp_t *qp)
{
	H264_RETURN_ON_DRIVER_NOT_INIT();
	if(qp->init_qp > MAX_QP || qp->init_qp < MIN_QP) {
		return BK_ERR_H264_INVALID_QP;
	}

	if (qp->i_min_qp >= qp->i_max_qp || qp->p_min_qp >= qp->p_max_qp)
	{
		return BK_ERR_H264_INVALID_QP;
	}

	if (qp->i_max_qp > MAX_QP || qp->p_max_qp > MAX_QP)
	{
		return BK_ERR_H264_INVALID_QP;
	}

	h264_config_t param_set = {0};
	param_set.qp = qp->init_qp;
	param_set.cqp_off = h264_commom_config.cqp_off;
	param_set.tqp = h264_commom_config.tqp;
	param_set.iframe_max_qp = qp->i_max_qp;
	param_set.iframe_min_qp = h264_commom_config.iframe_min_qp;
	param_set.pframe_max_qp = qp->p_max_qp;
	param_set.pframe_min_qp = h264_commom_config.pframe_min_qp;

	const h264_config_t *para = &param_set;
	h264_hal_qp_config(&s_h264.hal,para);

	return BK_OK;
}

bk_err_t bk_h264_set_quality(uint32_t imb_bits, uint32_t pmb_bits)
{
	H264_RETURN_ON_DRIVER_NOT_INIT();
	if(imb_bits > MAX_IFRAME_BITS || imb_bits < MIN_FRAME_BITS) {
		return BK_ERR_H264_INVALID_IMB_BITS;
	}
	if(pmb_bits > MAX_PFRAME_BITS || pmb_bits < MIN_FRAME_BITS) {
		return BK_ERR_H264_INVALID_PMB_BITS;
	}
	h264_config_t param_set = {0};
	param_set.imb_bits = imb_bits;
	param_set.pmb_bits = pmb_bits;

	const h264_config_t *para = &param_set;
	h264_hal_mb_bits_config(&s_h264.hal, para);

	return BK_OK;
}

bk_err_t bk_h264_deblocking_filter_config_init(const h264_config_t *config, uint32_t alpha_off, uint32_t beta_off)
{
	H264_RETURN_ON_DRIVER_NOT_INIT();
	return h264_hal_filter_config(&s_h264.hal, config, alpha_off, beta_off);
}

bk_err_t bk_h264_encode_enable()
{
	H264_RETURN_ON_DRIVER_NOT_INIT();
	h264_hal_encode_enable(&s_h264.hal);
	return BK_OK;
}

bk_err_t bk_h264_encode_disable()
{
	H264_RETURN_ON_DRIVER_NOT_INIT();
	h264_hal_encode_disable(&s_h264.hal);
	return BK_OK;
}

bk_err_t bk_h264_camera_gpio_set(void)
{
	h264_gpio_map_t h264_gpio_map_table[] = H264_GPIO_MAP;

	for (uint32_t i = 0; i < 10; i++) {
		gpio_dev_unmap(h264_gpio_map_table[i].gpio_id);
		gpio_dev_map(h264_gpio_map_table[i].gpio_id, h264_gpio_map_table[i].dev);
	}

	return BK_OK;
}

bk_err_t bk_h264_camera_clk_enhance(void)
{
	h264_gpio_map_t h264_gpio_map_table[] = H264_GPIO_MAP;

	for (uint32_t i = 10; i < 12; i++) {
		gpio_dev_unmap(h264_gpio_map_table[i].gpio_id);
		gpio_dev_map(h264_gpio_map_table[i].gpio_id, h264_gpio_map_table[i].dev);
	}

	bk_gpio_disable_output(h264_gpio_map_table[0].gpio_id);
	bk_gpio_set_capacity(h264_gpio_map_table[0].gpio_id,1);

	return BK_OK;
}

uint32_t bk_h264_get_encode_count()
{
	H264_RETURN_ON_DRIVER_NOT_INIT();
	return h264_hal_get_encode_count(&s_h264.hal);
}

uint32_t bk_h264_get_p_frame_number(void)
{
	H264_RETURN_ON_DRIVER_NOT_INIT();

	return h264_hal_get_num_pframe(&s_h264.hal);
}

bk_err_t bk_h264_get_fifo_addr(uint32_t *fifo_addr)
{
	*fifo_addr = H264_R_RX_FIFO;
	return BK_OK;
}

bk_err_t bk_h264_register_isr(h264_isr_type_t type_id, h264_isr_t isr, void *param)
{
	if(type_id >= H264_ISR_MAX) {
		return BK_ERR_H264_ISR_INVALID_ID;
	}
	uint32_t int_level = rtos_disable_int();
	s_h264.h264_isr_handler[type_id].isr_handler = isr;
	s_h264.h264_isr_handler[type_id].param = param;
	rtos_enable_int(int_level);
	return BK_OK;
}

bk_err_t bk_h264_unregister_isr(h264_isr_type_t type_id)
{
	if(type_id >= H264_ISR_MAX) {
		return BK_ERR_H264_ISR_INVALID_ID;
	}
	uint32_t int_level = rtos_disable_int();
	s_h264.h264_isr_handler[type_id].isr_handler = NULL;
	s_h264.h264_isr_handler[type_id].param = NULL;
	rtos_enable_int(int_level);
	return BK_OK;
}

bk_err_t bk_h264_get_h264_base_config(h264_base_config_t *config)
{
	H264_RETURN_ON_DRIVER_NOT_INIT();
	if (config == NULL)
	{
		H264_LOGE("%s, config is null ptr\r\n");
		return BK_FAIL;
	}

	config->h264_state = s_h264.hal.hw->enc_ctrl.enc_en;
	config->profile_id = s_h264.hal.hw->profile_idc & 0x1;
	config->p_frame_cnt = bk_h264_get_p_frame_number() & 0xFF;
	config->qp = s_h264.hal.hw->qp & 0xFF;
	config->num_imb_bits = s_h264.hal.hw->iframe_bit_ctrl.num_imb_bits & 0xFFF;
	config->num_pmb_bits = s_h264.hal.hw->num_pmb_bits & 0xFFFF;
	config->width = s_h264.hal.hw->img_width & 0xFFFF;
	config->height = s_h264.hal.hw->img_height & 0xFFFF;

	return BK_OK;
}

static void h264_isr(void)
{
	uint32_t int_status = h264_hal_get_int_stat(&s_h264.hal);
	h264_hal_int_clear(&s_h264.hal, int_status);

	H264_LOGD("h264 int status: %x \r\n ", int_status);

	if (int_status & INT_SKIP_FRAME)
	{
		H264_INT_INC(h264_int_count,H264_SKIP_FRAME);
		if (s_h264.h264_isr_handler[H264_SKIP_FRAME].isr_handler)
		{
			s_h264.h264_isr_handler[H264_SKIP_FRAME].isr_handler(0, s_h264.h264_isr_handler[H264_SKIP_FRAME].param);
		}
	}

	if (int_status & INT_FINAL_OUT)
	{
		H264_INT_INC(h264_int_count,H264_FINAL_OUT);
		if (s_h264.h264_isr_handler[H264_FINAL_OUT].isr_handler)
		{
			s_h264.h264_isr_handler[H264_FINAL_OUT].isr_handler(0, s_h264.h264_isr_handler[H264_FINAL_OUT].param);
		}

		if (h264_compress.enable)
		{
			bk_h264_set_qp(&h264_compress.qp);

			bk_h264_set_quality(h264_compress.imb_bits, h264_compress.pmb_bits);

			h264_compress.enable = 0;
		}
	}

	if (int_status & INT_LINE_DONE)
	{
		H264_INT_INC(h264_int_count,H264_LINE_DONE);
		if (s_h264.h264_isr_handler[H264_LINE_DONE].isr_handler)
		{
			s_h264.h264_isr_handler[H264_LINE_DONE].isr_handler(0, s_h264.h264_isr_handler[H264_LINE_DONE].param);
		}
	}
}

bk_err_t bk_h264_int_count_show(void)
{
	H264_LOGI("h264 int SKIP_FRAME count: %d \r\n ", h264_int_count[H264_SKIP_FRAME]);
	H264_LOGI("h264 int FINAL_OUT count: %d \r\n ", h264_int_count[H264_FINAL_OUT]);
	H264_LOGI("h264 int LINE_DONE count: %d \r\n ", h264_int_count[H264_LINE_DONE]);
	return BK_OK;
}

bk_err_t bk_h264_config_reset(void)
{
	int ret = BK_OK;
	uint32_t fps = 0;
	const h264_config_t *config = &h264_commom_config;
	compress_ratio_t ratio = {0};
	H264_RETURN_ON_DRIVER_NOT_INIT();
	uint16_t width = s_h264.hal.hw->img_width & 0xFFFF;
	uint16_t height = s_h264.hal.hw->img_height & 0xFFFF;
	ratio.qp.init_qp = s_h264.hal.hw->qp & 0xFF;
	ratio.qp.i_max_qp = s_h264.hal.hw->iframe_qp_boundary.v & 0x3F;
	ratio.qp.p_max_qp = s_h264.hal.hw->pframe_qp_boudary.v & 0x3F;
	ratio.imb_bits = s_h264.hal.hw->iframe_bit_ctrl.v & 0xFFF;
	ratio.pmb_bits = s_h264.hal.hw->num_pmb_bits & 0xFFFF;
	fps = s_h264.hal.hw->vui_time_scale_L & 0xFFFF;
	h264_hal_reset(&s_h264.hal);

	/* h264 global ctrl set */
	h264_hal_set_global_ctrl(&s_h264.hal);
	h264_hal_enable_external_clk_en(&s_h264.hal);

	if(h264_hal_set_img_width(&s_h264.hal, width, height))
	{
		return BK_ERR_H264_INVALID_RESOLUTION_TYPE;
	}

	h264_hal_gop_config(&s_h264.hal, config);
	h264_hal_qp_config(&s_h264.hal, config);
	h264_hal_idc_config(&s_h264.hal, config);
	h264_hal_mb_config(&s_h264.hal, config);
	h264_hal_scale_factor_config(&s_h264.hal);
	h264_hal_vui_config(&s_h264.hal, config);
	h264_hal_cpb_vui_config(&s_h264.hal, config);
	h264_hal_frame_cbr_config(&s_h264.hal, config);
	bk_h264_set_qp(&ratio.qp);
	bk_h264_set_quality(ratio.imb_bits, ratio.pmb_bits);
	bk_h264_updata_encode_fps(fps);
	h264_int_enable();

	return ret;
}

bk_err_t bk_h264_soft_reset(void)
{
	H264_RETURN_ON_DRIVER_NOT_INIT();
	h264_hal_soft_reset_active(&s_h264.hal);
	h264_hal_soft_reset_deactive(&s_h264.hal);
	return BK_OK;
}

bk_err_t bk_h264_updata_encode_fps(uint32 fps)
{
    H264_RETURN_ON_DRIVER_NOT_INIT();
    h264_hal_set_vui_fps(&s_h264.hal, fps);

    return BK_OK;
}
