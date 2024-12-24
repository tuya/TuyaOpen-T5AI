// Copyright 2021-2022 Beken
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
* This file is generated from BK7256_ADDR_Mapping_MP2.xlsm automatically
* Modify it manually is not recommended
* CHIP ID:BK7256,GENARATE TIME:2022-07-08 17:30:58
************************************************************************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <soc/soc.h>

#define LCD_DISP_LL_REG_BASE      (SOC_LCD_DISP_REG_BASE) //REG_BASE:0x48060000

/* REG_0x00 */
#define LCD_DISP_DEVICE_ID_ADDR  (LCD_DISP_LL_REG_BASE  + 0x0*4) //REG ADDR :0x48060000
#define LCD_DISP_DEVICE_ID_DEVICE_ID_POS (0) 
#define LCD_DISP_DEVICE_ID_DEVICE_ID_MASK (0xFFFFFFFF) 

static inline uint32_t lcd_disp_ll_get_device_id_value(void)
{
    return REG_READ(LCD_DISP_DEVICE_ID_ADDR);
}

/* REG_0x00:device_id->device_id:0x0[31:0],Device ID:ASCII Code "DISP",0x00415544,R*/
static inline uint32_t lcd_disp_ll_get_device_id_device_id(void)
{
    return REG_READ(LCD_DISP_DEVICE_ID_ADDR);
}

/* REG_0x01 */
#define LCD_DISP_VERSION_ID_ADDR  (LCD_DISP_LL_REG_BASE  + 0x1*4) //REG ADDR :0x48060004
#define LCD_DISP_VERSION_ID_VERSION_ID_POS (0) 
#define LCD_DISP_VERSION_ID_VERSION_ID_MASK (0xFFFFFFFF) 

static inline uint32_t lcd_disp_ll_get_version_id_value(void)
{
    return REG_READ(LCD_DISP_VERSION_ID_ADDR);
}

/* REG_0x01:version_id->version_id:0x1[31:0],Version ID: ¡ã?¡À?o? V1.1,0x00010001,R*/
static inline uint32_t lcd_disp_ll_get_version_id_version_id(void)
{
    return REG_READ(LCD_DISP_VERSION_ID_ADDR);
}

/* REG_0x02 */
#define LCD_DISP_MODULE_CONTROL_ADDR  (LCD_DISP_LL_REG_BASE  + 0x2*4) //REG ADDR :0x48060008
#define LCD_DISP_MODULE_CONTROL_SOFT_RESET_POS (0) 
#define LCD_DISP_MODULE_CONTROL_SOFT_RESET_MASK (0x1) 

#define LCD_DISP_MODULE_CONTROL_CLK_GATE_POS (1) 
#define LCD_DISP_MODULE_CONTROL_CLK_GATE_MASK (0x1) 

#define LCD_DISP_MODULE_CONTROL_RESERVED_POS (2) 
#define LCD_DISP_MODULE_CONTROL_RESERVED_MASK (0x3FFFFFFF) 

static inline uint32_t lcd_disp_ll_get_module_control_value(void)
{
    return REG_READ(LCD_DISP_MODULE_CONTROL_ADDR);
}

static inline void lcd_disp_ll_set_module_control_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_MODULE_CONTROL_ADDR,value);
}

/* REG_0x02:module_control->soft_reset:0x2[0],¨¨¨ª?tD¡ä1?¡ä??DISP?¡ê?¨¦¡ê?D¨¨¨°a¨¨¨ª?t??3y,0x0,R/W*/
static inline uint32_t lcd_disp_ll_get_module_control_soft_reset(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_MODULE_CONTROL_ADDR);
    reg_value = ((reg_value >> LCD_DISP_MODULE_CONTROL_SOFT_RESET_POS) & LCD_DISP_MODULE_CONTROL_SOFT_RESET_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_module_control_soft_reset(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_MODULE_CONTROL_ADDR);
    reg_value &= ~(LCD_DISP_MODULE_CONTROL_SOFT_RESET_MASK << LCD_DISP_MODULE_CONTROL_SOFT_RESET_POS);
    reg_value |= ((value & LCD_DISP_MODULE_CONTROL_SOFT_RESET_MASK) << LCD_DISP_MODULE_CONTROL_SOFT_RESET_POS);
    REG_WRITE(LCD_DISP_MODULE_CONTROL_ADDR,reg_value);
}

/* REG_0x02:module_control->clk_gate:0x2[1],0x0: ¨º1?¨¹ahb clock gate; 0x1: 2?¨º1?¨¹ahb clock gate,0x0,R/W*/
static inline uint32_t lcd_disp_ll_get_module_control_clk_gate(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_MODULE_CONTROL_ADDR);
    reg_value = ((reg_value >> LCD_DISP_MODULE_CONTROL_CLK_GATE_POS) & LCD_DISP_MODULE_CONTROL_CLK_GATE_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_module_control_clk_gate(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_MODULE_CONTROL_ADDR);
    reg_value &= ~(LCD_DISP_MODULE_CONTROL_CLK_GATE_MASK << LCD_DISP_MODULE_CONTROL_CLK_GATE_POS);
    reg_value |= ((value & LCD_DISP_MODULE_CONTROL_CLK_GATE_MASK) << LCD_DISP_MODULE_CONTROL_CLK_GATE_POS);
    REG_WRITE(LCD_DISP_MODULE_CONTROL_ADDR,reg_value);
}

/* REG_0x03 */
#define LCD_DISP_GLOBAL_STATUS_ADDR  (LCD_DISP_LL_REG_BASE  + 0x3*4)
static inline uint32_t lcd_disp_ll_get_global_status(void)
{
    return REG_READ(LCD_DISP_GLOBAL_STATUS_ADDR);
}

/* REG_0x04 */
#define LCD_DISP_DISPLAY_INT_ADDR  (LCD_DISP_LL_REG_BASE  + 0x4*4) //REG ADDR :0x48060010
#define LCD_DISP_DISPLAY_INT_RGB_INT_EN_POS (0) 
#define LCD_DISP_DISPLAY_INT_RGB_INT_EN_MASK (0x3) 

#define LCD_DISP_DISPLAY_INT_I8080_INT_EN_POS (2) 
#define LCD_DISP_DISPLAY_INT_I8080_INT_EN_MASK (0x3) 

#define LCD_DISP_DISPLAY_INT_RGB_SOF_POS (4) 
#define LCD_DISP_DISPLAY_INT_RGB_SOF_MASK (0x1) 

#define LCD_DISP_DISPLAY_INT_RGB_EOF_POS (5) 
#define LCD_DISP_DISPLAY_INT_RGB_EOF_MASK (0x1) 

#define LCD_DISP_DISPLAY_INT_I8080_SOF_POS (6) 
#define LCD_DISP_DISPLAY_INT_I8080_SOF_MASK (0x1) 

#define LCD_DISP_DISPLAY_INT_I8080_EOF_POS (7) 
#define LCD_DISP_DISPLAY_INT_I8080_EOF_MASK (0x1) 

#define LCD_DISP_DISPLAY_INT_INT_DE_POS (8) 
#define LCD_DISP_DISPLAY_INT_INT_DE_MASK (0x1) 

#define LCD_DISP_DISPLAY_INT_INT_TEAR_EFF_POS (9) 
#define LCD_DISP_DISPLAY_INT_INT_TEAR_EFF_MASK (0x1) 

#define LCD_DISP_DISPLAY_INT_INT_TIME_ARV_POS (10) 
#define LCD_DISP_DISPLAY_INT_INT_TIME_ARV_MASK (0x1) 

#define LCD_DISP_DISPLAY_INT_RESERVED0_POS (11) 
#define LCD_DISP_DISPLAY_INT_RESERVED0_MASK (0x3FFF) 

#define LCD_DISP_DISPLAY_INT_INT_TIME_ARV_ENA_POS (25) 
#define LCD_DISP_DISPLAY_INT_INT_TIME_ARV_ENA_MASK (0x1) 

#define LCD_DISP_DISPLAY_INT_INT_TEAR_EFF_ENA_POS (26) 
#define LCD_DISP_DISPLAY_INT_INT_TEAR_EFF_ENA_MASK (0x1) 

#define LCD_DISP_DISPLAY_INT_DE_INT_EN_POS (27) 
#define LCD_DISP_DISPLAY_INT_DE_INT_EN_MASK (0x1) 

#define LCD_DISP_DISPLAY_INT_DISCONTI_MODE_POS (28) 
#define LCD_DISP_DISPLAY_INT_DISCONTI_MODE_MASK (0x1) 

#define LCD_DISP_DISPLAY_INT_RESERVED1_POS (29) 
#define LCD_DISP_DISPLAY_INT_RESERVED1_MASK (0x7) 

static inline uint32_t lcd_disp_ll_get_display_int_value(void)
{
    return REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
}

static inline void lcd_disp_ll_set_display_int_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_DISPLAY_INT_ADDR,value);
}

