/*
 * FreeRTOS V202107.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/******************************************************************************
 * NOTE 1:  This project provides two demo applications.  A simple blinky
 * style project, and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting in main.c is used to select
 * between the two.  See the notes on using mainCREATE_SIMPLE_BLINKY_DEMO_ONLY
 * in main.c.  This file implements the simply blinky style version.
 *
 * NOTE 2:  This file only contains the source code that is specific to the
 * basic demo.  Generic functions, such FreeRTOS hook functions, and functions
 * required to configure the hardware are defined in main.c.
 ******************************************************************************
 *
 * main_blinky() creates one queue, and two tasks.  It then starts the
 * scheduler.
 *
 * The Queue Send Task:
 * The queue send task is implemented by the prvQueueSendTask() function in
 * this file.  prvQueueSendTask() sits in a loop that causes it to repeatedly
 * block for 200 milliseconds, before sending the value 100 to the queue that
 * was created within main_blinky().  Once the value is sent, the task loops
 * back around to block for another 200 milliseconds...and so on.
 *
 * The Queue Receive Task:
 * The queue receive task is implemented by the prvQueueReceiveTask() function
 * in this file.  prvQueueReceiveTask() sits in a loop where it repeatedly
 * blocks on attempts to read data from the queue that was created within
 * main_blinky().  When data is received, the task checks the value of the
 * data, and if the value equals the expected 100, toggles an LED.  The 'block
 * time' parameter passed to the queue receive function specifies that the
 * task should be held in the Blocked state indefinitely to wait for data to
 * be available on the queue.  The queue receive task will only leave the
 * Blocked state when the queue send task writes to the queue.  As the queue
 * send task writes to the queue every 200 milliseconds, the queue receive
 * task leaves the Blocked state every 200 milliseconds, and therefore toggles
 * the LED every 200 milliseconds.
 */

/* Library includes. */
#include <stdint.h>
#include "deep_lv.h"
#include "bk7236xx.h"
#include "partition_bk7236xx.h"
#include "FreeRTOS.h"
#include "core_star.h"

static DLV_SEC dlv_context_t s_dlv_context = {{0}};
static uint32_t port_deep_lv_flag = 0;

extern void flush_all_dcache(void);

uint32_t dlv_is_startup(void)
{
	return port_deep_lv_flag;
}

void dlv_startup(void)
{
	port_deep_lv_flag = 0;
	dlv_context_restore();
}

void dlv_trigger_backup_context(void)
{
	__asm volatile
	(
		" .syntax unified           \n"
		" dsb                       \n"
		" isb                       \n"
		" svc %0                    \n"
		" nop                       \n"
		"                           \n"
		::"i"(portSVC_DEEP_LV_ENTER):"memory"
	);

	/*PATCH:get control register*/
	dlv_context_t *dlv = &s_dlv_context;
	dlv_core_t *core_info = &(dlv->core);
	core_info->control_s_val = __get_CONTROL();
	core_info->control_ns_val = __TZ_get_CONTROL_NS();
}

/*************************************************************/
DLV_STATIC void dlv_scb_save(dlv_context_t *dlv)
{
	dlv_scb_t *scb_info = &(dlv->sys_ctrl);

	scb_info->aircr_val = SCB->AIRCR;
	scb_info->ccr_val = SCB->CCR;
	scb_info->cpacr_val = SCB->CPACR;
	scb_info->nsacr_val = SCB->NSACR;
	scb_info->vtor_val = SCB->VTOR;
}

DLV_STATIC void dlv_nvic_save(dlv_context_t *dlv)
{
	dlv_nvic_t *nvic_info = &(dlv->nvic);

	nvic_info->iser_val[0] = NVIC->ISER[0];
	nvic_info->iser_val[1] = NVIC->ISER[1];
	nvic_info->icer_val[0] = NVIC->ICER[0];
	nvic_info->icer_val[1] = NVIC->ICER[1];
	nvic_info->itns_val[0] = NVIC->ITNS[0];
	nvic_info->itns_val[1] = NVIC->ITNS[1];
	nvic_info->ipr_val[0] = NVIC->IPR[0];
	nvic_info->ipr_val[1] = NVIC->IPR[1];
}

