/**
 * @file tkl_display_rotate.c
 * @version 0.1
 * @date 2025-03-06
 */
#include "tkl_display_private.h"

#include <stdlib.h>
#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/rott_driver.h>

#include "tkl_semaphore.h"
#include "tkl_display.h"
#include "tkl_display_private.h"
/***********************************************************
************************macro define************************
***********************************************************/


/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
	media_rotate_t    rotate_angle;
	TKL_SEM_HANDLE    rot_sem;
	uint8_t           rotate_en;
}rotate_t;


/***********************************************************
***********************variable define**********************
***********************************************************/
static rotate_t  s_rot = {0};

/***********************************************************
***********************function define**********************
***********************************************************/
static media_rotate_t __convert_rotation_tkl_to_bk(TKL_DISP_ROTATION_E tkl_rott)
{
	media_rotate_t bk_rott = ROTATE_NONE;

	switch(tkl_rott) {
		case TKL_DISP_ROTATION_0:
			bk_rott = ROTATE_NONE;
		break;
		case TKL_DISP_ROTATION_90:
			bk_rott = ROTATE_90;
		break;
		case TKL_DISP_ROTATION_180:
			bk_rott = ROTATE_180;
		break;
		case TKL_DISP_ROTATION_270:
			bk_rott = ROTATE_270;
		break;
	}

	return bk_rott;
}

static void __rotate_complete_cb(void)
{
	bk_printf("rotate_complete_cb\r\n");
	tkl_semaphore_post(s_rot.rot_sem);
}

static void __rotate_watermark_cb(void)
{
	bk_printf("rotate_watermark_cb\r\n");
	
}

static void __rotate_cfg_err_cb(void)
{
	bk_printf("rotate_cfg_err_cb\r\n");
}

static void __rott_pfc_complete_cb(void)
{
	bk_printf("rott_pfc_complete_cb\r\n");
	tkl_semaphore_post(s_rot.rot_sem);
}


static OPERATE_RET __tkl_display_hw_rotate_yuv2rgb565(TKL_DISP_FRAMEBUFFER_S *src, void *dst_buffer, TKL_DISP_ROTATION_E rotate)
{
    OPERATE_RET  ret = OPRT_OK;
	bk_err_t      bk_ret = BK_OK;
	rott_config_t rott_cfg = {0};

	if(NULL == src || NULL == dst_buffer) {
		return OPRT_INVALID_PARM;
	}

    memset(&rott_cfg, 0x00, sizeof(rott_config_t));
	
	rott_cfg.input_addr  = src->buffer;
	rott_cfg.output_addr = dst_buffer;
	rott_cfg.rot_mode = __convert_rotation_tkl_to_bk(rotate);

	switch (src->format) {
		case TKL_DISP_PIXEL_FMT_YUYV:
			rott_cfg.input_fmt = PIXEL_FMT_YUYV;
			rott_cfg.input_flow = ROTT_INPUT_NORMAL;
			rott_cfg.output_flow = ROTT_OUTPUT_NORMAL;
			break;
		case TKL_DISP_PIXEL_FMT_VUYY:
			rott_cfg.input_fmt = PIXEL_FMT_VUYY;
			rott_cfg.input_flow = ROTT_INPUT_NORMAL;
			rott_cfg.output_flow = ROTT_OUTPUT_NORMAL;
			break;
		case TKL_DISP_PIXEL_FMT_RGB565_LE:
			rott_cfg.input_fmt = PIXEL_FMT_RGB565_LE;
			rott_cfg.input_flow = ROTT_INPUT_REVESE_HALFWORD_BY_HALFWORD;
			rott_cfg.output_flow = ROTT_OUTPUT_NORMAL;
			break;
		case TKL_DISP_PIXEL_FMT_RGB565:
		default:
			rott_cfg.input_fmt = PIXEL_FMT_RGB565;
			rott_cfg.input_flow = ROTT_INPUT_REVESE_HALFWORD_BY_HALFWORD;
			rott_cfg.output_flow = ROTT_OUTPUT_NORMAL;
			break;
	}
	rott_cfg.picture_xpixel = src->rect.width;
	rott_cfg.picture_ypixel = src->rect.height;

	bk_ret = rott_config(&rott_cfg);
	if (bk_ret != BK_OK) {
		bk_printf(" rott_config ERR\n");
    }
	bk_rott_enable();

	ret = tkl_semaphore_wait(s_rot.rot_sem, TKL_SEM_WAIT_FOREVER);
	if (ret != BK_OK) {
		bk_printf("%s semaphore get failed: %d\n", __func__, ret);
	}

	return ret;
}

