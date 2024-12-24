/**
* @file tkl_ipc.h
* @brief Common process - adapter the inter-processor communication api
* @version 0.1
* @date 2021-08-18
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_IPC_H__
#define __TKL_IPC_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// XXX for temporary use begin
enum {
    TKL_IPC_TYPE_FS_MKDIR = 0x100,
    TKL_IPC_TYPE_FS_REMOVE,                 // 0x101
    TKL_IPC_TYPE_FS_MODE,                   // 0x102
    TKL_IPC_TYPE_FS_IS_EXIST,               // 0x103
    TKL_IPC_TYPE_FS_RENAME,                 // 0x104
    TKL_IPC_TYPE_FS_DIR_OPEN,               // 0x105
    TKL_IPC_TYPE_FS_DIR_CLOSE,              // 0x106
    TKL_IPC_TYPE_FS_DIR_READ,               // 0x107
    TKL_IPC_TYPE_FS_DIR_NAME,               // 0x108
    TKL_IPC_TYPE_FS_DIR_IS_DIRECTORY,       // 0x109
    TKL_IPC_TYPE_FS_DIR_IS_REGULAR,         // 0x10a
    TKL_IPC_TYPE_FS_FOPEN,                  // 0x10b
    TKL_IPC_TYPE_FS_FCLOSE,                 // 0x10c
    TKL_IPC_TYPE_FS_FREAD,                  // 0x10d
    TKL_IPC_TYPE_FS_FWRITE,                 // 0x10e
    TKL_IPC_TYPE_FS_FSYNC,                  // 0x10f
    TKL_IPC_TYPE_FS_FGETS,                  // 0x110
    TKL_IPC_TYPE_FS_FEOF,                   // 0x111
    TKL_IPC_TYPE_FS_FSEEK,                  // 0x112
    TKL_IPC_TYPE_FS_FTELL,                  // 0x113
    TKL_IPC_TYPE_FS_FGETSIZE,               // 0x114
    TKL_IPC_TYPE_FS_FACCESS,                // 0x115
    TKL_IPC_TYPE_FS_FGETC,                  // 0x116
    TKL_IPC_TYPE_FS_FFLUSH,                 // 0x117
    TKL_IPC_TYPE_FS_FILENO,                 // 0x118
    TKL_IPC_TYPE_FS_FTRUNCATE,              // 0x119

    TKL_IPC_TYPE_LVGL = 0x200,
};


struct ipc_msg_s {
    int type;
    union {
        uint8_t buf[256];
        uint32_t buf32[64];
    };
    uint32_t len;
};
// XXX for temporary use end

typedef void* TKL_IPC_HANDLE;

typedef OPERATE_RET (*TKL_IPC_FUNC_CB)(TKL_IPC_HANDLE handle, uint8_t *buf, uint32_t buf_len);

typedef struct {
    TKL_IPC_FUNC_CB  cb;
} TKL_IPC_CONF_T;

/**
 * @brief Function for initializing the inter-processor communication
 *
 * @param[in] config:  @see TKL_IPC_CONF_T
 * @param[out] handle:  ipc handles (allocated by tkl, freeed by tal)
 * @param[out] cnt:  cnt of ipc handle
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_ipc_init(TKL_IPC_CONF_T *config, TKL_IPC_HANDLE *handles, uint8_t *cnt);

/**
 * @brief   Function for send message between processors
 * @param[in] handle  ipc handle
 * @param[in] buf     message buffer
 * @param[in] buf_len message length
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_ipc_send(TKL_IPC_HANDLE handle, const uint8_t *buf, uint32_t buf_len);

#ifdef __cplusplus
}
#endif

#endif
