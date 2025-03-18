/*
 * tkl_video_in.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "media_app.h"
#include "media_evt.h"
#include "tkl_video_in.h"
#include "tkl_semaphore.h"
#include "tkl_pinmux.h"
#include "tkl_i2c.h"
#include "tkl_system.h"

extern void tuya_multimedia_power_on(void);

// default
static uint8_t __vi_uvc_power_pin = TUYA_GPIO_NUM_28;
static uint8_t __vi_uvc_active_level = TUYA_GPIO_LEVEL_HIGH;

static uint8_t __vi_dvp_power_pin = TUYA_GPIO_NUM_28;
static uint8_t __vi_dvp_active_level = TUYA_GPIO_LEVEL_HIGH;
static uint8_t __vi_dvp_i2c_clk = TUYA_GPIO_NUM_6;
static uint8_t __vi_dvp_i2c_sda = TUYA_GPIO_NUM_7;

volatile uint8_t vi_uvc_status = 0;
volatile uint8_t vi_dvp_status = 0;

OPERATE_RET tkl_vi_get_power_info(uint8_t device_type, uint8_t *io, uint8_t *active)
{
    if (device_type == UVC_CAMERA) {
        *io = __vi_uvc_power_pin;
        *active = __vi_uvc_active_level;
    } else if (device_type == DVP_CAMERA) {
        *io = __vi_dvp_power_pin;
        *active = __vi_dvp_active_level;
    }

    return 0;
}

int tkl_vi_set_power_info(uint8_t device_type, uint8_t io, uint8_t active)
{
    if (device_type == UVC_CAMERA) {
        __vi_uvc_power_pin = io;
        __vi_uvc_active_level = active;
        bk_printf("set vi uvc power info: %d %d\r\n", io, active);
    } else if (device_type == DVP_CAMERA) {
        __vi_dvp_power_pin = io;
        __vi_dvp_active_level = active;
        bk_printf("set vi dvp power info: %d %d\r\n", io, active);
    }
    return 0;
}

int tkl_vi_set_dvp_i2c_pin(uint8_t clk, uint8_t sda)
{
    __vi_dvp_i2c_clk = clk;
    __vi_dvp_i2c_sda = sda;
    bk_printf("set dvp i2c, clk: %d sda: %d\r\n", clk, sda);

    tkl_io_pinmux_config(clk, TUYA_IIC0_SCL);
    tkl_io_pinmux_config(sda, TUYA_IIC0_SDA);

    // dvp used sw i2c0
    TUYA_IIC_BASE_CFG_T cfg = {
        .role = TUYA_IIC_MODE_MASTER,
        .speed = TUYA_IIC_BUS_SPEED_100K,
        .addr_width = TUYA_IIC_ADDRESS_7BIT,
    };
    tkl_i2c_init(TUYA_I2C_NUM_0, &cfg);

    return 0;
}

int tkl_vi_get_dvp_i2c_idx(void)
{
    return TUYA_I2C_NUM_0;
}

void tkl_vi_update_ll_config(TKL_VI_CONFIG_T *pconfig)
{
    if (pconfig == NULL)
        return;

    // TKL_VI_CONFIG_T *conf = (TKL_VI_CONFIG_T *)config;
    TKL_VI_EXT_CONFIG_T *ext = (TKL_VI_EXT_CONFIG_T *)pconfig->pdata;

    if (ext->type == TKL_VI_EXT_CONF_CAMERA) {
        if (ext->camera.camera_type == TKL_VI_CAMERA_TYPE_UVC) {
            __vi_uvc_power_pin    = ext->camera.power_pin;
            __vi_uvc_active_level = ext->camera.active_level;
        } else if (ext->camera.camera_type == TKL_VI_CAMERA_TYPE_DVP) {
            __vi_dvp_power_pin    = ext->camera.power_pin;
            __vi_dvp_active_level = ext->camera.active_level;
            __vi_dvp_i2c_clk = ext->camera.i2c.clk;
            __vi_dvp_i2c_sda = ext->camera.i2c.sda;
        }
    }
}

/**
 * @brief vi init
 *
* @param[in] pconfig: vi config
* @param[in] count: count of pconfig
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_init(TKL_VI_CONFIG_T *pconfig, int count)
{
    OPERATE_RET ret;

    media_camera_device_t device = {0};
    // TODO pconfig->isp.width / pconfig->isp.height
    // init camera
    if (pconfig == NULL) {
        device.type = UVC_CAMERA;
        device.mode = JPEG_MODE;
        device.fmt = PIXEL_FMT_JPEG;
        device.info.resolution.width = 864;
        device.info.resolution.height = 480;
        device.info.fps = FPS15;
    } else {
        TKL_VI_EXT_CONFIG_T *ext = (TKL_VI_EXT_CONFIG_T *)pconfig->pdata;
        if (ext->type == TKL_VI_EXT_CONF_CAMERA) {
            if (ext->camera.camera_type == TKL_VI_CAMERA_TYPE_UVC) {
                device.type = UVC_CAMERA;
                device.mode = JPEG_MODE;
                device.fmt = PIXEL_FMT_JPEG;
            } else if (ext->camera.camera_type == TKL_VI_CAMERA_TYPE_DVP) {
                device.type = DVP_CAMERA;
                device.mode = H264_YUV_MODE;
                device.fmt = PIXEL_FMT_H264;
            } else {
                bk_printf("not support camera type: %d\r\n", ext->camera.camera_type);
                return OPRT_NOT_SUPPORTED;
            }

            if (ext->camera.camera_type == TKL_VI_CAMERA_TYPE_UVC) {
                __vi_uvc_power_pin    = ext->camera.power_pin;
                __vi_uvc_active_level = ext->camera.active_level;
            } else if (ext->camera.camera_type == TKL_VI_CAMERA_TYPE_DVP) {
                __vi_dvp_power_pin    = ext->camera.power_pin;
                __vi_dvp_active_level = ext->camera.active_level;
                __vi_dvp_i2c_clk = ext->camera.i2c.clk;
                __vi_dvp_i2c_sda = ext->camera.i2c.sda;
            }

            if (pconfig->isp.fps == 15) {
                device.info.fps = FPS15;
            } else if (pconfig->isp.fps == 25) {
                device.info.fps = FPS25;
            } else if (pconfig->isp.fps == 30) {
                device.info.fps = FPS30;
            } else {
                bk_printf("not support camera fps: %d\r\n", pconfig->isp.fps);
                return OPRT_NOT_SUPPORTED;
            }

            device.info.resolution.width  = pconfig->isp.width;
            device.info.resolution.height = pconfig->isp.height;
        }
    }

    bk_printf("video config:\r\n");
    bk_printf("\ttype %d, mode %d, fmt %d\r\n", device.type, device.mode, device.fmt);
    bk_printf("\twidth %d, height %d, fps %d\r\n", device.info.resolution.width, device.info.resolution.height, device.info.fps);

    if (device.type == UVC_CAMERA) {
        bk_printf("\tpower ctrl: %d %d\r\n", __vi_uvc_power_pin, __vi_uvc_active_level);
        device.ty_param[0] = __vi_uvc_power_pin;
        device.ty_param[1] = __vi_uvc_active_level;
    } else if (device.type == DVP_CAMERA) {
        bk_printf("\tpower ctrl: %d %d, i2c: %d %d\r\n",
                __vi_dvp_power_pin, __vi_dvp_active_level, __vi_dvp_i2c_clk, __vi_dvp_i2c_sda);
        device.ty_param[2] = __vi_dvp_power_pin;
        device.ty_param[3] = __vi_dvp_active_level;
        device.ty_param[4] = __vi_dvp_i2c_clk;
        device.ty_param[5] = __vi_dvp_i2c_sda;
    }
    // tuya_multimedia_power_on();
    ret = media_app_camera_open(&device);

    if (ret != OPRT_OK) {
        return ret;
    }

    if (device.type == UVC_CAMERA)
        vi_uvc_status = 1;
    else if (device.type == DVP_CAMERA)
        vi_dvp_status = 1;

    return ret;
}

/**
* @brief vi uninit
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_uninit(TKL_VI_CAMERA_TYPE_E device_type)
{
    OPERATE_RET ret;
    camera_type_t type = UNKNOW_CAMERA;

    if (device_type == TKL_VI_CAMERA_TYPE_UVC) {
        type = UVC_CAMERA;
        vi_uvc_status = 2;
    }
    else if (device_type == TKL_VI_CAMERA_TYPE_DVP) {
        type = DVP_CAMERA;
        vi_dvp_status = 2;
    }

    tkl_system_sleep(100);

    ret = media_app_camera_close(type);
    return ret;
}

/**
* @brief vi set mirror and flip
*
* @param[in] chn: vi chn
* @param[in] flag: mirror and flip value
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_set_mirror_flip(TKL_VI_CHN_E chn, TKL_VI_MIRROR_FLIP_E flag)
{
    return OPRT_NOT_SUPPORTED;
}


/**
* @brief vi get mirror and flip
*
* @param[in] chn: vi chn
* @param[out] flag: mirror and flip value
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_get_mirror_flip(TKL_VI_CHN_E chn, TKL_VI_MIRROR_FLIP_E *flag)
{
    return OPRT_NOT_SUPPORTED;
}

/**
* @brief  set sensor reg value
*
* @param[in] chn: vi chn
* @param[in] pparam: reg info
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_sensor_reg_set( TKL_VI_CHN_E chn, TKL_VI_SENSOR_REG_CONFIG_T *parg)
{
    return OPRT_NOT_SUPPORTED;
}

/**
* @brief  get sensor reg value
*
* @param[in] chn: vi chn
* @param[in] pparam: reg info
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_sensor_reg_get( TKL_VI_CHN_E chn, TKL_VI_SENSOR_REG_CONFIG_T *parg)
{
    return OPRT_OK;
}


/**
* @brief detect init
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @param[in] pconfig: config
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_init(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type, TKL_VI_DETECT_CONFIG_T *p_config)
{
    return OPRT_OK;
}


/**
* @brief detect start
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_start(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type)
{
    return OPRT_NOT_SUPPORTED;
}

/**
* @brief detect stop
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_stop(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type)
{
    return OPRT_OK;
}

/**
* @brief get detection results
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @param[out] presult: detection results
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_get_result(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type, TKL_VI_DETECT_RESULT_T *presult)
{
    return OPRT_NOT_SUPPORTED;
}

/**
* @brief set detection param
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @param[in] pparam: detection param
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_set(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type, TKL_VI_DETECT_PARAM_T *pparam)
{
    return OPRT_OK;
}

/**
* @brief detect uninit
*
* @param[in] chn: vi chn
* @param[in] type: detect type
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_uninit(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type)
{
    return OPRT_NOT_SUPPORTED;
}



