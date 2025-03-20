/**
 * @file tkl_disp_drv_lcd.c
 * @version 0.1
 * @date 2025-02-25
 */
#include "tkl_display_private.h"
#include <components/system.h>
#include "lcd_disp_hal.h"

#include "tkl_semaphore.h"
#include "tkl_memory.h"
#include "tkl_system.h"
#include "tkl_gpio.h"
#include "tkl_pwm.h"
#include "tkl_spi.h"
/***********************************************************
************************macro define************************
***********************************************************/


/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    TUYA_LCD_CFG_T     cfg;
    lcd_rgb_t          bk_rgb_cfg;
    lcd_device_t       bk_lcd_dev;
}TKL_LCD_INFO_T;

/***********************************************************
***********************variable define**********************
***********************************************************/

/***********************************************************
***********************function define**********************
***********************************************************/
#if CONFIG_LV_ATTRIBUTE_FAST_MEM
static void lcd_driver_display_rgb_isr(void)
#else
static __attribute__((section(".itcm_sec_code"))) void tkl_disp_driver_lcd_isr(void)
#endif
{
    tkl_disp_driver_display_frame_complete();
}

static void __lcd_blacklight_init(TUYA_LCD_BL_CFG_T *bl_cfg)
{
    TUYA_GPIO_BASE_CFG_T cfg;

    if(NULL == bl_cfg) {
        return;
    }

    if(bl_cfg->mode == TUYA_DISP_BL_GPIO) {
        cfg.mode   = TUYA_GPIO_PUSH_PULL;
        cfg.direct = TUYA_GPIO_OUTPUT;
        cfg.level  = (bl_cfg->gpio.active_lv == TUYA_GPIO_LEVEL_LOW)? TUYA_GPIO_LEVEL_HIGH: TUYA_GPIO_LEVEL_LOW;
        tkl_gpio_init(bl_cfg->gpio.io, &cfg);
    }else if(bl_cfg->mode == TUYA_DISP_BL_PWM) {
        tkl_pwm_init(bl_cfg->pwm.id, &bl_cfg->pwm.cfg);
    }else {
        bk_printf("not support bl mode:%d\r\n", bl_cfg->mode);
    }

    return;
}

static OPERATE_RET __tkl_disp_drv_lcd_init(TKL_DISP_DRV_HANDLE handle)
{
    TKL_LCD_INFO_T *p_lcd_info = NULL;
    bk_err_t bk_ret = BK_OK;

    if(NULL == handle) {
        return OPRT_INVALID_PARM;
    }

    p_lcd_info = (TKL_LCD_INFO_T *)handle;

    bk_ret = lcd_driver_init(&p_lcd_info->bk_lcd_dev);
    if(bk_ret != BK_OK) {
        return OPRT_COM_ERROR;
    }

    bk_lcd_isr_register(RGB_OUTPUT_EOF, tkl_disp_driver_lcd_isr);

    __lcd_blacklight_init(&p_lcd_info->cfg.bl);

    return OPRT_OK;
}

static OPERATE_RET __tkl_disp_drv_lcd_display_frame(TKL_DISP_DRV_HANDLE handle, TKL_DISP_FRAMEBUFFER_S *fb)
{
    OPERATE_RET ret = OPRT_OK;
    pixel_format_t bk_fmt = PIXEL_FMT_RGB565;

    if(NULL == handle || NULL == fb) {
        return OPRT_INVALID_PARM;
    }

    ret = tkl_disp_convert_pixel_fmt_tkl_to_bk(fb->format, &bk_fmt);
    if(ret != OPRT_OK) {
        bk_printf("tkl_disp_convert_pixel_fmt_tkl_to_bk err:%d\r\n", ret);
        return ret;
    }

    lcd_driver_display_disable();

    bk_lcd_pixel_config(fb->rect.width, fb->rect.height);

    bk_lcd_set_yuv_mode(bk_fmt);

    lcd_driver_set_display_base_addr((uint32_t)fb->buffer);

    lcd_driver_display_enable();

    return OPRT_OK;
}