static inline uint32_t lcd_disp_ll_get_display_int_rgb_int_en(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISPLAY_INT_RGB_INT_EN_POS) & LCD_DISP_DISPLAY_INT_RGB_INT_EN_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_display_int_rgb_int_en(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value &= ~(LCD_DISP_DISPLAY_INT_RGB_INT_EN_MASK << LCD_DISP_DISPLAY_INT_RGB_INT_EN_POS);
    reg_value |= ((value & LCD_DISP_DISPLAY_INT_RGB_INT_EN_MASK) << LCD_DISP_DISPLAY_INT_RGB_INT_EN_POS);
    REG_WRITE(LCD_DISP_DISPLAY_INT_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_display_int_i8080_int_en(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISPLAY_INT_I8080_INT_EN_POS) & LCD_DISP_DISPLAY_INT_I8080_INT_EN_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_display_int_i8080_int_en(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value &= ~(LCD_DISP_DISPLAY_INT_I8080_INT_EN_MASK << LCD_DISP_DISPLAY_INT_I8080_INT_EN_POS);
    reg_value |= ((value & LCD_DISP_DISPLAY_INT_I8080_INT_EN_MASK) << LCD_DISP_DISPLAY_INT_I8080_INT_EN_POS);
    REG_WRITE(LCD_DISP_DISPLAY_INT_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_display_int_rgb_sof(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISPLAY_INT_RGB_SOF_POS) & LCD_DISP_DISPLAY_INT_RGB_SOF_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_display_int_rgb_sof(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value &= ~(LCD_DISP_DISPLAY_INT_RGB_SOF_MASK << LCD_DISP_DISPLAY_INT_RGB_SOF_POS);
    reg_value |= ((value & LCD_DISP_DISPLAY_INT_RGB_SOF_MASK) << LCD_DISP_DISPLAY_INT_RGB_SOF_POS);
    REG_WRITE(LCD_DISP_DISPLAY_INT_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_display_int_rgb_eof(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISPLAY_INT_RGB_EOF_POS) & LCD_DISP_DISPLAY_INT_RGB_EOF_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_display_int_rgb_eof(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value &= ~(LCD_DISP_DISPLAY_INT_RGB_EOF_MASK << LCD_DISP_DISPLAY_INT_RGB_EOF_POS);
    reg_value |= ((value & LCD_DISP_DISPLAY_INT_RGB_EOF_MASK) << LCD_DISP_DISPLAY_INT_RGB_EOF_POS);
    REG_WRITE(LCD_DISP_DISPLAY_INT_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_display_int_i8080_sof(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISPLAY_INT_I8080_SOF_POS) & LCD_DISP_DISPLAY_INT_I8080_SOF_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_display_int_i8080_sof(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value &= ~(LCD_DISP_DISPLAY_INT_I8080_SOF_MASK << LCD_DISP_DISPLAY_INT_I8080_SOF_POS);
    reg_value |= ((value & LCD_DISP_DISPLAY_INT_I8080_SOF_MASK) << LCD_DISP_DISPLAY_INT_I8080_SOF_POS);
    REG_WRITE(LCD_DISP_DISPLAY_INT_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_display_int_i8080_eof(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISPLAY_INT_I8080_EOF_POS) & LCD_DISP_DISPLAY_INT_I8080_EOF_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_display_int_i8080_eof(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value &= ~(LCD_DISP_DISPLAY_INT_I8080_EOF_MASK << LCD_DISP_DISPLAY_INT_I8080_EOF_POS);
    reg_value |= ((value & LCD_DISP_DISPLAY_INT_I8080_EOF_MASK) << LCD_DISP_DISPLAY_INT_I8080_EOF_POS);
    REG_WRITE(LCD_DISP_DISPLAY_INT_ADDR,reg_value);
}

/* REG_0x04:display_int->int_de:0x4[8],de signal discontious interrupt,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_display_int_int_de(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISPLAY_INT_INT_DE_POS) & LCD_DISP_DISPLAY_INT_INT_DE_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_display_int_int_de(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value &= ~(LCD_DISP_DISPLAY_INT_INT_DE_MASK << LCD_DISP_DISPLAY_INT_INT_DE_POS);
    reg_value |= ((value & LCD_DISP_DISPLAY_INT_INT_DE_MASK) << LCD_DISP_DISPLAY_INT_INT_DE_POS);
    REG_WRITE(LCD_DISP_DISPLAY_INT_ADDR,reg_value);
}

/* REG_0x04:display_int->int_tear_eff:0x4[9],tear effect posedge interrupt,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_display_int_int_tear_eff(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISPLAY_INT_INT_TEAR_EFF_POS) & LCD_DISP_DISPLAY_INT_INT_TEAR_EFF_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_display_int_int_tear_eff(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value &= ~(LCD_DISP_DISPLAY_INT_INT_TEAR_EFF_MASK << LCD_DISP_DISPLAY_INT_INT_TEAR_EFF_POS);
    reg_value |= ((value & LCD_DISP_DISPLAY_INT_INT_TEAR_EFF_MASK) << LCD_DISP_DISPLAY_INT_INT_TEAR_EFF_POS);
    REG_WRITE(LCD_DISP_DISPLAY_INT_ADDR,reg_value);
}

/* REG_0x04:display_int->int_time_arv:0x4[10],frame delay arrive interrupt,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_display_int_int_time_arv(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISPLAY_INT_INT_TIME_ARV_POS) & LCD_DISP_DISPLAY_INT_INT_TIME_ARV_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_display_int_int_time_arv(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value &= ~(LCD_DISP_DISPLAY_INT_INT_TIME_ARV_MASK << LCD_DISP_DISPLAY_INT_INT_TIME_ARV_POS);
    reg_value |= ((value & LCD_DISP_DISPLAY_INT_INT_TIME_ARV_MASK) << LCD_DISP_DISPLAY_INT_INT_TIME_ARV_POS);
    REG_WRITE(LCD_DISP_DISPLAY_INT_ADDR,reg_value);
}

/* REG_0x04:display_int->int_time_arv_ena:0x4[25],frame delay arrive interrupt enable,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_display_int_int_time_arv_ena(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISPLAY_INT_INT_TIME_ARV_ENA_POS) & LCD_DISP_DISPLAY_INT_INT_TIME_ARV_ENA_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_display_int_int_time_arv_ena(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value &= ~(LCD_DISP_DISPLAY_INT_INT_TIME_ARV_ENA_MASK << LCD_DISP_DISPLAY_INT_INT_TIME_ARV_ENA_POS);
    reg_value |= ((value & LCD_DISP_DISPLAY_INT_INT_TIME_ARV_ENA_MASK) << LCD_DISP_DISPLAY_INT_INT_TIME_ARV_ENA_POS);
    REG_WRITE(LCD_DISP_DISPLAY_INT_ADDR,reg_value);
}

/* REG_0x04:display_int->int_tear_eff_ena:0x4[26],tear effect posedge interrupt enable,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_display_int_int_tear_eff_ena(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISPLAY_INT_INT_TEAR_EFF_ENA_POS) & LCD_DISP_DISPLAY_INT_INT_TEAR_EFF_ENA_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_display_int_int_tear_eff_ena(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value &= ~(LCD_DISP_DISPLAY_INT_INT_TEAR_EFF_ENA_MASK << LCD_DISP_DISPLAY_INT_INT_TEAR_EFF_ENA_POS);
    reg_value |= ((value & LCD_DISP_DISPLAY_INT_INT_TEAR_EFF_ENA_MASK) << LCD_DISP_DISPLAY_INT_INT_TEAR_EFF_ENA_POS);
    REG_WRITE(LCD_DISP_DISPLAY_INT_ADDR,reg_value);
}

/* REG_0x04:display_int->de_int_en:0x4[27],de signal discontious interrupt enable,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_display_int_de_int_en(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISPLAY_INT_DE_INT_EN_POS) & LCD_DISP_DISPLAY_INT_DE_INT_EN_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_display_int_de_int_en(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value &= ~(LCD_DISP_DISPLAY_INT_DE_INT_EN_MASK << LCD_DISP_DISPLAY_INT_DE_INT_EN_POS);
    reg_value |= ((value & LCD_DISP_DISPLAY_INT_DE_INT_EN_MASK) << LCD_DISP_DISPLAY_INT_DE_INT_EN_POS);
    REG_WRITE(LCD_DISP_DISPLAY_INT_ADDR,reg_value);
}

/* REG_0x04:display_int->disconti_mode:0x4[28],¨ºy?Y2?¨¢?D??¡ê¨º?¡ê?0¡êoclose¡ê?1¡êoopen?¡ê Rgb?¡ê¨º?¡êorgb clk¦Ì?25mhz¨º¡À¡ê?discontiD¨¨¨°a¡ä¨°?a,2?¨¨??¨´?¨¨?¨²2?¨¦?;I8080?¡ê¨º?¡êo??¨º1¨®?disconti_mode.,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_display_int_disconti_mode(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISPLAY_INT_DISCONTI_MODE_POS) & LCD_DISP_DISPLAY_INT_DISCONTI_MODE_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_display_int_disconti_mode(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISPLAY_INT_ADDR);
    reg_value &= ~(LCD_DISP_DISPLAY_INT_DISCONTI_MODE_MASK << LCD_DISP_DISPLAY_INT_DISCONTI_MODE_POS);
    reg_value |= ((value & LCD_DISP_DISPLAY_INT_DISCONTI_MODE_MASK) << LCD_DISP_DISPLAY_INT_DISCONTI_MODE_POS);
    REG_WRITE(LCD_DISP_DISPLAY_INT_ADDR,reg_value);
}

/* REG_0x05 */
#define LCD_DISP_RGB_CFG_ADDR  (LCD_DISP_LL_REG_BASE  + 0x5*4)
#define LCD_DISP_RGB_CFG_X_PIXEL_POS (0)
#define LCD_DISP_RGB_CFG_X_PIXEL_MASK (0x7FF)

#define LCD_DISP_RGB_CFG_DCLK_REV_POS (11)
#define LCD_DISP_RGB_CFG_DCLK_REV_MASK (0x1)

#define LCD_DISP_RGB_CFG_Y_PIXEL_POS (12)
#define LCD_DISP_RGB_CFG_Y_PIXEL_MASK (0x7FF)

#define LCD_DISP_RGB_CFG_OUT_RGB666_LSB_SEL_POS (23) 
#define LCD_DISP_RGB_CFG_OUT_RGB666_LSB_SEL_MASK (0x1) 

#define LCD_DISP_RGB_CFG_RGB_DISP_ON_POS (24)
#define LCD_DISP_RGB_CFG_RGB_DISP_ON_MASK (0x1)

#define LCD_DISP_RGB_CFG_RGB_ON_POS (25)
#define LCD_DISP_RGB_CFG_RGB_ON_MASK (0x1)

#define LCD_DISP_RGB_CFG_LCD_DISPLAY_ON_POS (26)
#define LCD_DISP_RGB_CFG_LCD_DISPLAY_ON_MASK (0x1)

#define LCD_DISP_RGB_CFG_OUT_FMT_SEL_POS (27) 
#define LCD_DISP_RGB_CFG_OUT_FMT_SEL_MASK (0x7) 

#define LCD_DISP_RGB_CFG_RESERVED_POS (30) 
#define LCD_DISP_RGB_CFG_RESERVED_MASK (0x1) 

#define LCD_DISP_RGB_CFG_SRC_DATA_SEL_POS (31) 
#define LCD_DISP_RGB_CFG_SRC_DATA_SEL_MASK (0x1) 

static inline uint32_t lcd_disp_ll_get_rgb_cfg_value(void)
{
    return REG_READ(LCD_DISP_RGB_CFG_ADDR);
}

static inline void lcd_disp_ll_set_rgb_cfg_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_RGB_CFG_ADDR,value);
}

static inline uint32_t lcd_disp_ll_get_rgb_cfg_x_pixel(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_CFG_X_PIXEL_POS) & LCD_DISP_RGB_CFG_X_PIXEL_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_cfg_x_pixel(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value &= ~(LCD_DISP_RGB_CFG_X_PIXEL_MASK << LCD_DISP_RGB_CFG_X_PIXEL_POS);
    reg_value |= ((value & LCD_DISP_RGB_CFG_X_PIXEL_MASK) << LCD_DISP_RGB_CFG_X_PIXEL_POS);
    REG_WRITE(LCD_DISP_RGB_CFG_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_rgb_cfg_dclk_rev(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_CFG_DCLK_REV_POS) & LCD_DISP_RGB_CFG_DCLK_REV_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_cfg_dclk_rev(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value &= ~(LCD_DISP_RGB_CFG_DCLK_REV_MASK << LCD_DISP_RGB_CFG_DCLK_REV_POS);
    reg_value |= ((value & LCD_DISP_RGB_CFG_DCLK_REV_MASK) << LCD_DISP_RGB_CFG_DCLK_REV_POS);
    REG_WRITE(LCD_DISP_RGB_CFG_ADDR,reg_value);
}

/* REG_0x05:rgb_cfg->y_pixel:0x5[22:12],¨¢D????3¡è?¨¨,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_rgb_cfg_y_pixel(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_CFG_Y_PIXEL_POS) & LCD_DISP_RGB_CFG_Y_PIXEL_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_cfg_y_pixel(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value &= ~(LCD_DISP_RGB_CFG_Y_PIXEL_MASK << LCD_DISP_RGB_CFG_Y_PIXEL_POS);
    reg_value |= ((value & LCD_DISP_RGB_CFG_Y_PIXEL_MASK) << LCD_DISP_RGB_CFG_Y_PIXEL_POS);
    REG_WRITE(LCD_DISP_RGB_CFG_ADDR,reg_value);
}

/* REG_0x05:rgb_cfg->out_rgb666_lsb_sel:0x5[23],rgb666?¡ê¨º???¦Ì¨ª??21??¦Ì??????¡ê1.¦Ì¨ª???3?¨®21???¡ê0.¨°?rgb888?a¡Á?¡ê?????21???¡ê,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_rgb_cfg_out_rgb666_lsb_sel(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_CFG_OUT_RGB666_LSB_SEL_POS) & LCD_DISP_RGB_CFG_OUT_RGB666_LSB_SEL_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_cfg_out_rgb666_lsb_sel(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value &= ~(LCD_DISP_RGB_CFG_OUT_RGB666_LSB_SEL_MASK << LCD_DISP_RGB_CFG_OUT_RGB666_LSB_SEL_POS);
    reg_value |= ((value & LCD_DISP_RGB_CFG_OUT_RGB666_LSB_SEL_MASK) << LCD_DISP_RGB_CFG_OUT_RGB666_LSB_SEL_POS);
    REG_WRITE(LCD_DISP_RGB_CFG_ADDR,reg_value);
}

/* REG_0x05:rgb_cfg->rgb_disp_on:0x5[24],rgb display enable,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_rgb_cfg_rgb_disp_on(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_CFG_RGB_DISP_ON_POS) & LCD_DISP_RGB_CFG_RGB_DISP_ON_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_cfg_rgb_disp_on(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value &= ~(LCD_DISP_RGB_CFG_RGB_DISP_ON_MASK << LCD_DISP_RGB_CFG_RGB_DISP_ON_POS);
    reg_value |= ((value & LCD_DISP_RGB_CFG_RGB_DISP_ON_MASK) << LCD_DISP_RGB_CFG_RGB_DISP_ON_POS);
    REG_WRITE(LCD_DISP_RGB_CFG_ADDR,reg_value);
}

/* REG_0x05:rgb_cfg->rgb_on:0x5[25],rgb output function enable,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_rgb_cfg_rgb_on(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_CFG_RGB_ON_POS) & LCD_DISP_RGB_CFG_RGB_ON_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_cfg_rgb_on(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value &= ~(LCD_DISP_RGB_CFG_RGB_ON_MASK << LCD_DISP_RGB_CFG_RGB_ON_POS);
    reg_value |= ((value & LCD_DISP_RGB_CFG_RGB_ON_MASK) << LCD_DISP_RGB_CFG_RGB_ON_POS);
    REG_WRITE(LCD_DISP_RGB_CFG_ADDR,reg_value);
}

/* REG_0x05:rgb_cfg->lcd_display_on:0x5[26],lcd??????,?disp_on???????,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_rgb_cfg_lcd_display_on(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_CFG_LCD_DISPLAY_ON_POS) & LCD_DISP_RGB_CFG_LCD_DISPLAY_ON_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_cfg_lcd_display_on(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value &= ~(LCD_DISP_RGB_CFG_LCD_DISPLAY_ON_MASK << LCD_DISP_RGB_CFG_LCD_DISPLAY_ON_POS);
    reg_value |= ((value & LCD_DISP_RGB_CFG_LCD_DISPLAY_ON_MASK) << LCD_DISP_RGB_CFG_LCD_DISPLAY_ON_POS);
    REG_WRITE(LCD_DISP_RGB_CFG_ADDR,reg_value);
}

/* REG_0x05:rgb_cfg->out_fmt_sel:0x5[29:27],for rgb module: 0:rgb565 output; 1:rgb666 output; 2:rgb888 outptut. ; For i8080module: 0:yuv/rgb565 input, output rgb565.;                               1:yuv/rgb565 input, output rgb666.;                               2:rgb888 input, output rgb888;                               3:yuv/rgb565 input, output rgb888.;                               4:rgb888 input, output rgb565.;                               5:rgb888 input, output rgb666.;                               6:rgb888 input, output rgb888 byte by byte.;                               7yuv/rgb565 input, output rgb888.,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_rgb_cfg_out_fmt_sel(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_CFG_OUT_FMT_SEL_POS) & LCD_DISP_RGB_CFG_OUT_FMT_SEL_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_cfg_out_fmt_sel(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value &= ~(LCD_DISP_RGB_CFG_OUT_FMT_SEL_MASK << LCD_DISP_RGB_CFG_OUT_FMT_SEL_POS);
    reg_value |= ((value & LCD_DISP_RGB_CFG_OUT_FMT_SEL_MASK) << LCD_DISP_RGB_CFG_OUT_FMT_SEL_POS);
    REG_WRITE(LCD_DISP_RGB_CFG_ADDR,reg_value);
}

/* REG_0x05:rgb_cfg->src_data_sel:0x5[31],input data sel: 0:rgb565/yuv data.  1:rgb888 data.,0x0,r/w */
static inline uint32_t lcd_disp_ll_get_rgb_cfg_src_data_sel(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_CFG_SRC_DATA_SEL_POS) & LCD_DISP_RGB_CFG_SRC_DATA_SEL_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_cfg_src_data_sel(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CFG_ADDR);
    reg_value &= ~(LCD_DISP_RGB_CFG_SRC_DATA_SEL_MASK << LCD_DISP_RGB_CFG_SRC_DATA_SEL_POS);
    reg_value |= ((value & LCD_DISP_RGB_CFG_SRC_DATA_SEL_MASK) << LCD_DISP_RGB_CFG_SRC_DATA_SEL_POS);
    REG_WRITE(LCD_DISP_RGB_CFG_ADDR,reg_value);
}

/* REG_0x06 */
/* REG_0x07 */
#define LCD_DISP_SYNC_CFG_ADDR  (LCD_DISP_LL_REG_BASE  + 0x7*4)
#define LCD_DISP_SYNC_CFG_HSYNC_BACK_PORCH_POS (0)
#define LCD_DISP_SYNC_CFG_HSYNC_BACK_PORCH_MASK (0xFF)

#define LCD_DISP_SYNC_CFG_HSYNC_FRONT_PORCH_POS (8)
#define LCD_DISP_SYNC_CFG_HSYNC_FRONT_PORCH_MASK (0x7F)

#define LCD_DISP_SYNC_CFG_VSYNC_BACK_PORCH_POS (15)
#define LCD_DISP_SYNC_CFG_VSYNC_BACK_PORCH_MASK (0x1F)

#define LCD_DISP_SYNC_CFG_VSYNC_FRONT_PORCH_POS (20)
#define LCD_DISP_SYNC_CFG_VSYNC_FRONT_PORCH_MASK (0x7F)

#define LCD_DISP_SYNC_CFG_RESERVED4_POS (27) 
#define LCD_DISP_SYNC_CFG_RESERVED4_MASK (0x1) 

#define LCD_DISP_SYNC_CFG_YUV_SEL_POS (28) 
#define LCD_DISP_SYNC_CFG_YUV_SEL_MASK (0x7) 

#define LCD_DISP_SYNC_CFG_RESERVED5_POS (31) 
#define LCD_DISP_SYNC_CFG_RESERVED5_MASK (0x1) 

static inline uint32_t lcd_disp_ll_get_sync_cfg_value(void)
{
    return REG_READ(LCD_DISP_SYNC_CFG_ADDR);
}

static inline void lcd_disp_ll_set_sync_cfg_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_SYNC_CFG_ADDR,value);
}

static inline uint32_t lcd_disp_ll_get_sync_cfg_hsync_back_porch(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_SYNC_CFG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_SYNC_CFG_HSYNC_BACK_PORCH_POS) & LCD_DISP_SYNC_CFG_HSYNC_BACK_PORCH_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_sync_cfg_hsync_back_porch(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_SYNC_CFG_ADDR);
    reg_value &= ~(LCD_DISP_SYNC_CFG_HSYNC_BACK_PORCH_MASK << LCD_DISP_SYNC_CFG_HSYNC_BACK_PORCH_POS);
    reg_value |= ((value & LCD_DISP_SYNC_CFG_HSYNC_BACK_PORCH_MASK) << LCD_DISP_SYNC_CFG_HSYNC_BACK_PORCH_POS);
    REG_WRITE(LCD_DISP_SYNC_CFG_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_sync_cfg_hsync_front_porch(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_SYNC_CFG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_SYNC_CFG_HSYNC_FRONT_PORCH_POS) & LCD_DISP_SYNC_CFG_HSYNC_FRONT_PORCH_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_sync_cfg_hsync_front_porch(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_SYNC_CFG_ADDR);
    reg_value &= ~(LCD_DISP_SYNC_CFG_HSYNC_FRONT_PORCH_MASK << LCD_DISP_SYNC_CFG_HSYNC_FRONT_PORCH_POS);
    reg_value |= ((value & LCD_DISP_SYNC_CFG_HSYNC_FRONT_PORCH_MASK) << LCD_DISP_SYNC_CFG_HSYNC_FRONT_PORCH_POS);
    REG_WRITE(LCD_DISP_SYNC_CFG_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_sync_cfg_vsync_back_porch(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_SYNC_CFG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_SYNC_CFG_VSYNC_BACK_PORCH_POS) & LCD_DISP_SYNC_CFG_VSYNC_BACK_PORCH_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_sync_cfg_vsync_back_porch(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_SYNC_CFG_ADDR);
    reg_value &= ~(LCD_DISP_SYNC_CFG_VSYNC_BACK_PORCH_MASK << LCD_DISP_SYNC_CFG_VSYNC_BACK_PORCH_POS);
    reg_value |= ((value & LCD_DISP_SYNC_CFG_VSYNC_BACK_PORCH_MASK) << LCD_DISP_SYNC_CFG_VSYNC_BACK_PORCH_POS);
    REG_WRITE(LCD_DISP_SYNC_CFG_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_sync_cfg_vsync_front_porch(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_SYNC_CFG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_SYNC_CFG_VSYNC_FRONT_PORCH_POS) & LCD_DISP_SYNC_CFG_VSYNC_FRONT_PORCH_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_sync_cfg_vsync_front_porch(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_SYNC_CFG_ADDR);
    reg_value &= ~(LCD_DISP_SYNC_CFG_VSYNC_FRONT_PORCH_MASK << LCD_DISP_SYNC_CFG_VSYNC_FRONT_PORCH_POS);
    reg_value |= ((value & LCD_DISP_SYNC_CFG_VSYNC_FRONT_PORCH_MASK) << LCD_DISP_SYNC_CFG_VSYNC_FRONT_PORCH_POS);
    REG_WRITE(LCD_DISP_SYNC_CFG_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_sync_cfg_yuv_sel(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_SYNC_CFG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_SYNC_CFG_YUV_SEL_POS) & LCD_DISP_SYNC_CFG_YUV_SEL_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_sync_cfg_yuv_sel(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_SYNC_CFG_ADDR);
    reg_value &= ~(LCD_DISP_SYNC_CFG_YUV_SEL_MASK << LCD_DISP_SYNC_CFG_YUV_SEL_POS);
    reg_value |= ((value & LCD_DISP_SYNC_CFG_YUV_SEL_MASK) << LCD_DISP_SYNC_CFG_YUV_SEL_POS);
    REG_WRITE(LCD_DISP_SYNC_CFG_ADDR,reg_value);
}

/* REG_0x08 */
#define LCD_DISP_I8080_CONFIG_ADDR  (LCD_DISP_LL_REG_BASE  + 0x8*4)
#define LCD_DISP_I8080_CONFIG_I8080_DISP_EN_POS (0)
#define LCD_DISP_I8080_CONFIG_I8080_DISP_EN_MASK (0x1)

#define LCD_DISP_I8080_CONFIG_I8080_DAT_ON_POS (1)
#define LCD_DISP_I8080_CONFIG_I8080_DAT_ON_MASK (0x1)

#define LCD_DISP_I8080_CONFIG_I8080_FIFO_MODE_POS (2)
#define LCD_DISP_I8080_CONFIG_I8080_FIFO_MODE_MASK (0x1)

#define LCD_DISP_I8080_CONFIG_I8080_FIFO_CLR_POS (3)
#define LCD_DISP_I8080_CONFIG_I8080_FIFO_CLR_MASK (0x1)

#define LCD_DISP_I8080_CONFIG_I8080_CMDFIFO_CLR_POS (4)
#define LCD_DISP_I8080_CONFIG_I8080_CMDFIFO_CLR_MASK (0x1)

#define LCD_DISP_I8080_CONFIG_RESERVED0_POS (5) 
#define LCD_DISP_I8080_CONFIG_RESERVED0_MASK (0x3) 

#define LCD_DISP_I8080_CONFIG_GPIO_I8080_ON_POS (7) 
#define LCD_DISP_I8080_CONFIG_GPIO_I8080_ON_MASK (0x1) 

#define LCD_DISP_I8080_CONFIG_TIK_CNT_POS (8)
#define LCD_DISP_I8080_CONFIG_TIK_CNT_MASK (0x3)

#define LCD_DISP_I8080_CONFIG_RESERVED1_POS (10)
#define LCD_DISP_I8080_CONFIG_RESERVED1_MASK (0x3)

#define LCD_DISP_I8080_CONFIG_I8080_1MS_COUNT_POS (12)
#define LCD_DISP_I8080_CONFIG_I8080_1MS_COUNT_MASK (0x1FF)

#define LCD_DISP_I8080_CONFIG_RESERVED2_POS (21) 
#define LCD_DISP_I8080_CONFIG_RESERVED2_MASK (0x7FF) 

static inline uint32_t lcd_disp_ll_get_i8080_config_value(void)
{
    return REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
}

static inline void lcd_disp_ll_set_i8080_config_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_I8080_CONFIG_ADDR,value);
}

static inline uint32_t lcd_disp_ll_get_i8080_config_i8080_disp_en(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_I8080_CONFIG_I8080_DISP_EN_POS) & LCD_DISP_I8080_CONFIG_I8080_DISP_EN_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_i8080_config_i8080_disp_en(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
    reg_value &= ~(LCD_DISP_I8080_CONFIG_I8080_DISP_EN_MASK << LCD_DISP_I8080_CONFIG_I8080_DISP_EN_POS);
    reg_value |= ((value & LCD_DISP_I8080_CONFIG_I8080_DISP_EN_MASK) << LCD_DISP_I8080_CONFIG_I8080_DISP_EN_POS);
    REG_WRITE(LCD_DISP_I8080_CONFIG_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_i8080_config_i8080_dat_on(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_I8080_CONFIG_I8080_DAT_ON_POS) & LCD_DISP_I8080_CONFIG_I8080_DAT_ON_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_i8080_config_i8080_dat_on(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
    reg_value &= ~(LCD_DISP_I8080_CONFIG_I8080_DAT_ON_MASK << LCD_DISP_I8080_CONFIG_I8080_DAT_ON_POS);
    reg_value |= ((value & LCD_DISP_I8080_CONFIG_I8080_DAT_ON_MASK) << LCD_DISP_I8080_CONFIG_I8080_DAT_ON_POS);
    REG_WRITE(LCD_DISP_I8080_CONFIG_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_i8080_config_i8080_fifo_mode(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_I8080_CONFIG_I8080_FIFO_MODE_POS) & LCD_DISP_I8080_CONFIG_I8080_FIFO_MODE_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_i8080_config_i8080_fifo_mode(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
    reg_value &= ~(LCD_DISP_I8080_CONFIG_I8080_FIFO_MODE_MASK << LCD_DISP_I8080_CONFIG_I8080_FIFO_MODE_POS);
    reg_value |= ((value & LCD_DISP_I8080_CONFIG_I8080_FIFO_MODE_MASK) << LCD_DISP_I8080_CONFIG_I8080_FIFO_MODE_POS);
    REG_WRITE(LCD_DISP_I8080_CONFIG_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_i8080_config_i8080_fifo_clr(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_I8080_CONFIG_I8080_FIFO_CLR_POS) & LCD_DISP_I8080_CONFIG_I8080_FIFO_CLR_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_i8080_config_i8080_fifo_clr(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
    reg_value &= ~(LCD_DISP_I8080_CONFIG_I8080_FIFO_CLR_MASK << LCD_DISP_I8080_CONFIG_I8080_FIFO_CLR_POS);
    reg_value |= ((value & LCD_DISP_I8080_CONFIG_I8080_FIFO_CLR_MASK) << LCD_DISP_I8080_CONFIG_I8080_FIFO_CLR_POS);
    REG_WRITE(LCD_DISP_I8080_CONFIG_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_i8080_config_i8080_cmdfifo_clr(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_I8080_CONFIG_I8080_CMDFIFO_CLR_POS) & LCD_DISP_I8080_CONFIG_I8080_CMDFIFO_CLR_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_i8080_config_i8080_cmdfifo_clr(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
    reg_value &= ~(LCD_DISP_I8080_CONFIG_I8080_CMDFIFO_CLR_MASK << LCD_DISP_I8080_CONFIG_I8080_CMDFIFO_CLR_POS);
    reg_value |= ((value & LCD_DISP_I8080_CONFIG_I8080_CMDFIFO_CLR_MASK) << LCD_DISP_I8080_CONFIG_I8080_CMDFIFO_CLR_POS);
    REG_WRITE(LCD_DISP_I8080_CONFIG_ADDR,reg_value);
}

/* REG_0x08:i8080_config->gpio_i8080_on:0x8[7],i8080 output function enable,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_i8080_config_gpio_i8080_on(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_I8080_CONFIG_GPIO_I8080_ON_POS) & LCD_DISP_I8080_CONFIG_GPIO_I8080_ON_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_i8080_config_gpio_i8080_on(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
    reg_value &= ~(LCD_DISP_I8080_CONFIG_GPIO_I8080_ON_MASK << LCD_DISP_I8080_CONFIG_GPIO_I8080_ON_POS);
    reg_value |= ((value & LCD_DISP_I8080_CONFIG_GPIO_I8080_ON_MASK) << LCD_DISP_I8080_CONFIG_GPIO_I8080_ON_POS);
    REG_WRITE(LCD_DISP_I8080_CONFIG_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_i8080_config_tik_cnt(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_I8080_CONFIG_TIK_CNT_POS) & LCD_DISP_I8080_CONFIG_TIK_CNT_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_i8080_config_tik_cnt(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
    reg_value &= ~(LCD_DISP_I8080_CONFIG_TIK_CNT_MASK << LCD_DISP_I8080_CONFIG_TIK_CNT_POS);
    reg_value |= ((value & LCD_DISP_I8080_CONFIG_TIK_CNT_MASK) << LCD_DISP_I8080_CONFIG_TIK_CNT_POS);
    REG_WRITE(LCD_DISP_I8080_CONFIG_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_i8080_config_i8080_1ms_count(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
    reg_value = ((reg_value >> LCD_DISP_I8080_CONFIG_I8080_1MS_COUNT_POS) & LCD_DISP_I8080_CONFIG_I8080_1MS_COUNT_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_i8080_config_i8080_1ms_count(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CONFIG_ADDR);
    reg_value &= ~(LCD_DISP_I8080_CONFIG_I8080_1MS_COUNT_MASK << LCD_DISP_I8080_CONFIG_I8080_1MS_COUNT_POS);
    reg_value |= ((value & LCD_DISP_I8080_CONFIG_I8080_1MS_COUNT_MASK) << LCD_DISP_I8080_CONFIG_I8080_1MS_COUNT_POS);
    REG_WRITE(LCD_DISP_I8080_CONFIG_ADDR,reg_value);
}

/* REG_0x09 */
#define LCD_DISP_I8080_CMD_FIFO_ADDR  (LCD_DISP_LL_REG_BASE  + 0x9*4)
#define LCD_DISP_I8080_CMD_FIFO_I8080_CMD_FIFO_POS (0)
#define LCD_DISP_I8080_CMD_FIFO_I8080_CMD_FIFO_MASK (0xFFFF)

#define LCD_DISP_I8080_CMD_FIFO_RESERVED_POS (16)
#define LCD_DISP_I8080_CMD_FIFO_RESERVED_MASK (0xFFFF)

static inline void lcd_disp_ll_set_i8080_cmd_fifo_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_I8080_CMD_FIFO_ADDR,value);
}

/* REG_0x09:i8080_cmd_fifo->i8080_cmd_fifo:0x9[15:0],i8080 command fifo，初始化和持续读写的cmd写入此fifo。,0x0,w*/
static inline void lcd_disp_ll_set_i8080_cmd_fifo_i8080_cmd_fifo(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_CMD_FIFO_ADDR);
    reg_value &= ~(LCD_DISP_I8080_CMD_FIFO_I8080_CMD_FIFO_MASK << LCD_DISP_I8080_CMD_FIFO_I8080_CMD_FIFO_POS);
    reg_value |= ((value & LCD_DISP_I8080_CMD_FIFO_I8080_CMD_FIFO_MASK) << LCD_DISP_I8080_CMD_FIFO_I8080_CMD_FIFO_POS);
    REG_WRITE(LCD_DISP_I8080_CMD_FIFO_ADDR,reg_value);
}

/* REG_0x0A */
#define LCD_DISP_I8080_DAT_FIFO_ADDR  (LCD_DISP_LL_REG_BASE  + 0xA*4) //REG ADDR :0x48060028
#define LCD_DISP_I8080_DAT_FIFO_I8080_DAT_FIFO_POS (0) 
#define LCD_DISP_I8080_DAT_FIFO_I8080_DAT_FIFO_MASK (0xFFFFFFFF) 

static inline uint32_t lcd_disp_ll_get_i8080_dat_fifo_value(void)
{
    return REG_READ(LCD_DISP_I8080_DAT_FIFO_ADDR);
}

static inline void lcd_disp_ll_set_i8080_dat_fifo_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_I8080_DAT_FIFO_ADDR,value);
}

static inline uint32_t lcd_disp_ll_get_i8080_dat_fifo_i8080_dat_fifo(void)
{
    return REG_READ(LCD_DISP_I8080_DAT_FIFO_ADDR);
}

static inline void lcd_disp_ll_set_i8080_dat_fifo_i8080_dat_fifo(uint32_t value)
{
    REG_WRITE(LCD_DISP_I8080_DAT_FIFO_ADDR,value);
}

/* REG_0x0b */
#define LCD_DISP_I8080_THRD_ADDR  (LCD_DISP_LL_REG_BASE  + 0xB*4) //REG ADDR :0x4806002c
#define LCD_DISP_I8080_THRD_RESERVED0_POS (0) 
#define LCD_DISP_I8080_THRD_RESERVED0_MASK (0xFF) 

#define LCD_DISP_I8080_THRD_CMD_WR_THRD_POS (8) 
#define LCD_DISP_I8080_THRD_CMD_WR_THRD_MASK (0xFF) 

#define LCD_DISP_I8080_THRD_RESERVED1_POS (16) 
#define LCD_DISP_I8080_THRD_RESERVED1_MASK (0xFF) 

#define LCD_DISP_I8080_THRD_CMD_RD_THRD_POS (24)
#define LCD_DISP_I8080_THRD_CMD_RD_THRD_MASK (0xFF)

static inline uint32_t lcd_disp_ll_get_i8080_thrd_value(void)
{
    return REG_READ(LCD_DISP_I8080_THRD_ADDR);
}

static inline void lcd_disp_ll_set_i8080_thrd_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_I8080_THRD_ADDR,value);
}

static inline uint32_t lcd_disp_ll_get_i8080_thrd_cmd_wr_thrd(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_THRD_ADDR);
    reg_value = ((reg_value >> LCD_DISP_I8080_THRD_CMD_WR_THRD_POS) & LCD_DISP_I8080_THRD_CMD_WR_THRD_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_i8080_thrd_cmd_wr_thrd(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_THRD_ADDR);
    reg_value &= ~(LCD_DISP_I8080_THRD_CMD_WR_THRD_MASK << LCD_DISP_I8080_THRD_CMD_WR_THRD_POS);
    reg_value |= ((value & LCD_DISP_I8080_THRD_CMD_WR_THRD_MASK) << LCD_DISP_I8080_THRD_CMD_WR_THRD_POS);
    REG_WRITE(LCD_DISP_I8080_THRD_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_i8080_thrd_cmd_rd_thrd(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_THRD_ADDR);
    reg_value = ((reg_value >> LCD_DISP_I8080_THRD_CMD_RD_THRD_POS) & LCD_DISP_I8080_THRD_CMD_RD_THRD_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_i8080_thrd_cmd_rd_thrd(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_I8080_THRD_ADDR);
    reg_value &= ~(LCD_DISP_I8080_THRD_CMD_RD_THRD_MASK << LCD_DISP_I8080_THRD_CMD_RD_THRD_POS);
    reg_value |= ((value & LCD_DISP_I8080_THRD_CMD_RD_THRD_MASK) << LCD_DISP_I8080_THRD_CMD_RD_THRD_POS);
    REG_WRITE(LCD_DISP_I8080_THRD_ADDR,reg_value);
}

/* REG_0x0C */
#define LCD_DISP_DISP_STATUS_ADDR  (LCD_DISP_LL_REG_BASE  + 0xC*4) //REG ADDR :0x48060030
#define LCD_DISP_DISP_STATUS_RGB_VER_CNT_POS (0)
#define LCD_DISP_DISP_STATUS_RGB_VER_CNT_MASK (0x7FF)

#define LCD_DISP_DISP_STATUS_I8080_CMD_CFG_DONE_POS (11)
#define LCD_DISP_DISP_STATUS_I8080_CMD_CFG_DONE_MASK (0x1)

#define LCD_DISP_DISP_STATUS_I8080_VER_CNT_POS (12)
#define LCD_DISP_DISP_STATUS_I8080_VER_CNT_MASK (0x7FF)

#define LCD_DISP_DISP_STATUS_DISP_FIFO_EMPTY_POS (23)
#define LCD_DISP_DISP_STATUS_DISP_FIFO_EMPTY_MASK (0x1)

#define LCD_DISP_DISP_STATUS_DISP_FIFO_NEAR_FULL_POS (24)
#define LCD_DISP_DISP_STATUS_DISP_FIFO_NEAR_FULL_MASK (0x1)

#define LCD_DISP_DISP_STATUS_RESERVED_POS (25)
#define LCD_DISP_DISP_STATUS_RESERVED_MASK (0x7F)

static inline uint32_t lcd_disp_ll_get_disp_status_value(void)
{
    return REG_READ(LCD_DISP_DISP_STATUS_ADDR);
}

static inline uint32_t lcd_disp_ll_get_disp_status_rgb_ver_cnt(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISP_STATUS_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISP_STATUS_RGB_VER_CNT_POS)&LCD_DISP_DISP_STATUS_RGB_VER_CNT_MASK);
    return reg_value;
}

static inline uint32_t lcd_disp_ll_get_disp_status_i8080_cmd_cfg_done(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISP_STATUS_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISP_STATUS_I8080_CMD_CFG_DONE_POS)&LCD_DISP_DISP_STATUS_I8080_CMD_CFG_DONE_MASK);
    return reg_value;
}

static inline uint32_t lcd_disp_ll_get_disp_status_i8080_ver_cnt(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISP_STATUS_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISP_STATUS_I8080_VER_CNT_POS)&LCD_DISP_DISP_STATUS_I8080_VER_CNT_MASK);
    return reg_value;
}

static inline uint32_t lcd_disp_ll_get_disp_status_disp_fifo_empty(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISP_STATUS_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISP_STATUS_DISP_FIFO_EMPTY_POS)&LCD_DISP_DISP_STATUS_DISP_FIFO_EMPTY_MASK);
    return reg_value;
}

static inline uint32_t lcd_disp_ll_get_disp_status_disp_fifo_near_full(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISP_STATUS_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISP_STATUS_DISP_FIFO_NEAR_FULL_POS)&LCD_DISP_DISP_STATUS_DISP_FIFO_NEAR_FULL_MASK);
    return reg_value;
}

/* REG_0x0d */
#define LCD_DISP_RGB_SYNC_LOW_ADDR  (LCD_DISP_LL_REG_BASE  + 0xD*4) //REG ADDR :0x48060034
#define LCD_DISP_RGB_SYNC_LOW_HSYNC_BACK_LOW_POS (0) 
#define LCD_DISP_RGB_SYNC_LOW_HSYNC_BACK_LOW_MASK (0x3F) 

#define LCD_DISP_RGB_SYNC_LOW_RESERVED0_POS (6) 
#define LCD_DISP_RGB_SYNC_LOW_RESERVED0_MASK (0x3) 

#define LCD_DISP_RGB_SYNC_LOW_VSYNC_BACK_LOW_POS (8)
#define LCD_DISP_RGB_SYNC_LOW_VSYNC_BACK_LOW_MASK (0x3F)

#define LCD_DISP_RGB_SYNC_LOW_RESERVED1_POS (14)
#define LCD_DISP_RGB_SYNC_LOW_RESERVED1_MASK (0x3)

#define LCD_DISP_RGB_SYNC_LOW_PFC_PIXEL_REVE_POS (16)
#define LCD_DISP_RGB_SYNC_LOW_PFC_PIXEL_REVE_MASK (0x1)

#define LCD_DISP_RGB_SYNC_LOW_DISP_ON_SEL_POS (17) 
#define LCD_DISP_RGB_SYNC_LOW_DISP_ON_SEL_MASK (0x1) 

#define LCD_DISP_RGB_SYNC_LOW_DISP_ON_ENA_POS (18) 
#define LCD_DISP_RGB_SYNC_LOW_DISP_ON_ENA_MASK (0x1) 

#define LCD_DISP_RGB_SYNC_LOW_RESERVED2_POS (19) 
#define LCD_DISP_RGB_SYNC_LOW_RESERVED2_MASK (0x1FFF) 

static inline uint32_t lcd_disp_ll_get_rgb_sync_low_value(void)
{
    return REG_READ(LCD_DISP_RGB_SYNC_LOW_ADDR);
}

static inline void lcd_disp_ll_set_rgb_sync_low_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_RGB_SYNC_LOW_ADDR,value);
}

