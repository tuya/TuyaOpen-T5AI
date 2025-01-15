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

#include <stdlib.h>
#include <common/bk_include.h>
#include <os/mem.h>
#include "arch_interrupt.h"
#include "gpio_map.h"
#include "gpio_driver.h"
#include <driver/gpio.h>
#include "gpio_map.h"
#include <driver/int.h>
#include "dma_hal.h"
#include <driver/dma.h>
//#include "dma_driver.h"
#include "sys_driver.h"
#include "jpeg_dec_macro_def.h"
#include "jpeg_dec_ll_macro_def.h"
#include "jpeg_dec_hal.h"
#include <driver/jpeg_dec.h>
#include "driver/jpeg_dec_types.h"
#include <driver/hal/hal_jpeg_dec_types.h>
#include <modules/pm.h>
#include "bk_general_dma.h"
#include <soc/mapping.h>
#include <driver/media_types.h>

#define JPEGDEC_TAG "jpeg hw_decode"
#define LOGI(...) BK_LOGI(JPEGDEC_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(JPEGDEC_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(JPEGDEC_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(JPEGDEC_TAG, ##__VA_ARGS__)

#if (USE_JPEG_DEC_COMPLETE_CALLBACKS == 1)
jpeg_dec_isr_cb_t  s_jpeg_dec_isr[DEC_ISR_MAX] = {NULL};
static jpeg_dec_res_t result = {0};
static void jpeg_decoder_isr(void);
#endif

#define JPEGDEC_RETURN_ON_NOT_INIT() do {\
	if (!s_jpegdec_driver_is_init) {\
		return BK_ERR_JPEGDEC_NOT_INIT;\
	}\
} while(0)
static bool s_jpegdec_driver_is_init = false;

uint32_t image_ppi = 0;
uint8_t *jpeg_address = NULL;
uint32_t jpeg_size = 0;
uint32_t dec_mode = 0;

#define JPEG_TAIL_SIZE (2)
#define JPEG_DEC_STRIP


bk_err_t bk_jpeg_dec_driver_init(void)
{
	if (s_jpegdec_driver_is_init) {
		LOGW("%s, hw jpegdec already init. \n", __func__);
		return BK_OK;
	}

	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_VIDP_JPEG_DE, PM_POWER_MODULE_STATE_ON);
	sys_drv_int_enable(JPEGDEC_INTERRUPT_CTRL_BIT);
	sys_drv_set_jpeg_dec_disckg(1);

#if (USE_JPEG_DEC_COMPLETE_CALLBACKS == 1)
	bk_int_isr_register(INT_SRC_JPEG_DEC, jpeg_decoder_isr, NULL);
#endif
	jpg_decoder_init();

	LOGI("%s, hw jpegdec init \n", __func__);

	s_jpegdec_driver_is_init = true;
	return BK_OK;
}

bk_err_t bk_jpeg_dec_driver_deinit(void)
{
	if (!s_jpegdec_driver_is_init) {
		LOGW("%s, jpegdec already deinit. \n", __func__);
		return BK_OK;
	}

	sys_drv_set_jpeg_dec_disckg(0);
	sys_drv_int_disable(JPEGDEC_INTERRUPT_CTRL_BIT);

	bk_int_isr_unregister(INT_SRC_JPEG_DEC);
	jpg_decoder_deinit();
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_VIDP_JPEG_DE, PM_POWER_MODULE_STATE_OFF);
	s_jpegdec_driver_is_init = false;

	LOGW("%s, hw jpegdec deinit. \n", __func__);

	return BK_OK;
}
bk_err_t jpeg_dec_set_dst_addr(unsigned char * output_buf)
{
	jpeg_dec_ll_set_reg0x59_value((uint32_t)output_buf);
	return BK_OK;
}

