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


typedef volatile union {
	struct {
		uint32_t deviceid                 : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} lin_deviceid_t;


typedef volatile union {
	struct {
		uint32_t versionid                : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} lin_versionid_t;


typedef volatile union {
	struct {
		uint32_t soft_rst                 :  1; /**<bit[0 : 0] */
		uint32_t bypass_cfg               :  1; /**<bit[1 : 1] */
		uint32_t lin_master               :  1; /**<bit[2 : 2] */
		uint32_t reserved_bit_3_31        : 29; /**<bit[3 : 31] */
	};
	uint32_t v;
} lin_global_ctrl_t;


typedef volatile union {
	struct {
		uint32_t global_status            : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} lin_global_status_t;


typedef volatile union {
	struct {
		uint32_t byte                     :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} lin_data_t;

#if 0
typedef volatile union {
	struct {
		uint32_t byte                     :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} lin_data1_t;


typedef volatile union {
	struct {
		uint32_t byte                     :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} lin_data2_t;


typedef volatile union {
	struct {
		uint32_t byte                     :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} lin_data3_t;


typedef volatile union {
	struct {
		uint32_t byte                     :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} lin_data4_t;


typedef volatile union {
	struct {
		uint32_t byte                     :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} lin_data5_t;


typedef volatile union {
	struct {
		uint32_t byte                     :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} lin_data6_t;


typedef volatile union {
	struct {
		uint32_t byte                     :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} lin_data7_t;
#endif

typedef volatile union {
	struct {
		uint32_t start_req                :  1; /**<bit[0 : 0] */
		uint32_t wakeup                   :  1; /**<bit[1 : 1] */
		uint32_t reset_error              :  1; /**<bit[2 : 2] */
		uint32_t reset_int                :  1; /**<bit[3 : 3] */
		uint32_t data_ack                 :  1; /**<bit[4 : 4] */
		uint32_t transmit                 :  1; /**<bit[5 : 5] */
		uint32_t sleep                    :  1; /**<bit[6 : 6] */
		uint32_t stop                     :  1; /**<bit[7 : 7] */
		uint32_t reserved_8_31            : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} lin_ctrl_t;


typedef volatile union {
	struct {
		uint32_t complete                 :  1; /**<bit[0 : 0] */
		uint32_t wake_up                  :  1; /**<bit[1 : 1] */
		uint32_t error                    :  1; /**<bit[2 : 2] */
		uint32_t intr                     :  1; /**<bit[3 : 3] */
		uint32_t data_req                 :  1; /**<bit[4 : 4] */
		uint32_t aborted                  :  1; /**<bit[5 : 5] */
		uint32_t bus_idle_timeout         :  1; /**<bit[6 : 6] */
		uint32_t lin_active               :  1; /**<bit[7 : 7] */
		uint32_t reserved_8_31            : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} lin_status_t;


typedef volatile union {
	struct {
		uint32_t bit                      :  1; /**<bit[0 : 0] */
		uint32_t chk                      :  1; /**<bit[1 : 1] */
		uint32_t timeout                  :  1; /**<bit[2 : 2] */
		uint32_t parity                   :  1; /**<bit[3 : 3] */
		uint32_t reserved_4_31            : 28; /**<bit[4 : 31] */
	};
	uint32_t v;
} lin_err_t;


typedef volatile union {
	struct {
		uint32_t data_length              :  4; /**<bit[0 : 3] */
		uint32_t reserved_4_6             :  3; /**<bit[4 : 6] */
		uint32_t enh_check                :  1; /**<bit[7 : 7] */
		uint32_t reserved_8_31            : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} lin_type_t;


typedef volatile union {
	struct {
		uint32_t bt_div1                  :  8; /**<bit[0 : 7] */
		uint32_t reserved_bit_8_31        : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} lin_btcfg0_t;


typedef volatile union {
	struct {
		uint32_t bt_div2                  :  1; /**<bit[0 : 0] */
		uint32_t bt_mul                   :  5; /**<bit[1 : 5] */
		uint32_t prescl1                  :  2; /**<bit[6 : 7] */
		uint32_t reserved_8_31            : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} lin_btcfg1_t;


typedef volatile union {
	struct {
		uint32_t id                       :  4; /**<bit[0 : 3] */
		uint32_t tran_dir                 :  2; /**<bit[4 : 5] */
		uint32_t reserved_6_31            : 26; /**<bit[6 : 31] */
	};
	uint32_t v;
} lin_ident_t;


typedef volatile union {
	struct {
		uint32_t wup_repeat_time          :  2; /**<bit[0 : 1] */
		uint32_t bus_inactivity_time      :  2; /**<bit[2 : 3] */
		uint32_t reserved_4_5             :  2; /**<bit[4 : 5] */
		uint32_t prescl2                  :  1; /**<bit[6 : 6] */
		uint32_t reserved_7_31            : 25; /**<bit[7 : 31] */
	};
	uint32_t v;
} lin_tcfg_t;

typedef volatile struct {
	volatile lin_deviceid_t deviceid;
	volatile lin_versionid_t versionid;
	volatile lin_global_ctrl_t global_ctrl;
	volatile lin_global_status_t global_status;
	volatile uint32_t rsv_4_1f[28];
	volatile lin_data_t data[8];
	volatile lin_ctrl_t ctrl;
	volatile lin_status_t status;
	volatile lin_err_t err;
	volatile lin_type_t type;
	volatile lin_btcfg0_t btcfg0;
	volatile lin_btcfg1_t btcfg1;
	volatile lin_ident_t ident;
	volatile lin_tcfg_t tcfg;
} lin_hw_t;

#ifdef __cplusplus
}
#endif
