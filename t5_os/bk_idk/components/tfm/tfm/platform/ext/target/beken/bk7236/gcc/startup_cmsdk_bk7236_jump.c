;/*
; * Copyright (c) 2009-2020 ARM Limited
; *
; * Licensed under the Apache License, Version 2.0 (the "License");
; * you may not use this file except in compliance with the License.
; * You may obtain a copy of the License at
; *
; *     http://www.apache.org/licenses/LICENSE-2.0
; *
; * Unless required by applicable law or agreed to in writing, software
; * distributed under the License is distributed on an "AS IS" BASIS,
; * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; * See the License for the specific language governing permissions and
; * limitations under the License.
; *
; *
; * This file is derivative of CMSIS V5.00 startup_ARMCM33.S
; */

#include "tfm_plat_config.h"
#include "region_defs.h"
#include "os/os.h"
#include "cmsis_gcc.h"

#define ENTRY_SECTION  __attribute__((section(".fix.reset_entry")))

extern uint32_t __INITIAL_SP;
extern uint32_t __STACK_LIMIT;
extern __NO_RETURN void __PROGRAM_START(void);
extern void SystemInit (void);
extern void deep_sleep_reset(void);

typedef void(*VECTOR_TABLE_Type)(void);

__NO_RETURN void Reset_Handler  (void);

void NMI_Handler            (void) __attribute__ ((weak));
void HardFault_Handler      (void) __attribute__ ((weak));
void MemManage_Handler      (void) __attribute__ ((weak));
void BusFault_Handler       (void) __attribute__ ((weak));
void UsageFault_Handler     (void) __attribute__ ((weak));
void SecureFault_Handler    (void) __attribute__ ((weak));
void SVC_Handler            (void) __attribute__ ((weak));
void DebugMon_Handler       (void) __attribute__ ((weak));
void PendSV_Handler         (void) __attribute__ ((weak));
void SysTick_Handler        (void) __attribute__ ((weak));
void UART_InterruptHandler  (void) __attribute__ ((weak));

const VECTOR_TABLE_Type __VECTOR_TABLE[] __VECTOR_TABLE_ATTRIBUTE = {
	(VECTOR_TABLE_Type)(&__INITIAL_SP),      /*     Initial Stack Pointer */
	/* Core interrupts */
	Reset_Handler,                            /*     Reset Handler */
	NMI_Handler,                              /* -14 NMI Handler */
	HardFault_Handler,                        /* -13 Hard Fault Handler */
	MemManage_Handler,                        /* -12 MPU Fault Handler */
	BusFault_Handler,                         /* -11 Bus Fault Handler */
	UsageFault_Handler,                       /* -10 Usage Fault Handler */
	SecureFault_Handler,                      /*  -9 Secure Fault Handler */
	0,                                        /*  -8 */
	0,                                        /*  -7 */
	0,                                        /*  -6 */
	SVC_Handler,                              /*  -5 SVCall Handler */
	DebugMon_Handler,                         /*  -4 Debug Monitor Handler */
	0,                                        /*  -3 */
	PendSV_Handler,                           /*  -2 PendSV Handler */
	SysTick_Handler,                          /*  -1 SysTick Handler */
	/* External interrupts */
	0,                                        /* Interrupt 0 */
	0,                                        /* Interrupt 1 */
	0,                                        /* Interrupt 2 */
	0,                                        /* Interrupt 3 */
	0,                    					  /* Interrupt 4 */
	0,                                        /* Interrupt 5 */
	0,                                        /* Interrupt 6 */
	0,                                        /* Interrupt 7 */
	0,                                        /* Interrupt 8 */
	0,                                        /* Interrupt 9 */
	0,                                        /* Interrupt 10 */
	0,                                        /* Interrupt 11 */
	0,                                        /* Interrupt 12 */
	0,                                        /* Interrupt 13 */
	0,                                        /* Interrupt 14 */
	0,                                        /* Interrupt 15 */
	0,                                        /* Interrupt 16 */
	0,                                        /* Interrupt 17 */
	0,                                        /* Interrupt 18 */
	0,                                        /* Interrupt 19 */
	0,                                        /* Interrupt 20 */
	0,                                        /* Interrupt 21 */
	0,                                        /* Interrupt 22 */
	0,                                        /* Interrupt 23 */
	0,                                        /* Interrupt 24 */
	0,                                        /* Interrupt 25 */
	0,                                        /* Interrupt 26 */
	0,                                        /* Interrupt 27 */
	0,                                        /* Interrupt 28 */
	(void (*)(void))0x140,                    /* Default Jump BIN offset  */
	(void (*)(void))0x100,                    /* Default Jump BIN length */
	0,                                        /* Interrupt 31 */
	0,                                        /* Interrupt 32 */
	0,                                        /* Interrupt 33 */
	0,                                        /* Interrupt 34 */
	0,                                        /* Interrupt 35 */
	0,                                        /* Interrupt 36 */
	0,                                        /* Interrupt 37 */
	0,                                        /* Interrupt 38 */
	0,                                        /* Interrupt 39 */
	0,                                        /* Interrupt 40 */
	0,                                        /* Interrupt 41 */
	0,                                        /* Interrupt 42 */
	0,                                        /* Interrupt 43 */
	0,                                        /* Interrupt 44 */
	0,                                        /* Interrupt 45 */
	0,                                        /* Interrupt 46 */
	0,                                        /* Interrupt 47 */
	(void (*)(void))0x32374B42,               /* offset 0x100, magic code: BK7236 */
	(void (*)(void))0x00003633,
	/* Reserve 32bytes to protect magic code */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

__NO_RETURN ENTRY_SECTION __attribute__((naked)) void Reset_Handler(void)
{
	deep_sleep_reset();
	__set_MSPLIM((uint32_t)(&__STACK_LIMIT));
	/* CMSIS System Initialization */
	SystemInit();

	/* Enter PreMain (C library entry point) */
	__PROGRAM_START();
}