bk_err_t bk_jpec_dec_int_en(jpeg_dec_isr_type_t isr_id, bool en)
{
    if (isr_id == DEC_END_OF_FRAME)
    {
        jpeg_dec_auto_frame_end_int_en(en);
    }
    else if (isr_id == DEC_EVERY_LINE_INT)
    {
        jpeg_dec_auto_line_num_int_en(en);
//        bk_jpeg_dec_line_num_set(LINE_16);
    }
    else if (isr_id == DEC_ERR)
    {
#if CONFIG_SOC_BK7236XX
        jpeg_dec_ll_set_reg0x5e_dec_huf_err_int(en);
#endif
    }
    else
    {
        LOGW("%s error. \n", __func__);
    }

	return BK_OK;
}

bk_err_t bk_jpeg_dec_hw_start(uint32_t length, unsigned char *input_buf, unsigned char * output_buf)
{
	int ret = 0;
	JPEGDEC_RETURN_ON_NOT_INIT();

	jpeg_address = input_buf;
	jpeg_size = length;
//    jpeg_dec_ll_set_reg0x56_value(0x04040404);
    result.ok = true;
    bk_jpec_dec_int_en(dec_mode, 1);
	ret = JpegdecInit(length, input_buf, output_buf, &image_ppi);
	if(ret != JDR_OK)
	{
		LOGE("JpegdecInit error %x \r\n", ret);
		return ret;
	}
	ret = jd_decomp_hw();
	if(ret != JDR_OK)
	{
		LOGE("jd_decomp error %x \r\n", ret);
		return ret;
	}
	return JDR_OK;
}

void bk_jpeg_dec_line_num_set(line_num_t line_num)
{
	jpeg_dec_set_line_num(line_num);
}

void bk_jpeg_dec_by_line_start(void)
{
	jpeg_dec_hal_set_dec_cmd(JPEGDEC_DC_START);
}

bool bk_jpeg_dec_by_line_is_last_line_complete(void)
{
	return !jpeg_dec_hal_get_mcu_index();
}

bk_err_t bk_jpeg_dec_stop(void)
{
    bk_jpec_dec_int_en(dec_mode, 0);
	jpeg_dec_hal_set_mcu_x(0);
	jpeg_dec_hal_set_mcu_y(0);
	jpeg_dec_hal_set_dec_cmd(JPEGDEC_DC_CLEAR);
	jpeg_dec_hal_set_int_status_value(0xFFFF);
	jpeg_dec_hal_set_uv_vld_value(0);
	jpeg_dec_hal_set_jpeg_dec_en(0);
	return BK_OK;
}

bk_err_t bk_jpeg_dec_out_format(pixel_format_t   pixel_fmt)
{
#if CONFIG_SOC_BK7236XX
		switch (pixel_fmt)
		{
			case PIXEL_FMT_YUYV:
				jpeg_dec_ll_set_reg0x50_out_fmt(0);
				break;
			case PIXEL_FMT_YYUV:
				jpeg_dec_ll_set_reg0x50_out_fmt(3);
				break;
			
			case PIXEL_FMT_VYUY:
				jpeg_dec_ll_set_reg0x50_out_fmt(2);//vy1uy0
				break;
			case PIXEL_FMT_VUYY:
			default:
				jpeg_dec_ll_set_reg0x50_out_fmt(1);
				break;
		}
#endif
	return BK_OK;
}

#if (USE_JPEG_DEC_COMPLETE_CALLBACKS == 1)
bk_err_t bk_jpeg_dec_isr_register(jpeg_dec_isr_type_t isr_id, jpeg_dec_isr_cb_t cb_isr)
{
	if ((isr_id) >= DEC_ISR_MAX)
		return BK_FAIL;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	if (isr_id == DEC_END_OF_FRAME)
	{
		jpeg_dec_auto_frame_end_int_en(1);
		s_jpeg_dec_isr[DEC_END_OF_FRAME] = cb_isr;
        dec_mode = DEC_END_OF_FRAME;
	}
	else if (isr_id == DEC_EVERY_LINE_INT)
	{
		jpeg_dec_auto_line_num_int_en(1);
		//jpeg_dec_set_line_num(0);   //default by every 8 line decode once 
		s_jpeg_dec_isr[DEC_EVERY_LINE_INT] = cb_isr;
        dec_mode = DEC_EVERY_LINE_INT;
	}
	else
	{
#if CONFIG_SOC_BK7236XX
		jpeg_dec_ll_set_reg0x5e_dec_huf_err_int(1);

		s_jpeg_dec_isr[DEC_ERR] = cb_isr;
#endif
	}

	GLOBAL_INT_RESTORE();
	return BK_OK;
}

