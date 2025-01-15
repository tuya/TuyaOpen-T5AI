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

#include "sdkconfig.h"
#include "armstar.h"
#include "arch_interrupt.h"
#include "components/log.h"

#define INT_NUMBER_MAX              (64)
#define TO_NVIC_IRQ(irq)            ((uint32_t)(irq))

static int_group_isr_t s_irq_handler[INT_NUMBER_MAX];

void arch_int_enable_irq(uint32_t irq)
{
	NVIC_EnableIRQ(TO_NVIC_IRQ(irq));
}

void arch_int_disable_irq(uint32_t irq)
{
	NVIC_DisableIRQ(TO_NVIC_IRQ(irq));
}

uint32_t arch_int_get_enable_irq(uint32_t irq)
{
	return NVIC_GetEnableIRQ(TO_NVIC_IRQ(irq));
}

void arch_int_set_target_state(uint32_t irq)
{
	NVIC_SetTargetState(TO_NVIC_IRQ(irq));
}

void arch_int_clear_target_state(uint32_t irq)
{
	NVIC_ClearTargetState(TO_NVIC_IRQ(irq));
}

uint32_t arch_int_get_target_state(uint32_t irq)
{
	return NVIC_GetTargetState(TO_NVIC_IRQ(irq));
}

void arch_interrupt_set_priority(uint32_t int_number, uint32_t int_priority)
{
	if (int_number > 0 && int_number < INT_NUMBER_MAX) {
		NVIC_SetPriority(TO_NVIC_IRQ(int_number), int_priority);
	}

	return;
}

void arch_interrupt_register_int(uint32_t int_number, int_group_isr_t isr_callback)
{
	if ((int_number > (INT_NUMBER_MAX - 1)) || isr_callback == NULL) {
		return;
	}

	s_irq_handler[int_number] = isr_callback;
	NVIC_EnableIRQ(int_number);
}

void arch_interrupt_unregister_int(uint32_t int_number)
{
	if (int_number > (INT_NUMBER_MAX - 1)) {
		return;
	}
	NVIC_DisableIRQ(int_number);
	s_irq_handler[int_number] = NULL;
}

__attribute__((section(".itcm_sec_code"))) int_group_isr_t arch_interrupt_get_handler(uint32_t int_number)
{
	return s_irq_handler[int_number];
}
// eof

