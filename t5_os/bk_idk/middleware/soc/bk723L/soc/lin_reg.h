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


#define LIN_DEVICEID_ADDR (SOC_LIN_REG_BASE + (0x0 << 2))

#define LIN_DEVICEID_DEVICEID_POS (0)
#define LIN_DEVICEID_DEVICEID_MASK (0xffffffff)

#define LIN_VERSIONID_ADDR (SOC_LIN_REG_BASE + (0x1 << 2))

#define LIN_VERSIONID_VERSIONID_POS (0)
#define LIN_VERSIONID_VERSIONID_MASK (0xffffffff)

#define LIN_GLOBAL_CTRL_ADDR (SOC_LIN_REG_BASE + (0x2 << 2))

#define LIN_GLOBAL_CTRL_SOFT_RST_POS (0)
#define LIN_GLOBAL_CTRL_SOFT_RST_MASK (0x1)

#define LIN_GLOBAL_CTRL_BYPASS_CFG_POS (1)
#define LIN_GLOBAL_CTRL_BYPASS_CFG_MASK (0x1)

#define LIN_GLOBAL_CTRL_LIN_MASTER_POS (2)
#define LIN_GLOBAL_CTRL_LIN_MASTER_MASK (0x1)

#define LIN_GLOBAL_CTRL_RESERVED_BIT_3_31_POS (3)
#define LIN_GLOBAL_CTRL_RESERVED_BIT_3_31_MASK (0x1fffffff)

#define LIN_GLOBAL_STATUS_ADDR (SOC_LIN_REG_BASE + (0x3 << 2))

#define LIN_GLOBAL_STATUS_GLOBAL_STATUS_POS (0)
#define LIN_GLOBAL_STATUS_GLOBAL_STATUS_MASK (0xffffffff)

#define LIN_DATA0_ADDR (SOC_LIN_REG_BASE + (0x20 << 2))

#define LIN_DATA0_BYTE_POS (0)
#define LIN_DATA0_BYTE_MASK (0xff)

#define LIN_DATA0_RESERVED_BIT_8_31_POS (8)
#define LIN_DATA0_RESERVED_BIT_8_31_MASK (0xffffff)

#define LIN_DATA1_ADDR (SOC_LIN_REG_BASE + (0x21 << 2))

#define LIN_DATA1_BYTE_POS (0)
#define LIN_DATA1_BYTE_MASK (0xff)

#define LIN_DATA1_RESERVED_BIT_8_31_POS (8)
#define LIN_DATA1_RESERVED_BIT_8_31_MASK (0xffffff)

#define LIN_DATA2_ADDR (SOC_LIN_REG_BASE + (0x22 << 2))

#define LIN_DATA2_BYTE_POS (0)
#define LIN_DATA2_BYTE_MASK (0xff)

#define LIN_DATA2_RESERVED_BIT_8_31_POS (8)
#define LIN_DATA2_RESERVED_BIT_8_31_MASK (0xffffff)

#define LIN_DATA3_ADDR (SOC_LIN_REG_BASE + (0x23 << 2))

#define LIN_DATA3_BYTE_POS (0)
#define LIN_DATA3_BYTE_MASK (0xff)

#define LIN_DATA3_RESERVED_BIT_8_31_POS (8)
#define LIN_DATA3_RESERVED_BIT_8_31_MASK (0xffffff)

#define LIN_DATA4_ADDR (SOC_LIN_REG_BASE + (0x24 << 2))

#define LIN_DATA4_BYTE_POS (0)
#define LIN_DATA4_BYTE_MASK (0xff)

#define LIN_DATA4_RESERVED_BIT_8_31_POS (8)
#define LIN_DATA4_RESERVED_BIT_8_31_MASK (0xffffff)

#define LIN_DATA5_ADDR (SOC_LIN_REG_BASE + (0x25 << 2))

