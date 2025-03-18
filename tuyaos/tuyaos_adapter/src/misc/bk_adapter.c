/****************************************************************************
 * @file bk_adapter.c
 * @brief this module is used to bk_adapter
 * @version 0.0.1
 * @date 2023-06-28
 *
 * @copyright Copyright(C) 2021-2022 Tuya Inc. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "tuya_cloud_types.h"
#include <stdint.h>
#include "sdkconfig.h"
#include "gpio_map.h"
#include "driver/media_types.h"
#include "tkl_gpio.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Data Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * Security Check Interface : To check the contents of hostids
 **/
#if CONFIG_SYS_CPU1
uint32_t mem_sanity_check(void *mem)
{
    return 1;
}
#endif

extern bk_err_t media_app_get_usb_connect_status(void);
#include "tkl_system.h"
void tuya_get_usb_dev(uint32_t *vid, uint32_t *pid)
{
    int cnt = 10, status = 0;
    if (vid == NULL || pid == NULL)
        return;

    TUYA_GPIO_BASE_CFG_T cfg;
    cfg.mode = TUYA_GPIO_PULLUP;
    cfg.direct = TUYA_GPIO_OUTPUT;
    cfg.level = TUYA_GPIO_LEVEL_HIGH;
    tkl_gpio_init(TUYA_GPIO_NUM_28, &cfg);

    tkl_gpio_write(TUYA_GPIO_NUM_28, TUYA_GPIO_LEVEL_HIGH);

    do {
        status = media_app_get_usb_connect_status();
        if (status) {
            bk_printf("found\r\n\r\n");
            *vid = 0x1111;
            *pid = 0x2222;
            break;
        }
        bk_printf("not found, next\r\n\r\n");
        tkl_system_sleep(50);
    } while (cnt++ < 10);

    tkl_gpio_write(TUYA_GPIO_NUM_28, TUYA_GPIO_LEVEL_LOW);
}

#if CONFIG_SYS_CPU0 && CONFIG_SOC_BK7258
#include "tuya_cloud_types.h"
#include "tkl_gpio.h"
#include "tkl_display.h"

enum {
    MUTIL_INIT = 0,
    MUTIL_ON,
    MUTIL_OFF,
};

static inline int __attribute__((always_inline)) gpio_level_check_and_set(uint32_t io, uint32_t active, int flag)
{
    int expect = 0;
    TUYA_GPIO_LEVEL_E level = TUYA_GPIO_LEVEL_LOW;

    tkl_gpio_read(io, &level);

    switch (flag) {
        case MUTIL_INIT:
            expect = (active == TUYA_GPIO_LEVEL_HIGH)? TUYA_GPIO_LEVEL_LOW: TUYA_GPIO_LEVEL_HIGH;
            break;

        case MUTIL_ON:
            expect = (active == TUYA_GPIO_LEVEL_HIGH)? TUYA_GPIO_LEVEL_HIGH: TUYA_GPIO_LEVEL_LOW;
            if (expect != level) {
                tkl_gpio_write(io, expect);
            }
            break;

        case MUTIL_OFF:
            expect = (active == TUYA_GPIO_LEVEL_HIGH)? TUYA_GPIO_LEVEL_LOW: TUYA_GPIO_LEVEL_HIGH;
            if (expect != level) {
                tkl_gpio_write(io, expect);
            }
            break;

        default:
            break;
    }

    return expect;
}

static uint32_t is_init = 0;
static void __mutil_power_init(void)
{
#if CONFIG_TUYA_LOGIC_MODIFY
    TUYA_GPIO_BASE_CFG_T cfg;
    cfg.direct = TUYA_GPIO_OUTPUT;
    cfg.level = TUYA_GPIO_LEVEL_LOW;

    uint8_t usb_ldo, lcd_ldo, lcd_bl, active_level;

    tkl_vi_get_power_info(UVC_CAMERA, &usb_ldo, &active_level);
    cfg.level = gpio_level_check_and_set(usb_ldo, active_level, MUTIL_INIT);
    tkl_gpio_init(usb_ldo, &cfg);

//     tkl_display_power_ctrl_pin(&lcd_ldo, &active_level);
//     cfg.level = gpio_level_check_and_set(lcd_ldo, active_level, MUTIL_INIT);
//     tkl_gpio_init(lcd_ldo, &cfg);

//    if (tkl_display_bl_mode() == TKL_DISP_BL_GPIO) {
//        tkl_display_bl_ctrl_io(&lcd_bl, &active_level);
//        cfg.level = gpio_level_check_and_set(lcd_bl, active_level, MUTIL_INIT);
//        tkl_gpio_init(lcd_bl, &cfg);
//    }
#ifdef MUTEX_CTRL
    tkl_gpio_init(MUTEX_CTRL, &cfg);
#endif // MUTEX_CTRL
#endif // CONFIG_TUYA_LOGIC_MODIFY
}

void tuya_multimedia_power_on(void)
{
#if CONFIG_TUYA_LOGIC_MODIFY
//    if (!is_init) {
        __mutil_power_init();
        is_init = 1;
//    }
    uint8_t usb_ldo, lcd_ldo, lcd_bl, active_level;
    // 3.3V / USB Enable
    TUYA_GPIO_LEVEL_E level = TUYA_GPIO_LEVEL_LOW;
    tkl_display_power_ctrl_pin(&lcd_ldo, &active_level);
    gpio_level_check_and_set(lcd_ldo, active_level, MUTIL_ON);

    tkl_vi_get_power_info(UVC_CAMERA, &usb_ldo, &active_level);
    gpio_level_check_and_set(usb_ldo, active_level, MUTIL_ON);

#endif // CONFIG_TUYA_LOGIC_MODIFY
}

void tuya_multimedia_power_off(void)
{
#if CONFIG_TUYA_LOGIC_MODIFY
//    if (!is_init) {
        __mutil_power_init();
        is_init = 1;
//    }

    uint8_t usb_ldo, lcd_ldo, lcd_bl, active_level;

//     if (tkl_display_bl_mode() == TKL_DISP_BL_GPIO) {
//         tkl_display_bl_ctrl_io(&lcd_bl, &active_level);
//         gpio_level_check_and_set(lcd_bl, active_level, MUTIL_OFF);
//     }

    // 3.3V / USB Enable
    TUYA_GPIO_LEVEL_E level = TUYA_GPIO_LEVEL_LOW;
    tkl_display_power_ctrl_pin(&lcd_ldo, &active_level);
    gpio_level_check_and_set(lcd_ldo, active_level, MUTIL_OFF);

    tkl_vi_get_power_info(UVC_CAMERA, &usb_ldo, &active_level);
    gpio_level_check_and_set(usb_ldo, active_level, MUTIL_OFF);

#endif // CONFIG_TUYA_LOGIC_MODIFY
}

#endif // CONFIG_SYS_CPU0 && CONFIG_SOC_BK7258

#if CONFIG_SYS_CPU0
uint8_t* dhcp_lookup_mac(uint8_t *chaddr)
{
    return NULL;
}
#endif

