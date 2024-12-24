#include "bkreg.h"
#include "drv_uart.h"

typedef struct _UartContext
{
    volatile uint32_t did;
    volatile uint32_t vid;
    volatile uint32_t ctrl;
    volatile uint32_t status;
    volatile uint32_t config;
    volatile uint32_t fifo_cfg;
    volatile uint32_t fifo_status;
    volatile uint32_t fifo_port;
    volatile uint32_t int_enable;
    volatile uint32_t int_status;
    volatile uint32_t flow_cfg;
    volatile uint32_t wake_cfg;
}UartContext;

#define UART_BASE_ADDR(idx)     ((idx == 0) ? MDU_UART0_BASE_ADDR : ((idx == 1) ? MDU_UART1_BASE_ADDR : MDU_UART2_BASE_ADDR))

int32_t uart_init(UART_PORT idx, uint32_t baud_rate, UART_DATA_BITS data_bits, UART_STOP_BITS stop_bits, UART_PARITY parity_mode)
{
    if(idx < UART_COUNT)
    {
        volatile UartContext* uart = (volatile UartContext*)UART_BASE_ADDR(idx);

        uint32_t baud_divisor = 26000000 / baud_rate - 1;

        //Parameters valid check
        //TODO

        uart->ctrl   = 1;
        uart->config = ((uint32_t)stop_bits << SFT_UART0_CONF_STOP_LEN)
                     | (((uint32_t)parity_mode & 0x1) << SFT_UART0_CONF_PAR_MODE)
                     | ((((uint32_t)parity_mode >> 1) & 0x1) << SFT_UART0_CONF_PAR_EN)
                     | ((uint32_t)data_bits << SFT_UART0_CONF_LEN)
                     | (baud_divisor << SFT_UART0_CONF_CLK_DIVID)
                     | (1 << SFT_UART0_CONF_RX_ENABLE)
                     | (1 << SFT_UART0_CONF_TX_ENABLE);

        uart->flow_cfg   = 0;
        uart->wake_cfg   = 0;
        uart->fifo_cfg   = ((0x80 << SFT_UART0_FIFO_CONF_RX_FIFO_THRESHOLD) | (0x80 << SFT_UART0_FIFO_CONF_TX_FIFO_THRESHOLD) | (0x0 << SFT_UART0_FIFO_CONF_RX_STOP_DETECT_TIME));
        uart->int_enable = MSK_UART0_INT_ENABLE_RX_STOP_END_MASK | MSK_UART0_INT_ENABLE_RX_FIFO_NEED_READ_MASK;

        return 0;
    }
    else
    {
        return -1;
    }
}

