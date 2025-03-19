/**
 * @file tkl_display_dma2d.c
 * @version 0.1
 * @date 2025-03-11
 */
#include "tkl_display_private.h"
#include "dma2d_hal.h"
#include <driver/dma2d_types.h>
#include <driver/dma2d.h>

#include "tkl_semaphore.h"

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
static TKL_SEM_HANDLE       sg_dma2d_sem_hdl;

/***********************************************************
***********************function define**********************
***********************************************************/
static input_color_mode_t __get_dma2d_input_color_mode(TKL_DISP_PIXEL_FMT_E format)
{
    input_color_mode_t input_mode = 0;

    switch(format) {

        case TKL_DISP_PIXEL_FMT_RGBX:
        case TKL_DISP_PIXEL_FMT_ABGR:
        case TKL_DISP_PIXEL_FMT_RGBA:
        case TKL_DISP_PIXEL_FMT_ARGB:
            input_mode = DMA2D_INPUT_ARGB4444;
            break;
        case TKL_DISP_PIXEL_FMT_RGB666:
        case TKL_DISP_PIXEL_FMT_RGB888: 
            input_mode = DMA2D_INPUT_RGB888;
            break;
        case TKL_DISP_PIXEL_FMT_RGB565:    
        case TKL_DISP_PIXEL_FMT_RGB565_LE: 
            input_mode = DMA2D_INPUT_RGB565;
            break;
        case TKL_DISP_PIXEL_FMT_YUYV:
            input_mode = DMA2D_INPUT_YUYV;
            break;
        case TKL_DISP_PIXEL_FMT_VUYY:
            input_mode = DMA2D_INPUT_VUYY;
            break;        
        default:
            bk_printf("not support get format:%d bpp\r\n", format);
            break;
    }

    return input_mode;
}

static out_color_mode_t __get_dma2d_out_color_mode(TKL_DISP_PIXEL_FMT_E format)
{
    out_color_mode_t out_mode = 0;

    switch(format) {

        case TKL_DISP_PIXEL_FMT_RGBX:
        case TKL_DISP_PIXEL_FMT_ABGR:
        case TKL_DISP_PIXEL_FMT_RGBA:
        case TKL_DISP_PIXEL_FMT_ARGB:
            out_mode = DMA2D_OUTPUT_ARGB4444;
            break;
        case TKL_DISP_PIXEL_FMT_RGB666:
        case TKL_DISP_PIXEL_FMT_RGB888: 
            out_mode = DMA2D_OUTPUT_RGB888;
            break;
        case TKL_DISP_PIXEL_FMT_RGB565:    
        case TKL_DISP_PIXEL_FMT_RGB565_LE: 
            out_mode = DMA2D_OUTPUT_RGB565;
            break;
        case TKL_DISP_PIXEL_FMT_YUYV:
            out_mode = DMA2D_OUTPUT_YUYV;
            break;
        case TKL_DISP_PIXEL_FMT_VUYY:
            out_mode = DMA2D_OUTPUT_VUYY;
            break;        
        default:
            bk_printf("not support get format:%d bpp\r\n", format);
            break;
    }

    return out_mode;
}

static void __dma2d_config_error(void)
{
    bk_printf("%s \n", __func__);
}

static void __dma2d_transfer_error(void)
{
    bk_printf("%s \n", __func__);
}

static void __dma2d_transfer_complete(void)
{
    tkl_semaphore_post(sg_dma2d_sem_hdl);
}

void tkl_display_dma2d_init(void)
{
    static bool is_init = false;

    if(true == is_init) {
        return;
    }

    tkl_semaphore_create_init(&sg_dma2d_sem_hdl, 0, 1);

    bk_dma2d_driver_init();
    bk_dma2d_register_int_callback_isr(DMA2D_CFG_ERROR_ISR, __dma2d_config_error);
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_ERROR_ISR, __dma2d_transfer_error);
    bk_dma2d_register_int_callback_isr(DMA2D_TRANS_COMPLETE_ISR, __dma2d_transfer_complete);
    bk_dma2d_int_enable(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE, 1);

    is_init = true;
}

void tkl_dispaly_dma2d_framebuffer_transfer(TKL_DISP_FRAMEBUFFER_S *dst_fb, TKL_DISP_FRAMEBUFFER_S *src_fb)
{
    bk_err_t ret = BK_OK;
    dma2d_memcpy_pfc_t dma2d_memcpy_pfc = {0};
    uint8_t src_pixel_size = 0, dst_pixel_size = 0;
    input_color_mode_t in_color_mode;
    out_color_mode_t out_color_mode;

    if(NULL == src_fb || NULL == dst_fb) {
        return;
    }

    memset(&dma2d_memcpy_pfc, 0x00, sizeof(dma2d_memcpy_pfc_t));

    src_pixel_size = tkl_disp_convert_pixel_fmt_to_size(src_fb->format);
    dst_pixel_size = tkl_disp_convert_pixel_fmt_to_size(dst_fb->format);
    in_color_mode  = __get_dma2d_input_color_mode(src_fb->format);
    out_color_mode = __get_dma2d_out_color_mode(dst_fb->format);

    if(src_fb->format == dst_fb->format) {
        dma2d_memcpy_pfc.mode = DMA2D_M2M;
    }else {
        dma2d_memcpy_pfc.mode = DMA2D_M2M_PFC;
    }
    dma2d_memcpy_pfc.input_color_mode  = in_color_mode;
    dma2d_memcpy_pfc.src_pixel_byte    = src_pixel_size;
    dma2d_memcpy_pfc.output_color_mode = out_color_mode;
    dma2d_memcpy_pfc.dst_pixel_byte    = dst_pixel_size;

    dma2d_memcpy_pfc.input_addr  = (char *)src_fb->buffer;
    dma2d_memcpy_pfc.output_addr = (char *)dst_fb->buffer;

    dma2d_memcpy_pfc.dma2d_width      = src_fb->rect.width;
    dma2d_memcpy_pfc.dma2d_height     = src_fb->rect.height;
    dma2d_memcpy_pfc.src_frame_width  = src_fb->rect.width;
    dma2d_memcpy_pfc.src_frame_height = src_fb->rect.height;
    dma2d_memcpy_pfc.dst_frame_width  = dst_fb->rect.width;
    dma2d_memcpy_pfc.dst_frame_height = dst_fb->rect.height;

    dma2d_memcpy_pfc.src_frame_xpos = 0;
    dma2d_memcpy_pfc.src_frame_ypos = 0;
    dma2d_memcpy_pfc.dst_frame_xpos = src_fb->rect.x;
    dma2d_memcpy_pfc.dst_frame_ypos = src_fb->rect.y;

    bk_dma2d_memcpy_or_pixel_convert(&dma2d_memcpy_pfc);
    bk_dma2d_start_transfer();

    ret = tkl_semaphore_wait(sg_dma2d_sem_hdl, 1000);
    if (ret != kNoErr) {
        bk_printf("%s get fail! ret = %d\r\n", __func__, ret);
    }

    return;
}