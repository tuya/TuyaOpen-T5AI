/*
 * tkl_ipc.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include <os/os.h>
#include "sdkconfig.h"
#include "tuya_error_code.h"
#include "tkl_ipc.h"
#include "media_ipc.h"
#include "tkl_mutex.h"

#if CONFIG_CPU_INDEX == 0
    #define LOCAL_CPU_CORE_NAME         "cpu0"
#elif CONFIG_CPU_INDEX == 1
    #define LOCAL_CPU_CORE_NAME         "cpu1"
#elif CONFIG_CPU_INDEX == 2
    #define LOCAL_CPU_CORE_NAME         "cpu2"
#endif

#define TKL_IPC_CHANNEL_NAME01      "tuya01"
#define TKL_IPC_CHANNEL_NAME02      "tuya02"
#define TKL_IPC_CHANNEL_NAME12      "tuya12"

TKL_IPC_FUNC_CB upper_cb = NULL;
meida_ipc_t handle = NULL;
TKL_MUTEX_HANDLE tkl_ipc_mutex= NULL;

static int __tkl_ipc_callback(uint8_t *data, uint32_t size, void *param)
{
    if (upper_cb) {
        upper_cb(data, size);
    }
    return 0;
}

OPERATE_RET tkl_ipc_init(TKL_IPC_CONF_T *config)
{
    if ( config == NULL) {
        bk_printf("Error: parameter invalid %x\r\n", config);
        return OPRT_INVALID_PARM;
    }

    if (config->cb == NULL) {
        bk_printf("Warning: not set ipc cb\r\n");
        return OPRT_INVALID_PARM;
    }

    upper_cb = config->cb;

    media_ipc_chan_cfg_t cfg = {0};
    cfg.cb      = __tkl_ipc_callback;

    // #if CONFIG_CPU_INDEX == 0
    // cfg.name    = TKL_IPC_CHANNEL_NAME1;
    // #elif CONFIG_CPU_INDEX == 1
    // cfg.name    = TKL_IPC_CHANNEL_NAME0;
    // #endif
    cfg.name    = TKL_IPC_CHANNEL_NAME01;

    int ret = media_ipc_channel_open(&handle, &cfg);
    if (ret != BK_OK) {
        bk_printf("Warning: %s create ipc %s failed, %d\n", LOCAL_CPU_CORE_NAME, cfg.name);
    }
    bk_printf("core %s create ipc channel: %s, %x\r\n", LOCAL_CPU_CORE_NAME, cfg.name, handle);


    tkl_mutex_create_init(&tkl_ipc_mutex);

    return OPRT_OK;
}

OPERATE_RET tkl_ipc_send(const uint8_t *buf, uint32_t buf_len)
{
    int ret = media_ipc_send(&handle, (void*)buf, buf_len, MIPC_CHAN_SEND_FLAG_SYNC);
    if (ret != BK_OK) {
        bk_printf("Error: ipc send failed, %d\n", ret);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}

OPERATE_RET tkl_ipc_send_no_sync(const uint8_t *buf, uint32_t buf_len)
{
    tkl_mutex_lock(tkl_ipc_mutex);
    int ret = media_ipc_send(&handle, (void*)buf, buf_len, 0);
    if (ret != BK_OK) {
        bk_printf("Error: ipc send failed, %d\n", ret);
        return OPRT_COM_ERROR;
    }
    tkl_mutex_unlock(tkl_ipc_mutex);
    return OPRT_OK;
}

