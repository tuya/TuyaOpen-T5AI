#include "tkl_gpio.h"
#include <driver/gpio.h>
#include "gpio_driver.h"
#include "sdkconfig.h"
#include <driver/uart.h>



typedef struct {
    gpio_id_t           gpio;
    TUYA_GPIO_IRQ_CB    cb;
    void                *args;
} pin_dev_map_t;

static pin_dev_map_t pinmap[] = {
    {GPIO_0,  NULL, NULL}, {GPIO_1,  NULL, NULL}, {GPIO_2,  NULL, NULL}, {GPIO_3,  NULL, NULL},
    {GPIO_4,  NULL, NULL}, {GPIO_5,  NULL, NULL}, {GPIO_6,  NULL, NULL}, {GPIO_7,  NULL, NULL},
    {GPIO_8,  NULL, NULL}, {GPIO_9,  NULL, NULL}, {GPIO_10, NULL, NULL}, {GPIO_11, NULL, NULL},
    {GPIO_12, NULL, NULL}, {GPIO_13, NULL, NULL}, {GPIO_14, NULL, NULL}, {GPIO_15, NULL, NULL},
    {GPIO_16, NULL, NULL}, {GPIO_17, NULL, NULL}, {GPIO_18, NULL, NULL}, {GPIO_19, NULL, NULL},
    {GPIO_20, NULL, NULL}, {GPIO_21, NULL, NULL}, {GPIO_22, NULL, NULL}, {GPIO_23, NULL, NULL},
    {GPIO_24, NULL, NULL}, {GPIO_25, NULL, NULL}, {GPIO_26, NULL, NULL}, {GPIO_27, NULL, NULL},
    {GPIO_28, NULL, NULL}, {GPIO_29, NULL, NULL}, {GPIO_30, NULL, NULL}, {GPIO_31, NULL, NULL},
    {GPIO_32, NULL, NULL}, {GPIO_33, NULL, NULL}, {GPIO_34, NULL, NULL}, {GPIO_35, NULL, NULL},
    {GPIO_36, NULL, NULL}, {GPIO_37, NULL, NULL}, {GPIO_38, NULL, NULL}, {GPIO_39, NULL, NULL},
    {GPIO_40, NULL, NULL}, {GPIO_41, NULL, NULL}, {GPIO_42, NULL, NULL}, {GPIO_43, NULL, NULL},
    {GPIO_44, NULL, NULL}, {GPIO_45, NULL, NULL}, {GPIO_46, NULL, NULL}, {GPIO_47, NULL, NULL},
    {GPIO_48, NULL, NULL},
#if CONFIG_SOC_BK7258
    {GPIO_49, NULL, NULL}, {GPIO_50, NULL, NULL}, {GPIO_51, NULL, NULL},
    {GPIO_52, NULL, NULL}, {GPIO_53, NULL, NULL}, {GPIO_54, NULL, NULL}, {GPIO_55, NULL, NULL},
#endif // CONFIG_SOC_BK7258
};

#define PIN_DEV_CHECK_ERROR_RETURN(__PIN, __ERROR)                          \
    if (__PIN >= sizeof(pinmap)/sizeof(pinmap[0])) {                        \
        return __ERROR;                                                     \
    }

extern void bk_printf(const char *fmt, ...);

