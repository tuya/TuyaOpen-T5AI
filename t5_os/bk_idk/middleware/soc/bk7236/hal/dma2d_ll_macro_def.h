// Copyright 2020-2022 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/***********************************************************************************************************************************
* This file is generated from BK7236_ADDR_Mapping_MP2.xlsm automatically
* Modify it manually is not recommended
* CHIP ID:BK7236,GENARATE TIME:2022-12-6 17:17:11
************************************************************************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <soc/soc.h>

#define DMA2D_LL_REG_BASE      (SOC_DMA2D_REG_BASE) //REG_BASE:0x48080000

/* REG_0x00 */
#define DMA2D_DEVICE_ID_ADDR  (DMA2D_LL_REG_BASE  + 0x0*4) //REG ADDR :0x48080000
#define DMA2D_DEVICE_ID_DEVICE_ID_POS (0) 
#define DMA2D_DEVICE_ID_DEVICE_ID_MASK (0xFFFFFFFF) 

static inline uint32_t dma2d_ll_get_device_id_value(void)
{
    return REG_READ(DMA2D_DEVICE_ID_ADDR);
}

/* REG_0x00:device_id->device_id:0x0[31:0],Device ID:ASCII Code "DMAD",0x00415544,R*/
static inline uint32_t dma2d_ll_get_device_id_device_id(void)
{
    return REG_READ(DMA2D_DEVICE_ID_ADDR);
}

/* REG_0x01 */
#define DMA2D_VERSION_ID_ADDR  (DMA2D_LL_REG_BASE  + 0x1*4) //REG ADDR :0x48080004
#define DMA2D_VERSION_ID_VERSION_ID_POS (0) 
#define DMA2D_VERSION_ID_VERSION_ID_MASK (0xFFFFFFFF) 

static inline uint32_t dma2d_ll_get_version_id_value(void)
{
    return REG_READ(DMA2D_VERSION_ID_ADDR);
}

/* REG_0x01:version_id->version_id:0x1[31:0],Version ID: 版本号 V1.1,0x00010001,R*/
static inline uint32_t dma2d_ll_get_version_id_version_id(void)
{
    return REG_READ(DMA2D_VERSION_ID_ADDR);
}

/* REG_0x02 */
#define DMA2D_MODULE_CONTROL_ADDR  (DMA2D_LL_REG_BASE  + 0x2*4)
#define DMA2D_MODULE_CONTROL_SOFT_RESET_POS (0) 
#define DMA2D_MODULE_CONTROL_SOFT_RESET_MASK (0x1) 

#define DMA2D_MODULE_CONTROL_CLK_GATE_POS (1)
#define DMA2D_MODULE_CONTROL_CLK_GATE_MASK (0x1)

#define DMA2D_MODULE_CONTROL_RESERVED0_POS (2) 
#define DMA2D_MODULE_CONTROL_RESERVED0_MASK (0x3FFFFFFF) 

static inline uint32_t dma2d_ll_get_module_control_value(void)
{
    return REG_READ(DMA2D_MODULE_CONTROL_ADDR);
}

static inline void dma2d_ll_set_module_control_value(uint32_t value)
{
    REG_WRITE(DMA2D_MODULE_CONTROL_ADDR,value);
}

/* REG_0x02:module_control->soft_reset:0x2[0],软件写1复位DISP模块，需要软件清除,0x0,R/W*/
static inline uint32_t dma2d_ll_get_module_control_soft_reset(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_MODULE_CONTROL_ADDR);
    reg_value = ((reg_value >> DMA2D_MODULE_CONTROL_SOFT_RESET_POS) & DMA2D_MODULE_CONTROL_SOFT_RESET_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_module_control_soft_reset(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_MODULE_CONTROL_ADDR);
    reg_value &= ~(DMA2D_MODULE_CONTROL_SOFT_RESET_MASK << DMA2D_MODULE_CONTROL_SOFT_RESET_POS);
    reg_value |= ((value & DMA2D_MODULE_CONTROL_SOFT_RESET_MASK) << DMA2D_MODULE_CONTROL_SOFT_RESET_POS);
    REG_WRITE(DMA2D_MODULE_CONTROL_ADDR,reg_value);
}

static inline uint32_t dma2d_ll_get_module_control_clk_gate(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_MODULE_CONTROL_ADDR);
    reg_value = ((reg_value >> DMA2D_MODULE_CONTROL_CLK_GATE_POS) & DMA2D_MODULE_CONTROL_CLK_GATE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_module_control_clk_gate(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_MODULE_CONTROL_ADDR);
    reg_value &= ~(DMA2D_MODULE_CONTROL_CLK_GATE_MASK << DMA2D_MODULE_CONTROL_CLK_GATE_POS);
    reg_value |= ((value & DMA2D_MODULE_CONTROL_CLK_GATE_MASK) << DMA2D_MODULE_CONTROL_CLK_GATE_POS);
    REG_WRITE(DMA2D_MODULE_CONTROL_ADDR,reg_value);
}

/* REG_0x03 */
#define DMA2D_GLOBAL_STATUS_ADDR  (DMA2D_LL_REG_BASE  + 0x3*4)
#define DMA2D_GLOBAL_STATUS_GLOBAL_STATUS_POS (0) 
#define DMA2D_GLOBAL_STATUS_GLOBAL_STATUS_MASK (0xFFFFFFFF) 

static inline uint32_t dma2d_ll_get_global_status_value(void)
{
    return REG_READ(DMA2D_GLOBAL_STATUS_ADDR);
}

/* REG_0x03:global_status->global_Status:0x3[31:0], ,0x0,R*/
static inline uint32_t dma2d_ll_get_global_status_global_status(void)
{
    return REG_READ(DMA2D_GLOBAL_STATUS_ADDR);
}

/* REG_0x04 */
#define DMA2D_DMA2D_CONTROL_REG_ADDR  (DMA2D_LL_REG_BASE  + 0x4*4)
#define DMA2D_DMA2D_CONTROL_REG_TRAN_START_POS (0)
#define DMA2D_DMA2D_CONTROL_REG_TRAN_START_MASK (0x1)

#define DMA2D_DMA2D_CONTROL_REG_TRAN_SUSPEND_POS (1)
#define DMA2D_DMA2D_CONTROL_REG_TRAN_SUSPEND_MASK (0x1)

#define DMA2D_DMA2D_CONTROL_REG_TRAN_ABORT_POS (2)
#define DMA2D_DMA2D_CONTROL_REG_TRAN_ABORT_MASK (0x1)

#define DMA2D_DMA2D_CONTROL_REG_RESERVED3_POS (3)
#define DMA2D_DMA2D_CONTROL_REG_RESERVED3_MASK (0x7)

#define DMA2D_DMA2D_CONTROL_REG_LINE_OFFSET_MODE_POS (6)
#define DMA2D_DMA2D_CONTROL_REG_LINE_OFFSET_MODE_MASK (0x1)

#define DMA2D_DMA2D_CONTROL_REG_RESERVED2_POS (7)
#define DMA2D_DMA2D_CONTROL_REG_RESERVED2_MASK (0x1)

#define DMA2D_DMA2D_CONTROL_REG_ERROR_INT_EN_POS (8)
#define DMA2D_DMA2D_CONTROL_REG_ERROR_INT_EN_MASK (0x1)

#define DMA2D_DMA2D_CONTROL_REG_COMPLETE_INT_EN_POS (9)
#define DMA2D_DMA2D_CONTROL_REG_COMPLETE_INT_EN_MASK (0x1)

#define DMA2D_DMA2D_CONTROL_REG_WATERM_INT_EN_POS (10)
#define DMA2D_DMA2D_CONTROL_REG_WATERM_INT_EN_MASK (0x1)

#define DMA2D_DMA2D_CONTROL_REG_CLUT_ERROR_INT_EN_POS (11)
#define DMA2D_DMA2D_CONTROL_REG_CLUT_ERROR_INT_EN_MASK (0x1)

#define DMA2D_DMA2D_CONTROL_REG_CLUT_CMPLT_INT_EN_POS (12)
#define DMA2D_DMA2D_CONTROL_REG_CLUT_CMPLT_INT_EN_MASK (0x1)

#define DMA2D_DMA2D_CONTROL_REG_CONFIG_ERROR_INT_EN_POS (13)
#define DMA2D_DMA2D_CONTROL_REG_CONFIG_ERROR_INT_EN_MASK (0x1)

#define DMA2D_DMA2D_CONTROL_REG_RESERVED1_POS (14)
#define DMA2D_DMA2D_CONTROL_REG_RESERVED1_MASK (0x3)

#define DMA2D_DMA2D_CONTROL_REG_MODE_POS (16)
#define DMA2D_DMA2D_CONTROL_REG_MODE_MASK (0x7)

#define DMA2D_DMA2D_CONTROL_REG_RESERVED0_POS (19) 
#define DMA2D_DMA2D_CONTROL_REG_RESERVED0_MASK (0x7) 

#define DMA2D_DMA2D_CONTROL_REG_OUT_BYTE_REVESE_POS (21) 
#define DMA2D_DMA2D_CONTROL_REG_OUT_BYTE_REVESE_MASK (0x1) 

#define DMA2D_DMA2D_CONTROL_REG_DMA2D_AHB_BURST_LEN_POS (22) 
#define DMA2D_DMA2D_CONTROL_REG_DMA2D_AHB_BURST_LEN_MASK (0x7) 

#define DMA2D_DMA2D_CONTROL_REG_RESERVED5_POS (25) 
#define DMA2D_DMA2D_CONTROL_REG_RESERVED5_MASK (0x7F) 

static inline uint32_t dma2d_ll_get_dma2d_control_reg_value(void)
{
    return REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
}

static inline void dma2d_ll_set_dma2d_control_reg_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_CONTROL_REG_ADDR,value);
}

