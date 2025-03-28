/**
 * @file tkl_disp_drv_lcd.h
 * @version 0.1
 * @date 2025-02-25
 */

#ifndef __TKL_DISP_DRV_LCD_H__
#define __TKL_DISP_DRV_LCD_H__

#include "tuya_cloud_types.h"
#include "tkl_disp_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
************************macro define************************
***********************************************************/
#define INVALID_GPIO_PIN                 0xFF

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef enum {
    TUYA_LCD_TYPE_RGB = 0,
    TUYA_LCD_TYPE_MCU,
    TUYA_LCD_TYPE_SPI,
    TUYA_LCD_TYPE_QSPI,
} TUYA_LCD_TYPE_E;

/** rgb lcd clk select, infulence pfs, select according to lcd device spec*/
typedef enum {
    TUYA_LCD_80M,
    TUYA_LCD_64M,
    TUYA_LCD_60M,
    TUYA_LCD_54M,
    TUYA_LCD_45M, //45.7M
    TUYA_LCD_40M,
    TUYA_LCD_35M, //35.5
    TUYA_LCD_32M,
    TUYA_LCD_30M,
    TUYA_LCD_26M, //26.6M
    TUYA_LCD_24M, //24.6M
    TUYA_LCD_22M, //22.85M
    TUYA_LCD_20M,
    TUYA_LCD_17M, //17.1M
    TUYA_LCD_15M,
    TUYA_LCD_12M,
    TUYA_LCD_10M,
    TUYA_LCD_9M,  //9.2M
    TUYA_LCD_8M,
    TUYA_LCD_7M   //7.5M
} TUYA_LCD_CLK_T;

/** RGB interface config data active edge*/
typedef enum {
    TUYA_RGB_POSITIVE_EDGE = 0,
    TUYA_RGB_NEGATIVE_EDGE
} TUYA_RGB_ACTIVE_EDGE_E;

typedef struct
{
    TUYA_LCD_CLK_T         clk;           /**< config lcd clk */
    TUYA_RGB_ACTIVE_EDGE_E active_edge;

    uint16_t hsync_back_porch;            /**< rang 0~0x3FF (0~1023), should refer lcd device spec*/
    uint16_t hsync_front_porch;           /**< rang 0~0x3FF (0~1023), should refer lcd device spec*/
    uint16_t vsync_back_porch;            /**< rang 0~0xFF (0~255), should refer lcd device spec*/
    uint16_t vsync_front_porch;           /**< rang 0~0xFF (0~255), should refer lcd device spec*/
    uint8_t  hsync_pulse_width;           /**< rang 0~0x3F (0~7), should refer lcd device spec*/
    uint8_t  vsync_pulse_width;           /**< rang 0~0x3F (0~7), should refer lcd device spec*/
} TUYA_LCD_RGB_CFG_T;

/** SPI interface config param, TODO */
typedef struct
{
    TUYA_GPIO_NUM_E rst_pin;
    TUYA_GPIO_NUM_E cs_pin;
    TUYA_GPIO_NUM_E dc_pin;
    TUYA_SPI_NUM_E  spi_port;
    uint32_t        spi_clk;
    uint8_t         cmd_caset;
    uint8_t         cmd_raset;
    uint8_t         cmd_ramwr;
    const uint8_t  *init_seq;
} TUYA_LCD_SPI_CFG_T;

typedef enum {
    TUYA_DISP_BL_GPIO = 0,
    TUYA_DISP_BL_PWM,
    TUYA_DISP_BL_NOT_EXIST,
} TUYA_LCD_BL_MODE_E;

typedef struct {
    TUYA_GPIO_NUM_E         io;
    TUYA_GPIO_LEVEL_E       active_lv;   
}LCD_BL_GPIO_CFG_T;

typedef struct {
    TUYA_PWM_NUM_E          id;
    TUYA_PWM_BASE_CFG_T     cfg;
}LCD_BL_PWM_CFG_T;

typedef struct
{
    TUYA_LCD_BL_MODE_E      mode;  
    union {
        LCD_BL_GPIO_CFG_T  gpio;
        LCD_BL_PWM_CFG_T   pwm;
    };
} TUYA_LCD_BL_CFG_T;

typedef struct {
    int                     id;
    int                     width;
    int                     height;
    TKL_DISP_PIXEL_FMT_E    fmt;
    TUYA_LCD_TYPE_E         lcd_tp;
    TKL_DISP_ROTATION_E     rotation;
    union {
        const TUYA_LCD_RGB_CFG_T  *p_rgb;
        const TUYA_LCD_SPI_CFG_T  *p_spi;
    };
    TUYA_LCD_BL_CFG_T       bl; 
    TUYA_GPIO_NUM_E         power_io;
    TUYA_GPIO_LEVEL_E       power_active_lv;   
}TUYA_LCD_CFG_T;

/***********************************************************
********************function declaration********************
***********************************************************/
OPERATE_RET tkl_disp_register_lcd_dev(TUYA_LCD_CFG_T *device);

#ifdef __cplusplus
}
#endif

#endif /* __TKL_DISP_DRV_LCD_H__ */
