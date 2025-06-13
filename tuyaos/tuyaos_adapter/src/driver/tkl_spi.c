#include "driver/dma.h"
#include "tkl_spi.h"
#include <driver/spi.h>
#include <sdkconfig.h>

#define SPI_DMA_MAX_LEN 65535
static spi_config_t spi_config = {0};

struct spi_irq_config {
    uint8_t irq_enable;
    TUYA_SPI_IRQ_CB cb;
};
static struct spi_irq_config spi_irq[TUYA_SPI_NUM_MAX] = {0};

extern bk_err_t bk_spi_dma_read_bytes_async(spi_id_t id, void *data, uint32_t size);
extern bk_err_t bk_spi_dma_write_bytes_async(spi_id_t id, const void *data, uint32_t size);

bk_err_t tuya_bk_spi_dma_read_bytes_async(spi_id_t id, void *data, uint32_t size)
{
    return bk_spi_dma_read_bytes_async((spi_id_t)id, data, size);
}
bk_err_t tuya_bk_spi_dma_write_bytes_async(spi_id_t id, const void *data, uint32_t size)
{
    return bk_spi_dma_write_bytes_async((spi_id_t)id, data, size);
}

// rx isr callback
static void spi_tx_callback_dispatch(spi_id_t id, void *param)
{
    if (spi_irq[id].cb) {
        spi_irq[id].cb((TUYA_SPI_NUM_E)id, TUYA_SPI_EVENT_TX_COMPLETE);
    }
}

// tx isr callback
static void spi_rx_callback_dispatch(spi_id_t id, void *param)
{
    if (spi_irq[id].cb) {
        spi_irq[id].cb((TUYA_SPI_NUM_E)id, TUYA_SPI_EVENT_RX_COMPLETE);
    }
}

OPERATE_RET tkl_spi_init(TUYA_SPI_NUM_E port, const TUYA_SPI_BASE_CFG_T *cfg)
{
    if (port > TUYA_SPI_NUM_1) {
        return OPRT_INVALID_PARM;
    }

    if (cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    if(bk_spi_driver_init() != BK_OK)
        return OPRT_COM_ERROR;

    spi_config.wire_mode = SPI_4WIRE_MODE;
    spi_config.baud_rate = cfg->freq_hz;

#if (CONFIG_SPI_DMA)
    if (cfg->spi_dma_flags) {
        spi_config.dma_mode = SPI_DMA_MODE_ENABLE;

        if (port == TUYA_SPI_NUM_0) {
            spi_config.spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0);
            spi_config.spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0_RX);
        }

        if (port == TUYA_SPI_NUM_1) {
            spi_config.spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI1);
            spi_config.spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI1_RX);
        }
        spi_config.spi_tx_dma_width = DMA_DATA_WIDTH_8BITS;
        spi_config.spi_rx_dma_width = DMA_DATA_WIDTH_8BITS;
    }
#endif
    switch (cfg->role) {
        case TUYA_SPI_ROLE_SLAVE:
        case TUYA_SPI_ROLE_SLAVE_SIMPLEX:
            spi_config.role = SPI_ROLE_SLAVE;
            break;
        case TUYA_SPI_ROLE_MASTER:
        case TUYA_SPI_ROLE_MASTER_SIMPLEX:
        default:
            spi_config.role = SPI_ROLE_MASTER;
            break;
    }

    if (cfg->databits == TUYA_SPI_DATA_BIT16) {
        spi_config.bit_width = SPI_BIT_WIDTH_16BITS;
    } else {
        spi_config.bit_width = SPI_BIT_WIDTH_8BITS;
    }

    switch (cfg->mode) {
        case TUYA_SPI_MODE0:
            spi_config.polarity = SPI_POLARITY_LOW;
            spi_config.phase = SPI_PHASE_1ST_EDGE;
            break;
        case TUYA_SPI_MODE1:
            spi_config.polarity = SPI_POLARITY_LOW;
            spi_config.phase = SPI_PHASE_2ND_EDGE;
            break;
        case TUYA_SPI_MODE2:
            spi_config.polarity = SPI_POLARITY_HIGH;
            spi_config.phase = SPI_PHASE_1ST_EDGE;
            break;
        case TUYA_SPI_MODE3:
        default:
            spi_config.polarity = SPI_POLARITY_HIGH;
            spi_config.phase = SPI_PHASE_2ND_EDGE;
            break;
    }

    if (cfg->bitorder == TUYA_SPI_ORDER_LSB2MSB) {
        spi_config.bit_order = SPI_LSB_FIRST;
    } else {
        spi_config.bit_order = SPI_MSB_FIRST;
    }

    BK_RETURN_ON_ERR(bk_spi_init((spi_id_t)port, &spi_config));
    return OPRT_OK;
}

OPERATE_RET tkl_spi_deinit(TUYA_SPI_NUM_E port)
{
    BK_RETURN_ON_ERR(bk_spi_deinit((spi_id_t)port));
    memset(&spi_config, 0, sizeof(spi_config));
    return OPRT_OK;
}