/* REG_0x04:dma2d_control_reg->tran_start:0x4[0],dma2d transfer start.,0x0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_control_reg_tran_start(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_CONTROL_REG_TRAN_START_POS) & DMA2D_DMA2D_CONTROL_REG_TRAN_START_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_control_reg_tran_start(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_CONTROL_REG_TRAN_START_MASK << DMA2D_DMA2D_CONTROL_REG_TRAN_START_POS);
    reg_value |= ((value & DMA2D_DMA2D_CONTROL_REG_TRAN_START_MASK) << DMA2D_DMA2D_CONTROL_REG_TRAN_START_POS);
    REG_WRITE(DMA2D_DMA2D_CONTROL_REG_ADDR,reg_value);
}

/* REG_0x04:dma2d_control_reg->tran_suspend:0x4[1],dma2d transfer suspend.,0x0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_control_reg_tran_suspend(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_CONTROL_REG_TRAN_SUSPEND_POS) & DMA2D_DMA2D_CONTROL_REG_TRAN_SUSPEND_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_control_reg_tran_suspend(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_CONTROL_REG_TRAN_SUSPEND_MASK << DMA2D_DMA2D_CONTROL_REG_TRAN_SUSPEND_POS);
    reg_value |= ((value & DMA2D_DMA2D_CONTROL_REG_TRAN_SUSPEND_MASK) << DMA2D_DMA2D_CONTROL_REG_TRAN_SUSPEND_POS);
    REG_WRITE(DMA2D_DMA2D_CONTROL_REG_ADDR,reg_value);
}

/* REG_0x04:dma2d_control_reg->tran_abort:0x4[2],dma2d transfer abort.,0x0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_control_reg_tran_abort(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_CONTROL_REG_TRAN_ABORT_POS) & DMA2D_DMA2D_CONTROL_REG_TRAN_ABORT_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_control_reg_tran_abort(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_CONTROL_REG_TRAN_ABORT_MASK << DMA2D_DMA2D_CONTROL_REG_TRAN_ABORT_POS);
    reg_value |= ((value & DMA2D_DMA2D_CONTROL_REG_TRAN_ABORT_MASK) << DMA2D_DMA2D_CONTROL_REG_TRAN_ABORT_POS);
    REG_WRITE(DMA2D_DMA2D_CONTROL_REG_ADDR,reg_value);
}

/* REG_0x04:dma2d_control_reg->line_offset_mode:0x4[6],line's offset mode sel: 0:in pixel express; 1: in bytes express.,0x0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_control_reg_line_offset_mode(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_CONTROL_REG_LINE_OFFSET_MODE_POS) & DMA2D_DMA2D_CONTROL_REG_LINE_OFFSET_MODE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_control_reg_line_offset_mode(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_CONTROL_REG_LINE_OFFSET_MODE_MASK << DMA2D_DMA2D_CONTROL_REG_LINE_OFFSET_MODE_POS);
    reg_value |= ((value & DMA2D_DMA2D_CONTROL_REG_LINE_OFFSET_MODE_MASK) << DMA2D_DMA2D_CONTROL_REG_LINE_OFFSET_MODE_POS);
    REG_WRITE(DMA2D_DMA2D_CONTROL_REG_ADDR,reg_value);
}

/* REG_0x04:dma2d_control_reg->error_int_en:0x4[8],trabsfer error int ena.,0x0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_control_reg_error_int_en(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_CONTROL_REG_ERROR_INT_EN_POS) & DMA2D_DMA2D_CONTROL_REG_ERROR_INT_EN_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_control_reg_error_int_en(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_CONTROL_REG_ERROR_INT_EN_MASK << DMA2D_DMA2D_CONTROL_REG_ERROR_INT_EN_POS);
    reg_value |= ((value & DMA2D_DMA2D_CONTROL_REG_ERROR_INT_EN_MASK) << DMA2D_DMA2D_CONTROL_REG_ERROR_INT_EN_POS);
    REG_WRITE(DMA2D_DMA2D_CONTROL_REG_ADDR,reg_value);
}

/* REG_0x04:dma2d_control_reg->complete_int_en:0x4[9],transfer complete int ena.,0x0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_control_reg_complete_int_en(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_CONTROL_REG_COMPLETE_INT_EN_POS) & DMA2D_DMA2D_CONTROL_REG_COMPLETE_INT_EN_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_control_reg_complete_int_en(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_CONTROL_REG_COMPLETE_INT_EN_MASK << DMA2D_DMA2D_CONTROL_REG_COMPLETE_INT_EN_POS);
    reg_value |= ((value & DMA2D_DMA2D_CONTROL_REG_COMPLETE_INT_EN_MASK) << DMA2D_DMA2D_CONTROL_REG_COMPLETE_INT_EN_POS);
    REG_WRITE(DMA2D_DMA2D_CONTROL_REG_ADDR,reg_value);
}

/* REG_0x04:dma2d_control_reg->waterm_int_en:0x4[10],transfer watermark int ena.,0x0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_control_reg_waterm_int_en(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_CONTROL_REG_WATERM_INT_EN_POS) & DMA2D_DMA2D_CONTROL_REG_WATERM_INT_EN_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_control_reg_waterm_int_en(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_CONTROL_REG_WATERM_INT_EN_MASK << DMA2D_DMA2D_CONTROL_REG_WATERM_INT_EN_POS);
    reg_value |= ((value & DMA2D_DMA2D_CONTROL_REG_WATERM_INT_EN_MASK) << DMA2D_DMA2D_CONTROL_REG_WATERM_INT_EN_POS);
    REG_WRITE(DMA2D_DMA2D_CONTROL_REG_ADDR,reg_value);
}

/* REG_0x04:dma2d_control_reg->clut_error_int_en:0x4[11],clut transfer error int ena.,0x0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_control_reg_clut_error_int_en(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_CONTROL_REG_CLUT_ERROR_INT_EN_POS) & DMA2D_DMA2D_CONTROL_REG_CLUT_ERROR_INT_EN_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_control_reg_clut_error_int_en(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_CONTROL_REG_CLUT_ERROR_INT_EN_MASK << DMA2D_DMA2D_CONTROL_REG_CLUT_ERROR_INT_EN_POS);
    reg_value |= ((value & DMA2D_DMA2D_CONTROL_REG_CLUT_ERROR_INT_EN_MASK) << DMA2D_DMA2D_CONTROL_REG_CLUT_ERROR_INT_EN_POS);
    REG_WRITE(DMA2D_DMA2D_CONTROL_REG_ADDR,reg_value);
}

/* REG_0x04:dma2d_control_reg->clut_cmplt_int_en:0x4[12],clut transfer complete int ena.,0x0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_control_reg_clut_cmplt_int_en(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_CONTROL_REG_CLUT_CMPLT_INT_EN_POS) & DMA2D_DMA2D_CONTROL_REG_CLUT_CMPLT_INT_EN_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_control_reg_clut_cmplt_int_en(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_CONTROL_REG_CLUT_CMPLT_INT_EN_MASK << DMA2D_DMA2D_CONTROL_REG_CLUT_CMPLT_INT_EN_POS);
    reg_value |= ((value & DMA2D_DMA2D_CONTROL_REG_CLUT_CMPLT_INT_EN_MASK) << DMA2D_DMA2D_CONTROL_REG_CLUT_CMPLT_INT_EN_POS);
    REG_WRITE(DMA2D_DMA2D_CONTROL_REG_ADDR,reg_value);
}

/* REG_0x04:dma2d_control_reg->config_error_int_en:0x4[13],config error int ena,,0x0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_control_reg_config_error_int_en(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_CONTROL_REG_CONFIG_ERROR_INT_EN_POS) & DMA2D_DMA2D_CONTROL_REG_CONFIG_ERROR_INT_EN_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_control_reg_config_error_int_en(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_CONTROL_REG_CONFIG_ERROR_INT_EN_MASK << DMA2D_DMA2D_CONTROL_REG_CONFIG_ERROR_INT_EN_POS);
    reg_value |= ((value & DMA2D_DMA2D_CONTROL_REG_CONFIG_ERROR_INT_EN_MASK) << DMA2D_DMA2D_CONTROL_REG_CONFIG_ERROR_INT_EN_POS);
    REG_WRITE(DMA2D_DMA2D_CONTROL_REG_ADDR,reg_value);
}

/* REG_0x04:dma2d_control_reg->mode:0x4[18:16],DMA2D mode sel: 000:m2m; 001:m2m pixel convert with fg; 010:m2m blend; 011:r2m.only with output; 100: m2m blend fix fg; 101:m2m blend fix bg;,0x0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_control_reg_mode(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_CONTROL_REG_MODE_POS) & DMA2D_DMA2D_CONTROL_REG_MODE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_control_reg_mode(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_CONTROL_REG_MODE_MASK << DMA2D_DMA2D_CONTROL_REG_MODE_POS);
    reg_value |= ((value & DMA2D_DMA2D_CONTROL_REG_MODE_MASK) << DMA2D_DMA2D_CONTROL_REG_MODE_POS);
    REG_WRITE(DMA2D_DMA2D_CONTROL_REG_ADDR,reg_value);
}

/* REG_0x00:dma2d_control_reg->out_byte_revese:0x0[     21],in output rgb888 formart, reverse data byte by byte.,0,R/W*/
static inline uint32_t dma2d_ll_get_dma2d_control_reg_out_byte_revese(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_CONTROL_REG_OUT_BYTE_REVESE_POS) & DMA2D_DMA2D_CONTROL_REG_OUT_BYTE_REVESE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_control_reg_out_byte_revese(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_CONTROL_REG_OUT_BYTE_REVESE_MASK << DMA2D_DMA2D_CONTROL_REG_OUT_BYTE_REVESE_POS);
    reg_value |= ((value & DMA2D_DMA2D_CONTROL_REG_OUT_BYTE_REVESE_MASK) << DMA2D_DMA2D_CONTROL_REG_OUT_BYTE_REVESE_POS);
    REG_WRITE(DMA2D_DMA2D_CONTROL_REG_ADDR,reg_value);
}


/* REG_0x04:dma2d_control_reg->dma2d_ahb_burst_len:0x4[24:22],0:64Word per burst;1:32;2:16;3:8;4:6¡£,0x0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_control_reg_dma2d_ahb_burst_len(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_CONTROL_REG_DMA2D_AHB_BURST_LEN_POS) & DMA2D_DMA2D_CONTROL_REG_DMA2D_AHB_BURST_LEN_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_control_reg_dma2d_ahb_burst_len(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_CONTROL_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_CONTROL_REG_DMA2D_AHB_BURST_LEN_MASK << DMA2D_DMA2D_CONTROL_REG_DMA2D_AHB_BURST_LEN_POS);
    reg_value |= ((value & DMA2D_DMA2D_CONTROL_REG_DMA2D_AHB_BURST_LEN_MASK) << DMA2D_DMA2D_CONTROL_REG_DMA2D_AHB_BURST_LEN_POS);
    REG_WRITE(DMA2D_DMA2D_CONTROL_REG_ADDR,reg_value);
}

/* REG_0x05 */
#define DMA2D_DMA2D_INT_STATUS_ADDR  (DMA2D_LL_REG_BASE  + 0x5*4) //REG ADDR :0x48080014
#define DMA2D_DMA2D_INT_STATUS_ERROR_INT_POS (0) 
#define DMA2D_DMA2D_INT_STATUS_ERROR_INT_MASK (0x1) 

#define DMA2D_DMA2D_INT_STATUS_COMPLETE_INT_POS (1)
#define DMA2D_DMA2D_INT_STATUS_COMPLETE_INT_MASK (0x1)

#define DMA2D_DMA2D_INT_STATUS_WATERM_INT_POS (2)
#define DMA2D_DMA2D_INT_STATUS_WATERM_INT_MASK (0x1)

#define DMA2D_DMA2D_INT_STATUS_CLUT_ERROR_INT_POS (3)
#define DMA2D_DMA2D_INT_STATUS_CLUT_ERROR_INT_MASK (0x1)

#define DMA2D_DMA2D_INT_STATUS_CLUT_CMPLT_INT_POS (4)
#define DMA2D_DMA2D_INT_STATUS_CLUT_CMPLT_INT_MASK (0x1)

#define DMA2D_DMA2D_INT_STATUS_CONFIG_ERROR_INT_POS (5)
#define DMA2D_DMA2D_INT_STATUS_CONFIG_ERROR_INT_MASK (0x1)

#define DMA2D_DMA2D_INT_STATUS_RESERVED_POS (6)
#define DMA2D_DMA2D_INT_STATUS_RESERVED_MASK (0x3FFFFFF)

static inline uint32_t dma2d_ll_get_dma2d_int_status_value(void)
{
    return REG_READ(DMA2D_DMA2D_INT_STATUS_ADDR);
}

/* REG_0x05:dma2d_int_status->error_int:0x5[0],transfer error int flag.,0x0,R*/
static inline uint32_t dma2d_ll_get_dma2d_int_status_error_int(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_INT_STATUS_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_INT_STATUS_ERROR_INT_POS)&DMA2D_DMA2D_INT_STATUS_ERROR_INT_MASK);
    return reg_value;
}

/* REG_0x05:dma2d_int_status->complete_int:0x5[1],transfer complete int flag.,0x0,R*/
static inline uint32_t dma2d_ll_get_dma2d_int_status_complete_int(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_INT_STATUS_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_INT_STATUS_COMPLETE_INT_POS)&DMA2D_DMA2D_INT_STATUS_COMPLETE_INT_MASK);
    return reg_value;
}

/* REG_0x05:dma2d_int_status->waterm_int:0x5[2],transfer watermark intt flag.,0x0,R*/
static inline uint32_t dma2d_ll_get_dma2d_int_status_waterm_int(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_INT_STATUS_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_INT_STATUS_WATERM_INT_POS)&DMA2D_DMA2D_INT_STATUS_WATERM_INT_MASK);
    return reg_value;
}

/* REG_0x05:dma2d_int_status->clut_error_int:0x5[3],clut transfer error intt flag.,0x0,R*/
static inline uint32_t dma2d_ll_get_dma2d_int_status_clut_error_int(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_INT_STATUS_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_INT_STATUS_CLUT_ERROR_INT_POS)&DMA2D_DMA2D_INT_STATUS_CLUT_ERROR_INT_MASK);
    return reg_value;
}

