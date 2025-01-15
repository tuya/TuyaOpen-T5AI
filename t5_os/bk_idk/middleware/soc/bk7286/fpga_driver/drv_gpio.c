#include "bkreg.h"
#include "drv_gpio.h"

#define gpio_context ((GPIOContext*)MDU_GPIO_BASE_ADDR)

typedef struct _GPIOContext
{
    volatile uint32_t config[GPIO_COUNT];
    volatile uint32_t reserved[16];
    volatile uint64_t int_status;
}GPIOContext;

void system_gpio_peri_config(uint32_t idx, uint32_t mode);

void gpio_config(GPIO_PIN pin, GPIO_DIR dir, GPIO_PULL_MODE pull, GPIO_PERI_MODE peri)
{
    if(pin >= GPIO_COUNT) return;

    gpio_context->config[pin] = ((uint32_t)dir << SFT_GPIO_0x00_INPUT_ENA) | ((uint32_t)pull << SFT_GPIO_0x00_PULL_MODE) | ((!!peri) << SFT_GPIO_0x00_FUN_ENA);

    if(peri)
    {
        system_gpio_peri_config(pin, peri-1);
    }
}

void gpio_int_mode_set(GPIO_PIN pin, GPIO_INT_MODE mode)
{
    if(pin >= GPIO_COUNT) return;

    gpio_context->config[pin] = (gpio_context->config[pin] & ~MSK_GPIO_0x00_INT_TYPE) | ((uint32_t)mode << SFT_GPIO_0x00_INT_TYPE);
}

void gpio_int_enable(GPIO_PIN pin, uint32_t enable)
{
    if(pin >= GPIO_COUNT) return;

    gpio_context->config[pin] = (gpio_context->config[pin] & ~MSK_GPIO_0x00_INT_ENA) | ((uint32_t)(!!enable) << SFT_GPIO_0x00_INT_ENA);
}

void gpio_monitor_enable(GPIO_PIN pin, uint32_t enable)
{
    if(pin >= GPIO_COUNT) return;

    if(enable)
    {
        gpio_context->config[pin] |= MSK_GPIO_0x00_INPUT_MONITOR;
    }
    else
    {
        gpio_context->config[pin] &= ~MSK_GPIO_0x00_INPUT_MONITOR;
    }
}

void gpio_driver_capacity_set(GPIO_PIN pin, GPIO_DRV_CAP capacity)
{
    if(pin >= GPIO_COUNT) return;

    gpio_context->config[pin] = (gpio_context->config[pin] & ~MSK_GPIO_0x00_CAPACITY) | ((uint32_t)capacity << SFT_GPIO_0x00_CAPACITY);
}

uint32_t gpio_input_get(GPIO_PIN pin)
{
    return pin < GPIO_COUNT ? ((gpio_context->config[pin] & MSK_GPIO_0x00_INPUT) >> SFT_GPIO_0x00_INPUT) : 0;
}

uint32_t gpio_output_get(GPIO_PIN pin)
{
    return pin < GPIO_COUNT ? ((gpio_context->config[pin] & MSK_GPIO_0x00_OUTPUT) >> SFT_GPIO_0x00_OUTPUT) : 0;
}

void gpio_output_set(GPIO_PIN pin, uint32_t value)
{
    if(pin >= GPIO_COUNT) return;

    if(value)
    {
        gpio_context->config[pin] |= MSK_GPIO_0x00_OUTPUT;
    }
    else
    {
        gpio_context->config[pin] &= ~MSK_GPIO_0x00_OUTPUT;
    }
}