static inline uint32_t lcd_disp_ll_get_rgb_sync_low_hsync_back_low(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_SYNC_LOW_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_SYNC_LOW_HSYNC_BACK_LOW_POS) & LCD_DISP_RGB_SYNC_LOW_HSYNC_BACK_LOW_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_sync_low_hsync_back_low(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_SYNC_LOW_ADDR);
    reg_value &= ~(LCD_DISP_RGB_SYNC_LOW_HSYNC_BACK_LOW_MASK << LCD_DISP_RGB_SYNC_LOW_HSYNC_BACK_LOW_POS);
    reg_value |= ((value & LCD_DISP_RGB_SYNC_LOW_HSYNC_BACK_LOW_MASK) << LCD_DISP_RGB_SYNC_LOW_HSYNC_BACK_LOW_POS);
    REG_WRITE(LCD_DISP_RGB_SYNC_LOW_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_rgb_sync_low_vsync_back_low(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_SYNC_LOW_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_SYNC_LOW_VSYNC_BACK_LOW_POS) & LCD_DISP_RGB_SYNC_LOW_VSYNC_BACK_LOW_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_sync_low_vsync_back_low(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_SYNC_LOW_ADDR);
    reg_value &= ~(LCD_DISP_RGB_SYNC_LOW_VSYNC_BACK_LOW_MASK << LCD_DISP_RGB_SYNC_LOW_VSYNC_BACK_LOW_POS);
    reg_value |= ((value & LCD_DISP_RGB_SYNC_LOW_VSYNC_BACK_LOW_MASK) << LCD_DISP_RGB_SYNC_LOW_VSYNC_BACK_LOW_POS);
    REG_WRITE(LCD_DISP_RGB_SYNC_LOW_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_rgb_sync_low_pfc_pixel_reve(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_SYNC_LOW_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_SYNC_LOW_PFC_PIXEL_REVE_POS) & LCD_DISP_RGB_SYNC_LOW_PFC_PIXEL_REVE_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_sync_low_pfc_pixel_reve(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_SYNC_LOW_ADDR);
    reg_value &= ~(LCD_DISP_RGB_SYNC_LOW_PFC_PIXEL_REVE_MASK << LCD_DISP_RGB_SYNC_LOW_PFC_PIXEL_REVE_POS);
    reg_value |= ((value & LCD_DISP_RGB_SYNC_LOW_PFC_PIXEL_REVE_MASK) << LCD_DISP_RGB_SYNC_LOW_PFC_PIXEL_REVE_POS);
    REG_WRITE(LCD_DISP_RGB_SYNC_LOW_ADDR,reg_value);
}

/* REG_0x0d:rgb_sync_low->disp_on_sel:0xd[17],disp_on signal contrl way selected.0: module contrl, 1: sft contrl.,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_rgb_sync_low_disp_on_sel(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_SYNC_LOW_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_SYNC_LOW_DISP_ON_SEL_POS) & LCD_DISP_RGB_SYNC_LOW_DISP_ON_SEL_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_sync_low_disp_on_sel(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_SYNC_LOW_ADDR);
    reg_value &= ~(LCD_DISP_RGB_SYNC_LOW_DISP_ON_SEL_MASK << LCD_DISP_RGB_SYNC_LOW_DISP_ON_SEL_POS);
    reg_value |= ((value & LCD_DISP_RGB_SYNC_LOW_DISP_ON_SEL_MASK) << LCD_DISP_RGB_SYNC_LOW_DISP_ON_SEL_POS);
    REG_WRITE(LCD_DISP_RGB_SYNC_LOW_ADDR,reg_value);
}

/* REG_0x0d:rgb_sync_low->disp_on_ena:0xd[18],open disp on.,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_rgb_sync_low_disp_on_ena(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_SYNC_LOW_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_SYNC_LOW_DISP_ON_ENA_POS) & LCD_DISP_RGB_SYNC_LOW_DISP_ON_ENA_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_sync_low_disp_on_ena(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_SYNC_LOW_ADDR);
    reg_value &= ~(LCD_DISP_RGB_SYNC_LOW_DISP_ON_ENA_MASK << LCD_DISP_RGB_SYNC_LOW_DISP_ON_ENA_POS);
    reg_value |= ((value & LCD_DISP_RGB_SYNC_LOW_DISP_ON_ENA_MASK) << LCD_DISP_RGB_SYNC_LOW_DISP_ON_ENA_POS);
    REG_WRITE(LCD_DISP_RGB_SYNC_LOW_ADDR,reg_value);
}

/* REG_0x0E */
#define LCD_DISP_RGB_CLUM_OFFSET_ADDR  (LCD_DISP_LL_REG_BASE  + 0xE*4) //REG ADDR :0x48060038
#define LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_OFFSET_CLUM_L_POS (0) 
#define LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_OFFSET_CLUM_L_MASK (0x7FF) 

#define LCD_DISP_RGB_CLUM_OFFSET_RESERVED0_POS (11) 
#define LCD_DISP_RGB_CLUM_OFFSET_RESERVED0_MASK (0x1F) 

#define LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_OFFSET_CLUM_R_POS (16)
#define LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_OFFSET_CLUM_R_MASK (0x7FF)

#define LCD_DISP_RGB_CLUM_OFFSET_RESERVED1_POS (27)
#define LCD_DISP_RGB_CLUM_OFFSET_RESERVED1_MASK (0x1)

#define LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_AREA_ENA_POS (28)
#define LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_AREA_ENA_MASK (0x1)

#define LCD_DISP_RGB_CLUM_OFFSET_RESERVED2_POS (29) 
#define LCD_DISP_RGB_CLUM_OFFSET_RESERVED2_MASK (0x7) 

static inline uint32_t lcd_disp_ll_get_rgb_clum_offset_value(void)
{
    return REG_READ(LCD_DISP_RGB_CLUM_OFFSET_ADDR);
}

static inline void lcd_disp_ll_set_rgb_clum_offset_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_RGB_CLUM_OFFSET_ADDR,value);
}

