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
		uint32_t id                       :  29; /**<bit[0 : 28] */
		uint32_t reserved_29_30           :  2; /**<bit[29 : 30] */
		uint32_t esi                      :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} can_id_esi_t;

// typedef volatile union {
// 	struct {
// 		uint32_t id                       :  5; /**<bit[0 : 4] */
// 		uint32_t reserved_5_6             :  2; /**<bit[5 : 6] */
// 		uint32_t esi                      :  1; /**<bit[7 : 7] */
// 	};
// 	uint32_t v;
// } can_id_esi_t;


typedef volatile union {
	struct {
		uint32_t dlc                      :  4; /**<bit[0 : 3] */
		uint32_t brs                      :  1; /**<bit[4 : 4] */
		uint32_t fdf                      :  1; /**<bit[5 : 5] */
		uint32_t rtr                      :  1; /**<bit[6 : 6] */
		uint32_t ide                      :  1; /**<bit[7 : 7] */
		uint32_t reserved_8_31            : 24; /**<bit[8 : 31] */
	};
	uint32_t v;
} can_buf_ctrl_t;


typedef volatile union {
	struct {
		uint32_t data                    : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} can_data_t;

typedef volatile union {
	struct {
		uint32_t tts                      : 32; /**<bit[0 : 31] */
	};
	uint32_t v;
} can_tts_t;


typedef volatile union {
	struct {
		uint32_t busoff                   :  1; /**<bit[0 : 0] */
		uint32_t tactive                  :  1; /**<bit[1 : 1] */
		uint32_t ractive                  :  1; /**<bit[2 : 2] */
		uint32_t tsss                     :  1; /**<bit[3 : 3] */
		uint32_t tpss                     :  1; /**<bit[4 : 4] */
		uint32_t lbmi                     :  1; /**<bit[5 : 5] */
		uint32_t lbme                     :  1; /**<bit[6 : 6] */
		uint32_t reset                    :  1; /**<bit[7 : 7] */
		uint32_t tsa                      :  1; /**<bit[8 : 8] */
		uint32_t tsall                    :  1; /**<bit[9 : 9] */
		uint32_t tsone                    :  1; /**<bit[10 : 10] */
		uint32_t tpa                      :  1; /**<bit[11 : 11] */
		uint32_t tpe                      :  1; /**<bit[12 : 12] */
		uint32_t stby                     :  1; /**<bit[13 : 13] */
		uint32_t lom                      :  1; /**<bit[14 : 14] */
		uint32_t tbsel                    :  1; /**<bit[15 : 15] */
		uint32_t tsstat                   :  2; /**<bit[16 : 17] */
		uint32_t reserved_18_19           :  2; /**<bit[18 : 19] */
		uint32_t tttbm                    :  1; /**<bit[20 : 20] */
		uint32_t tsmode                   :  1; /**<bit[21 : 21] */
		uint32_t tsnext                   :  1; /**<bit[22 : 22] */
		uint32_t fd_iso                   :  1; /**<bit[23 : 23] */
		uint32_t rstat                    :  2; /**<bit[24 : 25] */
		uint32_t reserved_26_26           :  1; /**<bit[26 : 26] */
		uint32_t rball                    :  1; /**<bit[27 : 27] */
		uint32_t rrel                     :  1; /**<bit[28 : 28] */
		uint32_t rov                      :  1; /**<bit[29 : 29] */
		uint32_t rom                      :  1; /**<bit[30 : 30] */
		uint32_t sack                     :  1; /**<bit[31 : 21] */
	};
	uint32_t v;
} can_cfg_t;

typedef volatile union {
	struct {
		uint32_t tsff                     :  1; /**<bit[0 : 0] */
		uint32_t eie                      :  1; /**<bit[1 : 1] */
		uint32_t tsie                     :  1; /**<bit[2 : 2] */
		uint32_t tpie                     :  1; /**<bit[3 : 3] */
		uint32_t rafie                    :  1; /**<bit[4 : 4] */
		uint32_t rfie                     :  1; /**<bit[5 : 5] */
		uint32_t roie                     :  1; /**<bit[6 : 6] */
		uint32_t rie                      :  1; /**<bit[7 : 7] */
		uint32_t aif                      :  1; /**<bit[8 : 8] */
		uint32_t eif                      :  1; /**<bit[9 : 9] */
		uint32_t tsif                     :  1; /**<bit[10 : 10] */
		uint32_t tpif                     :  1; /**<bit[11 : 11] */
		uint32_t rafif                    :  1; /**<bit[12 : 12] */
		uint32_t rfif                     :  1; /**<bit[13 : 13] */
		uint32_t roif                     :  1; /**<bit[14 : 14] */
		uint32_t rif                      :  1; /**<bit[15 : 15] */
		uint32_t beif                     :  1; /**<bit[16 : 16] */
		uint32_t beie                     :  1; /**<bit[17 : 17] */
		uint32_t alif                     :  1; /**<bit[18 : 18] */
		uint32_t alie                     :  1; /**<bit[19 : 19] */
		uint32_t epif                     :  1; /**<bit[20 : 20] */
		uint32_t epie                     :  1; /**<bit[21 : 21] */
		uint32_t epass                    :  1; /**<bit[22 : 22] */
		uint32_t ewarn                    :  1; /**<bit[23 : 23] */
		uint32_t ewl                      :  4; /**<bit[24 : 27] */
		uint32_t afwl                     :  4; /**<bit[28 : 31] */
	};
	uint32_t v;
} can_ie_t;

typedef volatile union {
	struct {
		uint32_t s_seg_1                  :  8; /**<bit[0 : 7] */
		uint32_t s_seg_2                  :  7; /**<bit[8 : 14] */
		uint32_t reserved_15_15           :  1; /**<bit[15 : 15] */
		uint32_t s_sjw                    :  7; /**<bit[16 : 22] */
		uint32_t reserved_23_23           :  1; /**<bit[23 : 23] */
		uint32_t s_presc                  :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} can_sseg_t;

typedef volatile union {
	struct {
		uint32_t f_seg_1                  :  5; /**<bit[0 : 4] */
		uint32_t reserved_5_7             :  3; /**<bit[5 : 7] */
		uint32_t f_seg_2                  :  4; /**<bit[8 : 11] */
		uint32_t reserved_12_15           :  4; /**<bit[12 : 15] */
		uint32_t f_sjw                    :  4; /**<bit[16 : 19] */
		uint32_t reserved_20_23           :  4; /**<bit[20 : 23] */
		uint32_t f_presc                  :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} can_fseg_t;

typedef volatile union {
	struct {
		uint32_t alc                      :  5; /**<bit[0 : 4] */
		uint32_t koer                     :  3; /**<bit[5 : 7] */
		uint32_t sspoff                   :  7; /**<bit[8 : 14] */
		uint32_t tdcen                    :  1; /**<bit[15 : 15] */
		uint32_t recnt                    :  8; /**<bit[16 : 23] */
		uint32_t tecnt                    :  8; /**<bit[24 : 31] */
	};
	uint32_t v;
} can_cap_t;

typedef volatile union {
	struct {
		uint32_t acfadr                   :  4; /**<bit[0 : 3] */
		uint32_t reserved_4_4             :  1; /**<bit[4 : 4] */
		uint32_t selmask                  :  1; /**<bit[5 : 5] */
		uint32_t reserved_6_7             :  2; /**<bit[6 : 7] */
		uint32_t timeen                   :  1; /**<bit[8 : 8] */
		uint32_t timepos                  :  1; /**<bit[9 : 9] */
		uint32_t reserved_10_15           :  6; /**<bit[10 : 15] */
		uint32_t acf_en                   :  16; /**<bit[16 : 31] */
	};
	uint32_t v;
} can_acf_t;

typedef volatile union {
	struct {
		uint32_t acode_or_amask           :  29; /**<bit[0 : 28] */
		uint32_t aide                     :  1; /**<bit[29 : 29] */
		uint32_t aidee                    :  1; /**<bit[30 : 30] */
		uint32_t reserved_31_31           :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} can_aid_t;

typedef volatile union {
	struct {
		uint32_t ver_0                    :  8; /**<bit[0 : 7] */
		uint32_t ver_1                    :  8; /**<bit[8 : 15] */
		uint32_t tbptr                    :  6; /**<bit[16 : 21] */
		uint32_t tbf                      :  1; /**<bit[22 : 22] */
		uint32_t tbe                      :  1; /**<bit[23 : 23] */
		uint32_t tten                     :  1; /**<bit[24 : 25] */
		uint32_t t_presc                  :  2; /**<bit[25 : 26] */
		uint32_t ttif                     :  1; /**<bit[27 : 27] */
		uint32_t ttie                     :  1; /**<bit[28 : 28] */
		uint32_t teif                     :  1; /**<bit[29 : 29] */
		uint32_t wtif                     :  1; /**<bit[30 : 30] */
		uint32_t wtie                     :  1; /**<bit[31 : 31] */		
	};
	uint32_t v;
} can_ttcfg_t;

typedef volatile union {
	struct {
		uint32_t ref_id                   : 29; /**<bit[0 : 28] */
		uint32_t reserved_29_30           :  2; /**<bit[29 : 30] */
		uint32_t ref_ide                  :  1; /**<bit[31 : 31] */
	};
	uint32_t v;
} can_ref_msg_t;


typedef volatile union {
	struct {
		uint32_t ttptr                    :  6; /**<bit[0 : 5] */
		uint32_t reserved_6_7             :  2; /**<bit[6 : 7] */
		uint32_t ttype                    :  3; /**<bit[8 : 10] */
		uint32_t reserved_11_11           :  1; /**<bit[11 : 11] */
		uint32_t tew                      :  4; /**<bit[12 : 15] */
		uint32_t tttrig                   : 16; /**<bit[16 : 31] */
	};
	uint32_t v;
} can_trig_cfg_t;

typedef volatile union {
	struct {
		uint32_t ttwtrig                  : 16; /**<bit[0 : 15] */
		uint32_t mpen                     :  1; /**<bit[16 : 16] */
		uint32_t mdwie                    :  1; /**<bit[17 : 17] */
		uint32_t mdwif                    :  1; /**<bit[18 : 18] */
		uint32_t mdeif                    :  1; /**<bit[19 : 19] */
		uint32_t maeif                    :  1; /**<bit[20 : 20] */
		uint32_t reserved_21_23           :  3; /**<bit[21 : 23] */
		uint32_t acfa                     :  1; /**<bit[24 : 24] */
		uint32_t txs                      :  1; /**<bit[25 : 25] */
		uint32_t txb                      :  1; /**<bit[26 : 26] */
		uint32_t heloc                    :  2; /**<bit[27 : 28] */
		uint32_t reserved_29_31           :  3; /**<bit[29 : 31] */
	};
	uint32_t v;
} can_mem_stat_t;

typedef volatile union {
	struct {
		uint32_t mebp1                    :  6; /**<bit[0 : 5] */
		uint32_t me1ee                    :  1; /**<bit[6 : 6] */
		uint32_t meaee                    :  1; /**<bit[7 : 7] */
		uint32_t mebp2                    :  6; /**<bit[8 : 13] */
		uint32_t me2ee                    :  1; /**<bit[14 : 14] */
		uint32_t reserved_15_15           :  1; /**<bit[15 : 15] */
		uint32_t meeec                    :  4; /**<bit[16 : 19] */
		uint32_t menec                    :  4; /**<bit[20 : 23] */
		uint32_t mel                      :  2; /**<bit[24 : 25] */
		uint32_t mes                      :  1; /**<bit[26 : 26] */
		uint32_t reserved_27_31           :  5; /**<bit[27 : 31] */
	};
	uint32_t v;
} can_mem_es_t;

typedef volatile union {
	struct {
		uint32_t xmren                    :  1; /**<bit[0 : 0] */
		uint32_t seif                     :  1; /**<bit[1 : 1] */
		uint32_t swie                     :  1; /**<bit[2 : 2] */
		uint32_t swif                     :  1; /**<bit[3 : 3] */
		uint32_t fstim                    :  3; /**<bit[4 : 6] */
		uint32_t reserved_7_31            : 25; /**<bit[7 : 31] */
	};
	uint32_t v;
} can_scfg_t;

typedef volatile union {
	struct {
		uint32_t can_fd_enable            :  1; /**<bit[0 : 0] */
		uint32_t reserved_1_31            : 31; /**<bit[1 : 31] */
	};
	uint32_t v;
} can_fd_t;

#ifdef __cplusplus
}
#endif
