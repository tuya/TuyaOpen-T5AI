// Copyright 2023-2024 Beken
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

#pragma once

#include <driver/hal/hal_lin_types.h>
#include <driver/lin_types.h>
#include <components/log.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t isr_cnt;
	uint32_t error_cnt;
	uint32_t completed_cnt;
	uint32_t data_req_cnt; /* slave only */
	uint32_t trans_cnt;
	uint32_t wakeup_cnt;
	uint32_t aborted_cnt; /* slave only */
	uint32_t bus_idle_timeout_cnt; /* slave only */
	uint32_t active_cnt;
	uint32_t bit_err_cnt;
	uint32_t chk_err_cnt;
	uint32_t timeout_err_cnt;
	uint32_t parity_err_cnt; /* slave only */
} lin_statis_t;

#if CONFIG_LIN_STATIS

#define LIN_STATIS_DEC()  lin_statis_t* lin_statis = NULL
#define LIN_STATIS_GET(_statis, _id) (_statis) = lin_statis_get_statis((_id))

#define LIN_STATIS_SET(_statis, _v) do{\
		(_statis) = (_v);\
	} while(0)

#define LIN_STATIS_ADD(_statis, _v) do{\
		(_statis) += (_v);\
	} while(0)

bk_err_t lin_statis_init(void);
bk_err_t lin_statis_id_init(lin_id_t id);
lin_statis_t* lin_statis_get_statis(lin_id_t id);
void lin_statis_dump(lin_id_t id);
#else
#define LIN_STATIS_DEC() 
#define LIN_STATIS_GET(_statis, _id) 
#define LIN_STATIS_SET(_id, _v)
#define LIN_STATIS_ADD(_statis, _v)
#define lin_statis_init()
#define lin_statis_id_init(id)
#define lin_statis_get_statis(id) NULL
#define	lin_statis_dump(id)
#endif

#define LIN_STATIS_INC(_statis) LIN_STATIS_ADD((_statis), 1)

#ifdef __cplusplus
}
#endif