/* REG_0x05:dma2d_int_status->clut_cmplt_int:0x5[4],clut transfer complete intt flag.,0x0,R*/
static inline uint32_t dma2d_ll_get_dma2d_int_status_clut_cmplt_int(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_INT_STATUS_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_INT_STATUS_CLUT_CMPLT_INT_POS)&DMA2D_DMA2D_INT_STATUS_CLUT_CMPLT_INT_MASK);
    return reg_value;
}

/* REG_0x05:dma2d_int_status->config_error_int:0x5[5],config error int t flag.,0x0,R*/
static inline uint32_t dma2d_ll_get_dma2d_int_status_config_error_int(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_INT_STATUS_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_INT_STATUS_CONFIG_ERROR_INT_POS)&DMA2D_DMA2D_INT_STATUS_CONFIG_ERROR_INT_MASK);
    return reg_value;
}

/* REG_0x06 */
#define DMA2D_DMA2D_INT_CLEAR_ADDR  (DMA2D_LL_REG_BASE  + 0x6*4) //REG ADDR :0x48080018
#define DMA2D_DMA2D_INT_CLEAR_CLR_ERROR_INT_POS (0) 
#define DMA2D_DMA2D_INT_CLEAR_CLR_ERROR_INT_MASK (0x1) 

#define DMA2D_DMA2D_INT_CLEAR_CLR_COMPLETE_INT_POS (1)
#define DMA2D_DMA2D_INT_CLEAR_CLR_COMPLETE_INT_MASK (0x1)

#define DMA2D_DMA2D_INT_CLEAR_CLR_WATERM_INT_POS (2)
#define DMA2D_DMA2D_INT_CLEAR_CLR_WATERM_INT_MASK (0x1)

#define DMA2D_DMA2D_INT_CLEAR_CLR_CLUT_ERROR_INT_POS (3)
#define DMA2D_DMA2D_INT_CLEAR_CLR_CLUT_ERROR_INT_MASK (0x1)

#define DMA2D_DMA2D_INT_CLEAR_CLR_CLUT_CMPLT_INT_POS (4)
#define DMA2D_DMA2D_INT_CLEAR_CLR_CLUT_CMPLT_INT_MASK (0x1)

#define DMA2D_DMA2D_INT_CLEAR_CLR_CONFIG_ERROR_INT_POS (5)
#define DMA2D_DMA2D_INT_CLEAR_CLR_CONFIG_ERROR_INT_MASK (0x1)

#define DMA2D_DMA2D_INT_CLEAR_RESERVED_POS (6)
#define DMA2D_DMA2D_INT_CLEAR_RESERVED_MASK (0x3FFFFFF)

/*write only reg:dma2d_int_clear:default value:0x0*/
static inline void dma2d_ll_set_dma2d_int_clear_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_INT_CLEAR_ADDR,value);
}

/* REG_0x07 */
#define DMA2D_DMA2D_FG_ADDRESS_ADDR  (DMA2D_LL_REG_BASE  + 0x7*4) //REG ADDR :0x4808001c
#define DMA2D_DMA2D_FG_ADDRESS_FG_ADDRESS_POS (0) 
#define DMA2D_DMA2D_FG_ADDRESS_FG_ADDRESS_MASK (0xFFFFFFFF) 

static inline uint32_t dma2d_ll_get_dma2d_fg_address_value(void)
{
    return REG_READ(DMA2D_DMA2D_FG_ADDRESS_ADDR);
}

static inline void dma2d_ll_set_dma2d_fg_address_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_FG_ADDRESS_ADDR,value);
}

/* REG_0x07:dma2d_fg_address->fg_address:0x7[31:0],foreground mem address, written when transfer disable.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_address_fg_address(void)
{
    return REG_READ(DMA2D_DMA2D_FG_ADDRESS_ADDR);
}

static inline void dma2d_ll_set_dma2d_fg_address_fg_address(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_FG_ADDRESS_ADDR,value);
}

/* REG_0x08 */
#define DMA2D_DMA2D_FG_OFFSET_ADDR  (DMA2D_LL_REG_BASE  + 0x8*4)
#define DMA2D_DMA2D_FG_OFFSET_FG_LINE_OFFSET_POS (0)
#define DMA2D_DMA2D_FG_OFFSET_FG_LINE_OFFSET_MASK (0xFFFF)

#define DMA2D_DMA2D_FG_OFFSET_RESERVED_POS (16)
#define DMA2D_DMA2D_FG_OFFSET_RESERVED_MASK (0xFFFF)

static inline uint32_t dma2d_ll_get_dma2d_fg_offset_value(void)
{
    return REG_READ(DMA2D_DMA2D_FG_OFFSET_ADDR);
}

static inline void dma2d_ll_set_dma2d_fg_offset_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_FG_OFFSET_ADDR,value);
}

/* REG_0x08:dma2d_fg_offset->fg_line_offset:0x8[15:0],line offset for fg, offset In pixel or bytes.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_offset_fg_line_offset(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_OFFSET_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_OFFSET_FG_LINE_OFFSET_POS) & DMA2D_DMA2D_FG_OFFSET_FG_LINE_OFFSET_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_offset_fg_line_offset(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_OFFSET_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_OFFSET_FG_LINE_OFFSET_MASK << DMA2D_DMA2D_FG_OFFSET_FG_LINE_OFFSET_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_OFFSET_FG_LINE_OFFSET_MASK) << DMA2D_DMA2D_FG_OFFSET_FG_LINE_OFFSET_POS);
    REG_WRITE(DMA2D_DMA2D_FG_OFFSET_ADDR,reg_value);
}

/* REG_0x09 */
#define DMA2D_DMA2D_BG_ADDRESS_ADDR  (DMA2D_LL_REG_BASE  + 0x9*4) //REG ADDR :0x48080024
#define DMA2D_DMA2D_BG_ADDRESS_BG_ADDRESS_POS (0) 
#define DMA2D_DMA2D_BG_ADDRESS_BG_ADDRESS_MASK (0xFFFFFFFF) 

static inline uint32_t dma2d_ll_get_dma2d_bg_address_value(void)
{
    return REG_READ(DMA2D_DMA2D_BG_ADDRESS_ADDR);
}

static inline void dma2d_ll_set_dma2d_bg_address_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_BG_ADDRESS_ADDR,value);
}

/* REG_0x09:dma2d_bg_address->bg_address:0x9[31:0],background mem address, written when transfer disable.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_address_bg_address(void)
{
    return REG_READ(DMA2D_DMA2D_BG_ADDRESS_ADDR);
}

static inline void dma2d_ll_set_dma2d_bg_address_bg_address(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_BG_ADDRESS_ADDR,value);
}

/* REG_0x0A */
#define DMA2D_DMA2D_BG_OFFSET_ADDR  (DMA2D_LL_REG_BASE  + 0xA*4) //REG ADDR :0x48080028
#define DMA2D_DMA2D_BG_OFFSET_BG_LINE_OFFSET_POS (0) 
#define DMA2D_DMA2D_BG_OFFSET_BG_LINE_OFFSET_MASK (0xFFFF) 

#define DMA2D_DMA2D_BG_OFFSET_RESERVED_POS (16)
#define DMA2D_DMA2D_BG_OFFSET_RESERVED_MASK (0xFFFF)

static inline uint32_t dma2d_ll_get_dma2d_bg_offset_value(void)
{
    return REG_READ(DMA2D_DMA2D_BG_OFFSET_ADDR);
}

static inline void dma2d_ll_set_dma2d_bg_offset_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_BG_OFFSET_ADDR,value);
}

/* REG_0x0a:dma2d_bg_offset->bg_line_offset:0xa[15:0],line offset for bg, offset In pixel or bytes.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_offset_bg_line_offset(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_OFFSET_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_OFFSET_BG_LINE_OFFSET_POS) & DMA2D_DMA2D_BG_OFFSET_BG_LINE_OFFSET_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_offset_bg_line_offset(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_OFFSET_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_OFFSET_BG_LINE_OFFSET_MASK << DMA2D_DMA2D_BG_OFFSET_BG_LINE_OFFSET_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_OFFSET_BG_LINE_OFFSET_MASK) << DMA2D_DMA2D_BG_OFFSET_BG_LINE_OFFSET_POS);
    REG_WRITE(DMA2D_DMA2D_BG_OFFSET_ADDR,reg_value);
}

/* REG_0x0B */
#define DMA2D_DMA2D_FG_PFC_CTRL_ADDR  (DMA2D_LL_REG_BASE  + 0xB*4) //REG ADDR :0x4808002c
#define DMA2D_DMA2D_FG_PFC_CTRL_FG_COLOR_MODE_POS (0) 
#define DMA2D_DMA2D_FG_PFC_CTRL_FG_COLOR_MODE_MASK (0xF) 

#define DMA2D_DMA2D_FG_PFC_CTRL_FG_CLUT_COLOR_MODE_POS (4)
#define DMA2D_DMA2D_FG_PFC_CTRL_FG_CLUT_COLOR_MODE_MASK (0x1)

#define DMA2D_DMA2D_FG_PFC_CTRL_FG_START_CLUT_POS (5)
#define DMA2D_DMA2D_FG_PFC_CTRL_FG_START_CLUT_MASK (0x1)

#define DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_YUV_FMT_POS (6) 
#define DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_YUV_FMT_MASK (0x3) 

#define DMA2D_DMA2D_FG_PFC_CTRL_FG_CLUT_SIZE_POS (8)
#define DMA2D_DMA2D_FG_PFC_CTRL_FG_CLUT_SIZE_MASK (0xFF)

#define DMA2D_DMA2D_FG_PFC_CTRL_FG_ALPHA_MODE_POS (16)
#define DMA2D_DMA2D_FG_PFC_CTRL_FG_ALPHA_MODE_MASK (0x3)

#define DMA2D_DMA2D_FG_PFC_CTRL_RESERVED_POS (18) 
#define DMA2D_DMA2D_FG_PFC_CTRL_RESERVED_MASK (0x3) 

#define DMA2D_DMA2D_FG_PFC_CTRL_ALPHA_INVERT_POS (20)
#define DMA2D_DMA2D_FG_PFC_CTRL_ALPHA_INVERT_MASK (0x1)

#define DMA2D_DMA2D_FG_PFC_CTRL_FG_RB_SWAP_POS (21)
#define DMA2D_DMA2D_FG_PFC_CTRL_FG_RB_SWAP_MASK (0x1)

#define DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_BYTE_REVE_POS (22) 
#define DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_BYTE_REVE_MASK (0x1) 

#define DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_HWORD_REVE_POS (23) 
#define DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_HWORD_REVE_MASK (0x1) 

#define DMA2D_DMA2D_FG_PFC_CTRL_FG_ALPHA_POS (24) 
#define DMA2D_DMA2D_FG_PFC_CTRL_FG_ALPHA_MASK (0xFF) 

static inline uint32_t dma2d_ll_get_dma2d_fg_pfc_ctrl_value(void)
{
    return REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
}

static inline void dma2d_ll_set_dma2d_fg_pfc_ctrl_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_FG_PFC_CTRL_ADDR,value);
}

