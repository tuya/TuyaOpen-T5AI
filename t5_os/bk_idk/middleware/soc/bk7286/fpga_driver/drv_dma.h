/**
 **************************************************************************************
 * @file    drv_dma.h
 * @brief   Driver API for DMA
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2019 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __DRV_DMA_H__
#define __DRV_DMA_H__

#include <stdint.h>

/**
 * @brief DMA request definition
 */
typedef enum
{
    DMA_REQ_MEM = 0,
    DMA_REQ_SDIO_RX,
    DMA_REQ_PSRAM_VIDEO_READ,
    DMA_REQ_PSRAM_AUDIO_READ,
    DMA_REQ_UART0_RX,
    DMA_REQ_UART1_RX,
    DMA_REQ_SPI2_RX,
    DMA_REQ_I2S2_RX,
    DMA_REQ_AUDIO_ADC,
    DMA_REQ_AUDIO_ENC,
    DMA_REQ_LOGICAL_ANALYSIS,
    DMA_REQ_SDIO_TX           = 1  << 5,
    DMA_REQ_PSRAM_VIDEO_WRITE = 2  << 5,
    DMA_REQ_PSRAM_AUDIO_WRITE = 3  << 5,
    DMA_REQ_UART0_TX          = 4  << 5,
    DMA_REQ_UART1_TX          = 5  << 5,
    DMA_REQ_SPI2_TX           = 6  << 5,
    DMA_REQ_I2S2_TX           = 7  << 5,
    DMA_REQ_AUDIO_DAC         = 9  << 5,
}DMA_REQ;

/**
 * @brief DMA transfer mode definition
 */
typedef enum
{
    DMA_MODE_SINGLE = 0,
    DMA_MODE_REPEAT,
}DMA_MODE;

/**
 * @brief DMA address mode definition
 */
typedef enum
{
    DMA_ADDR_NO_CHANGE     = 0,
    DMA_ADDR_AUTO_INCREASE = 1,
}DMA_ADDR_MODE;

/**
 * @brief DMA data type (or width) definition
 */
typedef enum
{
    DMA_DATA_TYPE_CHAR = 0,
    DMA_DATA_TYPE_SHORT,
    DMA_DATA_TYPE_LONG,
}DMA_DATA_TYPE;

/**
 * @brief DMA control command definition
 */
typedef enum
{
    DMA_CTRL_CMD_NULL,
    DMA_CTRL_CMD_PRIOPRITY_SET,
    DMA_CTRL_CMD_FINISH_INT_EN,
    DMA_CTRL_CMD_HALF_FINISH_INT_EN,
    DMA_CTRL_CMD_FINISH_INT_FLAG_GET,
    DMA_CTRL_CMD_HALF_FINISH_INT_FLAG_GET,
    DMA_CTRL_CMD_SRC_RD_INTERVAL_SET,
    DMA_CTRL_CMD_DST_WR_INTERVAL_SET,
    DMA_CTRL_CMD_REMAIN_LENGTH_GET,
    DMA_CTRL_CMD_SRC_BUFF_FLUSH,
    DMA_CTRL_CMD_FINISH_INT_FLAG_CLEAR,
    DMA_CTRL_CMD_HALF_FINISH_INT_FLAG_CLEAR,
    DMA_CTRL_CMD_FINISH_INT_EN_GET,
    DMA_CTRL_CMD_HALF_FINISH_INT_EN_GET,
}DMA_CTRL_CMD;

/**
 * @brief DMA channel malloc
 * @return DMA channel handler
 */
void* dma_channel_malloc(void);

/**
 * @brief DMA channel free
 * @param dma DMA channel handler
 */
void  dma_channel_free(void* dma);

/**
 * @brief DMA channel configuration
 * @param dma  DMA channel handler
 * @param req  DMA request mode, @see DMA_REQ
 * @param mode DMA transfer mode, @see DMA_MODE
 * @param src_start_addr src start address
 * @param src_stop_addr  src stop  address
 * @param src_addr_mode  src address mode, @see DMA_ADDR_MODE
 * @param src_data_type  src data type, @see DMA_DATA_TYPE
 * @param dst_start_addr dst start address
 * @param dst_stop_addr  dst stop  address
 * @param dst_addr_mode  dst address mode, @see DMA_ADDR_MODE
 * @param dst_data_type  dst data type, @see DMA_DATA_TYPE
 * @param data_length    transfer data length
 */
void dma_channel_config(void*           dma,
                        DMA_REQ         req,
                        DMA_MODE        mode,
                        uint32_t        src_start_addr,
                        uint32_t        src_stop_addr,
                        DMA_ADDR_MODE   src_addr_mode,
                        DMA_DATA_TYPE   src_data_type,
                        uint32_t        dst_start_addr,
                        uint32_t        dst_stop_addr,
                        DMA_ADDR_MODE   dst_addr_mode,
                        DMA_DATA_TYPE   dst_data_type,
                        uint32_t        data_length
                       );

void dma_channel_single_config(void*   _dma,
                        DMA_REQ         req,
                        DMA_ADDR_MODE   src_addr_mode,
                        DMA_DATA_TYPE   src_data_type,
                        DMA_ADDR_MODE   dst_addr_mode,
                        DMA_DATA_TYPE   dst_data_type,
                        uint32_t        int_enable
                       );

void dma_channel_single_start(void*    _dma,
                        uint32_t        src_addr,
                        uint32_t        dst_addr,
                        uint32_t        data_length
                       );

/**
 * @brief DMA channel control
 * @param dma DMA channel handler
 * @param dma control command
 * @param dma control argument
 */
void dma_channel_ctrl(void* dma, uint32_t cmd, uint32_t arg);

/**
 * @brief DMA channel enable
 * @param dma DMA channel handler
 * @param enable 0:disable, 1:enable
 */
void dma_channel_enable(void* dma, uint32_t enable);

/**
 * @brief Check DMA channel is enabled
 * @param dma DMA channel handler
 * @return 0:disable, 1:enable
 */
uint32_t dma_channel_is_enabled(void* dma);

/**
 * @brief Set current source address
 * @param dma DMA channel handler
 * @param addr address
 */
void dma_channel_src_curr_address_set(void* dma, uint32_t addr);

/**
 * @brief Set current destination address
 * @param dma DMA channel handler
 * @param addr address
 */
void dma_channel_dst_curr_address_set(void* dma, uint32_t addr);

/**
 * @brief Get current source read address
 * @param dma DMA channel handler
 * @return current source read address
 */
uint32_t dma_channel_src_curr_pointer_get(void* dma);

/**
 * @brief Get current destination write address
 * @param dma DMA channel handler
 * @return current destination write address
 */
uint32_t dma_channel_dst_curr_pointer_get(void* dma);

#endif//__DRV_DMA_H__
