/*
 * test_gpio.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "cli.h"
#include "cli_tuya_test.h"

extern OPERATE_RET tkl_gpio_init(TUYA_GPIO_NUM_E pin_id, const TUYA_GPIO_BASE_CFG_T *cfg);
static void __gpio_irq_test(void *args)
{
    bk_printf("--- [%s %d]\r\n", __func__, __LINE__);
}

static void __gpio_cmd_usage(void)
{
    bk_printf("xgpio input|output [gpio num] 0|1\r\n");
    bk_printf("xgpio irq [gpio num] [rase|fall|low|high|start|stop]\r\n");
}

void cli_gpio_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc == 1) {
        bk_printf("no parameter\r\n");
        __gpio_cmd_usage();
        return;
    }
    for (int i = 0; i < argc; i++) {
        bk_printf("argv[%d]: %s\r\n", i, argv[i]);
    }

    TUYA_GPIO_NUM_E pin_id = os_strtoul(argv[1], NULL, 10);
    TUYA_GPIO_BASE_CFG_T cfg;
    int irq = 0;

    if (!strcmp("input", argv[2]))
        cfg.direct = TUYA_GPIO_INPUT;
    else if (!strcmp("output", argv[2]))
        cfg.direct = TUYA_GPIO_OUTPUT;
    else if (!strcmp("irq", argv[2]))
        irq = 1;
    else {
        __gpio_cmd_usage();
        return;
    }

    if (irq) {
        TUYA_GPIO_IRQ_T cfg;
        cfg.cb = __gpio_irq_test;
        cfg.arg = NULL;
        if (!strcmp("rise", argv[3])) {
            cfg.mode = TUYA_GPIO_IRQ_RISE;
        } else if (!strcmp("fall", argv[3])) {
            cfg.mode = TUYA_GPIO_IRQ_FALL;
        } else if (!strcmp("high", argv[3])) {
            cfg.mode = TUYA_GPIO_IRQ_HIGH;
        } else if (!strcmp("low", argv[3])) {
            cfg.mode = TUYA_GPIO_IRQ_LOW;
        } else if (!strcmp("start", argv[3])) {
            tkl_gpio_irq_enable(pin_id);
            return;
        } else if (!strcmp("stop", argv[3])) {
            tkl_gpio_irq_disable(pin_id);
            return;
        } else {
            bk_printf("invalid parameter\r\n");
        }
        tkl_gpio_irq_init(pin_id, &cfg);
    } else {
        uint32_t argv3 = os_strtoul(argv[3], NULL, 10);
        if (argv3 == 0) {
            cfg.mode = TUYA_GPIO_PULLDOWN;
            cfg.level = TUYA_GPIO_LEVEL_LOW;
        } else if (argv3 == 1) {
            cfg.mode = TUYA_GPIO_PULLUP;
            cfg.level = TUYA_GPIO_LEVEL_HIGH;
        } else {
            cfg.mode = TUYA_GPIO_FLOATING;
            cfg.level = TUYA_GPIO_LEVEL_NONE;
        }
        tkl_gpio_init(pin_id, &cfg);
    }

    return;
}



