/**
 * @file tkl_fs.c
 * @brief the default weak implements of tuya os file system, this implement only used when OS=linux
 * @version 0.1
 * @date 2019-08-15
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */
#include <errno.h>
#include "tkl_fs.h"
#ifdef CONFIG_VFS
#include "bk_posix.h"
#include "driver/flash_partition.h"
#endif
#include "tkl_output.h"
#include "sdkconfig.h"
#include "tkl_ipc.h"
#include "tkl_semaphore.h"

extern void bk_printf(const char *fmt, ...);

#define FILE_HANDLE_OFFSET 0x100


#if !CONFIG_SYS_CPU0
TKL_SEM_HANDLE thread_fs_access_sem = NULL;
TKL_SEM_HANDLE fs_api_access_sem = NULL;
uint32_t fs_result[2];

void tkl_fs_init(void)
{
    fs_result[0] = 0;
    fs_result[1] = 0;
    tkl_semaphore_create_init(&thread_fs_access_sem, 1, 1);
    tkl_semaphore_create_init(&fs_api_access_sem, 0, 1);
}

// IN Parameter: int type, uint8_t *data, uint32_t len
// OUT Parameter: result
extern TKL_IPC_HANDLE __ipc_handle[2];
static void __tkl_fs_client(struct ipc_msg_s *msg, int *result)
{
    OPERATE_RET ret = tkl_semaphore_wait(thread_fs_access_sem, 5000);
    if (ret != OPRT_OK) {
        if (result != NULL)
            result[0] = -1;
        bk_printf("error %s %d\n", __func__, __LINE__);
        return;
    }

    msg->len = sizeof(msg->buf);
    tkl_ipc_send_no_sync(__ipc_handle[0], msg, sizeof(struct ipc_msg_s));

    // wait ack
    tkl_semaphore_wait(fs_api_access_sem, 5000);
    result[0] = fs_result[0];
    result[1] = fs_result[1];
    tkl_semaphore_post(thread_fs_access_sem);
}
#endif // CONFIG_SYS_CPU1