static inline uint32_t lcd_disp_ll_get_rgb_clum_offset_partial_offset_clum_l(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CLUM_OFFSET_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_OFFSET_CLUM_L_POS) & LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_OFFSET_CLUM_L_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_clum_offset_partial_offset_clum_l(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CLUM_OFFSET_ADDR);
    reg_value &= ~(LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_OFFSET_CLUM_L_MASK << LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_OFFSET_CLUM_L_POS);
    reg_value |= ((value & LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_OFFSET_CLUM_L_MASK) << LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_OFFSET_CLUM_L_POS);
    REG_WRITE(LCD_DISP_RGB_CLUM_OFFSET_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_rgb_clum_offset_partial_offset_clum_r(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CLUM_OFFSET_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_OFFSET_CLUM_R_POS) & LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_OFFSET_CLUM_R_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_clum_offset_partial_offset_clum_r(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CLUM_OFFSET_ADDR);
    reg_value &= ~(LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_OFFSET_CLUM_R_MASK << LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_OFFSET_CLUM_R_POS);
    reg_value |= ((value & LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_OFFSET_CLUM_R_MASK) << LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_OFFSET_CLUM_R_POS);
    REG_WRITE(LCD_DISP_RGB_CLUM_OFFSET_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_rgb_clum_offset_partial_area_ena(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CLUM_OFFSET_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_AREA_ENA_POS) & LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_AREA_ENA_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_clum_offset_partial_area_ena(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_CLUM_OFFSET_ADDR);
    reg_value &= ~(LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_AREA_ENA_MASK << LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_AREA_ENA_POS);
    reg_value |= ((value & LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_AREA_ENA_MASK) << LCD_DISP_RGB_CLUM_OFFSET_PARTIAL_AREA_ENA_POS);
    REG_WRITE(LCD_DISP_RGB_CLUM_OFFSET_ADDR,reg_value);
}

/* REG_0x0F */
#define LCD_DISP_RGB_LINE_OFFSET_ADDR  (LCD_DISP_LL_REG_BASE  + 0xF*4) //REG ADDR :0x4806003c
#define LCD_DISP_RGB_LINE_OFFSET_PARTIAL_OFFSET_CLUM_L_POS (0) 
#define LCD_DISP_RGB_LINE_OFFSET_PARTIAL_OFFSET_CLUM_L_MASK (0x7FF) 

#define LCD_DISP_RGB_LINE_OFFSET_RESERVED0_POS (11) 
#define LCD_DISP_RGB_LINE_OFFSET_RESERVED0_MASK (0x1F) 

#define LCD_DISP_RGB_LINE_OFFSET_PARTIAL_OFFSET_CLUM_R_POS (16)
#define LCD_DISP_RGB_LINE_OFFSET_PARTIAL_OFFSET_CLUM_R_MASK (0x7FF)

#define LCD_DISP_RGB_LINE_OFFSET_RESERVED1_POS (27) 
#define LCD_DISP_RGB_LINE_OFFSET_RESERVED1_MASK (0x1F) 

static inline uint32_t lcd_disp_ll_get_rgb_line_offset_value(void)
{
    return REG_READ(LCD_DISP_RGB_LINE_OFFSET_ADDR);
}

static inline void lcd_disp_ll_set_rgb_line_offset_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_RGB_LINE_OFFSET_ADDR,value);
}