static OPERATE_RET __tkl_disp_set_brightness(TKL_DISP_DRV_HANDLE handle, int brightness)
{
    TKL_LCD_INFO_T *p_lcd_info = NULL;
    TUYA_LCD_BL_CFG_T *bl_cfg = NULL;

    if(NULL == handle) {
        return OPRT_INVALID_PARM;
    }

    p_lcd_info = (TKL_LCD_INFO_T *)handle;
    bl_cfg = &p_lcd_info->cfg.bl;
 
    if(bl_cfg->mode == TUYA_DISP_BL_GPIO) {
        if(brightness) {
            tkl_gpio_write(bl_cfg->gpio.io, bl_cfg->gpio.active_lv);
        }else {
            tkl_gpio_write(bl_cfg->gpio.io, (bl_cfg->gpio.active_lv == TUYA_GPIO_LEVEL_HIGH) ? TUYA_GPIO_LEVEL_LOW: TUYA_GPIO_LEVEL_HIGH);
        }
    }else if(bl_cfg->mode == TUYA_DISP_BL_PWM) {
        if(brightness) {
            bl_cfg->pwm.cfg.duty = brightness*100;
            tkl_pwm_info_set(bl_cfg->pwm.id, &bl_cfg->pwm.cfg);
            tkl_pwm_start(bl_cfg->pwm.id);
        }else {
            tkl_pwm_stop(bl_cfg->pwm.id);
        }
    }else {
        return OPRT_NOT_SUPPORTED;
    }

    return OPRT_OK;
}

/************************************lcd spi ********************************/
static void __tkl_disp_drv_lcd_spi_irq_cb(TUYA_SPI_NUM_E port, TUYA_SPI_IRQ_EVT_E event)
{
    tkl_spi_irq_disable(port);

    tkl_disp_driver_display_frame_complete();
}

static OPERATE_RET __tkl_disp_drv_lcd_gpio_init(TUYA_LCD_SPI_CFG_T *p_cfg)
{
    TUYA_GPIO_BASE_CFG_T pin_cfg;
    OPERATE_RET ret = OPRT_OK;

    if(NULL == p_cfg) {
        return OPRT_INVALID_PARM;
    }

    pin_cfg.mode = TUYA_GPIO_PUSH_PULL;
    pin_cfg.direct = TUYA_GPIO_OUTPUT;
    pin_cfg.level = TUYA_GPIO_LEVEL_LOW;

    ret = tkl_gpio_init(p_cfg->cs_pin, &pin_cfg);
    if(ret != OPRT_OK) {
        return ret;
    }

    ret = tkl_gpio_init(p_cfg->dc_pin, &pin_cfg);
    if(ret != OPRT_OK) {
        return ret;
    }

    ret = tkl_gpio_init(p_cfg->rst_pin, &pin_cfg);
    if(ret != OPRT_OK) {
        return ret;
    }

    return ret;
}

static OPERATE_RET __tkl_disp_drv_lcd_spi_init(TUYA_SPI_NUM_E port, uint32_t spi_clk)
{
    OPERATE_RET ret = OPRT_OK;

    /*spi init*/
    TUYA_SPI_BASE_CFG_T spi_cfg = {.mode = TUYA_SPI_MODE0,
                                   .freq_hz = spi_clk,
                                   .databits = TUYA_SPI_DATA_BIT8,
                                   .bitorder = TUYA_SPI_ORDER_MSB2LSB,
                                   .role = TUYA_SPI_ROLE_MASTER,
                                   .type = TUYA_SPI_AUTO_TYPE,
                                   .spi_dma_flags = 1};

    bk_printf("spi init %d\r\n", spi_cfg.freq_hz);
    ret = tkl_spi_init(port, &spi_cfg);
    if(ret != OPRT_OK) {
        return ret;
    }

    tkl_spi_irq_init(port, __tkl_disp_drv_lcd_spi_irq_cb);

    return ret;
}

static void __tkl_disp_drv_spi_lcd_reset(TUYA_GPIO_NUM_E rst_pin)
{
    tkl_gpio_write(rst_pin, TUYA_GPIO_LEVEL_HIGH);
    tkl_system_sleep(100);

    tkl_gpio_write(rst_pin, TUYA_GPIO_LEVEL_LOW);
    tkl_system_sleep(100);

    tkl_gpio_write(rst_pin, TUYA_GPIO_LEVEL_HIGH);
    tkl_system_sleep(100);
}