/**
 * @brief Make directory
 *
 * @param[in] path: path of directory
 *
 * @note This API is used for making a directory
 *
 * @return 0 on success. Others on failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_fs_mkdir(const char *path)
{
#if CONFIG_SYS_CPU0
    int ret = mkdir(path,0777);
    if (ret && ret != -EEXIST) {
        bk_printf("tkl_fs_mkdir failed, path:%s ret =%d errno=%d\n ", path,ret,errno);
        return -1;
    }
    return 0;
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_MKDIR;
    memset(msg.buf, 0, sizeof(msg.buf));
    memcpy(msg.buf, path, strlen(path));
    msg.len = strlen(path);
    __tkl_fs_client(&msg, ret);
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief Remove directory
 *
 * @param[in] path: path of directory
 *
 * @note This API is used for removing a directory
 *
 * @return 0 on success. Others on failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_fs_remove(const char *path)
{
#if CONFIG_SYS_CPU0
    struct stat path_stat;
    int ret = 0;
    if (stat(path, &path_stat) != 0) {
        bk_printf("tkl_fs_remove stat error\n");
        return -1;
    }

    if (S_ISREG(path_stat.st_mode)) {
        // bk_printf("tkl_fs_remove %s is a regular file.\n", path);
        ret = unlink(path);
        if (ret != 0) {
            bk_printf("tkl_fs_remove unlink failed, path:%s\n", path);
        }
    } else if (S_ISDIR(path_stat.st_mode)) {
        // bk_printf("tkl_fs_remove %s is a directory.\n", path);
        ret =  rmdir(path);
        if (ret != 0) {
            bk_printf("tkl_fs_remove rmdir failed, path:%s\n", path);
        }
    } else {
        // bk_printf("tkl_fs_remove %s is neither a regular file nor a directory.\n", path);
        return -1;
    }
    // bk_printf("tkl_fs_remove %s %s\n", path, ret == 0 ? "success" : "failed");

    return ret;
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_REMOVE;
    memset(msg.buf, 0, sizeof(msg.buf));
    memcpy(msg.buf, path, strlen(path));
    msg.len = strlen(path);
    __tkl_fs_client(&msg, ret);
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief Get file mode
 *
 * @param[in] path: path of directory
 * @param[out] mode: bit attibute of directory
 *
 * @note This API is used for getting file mode.
 *
 * @return 0 on success. Others on failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_fs_mode(const char *path, uint32_t *mode)
{
#if CONFIG_SYS_CPU0
    if (mode == NULL) {
        return -1;
    }

    struct stat statbuf = { 0 };
    int ret             = stat(path, &statbuf);
    if (ret != 0) {
        return ret;
    }

    *mode = statbuf.st_mode;
    return 0;
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_MODE;
    memset(msg.buf, 0, sizeof(msg.buf));
    memcpy(msg.buf, path, strlen(path));
    msg.len = strlen(path);
    __tkl_fs_client(&msg, ret);
    *mode = ret[1];
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief Check whether the file or directory exists
 *
 * @param[in] path: path of directory
 * @param[out] is_exist: the file or directory exists or not
 *
 * @note This API is used to check whether the file or directory exists.
 *
 * @return 0 on success. Others on failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_fs_is_exist(const char *path, BOOL_T *is_exist)
{
#if CONFIG_SYS_CPU0
    if (is_exist == NULL) {
        return -1;
    }

    struct stat statbuf = { 0 };
    int ret             = stat(path, &statbuf);
    if (ret != 0) {
        *is_exist = FALSE;
        bk_printf("tkl_fs_is_exist path:%s stat failed \r\n",path);
        return ret;
    }
    else {
        bk_printf("tkl_fs_is_exist path:%s exist \r\n",path);
    }

    *is_exist = TRUE;
    return 0;
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_IS_EXIST;
    memset(msg.buf, 0, sizeof(msg.buf));
    memcpy(msg.buf, path, strlen(path));
    msg.len = strlen(path);
    __tkl_fs_client(&msg, ret);
    *is_exist = ret[1];
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief File rename
 *
 * @param[in] path_old: old path of directory
 * @param[in] path_new: new path of directory
 *
 * @note This API is used to rename the file.
 *
 * @return 0 on success. Others on failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_fs_rename(const char *path_old, const char *path_new)
{
#if CONFIG_SYS_CPU0
    return rename(path_old, path_new);
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_RENAME;
    memset(msg.buf, 0, sizeof(msg.buf));
    memcpy(msg.buf, path_old, strlen(path_old));
    memcpy(msg.buf+strlen(path_old)+1, path_new, strlen(path_new));
    msg.len = strlen(strlen(path_old)+1+strlen(path_new));
    __tkl_fs_client(&msg, ret);
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief Open directory
 *
 * @param[in] path: path of directory
 * @param[out] dir: handle of directory
 *
 * @note This API is used to open a directory
 *
 * @return 0 on success. Others on failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_dir_open(const char *path, TUYA_DIR *dir)
{
#if CONFIG_SYS_CPU0
    if (dir == NULL) {
        return -1;
    }

    DIR *d = opendir(path);
    if (d == NULL) {
        bk_printf("tkl_dir_open failed, path:%s\n", path);
        return -1;
    }
    // bk_printf("tkl_dir_open success, path:%s\n", path);

    *dir = d;
    return 0;
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_DIR_OPEN;
    memset(msg.buf, 0, sizeof(msg.buf));
    memcpy(msg.buf, path, strlen(path));
    msg.len = strlen(path);
    __tkl_fs_client(&msg, ret);
    *dir = ret[1];
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief Close directory
 *
 * @param[in] dir: handle of directory
 *
 * @note This API is used to close a directory
 *
 * @return 0 on success. Others on failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_dir_close(TUYA_DIR dir)
{
#if CONFIG_SYS_CPU0
    DIR *dirp = (DIR *)dir;
    int ret =  closedir(dirp);
    if (ret < 0) {
        bk_printf("tkl_dir_close failed\n");
    }
    else {
        bk_printf("tkl_dir_close success\n");
    }
    return ret;
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_DIR_CLOSE;
    msg.buf32[0] = (uint32_t)dir;
    msg.len = sizeof(uint32_t);
    __tkl_fs_client(&msg, ret);
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief Read directory
 *
 * @param[in] dir: handle of directory
 * @param[out] info: file information
 *
 * @note This API is used to read a directory.
 * Read the file information of the current node, and the internal pointer points to the next node.
 *
 * @return 0 on success. Others on failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_dir_read(TUYA_DIR dir, TUYA_FILEINFO *info)
{
#if CONFIG_SYS_CPU0
    if (info == NULL) {
        return -1;
    }

    DIR *dirp         = (DIR *)dir;
    struct dirent *dp = readdir(dirp);
    if (dp == NULL) {
        return -1;
    }

    *info = dp;
    return 0;
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_DIR_READ;
    msg.buf32[0] = (uint32_t)dir;
    msg.len = sizeof(uint32_t);
    __tkl_fs_client(&msg, ret);
    *info = ret[1];
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief Get the name of the file node
 *
 * @param[in] info: file information
 * @param[out] name: file name
 *
 * @note This API is used to get the name of the file node.
 *
 * @return 0 on success. Others on failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_dir_name(TUYA_FILEINFO info, const char **name)
{
    if (name == NULL) {
        return -1;
    }

    struct dirent *dp = (struct dirent *)info;
    if (dp == NULL) {
        return -1;
    }

    *name = dp->d_name;

    bk_printf("tkl_dir_name name:%s\r\n", *name);
    return 0;
}

/**
 * @brief Check whether the node is a directory
 *
 * @param[in] info: file information
 * @param[out] is_dir: is directory or not
 *
 * @note This API is used to check whether the node is a directory.
 *
 * @return 0 on success. Others on failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_dir_is_directory(TUYA_FILEINFO info, BOOL_T *is_dir)
{
    if (is_dir == NULL) {
        return -1;
    }

    struct dirent *dp = (struct dirent *)info;
    *is_dir           = (dp->d_type == DT_DIR);
    return 0;
}

/**
 * @brief Check whether the node is a normal file
 *
 * @param[in] info: file information
 * @param[out] is_regular: is normal file or not
 *
 * @note This API is used to check whether the node is a normal file.
 *
 * @return 0 on success. Others on failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_dir_is_regular(TUYA_FILEINFO info, BOOL_T *is_regular)
{
    if (is_regular == NULL) {
        return -1;
    }

    struct dirent *dp = (struct dirent *)info;
    *is_regular       = (dp->d_type == DT_REG);
    return 0;
}

/**
 * @brief Open file
 *
 * @param[in] path: path of file
 * @param[in] mode: file open mode: "r","w"...
 *
 * @note This API is used to open a file
 *
 * @return the file handle, NULL means failed
 */