#define LIN_DATA5_BYTE_POS (0)
#define LIN_DATA5_BYTE_MASK (0xff)

#define LIN_DATA5_RESERVED_BIT_8_31_POS (8)
#define LIN_DATA5_RESERVED_BIT_8_31_MASK (0xffffff)

#define LIN_DATA6_ADDR (SOC_LIN_REG_BASE + (0x26 << 2))

#define LIN_DATA6_BYTE_POS (0)
#define LIN_DATA6_BYTE_MASK (0xff)

#define LIN_DATA6_RESERVED_BIT_8_31_POS (8)
#define LIN_DATA6_RESERVED_BIT_8_31_MASK (0xffffff)

#define LIN_DATA7_ADDR (SOC_LIN_REG_BASE + (0x27 << 2))

#define LIN_DATA7_BYTE_POS (0)
#define LIN_DATA7_BYTE_MASK (0xff)

#define LIN_DATA7_RESERVED_BIT_8_31_POS (8)
#define LIN_DATA7_RESERVED_BIT_8_31_MASK (0xffffff)

#define LIN_CTRL_ADDR (SOC_LIN_REG_BASE + (0x28 << 2))

#define LIN_CTRL_START_REQ_POS (0)
#define LIN_CTRL_START_REQ_MASK (0x1)

#define LIN_CTRL_WAKEUP_POS (1)
#define LIN_CTRL_WAKEUP_MASK (0x1)

#define LIN_CTRL_RESET_ERROR_POS (2)
#define LIN_CTRL_RESET_ERROR_MASK (0x1)

#define LIN_CTRL_RESET_INT_POS (3)
#define LIN_CTRL_RESET_INT_MASK (0x1)

#define LIN_CTRL_DATA_ACK_POS (4)
#define LIN_CTRL_DATA_ACK_MASK (0x1)

#define LIN_CTRL_TRANSMIT_POS (5)
#define LIN_CTRL_TRANSMIT_MASK (0x1)

#define LIN_CTRL_SLEEP_POS (6)
#define LIN_CTRL_SLEEP_MASK (0x1)

#define LIN_CTRL_STOP_POS (7)
#define LIN_CTRL_STOP_MASK (0x1)

#define LIN_CTRL_RESERVED_8_31_POS (8)
#define LIN_CTRL_RESERVED_8_31_MASK (0xffffff)

#define LIN_STATUS_ADDR (SOC_LIN_REG_BASE + (0x29 << 2))

#define LIN_STATUS_COMPLETE_POS (0)
#define LIN_STATUS_COMPLETE_MASK (0x1)

#define LIN_STATUS_WAKE_UP_POS (1)
#define LIN_STATUS_WAKE_UP_MASK (0x1)

#define LIN_STATUS_ERROR_POS (2)
#define LIN_STATUS_ERROR_MASK (0x1)

#define LIN_STATUS_INT_POS (3)
#define LIN_STATUS_INT_MASK (0x1)

#define LIN_STATUS_DATA_REQ_POS (4)
#define LIN_STATUS_DATA_REQ_MASK (0x1)

#define LIN_STATUS_ABORTED_POS (5)
#define LIN_STATUS_ABORTED_MASK (0x1)

#define LIN_STATUS_BUS_IDLE_TIMEOUT_POS (6)
#define LIN_STATUS_BUS_IDLE_TIMEOUT_MASK (0x1)

#define LIN_STATUS_LIN_ACTIVE_POS (7)
#define LIN_STATUS_LIN_ACTIVE_MASK (0x1)

#define LIN_STATUS_RESERVED_8_31_POS (8)
#define LIN_STATUS_RESERVED_8_31_MASK (0xffffff)

#define LIN_ERR_ADDR (SOC_LIN_REG_BASE + (0x2a << 2))

#define LIN_ERR_BIT_POS (0)
#define LIN_ERR_BIT_MASK (0x1)

