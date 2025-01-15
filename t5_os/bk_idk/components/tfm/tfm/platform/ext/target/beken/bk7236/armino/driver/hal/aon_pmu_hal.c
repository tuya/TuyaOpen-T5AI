// Copyright 2020-2021 Beken
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

#include <common/bk_include.h>
#include "aon_pmu_hal.h"
#include "aon_pmu_ll.h"
#include "system_hw.h"
#include "sys_types.h"
#include "modules/pm.h"

#define AON_PMU_LOGI(...) 
//#define AON_PMU_LOGI printf

uint32_t aon_pmu_hal_get_chipid(void)
{
	return aon_pmu_ll_get_r7c_id();
}

void aon_pmu_hal_r0_latch_to_r7b(void) {
	aon_pmu_ll_set_r25(0x424B55AA);
	aon_pmu_ll_set_r25(0xBDB4AA55);
}

uint32_t aon_pmu_hal_get_reset_reason(void) {
	return aon_pmu_ll_get_r7b_reset_reason();
}

void aon_pmu_hal_set_reset_reason(uint32_t value, bool write_immediately) {
	if (write_immediately) {
		uint32_t r0 = aon_pmu_ll_get_r0();
		uint32_t r7b = aon_pmu_ll_get_r7b();
		aon_pmu_ll_set_r0(r7b);
		aon_pmu_ll_set_r0_reset_reason(value);
		aon_pmu_hal_r0_latch_to_r7b();
		aon_pmu_ll_set_r0(r0);
		aon_pmu_ll_set_r0_reset_reason(value);
	} else {
		aon_pmu_ll_set_r0_reset_reason(value);
	}
}

static uint32_t s_reset_reason = 0;

void aon_pmu_hal_back_and_clear_reset_reason(void)
{
        AON_PMU_LOGI("before clear, sram_reset_reason=%x, r0=%x, r7b=%x, reason=%x, r7a=%x\r\n",
		s_reset_reason, aon_pmu_ll_get_r0(), aon_pmu_ll_get_r7b(), aon_pmu_hal_get_reset_reason(), aon_pmu_ll_get_r7a());
        s_reset_reason = aon_pmu_hal_get_reset_reason();
        aon_pmu_hal_set_reset_reason(RESET_SOURCE_BOOTLOADER_UNKNOWN, true);
        AON_PMU_LOGI("after clear, sram_reset_reason=%x, r0=%x, r7b=%x, reason=%x, r7a=%x\r\n",
		s_reset_reason, aon_pmu_ll_get_r0(), aon_pmu_ll_get_r7b(), aon_pmu_hal_get_reset_reason(), aon_pmu_ll_get_r7a());
}

void aon_pmu_hal_restore_reset_reason(void)
{
        AON_PMU_LOGI("before restore, sram_reset_reason=%x, r0=%x, r7b=%x, reason=%x, r7a=%x\r\n",
		s_reset_reason, aon_pmu_ll_get_r0(), aon_pmu_ll_get_r7b(), aon_pmu_hal_get_reset_reason(), aon_pmu_ll_get_r7a());
        aon_pmu_hal_set_reset_reason(s_reset_reason, true);
        AON_PMU_LOGI("after restore, sram_reset_reason=%x, r0=%x, r7b=%x, reason=%x, r7a=%x\r\n",
		s_reset_reason, aon_pmu_ll_get_r0(), aon_pmu_ll_get_r7b(), aon_pmu_hal_get_reset_reason(), aon_pmu_ll_get_r7a());
}

uint32_t aon_pmu_hal_get_reset_reason_from_sram(void)
{
	return s_reset_reason;
}

#if CONFIG_AON_PMU_HAL_TEST
static void aon_pmu_hal_test(uint32_t reset_reason)
{
	AON_PMU_LOGI("test reset_reason=%d\r\n", reset_reason);
	aon_pmu_hal_set_reset_reason(reset_reason, true);
	aon_pmu_hal_back_and_clear_reset_reason();
	AON_PMU_LOGI("get sram reset_reason=%x\r\n", reset_reason);
	aon_pmu_hal_restore_reset_reason();
}

void aon_pmu_hal_test_all(void)
{
	for (int i=0; i<=0xF; i++) {
		aon_pmu_hal_test(i);
	}
}

void trigger_bl2_cbus_wdt_reset(void)
{
	AON_PMU_LOGI("Trigger CBUS CRC WDT reset\r\n");
	*(volatile uint32_t *)(0x02010f00) = 1;
}

#endif