static inline uint32_t lcd_disp_ll_get_rgb_line_offset_partial_offset_clum_l(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_LINE_OFFSET_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_LINE_OFFSET_PARTIAL_OFFSET_CLUM_L_POS) & LCD_DISP_RGB_LINE_OFFSET_PARTIAL_OFFSET_CLUM_L_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_line_offset_partial_offset_clum_l(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_LINE_OFFSET_ADDR);
    reg_value &= ~(LCD_DISP_RGB_LINE_OFFSET_PARTIAL_OFFSET_CLUM_L_MASK << LCD_DISP_RGB_LINE_OFFSET_PARTIAL_OFFSET_CLUM_L_POS);
    reg_value |= ((value & LCD_DISP_RGB_LINE_OFFSET_PARTIAL_OFFSET_CLUM_L_MASK) << LCD_DISP_RGB_LINE_OFFSET_PARTIAL_OFFSET_CLUM_L_POS);
    REG_WRITE(LCD_DISP_RGB_LINE_OFFSET_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_rgb_line_offset_partial_offset_clum_r(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_LINE_OFFSET_ADDR);
    reg_value = ((reg_value >> LCD_DISP_RGB_LINE_OFFSET_PARTIAL_OFFSET_CLUM_R_POS) & LCD_DISP_RGB_LINE_OFFSET_PARTIAL_OFFSET_CLUM_R_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_rgb_line_offset_partial_offset_clum_r(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_RGB_LINE_OFFSET_ADDR);
    reg_value &= ~(LCD_DISP_RGB_LINE_OFFSET_PARTIAL_OFFSET_CLUM_R_MASK << LCD_DISP_RGB_LINE_OFFSET_PARTIAL_OFFSET_CLUM_R_POS);
    reg_value |= ((value & LCD_DISP_RGB_LINE_OFFSET_PARTIAL_OFFSET_CLUM_R_MASK) << LCD_DISP_RGB_LINE_OFFSET_PARTIAL_OFFSET_CLUM_R_POS);
    REG_WRITE(LCD_DISP_RGB_LINE_OFFSET_ADDR,reg_value);
}

/* REG_0x10 */
#define LCD_DISP_DAT_FIFO_THRD_ADDR  (LCD_DISP_LL_REG_BASE  + 0x10*4) //REG ADDR :0x48060040
#define LCD_DISP_DAT_FIFO_THRD_RESERVED0_POS (0) 
#define LCD_DISP_DAT_FIFO_THRD_RESERVED0_MASK (0xFF) 

#define LCD_DISP_DAT_FIFO_THRD_DAT_WR_THRD_POS (8)
#define LCD_DISP_DAT_FIFO_THRD_DAT_WR_THRD_MASK (0x3FF)

#define LCD_DISP_DAT_FIFO_THRD_RESERVED1_POS (18)
#define LCD_DISP_DAT_FIFO_THRD_RESERVED1_MASK (0x3)

#define LCD_DISP_DAT_FIFO_THRD_DAT_RD_THRD_POS (20)
#define LCD_DISP_DAT_FIFO_THRD_DAT_RD_THRD_MASK (0x3FF)

#define LCD_DISP_DAT_FIFO_THRD_RESERVED2_POS (30) 
#define LCD_DISP_DAT_FIFO_THRD_RESERVED2_MASK (0x3) 

static inline uint32_t lcd_disp_ll_get_dat_fifo_thrd_value(void)
{
    return REG_READ(LCD_DISP_DAT_FIFO_THRD_ADDR);
}

static inline void lcd_disp_ll_set_dat_fifo_thrd_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_DAT_FIFO_THRD_ADDR,value);
}

