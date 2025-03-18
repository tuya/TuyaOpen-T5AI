/*
 * test_single_core.c
 * Copyright (C) 2025 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */


#include "sdkconfig.h"
#include "cli.h"
#include "cli_tuya_test.h"
#include "tkl_ipc.h"
#include "tkl_thread.h"

#if CONFIG_SYS_CPU1
static TKL_THREAD_HANDLE __tkl_test_thread = NULL;
static void tkl_mp3_test_thread(void *args)
{
    struct ipc_msg_s *msg = (struct ipc_msg_s *)args;

    bk_printf("--- trace cpu1 %s %d, %d %d\r\n", __func__, __LINE__, msg->type, msg->subtype);

    struct ipc_msg_param_s *param = NULL;
    param = (struct ipc_msg_param_s *)msg->req_param;
    int argc =  (uint32_t)param->p1;
    char **argv = (char **)param->p2;
    cli_aud_intf_mp3_play_test_cmd(NULL, 0, argc, argv);

    tkl_thread_release(__tkl_test_thread);
    __tkl_test_thread = NULL;
}

void tkl_test_ipc_func(struct ipc_msg_s *msg)
{
    int ret = 0;

    extern OPERATE_RET tuya_ipc_send_no_sync(struct ipc_msg_s *msg);
    msg->ret_value = 0;
    tuya_ipc_send_no_sync(msg);
    bk_printf("--- trace %s %d, %d %d\r\n", __func__, __LINE__, msg->type, msg->subtype);

    switch(msg->subtype) {
        case TKL_IPC_TYPE_TEST_MEDIA:
        {
            struct ipc_msg_param_s *param = NULL;
            param = (struct ipc_msg_param_s *)msg->req_param;
            int argc =  (uint32_t)param->p1;
            char **argv = (char **)param->p2;
            cli_tuya_media_cmd(NULL, 0, argc, argv);
        }
            break;

        case TKL_IPC_TYPE_TEST_SYSTEM_INFO:
        {
            struct ipc_msg_param_s *param = NULL;
            param = (struct ipc_msg_param_s *)msg->req_param;
            int argc =  (uint32_t)param->p1;
            char **argv = (char **)param->p2;
            // cli_tuya_system_cmd(NULL, NULL, argc, argv);
        }
            break;

        case TKL_IPC_TYPE_TEST_MP3:
        {
            if (__tkl_test_thread == NULL) {
                tkl_thread_create(&__tkl_test_thread, "test", 4096, 3, tkl_mp3_test_thread, (void *)msg);
            }
        }
            break;

        default:
            break;
    }

    return;
}
#endif

/////////////////////////////////////////////////////////////////////////////////

#if CONFIG_SYS_CPU0
static void __usage(void)
{
    bk_printf("USAGE:\r\n");
    bk_printf("     sc set [name]\r\n");
    bk_printf("     sc open lcd [0|1], last parameter: pipeline\r\n");
    bk_printf("     sc close lcd [0|1]\r\n");
    bk_printf("     sc open uvc [0|1]\r\n");
    bk_printf("     sc close uvc [0|1]\r\n");
    bk_printf("     sc open audio [0|1], last parameter: on board\r\n");
    bk_printf("     sc close audio [0|1]\r\n");
    bk_printf("     sc mp3 start xx.mp3\r\n");
}

static void __sc_set_lcd_name(int argc, char **argv)
{
    char *name = argv[2];
    char *p[3];
    char xmt_argv[3][16];
    int  xmt_argc = 3;

    struct ipc_msg_s msg = {0};
    struct ipc_msg_param_s param = {0};

    if (name == NULL) {
        bk_printf("parameter err %d\r\n", __LINE__);
        goto __err_set_name;
    }

    if (!os_strcmp(name, "st7701sn") &&
        !os_strcmp(name, "T50P181CQ") &&
        !os_strcmp(name, "T35P128CQ")) {
        bk_printf("parameter err %d\r\n", __LINE__);
        goto __err_set_name;
    }

    // xmt set T50P181CQ
    memcpy(&xmt_argv[0][0], "xmt", strlen("xmt") + 1);
    memcpy(&xmt_argv[1][0], "set", strlen("set") + 1);
    memcpy(&xmt_argv[2][0], name, strlen(name) + 1);

    p[0] = xmt_argv[0];
    p[1] = xmt_argv[1];
    p[2] = xmt_argv[2];

    param.p1 = (void *)xmt_argc;
    param.p2 = (void *)p;

    memset(&msg, 0, sizeof(struct ipc_msg_s));
    msg.type = TKL_IPC_TYPE_TEST;
    msg.subtype = TKL_IPC_TYPE_TEST_MEDIA;
    msg.req_param = &param;
    msg.req_len = sizeof(struct ipc_msg_param_s);
    OPERATE_RET ret = tuya_ipc_send_sync(&msg);
    if(ret) {
        bk_printf("send msg err %d\r\n", __LINE__);
        return;
    }

    return;

__err_set_name:
    __usage();
    return;
}