/**
 * @brief gpio init
 *
 * @param[in] pin_id: gpio pin id
 * @param[in] cfg:  gpio config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_init(TUYA_GPIO_NUM_E pin_id, const TUYA_GPIO_BASE_CFG_T *cfg)
{
    OPERATE_RET ret = OPRT_OK;
    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);

    bk_gpio_driver_init();
    gpio_dev_unmap(pinmap[pin_id].gpio);
    //! set pin direction
    switch (cfg->direct) {
        case TUYA_GPIO_INPUT:
            bk_gpio_disable_output(pinmap[pin_id].gpio);
            bk_gpio_enable_input(pinmap[pin_id].gpio);

            if(cfg->mode == TUYA_GPIO_PULLUP) {
                bk_gpio_enable_pull(pinmap[pin_id].gpio);
                bk_gpio_pull_up(pinmap[pin_id].gpio);
            } else if(cfg->mode == TUYA_GPIO_PULLDOWN) {
                bk_gpio_enable_pull(pinmap[pin_id].gpio);
                bk_gpio_pull_down(pinmap[pin_id].gpio);
            } else if(cfg->mode == TUYA_GPIO_FLOATING) {
                bk_gpio_disable_pull(pinmap[pin_id].gpio);
            } else {
                bk_printf("set direct error \r\n");
                return OPRT_NOT_SUPPORTED;
            }
            break;
        case TUYA_GPIO_OUTPUT:
            bk_gpio_disable_input(pinmap[pin_id].gpio);
            bk_gpio_enable_output(pinmap[pin_id].gpio);
            //! set pin init level
            if(TUYA_GPIO_LEVEL_LOW == cfg->level) {
                bk_gpio_set_output_low(pinmap[pin_id].gpio);
            } else {
                bk_gpio_set_output_high(pinmap[pin_id].gpio);
            }
            break;
        default:
            return OPRT_NOT_SUPPORTED;
    }

    return ret;
}

/**
 * @brief gpio deinit
 *
 * @param[in] pin_id: gpio pin id
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_deinit(TUYA_GPIO_NUM_E pin_id)
{
    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);

    //Set gpio to input floating mode
    bk_gpio_disable_interrupt(pinmap[pin_id].gpio);
    bk_gpio_disable_output(pinmap[pin_id].gpio);
    bk_gpio_enable_input(pinmap[pin_id].gpio);
    bk_gpio_disable_pull(pinmap[pin_id].gpio);

    return OPRT_OK;
}

/**
 * @brief gpio write
 *
 * @param[in] pin_id: gpio pin id
 * @param[in] level: gpio output level value
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_write(TUYA_GPIO_NUM_E pin_id, TUYA_GPIO_LEVEL_E level)
{
    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);

    if (level == TUYA_GPIO_LEVEL_HIGH) {
        bk_gpio_set_output_high(pinmap[pin_id].gpio);
    } else {
        bk_gpio_set_output_low(pinmap[pin_id].gpio);
    }

    return OPRT_OK;
}

/**
 * @brief gpio read
 *
 * @param[in] pin_id: gpio pin id
 * @param[out] level: gpio output level
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_read(TUYA_GPIO_NUM_E pin_id, TUYA_GPIO_LEVEL_E *level)
{
    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);

    if(bk_gpio_get_input(pinmap[pin_id].gpio)) {
        *level = TUYA_GPIO_LEVEL_HIGH;
    } else {
        *level = TUYA_GPIO_LEVEL_LOW;
    }
    return OPRT_OK;
}

/**
 * @brief gpio irq cb
 * NOTE: this API will call irq callback
 *
 * @param[in] arg: gpio pin id
 * @return none
 */
static void __tkl_gpio_irq_cb(gpio_id_t gpio_id)
{
    if(pinmap[gpio_id].cb){
        pinmap[gpio_id].cb(pinmap[gpio_id].args);
    }
    bk_gpio_clear_interrupt(gpio_id);
    return;
}

/**
 * @brief gpio irq init
 * NOTE: call this API will not enable interrupt
 *
 * @param[in] pin_id: gpio pin id
 * @param[in] cfg:  gpio irq config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_irq_init(TUYA_GPIO_NUM_E pin_id, const TUYA_GPIO_IRQ_T *cfg)
{
    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);

    gpio_int_type_t trigger;

    switch (cfg->mode) {
        case TUYA_GPIO_IRQ_RISE:
            trigger = GPIO_INT_TYPE_RISING_EDGE;
            break;
        case TUYA_GPIO_IRQ_FALL:
            trigger = GPIO_INT_TYPE_FALLING_EDGE;
            break;
        case TUYA_GPIO_IRQ_LOW:
            trigger = GPIO_INT_TYPE_LOW_LEVEL;
            break;
        case TUYA_GPIO_IRQ_HIGH:
            trigger = GPIO_INT_TYPE_HIGH_LEVEL;
            break;
        case TUYA_GPIO_IRQ_RISE_FALL:
        default:
            bk_printf("GPIO_INT_TYPE_HIGH_LEVEL Not supported yet !!!\r\n");
            return OPRT_NOT_SUPPORTED;
    }

    pinmap[pin_id].cb = (TUYA_GPIO_IRQ_CB)cfg->cb;
    pinmap[pin_id].args = cfg->arg;
    bk_gpio_register_isr(pinmap[pin_id].gpio, (gpio_isr_t)__tkl_gpio_irq_cb);
    bk_gpio_set_interrupt_type(pinmap[pin_id].gpio, trigger);
    return OPRT_OK;
}

/**
 * @brief gpio irq enable
 *
 * @param[in] pin_id: gpio pin id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_irq_enable(TUYA_GPIO_NUM_E pin_id)
{
    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
    bk_gpio_enable_interrupt(pinmap[pin_id].gpio);
    return OPRT_OK;
}

/**
 * @brief gpio irq disable
 *
 * @param[in] pin_id: gpio pin id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_irq_disable(TUYA_GPIO_NUM_E pin_id)
{
    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
    bk_gpio_disable_interrupt(pinmap[pin_id].gpio);
    return OPRT_OK;
}