static inline uint32_t lcd_disp_ll_get_dat_fifo_thrd_dat_wr_thrd(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DAT_FIFO_THRD_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DAT_FIFO_THRD_DAT_WR_THRD_POS) & LCD_DISP_DAT_FIFO_THRD_DAT_WR_THRD_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_dat_fifo_thrd_dat_wr_thrd(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DAT_FIFO_THRD_ADDR);
    reg_value &= ~(LCD_DISP_DAT_FIFO_THRD_DAT_WR_THRD_MASK << LCD_DISP_DAT_FIFO_THRD_DAT_WR_THRD_POS);
    reg_value |= ((value & LCD_DISP_DAT_FIFO_THRD_DAT_WR_THRD_MASK) << LCD_DISP_DAT_FIFO_THRD_DAT_WR_THRD_POS);
    REG_WRITE(LCD_DISP_DAT_FIFO_THRD_ADDR,reg_value);
}

static inline uint32_t lcd_disp_ll_get_dat_fifo_thrd_dat_rd_thrd(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DAT_FIFO_THRD_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DAT_FIFO_THRD_DAT_RD_THRD_POS) & LCD_DISP_DAT_FIFO_THRD_DAT_RD_THRD_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_dat_fifo_thrd_dat_rd_thrd(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DAT_FIFO_THRD_ADDR);
    reg_value &= ~(LCD_DISP_DAT_FIFO_THRD_DAT_RD_THRD_MASK << LCD_DISP_DAT_FIFO_THRD_DAT_RD_THRD_POS);
    reg_value |= ((value & LCD_DISP_DAT_FIFO_THRD_DAT_RD_THRD_MASK) << LCD_DISP_DAT_FIFO_THRD_DAT_RD_THRD_POS);
    REG_WRITE(LCD_DISP_DAT_FIFO_THRD_ADDR,reg_value);
}

/* REG_0x11 */
#define LCD_DISP_MATER_RD_BASE_ADDR_ADDR  (LCD_DISP_LL_REG_BASE  + 0x11*4)
#define LCD_DISP_MATER_RD_BASE_ADDR_MASTER_RD_BASE_ADR_POS (0)
#define LCD_DISP_MATER_RD_BASE_ADDR_MASTER_RD_BASE_ADR_MASK (0xFFFFFFFF)


static inline uint32_t lcd_disp_ll_get_mater_rd_base_addr(void)
{
    return REG_READ(LCD_DISP_MATER_RD_BASE_ADDR_ADDR);
}

static inline void lcd_disp_ll_set_mater_rd_base_addr(uint32_t value)
{
    REG_WRITE(LCD_DISP_MATER_RD_BASE_ADDR_ADDR,value);
}


/* REG_0x12 */
#define LCD_DISP_DISP_STATUS1_ADDR  (LCD_DISP_LL_REG_BASE  + 0x12*4) //REG ADDR :0x48060048
#define LCD_DISP_DISP_STATUS1_DISP_FIFO_WRCNT_POS (0) 
#define LCD_DISP_DISP_STATUS1_DISP_FIFO_WRCNT_MASK (0x3FF) 

#define LCD_DISP_DISP_STATUS1_RESERVED0_POS (10) 
#define LCD_DISP_DISP_STATUS1_RESERVED0_MASK (0x3) 

#define LCD_DISP_DISP_STATUS1_DISP_FIFO_NEAREMPTY_POS (12) 
#define LCD_DISP_DISP_STATUS1_DISP_FIFO_NEAREMPTY_MASK (0x1) 

