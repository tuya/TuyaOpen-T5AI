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

#include "platform_irq.h"
#include "STAR_SE.h"
#include "core_star.h"
#include "sdkconfig.h"
#include "mpu.h"
#include "region.h"
#include "cmsis.h"
#include "sleep.h"

#if CONFIG_DIRECT_XIP
void xip_deep_sleep_reset(void)
{
	struct boot_arm_vector_table *vt;

	if (sys_is_running_from_deep_sleep() && sys_is_enable_fast_boot() && (!sys_is_running_from_ota())) {
		//BOOT_LOG_INF("deep sleep fastboot");
		extern uint32_t get_flash_map_offset(uint32_t index);
		uint32_t phy_offset  = get_flash_map_offset(0);
		uint32_t virtual_off = FLASH_PHY2VIRTUAL(phy_offset);
		vt = (struct boot_arm_vector_table *)(FLASH_DEVICE_BASE +
					FLASH_CEIL_ALIGN(virtual_off + HDR_SZ, CPU_VECTOR_ALIGN_SZ));

#if CONFIG_DIRECT_XIP
		uint32_t candidate_slot = 0;
		uint32_t reg = aon_pmu_ll_get_r7b();
		aon_pmu_ll_set_r0(reg);
		candidate_slot = !!(reg & BIT(2));
		extern void flash_set_excute_enable(int enable);
		flash_set_excute_enable(candidate_slot);
#endif

		boot_platform_quit(vt);
	}
}
#endif

