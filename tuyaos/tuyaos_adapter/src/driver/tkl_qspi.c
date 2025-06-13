/**
 * @file tkl_qspi.c
 * @brief default weak implements of tuya pin
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#include "driver/dma.h"
#include "tkl_qspi.h"
#include <driver/qspi.h>
#include <sdkconfig.h>


#define TUYA_QSPI_CLK_DIV   (0x2)
#define QSPI_INIT_CLK_480M (480000000)
struct qspi_irq_config {
    uint8_t irq_enable;
    TUYA_QSPI_IRQ_CB cb;
};

static struct qspi_irq_config qspi_irq[TUYA_QSPI_NUM_MAX] = {0};
static TUYA_QSPI_BASE_CFG_T qspi_base_config[TUYA_QSPI_NUM_MAX] = {0};

// rx isr callback
static void qspi_tx_callback_dispatch(TUYA_QSPI_NUM_E id, void *param)
{
    if (qspi_irq[id].cb) {
        qspi_irq[id].cb((TUYA_QSPI_NUM_E)id, TUYA_QSPI_EVENT_TX);
    }
}

// tx isr callback
static void qspi_rx_callback_dispatch(TUYA_QSPI_NUM_E id, void *param)
{
    if (qspi_irq[id].cb) {
        qspi_irq[id].cb((TUYA_QSPI_NUM_E)id, TUYA_QSPI_EVENT_RX);
    }
}

/**
 * @brief Init the QSPI
 * NOTE: 
 *
 * @param[in] port: qspi port, id index starts at 0
 * @param[in] cfg:  QSPI parameter settings
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_qspi_init(TUYA_QSPI_NUM_E port, const TUYA_QSPI_BASE_CFG_T *cfg)
{
    qspi_config_t config = {0};
    if ((port > TUYA_QSPI_NUM_MAX) || (cfg == NULL)) {
        return OPRT_INVALID_PARM;
    }

    if (cfg->is_dma) {
       bk_printf("QSPI DMA mode is not supported yet!\n");
    }
    
    if(bk_qspi_driver_init() != BK_OK)
        return OPRT_COM_ERROR;

    config.src_clk = QSPI_SCLK_480M;
    config.src_clk_div = TUYA_QSPI_CLK_DIV;
    config.clk_div = (QSPI_INIT_CLK_480M / TUYA_QSPI_CLK_DIV / cfg->baudrate);

    if(bk_qspi_init(port, &config) != BK_OK)
        return OPRT_COM_ERROR;
    
    memcpy(&qspi_base_config[port], cfg, sizeof(TUYA_QSPI_BASE_CFG_T));
    return OPRT_OK;
}

/**
 * @brief Deinit the QSPI driver
 * NOTE: 
 *
 * @param[in] port: qspi port, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_qspi_deinit(TUYA_QSPI_NUM_E port)
{
    if (port > TUYA_QSPI_NUM_MAX) {
        return OPRT_INVALID_PARM;
    }
    if(bk_qspi_deinit(port) != BK_OK)
        return OPRT_COM_ERROR;
        
    memset(&qspi_base_config[port], 0, sizeof(TUYA_QSPI_BASE_CFG_T));
    return OPRT_OK;
}

/**
 * @brief qspi write data
 * NOTE: 
 *
 * @param[in] port: qspi port, id index starts at 0
 * @param[in] data:  address of buffer
 * @param[in] size:  size of read
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_qspi_send(TUYA_QSPI_NUM_E port, void *data, uint16_t size)
{
    bk_err_t ret = BK_OK;

    if ((data == NULL) || (port > TUYA_QSPI_NUM_MAX) || (size > MAX_QSPI_FIFO_SIZE)) {
        return OPRT_INVALID_PARM;
    }

    ret = bk_qspi_write(port, data, size);
    if (ret != BK_OK)
        return OPRT_COM_ERROR;
    return OPRT_OK;
}

OPERATE_RET tkl_qspi_send_cmd(TUYA_QSPI_NUM_E port, uint8_t cmd)
{
    bk_qspi_write_cmd(port, cmd);
}

OPERATE_RET tkl_qspi_send_data_indirect_mode(TUYA_QSPI_NUM_E port, uint8_t *data, uint32_t data_len)
{
    bk_qspi_write_data_indirect_mode(port, data, data_len);
}

/**
 * @brief qspi read from addr by mapping mode
 * NOTE: 
 *
 * @param[in] port: qspi port, id index starts at 0
 * @param[out] data:  address of buffer
 * @param[in] size:  size of read
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_qspi_recv(TUYA_QSPI_NUM_E port, void *data, uint16_t size)
{
    bk_err_t ret = BK_OK;

    if ((data == NULL) || (port > TUYA_QSPI_NUM_MAX) || (size > MAX_QSPI_FIFO_SIZE)) {
        return OPRT_INVALID_PARM;
    }

    ret = bk_qspi_read(port, data, size);
    if (ret != BK_OK)
        return OPRT_COM_ERROR;
    return OPRT_OK;
}

/**
 * @brief qspi command send
 * NOTE: 
 *
 * @param[in] port: qspi port, id index starts at 0
 * @param[in] command:  qspi command configure
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_qspi_comand(TUYA_QSPI_NUM_E port, TUYA_QSPI_CMD_T *command)
{
    bk_err_t ret = BK_OK;
    qspi_cmd_t cmd = {0};
    if ((command == NULL) || (port > TUYA_QSPI_NUM_MAX)) {
        return OPRT_INVALID_PARM;
    }
    cmd.op = command->op;
    cmd.cmd = command->cmd;
    cmd.addr = command->addr;
    cmd.addr_valid_bit = command->addr_size;
    cmd.data_len = command->data_len;
    cmd.dummy_cycle = command->dummy_cycle;
    cmd.wire_mode = command->data_lines;
    cmd.work_mode = INDIRECT_MODE;
    cmd.device = QSPI_FLASH;
    // ret = tuya_qspi_hal_command(port, &cmd);
    ret = bk_qspi_command(port, &cmd);
    if (ret != BK_OK)
        return OPRT_COM_ERROR;
    return OPRT_OK;
}

OPERATE_RET tkl_qspi_abort_transfer(TUYA_QSPI_NUM_E port)
{
    if (port > TUYA_QSPI_NUM_MAX) {
        return OPRT_INVALID_PARM;
    }

    if (bk_qspi_deinit((spi_id_t)port) != BK_OK)
        return OPRT_COM_ERROR;
    return OPRT_OK;
}

/**
 * @brief qspi irq init
 * NOTE: call this API will not enable interrupt
 *
 * @param[in] port: qspi port, id index starts at 0
 * @param[in] cb:  qspi irq cb
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_qspi_irq_init(TUYA_QSPI_NUM_E port, TUYA_QSPI_IRQ_CB cb)
{
    if (port > TUYA_QSPI_NUM_MAX) {
        return OPRT_INVALID_PARM;
    }

    qspi_irq[port].cb = cb;
    qspi_irq[port].irq_enable = 0;

    return OPRT_OK;
}

/**
 * @brief qspi irq enable
 *
 * @param[in] port: qspi port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_qspi_irq_enable(TUYA_QSPI_NUM_E port)
{
    if (port > TUYA_QSPI_NUM_MAX) {
        return OPRT_INVALID_PARM;
    }
    bk_qspi_register_tx_isr(qspi_tx_callback_dispatch, NULL);
    bk_qspi_register_rx_isr(qspi_rx_callback_dispatch, NULL);

    qspi_irq[port].irq_enable = 1;

    return OPRT_OK;
}

/**
 * @brief qspi irq disable
 *
 * @param[in] port: qspi port id, id index starts at 0
 *k
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_qspi_irq_disable(TUYA_QSPI_NUM_E port)
{
    if (port > TUYA_QSPI_NUM_MAX) {
        return OPRT_INVALID_PARM;
    }
    bk_qspi_register_tx_isr(NULL, NULL);
    bk_qspi_register_rx_isr(NULL, NULL);

    qspi_irq[port].irq_enable = 0;

    return OPRT_OK;
}