#define LCD_DISP_DISP_STATUS1_DISP_FIFO_EMPTY_POS (13) 
#define LCD_DISP_DISP_STATUS1_DISP_FIFO_EMPTY_MASK (0x1) 

#define LCD_DISP_DISP_STATUS1_RESERVED2_POS (14) 
#define LCD_DISP_DISP_STATUS1_RESERVED2_MASK (0x3) 

#define LCD_DISP_DISP_STATUS1_DISP_FIFO_RDCNT_POS (16) 
#define LCD_DISP_DISP_STATUS1_DISP_FIFO_RDCNT_MASK (0x3FF) 

#define LCD_DISP_DISP_STATUS1_RESERVED_POS (26) 
#define LCD_DISP_DISP_STATUS1_RESERVED_MASK (0x3F) 

static inline uint32_t lcd_disp_ll_get_disp_status1_value(void)
{
    return REG_READ(LCD_DISP_DISP_STATUS1_ADDR);
}

/* REG_0x12:disp_status1->disp_fifo_wrcnt:0x12[9:0],wr data count in rgb's data fifo/i8080 data fifo. ,0x0,r*/
static inline uint32_t lcd_disp_ll_get_disp_status1_disp_fifo_wrcnt(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISP_STATUS1_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISP_STATUS1_DISP_FIFO_WRCNT_POS)&LCD_DISP_DISP_STATUS1_DISP_FIFO_WRCNT_MASK);
    return reg_value;
}

/* REG_0x12:disp_status1->disp_fifo_near empty:0x12[12],fifo near empty in rgb's data fifo/i8080 data fifo.,0x0,r*/
static inline uint32_t lcd_disp_ll_get_disp_status1_disp_fifo_nearempty(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISP_STATUS1_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISP_STATUS1_DISP_FIFO_NEAREMPTY_POS)&LCD_DISP_DISP_STATUS1_DISP_FIFO_NEAREMPTY_MASK);
    return reg_value;
}

/* REG_0x12:disp_status1->disp_fifo_empty:0x12[13],fifo empty in rgb's data fifo/i8080 data fifo.,0x0,r*/
static inline uint32_t lcd_disp_ll_get_disp_status1_disp_fifo_empty(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISP_STATUS1_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISP_STATUS1_DISP_FIFO_EMPTY_POS)&LCD_DISP_DISP_STATUS1_DISP_FIFO_EMPTY_MASK);
    return reg_value;
}

/* REG_0x12:disp_status1->disp_fifo_rdcnt:0x12[25:16],rd data count in rgb's data fifo/i8080 data fifo. ,0x0,r*/
static inline uint32_t lcd_disp_ll_get_disp_status1_disp_fifo_rdcnt(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DISP_STATUS1_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DISP_STATUS1_DISP_FIFO_RDCNT_POS)&LCD_DISP_DISP_STATUS1_DISP_FIFO_RDCNT_MASK);
    return reg_value;
}
/* REG_0x13 */
#define LCD_DISP_CMD_COUNT_ADDR  (LCD_DISP_LL_REG_BASE  + 0x13*4)
#define LCD_DISP_CMD_COUNT_I8080_CMD_PARA_COUNT_POS (0)
#define LCD_DISP_CMD_COUNT_I8080_CMD_PARA_COUNT_MASK (0x3FFFFF)

static inline uint32_t lcd_disp_ll_get_cmd_count_value(void)
{
    return REG_READ(LCD_DISP_CMD_COUNT_ADDR);
}

static inline void lcd_disp_ll_set_cmd_count_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_CMD_COUNT_ADDR,value);
}

/* REG_0x13:cmd_count->i8080_cmd_para_count:0x13[21:0],i8080模块初始化期间，cmd指令后param的个数。,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_cmd_count_i8080_cmd_para_count(void)
{
	uint32_t reg_value;
	reg_value = REG_READ(LCD_DISP_CMD_COUNT_ADDR);
	reg_value = ((reg_value >> LCD_DISP_CMD_COUNT_I8080_CMD_PARA_COUNT_POS) & LCD_DISP_CMD_COUNT_I8080_CMD_PARA_COUNT_MASK);
	return reg_value;
}

static inline void lcd_disp_ll_set_cmd_count_i8080_cmd_para_count(uint32_t value)
{
	uint32_t reg_value;
	reg_value = REG_READ(LCD_DISP_CMD_COUNT_ADDR);
	reg_value &= ~(LCD_DISP_CMD_COUNT_I8080_CMD_PARA_COUNT_MASK << LCD_DISP_CMD_COUNT_I8080_CMD_PARA_COUNT_POS);
	reg_value |= ((value & LCD_DISP_CMD_COUNT_I8080_CMD_PARA_COUNT_MASK) << LCD_DISP_CMD_COUNT_I8080_CMD_PARA_COUNT_POS);
	REG_WRITE(LCD_DISP_CMD_COUNT_ADDR,reg_value);
}

/* REG_0x14 */
#define LCD_DISP_H_PORCH_ADDR  (LCD_DISP_LL_REG_BASE  + 0x14*4) //REG ADDR :0x48060050
#define LCD_DISP_H_PORCH_HSYNC_BACK_PORCH_POS (0) 
#define LCD_DISP_H_PORCH_HSYNC_BACK_PORCH_MASK (0x1FF) 

#define LCD_DISP_H_PORCH_RESERVED0_POS (9) 
#define LCD_DISP_H_PORCH_RESERVED0_MASK (0x7F) 

#define LCD_DISP_H_PORCH_HSYNC_FRONT_PORCH_POS (16) 
#define LCD_DISP_H_PORCH_HSYNC_FRONT_PORCH_MASK (0xFF) 

#define LCD_DISP_H_PORCH_RESERVED1_POS (24) 
#define LCD_DISP_H_PORCH_RESERVED1_MASK (0xFF) 

static inline uint32_t lcd_disp_ll_get_h_porch_value(void)
{
    return REG_READ(LCD_DISP_H_PORCH_ADDR);
}

static inline void lcd_disp_ll_set_h_porch_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_H_PORCH_ADDR,value);
}

/* REG_0x14:h_porch->hsync_back_porch:0x14[8:0],hsync¨¤-???¨¢load¨ª???¨ºy?Y¦Ì?mclk symbol¨ºy?¡ê,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_h_porch_hsync_back_porch(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_H_PORCH_ADDR);
    reg_value = ((reg_value >> LCD_DISP_H_PORCH_HSYNC_BACK_PORCH_POS) & LCD_DISP_H_PORCH_HSYNC_BACK_PORCH_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_h_porch_hsync_back_porch(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_H_PORCH_ADDR);
    reg_value &= ~(LCD_DISP_H_PORCH_HSYNC_BACK_PORCH_MASK << LCD_DISP_H_PORCH_HSYNC_BACK_PORCH_POS);
    reg_value |= ((value & LCD_DISP_H_PORCH_HSYNC_BACK_PORCH_MASK) << LCD_DISP_H_PORCH_HSYNC_BACK_PORCH_POS);
    REG_WRITE(LCD_DISP_H_PORCH_ADDR,reg_value);
}

/* REG_0x14:h_porch->hsync_front_porch:0x14[23:16],hsync¡ä¨®¨ºy?Y?¨¢¨º?¦Ì?hsyncD?o?¨¤-¦Ì¨ª¦Ì?mclk symbol¨ºy?¡ê,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_h_porch_hsync_front_porch(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_H_PORCH_ADDR);
    reg_value = ((reg_value >> LCD_DISP_H_PORCH_HSYNC_FRONT_PORCH_POS) & LCD_DISP_H_PORCH_HSYNC_FRONT_PORCH_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_h_porch_hsync_front_porch(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_H_PORCH_ADDR);
    reg_value &= ~(LCD_DISP_H_PORCH_HSYNC_FRONT_PORCH_MASK << LCD_DISP_H_PORCH_HSYNC_FRONT_PORCH_POS);
    reg_value |= ((value & LCD_DISP_H_PORCH_HSYNC_FRONT_PORCH_MASK) << LCD_DISP_H_PORCH_HSYNC_FRONT_PORCH_POS);
    REG_WRITE(LCD_DISP_H_PORCH_ADDR,reg_value);
}

/* REG_0x15 */
#define LCD_DISP_V_PORCH_ADDR  (LCD_DISP_LL_REG_BASE  + 0x15*4) //REG ADDR :0x48060054
#define LCD_DISP_V_PORCH_VSYNC_BACK_PORCH_POS (0) 
#define LCD_DISP_V_PORCH_VSYNC_BACK_PORCH_MASK (0xFF) 

#define LCD_DISP_V_PORCH_RESERVED0_POS (8) 
#define LCD_DISP_V_PORCH_RESERVED0_MASK (0xFF) 

#define LCD_DISP_V_PORCH_VSYNC_FRONT_PORCH_POS (16) 
#define LCD_DISP_V_PORCH_VSYNC_FRONT_PORCH_MASK (0xFF) 

#define LCD_DISP_V_PORCH_RESERVED1_POS (24) 
#define LCD_DISP_V_PORCH_RESERVED1_MASK (0xFF) 

static inline uint32_t lcd_disp_ll_get_v_porch_value(void)
{
    return REG_READ(LCD_DISP_V_PORCH_ADDR);
}

static inline void lcd_disp_ll_set_v_porch_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_V_PORCH_ADDR,value);
}

/* REG_0x15:v_porch->vsync_back_porch:0x15[7:0],vsync¨¤-???¨¢load hsync¨ª???¨ºy?Y¦Ì?hsync symbol¨ºy?¡ê,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_v_porch_vsync_back_porch(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_V_PORCH_ADDR);
    reg_value = ((reg_value >> LCD_DISP_V_PORCH_VSYNC_BACK_PORCH_POS) & LCD_DISP_V_PORCH_VSYNC_BACK_PORCH_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_v_porch_vsync_back_porch(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_V_PORCH_ADDR);
    reg_value &= ~(LCD_DISP_V_PORCH_VSYNC_BACK_PORCH_MASK << LCD_DISP_V_PORCH_VSYNC_BACK_PORCH_POS);
    reg_value |= ((value & LCD_DISP_V_PORCH_VSYNC_BACK_PORCH_MASK) << LCD_DISP_V_PORCH_VSYNC_BACK_PORCH_POS);
    REG_WRITE(LCD_DISP_V_PORCH_ADDR,reg_value);
}

/* REG_0x15:v_porch->vsync_front_porch:0x15[23:16],vsync¡ä¨®DD¨ºy?Y?¨¢¨º?¦Ì?vsyncD?o?¨¤-¦Ì¨ª¦Ì?hsync sym¨ºy?¡ê,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_v_porch_vsync_front_porch(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_V_PORCH_ADDR);
    reg_value = ((reg_value >> LCD_DISP_V_PORCH_VSYNC_FRONT_PORCH_POS) & LCD_DISP_V_PORCH_VSYNC_FRONT_PORCH_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_v_porch_vsync_front_porch(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_V_PORCH_ADDR);
    reg_value &= ~(LCD_DISP_V_PORCH_VSYNC_FRONT_PORCH_MASK << LCD_DISP_V_PORCH_VSYNC_FRONT_PORCH_POS);
    reg_value |= ((value & LCD_DISP_V_PORCH_VSYNC_FRONT_PORCH_MASK) << LCD_DISP_V_PORCH_VSYNC_FRONT_PORCH_POS);
    REG_WRITE(LCD_DISP_V_PORCH_ADDR,reg_value);
}

/* REG_0x16 */
#define LCD_DISP_LATENCY_ADDR  (LCD_DISP_LL_REG_BASE  + 0x16*4) //REG ADDR :0x48060058
#define LCD_DISP_LATENCY_LATENCY_ENA_POS (0) 
#define LCD_DISP_LATENCY_LATENCY_ENA_MASK (0x1) 