OPERATE_RET tkl_spi_send(TUYA_SPI_NUM_E port, void *data, uint32_t size)
{
    bk_err_t ret = BK_OK;

    if (data == NULL || port > TUYA_SPI_NUM_1) {
        return OPRT_INVALID_PARM;
    }

#if (CONFIG_SPI_DMA)
    if (SPI_DMA_MODE_ENABLE == spi_config.dma_mode) {
        if (spi_irq[port].irq_enable) {
            ret = tuya_bk_spi_dma_write_bytes_async((spi_id_t)port, data, size);
        } else {
            ret = bk_spi_dma_write_bytes((spi_id_t)port, data, size);
        }

        if (ret != BK_OK)
            return OPRT_COM_ERROR;
    } else
#endif
    {
        if (spi_irq[port].irq_enable) {
            ret = bk_spi_write_bytes_async((spi_id_t)port, data, size);
        } else {
            ret = bk_spi_write_bytes((spi_id_t)port, data, size);
        }

        if (ret != BK_OK)
            return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_spi_recv(TUYA_SPI_NUM_E port, VOID *data, uint32_t size)
{
    bk_err_t ret = BK_OK;

    if (data == NULL || port > TUYA_SPI_NUM_1) {
        return OPRT_INVALID_PARM;
    }

#if (CONFIG_SPI_DMA)
    if (SPI_DMA_MODE_ENABLE == spi_config.dma_mode) {
        if (spi_irq[port].irq_enable) {
            ret = tuya_bk_spi_dma_read_bytes_async((spi_id_t)port, data, size);
        } else {
            ret = bk_spi_dma_read_bytes((spi_id_t)port, data, size);
        }

        if (ret != BK_OK)
            return OPRT_COM_ERROR;
    } else
#endif
    {
        if (spi_irq[port].irq_enable) {
            ret = bk_spi_read_bytes_async((spi_id_t)port, data, size);
        } else {
            ret = bk_spi_read_bytes((spi_id_t)port, data, size);
        }

        if (ret != BK_OK)
            return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_spi_transfer(TUYA_SPI_NUM_E port, void* send_buf, void* receive_buf, uint32_t length)
{
    if (port > TUYA_SPI_NUM_1) {
        return OPRT_INVALID_PARM;
    }

#if (CONFIG_SPI_DMA)
    if (SPI_DMA_MODE_ENABLE == spi_config.dma_mode) {
        // if (length >= SPI_DMA_MAX_LEN) {
        //     return OPRT_INVALID_PARM;
        // }

        bk_spi_dma_duplex_init(port);
        bk_err_t ret = bk_spi_dma_duplex_xfer(port, send_buf, length, receive_buf, length);
        bk_spi_dma_duplex_deinit(port);
        if (ret != BK_OK) {
            return OPRT_COM_ERROR;
        }
    } else {
#endif
        if (length >= 4096) {
            return OPRT_INVALID_PARM;
        }

        if (bk_spi_transmit((spi_id_t)port, send_buf, length, receive_buf, length) != BK_OK)
            return OPRT_COM_ERROR;
#if (CONFIG_SPI_DMA)
    }
#endif
    return OPRT_OK;
}

OPERATE_RET tkl_spi_abort_transfer(TUYA_SPI_NUM_E port)
{
    if (port > TUYA_SPI_NUM_1) {
        return OPRT_INVALID_PARM;
    }

    if (bk_spi_deinit((spi_id_t)port) != BK_OK)
        return OPRT_COM_ERROR;
    return OPRT_OK;
}

/**
 * @brief spi irq init
 * NOTE: call this API will not enable interrupt
 *
 * @param[in] port: spi port, id index starts at 0
 * @param[in] cb:  spi irq cb
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_irq_init(TUYA_SPI_NUM_E port, TUYA_SPI_IRQ_CB cb)
{
    if (port > TUYA_SPI_NUM_1) {
        return OPRT_INVALID_PARM;
    }

    spi_irq[port].cb = cb;
    spi_irq[port].irq_enable = 0;

    return OPRT_OK;
}

/**
 * @brief spi irq enable
 *
 * @param[in] port: spi port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_irq_enable(TUYA_SPI_NUM_E port)
{

    bk_spi_register_tx_finish_isr((spi_id_t)port, spi_tx_callback_dispatch, NULL);
    bk_spi_register_rx_finish_isr((spi_id_t)port, spi_rx_callback_dispatch, NULL);

    spi_irq[port].irq_enable = 1;

    return OPRT_OK;
}

/**
 * @brief spi irq disable
 *
 * @param[in] port: spi port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_irq_disable(TUYA_SPI_NUM_E port)
{
    bk_spi_register_tx_finish_isr((spi_id_t)port, NULL, NULL);
    bk_spi_register_rx_finish_isr((spi_id_t)port, NULL, NULL);

    spi_irq[port].irq_enable = 0;

    return OPRT_OK;
}

/**
 * @brief spi get max supported dma data length.
 * 
 * @param[in] NULL
 *
 * @return >=0,number of supported dma data length. <0,err. 
 * during  tkl_spi_send, tkl_spi_recv and tkl_spi_transfer operation.
 */
uint32_t  tkl_spi_get_max_dma_data_length(void)
{
    return SPI_DMA_MAX_LEN;
}
