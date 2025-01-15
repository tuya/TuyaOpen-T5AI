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

#include "soc/soc.h"
#include "wdt_hal.h"
#include "aon_pmu_hal.h"
#include <components/system.h>

#ifdef __cplusplus
extern "C" {
#endif

void nmi_wdt_set(uint32_t val)
{
	REG_WRITE(SOC_WDT_REG_BASE + 4 * 2, 0x3); 
	REG_WRITE(SOC_WDT_REG_BASE + 4 * 4, 0x5A0000 | val);
	REG_WRITE(SOC_WDT_REG_BASE + 4 * 4, 0xA50000 | val);
}

void nmi_wdt_stop(void)
{
	nmi_wdt_set(0);
}

void aon_wdt_set(uint32_t val)
{
	REG_WRITE(SOC_AON_WDT_REG_BASE, 0x5A0000 | val);
	REG_WRITE(SOC_AON_WDT_REG_BASE, 0xA50000 | val);
}

void aon_wdt_feed()
{
#if CONFIG_BL2_WDT
	aon_wdt_set(CONFIG_BL2_WDT_PERIOD);
#endif
}

void aon_wdt_stop(void)
{
	aon_wdt_set(0);
}

void wdt_stop(void)
{
	nmi_wdt_stop();
	aon_wdt_stop();
}

void wdt_reboot(void)
{
	aon_pmu_hal_set_reset_reason(RESET_SOURCE_BOOTLOADER_REBOOT, true);
	aon_wdt_set(0x30);
	while(1);
}


#ifdef __cplusplus
}
#endif