static void __tkl_disp_drv_spi_lcd_write_cmd(TUYA_LCD_SPI_CFG_T *p_cfg, uint8_t *cmd, uint8_t data_count)
{
    if(NULL == p_cfg || NULL == cmd ) {
        bk_printf("param is null \r\n");
        return;
    }

    tkl_gpio_write(p_cfg->cs_pin, TUYA_GPIO_LEVEL_LOW);
    tkl_gpio_write(p_cfg->dc_pin, TUYA_GPIO_LEVEL_LOW);

    // send cmd
    tkl_spi_send(p_cfg->spi_port, cmd, 1);

    // send lcd_data
    if (data_count > 0) {
        tkl_gpio_write(p_cfg->dc_pin, TUYA_GPIO_LEVEL_HIGH);

        tkl_spi_send(p_cfg->spi_port, cmd + 1, data_count);
    }

    tkl_gpio_write(p_cfg->cs_pin, TUYA_GPIO_LEVEL_HIGH);
}

static OPERATE_RET __tkl_disp_drv_spi_lcd_write_ram(TUYA_LCD_SPI_CFG_T *p_cfg, uint8_t *data, uint32_t len)
{
    if(NULL == p_cfg || NULL == data ) {
        bk_printf("param is null \r\n");
        return OPRT_INVALID_PARM;
    }

    __tkl_disp_drv_spi_lcd_write_cmd(p_cfg, &p_cfg->cmd_ramwr, 0);

    tkl_gpio_write(p_cfg->cs_pin, TUYA_GPIO_LEVEL_LOW);
    tkl_gpio_write(p_cfg->dc_pin, TUYA_GPIO_LEVEL_HIGH);

    tkl_spi_send(p_cfg->spi_port , data, len);

    tkl_gpio_write(p_cfg->cs_pin, TUYA_GPIO_LEVEL_HIGH);

    return OPRT_OK;
}


static void __tkl_disp_drv_spi_lcd_set_window(TUYA_LCD_SPI_CFG_T *p_cfg, TKL_DISP_RECT_S *p_rect)
{
    uint8_t lcd_data[5];
    uint16_t pos_end = 0;

    if(NULL == p_cfg || NULL == p_rect ) {
        bk_printf("param is null \r\n");
        return;
    }

    pos_end = (p_rect->x + p_rect->width-1);
    lcd_data[0] = p_cfg->cmd_caset;
    lcd_data[1] = p_rect->x >> 8;
    lcd_data[2] = p_rect->x;
    lcd_data[3] = pos_end >> 8;
    lcd_data[4] = pos_end;
    __tkl_disp_drv_spi_lcd_write_cmd(p_cfg, lcd_data, 4);

    pos_end = (p_rect->y + p_rect->height-1);
    lcd_data[0] = p_cfg->cmd_raset;
    lcd_data[1] = p_rect->y >> 8;
    lcd_data[2] = p_rect->y;
    lcd_data[3] = pos_end >> 8;
    lcd_data[4] = pos_end;
    __tkl_disp_drv_spi_lcd_write_cmd(p_cfg, lcd_data, 4);
}

static OPERATE_RET __tkl_disp_drv_spi_lcd_init(TKL_DISP_DRV_HANDLE handle)
{
    TKL_LCD_INFO_T *p_lcd_info = NULL;
    TUYA_LCD_SPI_CFG_T *p_cfg;
    OPERATE_RET ret = OPRT_OK;

    if(NULL == handle) {
        return OPRT_INVALID_PARM;
    }

    p_lcd_info = (TKL_LCD_INFO_T *)handle;
    p_cfg =(TUYA_LCD_SPI_CFG_T *)p_lcd_info->cfg.p_spi;

    ret = __tkl_disp_drv_lcd_spi_init(p_cfg->spi_port, p_cfg->spi_clk);
    if(ret != OPRT_OK) {
        bk_printf("lcd spi init err:%d\r\n", ret);
        return ret;
    } 

    ret = __tkl_disp_drv_lcd_gpio_init(p_cfg);
    if(ret != OPRT_OK) {
        bk_printf("lcd gpio init err:%d\r\n", ret);
        return ret;
    }

    __lcd_blacklight_init(&p_lcd_info->cfg.bl);

    __tkl_disp_drv_spi_lcd_reset(p_cfg->rst_pin);

    const uint8_t *cmd = p_cfg->init_seq;
    while (*cmd) {
        __tkl_disp_drv_spi_lcd_write_cmd(p_cfg, (uint8_t *)(cmd + 2), *cmd - 1);
        tkl_system_sleep(*(cmd + 1));
        cmd += *cmd + 2;
    }

    return OPRT_OK;
}

