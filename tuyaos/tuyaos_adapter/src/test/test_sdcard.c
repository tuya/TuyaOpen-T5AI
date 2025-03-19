/*
 * test_sdcard.c
 * Copyright (C) 2025 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "cli.h"
#include "cli_tuya_test.h"
#include "tuya_cloud_types.h"

// #include "ff.h"         /* FatFS头文件 */
// #include "ffconf.h"     /* FatFS配置 */

#include <common/bk_include.h>
// #include "diskio.h"
#include "bk_posix.h"
#include "driver/sd_card_types.h"
#include "tkl_fs.h"

const char *mount_point = "/sdcard";

static void __cli_sdcard_mount(void)
{
    tkl_fs_mount(mount_point, DEV_SDCARD);
    return;
}

static void __cli_sdcard_umount(void)
{
    tkl_fs_unmount(mount_point);
}

static void __cli_sdcard_read(const char *path)
{
    char buf[64] = {'\0'};
    char fp[64] = {'\0'};

    if (path == NULL) {
        bk_printf("read failed, no file name spec\r\n");
        return;
    }

    sprintf(fp, "%s/%s", mount_point, path);

    TUYA_FILE f = tkl_fopen(fp, "r");
    if (f == NULL) {
        bk_printf("open %s failed\r\n", path);
        return;
    }

    tkl_fread(buf, 64, f);

    tkl_fclose(f);

    bk_printf("read: %s\r\n", buf);
}

static void __cli_sdcard_write(const char *path)
{
    if (path == NULL) {
        bk_printf("write failed, no file name spec\r\n");
        return;
    }
    char fp[64] = {'\0'};

    sprintf(fp, "%s/%s", mount_point, path);

    TUYA_FILE f = tkl_fopen(fp, "ab");
    if (f == NULL) {
        bk_printf("open %s failed\r\n", path);
        return;
    }

    char *str = "[abc123]";

    tkl_fwrite(str, strlen(str), f);

    tkl_fclose(f);
}

static void __cli_sdcard_list_file(const char* path)
{
    int res = 0, is_dir = 0;
    TUYA_DIR dir;
    TUYA_FILEINFO info;
    char *f_name = NULL;

    res = tkl_dir_open(path, &dir);                 /* Open the directory */
    if (res == 0) {
        bk_printf("%s\r\n", path);
        while (1) {
            res = tkl_dir_read(dir, &info);         /* Read a directory item */
            if (res != 0) {
                break;  /* Break on error */
            }

            tkl_dir_name(info, &f_name);

            if (f_name == NULL) {
                break;  /* Break on end of dir */
            }

            tkl_dir_is_directory(info, &is_dir);
            if (is_dir) {
                /* It is a directory */
                char *pathTemp = tkl_system_malloc(strlen(path)+strlen(f_name)+2);
                if(pathTemp == NULL) {
                    bk_printf("%s:os_malloc dir failed \r\n", __func__);
                    break;
                }
                sprintf(pathTemp, "%s/%s", path, f_name);
                __cli_sdcard_list_file(pathTemp);      /* Enter the directory */
                tkl_system_free(pathTemp);
            } else {
                /* It is a file. */
                bk_printf("%s/%s\r\n", path, f_name);
            }
        }
        tkl_dir_close(dir);
    } else {
        bk_printf("f_opendir failed\r\n");
    }

    return;
}

void cli_sdcard_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2) {
        bk_printf("invalid argc num\r\n");
        return;
    }

    if (!os_strcmp(argv[1], "init")) {
        bk_sd_card_init();
    } else if (!os_strcmp(argv[1], "deinit")) {
        bk_sd_card_deinit();
    } else if (!os_strcmp(argv[1], "mount")) {
        __cli_sdcard_mount();
    } else if (!os_strcmp(argv[1], "umount")) {
        __cli_sdcard_umount();
    } else if (!os_strcmp(argv[1], "ls")) {
        __cli_sdcard_list_file(mount_point);
    } else if (!os_strcmp(argv[1], "read")) {
        if (argv[2] == NULL) {
            bk_printf("no file name\r\n");
            return;
        }
        __cli_sdcard_read(argv[2]);
    } else if (!os_strcmp(argv[1], "write")) {
        if (argv[2] == NULL) {
            bk_printf("no file name\r\n");
            return;
        }
        __cli_sdcard_write(argv[2]);
    } else if (!os_strcmp(argv[1], "delete")) {
        bk_printf("TODO...\r\n");
    } else if (!os_strcmp(argv[1], "format")) {
        bk_printf("TODO...\r\n");
    } else if (!os_strcmp(argv[1], "xx")) {
        bk_sd_card_init();
        __cli_sdcard_mount();
        __cli_sdcard_list_file(mount_point);
        __cli_sdcard_umount();
        bk_sd_card_deinit();
    } else {

    }
}

