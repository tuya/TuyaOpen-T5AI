/**
 * @file tkl_i2c.c
 * @brief default weak implements of tuya pin
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */
#include <driver/gpio.h>
#include <driver/int.h>
#include <driver/i2c.h>
#include "clock_driver.h"
#include "gpio_driver.h"
#include "icu_driver.h"
#include <os/mem.h>
#include "power_driver.h"
#include <os/os.h>
#include "i2c_hw.h"
#include "i2c_driver.h"
#include "i2c_hal.h"
#include "i2c_statis.h"
#include "sys_driver.h"

#include "tkl_gpio.h"
#include "tal_log.h"
#include "tkl_i2c.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define I2C_WRITE           0
#define I2C_READ            1

#if 0
#define I2C_SCL_INIT()      __sw_i2c_scl_init(i2c_pin.scl)
#define I2C_SCL_H()         __sw_i2c_write(i2c_pin.scl, TUYA_GPIO_LEVEL_HIGH)
#define I2C_SCL_L()         __sw_i2c_write(i2c_pin.scl, TUYA_GPIO_LEVEL_LOW)

#define I2C_SDA_INIT_OUT()  __sw_i2c_sda_init(i2c_pin.sda, FALSE)
#define I2C_SDA_H()         __sw_i2c_write(i2c_pin.sda, TUYA_GPIO_LEVEL_HIGH)
#define I2C_SDA_L()         __sw_i2c_write(i2c_pin.sda, TUYA_GPIO_LEVEL_LOW)

#define I2C_SDA_INIT_IN()   __sw_i2c_sda_init(i2c_pin.sda, TRUE)
#define I2C_SDA_READ()      __sw_i2c_sda_read(i2c_pin.sda)
#endif

#define GPIO_OUTPUT_HIGH		0x2
#define GPIO_OUTPUT_LOW			0x0

#define I2C_SCL_INIT()      __sw_i2c_scl_init(i2c_pin.scl)
#define I2C_SCL_H() do { \
                    *((volatile unsigned long *) (SOC_AON_GPIO_REG_BASE+(i2c_pin.scl)*4)) = GPIO_OUTPUT_HIGH; \
                } while(0)

#define I2C_SCL_L() do { \
                    *((volatile unsigned long *) (SOC_AON_GPIO_REG_BASE+(i2c_pin.scl)*4)) = GPIO_OUTPUT_LOW; \
                } while(0)

#define I2C_SDA_INIT_OUT()  do { \
                    *((volatile unsigned long *) (SOC_AON_GPIO_REG_BASE+(i2c_pin.sda)*4)) = GPIO_OUTPUT_HIGH; \
                } while(0)

#define I2C_SDA_H() do { \
                    *((volatile unsigned long *) (SOC_AON_GPIO_REG_BASE+(i2c_pin.sda)*4)) = GPIO_OUTPUT_HIGH; \
                } while(0)

#define I2C_SDA_L() do { \
                    *((volatile unsigned long *) (SOC_AON_GPIO_REG_BASE+(i2c_pin.sda)*4)) = GPIO_OUTPUT_LOW; \
                } while(0)


#define I2C_SDA_INIT_IN() do { \
                    bk_gpio_disable_output(i2c_pin.sda); \
                    bk_gpio_enable_input(i2c_pin.sda); \
                } while(0)

#define I2C_SDA_READ() (uint8_t)(bk_gpio_get_input(i2c_pin.sda))


#define I2C_DELAY(us) \
do { \
    volatile uint32_t i = us; \
    while (i--); \
} while(0)

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    TUYA_GPIO_NUM_E scl;
    TUYA_GPIO_NUM_E sda;
} SR_I2C_GPIO_T;

typedef struct {
    uint8_t addr_width;
    uint8_t delay_us;
} SR_I2C_CONFIG_T;

/***********************************************************
********************function declaration********************
***********************************************************/
#define SR_I2C_FLAG_WR          (1u << 0)   // write
#define SR_I2C_FLAG_RD          (1u << 1)   // read
#define SR_I2C_FLAG_ADDR_10BIT  (1u << 2)   // 10bits address
#define SR_I2C_FLAG_NO_START    (1u << 3)   // no start
#define SR_I2C_FLAG_IGNORE_NACK (1u << 4)   // ignore NACK
#define SR_I2C_FLAG_NO_READ_ACK (1u << 5)   // read without ACK
#define SR_I2C_FLAG_NO_ADDR     (1u << 6)   // massage wuthout address

