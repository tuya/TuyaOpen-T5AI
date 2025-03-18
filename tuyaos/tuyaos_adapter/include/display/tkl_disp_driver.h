/**
 * @file tkl_disp_driver.h
 * @version 0.1
 * @date 2025-03-04
 */

#ifndef __TKL_DISP_DRIVER_H__
#define __TKL_DISP_DRIVER_H__

#include "tkl_display.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
************************macro define************************
***********************************************************/


/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef void* TKL_DISP_DRV_HANDLE;

typedef unsigned int TKL_DISP_DRV_CFG_CMD_E;
       
typedef struct {
    OPERATE_RET (*TKL_DISP_DRV_INIT)(TKL_DISP_DRV_HANDLE handle);
    OPERATE_RET (*TKL_DISP_DRV_DISPLAY_FRAME)(TKL_DISP_DRV_HANDLE handle, TKL_DISP_FRAMEBUFFER_S *fb);
    OPERATE_RET (*TKL_DISP_DRV_CONFIG)(TKL_DISP_DRV_HANDLE handle, TKL_DISP_DRV_CFG_CMD_E cmd, void *param);
    OPERATE_RET (*TKL_DISP_DRV_DEINIT)(TKL_DISP_DRV_HANDLE handle);
    OPERATE_RET (*TKL_DISP_SET_BRIGHTNESS)(TKL_DISP_DRV_HANDLE handle, int brightness);
}TKL_DISP_DRV_INTFS_T;

/***********************************************************
********************function declaration********************
***********************************************************/
/**
 * @brief register display driver
 *
 * @param disp_port display device id
 * @param disp_port display driver port
 * @param intfs     display driver interface
 * @param dev_info  display device infomation
 * @param handle    display driver handle
 * @return OPERATE_RET 0 on success. A negative error code on error.
 */
OPERATE_RET tkl_disp_driver_register(int dev_id, TKL_DISP_PORT_E disp_port, \
                                     TKL_DISP_DRV_INTFS_T *intfs, TKL_DISP_INFO_S *dev_info, \
                                     TKL_DISP_DRV_HANDLE handle);


void tkl_disp_driver_display_frame_complete(void);

#ifdef __cplusplus
}
#endif

#endif /* __TKL_DISP_DRIVER_H__ */
