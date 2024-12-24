 /**
 * @file tkl_display.h
 * @brief Common process - display
 * @version 0.1
 * @date 2021-07-26
 *
 * @copyright Copyright 2019-2021 Tuya Inc. All Rights Reserved.
 *
 * 1. 支持图像的位块传输。
 * 2. 支持色彩填充。
 * 3. 需要支持HDMI/VGA/DP等外接设备的热插拔通知。
 * 4. 需要支持HDMI/VGA/DP等外接设备格式/分辨率/刷新率的查询和设置
 * 5. 需要支持帧同步接口，避免图像刷新的撕裂。
 * 6. 需要支持多个图层。
 * 7. 需要提供一个类似gralloc的内存管理接口，用于将内核framebuffer或dma-buf直接映射给应用使用，减少拷贝操作
 */

#ifndef __TKL_DISPLAY_H__
#define __TKL_DISPLAY_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
    extern "C" {
#endif

typedef enum {
    TKL_DISP_LCD = 0,
    TKL_DISP_VGA,
    TKL_DISP_HDMI,
    TKL_DISP_DP,
    TKL_DISP_NUM
} TKL_DISP_PORT_E;

typedef enum {
    TKL_DISP_PIXEL_FMT_ABGR = 0,
    TKL_DISP_PIXEL_FMT_RGBX,
    TKL_DISP_PIXEL_FMT_RGBA,
    TKL_DISP_PIXEL_FMT_ARGB,
    TKL_DISP_PIXEL_FMT_RGB565,
    TKL_DISP_PIXEL_FMT_RGB666,
    TKL_DISP_PIXEL_FMT_RGB888,
} TKL_DISP_PIXEL_FMT_E;

typedef enum {
    TKL_DISP_ROTATION_0 = 0,
    TKL_DISP_ROTATION_90,
    TKL_DISP_ROTATION_180,
    TKL_DISP_ROTATION_270
} TKL_DISP_ROTATION_E;

typedef enum {
    TKL_DISP_POWER_OFF = 0,
    TKL_DISP_POWER_ON,
    TKL_DISP_POWER_NUM
} TKL_DISP_POWER_MODE_E;

typedef enum {
    TKL_DISP_BL_GPIO = 0,
    TKL_DISP_BL_PWM,
} TKL_DISP_BLIGHT_E;

typedef enum {
    TKL_DISP_POWERON_RESET = 0,
    TKL_DISP_GPIO_RESET,
} TKL_DISP_RST_MODE_E;

/** RGB interface config data active edge*/
typedef enum {
    POSITIVE_EDGE = 0,
    NEGATIVE_EDGE
} DATA_ACTIVE_EDGE_E;

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
} LCD_CLK_T;

typedef enum  {
    SPI_LCD_RESET = 0,
    SPI_CONFIG_REG,
    SPI_DELAY,
    SPI_ITEM_END
} LCD_SPI_CONFIG_TYPE;

typedef union {
    struct
    {
        uint16_t b : 5;
        uint16_t g : 6;
        uint16_t r : 5;
    }c16;

    struct
    {
        uint8_t b;
        uint8_t g;
        uint8_t r;
    }c24;

    struct
    {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t a;
    } c32;
    uint32_t full;
} TKL_DISP_COLOR_U;

typedef struct
{
    int x;
    int y;
    int width;
    int height;
} TKL_DISP_RECT_S;

typedef struct
{
    void *buffer;
    TKL_DISP_RECT_S rect;
    TKL_DISP_PIXEL_FMT_E format;
    int priority;
} TKL_DISP_FRAMEBUFFER_S;


typedef void (*TKL_DISP_VSYNC_CB)(TKL_DISP_PORT_E port, INT64_T timestamp);
typedef void (*TKL_DISP_HOTPLUG_CB)(TKL_DISP_PORT_E port, BOOL_T connected);
typedef struct {
    TKL_DISP_VSYNC_CB vsync_cb;
    TKL_DISP_HOTPLUG_CB hotplug_cb;
} TKL_DISP_EVENT_HANDLER_S;

typedef struct
{
    int mode;        // ref TKL_DISP_BLIGHT_E
    TUYA_GPIO_NUM_E io;
    TUYA_GPIO_LEVEL_E active_level;    // if control is gpio
} TKL_DISP_BL_CONF_S;

typedef struct
{
    TUYA_GPIO_NUM_E clk;
    TUYA_GPIO_NUM_E csx;
    TUYA_GPIO_NUM_E sda;
    TUYA_GPIO_NUM_E rst;                // invalid 0xFF
    TKL_DISP_RST_MODE_E rst_mode;       // ref TKL_DISP_RST_MODE_E
} TKL_DISP_SPI_CONF_S;

