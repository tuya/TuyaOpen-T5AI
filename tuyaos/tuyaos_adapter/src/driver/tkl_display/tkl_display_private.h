/**
 * @file tkl_display_private.h
 * @version 0.1
 * @date 2025-03-11
 */

#ifndef __TKL_DISPLAY_PRIVATE_H__
#define __TKL_DISPLAY_PRIVATE_H__

#include <os/os.h>
#include <driver/lcd.h>
#include <driver/media_types.h>

#include "tkl_display.h"
#include "tkl_disp_driver.h"
#include "tkl_disp_drv_lcd.h"


#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************
************************macro define************************
***********************************************************/


/***********************************************************
***********************typedef define***********************
***********************************************************/


/***********************************************************
********************function declaration********************
***********************************************************/
lcd_clk_t tkl_disp_convert_lcd_clk_tkl_to_bk(TUYA_LCD_CLK_T tkl_clk);

void tkl_disp_convert_rgb_cfg_tkl_to_bk(const TUYA_LCD_RGB_CFG_T *src_cfg,  lcd_rgb_t *dst_cfg);

OPERATE_RET tkl_disp_convert_pixel_fmt_tkl_to_bk(TKL_DISP_PIXEL_FMT_E src_fmt,  pixel_format_t *p_dst_fmt);

OPERATE_RET tkl_disp_convert_lcd_cfg_tkl_to_bk(TUYA_LCD_CFG_T *src_cfg, lcd_device_t *dst_cfg, lcd_rgb_t *dst_rgb);

uint8_t tkl_disp_convert_pixel_fmt_to_size(TKL_DISP_PIXEL_FMT_E format);


void tkl_display_dma2d_init(void);

void tkl_dispaly_dma2d_framebuffer_transfer(TKL_DISP_FRAMEBUFFER_S *dst_fb, TKL_DISP_FRAMEBUFFER_S *src_fb);


void tkl_display_framebuffer_init(void);

TKL_DISP_FRAMEBUFFER_S *tkl_disp_create_framebuffer(int width, int height, TKL_DISP_PIXEL_FMT_E format);

void tkl_disp_release_framebuffer(TKL_DISP_FRAMEBUFFER_S *buf);


OPERATE_RET tkl_display_rotate_init(void);

TKL_DISP_FRAMEBUFFER_S *tkl_display_create_rotate_frame(TKL_DISP_FRAMEBUFFER_S *frame, TKL_DISP_ROTATION_E rotate);

OPERATE_RET tkl_lcd_rotate_deinit(void);


#ifdef __cplusplus
}
#endif

#endif /* __TKL_DISPLAY_PRIVATE_H__ */
