/*
 * test_pwm.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "cli.h"
#include "cli_tuya_test.h"

#define CONFIG_BK7258_PWM_ENABLE 1

#if CONFIG_BK7258_PWM_ENABLE

static void pwm_simple_demo(void *param);

#include "driver/pwm_types.h"
struct pwm_test_s {
    uint32_t ch;
    uint32_t is_running;
    uint32_t is_init;
    uint32_t freq;
    uint32_t duty;
    pwm_init_config_t conf;
};
#define MAX_PWM_CHAN    6
static struct pwm_test_s *pconf = NULL;
static void __pwm_usage(void)
{
    bk_printf("usage: xpwm set [chan] [freq] [duty]\r\n");
    bk_printf("       xpwm start/stop [chan]\r\n");
    bk_printf("       xpwm show/free/help\r\n");
    bk_printf("chan rank: [18,24,32,34,36]\r\n");
    bk_printf("freq: 1 - 10000 Hz\r\n");
    bk_printf("duty: 0 - 100 %%\r\n");
    bk_printf("default chan 18, freq 1000, duty f*0.25\r\n");
}
static int __get_free_pwm(uint32_t ch)
{
    int i;
    // first find if channel in used
    for (i = 0; i < MAX_PWM_CHAN; i++) {
        if (pconf[i].ch == ch) {
            return i;
        }
    }
    // new channel, record
    for (i = 0; i < MAX_PWM_CHAN; i++) {
        if (pconf[i].ch == 0) {
            return i;
        }
    }

    return 0xff;
}
static inline int __chan_check(uint32_t ch)
{
    switch(ch) {
        case 18:
        case 24:
        case 32:
        case 34:
        case 36:
            return 1;
        default:
            return 0;
    }
}
static inline int __freq_check(uint32_t freq)
{
    return (freq < 1 || freq > 10000)? 0: 1;
}
static inline int __duty_check(uint32_t duty)
{
    return (duty > 100)? 0: 1;
}
static inline void __chan_dump(struct pwm_test_s *p, char *tag)
{
    if (tag != NULL)
        bk_printf("ch: %d, freq: %d, duty: %d%%, period: %d, htime: %d, %s\r\n",
                p->ch, p->freq, p->duty, p->conf.period_cycle, p->conf.duty_cycle, tag);
    else
        bk_printf("ch: %d, freq: %d, duty: %d%%, period: %d, htime: %d\r\n",
                p->ch, p->freq, p->duty, p->conf.period_cycle, p->conf.duty_cycle);
}
#define __PWM_FREQ2PERIOD(x) ((unsigned int)((26000000.0/x)))
static inline __pwm_channel_remap(uint32_t ch)
{
    switch(ch) {
        case 18:
            return 0;
        case 24:
            return 4;
        case 32:
            return 6;
        case 34:
            return 8;
        case 36:
            return 10;
        default:
            return 0;
    }
}

void cli_pwm_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2 || argc > 5) {
        __pwm_usage();
        return;
    }

    struct pwm_test_s pwm;
    memset(&pwm, 0, sizeof(struct pwm_test_s));

    if (pconf == NULL) {
        pconf = (struct pwm_test_s *)tkl_system_malloc(MAX_PWM_CHAN * sizeof(struct pwm_test_s));
        if (pconf == NULL) {
            bk_printf("malloc for pwm test failed\r\n");
            return;
        }
        memset(pconf, 0, MAX_PWM_CHAN * sizeof(struct pwm_test_s));
        for (int i = 0; i < MAX_PWM_CHAN; i++) {
            pconf[i].duty = 25;
            pconf[i].conf.period_cycle = 1000;
            pconf[i].conf.duty_cycle = 25;
        }
    }

    if (os_strcmp(argv[1], "set") == 0) {
        // default
        pwm.ch = 18;
        pwm.freq = 1000;
        pwm.duty = 25;
        pwm.conf.period_cycle = __PWM_FREQ2PERIOD(1000);
        pwm.conf.duty_cycle = pwm.conf.period_cycle / 4;

        switch (argc) {
            case 5: {    // duty
                        pwm.duty = os_strtoul(argv[4], NULL, 10);
                        if (pwm.duty > 100) {
                            bk_printf("duty set 100%\r\n");
                            pwm.duty = 100;
                        }
                    }
            case 4: {     // freq
                        uint32_t freq = os_strtoul(argv[3], NULL, 10);
                        if (!__freq_check(freq)) {
                            bk_printf("freq error: %d\r\n", freq);
                            __pwm_usage();
                            return;
                        }
                        pwm.freq = freq;
                        pwm.conf.period_cycle = __PWM_FREQ2PERIOD(freq);
                        if (argc == 4) { // set default
                            pwm.conf.duty_cycle = pwm.conf.period_cycle / 4;
                        }
                    }
            case 3: {     // channel
                        uint32_t chan = os_strtoul(argv[2], NULL, 10);
                        if (!__chan_check(chan)) {
                            bk_printf("no such channel: %d\r\n", chan);
                            __pwm_usage();
                            return;
                        }
                        pwm.ch = chan;
                        if (argc == 3) { // set default
                            pwm.conf.period_cycle = 1000;
                            pwm.conf.duty_cycle = 250;
                        }
                    }
            default:
                    break;
        }

        pwm.conf.duty_cycle = (pwm.conf.period_cycle * pwm.duty) / 100;
        int id = __get_free_pwm(pwm.ch);
        memcpy(&pconf[id], &pwm, sizeof(struct pwm_test_s));
        __chan_dump(&pconf[id], NULL);
    } else if (os_strcmp(argv[1], "start") == 0) {
        if (argc == 2) {
            bk_printf("all start\r\n");
            for (int i = 0; i < MAX_PWM_CHAN; i++) {
                if (pconf[i].ch != 0 && pconf[i].is_running != 1) {
                    if (!pconf[i].is_init) {
                        BK_LOG_ON_ERR(bk_pwm_init(__pwm_channel_remap(pconf[i].ch), &pconf[i].conf));
                        pconf[i].is_init = 1;
                    }
                    BK_LOG_ON_ERR(bk_pwm_start(__pwm_channel_remap(pconf[i].ch)));
                    pconf[i].is_running = 1;
                    __chan_dump(&pconf[i], "start");
                }
            }
        } else if (argc >= 3) {
            uint32_t chan = os_strtoul(argv[2], NULL, 10);
            if (!__chan_check(chan)) {
                bk_printf("no such channel: %d\r\n", chan);
                __pwm_usage();
                return;
            }
            int id = __get_free_pwm(chan);
            if (!pconf[id].is_init) {
                BK_LOG_ON_ERR(bk_pwm_init(__pwm_channel_remap(pconf[id].ch), &pconf[id].conf));
                pconf[id].is_init = 1;
            }
            BK_LOG_ON_ERR(bk_pwm_start(__pwm_channel_remap(pconf[id].ch)));
            pconf[id].is_running = 1;
            __chan_dump(&pconf[id], "start");
        }
    } else if (os_strcmp(argv[1], "stop") == 0) {
        if (argc == 2) {
            bk_printf("all stop\r\n");
            for (int i = 0; i < MAX_PWM_CHAN; i++) {
                if (pconf[i].ch != 0 && pconf[i].is_running == 1) {
                    BK_LOG_ON_ERR(bk_pwm_stop(__pwm_channel_remap(pconf[i].ch)));
                    pconf[i].is_running = 0;
                    __chan_dump(&pconf[i], "stop");
                }
            }
        } else if (argc >= 3) {
            uint32_t chan = os_strtoul(argv[2], NULL, 10);
            if (!__chan_check(chan)) {
                bk_printf("no such channel: %d\r\n", chan);
                __pwm_usage();
                return;
            }
            int id = __get_free_pwm(chan);
            BK_LOG_ON_ERR(bk_pwm_stop(__pwm_channel_remap(pconf[id].ch)));
            pconf[id].is_running = 0;
            __chan_dump(&pconf[id], "stop");
        }
    } else if (os_strcmp(argv[1], "free") == 0) {
        bk_printf("stop all & free\r\n");
        // stop
        for (int i = 0; i < MAX_PWM_CHAN; i++) {
            if (pconf[i].ch != 0) {
                BK_LOG_ON_ERR(bk_pwm_stop(__pwm_channel_remap(pconf[i].ch)));
                BK_LOG_ON_ERR(bk_pwm_deinit(__pwm_channel_remap(pconf[i].ch)));
                pconf[i].is_running = 0;
            }
        }
        // free
        if (pconf) {
            tkl_system_free(pconf);
            pconf = NULL;
        }
    } else if (os_strcmp(argv[1], "show") == 0) {
        bk_printf("pwm set info:\r\n");
        for (int i = 0; i < MAX_PWM_CHAN; i++) {
            if (pconf[i].ch != 0) {
                bk_printf("channel: %2d,freq: %d,\tduty: %d%%,\tperiod: %d,\thtime: %d,\tstatus: %s\r\n",
                        pconf[i].ch, pconf[i].freq, pconf[i].duty, pconf[i].conf.period_cycle,
                        pconf[i].conf.duty_cycle,
                        (pconf[i].is_running == 1? "running": "stop"));
            }
        }
    } else if (os_strcmp(argv[1], "help") == 0) {
        __pwm_usage();
        return;
    } else if (os_strcmp(argv[1], "demo") == 0) {
        pwm_simple_demo(NULL);
    } else {
        bk_printf("error, unknow command\r\n");
        return;
    }
}

#define PWM_DUTY                    5000 // 50% duty
#define PWM_FREQUENCY               10000
#include "tkl_pinmux.h"
static void pwm_simple_demo(void *param)
{
    OPERATE_RET rt = OPRT_OK;
    uint32_t frequency = PWM_FREQUENCY;
    uint32_t count = 0;

    uint32_t pwm_chan = tkl_io_pin_to_func(36, TUYA_IO_TYPE_PWM);

    /*pwm init*/
    TUYA_PWM_BASE_CFG_T pwm_cfg = {
        .duty = PWM_DUTY, /* 1-10000 */
        .frequency = PWM_FREQUENCY,
        .polarity  = TUYA_PWM_NEGATIVE,
    };

    if (OPRT_OK != tkl_pwm_init(pwm_chan, &pwm_cfg)) {
        bk_printf("error tkl_pwm_info_set\r\n");
        return -1;
    }

    /*start PWM3*/
    if (OPRT_OK != tkl_pwm_start(pwm_chan)) {
        bk_printf("error tkl_pwm_info_set\r\n");
        return -1;
    }
    bk_printf("PWM%d start\r\n", pwm_chan);

    while (1) {
        /*Frequency, duty cycle settings*/
        pwm_cfg.frequency = frequency;
        if (OPRT_OK != tkl_pwm_info_set(pwm_chan, &pwm_cfg)) {
            bk_printf("error tkl_pwm_info_set\r\n");
            return -1;
        }

        if (OPRT_OK != tkl_pwm_start(pwm_chan)) {
            bk_printf("error tkl_pwm_info_set\r\n");
            return -1;
        }
        bk_printf("pwm%d , frequency: %d\r\n", pwm_chan,  frequency);

        tal_system_sleep(2000);

        /*close pwm*/
        count++;
        if(count >= 3) {
            bk_printf("pwm%d test end\r\n", pwm_chan,  frequency);
            break;
        }
        frequency = frequency+10000;

    }

    if (OPRT_OK != tkl_pwm_stop(pwm_chan)) {
        bk_printf("error tkl_pwm_info_set\r\n");
        return -1;
    }

__EXIT:
    bk_printf("PWM task is finished, will delete");
    return;
}

#endif // CONFIG_BK7258_PWM_ENABLE

