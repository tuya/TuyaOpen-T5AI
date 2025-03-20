/**
 * @file tkl_display_framebuffer.c
 * @version 0.1
 * @date 2025-03-11
 */

#include "tkl_display_private.h"

#include "frame_buffer.h"

#include "tkl_memory.h"
/***********************************************************
************************macro define************************
***********************************************************/


/***********************************************************
***********************typedef define***********************
***********************************************************/


/***********************************************************
********************function declaration********************
***********************************************************/


/***********************************************************
***********************variable define**********************
***********************************************************/


/***********************************************************
***********************function define**********************
***********************************************************/
void tkl_display_framebuffer_init(void)
{
    static bool is_init = false;

    if(true == is_init) {
        return;
    }

    frame_buffer_fb_init(FB_INDEX_DISPLAY);
    frame_buffer_fb_register(MODULE_LCD, FB_INDEX_DISPLAY); 

    is_init = true;
}

TKL_DISP_FRAMEBUFFER_S *tkl_disp_create_framebuffer(int width, int height, TKL_DISP_PIXEL_FMT_E format)
{
    TKL_DISP_FRAMEBUFFER_S *fb = NULL;
    uint8_t pixel_size = 0;

    fb = (TKL_DISP_FRAMEBUFFER_S *)tkl_system_psram_malloc(sizeof(TKL_DISP_FRAMEBUFFER_S));
    if(NULL == fb) {
        return NULL;
    }
    memset(fb, 0x00, sizeof(TKL_DISP_FRAMEBUFFER_S));

    fb->rect.width  = width;
    fb->rect.height = height;
    fb->format      = format;

    pixel_size = tkl_disp_convert_pixel_fmt_to_size(format);

    frame_buffer_t *frame = frame_buffer_fb_display_malloc_wait(width*height*pixel_size);
    if (frame == NULL) {
		bk_printf("frame_buffer_fb_malloc failed\n");
        tkl_system_psram_free(fb);
		return NULL;
	}

    frame->fmt = PIXEL_FMT_RGB565;
    fb->buffer = frame->frame;
    fb->param  = (void*)frame;

    return fb;
}

void tkl_disp_release_framebuffer(TKL_DISP_FRAMEBUFFER_S *buf)
{
    if(NULL == buf) {
        return;
    }

    if(buf->param) {
        frame_buffer_fb_direct_free((frame_buffer_t *)buf->param);
    }

    tkl_system_psram_free(buf);

    return;
}