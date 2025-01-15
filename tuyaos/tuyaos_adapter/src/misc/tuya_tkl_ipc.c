/*
 * tuya_tkl_ipc.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "sdkconfig.h"
#include "tkl_ipc.h"
#include "tkl_fs.h"
#include "tkl_semaphore.h"

TKL_IPC_HANDLE __ipc_handle[2] = {0, 0};
extern OPERATE_RET tkl_ipc_send_no_sync(TKL_IPC_HANDLE handle, const uint8_t *buf, uint32_t buf_len);

#ifdef CONFIG_SYS_CPU1
extern TKL_SEM_HANDLE fs_api_access_sem;
#endif

// TODO ugly
static TUYA_FILEINFO info = NULL;
static struct ipc_msg_s result;

static OPERATE_RET tuya_ipc_cb(TKL_IPC_HANDLE handle, uint8_t *buf, uint32_t buf_len)
{
    struct ipc_msg_s *msg = (struct ipc_msg_s *)buf;
    int ret = -1;

#if CONFIG_SYS_CPU0
    memset(&result, 0, sizeof(struct ipc_msg_s));

    result.type = msg->type;

    switch (msg->type) {
        case TKL_IPC_TYPE_FS_MKDIR:
            if (msg->buf!= NULL) {
                char *path = (char *)msg->buf;
                ret = tkl_fs_mkdir(path);
            }
            result.buf32[0] = ret;
            result.len = sizeof(ret);
            break;
        case TKL_IPC_TYPE_FS_REMOVE:                 // 0x101
            if (msg->buf != NULL) {
                char *path = (char *)msg->buf;
                ret = tkl_fs_remove(path);
            }
            result.buf32[0] = ret;
            result.len = sizeof(ret);
            break;
        case TKL_IPC_TYPE_FS_MODE:                   // 0x102
            if (msg->buf != NULL) {
                char *path = (char *)msg->buf;
                uint32_t mode = 0;
                ret = tkl_fs_mode(path, &mode);
                result.buf32[0] = ret;
                result.buf32[1] = (int)mode;
                result.len = sizeof(ret) + sizeof(mode);
            } else {
                result.buf32[0] = ret;
                result.len = sizeof(ret);
            }
            break;
        case TKL_IPC_TYPE_FS_IS_EXIST:               // 0x103
            if (msg->buf != NULL) {
                char *path = (char *)msg->buf;
                BOOL_T is_exist = 0;
                ret = tkl_fs_is_exist(path, &is_exist);
                result.buf32[0] = ret;
                result.buf32[1] = (int)is_exist;
                result.len = 2*sizeof(int);
            } else {
                result.buf32[0] = ret;
                result.len = sizeof(int);
            }
            break;
        case TKL_IPC_TYPE_FS_RENAME:                 // 0x104
            if (msg->buf != NULL) {
                char *path_old = (char *)msg->buf;
                int ofs = strlen(path_old) + 1;
                char *path_new = (char *)((char *)msg->buf + ofs);
                if (path_new != NULL) {
                    ret = tkl_fs_rename(path_old, path_new);
                }
            }
            result.buf32[0] = ret;
            result.len = sizeof(ret);
            break;
        case TKL_IPC_TYPE_FS_DIR_OPEN:               // 0x105
            if (msg->buf != NULL) {
                char *path = (char *)msg->buf;
                TUYA_DIR dir = NULL;
                ret = tkl_dir_open(path, &dir);
                result.buf32[0] = ret;
                result.buf32[1] = (int)dir;
                result.len = 2*sizeof(ret);
            } else {
                result.buf32[0] = ret;
                result.len = sizeof(int);
            }
            break;
        case TKL_IPC_TYPE_FS_DIR_CLOSE:              // 0x106
            if (msg->buf != NULL) {
                TUYA_DIR dir = (TUYA_DIR)msg->buf32[0];
                ret = tkl_dir_close(dir);
            }
            result.buf32[0] = ret;
            result.len = sizeof(ret);
            break;
        case TKL_IPC_TYPE_FS_DIR_READ:               // 0x107
            if (msg->buf != NULL) {
                TUYA_DIR dir = (TUYA_DIR)msg->buf32[0];
                ret = tkl_dir_read(dir, &info);
                result.buf32[0] = ret;
                result.buf32[1] = (int)info;
                result.len = 2*sizeof(int);
            } else {
                result.buf32[0] = ret;
                result.len = sizeof(ret);
            }
            break;
        case TKL_IPC_TYPE_FS_DIR_NAME:               // 0x108
            // do nothing
            break;
        case TKL_IPC_TYPE_FS_DIR_IS_DIRECTORY:       // 0x109
            // do nothing
            break;
        case TKL_IPC_TYPE_FS_DIR_IS_REGULAR:         // 0x10a
            // do nothing
            break;
        case TKL_IPC_TYPE_FS_FOPEN:                  // 0x10b
            if (msg->buf != NULL) {
                char *path = (char *)msg->buf;
                uint32_t path_ofs = strlen(path) + 1;
                char *mode = (char *)msg->buf + path_ofs;

                bk_printf("open %s, mode: %s\r\n", path, mode);
                TUYA_FILE file = tkl_fopen(path, mode);
                result.buf32[0] = (int)file;
                result.len = sizeof(int);
            } else {
                result.buf32[0] = ret;
                result.len = sizeof(ret);
            }
            break;
        case TKL_IPC_TYPE_FS_FCLOSE:                 // 0x10c
            if (msg->buf != NULL) {
                TUYA_FILE file = (TUYA_FILE)msg->buf32[0];
                ret = tkl_fclose(file);
            }
            result.buf32[0] = (int)ret;
            result.len = sizeof(ret);
            break;
        case TKL_IPC_TYPE_FS_FREAD:                  // 0x10d
            if (msg->buf != NULL) {
                uint32_t *info = msg->buf32;
                uint8_t *buf = info[0];
                int bytes =  info[1];
                TUYA_FILE file = (TUYA_FILE)info[2];
                ret = tkl_fread(buf, bytes, file);
            }
            result.buf32[0] = (int)ret;
            result.len = sizeof(ret);
            break;
        case TKL_IPC_TYPE_FS_FWRITE:                 // 0x10e
            if (msg->buf != NULL) {
                uint32_t *info = msg->buf32;
                uint8_t *buf = info[0];
                int bytes =  info[1];
                TUYA_FILE file = (TUYA_FILE)info[2];
                ret = tkl_fwrite(buf, bytes, file);
            }
            result.buf32[0] = (int)ret;
            result.len = sizeof(ret);
            break;
        case TKL_IPC_TYPE_FS_FSYNC:                  // 0x10f
            if (msg->buf != NULL) {
                int *info = (int *)msg->buf32;
                int fd = info[0];
                ret = tkl_fsync(fd);
            }
            result.buf32[0] = (int)ret;
            result.len = sizeof(ret);
            break;
        case TKL_IPC_TYPE_FS_FGETS:                  // 0x110
            if (msg->buf != NULL) {
                uint32_t *info = msg->buf32;
                uint8_t *buf = info[0];
                int bytes =  info[1];
                TUYA_FILE file = (TUYA_FILE)info[2];
                ret = tkl_fgets(buf, bytes, file);
            }
            result.buf32[0] = (int)ret;
            result.len = sizeof(ret);
            break;
        case TKL_IPC_TYPE_FS_FEOF:                   // 0x111
            if (msg->buf != NULL) {
                uint32_t *info = msg->buf32;
                TUYA_FILE file = (TUYA_FILE)info[0];
                ret = tkl_feof(file);
            }
            result.buf32[0] = (int)ret;
            result.len = sizeof(ret);
            break;
        case TKL_IPC_TYPE_FS_FSEEK:                  // 0x112
            if (msg->buf != NULL) {
                uint32_t *info = msg->buf32;
                TUYA_FILE file = (TUYA_FILE)info[0];
                INT64_T offs = (info[1] << 32) | info[2];
                int whence = (TUYA_FILE)info[3];
                ret = tkl_fseek(file, offs, whence);
            }
            result.buf32[0] = (int)ret;
            result.len = sizeof(ret);
            break;
        case TKL_IPC_TYPE_FS_FTELL:                  // 0x113
            if (msg->buf != NULL) {
                uint32_t *info = msg->buf32;
                TUYA_FILE file = (TUYA_FILE)info[0];
                ret = tkl_ftell(file);
            }
            result.buf32[0] = (int)ret;
            result.len = sizeof(ret);
            break;
        case TKL_IPC_TYPE_FS_FGETSIZE:               // 0x114
            {
                int size = 0;
                if (msg->buf != NULL) {
                    char *path = (char *)msg->buf;
                    size = tkl_fgetsize(path);
                }
                result.buf32[0] = (int)size;
                result.len = sizeof(ret);
            }
            break;
        case TKL_IPC_TYPE_FS_FACCESS:                // 0x115
            if (msg->buf != NULL) {
                char *path = (char *)msg->buf;
                uint32_t path_ofs = strlen(path) + 1;
                char *mode = (char *)msg->buf + path_ofs;
                bk_printf("tkl_faccess: %s, mode: %s", path, mode);
                ret = tkl_faccess(path, mode);
            }
            result.buf32[0] = (int)ret;
            result.len = sizeof(ret);
            break;
        case TKL_IPC_TYPE_FS_FGETC:                  // 0x116
            if (msg->buf != NULL) {
                int *info = (int *)msg->buf;
                TUYA_FILE file = (TUYA_FILE)info[0];
                ret = tkl_fgetc(file);
            }
            result.buf32[0] = (int)ret;
            result.len = sizeof(ret);
            break;
        case TKL_IPC_TYPE_FS_FFLUSH:                 // 0x117
            break;
        case TKL_IPC_TYPE_FS_FILENO:                 // 0x118
            break;
        case TKL_IPC_TYPE_FS_FTRUNCATE:              // 0x119
            break;
        default:
            break;
    }

    if (msg->type == TKL_IPC_TYPE_LVGL) {
        tkl_lvgl_ipc_func_cb(handle, buf, buf_len);
    } else {
        // send result
        tkl_ipc_send_no_sync(__ipc_handle[0], &result, sizeof(struct ipc_msg_s));
        bk_printf("cpu0, ipc message type: %x, %p, result: %x %x %x %p\r\n", msg->type, buf, result.type, result.buf32[0], result.buf32[1], &result);
    }
#else // client
    if (msg->type == TKL_IPC_TYPE_LVGL) {
        tkl_lvgl_ipc_func_cb(handle, buf, buf_len);
    } else {
        extern uint32_t fs_result[2];
        fs_result[0] = msg->buf32[0];
        fs_result[1] = msg->buf32[1];
        bk_printf("cpu1, ipc result type: %x %x %x %p\r\n", msg->type, fs_result[0], fs_result[1], buf);
        tkl_semaphore_post(fs_api_access_sem);
    }
#endif // CONFIG_SYS_CPU0

    return 0;
}

OPERATE_RET tuya_ipc_init(void)
{
    TKL_IPC_CONF_T ipc_conf;
    ipc_conf.cb = tuya_ipc_cb;
    uint8_t cnt = 0;
    tkl_ipc_init(&ipc_conf, __ipc_handle, &cnt);
    return 0;
}

