/**
 **************************************************************************************
 * @file    drv_ring_buffer.c
 * @brief   An implementation for ring buffer
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2017 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#include <string.h>
#include "drv_dma.h"
#include "drv_ring_buffer.h"

#define RWP_SAFE_INTERVAL   (4)

void ring_buffer_init(RingBufferContext* rb, uint8_t* addr, uint32_t capacity, void* dma, uint32_t dma_type)
{
    rb->address  = addr;
    rb->capacity = capacity;
    rb->wp       = 0;
    rb->rp       = 0;
    rb->dma      = dma;
    rb->dma_type = dma_type;

    if(dma)
    {
        if(rb->dma_type == RB_DMA_TYPE_READ)
        {
            dma_channel_src_curr_address_set(dma, (uint32_t)addr);
        }
        else if(rb->dma_type == RB_DMA_TYPE_WRITE)
        {
            dma_channel_dst_curr_address_set(dma, (uint32_t)addr + capacity);
        }

        #if 0
        if(!dma_channel_is_enabled(dma))
        {
            dma_channel_enable(dma, 1);
            dma_channel_enable(dma, 0);
        }
        #endif
    }
}

void ring_buffer_clear(RingBufferContext* rb)
{
    rb->wp = 0;
    rb->rp = 0;

    if(rb->dma)
    {
        if(rb->dma_type == RB_DMA_TYPE_READ)
        {
            dma_channel_src_curr_address_set(rb->dma, (uint32_t)rb->address);
        }
        else if(rb->dma_type == RB_DMA_TYPE_WRITE)
        {
            dma_channel_dst_curr_address_set(rb->dma, (uint32_t)rb->address + rb->capacity);
        }
    }
}

uint32_t ring_buffer_read(RingBufferContext* rb, uint8_t* buffer, uint32_t size)
{
    uint32_t required_bytes = size;
    uint32_t read_bytes;
    uint32_t remain_bytes;
    uint32_t wp;

    if(rb->dma && rb->dma_type == RB_DMA_TYPE_WRITE)
    {
        wp = rb->wp = dma_channel_is_enabled(rb->dma) ? dma_channel_dst_curr_pointer_get(rb->dma) - (uint32_t)rb->address : 0;
    }
    else
    {
        wp = rb->wp;
    }

    if(required_bytes == 0) return 0;

    if(wp >= rb->rp)
    {
        remain_bytes = wp - rb->rp;

        if(required_bytes > remain_bytes)
        {
            read_bytes = remain_bytes;
            memcpy(buffer, &rb->address[rb->rp], read_bytes);
            rb->rp += read_bytes;
        }
        else
        {
            read_bytes = required_bytes;
            memcpy(buffer, &rb->address[rb->rp], read_bytes);
            rb->rp += read_bytes;
        }
    }
    else
    {
        remain_bytes = rb->capacity - rb->rp;

        if(required_bytes > remain_bytes)
        {
            read_bytes = remain_bytes;
            memcpy(buffer, &rb->address[rb->rp], read_bytes);

            if(required_bytes - read_bytes > wp)
            {
                memcpy(buffer + read_bytes, &rb->address[0], wp);
                rb->rp = wp;
                read_bytes += wp;
            }
            else
            {
                memcpy(buffer + read_bytes, &rb->address[0], required_bytes - read_bytes);
                rb->rp = required_bytes - read_bytes;
                read_bytes = required_bytes;
            }
        }
        else
        {
            read_bytes = required_bytes;
            memcpy(buffer, &rb->address[rb->rp], read_bytes);
            rb->rp += read_bytes;
        }
    }

    #if 0
    if(rb->dma && rb->dma_type == RB_DMA_TYPE_WRITE)
    {
        dma_channel_dst_curr_address_set(rb->dma, (uint32_t)&rb->address[rb->rp]);
    }
    #endif

    return read_bytes;
}

uint32_t ring_buffer_write(RingBufferContext* rb, uint8_t* buffer, uint32_t size)
{
    uint32_t remain_bytes;
    uint32_t write_bytes = size;
    uint32_t rp;

    if(write_bytes == 0) return 0;

    if(rb->dma && rb->dma_type == RB_DMA_TYPE_READ)
    {
        rp = rb->rp = dma_channel_is_enabled(rb->dma) ? dma_channel_src_curr_pointer_get(rb->dma) - (uint32_t)rb->address : 0;
    }
    else
    {
        rp = rb->rp;
    }

    if(rb->wp >= rp)
    {
        remain_bytes = rb->capacity - rb->wp + rp;

        if(remain_bytes >= write_bytes + RWP_SAFE_INTERVAL)
        {
            remain_bytes = rb->capacity - rb->wp;

            if(remain_bytes >= write_bytes)
            {
                memcpy(&rb->address[rb->wp], buffer, write_bytes);
                rb->wp += write_bytes;
            }
            else
            {
                memcpy(&rb->address[rb->wp], buffer, remain_bytes);
                rb->wp = write_bytes - remain_bytes;
                memcpy(&rb->address[0], &buffer[remain_bytes], rb->wp);
            }
        }
        else
        {
            return 0;
        }
    }
    else
    {
        remain_bytes = rp - rb->wp;

        if(remain_bytes >= write_bytes + RWP_SAFE_INTERVAL)
        {
            memcpy(&rb->address[rb->wp], buffer, write_bytes);
            rb->wp += write_bytes;
        }
        else
        {
            return 0;
        }
    }

    if(rb->wp >= rb->capacity && rb->rp)
    {
        rb->wp = 0;
    }

    if(rb->dma && rb->dma_type == RB_DMA_TYPE_READ)
    {
        dma_channel_src_curr_address_set(rb->dma, (uint32_t)&rb->address[rb->wp]);
    }

    return write_bytes;
}

uint32_t ring_buffer_get_fill_size(RingBufferContext* rb)
{
    uint32_t rp, wp;
    uint32_t fill_size;

    if(rb->dma)
    {
        if(rb->dma_type == RB_DMA_TYPE_READ)
        {
            rp = rb->rp = dma_channel_is_enabled(rb->dma) ? dma_channel_src_curr_pointer_get(rb->dma) - (uint32_t)rb->address : 0;
            wp = rb->wp;
        }
        else if(rb->dma_type == RB_DMA_TYPE_WRITE)
        {
            rp = rb->rp;
            wp = rb->wp = dma_channel_is_enabled(rb->dma) ? dma_channel_dst_curr_pointer_get(rb->dma) - (uint32_t)rb->address : 0;
        }
        else
        {
            rp = rb->rp;
            wp = rb->wp;
        }
    }
    else
    {
        rp = rb->rp;
        wp = rb->wp;
    }

    fill_size = wp >= rp ? wp - rp : rb->capacity - rp + wp;

    return fill_size;
}

uint32_t ring_buffer_get_free_size(RingBufferContext* rb)
{
    uint32_t free_size;

    free_size = rb->capacity - ring_buffer_get_fill_size(rb);

    return free_size > RWP_SAFE_INTERVAL ? free_size - RWP_SAFE_INTERVAL : 0;
}

void ring_buffer_dma_write_enable(RingBufferContext* rb, uint32_t enable)
{
    if(rb->dma_type == RB_DMA_TYPE_WRITE)
    {
        dma_channel_dst_curr_address_set(rb->dma, (uint32_t)rb->address + rb->capacity * (!!enable));
    }
}