/* REG_0x0b:dma2d_fg_pfc_ctrl->fg_color_mode:0xb[3:0],foreground image color mode.; 0000:ARGB888;0001:RGB888;0010:RGB565;0011:ARGB1555;0100:ARGB4444; 0101:L8;0110:AL44; 0111:AL88; 1000:L4; 1001:A8;1010:A4;1011:YUYV.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_color_mode(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_PFC_CTRL_FG_COLOR_MODE_POS) & DMA2D_DMA2D_FG_PFC_CTRL_FG_COLOR_MODE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_pfc_ctrl_fg_color_mode(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_PFC_CTRL_FG_COLOR_MODE_MASK << DMA2D_DMA2D_FG_PFC_CTRL_FG_COLOR_MODE_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_PFC_CTRL_FG_COLOR_MODE_MASK) << DMA2D_DMA2D_FG_PFC_CTRL_FG_COLOR_MODE_POS);
    REG_WRITE(DMA2D_DMA2D_FG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0b:dma2d_fg_pfc_ctrl->fg_clut_color_mode:0xb[4],color mode for clut. 0:argb8888; 1:rgb888.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_clut_color_mode(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_PFC_CTRL_FG_CLUT_COLOR_MODE_POS) & DMA2D_DMA2D_FG_PFC_CTRL_FG_CLUT_COLOR_MODE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_pfc_ctrl_fg_clut_color_mode(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_PFC_CTRL_FG_CLUT_COLOR_MODE_MASK << DMA2D_DMA2D_FG_PFC_CTRL_FG_CLUT_COLOR_MODE_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_PFC_CTRL_FG_CLUT_COLOR_MODE_MASK) << DMA2D_DMA2D_FG_PFC_CTRL_FG_CLUT_COLOR_MODE_POS);
    REG_WRITE(DMA2D_DMA2D_FG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0b:dma2d_fg_pfc_ctrl->fg_start_clut:0xb[5],automatic load the clut. Automatic reset.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_start_clut(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_PFC_CTRL_FG_START_CLUT_POS) & DMA2D_DMA2D_FG_PFC_CTRL_FG_START_CLUT_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_pfc_ctrl_fg_start_clut(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_PFC_CTRL_FG_START_CLUT_MASK << DMA2D_DMA2D_FG_PFC_CTRL_FG_START_CLUT_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_PFC_CTRL_FG_START_CLUT_MASK) << DMA2D_DMA2D_FG_PFC_CTRL_FG_START_CLUT_POS);
    REG_WRITE(DMA2D_DMA2D_FG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0b:dma2d_fg_pfc_ctrl->fg_y2r_yuv_fmt:0xb[7:6],0:yuyv, 1:uyvy, 2:yyuv, 3:uvyy, 4:vuyy.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_y2r_yuv_fmt(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_YUV_FMT_POS) & DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_YUV_FMT_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_pfc_ctrl_fg_y2r_yuv_fmt(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_YUV_FMT_MASK << DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_YUV_FMT_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_YUV_FMT_MASK) << DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_YUV_FMT_POS);
    REG_WRITE(DMA2D_DMA2D_FG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0b:dma2d_fg_pfc_ctrl->fg_clut_size:0xb[15:8],the size of clut used for foreground image. Size = fg_clut_size + 1;,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_clut_size(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_PFC_CTRL_FG_CLUT_SIZE_POS) & DMA2D_DMA2D_FG_PFC_CTRL_FG_CLUT_SIZE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_pfc_ctrl_fg_clut_size(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_PFC_CTRL_FG_CLUT_SIZE_MASK << DMA2D_DMA2D_FG_PFC_CTRL_FG_CLUT_SIZE_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_PFC_CTRL_FG_CLUT_SIZE_MASK) << DMA2D_DMA2D_FG_PFC_CTRL_FG_CLUT_SIZE_POS);
    REG_WRITE(DMA2D_DMA2D_FG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0b:dma2d_fg_pfc_ctrl->fg_alpha_mode:0xb[17:16],alpha value use for fg image. 00: nochange; 01:replace orginal, 10: alpha[7:0] multipied with orginal value.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_alpha_mode(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_PFC_CTRL_FG_ALPHA_MODE_POS) & DMA2D_DMA2D_FG_PFC_CTRL_FG_ALPHA_MODE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_pfc_ctrl_fg_alpha_mode(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_PFC_CTRL_FG_ALPHA_MODE_MASK << DMA2D_DMA2D_FG_PFC_CTRL_FG_ALPHA_MODE_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_PFC_CTRL_FG_ALPHA_MODE_MASK) << DMA2D_DMA2D_FG_PFC_CTRL_FG_ALPHA_MODE_POS);
    REG_WRITE(DMA2D_DMA2D_FG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0b:dma2d_fg_pfc_ctrl->alpha_invert:0xb[20],invert alpha value.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_pfc_ctrl_alpha_invert(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_PFC_CTRL_ALPHA_INVERT_POS) & DMA2D_DMA2D_FG_PFC_CTRL_ALPHA_INVERT_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_pfc_ctrl_alpha_invert(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_PFC_CTRL_ALPHA_INVERT_MASK << DMA2D_DMA2D_FG_PFC_CTRL_ALPHA_INVERT_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_PFC_CTRL_ALPHA_INVERT_MASK) << DMA2D_DMA2D_FG_PFC_CTRL_ALPHA_INVERT_POS);
    REG_WRITE(DMA2D_DMA2D_FG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0b:dma2d_fg_pfc_ctrl->fg_rb_swap:0xb[21],red blue swap to support rgb or argb. 0: regular mode.1:swap_mode.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_rb_swap(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_PFC_CTRL_FG_RB_SWAP_POS) & DMA2D_DMA2D_FG_PFC_CTRL_FG_RB_SWAP_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_pfc_ctrl_fg_rb_swap(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_PFC_CTRL_FG_RB_SWAP_MASK << DMA2D_DMA2D_FG_PFC_CTRL_FG_RB_SWAP_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_PFC_CTRL_FG_RB_SWAP_MASK) << DMA2D_DMA2D_FG_PFC_CTRL_FG_RB_SWAP_POS);
    REG_WRITE(DMA2D_DMA2D_FG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0b:dma2d_fg_pfc_ctrl->fg_y2r_byte_reve:0xb[22],yuv2rgb input data revrese byte by byte.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_y2r_byte_reve(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_BYTE_REVE_POS) & DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_BYTE_REVE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_pfc_ctrl_fg_y2r_byte_reve(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_BYTE_REVE_MASK << DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_BYTE_REVE_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_BYTE_REVE_MASK) << DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_BYTE_REVE_POS);
    REG_WRITE(DMA2D_DMA2D_FG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0b:dma2d_fg_pfc_ctrl->fg_y2r_hword_reve:0xb[23],yuv2rgb input data reverse halfword by halfword.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_y2r_hword_reve(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_HWORD_REVE_POS) & DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_HWORD_REVE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_pfc_ctrl_fg_y2r_hword_reve(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_HWORD_REVE_MASK << DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_HWORD_REVE_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_HWORD_REVE_MASK) << DMA2D_DMA2D_FG_PFC_CTRL_FG_Y2R_HWORD_REVE_POS);
    REG_WRITE(DMA2D_DMA2D_FG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0b:dma2d_fg_pfc_ctrl->fg_alpha:0xb[31:24],fg alpha value set. Use with fg_alpha_mode.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_pfc_ctrl_fg_alpha(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_PFC_CTRL_FG_ALPHA_POS) & DMA2D_DMA2D_FG_PFC_CTRL_FG_ALPHA_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_pfc_ctrl_fg_alpha(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_PFC_CTRL_FG_ALPHA_MASK << DMA2D_DMA2D_FG_PFC_CTRL_FG_ALPHA_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_PFC_CTRL_FG_ALPHA_MASK) << DMA2D_DMA2D_FG_PFC_CTRL_FG_ALPHA_POS);
    REG_WRITE(DMA2D_DMA2D_FG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0C */
#define DMA2D_DMA2D_FG_COLOR_REG_ADDR  (DMA2D_LL_REG_BASE  + 0xC*4) //REG ADDR :0x48080030
#define DMA2D_DMA2D_FG_COLOR_REG_BLUE_VALUE_POS (0) 
#define DMA2D_DMA2D_FG_COLOR_REG_BLUE_VALUE_MASK (0xFF) 

#define DMA2D_DMA2D_FG_COLOR_REG_GREEN_VALUE_POS (8) 
#define DMA2D_DMA2D_FG_COLOR_REG_GREEN_VALUE_MASK (0xFF) 

#define DMA2D_DMA2D_FG_COLOR_REG_RED_VALUE_POS (16) 
#define DMA2D_DMA2D_FG_COLOR_REG_RED_VALUE_MASK (0xFF) 

#define DMA2D_DMA2D_FG_COLOR_REG_RESERVED_POS (24) 
#define DMA2D_DMA2D_FG_COLOR_REG_RESERVED_MASK (0xFF) 

static inline uint32_t dma2d_ll_get_dma2d_fg_color_reg_value(void)
{
    return REG_READ(DMA2D_DMA2D_FG_COLOR_REG_ADDR);
}

static inline void dma2d_ll_set_dma2d_fg_color_reg_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_FG_COLOR_REG_ADDR,value);
}

/* REG_0x0c:dma2d_fg_color_reg->blue_value:0xc[7:0],blue_value in a4 or a8 mode of fg,,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_color_reg_blue_value(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_COLOR_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_COLOR_REG_BLUE_VALUE_POS) & DMA2D_DMA2D_FG_COLOR_REG_BLUE_VALUE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_color_reg_blue_value(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_COLOR_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_COLOR_REG_BLUE_VALUE_MASK << DMA2D_DMA2D_FG_COLOR_REG_BLUE_VALUE_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_COLOR_REG_BLUE_VALUE_MASK) << DMA2D_DMA2D_FG_COLOR_REG_BLUE_VALUE_POS);
    REG_WRITE(DMA2D_DMA2D_FG_COLOR_REG_ADDR,reg_value);
}

/* REG_0x0c:dma2d_fg_color_reg->green_value:0xc[15:8],green_value in a4 or a8 mode of fg,,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_color_reg_green_value(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_COLOR_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_COLOR_REG_GREEN_VALUE_POS) & DMA2D_DMA2D_FG_COLOR_REG_GREEN_VALUE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_color_reg_green_value(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_COLOR_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_COLOR_REG_GREEN_VALUE_MASK << DMA2D_DMA2D_FG_COLOR_REG_GREEN_VALUE_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_COLOR_REG_GREEN_VALUE_MASK) << DMA2D_DMA2D_FG_COLOR_REG_GREEN_VALUE_POS);
    REG_WRITE(DMA2D_DMA2D_FG_COLOR_REG_ADDR,reg_value);
}

/* REG_0x0c:dma2d_fg_color_reg->red_value:0xc[23:16],red_value in a4 or a8 mode of fg,,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_color_reg_red_value(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_COLOR_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_COLOR_REG_RED_VALUE_POS) & DMA2D_DMA2D_FG_COLOR_REG_RED_VALUE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_color_reg_red_value(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_COLOR_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_COLOR_REG_RED_VALUE_MASK << DMA2D_DMA2D_FG_COLOR_REG_RED_VALUE_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_COLOR_REG_RED_VALUE_MASK) << DMA2D_DMA2D_FG_COLOR_REG_RED_VALUE_POS);
    REG_WRITE(DMA2D_DMA2D_FG_COLOR_REG_ADDR,reg_value);
}

/* REG_0x0D */
#define DMA2D_DMA2D_BG_PFC_CTRL_ADDR  (DMA2D_LL_REG_BASE  + 0xD*4) //REG ADDR :0x48080034
#define DMA2D_DMA2D_BG_PFC_CTRL_BG_COLOR_MODE_POS (0) 
#define DMA2D_DMA2D_BG_PFC_CTRL_BG_COLOR_MODE_MASK (0xF) 

#define DMA2D_DMA2D_BG_PFC_CTRL_BG_CLUT_COLOR_MODE_POS (4) 
#define DMA2D_DMA2D_BG_PFC_CTRL_BG_CLUT_COLOR_MODE_MASK (0x1) 

#define DMA2D_DMA2D_BG_PFC_CTRL_BG_START_CLUT_POS (5) 
#define DMA2D_DMA2D_BG_PFC_CTRL_BG_START_CLUT_MASK (0x1) 

#define DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_YUV_FMT_POS (6) 
#define DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_YUV_FMT_MASK (0x3) 

#define DMA2D_DMA2D_BG_PFC_CTRL_BG_CLUT_SIZE_POS (8) 
#define DMA2D_DMA2D_BG_PFC_CTRL_BG_CLUT_SIZE_MASK (0xFF) 

