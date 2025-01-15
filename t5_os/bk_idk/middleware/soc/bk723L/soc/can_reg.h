// Copyright 2022-2023 Beken
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

// This is a generated file, if you need to modify it, use the script to
// generate and modify all the struct.h, ll.h, reg.h, debug_dump.c files!

#pragma once


#ifdef __cplusplus
extern "C" {
#endif


#define CAN_RID_ESI_ADDR (SOC_CAN_REG_BASE + (0x0 << 2))

#define CAN_RID_ESI_ID_POS (0)
#define CAN_RID_ESI_ID_MASK (0x1fffffff)

#define CAN_RID_ESI_RESERVED_29_30_POS (29)
#define CAN_RID_ESI_RESERVED_29_30_MASK (0x3)

#define CAN_RID_ESI_ESI_POS (31)
#define CAN_RID_ESI_ESI_MASK (0x1)

#define CAN_RBUF_CTRL_ADDR (SOC_CAN_REG_BASE + (0x1 << 2))

#define CAN_RBUF_CTRL_DLC_POS (0)
#define CAN_RBUF_CTRL_DLC_MASK (0xf)

#define CAN_RBUF_CTRL_BRS_POS (4)
#define CAN_RBUF_CTRL_BRS_MASK (0x1)

#define CAN_RBUF_CTRL_FDF_POS (5)
#define CAN_RBUF_CTRL_FDF_MASK (0x1)

#define CAN_RBUF_CTRL_RTR_POS (6)
#define CAN_RBUF_CTRL_RTR_MASK (0x1)

#define CAN_RBUF_CTRL_IDE_POS (7)
#define CAN_RBUF_CTRL_IDE_MASK (0x1)

#define CAN_RBUF_CTRL_RESERVED_8_31_POS (8)
#define CAN_RBUF_CTRL_RESERVED_8_31_MASK (0xffffff)

#define CAN_RDATA0_3_ADDR (SOC_CAN_REG_BASE + (0x2 << 2))
#define CAN_RDATA4_7_ADDR (SOC_CAN_REG_BASE + (0x3 << 2))
#define CAN_RDATA8_11_ADDR (SOC_CAN_REG_BASE + (0x4 << 2))
#define CAN_RDATA12_15_ADDR (SOC_CAN_REG_BASE + (0x5 << 2))
#define CAN_RDATA16_19_ADDR (SOC_CAN_REG_BASE + (0x6 << 2))
#define CAN_RDATA20_23_ADDR (SOC_CAN_REG_BASE + (0x7 << 2))
#define CAN_RDATA24_27_ADDR (SOC_CAN_REG_BASE + (0x8 << 2))
#define CAN_RDATA28_31_ADDR (SOC_CAN_REG_BASE + (0x9 << 2))
#define CAN_RDATA32_25_ADDR (SOC_CAN_REG_BASE + (0xa << 2))
#define CAN_RDATA36_39_ADDR (SOC_CAN_REG_BASE + (0xb << 2))
#define CAN_RDATA40_43_ADDR (SOC_CAN_REG_BASE + (0xc << 2))
#define CAN_RDATA44_47_ADDR (SOC_CAN_REG_BASE + (0xd << 2))
#define CAN_RDATA48_51_ADDR (SOC_CAN_REG_BASE + (0xe << 2))
#define CAN_RDATA52_55_ADDR (SOC_CAN_REG_BASE + (0xf << 2))
#define CAN_RDATA56_59_ADDR (SOC_CAN_REG_BASE + (0x10 << 2))
#define CAN_RDATA60_63_ADDR (SOC_CAN_REG_BASE + (0x11 << 2))

#define CAN_RDATA_POS (0)
#define CAN_RDATA_MASK (0xffffffff)

#define CAN_TID_ESI_ADDR (SOC_CAN_REG_BASE + (0x14 << 2))

#define CAN_TID_ESI_ID_POS (0)
#define CAN_TID_ESI_ID_MASK (0x1fffffff)

#define CAN_TID_ESI_RESERVED_29_30_POS (29)
#define CAN_TID_ESI_RESERVED_29_30_MASK (0x3)

#define CAN_TID_ESI_POS (31)
#define CAN_TID_ESI_MASK (0x1)

#define CAN_TBUF_CTRL_ADDR (SOC_CAN_REG_BASE + (0x15 << 2))

#define CAN_TBUF_CTRL_DLC_POS (0)
#define CAN_TBUF_CTRL_DLC_MASK (0xf)

#define CAN_TBUF_CTRL_BRS_POS (4)
#define CAN_TBUF_CTRL_BRS_MASK (0x1)

#define CAN_TBUF_CTRL_FDF_POS (5)
#define CAN_TBUF_CTRL_FDF_MASK (0x1)

#define CAN_TBUF_CTRL_RTR_POS (6)
#define CAN_TBUF_CTRL_RTR_MASK (0x1)

#define CAN_TBUF_CTRL_IDE_POS (7)
#define CAN_TBUF_CTRL_IDE_MASK (0x1)

#define CAN_TBUF_CTRL_RESERVED_8_31_POS (8)
#define CAN_TBUF_CTRL_RESERVED_8_31_MASK (0xffffff)

#define CAN_TDATA0_3_ADDR (SOC_CAN_REG_BASE + (0x16 << 2))
#define CAN_TDATA4_7_ADDR (SOC_CAN_REG_BASE + (0x17 << 2))
#define CAN_TDATA8_11_ADDR (SOC_CAN_REG_BASE + (0x18 << 2))
#define CAN_TDATA12_15_ADDR (SOC_CAN_REG_BASE + (0x19 << 2))
#define CAN_TDATA16_19_ADDR (SOC_CAN_REG_BASE + (0x1a << 2))
#define CAN_TDATA20_23_ADDR (SOC_CAN_REG_BASE + (0x1b << 2))
#define CAN_TDATA24_27_ADDR (SOC_CAN_REG_BASE + (0x1c << 2))
#define CAN_TDATA28_31_ADDR (SOC_CAN_REG_BASE + (0x1d << 2))
#define CAN_TDATA32_25_ADDR (SOC_CAN_REG_BASE + (0x1e << 2))
#define CAN_TDATA36_39_ADDR (SOC_CAN_REG_BASE + (0x1f << 2))
#define CAN_TDATA40_43_ADDR (SOC_CAN_REG_BASE + (0x20 << 2))
#define CAN_TDATA44_47_ADDR (SOC_CAN_REG_BASE + (0x21 << 2))
#define CAN_TDATA48_51_ADDR (SOC_CAN_REG_BASE + (0x22 << 2))
#define CAN_TDATA52_55_ADDR (SOC_CAN_REG_BASE + (0x23 << 2))
#define CAN_TDATA56_59_ADDR (SOC_CAN_REG_BASE + (0x24 << 2))
#define CAN_TDATA60_63_ADDR (SOC_CAN_REG_BASE + (0x25 << 2))

#define CAN_TDATA_POS (0)
#define CAN_TDATA_MASK (0xffffffff)

#define CAN_TTS_L_ADDR (SOC_CAN_REG_BASE + (0x26 << 2))

#define CAN_TTS_L_POS (0)
#define CAN_TTS_L_MASK (0xffffffff)

#define CAN_TTS_H_ADDR (SOC_CAN_REG_BASE + (0x27 << 2))

#define CAN_TTS_H_POS (0)
#define CAN_TTS_H_MASK (0xffffffff)

#define CAN_CFG_ADDR (SOC_CAN_REG_BASE + (0x28 << 2))

#define CAN_CFG_BUSOFF_POS (0)
#define CAN_CFG_BUSOFF_MASK (0x1)

#define CAN_CFG_TACTIVE_POS (1)
#define CAN_CFG_TACTIVE_MASK (0x1)

#define CAN_CFG_RACTIVE_POS (2)
#define CAN_CFG_RACTIVE_MASK (0x1)

#define CAN_CFG_TSSS_POS (3)
#define CAN_CFG_TSSS_MASK (0x1)

#define CAN_CFG_TPSS_POS (4)
#define CAN_CFG_TPSS_MASK (0x1)

#define CAN_CFG_LBMI_POS (5)
#define CAN_CFG_LBMI_MASK (0x1)

#define CAN_CFG_LBME_POS (6)
#define CAN_CFG_LBME_MASK (0x1)

#define CAN_CFG_RESET_POS (7)
#define CAN_CFG_RESET_MASK (0x1)

#define CAN_CFG_TSA_POS (8)
#define CAN_CFG_TSA_MASK (0x1)

#define CAN_CFG_TSALL_POS (9)
#define CAN_CFG_TSALL_MASK (0x1)

#define CAN_CFG_TSONE_POS (10)
#define CAN_CFG_TSONE_MASK (0x1)

#define CAN_CFG_TPA_POS (11)
#define CAN_CFG_TPA_MASK (0x1)

#define CAN_CFG_TPE_POS (12)
#define CAN_CFG_TPE_MASK (0x1)

#define CAN_CFG_STBY_POS (13)
#define CAN_CFG_STBY_MASK (0x1)

#define CAN_CFG_LOM_POS (14)
#define CAN_CFG_LOM_MASK (0x1)

#define CAN_CFG_TBSEL_POS (15)
#define CAN_CFG_TBSEL_MASK (0x1)

#define CAN_CFG_TSSTAT_POS (16)
#define CAN_CFG_TSSTAT_MASK (0x3)

#define CAN_CFG_RESERVED_18_19_POS (18)
#define CAN_CFG_RESERVED_18_19_MASK (0x3)

#define CAN_CFG_TTTBM_POS (20)
#define CAN_CFG_TTTBM_MASK (0x1)

#define CAN_CFG_TSMODE_POS (21)
#define CAN_CFG_TSMODE_MASK (0x1)

#define CAN_CFG_TSNEXT_POS (22)
#define CAN_CFG_TSNEXT_MASK (0x1)

#define CAN_CFG_FD_ISO_POS (23)
#define CAN_CFG_FD_ISO_MASK (0x1)

#define CAN_CFG_RSTAT_POS (24)
#define CAN_CFG_RSTAT_MASK (0x3)

#define CAN_CFG_RESERVED_26_26_POS (26)
#define CAN_CFG_RESERVED_26_26_MASK (0x1)

#define CAN_CFG_RBALL_POS (27)
#define CAN_CFG_RBALL_MASK (0x1)

#define CAN_CFG_RREL_POS (28)
#define CAN_CFG_RREL_MASK (0x1)

#define CAN_CFG_ROV_POS (29)
#define CAN_CFG_ROV_MASK (0x1)

#define CAN_CFG_ROM_POS (30)
#define CAN_CFG_ROM_MASK (0x1)

#define CAN_CFG_SACK_POS (31)
#define CAN_CFG_SACK_MASK (0x1)

#define CAN_IE_ADDR (SOC_CAN_REG_BASE + (0x29 << 2))

#define CAN_IE_TSFF_POS (0)
#define CAN_IE_TSFF_MASK (0x1)

#define CAN_IE_EIE_POS (1)
#define CAN_IE_EIE_MASK (0x1)

#define CAN_IE_TSIE_POS (2)
#define CAN_IE_TSIE_MASK (0x1)

#define CAN_IE_TPIE_POS (3)
#define CAN_IE_TPIE_MASK (0x1)

#define CAN_IE_RAFIE_POS (4)
#define CAN_IE_RAFIE_MASK (0x1)

#define CAN_IE_RFIE_POS (5)
#define CAN_IE_RFIE_MASK (0x1)

#define CAN_IE_ROIE_POS (6)
#define CAN_IE_ROIE_MASK (0x1)

#define CAN_IE_RIE_POS (7)
#define CAN_IE_RIE_MASK (0x1)

#define CAN_IE_AIF_POS (8)
#define CAN_IE_AIF_MASK (0x1)

#define CAN_IE_EIF_POS (9)
#define CAN_IE_EIF_MASK (0x1)

#define CAN_IE_TSIF_POS (10)
#define CAN_IE_TSIF_MASK (0x1)

#define CAN_IE_TPIF_POS (11)
#define CAN_IE_TPIF_MASK (0x1)

#define CAN_IE_RAFIF_POS (12)
#define CAN_IE_RAFIF_MASK (0x1)

#define CAN_IE_RFIF_POS (13)
#define CAN_IE_RFIF_MASK (0x1)

#define CAN_IE_ROIF_POS (14)
#define CAN_IE_ROIF_MASK (0x1)

#define CAN_IE_RIF_POS (15)
#define CAN_IE_RIF_MASK (0x1)

#define CAN_IE_BEIF_POS (16)
#define CAN_IE_BEIF_MASK (0x1)

#define CAN_IE_BEIE_POS (17)
#define CAN_IE_BEIE_MASK (0x1)

#define CAN_IE_ALIF_POS (18)
#define CAN_IE_ALIF_MASK (0x1)

#define CAN_IE_ALIE_POS (19)
#define CAN_IE_ALIE_MASK (0x1)

#define CAN_IE_EPIF_POS (20)
#define CAN_IE_EPIF_MASK (0x1)

#define CAN_IE_EPIE_POS (21)
#define CAN_IE_EPIE_MASK (0x1)

#define CAN_IE_EPASS_POS (22)
#define CAN_IE_EPASS_MASK (0x1)

#define CAN_IE_EWARN_POS (23)
#define CAN_IE_EWARN_MASK (0x1)

#define CAN_IE_EWL_POS (24)
#define CAN_IE_EWL_MASK (0xf)

#define CAN_IE_AFWL_POS (28)
#define CAN_IE_AFWL_MASK (0xf)

#define CAN_SSEG_ADDR (SOC_CAN_REG_BASE + (0x2a << 2))

#define CAN_SSEG1_POS (0)
#define CAN_SSEG1_MASK (0xff)

#define CAN_SSEG2_POS (8)
#define CAN_SSEG2_MASK (0x7f)

#define CAN_SSEG_RESERVED_15_15_POS (15)
#define CAN_SSEG_RESERVED_15_15_MASK (0x1)

#define CAN_SSJW_POS (16)
#define CAN_SSJW_MASK (0x7f)

#define CAN_SSJW_RESERVED_23_23_POS (23)
#define CAN_SSJW_RESERVED_23_23_MASK (0x1)

#define CAN_SPRESC_POS (24)
#define CAN_SPRESC_MASK (0xff)


#define CAN_FSEG_ADDR (SOC_CAN_REG_BASE + (0x2b << 2))

#define CAN_FSEG1_POS (0)
#define CAN_FSEG1_MASK (0x1f)

#define CAN_FSEG_RESERVED_5_7_POS (5)
#define CAN_FSEG_RESERVED_5_7_MASK (0x7)

#define CAN_FSEG2_POS (8)
#define CAN_FSEG2_MASK (0xf)

#define CAN_FSEG_RESERVED_12_15_POS (12)
#define CAN_FSEG_RESERVED_12_15_MASK (0xf)

#define CAN_FSJW_POS (16)
#define CAN_FSJW_MASK (0xf)

#define CAN_FSJW_RESERVED_20_23_POS (20)
#define CAN_FSJW_RESERVED_20_23_MASK (0xf)

#define CAN_FPRESC_POS (23)
#define CAN_FPRESC_MASK (0xff)

#define CAN_CAP_ADDR (SOC_CAN_REG_BASE + (0x2c << 2))

#define CAN_CAP_ALC_POS (0)
#define CAN_CAP_ALC_MASK (0x1f)

#define CAN_CAP_KOER_POS (5)
#define CAN_CAP_KOER_MASK (0x7)

#define CAN_CAP_SSPOFF_POS (8)
#define CAN_CAP_SSPOFF_MASK (0x7f)

#define CAN_CAP_TDCEN_POS (15)
#define CAN_CAP_TDCEN_MASK (0x1)

#define CAN_CAP_RECNT_POS (16)
#define CAN_CAP_RECNT_MASK (0xff)

#define CAN_CAP_TECNT_POS (24)
#define CAN_CAP_TECNT_MASK (0xff)

#define CAN_ACF_ADDR (SOC_CAN_REG_BASE + (0x2d << 2))

#define CAN_ACF_ACFADR_POS (0)
#define CAN_ACF_ACFADR_MASK (0xf)

#define CAN_ACF_RESERVED_4_4_POS (4)
#define CAN_ACF_RESERVED_4_4_MASK (0x1)

#define CAN_ACF_SELMASK_POS (5)
#define CAN_ACF_SELMASK_MASK (0x1)

#define CAN_ACF_RESERVED_6_7_POS (6)
#define CAN_ACF_RESERVED_6_7_MASK (0x3)

#define CAN_ACF_TIMEEN_POS (8)
#define CAN_ACF_TIMEEN_MASK (0x1)

#define CAN_ACF_TIMEPOS_POS (9)
#define CAN_ACF_TIMEPOS_MASK (0x1)

#define CAN_ACF_RESERVED_10_15_POS (10)
#define CAN_ACF_RESERVED_10_15_MASK (0x3f)

#define CAN_ACF_AE_X_POS (16)
#define CAN_ACF_AE_X_MASK (0xff)

#define CAN_ACF_AE_8_15_POS (24)
#define CAN_ACF_AE_8_15_MASK (0xff)

#define CAN_AID_ADDR (SOC_CAN_REG_BASE + (0x2e << 2))

#define CAN_AID_ACODE_OR_AMASK_POS (0)
#define CAN_AID_ACODE_OR_AMASK_MASK (0x1fffffff)

#define CAN_AIDE_POS (29)
#define CAN_AIDE_MASK (0x1)

#define CAN_AIDEE_POS (30)
#define CAN_AIDEE_MASK (0x1)

#define CAN_AID_RESERVED_BIT_31_31_POS (31)
#define CAN_AID_RESERVED_BIT_31_31_MASK (0x1)

#define CAN_TTCFG_ADDR (SOC_CAN_REG_BASE + (0x2f << 2))

#define CAN_TTCFG_VER_0_POS (0)
#define CAN_TTCFG_VER_0_MASK (0xff)

#define CAN_TTCFG_VER_1_POS (8)
#define CAN_TTCFG_VER_1_MASK (0xff)

#define CAN_TTCFG_TBPTR_POS (16)
#define CAN_TTCFG_TBPTR_MASK (0x3f)

#define CAN_TTCFG_TBF_POS (22)
#define CAN_TTCFG_TBF_MASK (0x1)

#define CAN_TTCFG_TBE_POS (23)
#define CAN_TTCFG_TBE_MASK (0x1)

#define CAN_TTCFG_TTEN_POS (24)
#define CAN_TTCFG_TTEN_MASK (0x1)

#define CAN_TTCFG_T_PRESC_POS (25)
#define CAN_TTCFG_T_PRESC_MASK (0x3)

#define CAN_TTCFG_TTIF_POS (27)
#define CAN_TTCFG_TTIF_MASK (0x1)

#define CAN_TTCFG_TTIE_POS (28)
#define CAN_TTCFG_TTIE_MASK (0x1)

#define CAN_TTCFG_TEIF_POS (29)
#define CAN_TTCFG_TEIF_MASK (0x1)

#define CAN_TTCFG_WTIF_POS (30)
#define CAN_TTCFG_WTIF_MASK (0x1)

#define CAN_TTCFG_WTIE_POS (31)
#define CAN_TTCFG_WTIE_MASK (0x1)

#define CAN_REF_MSG_ADDR (SOC_CAN_REG_BASE + (0x30 << 2))

#define CAN_REF_ID_POS (0)
#define CAN_REF_ID_MASK (0x1fffffff)

#define CAN_REF_RESERVED_BIT_29_30_POS (29)
#define CAN_REF_RESERVED_BIT_29_30_MASK (0x3)

#define CAN_REF_IDE_POS (31)
#define CAN_REF_IDE_MASK (0x1)

#define CAN_TRIG_CFG_ADDR (SOC_CAN_REG_BASE + (0x31 << 2))

#define CAN_TRIG_CFG_TTPTR_POS (0)
#define CAN_TRIG_CFG_TTPTR_MASK (0x3f)

#define CAN_TRIG_CFG_RESERVED_6_7_POS (6)
#define CAN_TRIG_CFG_RESERVED_6_7_MASK (0x3)

#define CAN_TRIG_TTYPE_POS (8)
#define CAN_TRIG_TTYPE_MASK (0x7)

#define CAN_TRIG_RESERVED_3_3_POS (11)
#define CAN_TRIG_RESERVED_3_3_MASK (0x1)

#define CAN_TRIG_TEW_POS (12)
#define CAN_TRIG_TEW_MASK (0xf)

#define CAN_TT_TRIG_POS (16)
#define CAN_TT_TRIG_MASK (0xffff)

#define CAN_MEM_STAT_ADDR (SOC_CAN_REG_BASE + (0x32 << 2))

#define CAN_MEM_TT_WTRIG_POS (0)
#define CAN_MEM_TT_WTRIG_MASK (0xffff)

#define CAN_MEM_MPEN_POS (16)
#define CAN_MEM_MPEN_MASK (0x1)

#define CAN_MEM_MDWIE_POS (17)
#define CAN_MEM_MDWIE_MASK (0x1)

#define CAN_MEM_MDWIF_POS (18)
#define CAN_MEM_MDWIF_MASK (0x1)

#define CAN_MEM_MDEIF_POS (19)
#define CAN_MEM_MDEIF_MASK (0x1)

#define CAN_MEM_MAEIF_POS (20)
#define CAN_MEM_MAEIF_MASK (0x1)

#define CAN_MEM_RESERVED_21_23_POS (21)
#define CAN_MEM_RESERVED_21_23_MASK (0x7)

#define CAN_MEM_ACFA_POS (024)
#define CAN_MEM_ACFA_MASK (0x1)

#define CAN_MEM_TXS_POS (25)
#define CAN_MEM_TXS_MASK (0x1)

#define CAN_MEM_TXB_POS (26)
#define CAN_MEM_TXB_MASK (0x1)

#define CAN_MEM_HELOC_POS (27)
#define CAN_MEM_HELOC_MASK (0x3)

#define CAN__RESERVED_29_31_POS (29)
#define CAN__RESERVED_29_31_MASK (0x7)

#define CAN_MEM_ES_ADDR (SOC_CAN_REG_BASE + (0x33 << 2))

#define CAN_MEM_ES_MEBP1_POS (0)
#define CAN_MEM_ES_MEBP1_MASK (0x3f)

#define CAN_MEM_ES_ME1EE_POS (6)
#define CAN_MEM_ES_ME1EE_MASK (0x1)

#define CAN_MEM_ES_MEAEE_POS (7)
#define CAN_MEM_ES_MEAEE_MASK (0x1)


#define CAN_MEM_ES_MEBP2_POS (8)
#define CAN_MEM_ES_MEBP2_MASK (0x3f)

#define CAN_MEM_ES_ME2EE_POS (14)
#define CAN_MEM_ES_ME2EE_MASK (0x1)

#define CAN_MEM_ES_RESERVED_15_15_POS (15)
#define CAN_MEM_ES_RESERVED_15_15_MASK (0x1)

#define CAN_MEM_ES_MEEEC_POS (16)
#define CAN_MEM_ES_MEEEC_MASK (0xf)

#define CAN_MEM_ES_MENEC_POS (20)
#define CAN_MEM_ES_MENEC_MASK (0xf)

#define CAN_MEM_ES_MEL_POS (24)
#define CAN_MEM_ES_MEL_MASK (0x3)

#define CAN_MEM_ES_MES_POS (26)
#define CAN_MEM_ES_MES_MASK (0x1)

#define CAN_MEM_ES_RESERVED_27_31_POS (27)
#define CAN_MEM_ES_RESERVED_27_31_MASK (0x1f)

#define CAN_SCFG_ADDR (SOC_CAN_REG_BASE + (0x34 << 2))

#define CAN_SCFG_XMREN_POS (0)
#define CAN_SCFG_XMREN_MASK (0x1)

#define CAN_SCFG_SEIF_POS (1)
#define CAN_SCFG_SEIF_MASK (0x1)

#define CAN_SCFG_SWIE_POS (2)
#define CAN_SCFG_SWIE_MASK (0x1)

#define CAN_SCFG_SWIF_POS (3)
#define CAN_SCFG_SWI_MASK (0x1)

#define CAN_SCFG_FSTIM_POS (4)
#define CAN_SCFG_FSTIM_MASK (0x7)

#define CAN_SCFG_RESERVED_7_31_POS (7)
#define CAN_SCFG_RESERVED_7_31_MASK (0x1ffffff)

#define CAN_FD_ADDR (SOC_CAN_REG_BASE + (0x200 << 2))

#define CAN_FD_ENABLE_POS (0)
#define CAN_FD_ENABLE_MASK (0x1)

#define CAN_RESERVED_1_31_POS (1)
#define CAN_RESERVED_1_31_MASK (0x7fffffff)

#ifdef __cplusplus
}
#endif