OPERATE_RET tkl_display_rotate_init(void)
{
    OPERATE_RET ret = OPRT_OK;
    static bool is_init = false;

    if(true == is_init) {
        return OPRT_OK;
    }

	ret = tkl_semaphore_create_init(&s_rot.rot_sem, 0, 1);
	if (ret != OPRT_OK) {
		bk_printf("%s rot_sem init failed: %d\n", __func__, ret);
		return ret;
	}

    bk_rott_driver_init();
    bk_rott_int_enable(ROTATE_COMPLETE_INT | ROTATE_CFG_ERR_INT | ROTATE_WARTERMARK_INT, 1);
    bk_rott_isr_register(ROTATE_COMPLETE_INT, __rotate_complete_cb);
    bk_rott_isr_register(ROTATE_WARTERMARK_INT, __rotate_watermark_cb);
    bk_rott_isr_register(ROTATE_CFG_ERR_INT, __rotate_cfg_err_cb);

	is_init = true;
	
	return ret;
}

void __tkl_disp_get_rott_rect(TKL_DISP_RECT_S *src_rect, TKL_DISP_RECT_S *dst_rect, TKL_DISP_ROTATION_E rotate)
{
	if(NULL == src_rect || NULL == dst_rect) {
		return;
	}

	switch(rotate) {
		case TKL_DISP_ROTATION_0:
			memcpy(dst_rect, src_rect, sizeof(TKL_DISP_RECT_S));
		break;
		case TKL_DISP_ROTATION_90:
			dst_rect->width  = src_rect->height;
			dst_rect->height = src_rect->width;
			dst_rect->x      = src_rect->x;
			dst_rect->y      = src_rect->y;
		break;
		case TKL_DISP_ROTATION_180:
			dst_rect->width  = src_rect->width;
			dst_rect->height = src_rect->height;
			dst_rect->x      = src_rect->x;
			dst_rect->y      = src_rect->y;
		break;
		case TKL_DISP_ROTATION_270:
			dst_rect->width  = src_rect->height;
			dst_rect->height = src_rect->width;
			dst_rect->x      = src_rect->x;
			dst_rect->y      = src_rect->y;
		break;
	}

	return;
}

TKL_DISP_FRAMEBUFFER_S *tkl_display_create_rotate_frame(TKL_DISP_FRAMEBUFFER_S *frame, TKL_DISP_ROTATION_E rotate)
{
	OPERATE_RET ret = OPRT_OK;
	TKL_DISP_FRAMEBUFFER_S *rott_frame = NULL;
	TKL_DISP_RECT_S rott_rect;
	uint8_t pixel_size = 0;

	if(NULL == frame) {
		return NULL;
	}

	pixel_size = tkl_disp_convert_pixel_fmt_to_size(frame->format);
	if((pixel_size != 2) && (pixel_size != 4)) {
		bk_printf("not support format:%d \r\n", frame->format);
		return NULL;
	}

	__tkl_disp_get_rott_rect(&frame->rect, &rott_rect, rotate);
	
	rott_frame = tkl_disp_create_framebuffer(rott_rect.width, rott_rect.height, frame->format);
	if(NULL == rott_frame) {
		return NULL;
	}

	memcpy(&rott_frame->rect, &rott_rect, sizeof(TKL_DISP_RECT_S));

	if(2 == pixel_size) {
		ret = __tkl_display_hw_rotate_yuv2rgb565(frame, rott_frame->buffer, rotate);
		if(ret != OPRT_OK) {
			tkl_disp_release_framebuffer(rott_frame);
			rott_frame = NULL;
			return NULL;
		}
		rott_frame->format = TKL_DISP_PIXEL_FMT_RGB565_LE;
	}else {
		//todo
		#if 0
		bk_ret = argb8888_rotate_degree270(frame->buffer, rott_frame->buffer, frame->rect.width, frame->rect.height);
		if(bk_ret != BK_OK) {
			tkl_disp_release_framebuffer(rott_frame);
			rott_frame = NULL;
			return NULL;	
		}
		#endif
	}

	bk_printf("rott rect.x:%d rect.y:%d rect.width:%d rect.height:%d \r\n", rott_frame->rect.x, rott_frame->rect.y, rott_frame->rect.width, rott_frame->rect.height);

	bk_printf("%s<-\r\n", __func__);

	return rott_frame;
}



OPERATE_RET tkl_lcd_rotate_deinit(void)
{
	OPERATE_RET ret = OPRT_OK;

	bk_rott_driver_deinit();

	ret = tkl_semaphore_release(s_rot.rot_sem);
	if (ret != OPRT_OK) {
		bk_printf("%s rot_sem deinit failed: %d\n", __func__, ret);
		return ret;
	}

    s_rot.rot_sem = NULL;

	return ret;
}