#define DMA2D_DMA2D_BG_PFC_CTRL_BG_ALPHA_MODE_POS (16) 
#define DMA2D_DMA2D_BG_PFC_CTRL_BG_ALPHA_MODE_MASK (0x3) 

#define DMA2D_DMA2D_BG_PFC_CTRL_RESERVED_POS (18) 
#define DMA2D_DMA2D_BG_PFC_CTRL_RESERVED_MASK (0x3) 

#define DMA2D_DMA2D_BG_PFC_CTRL_ALPHA_INVERT_POS (20) 
#define DMA2D_DMA2D_BG_PFC_CTRL_ALPHA_INVERT_MASK (0x1) 

#define DMA2D_DMA2D_BG_PFC_CTRL_BG_RB_SWAP_POS (21) 
#define DMA2D_DMA2D_BG_PFC_CTRL_BG_RB_SWAP_MASK (0x1) 

#define DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_BYTE_REVE_POS (22) 
#define DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_BYTE_REVE_MASK (0x1) 

#define DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_HWORD_REVE_POS (23) 
#define DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_HWORD_REVE_MASK (0x1) 

#define DMA2D_DMA2D_BG_PFC_CTRL_BG_ALPHA_POS (24) 
#define DMA2D_DMA2D_BG_PFC_CTRL_BG_ALPHA_MASK (0xFF) 

static inline uint32_t dma2d_ll_get_dma2d_bg_pfc_ctrl_value(void)
{
    return REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
}

static inline void dma2d_ll_set_dma2d_bg_pfc_ctrl_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_BG_PFC_CTRL_ADDR,value);
}

/* REG_0x0d:dma2d_bg_pfc_ctrl->bg_color_mode:0xd[3:0],background image color mode.; 0000:ARGB888;0001:RGB888;0010:RGB565;0011:ARGB1555;0100:ARGB4444; 0101:L8;0110:AL44; 0111:AL88; 1000:L4; 1001:A8;1010:A4; 1011lYUYV,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_color_mode(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_PFC_CTRL_BG_COLOR_MODE_POS) & DMA2D_DMA2D_BG_PFC_CTRL_BG_COLOR_MODE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_pfc_ctrl_bg_color_mode(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_PFC_CTRL_BG_COLOR_MODE_MASK << DMA2D_DMA2D_BG_PFC_CTRL_BG_COLOR_MODE_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_PFC_CTRL_BG_COLOR_MODE_MASK) << DMA2D_DMA2D_BG_PFC_CTRL_BG_COLOR_MODE_POS);
    REG_WRITE(DMA2D_DMA2D_BG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0d:dma2d_bg_pfc_ctrl->bg_clut_color_mode:0xd[4],color mode for clut. 0:argb8888; 1:rgb888.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_clut_color_mode(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_PFC_CTRL_BG_CLUT_COLOR_MODE_POS) & DMA2D_DMA2D_BG_PFC_CTRL_BG_CLUT_COLOR_MODE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_pfc_ctrl_bg_clut_color_mode(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_PFC_CTRL_BG_CLUT_COLOR_MODE_MASK << DMA2D_DMA2D_BG_PFC_CTRL_BG_CLUT_COLOR_MODE_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_PFC_CTRL_BG_CLUT_COLOR_MODE_MASK) << DMA2D_DMA2D_BG_PFC_CTRL_BG_CLUT_COLOR_MODE_POS);
    REG_WRITE(DMA2D_DMA2D_BG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0d:dma2d_bg_pfc_ctrl->bg_start_clut:0xd[5],automatic load the clut. Automatic reset.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_start_clut(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_PFC_CTRL_BG_START_CLUT_POS) & DMA2D_DMA2D_BG_PFC_CTRL_BG_START_CLUT_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_pfc_ctrl_bg_start_clut(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_PFC_CTRL_BG_START_CLUT_MASK << DMA2D_DMA2D_BG_PFC_CTRL_BG_START_CLUT_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_PFC_CTRL_BG_START_CLUT_MASK) << DMA2D_DMA2D_BG_PFC_CTRL_BG_START_CLUT_POS);
    REG_WRITE(DMA2D_DMA2D_BG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0d:dma2d_bg_pfc_ctrl->bg_y2r_yuv_fmt:0xd[7:6],0:yuyv, 1:uyvy, 2:yyuv, 3:uvyy, 4:vuyy.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_y2r_yuv_fmt(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_YUV_FMT_POS) & DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_YUV_FMT_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_pfc_ctrl_bg_y2r_yuv_fmt(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_YUV_FMT_MASK << DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_YUV_FMT_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_YUV_FMT_MASK) << DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_YUV_FMT_POS);
    REG_WRITE(DMA2D_DMA2D_BG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0d:dma2d_bg_pfc_ctrl->bg_clut_size:0xd[15:8],the size of clut used for background image. Size = fg_clut_size + 1;,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_clut_size(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_PFC_CTRL_BG_CLUT_SIZE_POS) & DMA2D_DMA2D_BG_PFC_CTRL_BG_CLUT_SIZE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_pfc_ctrl_bg_clut_size(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_PFC_CTRL_BG_CLUT_SIZE_MASK << DMA2D_DMA2D_BG_PFC_CTRL_BG_CLUT_SIZE_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_PFC_CTRL_BG_CLUT_SIZE_MASK) << DMA2D_DMA2D_BG_PFC_CTRL_BG_CLUT_SIZE_POS);
    REG_WRITE(DMA2D_DMA2D_BG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0d:dma2d_bg_pfc_ctrl->bg_alpha_mode:0xd[17:16],alpha value use for fg image. 00: nochange; 01:replace orginal, 10: alpha[7:0] multipied with orginal value.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_alpha_mode(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_PFC_CTRL_BG_ALPHA_MODE_POS) & DMA2D_DMA2D_BG_PFC_CTRL_BG_ALPHA_MODE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_pfc_ctrl_bg_alpha_mode(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_PFC_CTRL_BG_ALPHA_MODE_MASK << DMA2D_DMA2D_BG_PFC_CTRL_BG_ALPHA_MODE_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_PFC_CTRL_BG_ALPHA_MODE_MASK) << DMA2D_DMA2D_BG_PFC_CTRL_BG_ALPHA_MODE_POS);
    REG_WRITE(DMA2D_DMA2D_BG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0d:dma2d_bg_pfc_ctrl->alpha_invert:0xd[20],invert alpha value.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_pfc_ctrl_alpha_invert(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_PFC_CTRL_ALPHA_INVERT_POS) & DMA2D_DMA2D_BG_PFC_CTRL_ALPHA_INVERT_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_pfc_ctrl_alpha_invert(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_PFC_CTRL_ALPHA_INVERT_MASK << DMA2D_DMA2D_BG_PFC_CTRL_ALPHA_INVERT_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_PFC_CTRL_ALPHA_INVERT_MASK) << DMA2D_DMA2D_BG_PFC_CTRL_ALPHA_INVERT_POS);
    REG_WRITE(DMA2D_DMA2D_BG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0d:dma2d_bg_pfc_ctrl->bg_rb_swap:0xd[21],red blue swap to support rgb or argb. 0: regular mode.1:swap_mode.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_rb_swap(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_PFC_CTRL_BG_RB_SWAP_POS) & DMA2D_DMA2D_BG_PFC_CTRL_BG_RB_SWAP_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_pfc_ctrl_bg_rb_swap(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_PFC_CTRL_BG_RB_SWAP_MASK << DMA2D_DMA2D_BG_PFC_CTRL_BG_RB_SWAP_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_PFC_CTRL_BG_RB_SWAP_MASK) << DMA2D_DMA2D_BG_PFC_CTRL_BG_RB_SWAP_POS);
    REG_WRITE(DMA2D_DMA2D_BG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0d:dma2d_bg_pfc_ctrl->bg_y2r_byte_reve:0xd[22],yuv2rgb input data revrese byte by byte.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_y2r_byte_reve(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_BYTE_REVE_POS) & DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_BYTE_REVE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_pfc_ctrl_bg_y2r_byte_reve(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_BYTE_REVE_MASK << DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_BYTE_REVE_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_BYTE_REVE_MASK) << DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_BYTE_REVE_POS);
    REG_WRITE(DMA2D_DMA2D_BG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0d:dma2d_bg_pfc_ctrl->bg_y2r_hword_reve:0xd[23],yuv2rgb input data revrese halfword by halfword.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_y2r_hword_reve(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_HWORD_REVE_POS) & DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_HWORD_REVE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_pfc_ctrl_bg_y2r_hword_reve(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_HWORD_REVE_MASK << DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_HWORD_REVE_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_HWORD_REVE_MASK) << DMA2D_DMA2D_BG_PFC_CTRL_BG_Y2R_HWORD_REVE_POS);
    REG_WRITE(DMA2D_DMA2D_BG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0d:dma2d_bg_pfc_ctrl->bg_alpha:0xd[31:24],bg alpha value set. Use withbg_alpha_mode.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_pfc_ctrl_bg_alpha(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_PFC_CTRL_BG_ALPHA_POS) & DMA2D_DMA2D_BG_PFC_CTRL_BG_ALPHA_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_pfc_ctrl_bg_alpha(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_PFC_CTRL_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_PFC_CTRL_BG_ALPHA_MASK << DMA2D_DMA2D_BG_PFC_CTRL_BG_ALPHA_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_PFC_CTRL_BG_ALPHA_MASK) << DMA2D_DMA2D_BG_PFC_CTRL_BG_ALPHA_POS);
    REG_WRITE(DMA2D_DMA2D_BG_PFC_CTRL_ADDR,reg_value);
}

/* REG_0x0E */
#define DMA2D_DMA2D_BG_COLOR_REG_ADDR  (DMA2D_LL_REG_BASE  + 0xE*4) //REG ADDR :0x48080038
#define DMA2D_DMA2D_BG_COLOR_REG_BLUE_VALUE_POS (0) 
#define DMA2D_DMA2D_BG_COLOR_REG_BLUE_VALUE_MASK (0xFF) 

#define DMA2D_DMA2D_BG_COLOR_REG_GREEN_VALUE_POS (8) 
#define DMA2D_DMA2D_BG_COLOR_REG_GREEN_VALUE_MASK (0xFF) 

#define DMA2D_DMA2D_BG_COLOR_REG_RED_VALUE_POS (16) 
#define DMA2D_DMA2D_BG_COLOR_REG_RED_VALUE_MASK (0xFF) 

#define DMA2D_DMA2D_BG_COLOR_REG_RESERVED_POS (24) 
#define DMA2D_DMA2D_BG_COLOR_REG_RESERVED_MASK (0xFF) 

static inline uint32_t dma2d_ll_get_dma2d_bg_color_reg_value(void)
{
    return REG_READ(DMA2D_DMA2D_BG_COLOR_REG_ADDR);
}

static inline void dma2d_ll_set_dma2d_bg_color_reg_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_BG_COLOR_REG_ADDR,value);
}

