/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#include <stdint.h>

#include "cmsis.h"
#include "spm_ipc.h"
#include "tfm_hal_interrupt.h"
#include "tfm_peripherals_def.h"
#include "ffm/interrupt.h"
#include "load/interrupt_defs.h"
#include <components/log.h>

#define TAG "int"

static struct irq_t timer0_irq = {0};

void bfhfnmi_handler(void)
{
	uint32_t *psp_ns_pointer = (uint32_t  *)__TZ_get_PSP_NS();
	BK_LOGE(TAG, "\r\n[SPE]psp_ns_pointer:0x%x\r\n", psp_ns_pointer);

	printf_word_buf_hex(psp_ns_pointer, 128);
	while(1);
}

void NMI_Handler(void)
{
	BK_LOGE(TAG, "NMI\r\n");
	bfhfnmi_handler();
}

void HardFault_Handler(void)
{
	BK_LOGE(TAG, "HF\r\n");
	bfhfnmi_handler();
}

void MemManage_Handler(void)
{
	BK_LOGE(TAG, "MM\r\n");
	bfhfnmi_handler();
}

void BusFault_Handler(void)
{
	BK_LOGE(TAG, "BF\r\n");
	bfhfnmi_handler();
}

void UsageFault_Handler(void)
{
	BK_LOGE(TAG, "UF\r\n");
	bfhfnmi_handler();
}

void SecureFault_Handler(void)
{
	BK_LOGE(TAG, "SF\r\n");
	bfhfnmi_handler();
}

void UART1_Handler(void)
{
	BK_LOGE(TAG, "UART1 isr\r\n");
	while(1);
}

void GPIO_Handler(void)
{
	BK_LOGE(TAG, "GPIO s isr\r\n");
	while(1);
}

void TFM_TIMER0_Handler(void)
{
    spm_handle_interrupt(timer0_irq.p_pt, timer0_irq.p_ildi);
}

enum tfm_hal_status_t tfm_timer0_irq_init(void *p_pt,
                                          struct irq_load_info_t *p_ildi)
{
    timer0_irq.p_ildi = p_ildi;
    timer0_irq.p_pt = p_pt;

	NVIC_SetPriority(SVCall_IRQn, 1);
    NVIC_SetPriority(TFM_TIMER0_IRQ, DEFAULT_IRQ_PRIORITY);
    NVIC_ClearTargetState(TFM_TIMER0_IRQ);
    NVIC_DisableIRQ(TFM_TIMER0_IRQ);

    return TFM_HAL_SUCCESS;
}

#ifdef PSA_API_TEST_IPC
static struct irq_t ff_test_uart_irq;

void FF_TEST_UART_IRQ_Handler(void)
{
    spm_handle_interrupt(ff_test_uart_irq.p_pt, ff_test_uart_irq.p_ildi);
}

enum tfm_hal_status_t ff_test_uart_irq_init(void *p_pt,
                                            struct irq_load_info_t *p_ildi)
{
    ff_test_uart_irq.p_ildi = p_ildi;
    ff_test_uart_irq.p_pt = p_pt;

    NVIC_SetPriority(FF_TEST_UART_IRQ, DEFAULT_IRQ_PRIORITY);
    NVIC_ClearTargetState(FF_TEST_UART_IRQ);
    NVIC_DisableIRQ(FF_TEST_UART_IRQ);

    return TFM_HAL_SUCCESS;
}

#endif
