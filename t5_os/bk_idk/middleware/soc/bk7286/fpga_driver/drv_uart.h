#ifndef __DRV_UART_H__
#define __DRV_UART_H__

#include <stdint.h>

/**
 * @brief UART definition
 */
typedef enum
{
    UART0,
    UART1,
    UART2,
    UART_COUNT,
}UART_PORT;

/**
 * @brief UART data bits definition
 */
typedef enum
{
    UART_DATA_BITS_5 = 0,
    UART_DATA_BITS_6,
    UART_DATA_BITS_7,
    UART_DATA_BITS_8,
}UART_DATA_BITS;

/**
 * @brief UART stop bits definition
 */
typedef enum
{
    UART_STOP_BITS_1 = 0,
    UART_STOP_BITS_2,
}UART_STOP_BITS;

/**
 * @brief UART parity mode definition
 */
typedef enum
{
    UART_PARITY_NONE = 0,
    UART_PARITY_ODD  = 2,
    UART_PARITY_EVEN = 3,
}UART_PARITY;

/**
 * @brief UART FIFO status definition
 */
typedef enum
{
    UART_FIFO_STATUS_TX_FULL  = 1 << 16,
    UART_FIFO_STATUS_TX_EMPTY = 1 << 17,
    UART_FIFO_STATUS_RX_FULL  = 1 << 18,
    UART_FIFO_STATUS_RX_EMPTY = 1 << 19,
    UART_FIFO_STATUS_TX_READY = 1 << 20,
    UART_FIFO_STATUS_RX_READY = 1 << 21,
}UART_FIFO_STATUS;

/**
 * @brief UART interrupt definition
 */
typedef enum
{
    UART_INT_FIFO_NEED_WRITE = 1 << 0,
    UART_INT_FIFO_NEED_READ  = 1 << 1,
    UART_INT_FIFO_OVERFLOW   = 1 << 2,
    UART_INT_RX_PARITY_ERROR = 1 << 3,
    UART_INT_RX_STOP_ERROR   = 1 << 4,
    UART_INT_TX_DONE         = 1 << 5,
    UART_INT_RX_DONE         = 1 << 6,
    UART_INT_RX_WAKEUP       = 1 << 7,
}UART_INT;

/**
 * @brief UART wake sources definition
 */
typedef enum
{
    UART_WAKE_SRC_RX = 1 << 20,
    UART_WAKE_SRC_TX = 1 << 21,
    UART_WAKE_SRC_RX_NEGEDGE = 1 << 22,
}UART_WAKE_SRC;

/**
 * @brief UART control command definition
 */
typedef enum
{
    UART_CTRL_CMD_SET_TX_FIFO_THRESHOLD,
    UART_CTRL_CMD_SET_RX_FIFO_THRESHOLD,
    UART_CTRL_CMD_SET_RX_STOP_DETECT_TIME,
    UART_CTRL_CMD_GET_TX_FIFO_COUNT,
    UART_CTRL_CMD_GET_RX_FIFO_COUNT,
    UART_CTRL_CMD_GET_FIFO_STATUS,
    UART_CTRL_CMD_SET_TX_DATA,
    UART_CTRL_CMD_GET_RX_DATA,
    UART_CTRL_CMD_GET_INT_ENABLE,
    UART_CTRL_CMD_SET_INT_ENABLE,
    UART_CTRL_CMD_GET_INT_STATUS,
    UART_CTRL_CMD_SET_INT_STATUS,
    UART_CTRL_CMD_GET_RX_PORT_ADDR,
    UART_CTRL_CMD_GET_TX_PORT_ADDR,
    UART_CTRL_CMD_SET_FLOW_CTRL_REG,
    UART_CTRL_CMD_SET_FLOW_CTRL_ENABLE,
    UART_CTRL_CMD_SET_FLOW_CTRL_LOW_CNT,
    UART_CTRL_CMD_SET_FLOW_CTRL_HIGH_CNT,
    UART_CTRL_CMD_SET_FLOW_CTRL_CTS_POLARITY,
    UART_CTRL_CMD_SET_FLOW_CTRL_RTS_POLARITY,
    UART_CTRL_CMD_SET_WAKE_CNT,
    UART_CTRL_CMD_SET_WAKE_TX_DELAY,
    UART_CTRL_CMD_SET_WAKE_ENABLE,

}UART_CTRL_CMD;

/**
 * @brief UART initialization
 * @param idx  UART port
 * @param baud_rate uart baud rate, range from 3.2KHz ~ 3.25MHz
 * @param data_bits uart data bits, @see UART_DATA_BITS
 * @param stop_bits uart stop bits, @see UART_STOP_BITS
 * @param parity_mode uart parity mode, @see UART_PARITY
 * @return 0:SUCCESS, others:FAILURE
 */
int32_t uart_init(UART_PORT idx, uint32_t baud_rate, UART_DATA_BITS data_bits, UART_STOP_BITS stop_bits, UART_PARITY parity_mode);

/**
 * @brief UART control
 * @param idx  UART port
 * @param cmd uart control command
 * @param arg uart control argument
 * @return 0:SUCCESS, others:FAILURE
 */
int32_t uart_ctrl(UART_PORT idx, uint32_t cmd, uint32_t arg);

/**
 * @brief UART send
 * @param idx  UART port
 * @param buf send buffer
 * @param len bytes to be sent
 * @return actually send bytes
 */
int32_t uart_send(UART_PORT idx, uint8_t* buf, uint32_t len);

/**
 * @brief UART receive
 * @param idx  UART port
 * @param buf received buffer
 * @param len bytes to be received
 * @param timeout received timeout
 * @return actually received bytes
 */
int32_t uart_recv(UART_PORT idx, uint8_t* buf, uint32_t len, uint32_t timeout);

#endif//__DRV_UART_H__