/* REG_0x0e:dma2d_bg_color_reg->blue_value:0xe[7:0],blue_value in a4 or a8 mode of bg,,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_color_reg_blue_value(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_COLOR_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_COLOR_REG_BLUE_VALUE_POS) & DMA2D_DMA2D_BG_COLOR_REG_BLUE_VALUE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_color_reg_blue_value(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_COLOR_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_COLOR_REG_BLUE_VALUE_MASK << DMA2D_DMA2D_BG_COLOR_REG_BLUE_VALUE_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_COLOR_REG_BLUE_VALUE_MASK) << DMA2D_DMA2D_BG_COLOR_REG_BLUE_VALUE_POS);
    REG_WRITE(DMA2D_DMA2D_BG_COLOR_REG_ADDR,reg_value);
}

/* REG_0x0e:dma2d_bg_color_reg->green_value:0xe[15:8],green_value in a4 or a8 mode of bg,,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_color_reg_green_value(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_COLOR_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_COLOR_REG_GREEN_VALUE_POS) & DMA2D_DMA2D_BG_COLOR_REG_GREEN_VALUE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_color_reg_green_value(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_COLOR_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_COLOR_REG_GREEN_VALUE_MASK << DMA2D_DMA2D_BG_COLOR_REG_GREEN_VALUE_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_COLOR_REG_GREEN_VALUE_MASK) << DMA2D_DMA2D_BG_COLOR_REG_GREEN_VALUE_POS);
    REG_WRITE(DMA2D_DMA2D_BG_COLOR_REG_ADDR,reg_value);
}

/* REG_0x0e:dma2d_bg_color_reg->red_value:0xe[23:16],red_value in a4 or a8 mode of bg,,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_color_reg_red_value(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_COLOR_REG_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_COLOR_REG_RED_VALUE_POS) & DMA2D_DMA2D_BG_COLOR_REG_RED_VALUE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_color_reg_red_value(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_COLOR_REG_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_COLOR_REG_RED_VALUE_MASK << DMA2D_DMA2D_BG_COLOR_REG_RED_VALUE_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_COLOR_REG_RED_VALUE_MASK) << DMA2D_DMA2D_BG_COLOR_REG_RED_VALUE_POS);
    REG_WRITE(DMA2D_DMA2D_BG_COLOR_REG_ADDR,reg_value);
}

/* REG_0x0F */
#define DMA2D_FG_CLUT_MEM_ADDRESS_ADDR  (DMA2D_LL_REG_BASE  + 0xF*4) //REG ADDR :0x4808003c
#define DMA2D_FG_CLUT_MEM_ADDRESS_FG_CLUT_ADDRESS_POS (0) 
#define DMA2D_FG_CLUT_MEM_ADDRESS_FG_CLUT_ADDRESS_MASK (0xFFFFFFFF) 

static inline uint32_t dma2d_ll_get_fg_clut_mem_address_value(void)
{
    return REG_READ(DMA2D_FG_CLUT_MEM_ADDRESS_ADDR);
}

static inline void dma2d_ll_set_fg_clut_mem_address_value(uint32_t value)
{
    REG_WRITE(DMA2D_FG_CLUT_MEM_ADDRESS_ADDR,value);
}

/* REG_0x0f:fg_clut_mem_address->fg_clut_address:0xf[31:0],clut data dedicated to the fg image.,0,RW*/
static inline uint32_t dma2d_ll_get_fg_clut_mem_address_fg_clut_address(void)
{
    return REG_READ(DMA2D_FG_CLUT_MEM_ADDRESS_ADDR);
}

static inline void dma2d_ll_set_fg_clut_mem_address_fg_clut_address(uint32_t value)
{
    REG_WRITE(DMA2D_FG_CLUT_MEM_ADDRESS_ADDR,value);
}

/* REG_0x10 */
#define DMA2D_BG_CLUT_MEM_ADDRESS_ADDR  (DMA2D_LL_REG_BASE  + 0x10*4) //REG ADDR :0x48080040
#define DMA2D_BG_CLUT_MEM_ADDRESS_BG_CLUT_ADDRESS_POS (0) 
#define DMA2D_BG_CLUT_MEM_ADDRESS_BG_CLUT_ADDRESS_MASK (0xFFFFFFFF) 

static inline uint32_t dma2d_ll_get_bg_clut_mem_address_value(void)
{
    return REG_READ(DMA2D_BG_CLUT_MEM_ADDRESS_ADDR);
}

static inline void dma2d_ll_set_bg_clut_mem_address_value(uint32_t value)
{
    REG_WRITE(DMA2D_BG_CLUT_MEM_ADDRESS_ADDR,value);
}

/* REG_0x10:bg_clut_mem_address->bg_clut_address:0x10[31:0],clut data dedicated to the bg image.,0,RW*/
static inline uint32_t dma2d_ll_get_bg_clut_mem_address_bg_clut_address(void)
{
    return REG_READ(DMA2D_BG_CLUT_MEM_ADDRESS_ADDR);
}

static inline void dma2d_ll_set_bg_clut_mem_address_bg_clut_address(uint32_t value)
{
    REG_WRITE(DMA2D_BG_CLUT_MEM_ADDRESS_ADDR,value);
}

/* REG_0x11 */
#define DMA2D_OUT_PFC_CONTRL_ADDR  (DMA2D_LL_REG_BASE  + 0x11*4) //REG ADDR :0x48080044
#define DMA2D_OUT_PFC_CONTRL_OUT_COLOR_MODE_POS (0) 
#define DMA2D_OUT_PFC_CONTRL_OUT_COLOR_MODE_MASK (0x7) 

#define DMA2D_OUT_PFC_CONTRL_OUT_R2Y_YUV_FMT_POS (3) 
#define DMA2D_OUT_PFC_CONTRL_OUT_R2Y_YUV_FMT_MASK (0x7) 

#define DMA2D_OUT_PFC_CONTRL_RESERVED0_POS (6) 
#define DMA2D_OUT_PFC_CONTRL_RESERVED0_MASK (0x3FFF) 

#define DMA2D_OUT_PFC_CONTRL_OUT_ALPHA_INVERT_POS (20) 
#define DMA2D_OUT_PFC_CONTRL_OUT_ALPHA_INVERT_MASK (0x1) 

#define DMA2D_OUT_PFC_CONTRL_OUT_RB_SWAP_POS (21)
#define DMA2D_OUT_PFC_CONTRL_OUT_RB_SWAP_MASK (0x1)

#define DMA2D_OUT_PFC_CONTRL_RESERVED_POS (22)
#define DMA2D_OUT_PFC_CONTRL_RESERVED_MASK (0x3FF)

static inline uint32_t dma2d_ll_get_out_pfc_contrl_value(void)
{
    return REG_READ(DMA2D_OUT_PFC_CONTRL_ADDR);
}

static inline void dma2d_ll_set_out_pfc_contrl_value(uint32_t value)
{
    REG_WRITE(DMA2D_OUT_PFC_CONTRL_ADDR,value);
}

/* REG_0x11:out_pfc_contrl->out_color_mode:0x11[2:0],format of output image.0:argb888; 1:rgb888; 010:rgb565; 011:argb1555; 100:argb444, 101:yuyv.,0,RW*/
static inline uint32_t dma2d_ll_get_out_pfc_contrl_out_color_mode(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_OUT_PFC_CONTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_OUT_PFC_CONTRL_OUT_COLOR_MODE_POS) & DMA2D_OUT_PFC_CONTRL_OUT_COLOR_MODE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_out_pfc_contrl_out_color_mode(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_OUT_PFC_CONTRL_ADDR);
    reg_value &= ~(DMA2D_OUT_PFC_CONTRL_OUT_COLOR_MODE_MASK << DMA2D_OUT_PFC_CONTRL_OUT_COLOR_MODE_POS);
    reg_value |= ((value & DMA2D_OUT_PFC_CONTRL_OUT_COLOR_MODE_MASK) << DMA2D_OUT_PFC_CONTRL_OUT_COLOR_MODE_POS);
    REG_WRITE(DMA2D_OUT_PFC_CONTRL_ADDR,reg_value);
}

/* REG_0x11:out_pfc_contrl->out_r2y_yuv_fmt:0x11[5:3],0:yuyv, 1:yyuv, 2:uvyy, 3:uyvy, 4:vyuy, 5:vuyy.,0,RW*/
static inline uint32_t dma2d_ll_get_out_pfc_contrl_out_r2y_yuv_fmt(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_OUT_PFC_CONTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_OUT_PFC_CONTRL_OUT_R2Y_YUV_FMT_POS) & DMA2D_OUT_PFC_CONTRL_OUT_R2Y_YUV_FMT_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_out_pfc_contrl_out_r2y_yuv_fmt(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_OUT_PFC_CONTRL_ADDR);
    reg_value &= ~(DMA2D_OUT_PFC_CONTRL_OUT_R2Y_YUV_FMT_MASK << DMA2D_OUT_PFC_CONTRL_OUT_R2Y_YUV_FMT_POS);
    reg_value |= ((value & DMA2D_OUT_PFC_CONTRL_OUT_R2Y_YUV_FMT_MASK) << DMA2D_OUT_PFC_CONTRL_OUT_R2Y_YUV_FMT_POS);
    REG_WRITE(DMA2D_OUT_PFC_CONTRL_ADDR,reg_value);
}

/* REG_0x11:out_pfc_contrl->out_alpha_invert.:0x11[20],invert alpha value.,0,RW*/
static inline uint32_t dma2d_ll_get_out_pfc_contrl_out_alpha_invert(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_OUT_PFC_CONTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_OUT_PFC_CONTRL_OUT_ALPHA_INVERT_POS) & DMA2D_OUT_PFC_CONTRL_OUT_ALPHA_INVERT_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_out_pfc_contrl_out_alpha_invert(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_OUT_PFC_CONTRL_ADDR);
    reg_value &= ~(DMA2D_OUT_PFC_CONTRL_OUT_ALPHA_INVERT_MASK << DMA2D_OUT_PFC_CONTRL_OUT_ALPHA_INVERT_POS);
    reg_value |= ((value & DMA2D_OUT_PFC_CONTRL_OUT_ALPHA_INVERT_MASK) << DMA2D_OUT_PFC_CONTRL_OUT_ALPHA_INVERT_POS);
    REG_WRITE(DMA2D_OUT_PFC_CONTRL_ADDR,reg_value);
}

/* REG_0x11:out_pfc_contrl->out_rb_swap:0x11[21],ed blue swap to support rgb or argb. 0: regular mode.1:swap_mode.,0,RW*/
static inline uint32_t dma2d_ll_get_out_pfc_contrl_out_rb_swap(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_OUT_PFC_CONTRL_ADDR);
    reg_value = ((reg_value >> DMA2D_OUT_PFC_CONTRL_OUT_RB_SWAP_POS) & DMA2D_OUT_PFC_CONTRL_OUT_RB_SWAP_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_out_pfc_contrl_out_rb_swap(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_OUT_PFC_CONTRL_ADDR);
    reg_value &= ~(DMA2D_OUT_PFC_CONTRL_OUT_RB_SWAP_MASK << DMA2D_OUT_PFC_CONTRL_OUT_RB_SWAP_POS);
    reg_value |= ((value & DMA2D_OUT_PFC_CONTRL_OUT_RB_SWAP_MASK) << DMA2D_OUT_PFC_CONTRL_OUT_RB_SWAP_POS);
    REG_WRITE(DMA2D_OUT_PFC_CONTRL_ADDR,reg_value);
}

/* REG_0x12 */
#define DMA2D_OUT_COLOR_REG_ADDR  (DMA2D_LL_REG_BASE  + 0x12*4) //REG ADDR :0x48080048
#define DMA2D_OUT_COLOR_REG_OUTPUT_CLOR_REG_POS (0) 
#define DMA2D_OUT_COLOR_REG_OUTPUT_CLOR_REG_MASK (0xFFFFFFFF) 

static inline uint32_t dma2d_ll_get_out_color_reg_value(void)
{
    return REG_READ(DMA2D_OUT_COLOR_REG_ADDR);
}

static inline void dma2d_ll_set_out_color_reg_value(uint32_t value)
{
    REG_WRITE(DMA2D_OUT_COLOR_REG_ADDR,value);
}

/* REG_0x12:out_color_reg->output_clor_reg:0x12[31:0],output reg, in different color mode. Output is different.; Argb888 or rgb888: alpha[31:24];red[23:16]; green[15:8];blue[7:0].; Rgb565:red[15:11]; green[12:5]; blue[4:0];; argb1555:a[15];red[14:10]; green[4:0]; blue[4:0];; argb4444:alpha[15:12];red[11:8];green[7:4];blue[3;0].,0,RW*/
static inline uint32_t dma2d_ll_get_out_color_reg_output_clor_reg(void)
{
    return REG_READ(DMA2D_OUT_COLOR_REG_ADDR);
}

static inline void dma2d_ll_set_out_color_reg_output_clor_reg(uint32_t value)
{
    REG_WRITE(DMA2D_OUT_COLOR_REG_ADDR,value);
}