#define LIN_ERR_CHK_POS (1)
#define LIN_ERR_CHK_MASK (0x1)

#define LIN_ERR_TIMEOUT_POS (2)
#define LIN_ERR_TIMEOUT_MASK (0x1)

#define LIN_ERR_PARITY_POS (3)
#define LIN_ERR_PARITY_MASK (0x1)

#define LIN_ERR_RESERVED_4_31_POS (4)
#define LIN_ERR_RESERVED_4_31_MASK (0xfffffff)

#define LIN_TYPE_ADDR (SOC_LIN_REG_BASE + (0x2b << 2))

#define LIN_TYPE_DATA_LENGTH_POS (0)
#define LIN_TYPE_DATA_LENGTH_MASK (0xf)

#define LIN_TYPE_RESERVED_4_6_POS (4)
#define LIN_TYPE_RESERVED_4_6_MASK (0x7)

#define LIN_TYPE_ENH_CHECK_POS (7)
#define LIN_TYPE_ENH_CHECK_MASK (0x1)

#define LIN_TYPE_RESERVED_8_31_POS (8)
#define LIN_TYPE_RESERVED_8_31_MASK (0xffffff)

#define LIN_BTCFG0_ADDR (SOC_LIN_REG_BASE + (0x2c << 2))

#define LIN_BTCFG0_BT_DIV1_POS (0)
#define LIN_BTCFG0_BT_DIV1_MASK (0xff)
#define LIN_BTCFG0_BT_DIV1_LEN (0x8)

#define LIN_BTCFG0_RESERVED_BIT_8_31_POS (8)
#define LIN_BTCFG0_RESERVED_BIT_8_31_MASK (0xffffff)

#define LIN_BTCFG1_ADDR (SOC_LIN_REG_BASE + (0x2d << 2))

#define LIN_BTCFG1_BT_DIV2_POS (0)
#define LIN_BTCFG1_BT_DIV2_MASK (0x1)

#define LIN_BTCFG1_BT_MUL_POS (1)
#define LIN_BTCFG1_BT_MUL_MASK (0x1f)

#define LIN_BTCFG1_PRESCL1_POS (6)
#define LIN_BTCFG1_PRESCL1_MASK (0x3)
#define LIN_BTCFG1_PRESCL1_LEN (0x2)

#define LIN_BTCFG1_RESERVED_8_31_POS (8)
#define LIN_BTCFG1_RESERVED_8_31_MASK (0xffffff)

#define LIN_IDENT_ADDR (SOC_LIN_REG_BASE + (0x2e << 2))

#define LIN_IDENT_POS (0)
#define LIN_IDENT_MASK (0x3f)

#define LIN_IDENT_ID_POS (0)
#define LIN_IDENT_ID_MASK (0xf)

#define LIN_IDENT_DIR_POS (4)
#define LIN_IDENT_DIR_MASK (0x1)

#define LIN_IDENT_RESERVED_6_31_POS (6)
#define LIN_IDENT_RESERVED_6_31_MASK (0x3ffffff)

#define LIN_TCFG_ADDR (SOC_LIN_REG_BASE + (0x2f << 2))

#define LIN_TCFG_WUP_REPEAT_TIME_POS (0)
#define LIN_TCFG_WUP_REPEAT_TIME_MASK (0x3)

#define LIN_TCFG_BUS_INACTIVITY_TIME_POS (2)
#define LIN_TCFG_BUS_INACTIVITY_TIME_MASK (0x3)

#define LIN_TCFG_RESERVED_4_5_POS (4)
#define LIN_TCFG_RESERVED_4_5_MASK (0x3)

#define LIN_TCFG_PRESCL2_POS (6)
#define LIN_TCFG_PRESCL2_MASK (0x1)

#define LIN_TCFG_RESERVED_7_31_POS (7)
#define LIN_TCFG_RESERVED_7_31_MASK (0x1ffffff)

#ifdef __cplusplus
}
#endif
