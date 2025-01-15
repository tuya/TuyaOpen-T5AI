#ifndef __DRV_GPIO_H__
#define __DRV_GPIO_H__

#include <stdint.h>

/**
 * @brief GPIO definition
 */
typedef enum
{
            //1st FUNC        2nd FUNC        3rd FUNC            4th FUNC
    GPIO0 , //
    GPIO1 , //
    GPIO2 , //
    GPIO3 , //
    GPIO4 , //
    GPIO5 , //
    GPIO6 , //
    GPIO7 , //
    GPIO8 , //
    GPIO9 , //
    GPIO10, //
    GPIO11, //
    GPIO12, //
    GPIO13, //
    GPIO14, //
    GPIO15, //
    GPIO16, //
    GPIO17, //
    GPIO18, //
    GPIO19, //
    GPIO20, //
    GPIO21, //
    GPIO22, //
    GPIO23, //
    GPIO24, //
    GPIO25, //
    GPIO26, //
    GPIO27, //
    GPIO28, //
    GPIO29, //
    GPIO30, //
    GPIO31, //
    GPIO32, //
    GPIO33, //
    GPIO34, //
    GPIO35, //
    GPIO36, //
    GPIO37, //
    GPIO38, //
    GPIO39, //
    GPIO40, //
    GPIO41, //
    GPIO42, //
    GPIO43, //
    GPIO44, //
    GPIO45, //
    GPIO46, //
    GPIO47, //
    GPIO_COUNT,
}GPIO_PIN;

/**
 * @brief GPIO direction definition
 */
typedef enum
{
    GPIO_INPUT  = 3,
    GPIO_OUTPUT = 0,
    GPIO_INOUT  = 1,
    GPIO_HRES   = 2,
}GPIO_DIR;

/**
 * @brief GPIO pull mode definition
 */
typedef enum
{
    GPIO_PULL_NONE = 0,
    GPIO_PULL_DOWN = 2,
    GPIO_PULL_UP   = 3,
}GPIO_PULL_MODE;

/**
 * @brief GPIO peripheral mode definition
 */
typedef enum
{
    GPIO_PERI_NONE  = 0,
    GPIO_PERI_FUNC1 = 1,
    GPIO_PERI_FUNC2 = 2,
    GPIO_PERI_FUNC3 = 3,
    GPIO_PERI_FUNC4 = 4,
    GPIO_PERI_FUNC5 = 5,
    GPIO_PERI_FUNC6 = 6,
    GPIO_PERI_FUNC7 = 7,
    GPIO_PERI_FUNC8 = 8,
}GPIO_PERI_MODE;

/**
 * @brief GPIO driver capacity definition
 */
typedef enum
{
    GPIO_DRV_5mA = 0,
    GPIO_DRV_10mA,
    GPIO_DRV_15mA,
    GPIO_DRV_20mA,
}GPIO_DRV_CAP;

/**
 * @brief GPIO interrupt mode definition
 */
typedef enum
{
    GPIO_INT_MODE_LOW_LEVEL = 0,
    GPIO_INT_MODE_HIGH_LEVEL,
    GPIO_INT_MODE_POSEDGE,
    GPIO_INT_MODE_NEGEDGE,
}GPIO_INT_MODE;

/**
 * @brief GPIO configuration
 * @param pin  GPIO pin index, can be GPIO0~GPIO39, @see GPIO_PIN
 * @param dir  GPIO direction, @see GPIO_DIR
 * @param pull GPIO pull mode, @see GPIO_PULL_MODE
 * @param peri GPIO peripheral mode, @see GPIO_PERI_MODE
 */
void gpio_config(GPIO_PIN pin, GPIO_DIR dir, GPIO_PULL_MODE pull, GPIO_PERI_MODE peri);

/**
 * @brief GPIO interrupt mode setting
 * @param pin  GPIO pin index, can be GPIO0~GPIO39, @see GPIO_PIN
 * @param mode GPIO interrupt mode, @see GPIO_INT_MODE
 */
void gpio_int_mode_set(GPIO_PIN pin, GPIO_INT_MODE mode);

/**
 * @brief GPIO interrupt enable setting
 * @param pin  GPIO pin index, can be GPIO0~GPIO39, @see GPIO_PIN
 * @param enable  GPIO interrupt enable
 */
void gpio_int_enable(GPIO_PIN pin, uint32_t enable);

/**
 * @brief GPIO monitor enable setting
 * @param pin  GPIO pin index, can be GPIO0~GPIO39, @see GPIO_PIN
 * @param enable  GPIO monitor enable
 */
void gpio_monitor_enable(GPIO_PIN pin, uint32_t enable);

/**
 * @brief GPIO driver capacity setting
 * @param pin  GPIO pin index, can be GPIO0~GPIO39, @see GPIO_PIN
 * @param capacity  GPIO driver capacity
 */
void gpio_driver_capacity_set(GPIO_PIN pin, GPIO_DRV_CAP capacity);

/**
 * @brief GPIO input value get
 * @param pin  GPIO pin index, can be GPIO0~GPIO39, @see GPIO_PIN
 * @return current input value
 */
uint32_t gpio_input_get(GPIO_PIN pin);

/**
 * @brief GPIO output value get
 * @param pin  GPIO pin index, can be GPIO0~GPIO39, @see GPIO_PIN
 * @return current output value
 */
uint32_t gpio_output_get(GPIO_PIN pin);

/**
 * @brief GPIO output value setting
 * @param pin  GPIO pin index, can be GPIO0~GPIO39, @see GPIO_PIN
 * @param value  output value, range from 0~1
 */
void gpio_output_set(GPIO_PIN pin, uint32_t value);

#endif//__DRV_GPIO_H__