DLV_STATIC void dlv_sau_save(dlv_context_t *dlv)
{
	uint32_t i;
	dlv_sau_t *sau_info = &(dlv->sau);

	sau_info->is_enable = (SAU->CTRL) & SAU_CTRL_ENABLE_Msk;
	if(sau_info->is_enable){
		sau_info->ctrl_val = SAU->CTRL;
		sau_info->rnr_val = SAU->RNR;

		for(i = 0; i < 8; i ++){
			SAU->RNR = i;
			sau_info->rbar_val[i] = SAU->RBAR;
			sau_info->rlar_val[i] = SAU->RLAR;
		}
	}
}

DLV_STATIC void dlv_mpu_save(dlv_context_t *dlv)
{
	uint32_t i;
	dlv_mpu_t *mpu_info = &(dlv->mpu);

	mpu_info->is_enable = (MPU->CTRL) & MPU_CTRL_ENABLE_Msk;
	if(mpu_info->is_enable){
		mpu_info->ctrl_val = MPU->CTRL;
		mpu_info->rnr_val = MPU->RNR;

		for(i = 0; i < 16; i ++){
			MPU->RNR = i;
			mpu_info->rbar_val[i] = MPU->RBAR;
			mpu_info->rlar_val[i] = MPU->RLAR;
		}
	}
}

DLV_STATIC void dlv_fpu_save(dlv_context_t *dlv)
{
	dlv_fpu_t *fpu_info = &(dlv->fpu);

	fpu_info->fpcar_val = FPU->FPCAR;
	fpu_info->fpccr_val = FPU->FPCCR;
	fpu_info->fpdscr_val = FPU->FPDSCR;
}

DLV_STATIC void dlv_itcm_save(dlv_context_t *dlv)
{
	dlv_itcm_t *itcm_info = &(dlv->itcm);

	itcm_info->tcmcr_val = TCM->ITCMCR;
	itcm_info->tgu_ctrl_val = ITGU->TGU_CTRL;
	itcm_info->tgu_lut_val = ITGU->TGU_LUT;
}

DLV_STATIC void dlv_dtcm_save(dlv_context_t *dlv)
{
	dlv_dtcm_t *dtcm_info = &(dlv->dtcm);

	dtcm_info->tcmcr_val = TCM->DTCMCR;
	dtcm_info->tgu_ctrl_val = DTGU->TGU_CTRL;
	dtcm_info->tgu_lut_val = DTGU->TGU_LUT;
}

DLV_STATIC void dlv_core_save(dlv_context_t *dlv)
{
	dlv_core_t *core_info = &(dlv->core);

	core_info->msplim_val = __get_MSPLIM();
	core_info->psplim_val = __get_PSPLIM();
	core_info->psp_val = __get_PSP();

	core_info->basepri_s_val = __get_BASEPRI();
	core_info->basepri_ns_val = __TZ_get_BASEPRI_NS();
#if CONFIG_GET_AT_SVC_CONTEXT
	/*FIXME:control register is not the contex of deep lv*/
	core_info->control_s_val = __get_CONTROL();
	core_info->control_ns_val = __TZ_get_CONTROL_NS();
#endif
	core_info->primask_s_val  = __get_PRIMASK();
	core_info->primask_ns_val = __TZ_get_PRIMASK_NS();
	core_info->fault_mask_s_val = __get_FAULTMASK();
	core_info->fault_mask_ns_val = __TZ_get_FAULTMASK_NS();
}

void dlv_stack_frame_save_and_dlv(void)
{
	uint32_t xpsr_val, lr_val, reg;
	dlv_context_t *dlv = &s_dlv_context;
	dlv_stack_frame_t *stack_frame_info = &(dlv->stk_frame);

	port_deep_lv_flag = 1;
	__asm volatile
	(
		"mrs %[result], xPSR\n"
		:[result]"=r"(xpsr_val)
	);
	__asm volatile
	(
		"mov %[result], lr\n"
		:[result]"=r"(lr_val)
	);
	stack_frame_info->lr = lr_val;
	stack_frame_info->xpsr = xpsr_val;

	flush_all_dcache();
	
	__asm volatile
	(
		"dsb          \n"
		"isb          \n"
	);

	//FIXME power down cpu1, TODO hard coding
	reg = *((volatile uint32_t *)(0x44010000 + 0x5 * 4));
	reg |= (1 << 1);       /* power down cpu1*/
	reg &= (~(1 << 0));    /*clear reset cpu1 bit*/
	*((volatile uint32_t *)(0x44010000 + 0x5 * 4)) = reg;
}