int32_t uart_ctrl(UART_PORT idx, uint32_t cmd, uint32_t arg)
{
    if(idx < UART_COUNT)
    {
        volatile UartContext* uart = (volatile UartContext*)UART_BASE_ADDR(idx);

        switch(cmd)
        {
        case UART_CTRL_CMD_SET_TX_FIFO_THRESHOLD:
            uart->fifo_cfg = (uart->fifo_cfg & ~MSK_UART0_FIFO_CONF_TX_FIFO_THRESHOLD) | ((arg & MAX_UART0_FIFO_CONF_TX_FIFO_THRESHOLD) << SFT_UART0_FIFO_CONF_TX_FIFO_THRESHOLD);
            break;
        case UART_CTRL_CMD_SET_RX_FIFO_THRESHOLD:
            uart->fifo_cfg = (uart->fifo_cfg & ~MSK_UART0_FIFO_CONF_RX_FIFO_THRESHOLD) | ((arg & MAX_UART0_FIFO_CONF_RX_FIFO_THRESHOLD) << SFT_UART0_FIFO_CONF_RX_FIFO_THRESHOLD);
            break;
        case UART_CTRL_CMD_SET_RX_STOP_DETECT_TIME:
            uart->fifo_cfg = (uart->fifo_cfg & ~MSK_UART0_FIFO_CONF_RX_STOP_DETECT_TIME) | ((arg & MAX_UART0_FIFO_CONF_RX_STOP_DETECT_TIME) << SFT_UART0_FIFO_CONF_RX_STOP_DETECT_TIME);
            break;
        case UART_CTRL_CMD_GET_TX_FIFO_COUNT:
            *(uint32_t*)arg = (uart->fifo_status & MSK_UART0_FIFO_STATUS_TX_FIFO_COUNT) >> SFT_UART0_FIFO_STATUS_TX_FIFO_COUNT;
            break;
        case UART_CTRL_CMD_GET_RX_FIFO_COUNT:
            *(uint32_t*)arg = (uart->fifo_status & MSK_UART0_FIFO_STATUS_RX_FIFO_COUNT) >> SFT_UART0_FIFO_STATUS_RX_FIFO_COUNT;
            break;
        case UART_CTRL_CMD_GET_FIFO_STATUS:
            *(uint32_t*)arg = uart->fifo_status;
            break;
        case UART_CTRL_CMD_SET_TX_DATA:
            uart->fifo_port = arg;
            break;
        case UART_CTRL_CMD_GET_RX_DATA:
            *(uint32_t*)arg = uart->fifo_port >> 8;
            break;
        case UART_CTRL_CMD_SET_INT_ENABLE:
            uart->int_enable = arg;
            break;
        case UART_CTRL_CMD_GET_INT_ENABLE:
            *(uint32_t*)arg = uart->int_enable;
            break;
        case UART_CTRL_CMD_GET_INT_STATUS:
            *(uint32_t*)arg = uart->int_status;
            break;
        case UART_CTRL_CMD_SET_INT_STATUS:
            uart->int_status = arg;
            break;
        case UART_CTRL_CMD_GET_RX_PORT_ADDR:
            *(uint32_t*)arg = ((uint32_t)&uart->fifo_port) + 1;
            break;
        case UART_CTRL_CMD_GET_TX_PORT_ADDR:
            *(uint32_t*)arg = ((uint32_t)&uart->fifo_port) + 0;
            break;
        case UART_CTRL_CMD_SET_FLOW_CTRL_REG:
            uart->flow_cfg = arg;
            break;
        case UART_CTRL_CMD_SET_FLOW_CTRL_ENABLE:
            uart->flow_cfg = (uart->flow_cfg & ~MSK_UART0_FLOW_CONF_FLOW_CONTROL_ENA) | ((uint32_t)(!!arg) << SFT_UART0_FLOW_CONF_FLOW_CONTROL_ENA);
            break;
        case UART_CTRL_CMD_SET_FLOW_CTRL_LOW_CNT:
            uart->flow_cfg = (uart->flow_cfg & ~MSK_UART0_FLOW_CONF_FLOW_CTL_LOW_CNT) | ((arg & MAX_UART0_FLOW_CONF_FLOW_CTL_LOW_CNT) << SFT_UART0_FLOW_CONF_FLOW_CTL_LOW_CNT);
            break;
        case UART_CTRL_CMD_SET_FLOW_CTRL_HIGH_CNT:
            uart->flow_cfg = (uart->flow_cfg & ~MSK_UART0_FLOW_CONF_FLOW_CTL_HIGH_CNT) | ((arg & MAX_UART0_FLOW_CONF_FLOW_CTL_HIGH_CNT) << SFT_UART0_FLOW_CONF_FLOW_CTL_HIGH_CNT);
            break;
        case UART_CTRL_CMD_SET_FLOW_CTRL_CTS_POLARITY:
            uart->flow_cfg = (uart->flow_cfg & ~MSK_UART0_FLOW_CONF_CTS_POLARITY_SEL) | ((uint32_t)(!!arg) << SFT_UART0_FLOW_CONF_CTS_POLARITY_SEL);
            break;
        case UART_CTRL_CMD_SET_FLOW_CTRL_RTS_POLARITY:
            uart->flow_cfg = (uart->flow_cfg & ~MSK_UART0_FLOW_CONF_RTS_POLARITY_SEL) | ((uint32_t)(!!arg) << SFT_UART0_FLOW_CONF_RTS_POLARITY_SEL);
            break;
        case UART_CTRL_CMD_SET_WAKE_CNT:
            uart->wake_cfg = (uart->wake_cfg & ~MSK_UART0_WAKE_CONF_WAKE_COUNT) | ((arg & MAX_UART0_WAKE_CONF_WAKE_COUNT) << SFT_UART0_WAKE_CONF_WAKE_COUNT);
            break;
        case UART_CTRL_CMD_SET_WAKE_TX_DELAY:
            uart->wake_cfg = (uart->wake_cfg & ~MSK_UART0_WAKE_CONF_TXD_WAIT_CNT) | ((arg & MAX_UART0_WAKE_CONF_TXD_WAIT_CNT) << SFT_UART0_WAKE_CONF_TXD_WAIT_CNT);
            break;
        case UART_CTRL_CMD_SET_WAKE_ENABLE:
            uart->wake_cfg = (uart->wake_cfg & ~(7U << 20)) | arg;
            break;
        default:
            break;
        }

        return 0;
    }
    else
    {
        return -1;
    }
}

int32_t uart_send(UART_PORT idx, uint8_t* buf, uint32_t len)
{
    if(idx < UART_COUNT)
    {
        uint32_t size = len;

        volatile UartContext* uart = (volatile UartContext*)UART_BASE_ADDR(idx);

        while(size)
        {
            if(uart->fifo_status & MSK_UART0_FIFO_STATUS_FIFO_WR_READY)
            {
                uart->fifo_port = *buf++;
                size--;
            }
        }

        return (int32_t)len;
    }
    else
    {
        return 0;
    }
}
