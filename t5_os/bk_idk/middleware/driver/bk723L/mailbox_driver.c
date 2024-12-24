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

#include <components/log.h>
#include <common/bk_include.h>
#include "sys_driver.h"
#include "mailbox_driver.h"

void mailbox_interrupt_enable(mailbox_endpoint_t src)
{
	switch(src) {
		case MAILBOX_CPU0:
			sys_drv_int_group2_enable(MBOX0_INTERRUPT_CTRL_BIT);
			break;
		case MAILBOX_CPU1:
			sys_drv_int_group2_enable(MBOX1_INTERRUPT_CTRL_BIT);
			break;
		default:
			break;
		}
}

void mailbox_interrupt_disable(mailbox_endpoint_t src)
{
	switch(src) {
		case MAILBOX_CPU0:
			sys_drv_int_group2_disable(MBOX0_INTERRUPT_CTRL_BIT);
			break;
		case MAILBOX_CPU1:
			sys_drv_int_group2_disable(MBOX1_INTERRUPT_CTRL_BIT);
			break;
		default:
			break;
		}
}

#if CONFIG_SOC_BK7236_SMP_TEMP
int32 sys_drv_core_intr_group2_enable(uint32 core_id, uint32 param);
int32 sys_drv_core_intr_group2_disable(uint32 core_id, uint32 param);

void core_mbox_interrupt_enable(uint32 core_id, mailbox_endpoint_t src)
{
	switch(src) {
		case MAILBOX_CPU0:
			sys_drv_core_intr_group2_enable(core_id, MBOX0_INTERRUPT_CTRL_BIT);
			break;
		case MAILBOX_CPU1:
			sys_drv_core_intr_group2_enable(core_id, MBOX1_INTERRUPT_CTRL_BIT);
			break;
		default:
			break;
		}
}

void core_mbox_interrupt_disable(uint32 core_id, mailbox_endpoint_t src)
{
	switch(src) {
		case MAILBOX_CPU0:
			sys_drv_core_intr_group2_disable(core_id, MBOX0_INTERRUPT_CTRL_BIT);
			break;
		case MAILBOX_CPU1:
			sys_drv_core_intr_group2_disable(core_id, MBOX1_INTERRUPT_CTRL_BIT);
			break;
		default:
			break;
		}
}
#endif
// eof

