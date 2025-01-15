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

#include "bk7236xx.h"
#include "smp.h"
#include "soc/soc.h"

void cpu0_set_core_id(void)
{
	REG_WRITE(CPU_ID_ADDR, CPU0_CORE_ID);
}

void cpu1_set_core_id(void)
{
	REG_WRITE(CPU_ID_ADDR, CPU1_CORE_ID);
}

void cpu2_set_core_id(void)
{
	REG_WRITE(CPU_ID_ADDR, CPU2_CORE_ID);
}

uint32_t cpu_get_core_id(void)
{
	return REG_READ(CPU_ID_ADDR);
}

// eof