#ifdef JPEG_DEC_STRIP
bool jpeg_dec_comp_status(uint8_t *src, uint32_t src_size, uint32_t dec_size)
{
	bool ok = false;
	uint32_t i, tail = src_size - 1, strip = 0, max = 50;

	for (i = tail; i > 0 && max > 0; i--, max--)
	{
		if (src[i] == 0xD9
			&& src[i - 1] == 0xFF)
		{
			tail = i - JPEG_TAIL_SIZE;
			break;
		}

		if (src[i] == 0x00)
		{
			strip++;
		}
	}

	for (i = tail; i > 0 && max > 0; i--, max--)
	{
		if (src[i] == 0xFF)
		{
			strip++;
		}
		else
		{
			break;
		}
	}

	if (max > 0
		&& strip + dec_size == src_size - JPEG_TAIL_SIZE)
	{
		ok = true;
	}
    // Modified by TUYA Start
    else if (max > 0 && src_size - dec_size == 3 && src[src_size - 3] == 0x00)
	{
		ok = true;
	}
    // Modified by TUYA End
	else
	{
		LOGD("decoder_error, %u, %u, %u, %u\n", src_size, dec_size, strip, max);
    }
	return ok;
}
#endif

static void jpeg_decoder_isr(void)
{
    result.pixel_x = image_ppi >> 16;
    result.pixel_y = image_ppi & 0xFFFF;

#if CONFIG_SOC_BK7236XX  //enable huf err int
	if(jpeg_dec_ll_get_reg0x5f_dec_huf_err_int_clr())
	{
        LOGI("%s int status = %x \r\n", __func__, jpeg_dec_hal_get_int_status_value());
        bk_jpeg_dec_stop();
        jpeg_dec_ll_set_reg0x5f_dec_huf_err_int_clr(1);
        jpeg_dec_ll_set_reg0x56_value(0xFFFFFFFF);

        if (result.ok)
        {
    		if (s_jpeg_dec_isr[DEC_ERR])
            {
    			s_jpeg_dec_isr[DEC_ERR](&result);
            }
        }
        result.ok = false;
        return;
	}
#endif

	if (jpeg_dec_ll_get_reg0x5f_dec_frame_int_clr()) 
    {
		if (jpeg_dec_hal_get_jpeg_dec_linen())  //enable line num en
		{
			if (jpeg_dec_hal_get_mcu_index() == 0) //last line
			{
#if CONFIG_SOC_BK7256
				register uint32_t rd_cnt = jpeg_dec_hal_get_master_rd_cnt();
				register uint32_t base_raddr = jpeg_dec_hal_get_base_raddr();
				register uint8_t dri = *((uint8_t *)(base_raddr + rd_cnt));
				if (dri == 0xff)
				{
					dri = *((uint8_t *)(base_raddr + rd_cnt + 1));
					if ((dri == 0xD0) || (dri == 0xD1) || (dri == 0xD2) || (dri == 0xD3) || (dri == 0xD4) || (dri == 0xD5) || (dri == 0xD6) || (dri == 0xD7))
					{
						dri = *(uint8_t *)(base_raddr + rd_cnt + 2);
						if ((dri == 0xD0) || (dri == 0xD1) || (dri == 0xD2) || (dri == 0xD3) || (dri == 0xD4) || (dri == 0xD5) || (dri == 0xD6) || (dri == 0xD7) || (dri == 0x00))
						{
							bk_jpeg_dec_stop();
							return;
						}
					}
				}
#endif

#if (CONFIG_JPEGDEC_HW_SUPPORT_FFD9_CHECK)
//                extern  void bk_delay(int num);
                while(jpeg_dec_ll_get_reg0xd_state_dec_busy())
                {
                    LOGE("%s %d dec_busy %x\n", __func__, __LINE__);
                }
                                
//                bk_delay(1);
                result.size = jpeg_dec_ll_get_reg0x5d_value();
                result.ok = result.size == jpeg_size;

                if(!result.ok)
                {
                    //result.ok = (result.size == jpeg_size - JPEG_TAIL_SIZE);
                    LOGD("decoder error, %u, %u, %x %x %x %x, %x %x %x %x\n", jpeg_size, result.size, jpeg_address[0], jpeg_address[1], jpeg_address[2],jpeg_address[3], jpeg_address[jpeg_size-4], jpeg_address[jpeg_size-3], jpeg_address[jpeg_size-2],jpeg_address[jpeg_size-1]);
                    if(!result.ok)
                        LOGD("double check decoder error, %u, %u, %x %x %x %x, %x %x %x %x\n", jpeg_size, result.size, jpeg_address[0], jpeg_address[1], jpeg_address[2],jpeg_address[3], jpeg_address[jpeg_size-4], jpeg_address[jpeg_size-3], jpeg_address[jpeg_size-2],jpeg_address[jpeg_size-1]);
                }
#else
                result.size = jpeg_dec_ll_get_reg0x5d_value();
                result.ok = jpeg_dec_comp_status(jpeg_address, jpeg_size, result.size);
#endif
                bk_jpeg_dec_stop();
                if (s_jpeg_dec_isr[DEC_EVERY_LINE_INT]) 
                    s_jpeg_dec_isr[DEC_EVERY_LINE_INT](&result);
                
            }
            else
			{
				jpeg_dec_hal_set_dec_frame_int_clr(1); 
#if CONFIG_SOC_BK7256
				register uint32_t rd_cnt = jpeg_dec_hal_get_master_rd_cnt();
				register uint32_t base_raddr = jpeg_dec_hal_get_base_raddr();
				register uint8_t dri = *(uint8_t *)(base_raddr + rd_cnt);
				if (dri == 0xff)
				{
					dri = *((uint8_t *)(base_raddr + rd_cnt + 1));
					if ((dri == 0xD0) || (dri == 0xD1) || (dri == 0xD2) || (dri == 0xD3) || (dri == 0xD4) || (dri == 0xD5) || (dri == 0xD6) || (dri == 0xD7))
					{
						dri = *(uint8_t *)(base_raddr + rd_cnt + 2);
						if ((dri == 0xD0) || (dri == 0xD1) || (dri == 0xD2) || (dri == 0xD3) || (dri == 0xD4) || (dri == 0xD5) || (dri == 0xD6) || (dri == 0xD7) || (dri == 0x00))
						{
							bk_jpeg_dec_stop();
							return;
						}
					}
				}
#endif

                result.ok = true;
				if (s_jpeg_dec_isr[DEC_EVERY_LINE_INT]) 
					s_jpeg_dec_isr[DEC_EVERY_LINE_INT](&result);
				//jpeg_dec_hal_set_dec_cmd(JPEGDEC_START);
				
			}
			//jpeg_dec_hal_set_dec_frame_int_clr(1);
		}
		else
		{

#if (!CONFIG_JPEGDEC_HW_SUPPORT_FFD9_CHECK)
            result.size = jpeg_dec_ll_get_reg0x5d_value();
#ifdef JPEG_DEC_STRIP
    		result.ok = jpeg_dec_comp_status(jpeg_address, jpeg_size, result.size);
#else
    		result.ok = result.size == (jpeg_size - JPEG_TAIL_SIZE);
#endif

            bk_jpeg_dec_stop();
            if (s_jpeg_dec_isr[DEC_END_OF_FRAME]) {
                s_jpeg_dec_isr[DEC_END_OF_FRAME](&result);
            }
#endif
		}
	} 
    else
    {
        LOGE("%s %d %x\n", __func__, __LINE__, jpeg_dec_hal_get_int_status_value());
        jpeg_dec_hal_set_int_status_value(0xFFFF);
    }
}



#else
bk_err_t  bk_jpeg_dec_isr_register(jpeg_dec_isr_t jpeg_dec_isr)
{
	bk_int_isr_register(INT_SRC_JPEG_DEC, jpeg_dec_isr, NULL);
	return BK_OK;
}
#endif