/* REG_0x13 */
#define DMA2D_DMA2D_OUT_MEM_ADDRESS_ADDR  (DMA2D_LL_REG_BASE  + 0x13*4) //REG ADDR :0x4808004c
#define DMA2D_DMA2D_OUT_MEM_ADDRESS_OUT_MEM_ADDRESS_POS (0) 
#define DMA2D_DMA2D_OUT_MEM_ADDRESS_OUT_MEM_ADDRESS_MASK (0xFFFFFFFF) 

static inline uint32_t dma2d_ll_get_dma2d_out_mem_address_value(void)
{
    return REG_READ(DMA2D_DMA2D_OUT_MEM_ADDRESS_ADDR);
}

static inline void dma2d_ll_set_dma2d_out_mem_address_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_OUT_MEM_ADDRESS_ADDR,value);
}

/* REG_0x13:dma2d_out_mem_address->out_mem_address:0x13[31:0],address of data output fifo.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_out_mem_address_out_mem_address(void)
{
    return REG_READ(DMA2D_DMA2D_OUT_MEM_ADDRESS_ADDR);
}

static inline void dma2d_ll_set_dma2d_out_mem_address_out_mem_address(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_OUT_MEM_ADDRESS_ADDR,value);
}

/* REG_0x14 */
#define DMA2D_OUTPUT_OFFSET_ADDR  (DMA2D_LL_REG_BASE  + 0x14*4) //REG ADDR :0x48080050
#define DMA2D_OUTPUT_OFFSET_OUT_LINE_OFFSET_POS (0) 
#define DMA2D_OUTPUT_OFFSET_OUT_LINE_OFFSET_MASK (0xFFFF) 

#define DMA2D_OUTPUT_OFFSET_RESERVED_POS (16) 
#define DMA2D_OUTPUT_OFFSET_RESERVED_MASK (0xFFFF) 

static inline uint32_t dma2d_ll_get_output_offset_value(void)
{
    return REG_READ(DMA2D_OUTPUT_OFFSET_ADDR);
}

static inline void dma2d_ll_set_output_offset_value(uint32_t value)
{
    REG_WRITE(DMA2D_OUTPUT_OFFSET_ADDR,value);
}

/* REG_0x14:output_offset->out_line_offset:0x14[15:0],output line offset. Offfset with bytes or pixel.in pixel[15:14] ignored.,0,RW*/
static inline uint32_t dma2d_ll_get_output_offset_out_line_offset(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_OUTPUT_OFFSET_ADDR);
    reg_value = ((reg_value >> DMA2D_OUTPUT_OFFSET_OUT_LINE_OFFSET_POS) & DMA2D_OUTPUT_OFFSET_OUT_LINE_OFFSET_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_output_offset_out_line_offset(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_OUTPUT_OFFSET_ADDR);
    reg_value &= ~(DMA2D_OUTPUT_OFFSET_OUT_LINE_OFFSET_MASK << DMA2D_OUTPUT_OFFSET_OUT_LINE_OFFSET_POS);
    reg_value |= ((value & DMA2D_OUTPUT_OFFSET_OUT_LINE_OFFSET_MASK) << DMA2D_OUTPUT_OFFSET_OUT_LINE_OFFSET_POS);
    REG_WRITE(DMA2D_OUTPUT_OFFSET_ADDR,reg_value);
}

/* REG_0x15 */
#define DMA2D_DMA2D_NUMBER_OF_LINE_ADDR  (DMA2D_LL_REG_BASE  + 0x15*4) //REG ADDR :0x48080054
#define DMA2D_DMA2D_NUMBER_OF_LINE_NUMBER_LINE_POS (0) 
#define DMA2D_DMA2D_NUMBER_OF_LINE_NUMBER_LINE_MASK (0xFFFF) 

#define DMA2D_DMA2D_NUMBER_OF_LINE_PEXEL_LINE_POS (16) 
#define DMA2D_DMA2D_NUMBER_OF_LINE_PEXEL_LINE_MASK (0x3FFF) 

#define DMA2D_DMA2D_NUMBER_OF_LINE_RESERVED_POS (30) 
#define DMA2D_DMA2D_NUMBER_OF_LINE_RESERVED_MASK (0x3) 

static inline uint32_t dma2d_ll_get_dma2d_number_of_line_value(void)
{
    return REG_READ(DMA2D_DMA2D_NUMBER_OF_LINE_ADDR);
}

static inline void dma2d_ll_set_dma2d_number_of_line_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_NUMBER_OF_LINE_ADDR,value);
}

/* REG_0x15:dma2d_number_of_line->number_line:0x15[15:0],X PIXEL.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_number_of_line_number_line(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_NUMBER_OF_LINE_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_NUMBER_OF_LINE_NUMBER_LINE_POS) & DMA2D_DMA2D_NUMBER_OF_LINE_NUMBER_LINE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_number_of_line_number_line(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_NUMBER_OF_LINE_ADDR);
    reg_value &= ~(DMA2D_DMA2D_NUMBER_OF_LINE_NUMBER_LINE_MASK << DMA2D_DMA2D_NUMBER_OF_LINE_NUMBER_LINE_POS);
    reg_value |= ((value & DMA2D_DMA2D_NUMBER_OF_LINE_NUMBER_LINE_MASK) << DMA2D_DMA2D_NUMBER_OF_LINE_NUMBER_LINE_POS);
    REG_WRITE(DMA2D_DMA2D_NUMBER_OF_LINE_ADDR,reg_value);
}

/* REG_0x15:dma2d_number_of_line->pexel_line:0x15[29:16],Y_PIXEL.,None,RW*/
static inline uint32_t dma2d_ll_get_dma2d_number_of_line_pexel_line(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_NUMBER_OF_LINE_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_NUMBER_OF_LINE_PEXEL_LINE_POS) & DMA2D_DMA2D_NUMBER_OF_LINE_PEXEL_LINE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_number_of_line_pexel_line(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_NUMBER_OF_LINE_ADDR);
    reg_value &= ~(DMA2D_DMA2D_NUMBER_OF_LINE_PEXEL_LINE_MASK << DMA2D_DMA2D_NUMBER_OF_LINE_PEXEL_LINE_POS);
    reg_value |= ((value & DMA2D_DMA2D_NUMBER_OF_LINE_PEXEL_LINE_MASK) << DMA2D_DMA2D_NUMBER_OF_LINE_PEXEL_LINE_POS);
    REG_WRITE(DMA2D_DMA2D_NUMBER_OF_LINE_ADDR,reg_value);
}

/* REG_0x16 */
#define DMA2D_DMA2D_LINE_WATERMARK_ADDR  (DMA2D_LL_REG_BASE  + 0x16*4) //REG ADDR :0x48080058
#define DMA2D_DMA2D_LINE_WATERMARK_LINE_WATERMARK_POS (0) 
#define DMA2D_DMA2D_LINE_WATERMARK_LINE_WATERMARK_MASK (0xFFFF) 

#define DMA2D_DMA2D_LINE_WATERMARK_RESERVED_POS (16) 
#define DMA2D_DMA2D_LINE_WATERMARK_RESERVED_MASK (0xFFFF) 

static inline uint32_t dma2d_ll_get_dma2d_line_watermark_value(void)
{
    return REG_READ(DMA2D_DMA2D_LINE_WATERMARK_ADDR);
}

static inline void dma2d_ll_set_dma2d_line_watermark_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_LINE_WATERMARK_ADDR,value);
}

/* REG_0x16:dma2d_line_watermark->line_watermark:0x16[15:0],config the line watermark int generation, transfer reach the watermark, int hold on.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_line_watermark_line_watermark(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_LINE_WATERMARK_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_LINE_WATERMARK_LINE_WATERMARK_POS) & DMA2D_DMA2D_LINE_WATERMARK_LINE_WATERMARK_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_line_watermark_line_watermark(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_LINE_WATERMARK_ADDR);
    reg_value &= ~(DMA2D_DMA2D_LINE_WATERMARK_LINE_WATERMARK_MASK << DMA2D_DMA2D_LINE_WATERMARK_LINE_WATERMARK_POS);
    reg_value |= ((value & DMA2D_DMA2D_LINE_WATERMARK_LINE_WATERMARK_MASK) << DMA2D_DMA2D_LINE_WATERMARK_LINE_WATERMARK_POS);
    REG_WRITE(DMA2D_DMA2D_LINE_WATERMARK_ADDR,reg_value);
}

/* REG_0x100 */
#define DMA2D_DMA2D_FG_CLUT0_ADDR  (DMA2D_LL_REG_BASE  + 0x100*4) //REG ADDR :0x48080400
#define DMA2D_DMA2D_FG_CLUT0_BLUE_POS (0) 
#define DMA2D_DMA2D_FG_CLUT0_BLUE_MASK (0xFF) 

#define DMA2D_DMA2D_FG_CLUT0_GREEN_POS (8) 
#define DMA2D_DMA2D_FG_CLUT0_GREEN_MASK (0xFF) 

#define DMA2D_DMA2D_FG_CLUT0_RED_POS (16) 
#define DMA2D_DMA2D_FG_CLUT0_RED_MASK (0xFF) 

#define DMA2D_DMA2D_FG_CLUT0_ALPHA_POS (24) 
#define DMA2D_DMA2D_FG_CLUT0_ALPHA_MASK (0xFF) 

static inline uint32_t dma2d_ll_get_dma2d_fg_clut0_value(void)
{
    return REG_READ(DMA2D_DMA2D_FG_CLUT0_ADDR);
}

static inline void dma2d_ll_set_dma2d_fg_clut0_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_FG_CLUT0_ADDR,value);
}

/* REG_0x100:dma2d_fg_clut0->blue:0x100[7:0],blue value foe index<y>for the fg.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_clut0_blue(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_CLUT0_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_CLUT0_BLUE_POS) & DMA2D_DMA2D_FG_CLUT0_BLUE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_clut0_blue(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_CLUT0_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_CLUT0_BLUE_MASK << DMA2D_DMA2D_FG_CLUT0_BLUE_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_CLUT0_BLUE_MASK) << DMA2D_DMA2D_FG_CLUT0_BLUE_POS);
    REG_WRITE(DMA2D_DMA2D_FG_CLUT0_ADDR,reg_value);
}

/* REG_0x100:dma2d_fg_clut0->green:0x100[15:8],green value foe index<y>for the fg.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_clut0_green(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_CLUT0_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_CLUT0_GREEN_POS) & DMA2D_DMA2D_FG_CLUT0_GREEN_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_clut0_green(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_CLUT0_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_CLUT0_GREEN_MASK << DMA2D_DMA2D_FG_CLUT0_GREEN_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_CLUT0_GREEN_MASK) << DMA2D_DMA2D_FG_CLUT0_GREEN_POS);
    REG_WRITE(DMA2D_DMA2D_FG_CLUT0_ADDR,reg_value);
}

/* REG_0x100:dma2d_fg_clut0->red:0x100[23:16],red value foe index<y>for the fg.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_clut0_red(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_CLUT0_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_CLUT0_RED_POS) & DMA2D_DMA2D_FG_CLUT0_RED_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_clut0_red(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_CLUT0_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_CLUT0_RED_MASK << DMA2D_DMA2D_FG_CLUT0_RED_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_CLUT0_RED_MASK) << DMA2D_DMA2D_FG_CLUT0_RED_POS);
    REG_WRITE(DMA2D_DMA2D_FG_CLUT0_ADDR,reg_value);
}

/* REG_0x100:dma2d_fg_clut0->alpha:0x100[31:24],alpha value foe index<y>for the fg.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_clut0_alpha(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_CLUT0_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_CLUT0_ALPHA_POS) & DMA2D_DMA2D_FG_CLUT0_ALPHA_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_clut0_alpha(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_CLUT0_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_CLUT0_ALPHA_MASK << DMA2D_DMA2D_FG_CLUT0_ALPHA_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_CLUT0_ALPHA_MASK) << DMA2D_DMA2D_FG_CLUT0_ALPHA_POS);
    REG_WRITE(DMA2D_DMA2D_FG_CLUT0_ADDR,reg_value);
}

/* REG_0x1FF */
#define DMA2D_DMA2D_FG_CLUT255_ADDR  (DMA2D_LL_REG_BASE  + 0x1FF*4) //REG ADDR :0x480807fc
#define DMA2D_DMA2D_FG_CLUT255_BLUE_POS (0) 
#define DMA2D_DMA2D_FG_CLUT255_BLUE_MASK (0xFF) 

