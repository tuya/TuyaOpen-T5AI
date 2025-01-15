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

// #define __PRINT_CORD_ID(x) #x
// #define LOCAL_CPU_CORE_NAME     "cpu"__PRINT_CORD_ID(CONFIG_CPU_INDEX)

#define TKL_IPC_CHANNEL_MAX         2

#if CONFIG_CPU_INDEX == 0
    #define LOCAL_CPU_CORE_NAME     "cpu0"
    #define TKL_IPC_CHANNEL_NAME0   "tuya01"
    #define TKL_IPC_CHANNEL_NAME1   "tuya02"
#elif CONFIG_CPU_INDEX == 1
    #define LOCAL_CPU_CORE_NAME     "cpu1"
    #define TKL_IPC_CHANNEL_NAME0   "tuya01"
    #define TKL_IPC_CHANNEL_NAME1   "tuya12"
#elif CONFIG_CPU_INDEX == 2
    #define LOCAL_CPU_CORE_NAME     "cpu2"
    #define TKL_IPC_CHANNEL_NAME0   "tuya02"
    #define TKL_IPC_CHANNEL_NAME1   "tuya12"
#endif

struct tkl_ipc_s {
    int idx;
    char *chan_name;
    char *param;
    meida_ipc_t handle;
};

static struct tkl_ipc_s __g_ipc[TKL_IPC_CHANNEL_MAX];

TKL_IPC_FUNC_CB upper_cb = NULL;

static int __tkl_ipc_callback(uint8_t *data, uint32_t size, void *param)
{
    if (upper_cb) {
        upper_cb(param, data, size);
    }
    return 0;
}

OPERATE_RET tkl_ipc_init(TKL_IPC_CONF_T *config, TKL_IPC_HANDLE *handles, uint8_t *cnt)
{
    uint8_t c = 0;
    meida_ipc_t handle1 = NULL;
    meida_ipc_t handle2 = NULL;

    if (handles == NULL || config == NULL) {
        bk_printf("Error: parameter invalid %x %x\r\n", config, handles);
        return OPRT_INVALID_PARM;
    }

    if (config->cb == NULL) {
        bk_printf("Warning: not set ipc cb\r\n");
        return OPRT_INVALID_PARM;
    }

    upper_cb = config->cb;

    media_ipc_chan_cfg_t cfg = {0};
    cfg.cb      = __tkl_ipc_callback;
    cfg.name    = TKL_IPC_CHANNEL_NAME0;
    cfg.param   = (void *)0x1;  // LOCAL_CPU_CORE_NAME;
    int ret = media_ipc_channel_open(&__g_ipc[0].handle, &cfg);
    if (ret != BK_OK) {
        bk_printf("Warning: %s create ipc %s failed, %d\n", LOCAL_CPU_CORE_NAME, TKL_IPC_CHANNEL_NAME0);
    } else {
        __g_ipc[0].idx = 0x1;
        __g_ipc[0].chan_name = TKL_IPC_CHANNEL_NAME0;
        __g_ipc[0].param = LOCAL_CPU_CORE_NAME;
        handles[0] = (TKL_IPC_HANDLE *)0x1;
        *cnt = 1;
    }
    bk_printf("core %s create ipc channel: %s, %x\r\n", LOCAL_CPU_CORE_NAME, TKL_IPC_CHANNEL_NAME0, __g_ipc[0].handle);

#if 0
    int ret = media_ipc_channel_open((meida_ipc_t)&handle1, &cfg);
    if (ret != BK_OK) {
        bk_printf("Warning: %s create ipc %s failed, %d\n", LOCAL_CPU_CORE_NAME, TKL_IPC_CHANNEL_NAME0);
    } else {
        __g_ipc[0].idx = 0x1;
        __g_ipc[0].chan_name = TKL_IPC_CHANNEL_NAME0;
        __g_ipc[0].param = LOCAL_CPU_CORE_NAME;
        __g_ipc[0].handle = handle1;
        handles[0] = (TKL_IPC_HANDLE *)0x1;
        c++;
    }

    cfg.cb      = config->cb;
    cfg.name    = TKL_IPC_CHANNEL_NAME1;
    cfg.param   = (void *)0x2;  // LOCAL_CPU_CORE_NAME;
    ret = media_ipc_channel_open((meida_ipc_t)handle2, &cfg);
    if (ret != BK_OK) {
        bk_printf("Warning: %s create ipc %s failed, %d\n", LOCAL_CPU_CORE_NAME, TKL_IPC_CHANNEL_NAME1);
    } else {
        __g_ipc[1].idx = 0x2;
        __g_ipc[1].chan_name = TKL_IPC_CHANNEL_NAME1;
        __g_ipc[1].param = LOCAL_CPU_CORE_NAME;
        __g_ipc[1].handle = handle2;
        handles[1] = (TKL_IPC_HANDLE *)0x2;
        c++;
    }
    *cnt = 2;
#endif

    return OPRT_OK;
}

OPERATE_RET tkl_ipc_send(TKL_IPC_HANDLE handle, const uint8_t *buf, uint32_t buf_len)
{
    uint8_t idx = (uint8_t)handle - 1;
    if (idx >= TKL_IPC_CHANNEL_MAX) {
        bk_printf("Error: ipc handle invalid, %d\n", idx);
        return OPRT_INVALID_PARM;
    }

    bk_printf("ipc send sync %x\n", __g_ipc[idx].handle);
    int ret = media_ipc_send(&__g_ipc[idx].handle, (void*)buf, buf_len, MIPC_CHAN_SEND_FLAG_SYNC);
    if (ret != BK_OK) {
        bk_printf("Error: ipc send failed, %d\n", ret);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}

OPERATE_RET tkl_ipc_send_no_sync(TKL_IPC_HANDLE handle, const uint8_t *buf, uint32_t buf_len)
{
    uint8_t idx = (uint8_t)handle - 1;
    if (idx >= TKL_IPC_CHANNEL_MAX) {
        bk_printf("Error: ipc handle invalid, %d\n", idx);
        return OPRT_INVALID_PARM;
    }

    bk_printf("ipc send async %x\n", __g_ipc[idx].handle);
    int ret = media_ipc_send(&__g_ipc[idx].handle, (void*)buf, buf_len, 0);
    if (ret != BK_OK) {
        bk_printf("Error: ipc send failed, %d\n", ret);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}