static void __sc_media_handle(int argc, char **argv, char *dev)
{
    char *p[4];
    char *opt = argv[1];
    char *pipeline = argv[3];
    char xmt_argv[4][16];
    int  xmt_argc = 4;

    struct ipc_msg_s msg = {0};
    struct ipc_msg_param_s param = {0};

    if (opt == NULL || pipeline == NULL || dev == NULL) {
        bk_printf("parameter err %d\r\n", __LINE__);
        goto __err_media;
    }

    if (!os_strcmp(opt, "open") && !os_strcmp(opt, "close")) {
        bk_printf("parameter err %d\r\n", __LINE__);
        goto __err_media;
    }

    if (!os_strcmp(pipeline, "0") && !os_strcmp(pipeline, "1") && os_strcmp(dev, "lvgl")) {
        bk_printf("parameter err %d\r\n", __LINE__);
        goto __err_media;
    }

    memset(xmt_argv, 0, sizeof(xmt_argv));

    // xmt open lcd 1
    memcpy(&xmt_argv[0][0], "xmt", strlen("xmt") + 1);
    memcpy(&xmt_argv[1][0], opt, strlen(opt) + 1);
    memcpy(&xmt_argv[2][0], dev, strlen(dev) + 1);
    memcpy(&xmt_argv[3][0], pipeline, strlen(pipeline) + 1);

    p[0] = xmt_argv[0];
    p[1] = xmt_argv[1];
    p[2] = xmt_argv[2];
    p[3] = xmt_argv[3];

    param.p1 = (void *)xmt_argc;
    param.p2 = (void *)p;

    memset(&msg, 0, sizeof(struct ipc_msg_s));
    msg.type = TKL_IPC_TYPE_TEST;
    msg.subtype = TKL_IPC_TYPE_TEST_MEDIA;
    msg.req_param = &param;
    msg.req_len = sizeof(struct ipc_msg_param_s);
    OPERATE_RET ret = tuya_ipc_send_sync(&msg);
    if(ret) {
        bk_printf("send msg err %d\r\n", __LINE__);
        return;
    }

    return;

__err_media:
    __usage();
    return;
}

static void __sc_mp3_handle(int argc, char **argv)
{
    // xmp3 open/close xx.mp3
    // sc mp3 start xx.mp3
    char *opt = argv[2];
    char *fname = argv[3];
    char *p[3];
    char xmt_argv[3][16];
    int  xmt_argc = 3;

    struct ipc_msg_s msg = {0};
    struct ipc_msg_param_s param = {0};

    if (fname == NULL || opt == NULL) {
        bk_printf("parameter err %d\r\n", __LINE__);
        goto __err_mp3;
    }

    // xmt set T50P181CQ
    memcpy(&xmt_argv[0][0], "xmp3", strlen("xmp3") + 1);
    memcpy(&xmt_argv[1][0], opt, strlen(opt) + 1);
    memcpy(&xmt_argv[2][0], fname, strlen(fname) + 1);

    p[0] = xmt_argv[0];
    p[1] = xmt_argv[1];
    p[2] = xmt_argv[2];

    param.p1 = (void *)xmt_argc;
    param.p2 = (void *)p;

    memset(&msg, 0, sizeof(struct ipc_msg_s));
    msg.type = TKL_IPC_TYPE_TEST;
    msg.subtype = TKL_IPC_TYPE_TEST_MP3;
    msg.req_param = &param;
    msg.req_len = sizeof(struct ipc_msg_param_s);
    os_printf("--- trace %s %d\r\n", __func__, __LINE__);
    OPERATE_RET ret = tuya_ipc_send_sync(&msg);
    os_printf("--- trace %s %d\r\n", __func__, __LINE__);
    if(ret) {
        bk_printf("send msg err %d\r\n", __LINE__);
        return;
    }

    return;

__err_mp3:
    __usage();
    return;
}

void cli_sc_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 3) {
        __usage();
        return;
    }

    if (!os_strcmp(argv[1], "set")) {
        __sc_set_lcd_name(argc, argv);
    } else if (!os_strcmp(argv[2], "lcd")) {
        __sc_media_handle(argc, argv, "lcd");
    } else if (!os_strcmp(argv[2], "uvc")) {
        __sc_media_handle(argc, argv, "uvc");
    } else if (!os_strcmp(argv[2], "lvgl")) {
        __sc_media_handle(argc, argv, "lvgl");
    } else if (!os_strcmp(argv[2], "audio")) {
        __sc_media_handle(argc, argv, "audio");
    } else if (!os_strcmp(argv[1], "mp3")) {
        __sc_mp3_handle(argc, argv);
    } else {
        __usage();
    }
}

#endif // CONFIG_SYS_CPU0

