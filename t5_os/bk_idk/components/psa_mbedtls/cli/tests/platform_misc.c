// Copyright 2024-2025 Beken
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

#include "common.h"
#include <components/log.h>
#include <modules/pm.h>
#include <driver/aon_rtc.h>

static uint32_t s_int0, s_int1;

void crypto_lock(void)
{
        s_int0 = *(volatile uint32_t*)(SOC_SYS_REG_BASE + 0x20*4);
        s_int1 = *(volatile uint32_t*)(SOC_SYS_REG_BASE + 0x21*4);

        *(volatile uint32_t*)(SOC_SYS_REG_BASE + 0x20*4) = 0;
        *(volatile uint32_t*)(SOC_SYS_REG_BASE + 0x21*4) = 0;
}

void crypto_unlock(void)
{
        *(volatile uint32_t*)(SOC_SYS_REG_BASE + 0x20*4) = s_int0;
        *(volatile uint32_t*)(SOC_SYS_REG_BASE + 0x21*4) = s_int1;
}

uint64_t crypto_get_time(void)
{
	return bk_aon_rtc_get_us();
}

uint32_t crypto_diff_time(uint64_t end, uint64_t begin)
{
	return end - begin;
}

static const char* cpu_freq_str[] = {"120M", "240M"};
static uint32_t s_cpu_freq_idx = 0;

int crypto_set_cpu_freq(uint32_t freq)
{
	if (0 == bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, freq)) {
		if (freq == PM_CPU_FRQ_240M) {
			s_cpu_freq_idx = 1;
		} else if (freq == PM_CPU_FRQ_120M) {
			s_cpu_freq_idx = 0;
		} 
	}

	return 0;
}

void crypto_perf_log(const char* type, const char* cpu_freq, uint32_t key_len, uint32_t data_len, uint32_t time)
{
	uint32_t throughput = (data_len * 8 * 1000) / time;

	(void*)cpu_freq;
	BK_LOGI("crypto_perf", "@%s@%s@%u@%u@%u@%u@\r\n", type, cpu_freq_str[s_cpu_freq_idx], key_len, data_len, time, throughput);
}
