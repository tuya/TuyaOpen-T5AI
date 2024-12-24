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

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "partitions_gen.h"
#include "flash_partition.h"
#include "flash_map_backend/flash_map_backend.h"
#include "efuse_ll.h"
#include "aon_pmu_ll.h"
#include "boot_hal.h"

#define VIR_OFFSET FLASH_PHY2VIRTUAL(CONFIG_PRIMARY_TFM_S_PHY_PARTITION_OFFSET)
#define APP_OFFSET (FLASH_DEVICE_BASE + FLASH_CEIL_ALIGN(VIR_OFFSET + HDR_SZ, CPU_VECTOR_ALIGN_SZ))

//Use inline to avoid stack using!
static inline void deep_sleep_reset(void)
{
	if (aon_pmu_ll_get_r7b_fast_boot() && efuse_ll_is_enable_fast_boot() && (!aon_pmu_ll_get_r7b_ota_finish())) {
		__set_MSPLIM(0);
		__set_MSP(*(volatile uint32_t*)(APP_OFFSET));
		__DSB();
		__ISB();

		boot_jump_to_next_image(*(volatile uint32_t*)((APP_OFFSET) + 4));
	}
}

void xip_deep_sleep_reset(void);

#ifdef __cplusplus
}
#endif