/*************************************************************/
DLV_STATIC void dlv_scb_restore(dlv_context_t *dlv)
{
	dlv_scb_t *scb_info = &(dlv->sys_ctrl);
	uint32_t ccr_val = scb_info->ccr_val;

	SCB->VTOR = scb_info->vtor_val;
	SCB->CPACR = scb_info->cpacr_val;
	SCB->CCR = ccr_val;
	SCB->NSACR = scb_info->nsacr_val;
	SCB->AIRCR = scb_info->aircr_val;

	if(ccr_val & SCB_CCR_IC_Msk){
		SCB_EnableICache();
	}

	if(ccr_val & SCB_CCR_DC_Msk){
		SCB_EnableDCache();
		SCB_CleanInvalidateDCache();
	}
}

DLV_STATIC void dlv_nvic_restore(dlv_context_t *dlv)
{
	dlv_nvic_t *nvic_info = &(dlv->nvic);

	NVIC->ISER[0] = nvic_info->iser_val[0];
	NVIC->ISER[1] = nvic_info->iser_val[1];
	NVIC->ICER[0] = nvic_info->icer_val[0];
	NVIC->ICER[1] = nvic_info->icer_val[1];
	NVIC->ITNS[0] = nvic_info->itns_val[0];
	NVIC->ITNS[1] = nvic_info->itns_val[1];
	NVIC->IPR[0] = nvic_info->ipr_val[0];
	NVIC->IPR[1] = nvic_info->ipr_val[1];
}

DLV_STATIC void dlv_sau_restore(dlv_context_t *dlv)
{
	#if (0 == CONFIG_SAU_RECONFIG)
	uint32_t i;
	#endif
	dlv_sau_t *sau_info = &(dlv->sau);

	if(sau_info->is_enable){
	#if CONFIG_SAU_RECONFIG
		TZ_SAU_Setup();
	#else
		for(i = 0; i < 8; i ++){
			SAU->RNR = i;
			SAU->RBAR = sau_info->rbar_val[i];
			SAU->RLAR = sau_info->rlar_val[i];
		}
		SAU->RNR = sau_info->rnr_val;
		SAU->CTRL = sau_info->ctrl_val;
	#endif
	}
}

DLV_STATIC void dlv_mpu_restore(dlv_context_t *dlv)
{
	uint32_t i;
	dlv_mpu_t *mpu_info = &(dlv->mpu);

	if(mpu_info->is_enable){
		for(i = 0; i < 16; i ++){
			MPU->RNR = i;
			MPU->RBAR = mpu_info->rbar_val[i];
			MPU->RLAR = mpu_info->rlar_val[i];
		}

		MPU->RNR = mpu_info->rnr_val;
		MPU->CTRL = mpu_info->ctrl_val;
	}
}

DLV_STATIC void dlv_fpu_restore(dlv_context_t *dlv)
{
	dlv_fpu_t *fpu_info = &(dlv->fpu);

	FPU->FPCAR = fpu_info->fpcar_val;
	FPU->FPCCR = fpu_info->fpccr_val;
	FPU->FPDSCR = fpu_info->fpdscr_val;
}

DLV_STATIC void dlv_itcm_restore(dlv_context_t *dlv)
{
	dlv_itcm_t *itcm_info = &(dlv->itcm);

	TCM->ITCMCR = itcm_info->tcmcr_val;
	ITGU->TGU_LUT = itcm_info->tgu_lut_val;
	ITGU->TGU_CTRL = itcm_info->tgu_ctrl_val;
}

DLV_STATIC void dlv_dtcm_restore(dlv_context_t *dlv)
{
	dlv_dtcm_t *dtcm_info = &(dlv->dtcm);

	TCM->DTCMCR = dtcm_info->tcmcr_val;
	DTGU->TGU_LUT = dtcm_info->tgu_lut_val;
	DTGU->TGU_CTRL = dtcm_info->tgu_ctrl_val;
}

