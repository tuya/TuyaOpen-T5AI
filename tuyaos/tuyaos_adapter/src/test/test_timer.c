/*
 * test_timer.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "tkl_timer.h"
#include "tuya_cloud_types.h"
#include "tuya_error_code.h"

#if 0
#define DELAY_TIME    500    //us
static uint32_t timer_id = 1;

static char sg_count = 0;

static void __timer_callback(void *args)
{
    /* TAL_PR_ , PR_ ，这两种打印里面有锁，不要在中断里使用 */
    bk_printf("\r\n------------- Timer Callback --------------\r\n");
    sg_count++;

    if(sg_count >= 5) {
        sg_count=0;
        tkl_timer_stop(timer_id);
        tkl_timer_deinit(timer_id);
        bk_printf("\r\ntimer %d is stop\r\n", timer_id);
    }

    return;
}

void cli_tkl_timer_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_printf("argc: %d\r\n cmd: ", argc);
    for (int i = 0; i < argc; i++) {
        bk_printf("%s ", argv[i]);
    }
    bk_printf("\r\n");

    timer_id = os_strtoul(argv[1], NULL, 10);

    OPERATE_RET ret = OPRT_OK;
    TUYA_TIMER_BASE_CFG_T sg_timer_cfg = {
        .mode = TUYA_TIMER_MODE_PERIOD,
        .args = NULL,
        .cb = __timer_callback
    };

    bk_printf("init timer %d\r\n", timer_id);
    ret = tkl_timer_init(timer_id, &sg_timer_cfg);
    if (OPRT_OK != ret) {
        bk_printf("init timer %d error\r\n", timer_id);
        goto __EXIT;
    }

    /*start timer*/
    bk_printf("start timer %d\r\n", timer_id);
    ret = tkl_timer_start(timer_id, DELAY_TIME);
    if (OPRT_OK != ret) {
        bk_printf("start timer %d error\r\n", timer_id);
        goto __EXIT;
    }

    bk_printf("~~~~~~~~~~~~~~~~~~~~\r\n");
    return;

__EXIT:
    bk_printf("xxxxxxxxxxxxxxxxxxxx\r\n");
    return;
}
#else

#include <os/os.h>
#include "cli.h"
#include <driver/timer.h>
#include "bk_misc.h"

static void cli_timer_isr(timer_id_t chan)
{
    bk_printf("[TIMER][ISR] chan:%d\r\n", chan);
}

static void cli_delay_us(int argc, char **argv)
{
	uint32_t us = 0;

	if (argc < 3) {
		bk_printf("timer delay [count]\r\n");
		return;
	}

	us = os_strtoul(argv[2], NULL, 10);
	uint32_t level = rtos_enter_critical();
	delay_us(us);
	rtos_exit_critical(level);
	bk_printf("delayed %u us\r\n", us);
}

void cli_timer_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int ret = 0;
    uint8_t channel = 0;
    uint32_t time_ms = 0, read_cnt = 0;

    if (os_strcmp(argv[1], "delay") == 0) {
        cli_delay_us(argc, argv);
    } else if (os_strcmp(argv[1], "init") == 0) {
        BK_LOG_ON_ERR(bk_timer_driver_init());
        return;
    }else {
        channel = os_strtoul(argv[1], NULL, 10);
    }

    if (os_strcmp(argv[2], "start") == 0) {
        time_ms = os_strtoul(argv[3], NULL, 10);
        bk_printf("[TIMER][START] channel:%d, time_ms:%d\r\n", channel, time_ms);
        ret = bk_timer_start(channel, time_ms, cli_timer_isr);
        if (ret != BK_OK) {
            bk_printf("[TIMER][START] start failed, error code:%x\n", ret);
        }
    } else if (os_strcmp(argv[2], "stop") == 0) {
        bk_printf("[TIMER][STOP] channel:%d\r\n", channel);
        bk_timer_stop(channel);
    } else if (os_strcmp(argv[2], "read") == 0) {
        read_cnt = bk_timer_get_cnt(channel);
        bk_printf("[TIMER][READ] read cnt:%x\r\n", read_cnt);
    } else if (os_strcmp(argv[2], "enable") == 0) {
        bk_timer_enable(channel);
        bk_printf("[TIMER][ENABLE] channel:%x\r\n", channel);
    } else if (os_strcmp(argv[2], "disable") == 0) {
        bk_timer_disable(channel);
        bk_printf("[TIMER][DISABLE] channel:%x\r\n", channel);
    } else if (os_strcmp(argv[2], "get_period") == 0) {
        read_cnt = bk_timer_get_period(channel);
        bk_printf("[TIMER][GET][PERIOD] period value:%x\r\n", read_cnt);
    } else {
        bk_printf("timer {chan} {start|stop|read} [...]\r\n");
    }
}

#define TIMER_CMD_CNT (sizeof(s_timer_commands) / sizeof(struct cli_command))
static const struct cli_command s_timer_commands[] = {
    {"timer", "timer {chan} {start|stop|read} [...]", cli_timer_cmd},
};

int cli_timer_init(void)
{
    BK_LOG_ON_ERR(bk_timer_driver_init());
    return cli_register_commands(s_timer_commands, TIMER_CMD_CNT);
}

#endif