extern void bk_printf(const char *fmt, ...);

/***********************************************************
***********************variable define**********************
***********************************************************/
static volatile uint32_t delay_us = 10;
static SR_I2C_GPIO_T sg_i2c_pin[TUYA_I2C_NUM_MAX] = {
    {TUYA_GPIO_NUM_20, TUYA_GPIO_NUM_21},
    {TUYA_GPIO_NUM_0, TUYA_GPIO_NUM_1},
    {TUYA_GPIO_NUM_MAX, TUYA_GPIO_NUM_MAX},
    {TUYA_GPIO_NUM_MAX, TUYA_GPIO_NUM_MAX},
    {TUYA_GPIO_NUM_MAX, TUYA_GPIO_NUM_MAX},
    {TUYA_GPIO_NUM_MAX, TUYA_GPIO_NUM_MAX}
};
static SR_I2C_CONFIG_T sg_i2c_cfg[TUYA_I2C_NUM_MAX] = {0};

/***********************************************************
***********************function define**********************
***********************************************************/
/**
 * @brief SCL pin init
 *
 * @param[in] : pin number
 *
 * @return none
 */
static void __sw_i2c_scl_init(TUYA_GPIO_NUM_E pin)
{
    TUYA_GPIO_BASE_CFG_T pin_cfg = {
        .mode = TUYA_GPIO_PUSH_PULL,
        .direct = TUYA_GPIO_OUTPUT,
        .level = TUYA_GPIO_LEVEL_HIGH
    };
    tkl_gpio_init(pin, &pin_cfg);
}

/**
 * @brief SDA pin init
 *
 * @param[in] pin: pin number
 * @param[in] in: TRUE on in, FALSE on out
 *
 * @return none
 */
static void __sw_i2c_sda_init(TUYA_GPIO_NUM_E pin, BOOL_T in)
{
    TUYA_GPIO_BASE_CFG_T pin_cfg;
    if (in) {
        pin_cfg.mode = TUYA_GPIO_PULLUP;
        pin_cfg.direct = TUYA_GPIO_INPUT;
        pin_cfg.level = TUYA_GPIO_LEVEL_HIGH;
    } else {
        pin_cfg.mode = TUYA_GPIO_PUSH_PULL;
        pin_cfg.direct = TUYA_GPIO_OUTPUT;
        pin_cfg.level = TUYA_GPIO_LEVEL_LOW;
    }
    tkl_gpio_init(pin, &pin_cfg);
}

/**
 * @brief read SDA pin
 *
 * @param[in] pin: pin number
 *
 * @return pin level
 */
static TUYA_GPIO_LEVEL_E __sw_i2c_sda_read(TUYA_GPIO_NUM_E pin)
{
    TUYA_GPIO_LEVEL_E level;
    tkl_gpio_read(pin, &level);
    return level;
}

/**
 * @brief write i2c pin
 *
 * @param[in] pin: pin number
 * @param[in] level: pin level
 *
 * @return none
 */
static void __sw_i2c_write(TUYA_GPIO_NUM_E pin, TUYA_GPIO_LEVEL_E level)
{
    tkl_gpio_write(pin, level);
}

/**
 * @brief i2c start
 *
 * @param[in] i2c_pin: pin number
 *
 * @return none
 */
static void __sw_i2c_start(SR_I2C_GPIO_T i2c_pin)
{
    I2C_SCL_H();
    I2C_SDA_H();
    I2C_DELAY(delay_us);

    I2C_SDA_L();
    I2C_DELAY(delay_us);

    I2C_SCL_L();
}

/**
 * @brief i2c stop
 *
 * @param[in] i2c_pin: pin number
 *
 * @return none
 */
static void __sw_i2c_stop(SR_I2C_GPIO_T i2c_pin)
{
    I2C_SCL_L();
    I2C_SDA_INIT_OUT();

    I2C_SDA_L();

    I2C_SCL_H();
    I2C_DELAY(delay_us);

    I2C_SDA_H();
    I2C_DELAY(delay_us);
}

/**
 * @brief i2c ack
 *
 * @param[in] i2c_pin: pin number
 *
 * @return none
 */
static void __sw_i2c_ack(SR_I2C_GPIO_T i2c_pin)
{
    I2C_SCL_L();
    I2C_SDA_INIT_OUT();

    I2C_SDA_L();
    I2C_DELAY(delay_us);
    I2C_SCL_H();
    I2C_DELAY(delay_us);
    I2C_SCL_L();
}

