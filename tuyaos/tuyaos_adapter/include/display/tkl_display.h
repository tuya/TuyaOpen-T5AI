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
    TKL_DISP_PIXEL_FMT_RGB565_LE,
    TKL_DISP_PIXEL_FMT_RGB666,
    TKL_DISP_PIXEL_FMT_RGB888,
    TKL_DISP_PIXEL_FMT_YUYV,
    TKL_DISP_PIXEL_FMT_VUYY,
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
    void                *buffer;
    TKL_DISP_RECT_S      rect;
    TKL_DISP_PIXEL_FMT_E format;
    int                  priority;
    void                *param;
} TKL_DISP_FRAMEBUFFER_S;

typedef void (*TKL_DISP_VSYNC_CB)(TKL_DISP_PORT_E port, int64_t timestamp);
typedef void (*TKL_DISP_HOTPLUG_CB)(TKL_DISP_PORT_E port, bool connected);

typedef struct {
    TKL_DISP_VSYNC_CB vsync_cb;
    TKL_DISP_HOTPLUG_CB hotplug_cb;
} TKL_DISP_EVENT_HANDLER_S;

typedef struct
{
    int width;
    int height;
    int bpp;
    int dpi;
    int fps;
    TKL_DISP_PIXEL_FMT_E format;
    TKL_DISP_ROTATION_E rotation;
} TKL_DISP_INFO_S;
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
 * @param event_handler display event callback function
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

#ifdef __cplusplus
}
#endif

#endif