typedef struct
{
    TUYA_GPIO_NUM_E tp_i2c_clk;
    TUYA_GPIO_NUM_E tp_i2c_sda;
    TUYA_GPIO_NUM_E tp_rst;                // reset gpio
    TUYA_GPIO_NUM_E tp_intr;               // interrupt gpio
} TKL_DISP_TP_CONF_S;

typedef struct
{
    LCD_CLK_T clk;                         /**< config lcd clk */
    DATA_ACTIVE_EDGE_E active_edge;

    uint16_t hsync_back_porch;            /**< rang 0~0x3FF (0~1023), should refer lcd device spec*/
    uint16_t hsync_front_porch;           /**< rang 0~0x3FF (0~1023), should refer lcd device spec*/
    uint16_t vsync_back_porch;            /**< rang 0~0xFF (0~255), should refer lcd device spec*/
    uint16_t vsync_front_porch;           /**< rang 0~0xFF (0~255), should refer lcd device spec*/
    uint8_t  hsync_pulse_width;           /**< rang 0~0x3F (0~7), should refer lcd device spec*/
    uint8_t  vsync_pulse_width;           /**< rang 0~0x3F (0~7), should refer lcd device spec*/
} TY_RGB_CFG_T;

/** SPI interface config param, TODO */
typedef struct
{
    uint32_t spi_clk;
    uint32_t init_cmd;
} TY_SPI_CFG_T;

struct lcd_reset_seq {
    uint16_t gpio_level;
    uint16_t delay_time;
};

struct lcd_reg_set {
    uint8_t r;
    uint8_t len;
    uint8_t v[16];
};

struct lcd_init_s {
    LCD_SPI_CONFIG_TYPE type;
    union {
        uint32_t delay_time;
        struct lcd_reg_set reg;
        struct lcd_reset_seq reset[3];
    };
};

typedef struct
{
    char *name;
    TUYA_LCD_TYPE_E type;
    TKL_DISP_PIXEL_FMT_E fmt;   /* output data format */
    union {
        const TY_RGB_CFG_T *rgb;
        const TY_SPI_CFG_T *spi;
    };

    struct lcd_init_s *lcd_init_sequence;
} TUYA_LCD_DEVICE_T;


#define IC_NAME_LENGTH    16
typedef struct
{
    uint32_t magic;                     // TUYA 0x54555941
    TKL_DISP_BL_CONF_S bl;
    TKL_DISP_SPI_CONF_S spi;
    TUYA_GPIO_NUM_E power_ctrl_pin;
    TUYA_GPIO_LEVEL_E power_active_level;
    TKL_DISP_PIXEL_FMT_E rgb_mode;
    INT8_T ic_name[IC_NAME_LENGTH];
    uint32_t deivce_ppi;
    TKL_DISP_TP_CONF_S tp;
    int enable_lcd_pipeline;
    TUYA_LCD_DEVICE_T *init_param;
} TKL_DISP_LL_CTRL_S;

typedef struct
{
    int width;
    int height;
    int bpp;
    int dpi;
    int fps;
    TKL_DISP_PIXEL_FMT_E format;
    TKL_DISP_ROTATION_E rotation;
    TKL_DISP_LL_CTRL_S ll_ctrl;
} TKL_DISP_INFO_S;

typedef enum {
    TKL_DISP_BLEND_WIFI = 0,
    TKL_DISP_BLEND_VERSION,
    TKL_DISP_BLEND_TIME,
    TKL_DISP_BLEND_DATA,
    TKL_DISP_BLEND_ALL,         // only used close
    TKL_DISP_BLEND_INVALID,
} TKL_DISP_BLEND_E;

typedef struct
{
    int x;
    int y;
    TKL_DISP_BLEND_E type;  // TKL_DISP_BLEND_E
    char data[32];
} TKL_DISP_BLEND_INFO_S;

typedef struct
{
	int device_id;
	void *device_info;
	TKL_DISP_PORT_E device_port;
}TKL_DISP_DEVICE_S;

/**
 * @brief Init and config display device
 *
 * @param display_device display device
 * @param cfg display configuration
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_init(TKL_DISP_DEVICE_S *display_device, TKL_DISP_EVENT_HANDLER_S *event_handler);

/**
 * @brief Release display device
 *
 * @param display_device display device
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_deinit(TKL_DISP_DEVICE_S *display_device);

/**
 * @brief Set display info
 *
 * @param display_device display device
 * @param info display device info
 * @return OPERATE_RET
 */