TUYA_WEAK_ATTRIBUTE TUYA_FILE tkl_fopen(const char *path, const char *mode)
{
#if CONFIG_SYS_CPU0
    int fd = 0;
    int flags;

    if (path == NULL || mode == NULL)
        return NULL;
    // 解析模式字符串
    if (strcmp(mode, "r") == 0) {
        flags = O_RDONLY;
    } else if (strcmp(mode, "w") == 0) {
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (strcmp(mode, "a") == 0) {
        flags = O_WRONLY | O_CREAT | O_APPEND;
    } else if (strcmp(mode, "rb") == 0) {
        flags = O_RDONLY;
    } else if (strcmp(mode, "wb") == 0) {
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (strcmp(mode, "ab") == 0) {
        flags = O_WRONLY | O_CREAT | O_APPEND;
    } else if (strcmp(mode, "r+") == 0 || strcmp(mode, "rb+") == 0 || strcmp(mode, "r+b") == 0) {
        flags = O_RDWR;
    } else if (strcmp(mode, "w+") == 0 || strcmp(mode, "wb+") == 0 || strcmp(mode, "w+b") == 0) {
        flags = O_RDWR | O_CREAT | O_TRUNC;
    } else if (strcmp(mode, "a+") == 0 || strcmp(mode, "ab+") == 0 || strcmp(mode, "a+b") == 0) {
        flags = O_RDWR | O_CREAT | O_APPEND;
    } else {
        // 不支持的模式
        bk_printf("mode not support, file path:%s mode: %s\n",path,mode);
        return NULL;
    }

    fd = open(path, flags);
    if (fd < 0) {
        bk_printf("tkl_fopen file failed, path:%s\n", path);
        return NULL;
    }
    else {
        bk_printf("tkl_fopen file success, path:%s fd = %d\n",path,fd);
    }
    return (TUYA_FILE)(fd + FILE_HANDLE_OFFSET);
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    memset(&msg, 0, sizeof(struct ipc_msg_s));
    msg.type = TKL_IPC_TYPE_FS_FOPEN;
    memset(msg.buf, 0, sizeof(msg.buf));
    memcpy(msg.buf, path, strlen(path));
    memcpy(msg.buf + strlen(path) + 1, (uint8_t *)mode, strlen(mode));
    msg.len = strlen(path)+1+strlen(mode);
    __tkl_fs_client(&msg, ret);
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief Close file
 *
 * @param[in] file: file handle
 *
 * @note This API is used to close a file
 *
 * @return 0 on success. EOF on failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_fclose(TUYA_FILE file)
{
#if CONFIG_SYS_CPU0
    int fd;
    int ret;

    fd = (int)file;
    fd -= FILE_HANDLE_OFFSET;
    if (fd < 0) {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
    ret = close(fd);
    if (ret < 0) {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
    // bk_printf("tkl_fclose file %d\n", fd);
    return OPRT_OK;
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_FCLOSE;
    msg.buf32[0] = (uint32_t)file;
    msg.len = sizeof(TUYA_FILE);
    __tkl_fs_client(&msg, ret);
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief Read file
 *
 * @param[in] buf: buffer for reading file
 * @param[in] bytes: buffer size
 * @param[in] file: file handle
 *
 * @note This API is used to read a file
 *
 * @return the bytes read from file
 */
TUYA_WEAK_ATTRIBUTE int tkl_fread(void *buf, int bytes, TUYA_FILE file)
{
#if CONFIG_SYS_CPU0
    int fd = (int)file;
    fd -= FILE_HANDLE_OFFSET;
    if (fd < 0) {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    int ret =  read(fd, buf, bytes);
    // bk_printf("tkl_fread fd: %d size: %d bytes = %d\n", fd,ret,bytes);
    return ret;
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_FREAD;
    msg.buf32[0] = (uint32_t)buf;
    msg.buf32[1] = (uint32_t)bytes;
    msg.buf32[2] = (uint32_t)file;
    __tkl_fs_client(&msg, ret);
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief write file
 *
 * @param[in] buf: buffer for writing file
 * @param[in] bytes: buffer size
 * @param[in] file: file handle
 *
 * @note This API is used to write a file
 *
 * @return the bytes write to file
 */
TUYA_WEAK_ATTRIBUTE int tkl_fwrite(void *buf, int bytes, TUYA_FILE file)
{
#if CONFIG_SYS_CPU0
    int fd = (int)file;
    fd -= FILE_HANDLE_OFFSET;
    if (fd < 0) {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
    // bk_printf("begin tkl_fwrite: fd=%d, bytes=%d\n", fd, bytes);

    return write(fd, buf, bytes);
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_FWRITE;
    msg.buf32[0] = (uint32_t)buf;
    msg.buf32[1] = (uint32_t)bytes;
    msg.buf32[2] = (uint32_t)file;
    __tkl_fs_client(&msg, ret);
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief write buffer to flash
 *
 * @param[in] fd: file fd
 *
 * @note This API is used to write buffer to flash
 *
 * @return 0 on success. others on failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_fsync(int fd)
{
#if CONFIG_SYS_CPU0
    int ret;

    fd -= FILE_HANDLE_OFFSET;
    ret = fsync(fd);
    if (ret < 0) {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
    return OPRT_OK;
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_FSYNC;
    msg.buf32[0] = (uint32_t)fd;
    msg.len = sizeof(uint32_t);
    __tkl_fs_client(&msg, ret);
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief Read string from file
 *
 * @param[in] buf: buffer for reading file
 * @param[in] len: buffer size
 * @param[in] file: file handle
 *
 * @note This API is used to read string from file
 *
 * @return the content get from file, NULL means failed
 */
TUYA_WEAK_ATTRIBUTE char *tkl_fgets(char *buf, int len, TUYA_FILE file)
{
#if CONFIG_SYS_CPU0
    if (len <= 0 || buf == NULL) {
        return NULL;
    }
    int fd = (int)file;
    fd -= FILE_HANDLE_OFFSET;
    if (fd < 0) {
        return NULL;
    }
    int i = 0;
    char ch;
    ssize_t bytes_read = 0;

    while (i < len - 1) {
        bytes_read = read(fd, &ch, 1);
        if (bytes_read == -1) {
            bk_printf("tkl_fgets read failed \r\n");
            return NULL;
        } else if (bytes_read == 0) {
            // End of file
            break;
        }

        buf[i++] = ch;

        if (ch == '\n') {
            break;
        }
    }

    buf[i] = '\0';

    if (i == 0 && bytes_read == 0) {
        // If no characters were read and EOF was reached, return NULL
        return NULL;
    }

    return buf;
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_FGETS;
    msg.buf32[0] = (uint32_t)buf;
    msg.buf32[1] = (uint32_t)len;
    msg.buf32[2] = (uint32_t)file;
    msg.len = 3*sizeof(uint32_t);
    __tkl_fs_client(&msg, ret);
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief Check wheather to reach the end fo the file
 *
 * @param[in] file: file handle
 *
 * @note This API is used to check wheather to reach the end fo the file
 *
 * @return 0 on not eof, others on eof
 */
TUYA_WEAK_ATTRIBUTE int tkl_feof(TUYA_FILE file)
{
#if CONFIG_SYS_CPU0
    int fd;

    fd = (int)file;
    fd -= FILE_HANDLE_OFFSET;
    if (fd < 0) {
        return OPRT_OS_ADAPTER_COM_ERROR;
    }
    // 获取当前文件指针位置
    int current_pos = lseek(fd, 0, SEEK_CUR);
    if (current_pos == (int)-1) {
        return -1;  // 错误：获取当前文件指针位置失败
    }

    // 获取文件大小
    int file_size = lseek(fd, 0, SEEK_END);
    if (file_size == (int)-1) {
        return -1;  // 错误：获取文件大小失败
    }

    // 恢复文件指针到原来的位置
    int restore_pos = lseek(fd, current_pos, SEEK_SET);
    if (restore_pos == (int)-1) {
        return -1;  // 错误：恢复文件指针位置失败
    }

    // 判断是否到达文件末尾
    return current_pos >= file_size;
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_FEOF;
    msg.buf32[0] = (uint32_t)file;
    msg.len = sizeof(uint32_t);
    __tkl_fs_client(&msg, ret);
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief Seek to the offset position of the file
 *
 * @param[in] file: file handle
 * @param[in] offs: offset
 * @param[in] whence: seek start point mode
 *
 * @note This API is used to seek to the offset position of the file.
 *
 * @return 0 on success, others on failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_fseek(TUYA_FILE file, INT64_T offs, int whence)
{
#if CONFIG_SYS_CPU0
    int fd = (int)file;
    fd -= FILE_HANDLE_OFFSET;
    // bk_printf("begin to tkl_fseek: fd=%d, offs=%ld, whence=%d\n", fd, offs, whence);
    int result = lseek(fd, offs, whence);
    if (result < 0) {
        return -1;
    }
    return 0;
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_FSEEK;
    msg.buf32[0] = (uint32_t)file;
    msg.buf32[1] = (uint32_t)((offs >> 32) & 0xffffffff);
    msg.buf32[2] = (uint32_t)(offs & 0xffffffff);
    msg.buf32[3] = (uint32_t)whence;
    msg.len = 4*sizeof(uint32_t);
    __tkl_fs_client(&msg, ret);
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief Get current position of file
 *
 * @param[in] file: file handle
 *
 * @note This API is used to get current position of file.
 *
 * @return the current offset of the file
 */
TUYA_WEAK_ATTRIBUTE INT64_T tkl_ftell(TUYA_FILE file)
{
#if CONFIG_SYS_CPU0
    int fd = (int)file;
    fd -= FILE_HANDLE_OFFSET;
    off_t pos = lseek(fd, 0, SEEK_CUR);
    if (pos == (off_t)-1) {
        return -1;
    }
    return pos;
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_FTELL;
    msg.buf32[0] = (uint32_t)file;
    msg.len = sizeof(uint32_t);
    __tkl_fs_client(&msg, ret);
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief Get file size
 *
 * @param[in] filepath file path + file name
 *
 * @note This API is used to get the size of file.
 *
 * @return the sizeof of file
 */
TUYA_WEAK_ATTRIBUTE int tkl_fgetsize(const char *filepath)
{
#if CONFIG_SYS_CPU0
    struct stat statbuf;
    stat(filepath, &statbuf);
    int size = statbuf.st_size;

    return size;
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_FGETSIZE;
    memset(msg.buf, 0, sizeof(msg.buf));
    memcpy(msg.buf, filepath, strlen(filepath));
    msg.len = strlen(filepath);
    __tkl_fs_client(&msg, ret);
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief Judge if the file can be access
 *
 * @param[in] filepath file path + file name
 *
 * @param[in] mode access mode
 *
 * @note This API is used to access one file.
 *
 * @return 0 success,-1 failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_faccess(const char *filepath, int mode)
{
#if CONFIG_SYS_CPU0
    struct stat st;
    if (stat(filepath, &st) != 0) {
        // 如果 stat 失败，返回错误
        return -1;
    }

    // 检查文件存在性
    if (mode == F_OK) {
        return 0;
    }
    // 检查其他用户权限
    if ((mode & R_OK) && !(st.st_mode & S_IROTH)) {
        return -1;
    }
    if ((mode & W_OK) && !(st.st_mode & S_IWOTH)) {
        return -1;
    }
    if ((mode & X_OK) && !(st.st_mode & S_IXOTH)) {
        return -1;
    }

    return 0;
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_FACCESS;
    memset(msg.buf, 0, sizeof(msg.buf));
    memcpy(msg.buf, filepath, strlen(filepath));
    memcpy(msg.buf+strlen(filepath)+1, (uint8_t *)&mode, sizeof(int));
    msg.len = strlen(filepath)+1+sizeof(int);
    __tkl_fs_client(&msg, ret);
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief read the next character from stream
 *
 * @param[in] file char stream
 *
 * @note This API is used to get one char from stream.
 *
 * @return as an unsigned char cast to a int ,or EOF on end of file or error
 */
TUYA_WEAK_ATTRIBUTE int tkl_fgetc(TUYA_FILE file)
{
#if CONFIG_SYS_CPU0
    unsigned char ch;
    ssize_t ret = tkl_fread(&ch, 1, file);
    if (ret == 1) {
        return ch;
    } else {
        return -1;
    }
#else // client
    int ret[2] = {0, 0};
    struct ipc_msg_s msg;
    msg.type = TKL_IPC_TYPE_FS_FGETC;
    msg.buf32[0] = (uint32_t)file;
    msg.len = sizeof(uint32_t);
    __tkl_fs_client(&msg, ret);
    return ret[0];
#endif // CONFIG_SYS_CPU0
}

/**
 * @brief flush the IO read/write stream
 *
 * @param[in] file char stream
 *
 * @note This API is used to flush the IO read/write stream.
 *
 * @return 0 success,-1 failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_fflush(TUYA_FILE file)
{
    return 0;
}

/**
 * @brief get the file fd
 *
 * @param[in] file char stream
 *
 * @note This API is used to get the file fd.
 *
 * @return the file fd
 */
TUYA_WEAK_ATTRIBUTE int tkl_fileno(TUYA_FILE file)
{
    int fd = (int)file;
    return fd;
}

/**
 * @brief truncate one file according to the length
 *
 * @param[in] fd file description
 *
 * @param[in] length the length want to truncate
 *
 * @note This API is used to truncate one file.
 *
 * @return 0 success,-1 failed
 */
TUYA_WEAK_ATTRIBUTE int tkl_ftruncate(int fd, UINT64_T length)
{
#if CONFIG_SYS_CPU0
    int plat_fd = (int)fd;
    plat_fd -= FILE_HANDLE_OFFSET;
    return ftruncate(plat_fd, length);
#else // client
    // Not support
    return -1;
#endif // CONFIG_SYS_CPU0
}

/**
* @brief mount file system
*
* @param[in] mount point
*
* @param[in] device type fs based
*
* @note This API is used to mount file system.
*
* @return 0 success,-1 failed
*/
extern int fatfs_mount(const char *mount_path, int type);
extern int littlefs_mount(const char *mount_path, int type);
int tkl_fs_mount(const char *path, FS_DEV_TYPE_T dev_type)
{
    int ret = 0;

    if (path == NULL)
        return -1;

    switch (dev_type) {
        case DEV_INNER_FLASH:
            ret = littlefs_mount(path, LFS_FLASH);
            break;
        case DEV_EXT_FLASH:
            ret = littlefs_mount(path, LFS_QSPI_FLASH);
            break;
        case DEV_SDCARD:
            ret = fatfs_mount(path, FATFS_DEVICE);
            break;
        default:
            return -1;
    }
    return ret;
}

/**
* @brief unmount file system
*
* @param[in] mount point
*
* @note This API is used to unmount file system.
*
* @return 0 success,-1 failed
*/
int tkl_fs_unmount(const char *path)
{
    return umount(path);
}