/**
 * @brief i2c no ack
 *
 * @param[in] i2c_pin: pin number
 *
 * @return none
 */
static void __sw_i2c_no_ack(SR_I2C_GPIO_T i2c_pin)
{
    I2C_SCL_L();
    I2C_SDA_INIT_OUT();

    I2C_SDA_H();
    I2C_DELAY(delay_us);
    I2C_SCL_H();
    I2C_DELAY(delay_us);
    I2C_SCL_L();
}

/**
 * @brief i2c get ack
 *
 * @param[in] i2c_pin: pin number
 *
 * @return none
 */
static BOOL_T __sw_i2c_get_ack(SR_I2C_GPIO_T i2c_pin)
{
    uint32_t timeout_count = 0;
    I2C_SCL_L();
    I2C_SDA_INIT_IN();
    I2C_DELAY(1);

    while (I2C_SDA_READ()) {
        if (timeout_count >= 500000) {
            __sw_i2c_stop(i2c_pin);
            bk_printf("wait ack timeout %d\r\n", I2C_SDA_READ());
            return FALSE;
        }
        I2C_DELAY(10);
        timeout_count++;
    }

    I2C_SCL_H();
    I2C_DELAY(delay_us);
    I2C_SCL_L();

    return TRUE;
}

/**
 * @brief i2c send byte
 *
 * @param[in] i2c_pin: pin number
 * @param[in] data: data to send
 *
 * @return none
 */
static void __sw_i2c_send_byte(SR_I2C_GPIO_T i2c_pin, uint8_t data)
{
    uint8_t i = 0;

    I2C_SCL_L();
    I2C_SDA_INIT_OUT();

    for (i = 0; i < 8; i++) {
        if (data & 0x80) {
            I2C_SDA_H();
        } else {
            I2C_SDA_L();
        }
        data = data << 1;

        I2C_DELAY(delay_us);
        I2C_SCL_H();
        I2C_DELAY(delay_us);
        I2C_SCL_L();
    }
}

/**
 * @brief i2c read byte
 *
 * @param[in] i2c_pin: pin number
 * @param[in] need_ack: if need ack
 *
 * @return read byte
 */
static uint8_t __sw_i2c_read_byte(SR_I2C_GPIO_T i2c_pin, BOOL_T need_ack)
{
    uint8_t read_byte = 0x00;
    uint8_t i = 0;

    I2C_SCL_L();
    I2C_SDA_INIT_IN();

    for (i = 0; i < 8; i++) {
        I2C_SCL_L();
        I2C_DELAY(delay_us);
        I2C_SCL_H();
        read_byte <<= 1;
        if (I2C_SDA_READ()) {
            read_byte++;
        }
    }

    if (need_ack) {
        __sw_i2c_ack(i2c_pin);
    } else {
        __sw_i2c_no_ack(i2c_pin);
    }

    return read_byte;
}

/**
 * @brief software i2c write data
 *
 * @param[in] port: i2c port
 * @param[in] addr: slave address
 * @param[in] buf: data buffer
 * @param[in] len: data length
 * @param[in] xfer_pending: TRUE : not send stop condition, FALSE : send stop condition.
 *
 * @return operation result
 */
static int __sw_i2c_write_data(uint8_t port, USHORT_T addr, const uint8_t *buf, uint8_t len, BOOL_T xfer_pending)
{
    __sw_i2c_start(sg_i2c_pin[port]);

    __sw_i2c_send_byte(sg_i2c_pin[port], (addr << 1) | I2C_WRITE);
    if (!__sw_i2c_get_ack(sg_i2c_pin[port])) {
        __sw_i2c_stop(sg_i2c_pin[port]);
        return -1;
    }

    for (uint8_t i = 0; i < len; i++) {
        __sw_i2c_send_byte(sg_i2c_pin[port], buf[i]);
        if (!__sw_i2c_get_ack(sg_i2c_pin[port])) {
            __sw_i2c_stop(sg_i2c_pin[port]);
            return -1;
        }
    }

    if (FALSE == xfer_pending)
        __sw_i2c_stop(sg_i2c_pin[port]);

    return 0;
}

/**
 * @brief software i2c read data
 *
 * @param[in] port: i2c port
 * @param[in] addr: slave address
 * @param[in] buf: data buffer
 * @param[in] len: data length
 * @param[in] xfer_pending: TRUE : not send stop condition, FALSE : send stop condition.
 *
 * @return operation result
 */