#define DMA2D_DMA2D_FG_CLUT255_GREEN_POS (8) 
#define DMA2D_DMA2D_FG_CLUT255_GREEN_MASK (0xFF) 

#define DMA2D_DMA2D_FG_CLUT255_RED_POS (16) 
#define DMA2D_DMA2D_FG_CLUT255_RED_MASK (0xFF) 

#define DMA2D_DMA2D_FG_CLUT255_ALPHA_POS (24) 
#define DMA2D_DMA2D_FG_CLUT255_ALPHA_MASK (0xFF) 

static inline uint32_t dma2d_ll_get_dma2d_fg_clut255_value(void)
{
    return REG_READ(DMA2D_DMA2D_FG_CLUT255_ADDR);
}

static inline void dma2d_ll_set_dma2d_fg_clut255_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_FG_CLUT255_ADDR,value);
}

/* REG_0x1ff:dma2d_fg_clut255->blue:0x1ff[7:0],blue value foe index<y>for the fg.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_clut255_blue(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_CLUT255_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_CLUT255_BLUE_POS) & DMA2D_DMA2D_FG_CLUT255_BLUE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_clut255_blue(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_CLUT255_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_CLUT255_BLUE_MASK << DMA2D_DMA2D_FG_CLUT255_BLUE_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_CLUT255_BLUE_MASK) << DMA2D_DMA2D_FG_CLUT255_BLUE_POS);
    REG_WRITE(DMA2D_DMA2D_FG_CLUT255_ADDR,reg_value);
}

/* REG_0x1ff:dma2d_fg_clut255->green:0x1ff[15:8],green value foe index<y>for the fg.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_clut255_green(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_CLUT255_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_CLUT255_GREEN_POS) & DMA2D_DMA2D_FG_CLUT255_GREEN_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_clut255_green(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_CLUT255_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_CLUT255_GREEN_MASK << DMA2D_DMA2D_FG_CLUT255_GREEN_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_CLUT255_GREEN_MASK) << DMA2D_DMA2D_FG_CLUT255_GREEN_POS);
    REG_WRITE(DMA2D_DMA2D_FG_CLUT255_ADDR,reg_value);
}

/* REG_0x1ff:dma2d_fg_clut255->red:0x1ff[23:16],red value foe index<y>for the fg.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_clut255_red(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_CLUT255_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_CLUT255_RED_POS) & DMA2D_DMA2D_FG_CLUT255_RED_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_clut255_red(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_CLUT255_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_CLUT255_RED_MASK << DMA2D_DMA2D_FG_CLUT255_RED_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_CLUT255_RED_MASK) << DMA2D_DMA2D_FG_CLUT255_RED_POS);
    REG_WRITE(DMA2D_DMA2D_FG_CLUT255_ADDR,reg_value);
}

/* REG_0x1ff:dma2d_fg_clut255->alpha:0x1ff[31:24],alpha value foe index<y>for the fg.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_fg_clut255_alpha(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_CLUT255_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_FG_CLUT255_ALPHA_POS) & DMA2D_DMA2D_FG_CLUT255_ALPHA_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_fg_clut255_alpha(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_FG_CLUT255_ADDR);
    reg_value &= ~(DMA2D_DMA2D_FG_CLUT255_ALPHA_MASK << DMA2D_DMA2D_FG_CLUT255_ALPHA_POS);
    reg_value |= ((value & DMA2D_DMA2D_FG_CLUT255_ALPHA_MASK) << DMA2D_DMA2D_FG_CLUT255_ALPHA_POS);
    REG_WRITE(DMA2D_DMA2D_FG_CLUT255_ADDR,reg_value);
}

/* REG_0x200 */
#define DMA2D_DMA2D_BG_CLUT0_ADDR  (DMA2D_LL_REG_BASE  + 0x200*4) //REG ADDR :0x48080800
#define DMA2D_DMA2D_BG_CLUT0_BLUE_POS (0) 
#define DMA2D_DMA2D_BG_CLUT0_BLUE_MASK (0xFF) 

#define DMA2D_DMA2D_BG_CLUT0_GREEN_POS (8) 
#define DMA2D_DMA2D_BG_CLUT0_GREEN_MASK (0xFF) 

#define DMA2D_DMA2D_BG_CLUT0_RED_POS (16) 
#define DMA2D_DMA2D_BG_CLUT0_RED_MASK (0xFF) 

#define DMA2D_DMA2D_BG_CLUT0_ALPHA_POS (24) 
#define DMA2D_DMA2D_BG_CLUT0_ALPHA_MASK (0xFF) 

static inline uint32_t dma2d_ll_get_dma2d_bg_clut0_value(void)
{
    return REG_READ(DMA2D_DMA2D_BG_CLUT0_ADDR);
}

static inline void dma2d_ll_set_dma2d_bg_clut0_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_BG_CLUT0_ADDR,value);
}

/* REG_0x200:dma2d_bg_clut0->blue:0x200[7:0],blue value foe index<y>for the bg.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_clut0_blue(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_CLUT0_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_CLUT0_BLUE_POS) & DMA2D_DMA2D_BG_CLUT0_BLUE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_clut0_blue(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_CLUT0_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_CLUT0_BLUE_MASK << DMA2D_DMA2D_BG_CLUT0_BLUE_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_CLUT0_BLUE_MASK) << DMA2D_DMA2D_BG_CLUT0_BLUE_POS);
    REG_WRITE(DMA2D_DMA2D_BG_CLUT0_ADDR,reg_value);
}

/* REG_0x200:dma2d_bg_clut0->green:0x200[15:8],green value foe index<y>for the bg.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_clut0_green(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_CLUT0_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_CLUT0_GREEN_POS) & DMA2D_DMA2D_BG_CLUT0_GREEN_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_clut0_green(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_CLUT0_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_CLUT0_GREEN_MASK << DMA2D_DMA2D_BG_CLUT0_GREEN_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_CLUT0_GREEN_MASK) << DMA2D_DMA2D_BG_CLUT0_GREEN_POS);
    REG_WRITE(DMA2D_DMA2D_BG_CLUT0_ADDR,reg_value);
}

/* REG_0x200:dma2d_bg_clut0->red:0x200[23:16],red value foe index<y>for the bg.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_clut0_red(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_CLUT0_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_CLUT0_RED_POS) & DMA2D_DMA2D_BG_CLUT0_RED_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_clut0_red(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_CLUT0_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_CLUT0_RED_MASK << DMA2D_DMA2D_BG_CLUT0_RED_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_CLUT0_RED_MASK) << DMA2D_DMA2D_BG_CLUT0_RED_POS);
    REG_WRITE(DMA2D_DMA2D_BG_CLUT0_ADDR,reg_value);
}

/* REG_0x200:dma2d_bg_clut0->alpha:0x200[31:24],alpha value foe index<y>for the bg.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_clut0_alpha(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_CLUT0_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_CLUT0_ALPHA_POS) & DMA2D_DMA2D_BG_CLUT0_ALPHA_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_clut0_alpha(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_CLUT0_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_CLUT0_ALPHA_MASK << DMA2D_DMA2D_BG_CLUT0_ALPHA_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_CLUT0_ALPHA_MASK) << DMA2D_DMA2D_BG_CLUT0_ALPHA_POS);
    REG_WRITE(DMA2D_DMA2D_BG_CLUT0_ADDR,reg_value);
}

/* REG_0x2FF */
#define DMA2D_DMA2D_BG_CLUT255_ADDR  (DMA2D_LL_REG_BASE  + 0x2FF*4) //REG ADDR :0x48080bfc
#define DMA2D_DMA2D_BG_CLUT255_BLUE_POS (0) 
#define DMA2D_DMA2D_BG_CLUT255_BLUE_MASK (0xFF) 

#define DMA2D_DMA2D_BG_CLUT255_GREEN_POS (8) 
#define DMA2D_DMA2D_BG_CLUT255_GREEN_MASK (0xFF) 

#define DMA2D_DMA2D_BG_CLUT255_RED_POS (16) 
#define DMA2D_DMA2D_BG_CLUT255_RED_MASK (0xFF) 

#define DMA2D_DMA2D_BG_CLUT255_ALPHA_POS (24) 
#define DMA2D_DMA2D_BG_CLUT255_ALPHA_MASK (0xFF) 

static inline uint32_t dma2d_ll_get_dma2d_bg_clut255_value(void)
{
    return REG_READ(DMA2D_DMA2D_BG_CLUT255_ADDR);
}

static inline void dma2d_ll_set_dma2d_bg_clut255_value(uint32_t value)
{
    REG_WRITE(DMA2D_DMA2D_BG_CLUT255_ADDR,value);
}

/* REG_0x2ff:dma2d_bg_clut255->blue:0x2ff[7:0],blue value foe index<y>for the bg.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_clut255_blue(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_CLUT255_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_CLUT255_BLUE_POS) & DMA2D_DMA2D_BG_CLUT255_BLUE_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_clut255_blue(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_CLUT255_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_CLUT255_BLUE_MASK << DMA2D_DMA2D_BG_CLUT255_BLUE_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_CLUT255_BLUE_MASK) << DMA2D_DMA2D_BG_CLUT255_BLUE_POS);
    REG_WRITE(DMA2D_DMA2D_BG_CLUT255_ADDR,reg_value);
}

/* REG_0x2ff:dma2d_bg_clut255->green:0x2ff[15:8],green value foe index<y>for the bg.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_clut255_green(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_CLUT255_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_CLUT255_GREEN_POS) & DMA2D_DMA2D_BG_CLUT255_GREEN_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_clut255_green(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_CLUT255_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_CLUT255_GREEN_MASK << DMA2D_DMA2D_BG_CLUT255_GREEN_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_CLUT255_GREEN_MASK) << DMA2D_DMA2D_BG_CLUT255_GREEN_POS);
    REG_WRITE(DMA2D_DMA2D_BG_CLUT255_ADDR,reg_value);
}

/* REG_0x2ff:dma2d_bg_clut255->red:0x2ff[23:16],red value foe index<y>for the bg.,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_clut255_red(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_CLUT255_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_CLUT255_RED_POS) & DMA2D_DMA2D_BG_CLUT255_RED_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_clut255_red(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_CLUT255_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_CLUT255_RED_MASK << DMA2D_DMA2D_BG_CLUT255_RED_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_CLUT255_RED_MASK) << DMA2D_DMA2D_BG_CLUT255_RED_POS);
    REG_WRITE(DMA2D_DMA2D_BG_CLUT255_ADDR,reg_value);
}

/* REG_0x2ff:dma2d_bg_clut255->alpha:0x2ff[31:24],alpha value foe index<y>for the bg. t,0,RW*/
static inline uint32_t dma2d_ll_get_dma2d_bg_clut255_alpha(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_CLUT255_ADDR);
    reg_value = ((reg_value >> DMA2D_DMA2D_BG_CLUT255_ALPHA_POS) & DMA2D_DMA2D_BG_CLUT255_ALPHA_MASK);
    return reg_value;
}

static inline void dma2d_ll_set_dma2d_bg_clut255_alpha(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(DMA2D_DMA2D_BG_CLUT255_ADDR);
    reg_value &= ~(DMA2D_DMA2D_BG_CLUT255_ALPHA_MASK << DMA2D_DMA2D_BG_CLUT255_ALPHA_POS);
    reg_value |= ((value & DMA2D_DMA2D_BG_CLUT255_ALPHA_MASK) << DMA2D_DMA2D_BG_CLUT255_ALPHA_POS);
    REG_WRITE(DMA2D_DMA2D_BG_CLUT255_ADDR,reg_value);
}

#ifdef __cplusplus 
}                  
#endif             
