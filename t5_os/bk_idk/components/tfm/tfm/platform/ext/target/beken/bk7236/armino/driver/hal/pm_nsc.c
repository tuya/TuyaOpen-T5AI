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

#include <soc/soc.h>
#include "hal_port.h"
#include "system_hw.h"
#include "tfm_secure_api.h"
#include "crypto_hw.h"
#include "uart_hal.h"

#define PSA_PM_UART1_BACKUP_REG_NUM    6

static uint32_t s_pm_uart1_backup[PSA_PM_UART1_BACKUP_REG_NUM] = {0};

/* Calling this API in tfm_s.bin to trigger the compiler to generate NSC symbols in tfm_s_veneers.o.
 **/
void psa_pm_nsc_stub(void)
{
	return 0;
}

static int shanhai_backup(uint64_t sleep_time, void *args)
{
	return 0;
}

static int shanhai_restore(uint64_t sleep_time, void *args)
{
	crypto_hw_accelerator_init();

	return 0;
}

static int otp_ahb_backup(uint64_t sleep_time, void *args)
{
	//TODO
	return 0;
}

static int otp_ahb_restore(uint64_t sleep_time, void *args)
{
	//TODO
	return 0;
}

static int otp_apb_backup(uint64_t sleep_time, void *args)
{
	//TODO
	return 0;
}

static int otp_apb_restore(uint64_t sleep_time, void *args)
{
	//TODO
	return 0;
}

static int uart1_backup(uint64_t sleep_time, void *args)
{
	uart_hw_t *hw = (uart_hw_t *)(SOC_UART1_REG_BASE);

	s_pm_uart1_backup[0] = hw->config.v;
	s_pm_uart1_backup[1] = hw->fifo_config.v;
	s_pm_uart1_backup[2] = hw->int_enable.v;
	s_pm_uart1_backup[3] = hw->flow_ctrl_config.v;
	s_pm_uart1_backup[4] = hw->wake_config.v;
	s_pm_uart1_backup[5] = hw->global_ctrl.v;

	return 0;
}

static int uart1_restore(uint64_t sleep_time, void *args)
{
	uart_hw_t *hw = (uart_hw_t *)(SOC_UART1_REG_BASE);

	hw->config.v           = s_pm_uart1_backup[0];
	hw->fifo_config.v      = s_pm_uart1_backup[1];
	hw->int_enable.v       = s_pm_uart1_backup[2];
	hw->flow_ctrl_config.v = s_pm_uart1_backup[3];
	hw->wake_config.v      = s_pm_uart1_backup[4];
	hw->global_ctrl.v      = s_pm_uart1_backup[5];

	return 0;
}

static int dma_backup(uint64_t sleep_time, void* args)
{
	return 0;
}

void tfm_hal_dma_init(void);
static int dma_restore(uint64_t sleep_time, void* args)
{
	uint32_t ppc_r7_old;
	uint32_t ppc_r7_new;

	ppc_r7_old = *((volatile uint32_t *)(0x41040000 + (7 << 2)));
	ppc_r7_new = ppc_r7_old & ~3;
	*((volatile uint32_t *)(0x41040000 + (7 << 2))) = ppc_r7_new;
	tfm_hal_dma_init();
	*((volatile uint32_t *)(0x41040000 + (7 << 2))) = ppc_r7_old;
	return 0;
}

__tfm_psa_secure_gateway_no_naked_attributes__ int psa_pm_secure_world_backup(uint64_t sleep_time, void *args)
{
	uart1_backup(sleep_time, args);
	dubhe_driver_cleanup();
	return 0;
}

__tfm_psa_secure_gateway_no_naked_attributes__ int psa_pm_secure_world_restore(uint64_t sleep_time, void *args)
{
	uart1_restore(sleep_time, args);
	dma_restore(sleep_time, args);
	dubhe_driver_init(SOC_SHANHAI_BASE);
	return 0;
}