static int __sw_i2c_read_data(uint8_t port, USHORT_T addr, uint8_t *buf, uint8_t len, BOOL_T xfer_pending)
{
    uint8_t i;
    __sw_i2c_start(sg_i2c_pin[port]);

    __sw_i2c_send_byte(sg_i2c_pin[port], (addr << 1) | I2C_READ);
    if (!__sw_i2c_get_ack(sg_i2c_pin[port])) {
        __sw_i2c_stop(sg_i2c_pin[port]);
        return -1;
    }

    for (i = 0; i < len-1; i++) {
        buf[i] = __sw_i2c_read_byte(sg_i2c_pin[port], TRUE);
    }
    buf[i] = __sw_i2c_read_byte(sg_i2c_pin[port], FALSE);

    if (FALSE == xfer_pending)
        __sw_i2c_stop(sg_i2c_pin[port]);

    return 0;
}

/**
 * @brief software i2c init
 *
 * @param[in] i2c_pin: i2c pin number
 *
 * @return operation result
 */
static void __sw_i2c_init(SR_I2C_GPIO_T i2c_pin)
{
    I2C_SCL_INIT();
    I2C_SDA_INIT_OUT();
    I2C_SCL_H();
    I2C_SDA_H();
}

/**
 * @brief set i2c scl pin
 * NOTE: call this API in tkl_io_pinmux_config
 * * *
 * @param[in] port: i2c port
 * @param[in] i2c_pin: i2c scl pin number
 *
 *
 * @return void
 * */
void __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_E port, const TUYA_PIN_NAME_E scl_pin)
{
    if (port >= TUYA_I2C_NUM_MAX) {
        return;
    }

    sg_i2c_pin[port].scl = scl_pin;
}

/**
 * @brief set i2c sda pin
 * NOTE: call this API in tkl_io_pinmux_config
 * * *
 * @param[in] port: i2c port
 * @param[in] i2c_pin: i2c scl pin number
 *
 *
 * @return void
 * */
void __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_E port, const TUYA_PIN_NAME_E sda_pin)
{
    if (port >= TUYA_I2C_NUM_MAX) {
        return;
    }

    sg_i2c_pin[port].sda = sda_pin;
}
/**
 * @brief i2c init
 *
 * @param[in] i2c_pin: i2c pin number
 * @param[out] port: i2c pin number
 *
 * @return OPRT_OK on success, others on error
 */
OPERATE_RET tkl_i2c_init(uint8_t port, const TUYA_IIC_BASE_CFG_T *cfg)
{
    if (port >= TUYA_I2C_NUM_MAX) {
        return OPRT_INVALID_PARM;
    }

    if ((port == 0) || (port == 1)) {
        bk_printf("check i2c pin whether used by other function.\n");
    }

    if ((sg_i2c_pin[port].scl == TUYA_GPIO_NUM_MAX) || (sg_i2c_pin[port].sda == TUYA_GPIO_NUM_MAX)) {
        bk_printf("i2c pin not set, please use tkl_io_pinmux_config first!\n");
        return OPRT_INVALID_PARM;
    }

    if (cfg->role == TUYA_IIC_MODE_SLAVE) {
        bk_printf("i2c slave mode not supported!\n");
        return OPRT_NOT_SUPPORTED;
    }

    if (cfg->speed == TUYA_IIC_BUS_SPEED_100K) {
        sg_i2c_cfg[port].delay_us = 10;
    }else {
        sg_i2c_cfg[port].delay_us = 1;
    }

    if (cfg->addr_width == TUYA_IIC_ADDRESS_7BIT)
        sg_i2c_cfg[port].addr_width = 7;
    else {
        sg_i2c_cfg[port].addr_width = 10;
        return OPRT_NOT_SUPPORTED;
    }

    __sw_i2c_init(sg_i2c_pin[port]);

    return OPRT_OK;
}

/**
 * @brief i2c deinit
 *
 * @param[in] i2c_pin: i2c pin number
 *
 * @return OPRT_OK on success, others on error
 */
OPERATE_RET tkl_i2c_deinit(uint8_t port)
{
    if (port >= TUYA_I2C_NUM_MAX) {
        return OPRT_INVALID_PARM;
    }

    tkl_gpio_deinit(sg_i2c_pin[port].scl);
    tkl_gpio_deinit(sg_i2c_pin[port].sda);
    sg_i2c_pin[port].scl = TUYA_GPIO_NUM_MAX;
    sg_i2c_pin[port].sda = TUYA_GPIO_NUM_MAX;

    return OPRT_OK;
}