static OPERATE_RET __tkl_disp_drv_spi_lcd_display_frame(TKL_DISP_DRV_HANDLE handle, TKL_DISP_FRAMEBUFFER_S *fb)
{
    TKL_LCD_INFO_T *p_lcd_info = NULL;
    OPERATE_RET ret = OPRT_OK;
    TUYA_LCD_SPI_CFG_T *p_cfg = NULL;
    uint32_t len = 0;

    if(NULL == handle) {
        return OPRT_INVALID_PARM;
    }

    p_lcd_info = (TKL_LCD_INFO_T *)handle;
    p_cfg = (TUYA_LCD_SPI_CFG_T *)p_lcd_info->cfg.p_spi;

    __tkl_disp_drv_spi_lcd_set_window(p_cfg, &fb->rect);

    tkl_spi_irq_enable(p_cfg->spi_port);

    len = fb->rect.width * fb->rect.height * tkl_disp_convert_pixel_fmt_to_size(fb->format);
    __tkl_disp_drv_spi_lcd_write_ram(p_cfg, (uint8_t*)fb->buffer, len);

    return ret;
}

OPERATE_RET tkl_disp_register_lcd_dev(TUYA_LCD_CFG_T *device)
{
    TKL_LCD_INFO_T *p_lcd_info = NULL;
    TKL_DISP_DRV_INTFS_T intfs;
    TKL_DISP_INFO_S      disp_info;

    OPERATE_RET ret = OPRT_OK;

    if(NULL == device) {
        return OPRT_INVALID_PARM;
    }

    memset(&intfs, 0x00, sizeof(TKL_DISP_DRV_INTFS_T));
    memset(&disp_info, 0x00, sizeof(TKL_DISP_INFO_S));

    p_lcd_info = (TKL_LCD_INFO_T *)tkl_system_psram_malloc(sizeof(TKL_LCD_INFO_T));
    if(NULL == p_lcd_info) {
        return OPRT_MALLOC_FAILED;
    }
    memset(p_lcd_info, 0x00, sizeof(TKL_LCD_INFO_T));
    memcpy(&p_lcd_info->cfg, device, sizeof(TUYA_LCD_CFG_T));

    if(device->lcd_tp == TUYA_LCD_TYPE_SPI) {
        intfs.TKL_DISP_DRV_INIT           = __tkl_disp_drv_spi_lcd_init;
        intfs.TKL_DISP_DRV_DISPLAY_FRAME  = __tkl_disp_drv_spi_lcd_display_frame;
        intfs.TKL_DISP_DRV_CONFIG         = NULL;
        intfs.TKL_DISP_DRV_DEINIT         = NULL;
    }else {
        ret = tkl_disp_convert_lcd_cfg_tkl_to_bk(device, &p_lcd_info->bk_lcd_dev, &p_lcd_info->bk_rgb_cfg);
        if(ret != OPRT_OK) {
            goto REG_LCD_ERR;
        }  

        intfs.TKL_DISP_DRV_INIT           = __tkl_disp_drv_lcd_init;
        intfs.TKL_DISP_DRV_DISPLAY_FRAME  = __tkl_disp_drv_lcd_display_frame;
        intfs.TKL_DISP_DRV_CONFIG         = NULL;
        intfs.TKL_DISP_DRV_DEINIT         = NULL;   
    }

    intfs.TKL_DISP_SET_BRIGHTNESS     = __tkl_disp_set_brightness;

    disp_info.width    = device->width;
    disp_info.height   = device->height;
    disp_info.format   = device->fmt;
    disp_info.rotation = device->rotation;

    ret = tkl_disp_driver_register(device->id, TKL_DISP_LCD, &intfs, \
                                   &disp_info, (TKL_DISP_DRV_HANDLE)p_lcd_info);
    if(ret != OPRT_OK) {
        goto REG_LCD_ERR;
    }  

    return OPRT_OK;

REG_LCD_ERR:
    if(p_lcd_info) {
        tkl_system_free(p_lcd_info);
        p_lcd_info = NULL;
    }

    return ret;
}









