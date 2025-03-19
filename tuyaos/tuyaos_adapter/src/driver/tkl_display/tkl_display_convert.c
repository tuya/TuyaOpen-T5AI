/**
 * @file tkl_display_convert.c
 * @version 0.1
 * @date 2025-03-11
 */


#include "tkl_display_private.h"

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
lcd_clk_t tkl_disp_convert_lcd_clk_tkl_to_bk(TUYA_LCD_CLK_T tkl_clk)
{
    lcd_clk_t lcd_clk = LCD_30M;

    switch (tkl_clk) {
        case TUYA_LCD_80M:
            lcd_clk = LCD_80M;
            break;
        case TUYA_LCD_64M:
            lcd_clk = LCD_64M;
            break;
        case TUYA_LCD_60M:
            lcd_clk = LCD_60M;
            break;
        case TUYA_LCD_54M:
            lcd_clk = LCD_54M;
            break;
        case TUYA_LCD_45M: //45.7M
            lcd_clk = LCD_45M;
            break;
        case TUYA_LCD_40M:
            lcd_clk = LCD_40M;
            break;
        case TUYA_LCD_35M: //35.5
            lcd_clk = LCD_35M;
            break;
        case TUYA_LCD_32M:
            lcd_clk = LCD_32M;
            break;
        case TUYA_LCD_30M:
            lcd_clk = LCD_30M;
            break;
        case TUYA_LCD_26M: //26.6M
            lcd_clk = LCD_26M;
            break;
        case TUYA_LCD_24M: //24.6M
            lcd_clk = LCD_24M;
            break;
        case TUYA_LCD_22M: //22.85M
            lcd_clk = LCD_22M;
            break;
        case TUYA_LCD_20M:
            lcd_clk = LCD_20M;
            break;
        case TUYA_LCD_17M: //17.1M
            lcd_clk = LCD_17M;
            break;
        case TUYA_LCD_15M:
            lcd_clk = LCD_15M;
            break;
        case TUYA_LCD_12M:
            lcd_clk = LCD_12M;
            break;
        case TUYA_LCD_10M:
            lcd_clk = LCD_10M;
            break;
        case TUYA_LCD_9M:  //9.2M
            lcd_clk = LCD_9M;
            break;
        case TUYA_LCD_8M:
            lcd_clk = LCD_8M;
            break;
        case TUYA_LCD_7M:
            lcd_clk = LCD_7M;
            break;
        default:
            bk_printf("rgb clk not support %d\r\n", tkl_clk);
            break;
    }

    return lcd_clk;
}

void tkl_disp_convert_rgb_cfg_tkl_to_bk(const TUYA_LCD_RGB_CFG_T *src_cfg,  lcd_rgb_t *dst_cfg)
{
    if(NULL == src_cfg || NULL == dst_cfg) {
        bk_printf("param is null \r\n");
        return;
    }

    dst_cfg->clk = tkl_disp_convert_lcd_clk_tkl_to_bk(src_cfg->clk);

    if (src_cfg->active_edge == TUYA_RGB_NEGATIVE_EDGE) {
        dst_cfg->data_out_clk_edge = NEGEDGE_OUTPUT;
    } else {
        dst_cfg->data_out_clk_edge = POSEDGE_OUTPUT;
    }

    dst_cfg->hsync_pulse_width = src_cfg->hsync_pulse_width;
    dst_cfg->vsync_pulse_width = src_cfg->vsync_pulse_width;
    dst_cfg->hsync_back_porch  = src_cfg->hsync_back_porch;
    dst_cfg->hsync_front_porch = src_cfg->hsync_front_porch;
    dst_cfg->vsync_back_porch  = src_cfg->vsync_back_porch;
    dst_cfg->vsync_front_porch = src_cfg->vsync_front_porch;

    return;
}

OPERATE_RET tkl_disp_convert_pixel_fmt_tkl_to_bk(TKL_DISP_PIXEL_FMT_E src_fmt,  pixel_format_t *p_dst_fmt)
{
    pixel_format_t fmt = 0;

    if(NULL == p_dst_fmt) {
        return OPRT_INVALID_PARM;
    }

    if (src_fmt == TKL_DISP_PIXEL_FMT_RGB888) {
        fmt = PIXEL_FMT_RGB888;
    } else if (src_fmt == TKL_DISP_PIXEL_FMT_RGB666) {
        fmt = PIXEL_FMT_RGB666;
    }else if (src_fmt == TKL_DISP_PIXEL_FMT_RGB565) {
        fmt = PIXEL_FMT_RGB565;
    }else if (src_fmt == TKL_DISP_PIXEL_FMT_RGB565_LE) {
        fmt = PIXEL_FMT_RGB565_LE;
    } else {
        bk_printf("fmt %d not support now\r\n", src_fmt);
        return OPRT_NOT_SUPPORTED;
    }

    *p_dst_fmt = fmt;

    return OPRT_OK;
}

OPERATE_RET tkl_disp_convert_lcd_cfg_tkl_to_bk(TUYA_LCD_CFG_T *src_cfg, lcd_device_t *dst_cfg, lcd_rgb_t *dst_rgb)
{
    OPERATE_RET ret = OPRT_OK;

    if(NULL == src_cfg || NULL == dst_cfg) {
        return OPRT_INVALID_PARM;
    }

    dst_cfg->id   = LCD_DEVICE_UNKNOW;
    dst_cfg->name = NULL;
    dst_cfg->ppi  = ((src_cfg->width<<16) | (src_cfg->height&0xFFFF));

    if(TUYA_LCD_TYPE_RGB == src_cfg->lcd_tp) {
        dst_cfg->type = LCD_TYPE_RGB565;
        tkl_disp_convert_rgb_cfg_tkl_to_bk(src_cfg->p_rgb, dst_rgb);
        dst_cfg->rgb = dst_rgb;
    }else {
        return OPRT_NOT_SUPPORTED;
    }

    dst_cfg->src_fmt = PIXEL_FMT_RGB565;

    ret = tkl_disp_convert_pixel_fmt_tkl_to_bk(src_cfg->fmt, &dst_cfg->out_fmt);
    if(ret != OPRT_OK) {
        return ret;
    }

    dst_cfg->ldo_pin = 0xFF;

    return OPRT_OK;
}

uint8_t tkl_disp_convert_pixel_fmt_to_size(TKL_DISP_PIXEL_FMT_E format)
{
    uint8_t pixel_size = 2;

    switch(format) {
        case TKL_DISP_PIXEL_FMT_ABGR:
        case TKL_DISP_PIXEL_FMT_RGBX:
        case TKL_DISP_PIXEL_FMT_RGBA:
        case TKL_DISP_PIXEL_FMT_ARGB:
            pixel_size = 4;
            break;
        case TKL_DISP_PIXEL_FMT_RGB565:    
        case TKL_DISP_PIXEL_FMT_RGB565_LE: 
        case TKL_DISP_PIXEL_FMT_YUYV:
        case TKL_DISP_PIXEL_FMT_VUYY:
            pixel_size = 2;
            break;
        case TKL_DISP_PIXEL_FMT_RGB888:
        case TKL_DISP_PIXEL_FMT_RGB666: 
            pixel_size = 3;
            break;           
        default:
            bk_printf("not support get format:%d fmt size\r\n", format);
            break;
    }

    return pixel_size;
}