/**
 * @brief i2c irq init
 * NOTE: call this API will not enable interrupt
 *
 * @param[in] port: i2c port, id index starts at 0
 * @param[in] cb:  i2c irq cb
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_irq_init(TUYA_I2C_NUM_E port, TUYA_I2C_IRQ_CB cb)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief i2c irq enable
 *
 * @param[in] port: i2c port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_irq_enable(TUYA_I2C_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief i2c irq disable
 *
 * @param[in] port: i2c port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_irq_disable(TUYA_I2C_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief i2c master send
 *
 * @param[in] port: i2c port
 * @param[in] dev_addr: iic addrress of slave device.
 * @param[in] data: i2c data to send
 * @param[in] size: Number of data items to send
 * @param[in] xfer_pending: xfer_pending: TRUE : not send stop condition, FALSE : send stop condition.
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_master_send(TUYA_I2C_NUM_E port, USHORT_T dev_addr, const void *data, uint32_t size, BOOL_T xfer_pending)
{
    int ret;

    if (port >= TUYA_I2C_NUM_MAX) {
        bk_printf("i2c port %d is invalid\n", port);
        return OPRT_INVALID_PARM;
    }

    delay_us = sg_i2c_cfg[port].delay_us;
    ret = __sw_i2c_write_data(port, dev_addr, data, (uint8_t)size, xfer_pending);
    if(ret < 0)
        return OPRT_COM_ERROR;

    return OPRT_OK;
}

/**
 * @brief i2c master recv
 *
 * @param[in] port: i2c port
 * @param[in] dev_addr: iic addrress of slave device.
 * @param[in] data: i2c buf to recv
 * @param[in] size: Number of data items to receive
 * @param[in] xfer_pending: TRUE : not send stop condition, FALSE : send stop condition.
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_master_receive(TUYA_I2C_NUM_E port, USHORT_T dev_addr, void *data, uint32_t size, BOOL_T xfer_pending)
{
    int ret;
    if (port >= TUYA_I2C_NUM_MAX) {
        bk_printf("i2c port %d is invalid\n", port);
        return OPRT_INVALID_PARM;
    }

    delay_us = sg_i2c_cfg[port].delay_us;
    ret = __sw_i2c_read_data(port, dev_addr, data, (uint8_t)size, xfer_pending);
    if(ret < 0)
        return OPRT_COM_ERROR;

    return OPRT_OK;
}
/**
 * @brief i2c slave
 *
 * @param[in] port: i2c port
 * @param[in] dev_addr: slave device addr
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_set_slave_addr(TUYA_I2C_NUM_E port, USHORT_T dev_addr)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief i2c slave send
 *
 * @param[in] port: i2c port
 * @param[in] data: i2c buf to send
 * @param[in] size: Number of data items to send
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

OPERATE_RET tkl_i2c_slave_send(TUYA_I2C_NUM_E port, const void *data, uint32_t size)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief IIC slave receive, Start receiving data as IIC Slave.
 *
 * @param[in] port: i2c port
 * @param[in] data: Pointer to buffer for data to receive from IIC Master
 * @param[in] size: Number of data items to receive
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

OPERATE_RET tkl_i2c_slave_receive(TUYA_I2C_NUM_E port, void *data, uint32_t size)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief IIC get status.
 *
 * @param[in] port: i2c port
 * @param[out]  TUYA_IIC_STATUS_T
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_get_status(TUYA_I2C_NUM_E port, TUYA_IIC_STATUS_T *status)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief i2c's reset
 *
 * @param[in] port: i2c port number
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET  tkl_i2c_reset(TUYA_I2C_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief i2c transferred data count.
 *
 * @param[in] port: i2c port id, id index starts at 0
 *
 * @return >=0,number of currently transferred data items. <0,err.
 * tkl_i2c_master_send:number of data bytes transmitted and acknowledged
 * tkl_i2c_master_receive:number of data bytes received
 * tkl_i2c_slave_send:number of data bytes transmitted
 * tkl_i2c_slave_receive:number of data bytes received and acknowledged
 */
int tkl_i2c_get_data_count(TUYA_I2C_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief i2c ioctl
 *
 * @param[in]       cmd     user def
 * @param[in]       args    args associated with the command
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_ioctl(TUYA_I2C_NUM_E port, uint32_t cmd,  void *args)
{
    return OPRT_NOT_SUPPORTED;
}
