/**
 **************************************************************************************
 * @file    drv_ring_buffer.h
 * @brief   An implementation for ring buffer
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2017 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __DRV_RING_BUFFER_H__
#define __DRV_RING_BUFFER_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include <stdint.h>

/**
 * @brief DMA type definition
 */
typedef enum
{
    RB_DMA_TYPE_NULL  = 0,
    RB_DMA_TYPE_READ  = 1,
    RB_DMA_TYPE_WRITE = 2,
}RB_DMA_TYPE;

/**
 * @brief ring buffer context definition
 */
typedef struct _RingBufferContext
{
    uint8_t* address;     /**< memory base address      */
    uint32_t capacity;    /**< memory capacity in bytes */
    uint32_t wp;          /**< write point in bytes     */
    uint32_t rp;          /**< read point in bytes      */
    void*    dma;
    uint32_t dma_type;
}RingBufferContext;

/**
 * @brief ring buffer initialization
 * @param rb   ring buffer context pointer
 * @param addr ring buffer start address
 * @param capacity ring buffer capacity
 * @param dma dma handler/pointer
 * @param dma_type dma tyepe, @see RB_DMA_TYPE
 */
void ring_buffer_init(RingBufferContext* rb, uint8_t* addr, uint32_t capacity, void* dma, uint32_t dma_type);

/**
 * @brief ring buffer clear
 * @param rb   ring buffer context pointer
 */
void ring_buffer_clear(RingBufferContext* rb);

/**
 * @brief ring buffer read
 * @param rb     ring buffer context pointer
 * @param buffer buffer to be read into
 * @param size   read size
 * @return actually read size
 */
uint32_t ring_buffer_read(RingBufferContext* rb, uint8_t* buffer, uint32_t size);

/**
 * @brief ring buffer write
 * @param rb     ring buffer context pointer
 * @param buffer buffer to be write from
 * @param size   write size
 * @return actually write size
 */
uint32_t ring_buffer_write(RingBufferContext* rb, uint8_t* buffer, uint32_t size);

/**
 * @brief get filled size of ring buffer
 * @param rb     ring buffer context pointer
 * @return the filled size of ring buffer
 */
uint32_t ring_buffer_get_fill_size(RingBufferContext* rb);

/**
 * @brief get free size of ring buffer
 * @param rb     ring buffer context pointer
 * @return the free size of ring buffer
 */
uint32_t ring_buffer_get_free_size(RingBufferContext* rb);

/**
 * @brief Enable/Disable ring buffer dma write
 * @param rb     ring buffer context pointer
 * @param enable dma write enable
 * @return the free size of ring buffer
 */
void ring_buffer_dma_write_enable(RingBufferContext* rb, uint32_t enable);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__DRV_RING_BUFFER_H__