DLV_STATIC void dlv_core_restore(dlv_context_t *dlv)
{
	dlv_core_t *core_info = &(dlv->core);

	__set_MSPLIM(core_info->msplim_val);
	__set_PSPLIM(core_info->psplim_val);
	__set_PSP(core_info->psp_val);

	__TZ_set_BASEPRI_NS(core_info->basepri_ns_val);
	__TZ_set_PRIMASK_NS(core_info->primask_ns_val);
	__TZ_set_FAULTMASK_NS(core_info->fault_mask_ns_val);
	__set_PRIMASK(core_info->primask_s_val);
	__set_FAULTMASK(core_info->fault_mask_s_val);
	__set_BASEPRI(core_info->basepri_s_val);
#if CONFIG_DUMMY_
	/*FIXME: it effects system stack:msp or psp*/
	__TZ_set_CONTROL_NS(core_info->control_ns_val);
	__set_CONTROL(core_info->control_s_val);
#endif
}

DLV_STATIC void dlv_stack_frame_restore(dlv_context_t *dlv)
{
	uint32_t xpsr_val, lr_val;
	dlv_stack_frame_t *stack_frame_info = &(dlv->stk_frame);

	xpsr_val = stack_frame_info->xpsr;
	lr_val = stack_frame_info->lr;

#if CONFIG_RESTORE_VIA_EXC_RETURN
	__asm volatile
	(
		"mrs r7, psp                       \n"
		"mov r8, #0xFFFFFFFD               \n" /*exc_return; 0xD--binary 1101, thread mode, psp, resv, secure*/
		"mov r9, %1                        \n"
		"mov r10, %0                       \n"
		"orr r10, r10, #0x01000000         \n"
		"stmdb r7!, {r8-r10}               \n"
		"stmdb r7!, {r0-r3, r12}           \n" /*this is the original caller context*/
		"msr psp, r7                       \n"
		"bx r8                             \n"
		::"r"(xpsr_val),"r"(lr_val):
	);
#else
	/* direct jump method*/
	dlv_core_t *core_info = &(dlv->core);

	__TZ_set_CONTROL_NS(core_info->control_ns_val);
	__set_CONTROL(core_info->control_s_val);
	__asm volatile
	(
		"mov r9, %1                        \n"
		"mov r10, %0                       \n"
		"orr r10, r10, #0x01000000         \n"
		"msr xPSR, r10                     \n"
		"bx r9                             \n"
		::"r"(xpsr_val),"r"(lr_val):
	);

#endif
}
/*************************************************************/
void dlv_context_save(void)
{
	dlv_context_t *dlv = &s_dlv_context;

	dlv_scb_save(dlv);
	dlv_nvic_save(dlv);
	dlv_sau_save(dlv);
	dlv_mpu_save(dlv);
	dlv_fpu_save(dlv);
	dlv_itcm_save(dlv);
	dlv_dtcm_save(dlv);
	dlv_core_save(dlv);
}

void deep_lv_enter(void)
{
	dlv_context_save();
}

void dlv_trigger_restore_context(void)
{
	__asm volatile
	(
		" .syntax unified           \n"
		" dsb                       \n"
		" isb                       \n"
		" svc %0                    \n"
		" nop                       \n"
		"                           \n"
		::"i"(portSVC_DEEP_LV_EXIT):"memory"
	);
}

void dlv_context_restore(void)
{
	dlv_context_t *dlv = &s_dlv_context;

	dlv_itcm_restore(dlv);
	dlv_dtcm_restore(dlv);
	dlv_scb_restore(dlv);
	dlv_sau_restore(dlv);
	dlv_mpu_restore(dlv);
	dlv_fpu_restore(dlv);
	dlv_core_restore(dlv);
	dlv_nvic_restore(dlv);
#if CONFIG_RESTORE_VIA_EXC_RETURN
	dlv_trigger_restore_context();
#else
	dlv_stack_frame_restore(dlv);
#endif
}

void deep_lv_exit(void)
{
	dlv_context_t *dlv = &s_dlv_context;

	dlv_stack_frame_restore(dlv);
}
// eof

