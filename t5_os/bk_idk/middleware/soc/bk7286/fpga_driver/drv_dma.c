/**
 **************************************************************************************
 * @file    drv_dma.c
 * @brief   Driver API for DMA
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2019 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#include <stddef.h>
#include "bkreg.h"
#include "drv_dma.h"

#define DMA_CHANNEL_NUM     (12)
#define DMA_USED_FLAG_MSK   (1 << 15)
#define dmac                ((volatile DMAContext*)MDU_GENER_DMA_BASE_ADDR)

typedef struct _DMAConfig
{
    volatile uint32_t cfg0;
    volatile uint32_t dst_start_addr;
    volatile uint32_t src_start_addr;
    volatile uint32_t dst_loop_stop_addr;
    volatile uint32_t dst_loop_start_addr;
    volatile uint32_t src_loop_stop_addr;
    volatile uint32_t src_loop_start_addr;
    volatile uint32_t cfg1;
}DMAConfig;

typedef struct _DMAContext
{
    volatile DMAConfig config[DMA_CHANNEL_NUM];

    volatile uint32_t  src_address[DMA_CHANNEL_NUM];
    volatile uint32_t  dst_address[DMA_CHANNEL_NUM];
    volatile uint32_t  src_pointer[DMA_CHANNEL_NUM];
    volatile uint32_t  dst_pointer[DMA_CHANNEL_NUM];

    volatile uint32_t  status[DMA_CHANNEL_NUM];

    volatile uint32_t  prio_mode; //0:round-robin, 1:fixed prioprity
    volatile uint32_t  finish_int_flag;
}DMAContext;

void* dma_channel_malloc(void)
{
    uint32_t i;

    for(i = 0; i < DMA_CHANNEL_NUM; i++)
    {
        if(!(dmac->config[i].cfg0 & DMA_USED_FLAG_MSK))
        {
            dmac->config[i].cfg0 |= DMA_USED_FLAG_MSK;
            break;
        }
    }

    return i < DMA_CHANNEL_NUM ? (void*)&dmac->config[i] : NULL;
}

void dma_channel_free(void* dma)
{
    uint32_t i = ((uint32_t)dma - (uint32_t)&dmac->config[0]) / sizeof(DMAConfig);

    if(i < DMA_CHANNEL_NUM && (uint32_t)&dmac->config[i] == (uint32_t)dma)
    {
        dmac->config[i].cfg0 &= ~DMA_USED_FLAG_MSK;
        dma = NULL;
    }
}

void dma_channel_config(void*           _dma,
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
                       )
{
    if(_dma)
    {
        uint32_t   idx = ((uint32_t)_dma - (uint32_t)&dmac->config[0]) / sizeof(DMAConfig);
        DMAConfig* dma = (DMAConfig*)_dma;

        dma->cfg0                = (mode << SFT_GENER_DMA_0x10_DMA_MODE)                |
                                   (src_data_type << SFT_GENER_DMA_0x10_SRC_DATA_WIDTH) |
                                   (dst_data_type << SFT_GENER_DMA_0x10_DEST_DATA_WIDTH)|
                                   (src_addr_mode << SFT_GENER_DMA_0x10_SRC_ADDR_INC)   |
                                   (dst_addr_mode << SFT_GENER_DMA_0x10_DEST_ADDR_INC)  |
                                   (src_addr_mode << SFT_GENER_DMA_0x10_SRC_ADDR_LOOP)  |
                                   (dst_addr_mode << SFT_GENER_DMA_0x10_DEST_ADDR_LOOP) |
                                   ((data_length - 1) << SFT_GENER_DMA_0x10_TRANS_LEN)  |
                                   (DMA_USED_FLAG_MSK);
        dma->cfg1                = (dma->cfg1 & ~(MSK_GENER_DMA_0x17_SRC_REQ_MUX | MSK_GENER_DMA_0x17_DEST_REQ_MUX)) | req;
        dma->src_start_addr      = src_start_addr;
        dma->src_loop_start_addr = src_start_addr;
        dma->src_loop_stop_addr  = src_stop_addr;
        dma->dst_start_addr      = dst_start_addr;
        dma->dst_loop_start_addr = dst_start_addr;
        dma->dst_loop_stop_addr  = dst_stop_addr;

        dmac->src_address[idx] = src_addr_mode == DMA_ADDR_NO_CHANGE ? 0 : src_start_addr;
        dmac->dst_address[idx] = dst_addr_mode == DMA_ADDR_NO_CHANGE ? 0 : dst_start_addr;
    }
}

void dma_channel_single_config(void*           _dma,
                               DMA_REQ         req,
                               DMA_ADDR_MODE   src_addr_mode,
                               DMA_DATA_TYPE   src_data_type,
                               DMA_ADDR_MODE   dst_addr_mode,
                               DMA_DATA_TYPE   dst_data_type,
                               uint32_t        int_enable
                               )
{
    if(_dma)
    {
        DMAConfig* dma = (DMAConfig*)_dma;

        dma->cfg0 = (DMA_MODE_SINGLE << SFT_GENER_DMA_0x10_DMA_MODE)     |
                    (src_data_type << SFT_GENER_DMA_0x10_SRC_DATA_WIDTH) |
                    (dst_data_type << SFT_GENER_DMA_0x10_DEST_DATA_WIDTH)|
                    (src_addr_mode << SFT_GENER_DMA_0x10_SRC_ADDR_INC)   |
                    (dst_addr_mode << SFT_GENER_DMA_0x10_DEST_ADDR_INC)  |
                    (int_enable << SFT_GENER_DMA_0x10_FINISH_INTEN)      |
                    (DMA_USED_FLAG_MSK);
        dma->cfg1 = req;
    }
}

void dma_channel_single_start(void*           _dma,
                              uint32_t        src_addr,
                              uint32_t        dst_addr,
                              uint32_t        data_length
                             )
{
    if(_dma)
    {
        uint32_t   idx = ((uint32_t)_dma - (uint32_t)&dmac->config[0]) / sizeof(DMAConfig);
        DMAConfig* dma = (DMAConfig*)_dma;

        dma->src_start_addr = src_addr;
        dma->dst_start_addr = dst_addr;

        dmac->src_address[idx] = src_addr;
        dmac->dst_address[idx] = dst_addr;

        dma->cfg0 = MSK_GENER_DMA_0x10_DMA_EN | (dma->cfg0 & ~MSK_GENER_DMA_0x10_TRANS_LEN) | ((data_length - 1) << SFT_GENER_DMA_0x10_TRANS_LEN);
    }
}

void dma_channel_ctrl(void* _dma, uint32_t cmd, uint32_t arg)
{
    if(_dma)
    {
        uint32_t   idx = ((uint32_t)_dma - (uint32_t)&dmac->config[0]) / sizeof(DMAConfig);
        DMAConfig* dma = (DMAConfig*)_dma;

        switch(cmd)
        {
        case DMA_CTRL_CMD_PRIOPRITY_SET:
            dma->cfg0 = (dma->cfg0 & ~(MSK_GENER_DMA_0x10_CHN_PRIOPRITY)) | ((arg & MAX_GENER_DMA_0x10_CHN_PRIOPRITY) << SFT_GENER_DMA_0x10_CHN_PRIOPRITY);
            break;
        case DMA_CTRL_CMD_FINISH_INT_EN:
            dma->cfg0 = (dma->cfg0 & ~(MSK_GENER_DMA_0x10_FINISH_INTEN)) | ((!!arg) << SFT_GENER_DMA_0x10_FINISH_INTEN);
            break;
        case DMA_CTRL_CMD_HALF_FINISH_INT_EN:
            dma->cfg0 = (dma->cfg0 & ~(MSK_GENER_DMA_0x10_HALF_FINISH_INTEN)) | ((!!arg) << SFT_GENER_DMA_0x10_HALF_FINISH_INTEN);
            break;
        case DMA_CTRL_CMD_FINISH_INT_FLAG_GET:
            *(uint32_t*)arg = (dmac->finish_int_flag >> idx) & 0x1;
            break;
        case DMA_CTRL_CMD_HALF_FINISH_INT_FLAG_GET:
            *(uint32_t*)arg = (dmac->finish_int_flag >> (16 + idx)) & 0x1;
            break;
        case DMA_CTRL_CMD_FINISH_INT_FLAG_CLEAR:
            dmac->finish_int_flag |= (1 << idx);
            break;
        case DMA_CTRL_CMD_HALF_FINISH_INT_FLAG_CLEAR:
            dmac->finish_int_flag |= (1 << (16 + idx));
            break;
        case DMA_CTRL_CMD_SRC_RD_INTERVAL_SET:
            dma->cfg1 = (dma->cfg1 & ~MSK_GENER_DMA_0x17_SRC_RD_INTLV) | ((arg & MAX_GENER_DMA_0x17_SRC_RD_INTLV) << SFT_GENER_DMA_0x17_SRC_RD_INTLV);
            break;
        case DMA_CTRL_CMD_DST_WR_INTERVAL_SET:
            dma->cfg1 = (dma->cfg1 & ~MSK_GENER_DMA_0x17_DEST_WR_INTLV) | ((arg & MAX_GENER_DMA_0x17_DEST_WR_INTLV) << SFT_GENER_DMA_0x17_DEST_WR_INTLV);
            break;
        case DMA_CTRL_CMD_REMAIN_LENGTH_GET:
            *(uint32_t*)arg = dmac->status[idx] & MSK_GENER_DMA_0x1C_REMAIN_LEN;
            break;
        case DMA_CTRL_CMD_SRC_BUFF_FLUSH:
            dmac->status[idx] |= MSK_GENER_DMA_0x1C_FLUSH_SRC_BUFF;
            break;
        case DMA_CTRL_CMD_FINISH_INT_EN_GET:
            *(uint32_t*)arg = (dma->cfg0 & MSK_GENER_DMA_0x10_FINISH_INTEN) >> SFT_GENER_DMA_0x10_FINISH_INTEN;
            break;
        case DMA_CTRL_CMD_HALF_FINISH_INT_EN_GET:
            *(uint32_t*)arg = (dma->cfg0 & MSK_GENER_DMA_0x10_HALF_FINISH_INTEN) >> SFT_GENER_DMA_0x10_HALF_FINISH_INTEN;
            break;
        default:
            break;
        }
    }
}

void dma_channel_enable(void* dma, uint32_t enable)
{
    if(enable)
    {
        ((DMAConfig*)dma)->cfg0 |= MSK_GENER_DMA_0x10_DMA_EN;
    }
    else
    {
        ((DMAConfig*)dma)->cfg0 &= ~MSK_GENER_DMA_0x10_DMA_EN;
    }
}

uint32_t dma_channel_is_enabled(void* dma)
{
    return ((DMAConfig*)dma)->cfg0 & MSK_GENER_DMA_0x10_DMA_EN;
}

void dma_channel_src_curr_address_set(void* dma, uint32_t addr)
{
    dmac->src_address[((uint32_t)dma - (uint32_t)&dmac->config[0]) / sizeof(DMAConfig)] = addr;
}

void dma_channel_dst_curr_address_set(void* dma, uint32_t addr)
{
    dmac->dst_address[((uint32_t)dma - (uint32_t)&dmac->config[0]) / sizeof(DMAConfig)] = addr;
}

uint32_t dma_channel_src_curr_pointer_get(void* dma)
{
    return dmac->src_pointer[((uint32_t)dma - (uint32_t)&dmac->config[0]) / sizeof(DMAConfig)];
}

uint32_t dma_channel_dst_curr_pointer_get(void* dma)
{
    return dmac->dst_pointer[((uint32_t)dma - (uint32_t)&dmac->config[0]) / sizeof(DMAConfig)];
}