OPERATE_RET tkl_disp_set_info(TKL_DISP_DEVICE_S *display_device, TKL_DISP_INFO_S *info);

/**
 * @brief Get display info
 *
 * @param display_device display device
 * @param info display device info
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_get_info(TKL_DISP_DEVICE_S *display_device, TKL_DISP_INFO_S *info);

/**
 * @brief
 *
 * @param display_device display device
 * @param buf framebuffer
 * @param rect destination area
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_blit(TKL_DISP_DEVICE_S *display_device, TKL_DISP_FRAMEBUFFER_S *buf, TKL_DISP_RECT_S *rect);

/**
 * @brief Fill destination area with color
 *
 * @param display_device display device
 * @param rect destination area to fill
 * @param color color to fill
 * @return OPERATE_RET
 */
OPERATE_RET tkl_disp_fill(TKL_DISP_DEVICE_S *display_device, TKL_DISP_RECT_S *rect, TKL_DISP_COLOR_U color);

/**
 * @brief Flush buffers to display device
 *
 * @param display_device display device
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_flush(TKL_DISP_DEVICE_S *display_device);

/**
 * @brief Wait for vsync signal
 *
 * @param display_device display device
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_wait_vsync(TKL_DISP_DEVICE_S *display_device);

/**
 * @brief Set display brightness(Backlight or HSB)
 *
 * @param display_device display device
 * @param brightness brightness
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_set_brightness(TKL_DISP_DEVICE_S *display_device, int brightness);

/**
 * @brief Get display brightness(Backlight or HSB)
 *
 * @param display_device display device
 * @param brightness brightness
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_get_brightness(TKL_DISP_DEVICE_S *display_device, int *brightness);

/**
 * @brief Sets the display screen's power state
 *
 * @param display_device display device
 * @param power_mode power state
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_set_power_mode(TKL_DISP_DEVICE_S *display_device, TKL_DISP_POWER_MODE_E power_mode);

/**
 * @brief Gets the display screen's power state
 *
 * @param display_device display device
 * @param power_mode power state
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_get_power_mode(TKL_DISP_DEVICE_S *display_device, TKL_DISP_POWER_MODE_E *power_mode);


/**
 * @brief Alloc mapped framebuffer or layer
 *
 * @param display_device display device
 * @return void* Pointer to mapped memory
 */
TKL_DISP_FRAMEBUFFER_S *tkl_disp_alloc_framebuffer(TKL_DISP_DEVICE_S *display_device);

/**
 * @brief Free mapped framebuffer or layer
 *
 * @param display_device display device
 * @param buf Pointer to mapped memory
 * @return void
 */
void tkl_disp_free_framebuffer(TKL_DISP_DEVICE_S *display_device, TKL_DISP_FRAMEBUFFER_S *buf);

/**
 * @brief Get capabilities supported by display(For external display device. e.g. HDMI/VGA)
 *
 * @param display_device display device
 * @param cfg configurations
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_get_capabilities(TKL_DISP_DEVICE_S *display_device, TKL_DISP_INFO_S **cfg);

/**
 * @brief Free capabilities get by tkl_disp_get_capabilities()
 *
 * @param display_device display device
 * @param cfg configurations
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_free_capabilities(TKL_DISP_DEVICE_S *display_device, TKL_DISP_INFO_S *cfg);

/**
 * @brief Set lcd blend info
 *
 * @param display_device display device
 * @param cfg configurations
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_set_blend_info(TKL_DISP_DEVICE_S *display_device, TKL_DISP_BLEND_INFO_S *cfg);

OPERATE_RET tkl_disp_cancel_blend_info(TKL_DISP_DEVICE_S *display_device, TKL_DISP_BLEND_INFO_S *cfg);

OPERATE_RET tkl_disp_open_startup_image(TKL_DISP_DEVICE_S *display_device, uint32_t address);

OPERATE_RET tkl_disp_close_startup_image(TKL_DISP_DEVICE_S *display_device);

int tkl_display_rgb_mode(void);

int tkl_display_bl_mode(void);

OPERATE_RET tkl_display_bl_ctrl_io(uint8_t *io, uint8_t *active_level);

OPERATE_RET tkl_display_power_ctrl_pin(uint8_t *io, uint8_t *active_level);

int tkl_disp_get_id(uint32_t width, uint32_t height, const char *name);

uint32_t tkl_disp_get_ppi(void);
char *tkl_disp_get_lcd_name(void);
#ifdef __cplusplus
}
#endif

#endif