#define LCD_DISP_LATENCY_LATENCY_SEL_POS (1) 
#define LCD_DISP_LATENCY_LATENCY_SEL_MASK (0x3) 

#define LCD_DISP_LATENCY_LATENCY_COUNT_POS (3) 
#define LCD_DISP_LATENCY_LATENCY_COUNT_MASK (0xFFF) 

#define LCD_DISP_LATENCY_REVERSED0_POS (15) 
#define LCD_DISP_LATENCY_REVERSED0_MASK (0x1) 

#define LCD_DISP_LATENCY_TEAR_EFF_ENA_POS (16) 
#define LCD_DISP_LATENCY_TEAR_EFF_ENA_MASK (0x1) 

#define LCD_DISP_LATENCY_TEAR_EFF_POL_POS (17) 
#define LCD_DISP_LATENCY_TEAR_EFF_POL_MASK (0x1) 

#define LCD_DISP_LATENCY_REVERSED1_POS (18) 
#define LCD_DISP_LATENCY_REVERSED1_MASK (0x3FFF) 

static inline uint32_t lcd_disp_ll_get_latency_value(void)
{
    return REG_READ(LCD_DISP_LATENCY_ADDR);
}

static inline void lcd_disp_ll_set_latency_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_LATENCY_ADDR,value);
}

/* REG_0x16:latency->latency_ena:0x16[0],frame_delay enable,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_latency_latency_ena(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_LATENCY_ADDR);
    reg_value = ((reg_value >> LCD_DISP_LATENCY_LATENCY_ENA_POS) & LCD_DISP_LATENCY_LATENCY_ENA_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_latency_latency_ena(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_LATENCY_ADDR);
    reg_value &= ~(LCD_DISP_LATENCY_LATENCY_ENA_MASK << LCD_DISP_LATENCY_LATENCY_ENA_POS);
    reg_value |= ((value & LCD_DISP_LATENCY_LATENCY_ENA_MASK) << LCD_DISP_LATENCY_LATENCY_ENA_POS);
    REG_WRITE(LCD_DISP_LATENCY_ADDR,reg_value);
}

/* REG_0x16:latency->latency_sel:0x16[2:1],frame_delay delay unit select:00:in dclk unit;;                               01:in hsync uint;;                               10:in vsync uint(frame);;                               11:nc.,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_latency_latency_sel(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_LATENCY_ADDR);
    reg_value = ((reg_value >> LCD_DISP_LATENCY_LATENCY_SEL_POS) & LCD_DISP_LATENCY_LATENCY_SEL_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_latency_latency_sel(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_LATENCY_ADDR);
    reg_value &= ~(LCD_DISP_LATENCY_LATENCY_SEL_MASK << LCD_DISP_LATENCY_LATENCY_SEL_POS);
    reg_value |= ((value & LCD_DISP_LATENCY_LATENCY_SEL_MASK) << LCD_DISP_LATENCY_LATENCY_SEL_POS);
    REG_WRITE(LCD_DISP_LATENCY_ADDR,reg_value);
}

/* REG_0x16:latency->latency_count:0x16[14:3],frame delay arrive uint count, then make latency int high.;  ; Such as: latency count = 10, latency sel = hsync uint.; The latency_int being high while 10 hsync passed after a frame end.,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_latency_latency_count(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_LATENCY_ADDR);
    reg_value = ((reg_value >> LCD_DISP_LATENCY_LATENCY_COUNT_POS) & LCD_DISP_LATENCY_LATENCY_COUNT_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_latency_latency_count(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_LATENCY_ADDR);
    reg_value &= ~(LCD_DISP_LATENCY_LATENCY_COUNT_MASK << LCD_DISP_LATENCY_LATENCY_COUNT_POS);
    reg_value |= ((value & LCD_DISP_LATENCY_LATENCY_COUNT_MASK) << LCD_DISP_LATENCY_LATENCY_COUNT_POS);
    REG_WRITE(LCD_DISP_LATENCY_ADDR,reg_value);
}

/* REG_0x16:latency->reversed0:0x16[15], ,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_latency_reversed0(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_LATENCY_ADDR);
    reg_value = ((reg_value >> LCD_DISP_LATENCY_REVERSED0_POS) & LCD_DISP_LATENCY_REVERSED0_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_latency_reversed0(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_LATENCY_ADDR);
    reg_value &= ~(LCD_DISP_LATENCY_REVERSED0_MASK << LCD_DISP_LATENCY_REVERSED0_POS);
    reg_value |= ((value & LCD_DISP_LATENCY_REVERSED0_MASK) << LCD_DISP_LATENCY_REVERSED0_POS);
    REG_WRITE(LCD_DISP_LATENCY_ADDR,reg_value);
}

/* REG_0x16:latency->tear_eff_ena:0x16[16],tear_effect signal function enable.,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_latency_tear_eff_ena(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_LATENCY_ADDR);
    reg_value = ((reg_value >> LCD_DISP_LATENCY_TEAR_EFF_ENA_POS) & LCD_DISP_LATENCY_TEAR_EFF_ENA_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_latency_tear_eff_ena(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_LATENCY_ADDR);
    reg_value &= ~(LCD_DISP_LATENCY_TEAR_EFF_ENA_MASK << LCD_DISP_LATENCY_TEAR_EFF_ENA_POS);
    reg_value |= ((value & LCD_DISP_LATENCY_TEAR_EFF_ENA_MASK) << LCD_DISP_LATENCY_TEAR_EFF_ENA_POS);
    REG_WRITE(LCD_DISP_LATENCY_ADDR,reg_value);
}

/* REG_0x16:latency->tear_eff_pol:0x16[17],tear_effect signal polarity.0: high effect. 1: low effect.,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_latency_tear_eff_pol(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_LATENCY_ADDR);
    reg_value = ((reg_value >> LCD_DISP_LATENCY_TEAR_EFF_POL_POS) & LCD_DISP_LATENCY_TEAR_EFF_POL_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_latency_tear_eff_pol(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_LATENCY_ADDR);
    reg_value &= ~(LCD_DISP_LATENCY_TEAR_EFF_POL_MASK << LCD_DISP_LATENCY_TEAR_EFF_POL_POS);
    reg_value |= ((value & LCD_DISP_LATENCY_TEAR_EFF_POL_MASK) << LCD_DISP_LATENCY_TEAR_EFF_POL_POS);
    REG_WRITE(LCD_DISP_LATENCY_ADDR,reg_value);
}

/* REG_0x16:latency->reversed1:0x16[31:18],NC,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_latency_reversed1(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_LATENCY_ADDR);
    reg_value = ((reg_value >> LCD_DISP_LATENCY_REVERSED1_POS) & LCD_DISP_LATENCY_REVERSED1_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_latency_reversed1(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_LATENCY_ADDR);
    reg_value &= ~(LCD_DISP_LATENCY_REVERSED1_MASK << LCD_DISP_LATENCY_REVERSED1_POS);
    reg_value |= ((value & LCD_DISP_LATENCY_REVERSED1_MASK) << LCD_DISP_LATENCY_REVERSED1_POS);
    REG_WRITE(LCD_DISP_LATENCY_ADDR,reg_value);
}

/* REG_0x17 */
#define LCD_DISP_DE_WAIT_ADDR  (LCD_DISP_LL_REG_BASE  + 0x17*4) //REG ADDR :0x4806005c
#define LCD_DISP_DE_WAIT_HSYNC_WAIT_ENA_POS (0) 
#define LCD_DISP_DE_WAIT_HSYNC_WAIT_ENA_MASK (0x1) 

#define LCD_DISP_DE_WAIT_HSYNC_WAIT_THRD_POS (1) 
#define LCD_DISP_DE_WAIT_HSYNC_WAIT_THRD_MASK (0x3FF) 

#define LCD_DISP_DE_WAIT_REVERSED_POS (11) 
#define LCD_DISP_DE_WAIT_REVERSED_MASK (0x1FFFFF) 

static inline uint32_t lcd_disp_ll_get_de_wait_value(void)
{
    return REG_READ(LCD_DISP_DE_WAIT_ADDR);
}

static inline void lcd_disp_ll_set_de_wait_value(uint32_t value)
{
    REG_WRITE(LCD_DISP_DE_WAIT_ADDR,value);
}

/* REG_0x17:de_wait->hsync_wait_ena:0x17[0],hysnc wait untill data papare done enable.this function using to avoid DE sig not discontinued. If fifo thrd less than setting value, hysnc will never high.,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_de_wait_hsync_wait_ena(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DE_WAIT_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DE_WAIT_HSYNC_WAIT_ENA_POS) & LCD_DISP_DE_WAIT_HSYNC_WAIT_ENA_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_de_wait_hsync_wait_ena(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DE_WAIT_ADDR);
    reg_value &= ~(LCD_DISP_DE_WAIT_HSYNC_WAIT_ENA_MASK << LCD_DISP_DE_WAIT_HSYNC_WAIT_ENA_POS);
    reg_value |= ((value & LCD_DISP_DE_WAIT_HSYNC_WAIT_ENA_MASK) << LCD_DISP_DE_WAIT_HSYNC_WAIT_ENA_POS);
    REG_WRITE(LCD_DISP_DE_WAIT_ADDR,reg_value);
}

/* REG_0x17:de_wait->hsync_wait_thrd:0x17[10:1],the fifo data need more than hysnc_wait_thrd, then will next line will display.,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_de_wait_hsync_wait_thrd(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DE_WAIT_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DE_WAIT_HSYNC_WAIT_THRD_POS) & LCD_DISP_DE_WAIT_HSYNC_WAIT_THRD_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_de_wait_hsync_wait_thrd(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DE_WAIT_ADDR);
    reg_value &= ~(LCD_DISP_DE_WAIT_HSYNC_WAIT_THRD_MASK << LCD_DISP_DE_WAIT_HSYNC_WAIT_THRD_POS);
    reg_value |= ((value & LCD_DISP_DE_WAIT_HSYNC_WAIT_THRD_MASK) << LCD_DISP_DE_WAIT_HSYNC_WAIT_THRD_POS);
    REG_WRITE(LCD_DISP_DE_WAIT_ADDR,reg_value);
}

/* REG_0x17:de_wait->reversed:0x17[31:11],NC,0x0,r/w*/
static inline uint32_t lcd_disp_ll_get_de_wait_reversed(void)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DE_WAIT_ADDR);
    reg_value = ((reg_value >> LCD_DISP_DE_WAIT_REVERSED_POS) & LCD_DISP_DE_WAIT_REVERSED_MASK);
    return reg_value;
}

static inline void lcd_disp_ll_set_de_wait_reversed(uint32_t value)
{
    uint32_t reg_value;
    reg_value = REG_READ(LCD_DISP_DE_WAIT_ADDR);
    reg_value &= ~(LCD_DISP_DE_WAIT_REVERSED_MASK << LCD_DISP_DE_WAIT_REVERSED_POS);
    reg_value |= ((value & LCD_DISP_DE_WAIT_REVERSED_MASK) << LCD_DISP_DE_WAIT_REVERSED_POS);
    REG_WRITE(LCD_DISP_DE_WAIT_ADDR,reg_value);
}

#ifdef __cplusplus 
}                  
#endif             
