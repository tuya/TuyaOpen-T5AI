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

#include <stdint.h>
#include "boot.h"
#include "sdkconfig.h"
#include "reset_reason.h"
#include <os/os.h>
#include "bk_arch.h"
#include "bk_rtos_debug.h"
#include <components/system.h>
#include <os/mem.h>
#include <components/log.h>
#include <common/bk_assert.h>
#include "arch_interrupt.h"
#include "stack_base.h"
#include <driver/wdt.h>
#include "wdt_driver.h"
#include <driver/wdt.h>
#include "bk_aon_wdt.h"
#include "armstar.h"
#if CONFIG_CM_BACKTRACE
#include "cm_backtrace.h"
#endif

#define MAX_DUMP_SYS_MEM_COUNT       (8)
#define SOC_DTCM_DATA_SIZE           (0x4000)
#define SOC_ITCM_DATA_SIZE           (0x4000)
#define SOC_SRAM_TOTAL_SIZE          (0xA0000)
#define SOC_SRAM_DATA_END            (SOC_SRAM0_DATA_BASE + SOC_SRAM_TOTAL_SIZE)

#if (CONFIG_SOC_BK7236_SMP_TEMP || CONFIG_SOC_BK7239_SMP_TEMP || CONFIG_SOC_BK7286_SMP_TEMP)
extern unsigned char __itcm_cpu0_end__;
extern unsigned char __dtcm_cpu0_start__;
extern unsigned char _estack_cpu0;
extern unsigned char _sstack_cpu0;

#define __dtcm_end__  __itcm_cpu0_end__
#define __dtcm_start__ __dtcm_cpu0_start__
#define _estack _estack_cpu0
#define _sstack _sstack_cpu0
#endif

typedef struct sys_mem_info
{
    uint32_t mem_base_addr;
    uint32_t mem_size;
} sys_mem_info_t;

static unsigned int s_mem_count = 0;
static sys_mem_info_t s_dump_sys_mem_info[MAX_DUMP_SYS_MEM_COUNT] = {0};

static hook_func s_wifi_dump_func = NULL;
static hook_func s_ble_dump_func = NULL;

volatile unsigned int g_enter_exception = 0;


static void rtos_dump_plat_memory(void) {
    // Dump DTCM
    stack_mem_dump((uint32_t)SOC_DTCM_DATA_BASE, (uint32_t)(SOC_DTCM_DATA_BASE + SOC_DTCM_DATA_SIZE));

    // Dump ITCM
    stack_mem_dump((uint32_t)(SOC_ITCM_DATA_BASE + 0x20) , (uint32_t)(SOC_ITCM_DATA_BASE + SOC_ITCM_DATA_SIZE));

    // Dump All SRAM
    stack_mem_dump((uint32_t)SOC_SRAM3_DATA_BASE, (uint32_t)SOC_SRAM4_DATA_BASE);
    stack_mem_dump((uint32_t)SOC_SRAM4_DATA_BASE, (uint32_t)SOC_SRAM_DATA_END);

    stack_mem_dump((uint32_t)SOC_SRAM0_DATA_BASE, (uint32_t)SOC_SRAM1_DATA_BASE);
    stack_mem_dump((uint32_t)SOC_SRAM1_DATA_BASE, (uint32_t)SOC_SRAM2_DATA_BASE);
    stack_mem_dump((uint32_t)SOC_SRAM2_DATA_BASE, (uint32_t)SOC_SRAM3_DATA_BASE);
}


unsigned int arch_is_enter_exception(void) {
    return g_enter_exception;
}

void arch_set_enter_exception(void) {
    g_enter_exception = 1;
}

void rtos_regist_wifi_dump_hook(hook_func wifi_func)
{
    s_wifi_dump_func = wifi_func;
}

void rtos_regist_ble_dump_hook(hook_func ble_func)
{
    s_ble_dump_func = ble_func;
}

void rtos_regist_plat_dump_hook(uint32_t mem_base_addr, uint32_t mem_size)
{
    if (mem_base_addr >= SOC_SRAM0_DATA_BASE
        && (mem_base_addr + mem_size) < SOC_SRAM_DATA_END) {
#if CONFIG_SPE
        //BK_DUMP_OUT("rtos_regist_plat_dump_hook memory(0x%x) in sram, need not dump again.\r\n", mem_base_addr);
#else
        //UART/Log is not ready yet!
#endif
        return;
    }

    for (int i = 0; i < s_mem_count; i++) {
        if(mem_base_addr == s_dump_sys_mem_info[i].mem_base_addr
         && mem_size == s_dump_sys_mem_info[i].mem_size) {
            BK_DUMP_OUT("rtos_regist_plat_dump_hook memory(0x%x) already register.\r\n", mem_base_addr);
            return;
         }
    }

    if (s_mem_count < MAX_DUMP_SYS_MEM_COUNT) {
        s_dump_sys_mem_info[s_mem_count].mem_base_addr = mem_base_addr;
        s_dump_sys_mem_info[s_mem_count].mem_size = mem_size;
        s_mem_count++;
    } else {
        BK_DUMP_OUT("rtos_regist_plat_dump_hook failed:s_mem_count(%d).\r\n", s_mem_count);
    }
}

void rtos_dump_plat_sys_mems(void) {
#if CONFIG_MEMDUMP_ALL
    rtos_dump_plat_memory();

    for (int i = 0; i < s_mem_count; i++) {
        uint32_t begin = s_dump_sys_mem_info[i].mem_base_addr;
        uint32_t end = begin + s_dump_sys_mem_info[i].mem_size;
        stack_mem_dump(begin, end);
    }
#endif
}

#if CONFIG_FREERTOS_SMP
#define CPU_ID     rtos_get_core_id()
#else

#if (CONFIG_MAILBOX) && (CONFIG_CPU_CNT > 1)
#include "mb_ipc_cmd.h"
#endif

#if (CONFIG_SYS_CPU0)
#define CPU_ID     0
#endif

#if (CONFIG_SYS_CPU1)
#define CPU_ID     1
#endif

#if (CONFIG_SYS_CPU2)
#define CPU_ID     2
#endif

#endif

static const char * const fault_type[] =
{
	[0]  = NULL,
	[1]  = NULL,
	[2]  = "Watchdog\r\n",
	[3]  = "HardFault\r\n",
	[4]  = "MemFault\r\n",
	[5]  = "BusFault\r\n",
	[6]  = "UsageFault\r\n",
	[7]  = "SecureFault\r\n",
	[8]  = NULL,
	[9]  = NULL,
	[10] = NULL,
	[11] = "SVC\r\n",
	[12] = "DebugFault\r\n",
	[13] = NULL,
	[14] = "PendSV\r\n",
	[15] = "SysTick\r\n",
};

#if (CONFIG_SHELL_ASYNCLOG)
extern void shell_set_log_cpu(u8 req_cpu);
#endif

static void dump_prologue(void)
{
#if CONFIG_FREERTOS_SMP

	#if (CONFIG_SHELL_ASYNCLOG)
	shell_set_log_cpu(CPU_ID);
	#endif

#else

#if (CONFIG_SYS_CPU0)
	#if (CONFIG_SHELL_ASYNCLOG)
	shell_set_log_cpu(CONFIG_CPU_CNT);
	shell_set_log_cpu(CPU_ID);
	#endif
#elif (CONFIG_SYS_CPU1)
	ipc_send_trap_handle_begin();
#endif

#endif
}

static void dump_epilogue(void)
{
#if CONFIG_FREERTOS_SMP

	#if (CONFIG_SHELL_ASYNCLOG)
	shell_set_log_cpu(CONFIG_CPU_CNT);
	#endif

#else
	
#if (CONFIG_SYS_CPU0)
	#if (CONFIG_SHELL_ASYNCLOG)
	shell_set_log_cpu(CONFIG_CPU_CNT);
	#endif
#elif (CONFIG_SYS_CPU1)
	ipc_send_trap_handle_end();
#endif

#endif
}

/**
 * this function will show registers of CPU
 *
 * @param mcause
 * @param context
 */
static void arch_dump_cpu_registers(uint32_t mcause, SAVED_CONTEXT *context) {

    BK_DUMP_OUT("CPU%d Current regs:\r\n", CPU_ID);

	// context of task.
    BK_DUMP_OUT("0 r0 x 0x%lx\r\n", context->r0);
    BK_DUMP_OUT("1 r1 x 0x%lx\r\n", context->r1);
    BK_DUMP_OUT("2 r2 x 0x%lx\r\n", context->r2);
    BK_DUMP_OUT("3 r3 x 0x%lx\r\n", context->r3);
    BK_DUMP_OUT("4 r4 x 0x%lx\r\n", context->r4);
    BK_DUMP_OUT("5 r5 x 0x%lx\r\n", context->r5);
    BK_DUMP_OUT("6 r6 x 0x%lx\r\n", context->r6);
    BK_DUMP_OUT("7 r7 x 0x%lx\r\n", context->r7);
    BK_DUMP_OUT("8 r8 x 0x%lx\r\n", context->r8);
    BK_DUMP_OUT("9 r9 x 0x%lx\r\n", context->r9);
    BK_DUMP_OUT("10 r10 x 0x%lx\r\n", context->r10);
    BK_DUMP_OUT("11 r11 x 0x%lx\r\n", context->r11);
    BK_DUMP_OUT("12 r12 x 0x%lx\r\n", context->r12);
    BK_DUMP_OUT("14 sp x 0x%lx\r\n", context->sp);
    BK_DUMP_OUT("15 lr x 0x%lx\r\n", context->lr);
    BK_DUMP_OUT("16 pc x 0x%lx\r\n", context->pc);
    BK_DUMP_OUT("17 xpsr x 0x%lx\r\n", context->xpsr);
    BK_DUMP_OUT("18 msp x 0x%lx\r\n", context->msp);
    BK_DUMP_OUT("19 psp x 0x%lx\r\n", context->psp);
    BK_DUMP_OUT("20 primask x 0x%lx\r\n", context->primask);
    BK_DUMP_OUT("21 basepri x 0x%lx\r\n", context->basepri);
    BK_DUMP_OUT("22 faultmask x 0x%lx\r\n", context->faultmask);
    BK_DUMP_OUT("23 fpscr x 0x%lx\r\n", context->fpscr);

	// context of ISR.
	mcause = __get_xPSR();
    BK_DUMP_OUT("30 CPU%d xPSR x 0x%lx\r\n", CPU_ID, mcause);
    BK_DUMP_OUT("31 LR x 0x%lx\r\n", context->control);       // exception LR.
    BK_DUMP_OUT("32 control x 0x%lx\r\n", __get_CONTROL());

    BK_DUMP_OUT("40 MMFAR x 0x%lx\r\n", SCB->MMFAR);
    BK_DUMP_OUT("41 BFAR x 0x%lx\r\n", SCB->BFAR);
    BK_DUMP_OUT("42 CFSR x 0x%lx\r\n", SCB->CFSR);
    BK_DUMP_OUT("43 HFSR x 0x%lx\r\n", SCB->HFSR);

	mcause = mcause & 0x1FF;

	if ( (mcause <= 0x0F) && (fault_type[mcause] != NULL) )
	{
		BK_DUMP_OUT((char *)fault_type[mcause]);
	}

}

static void dump_context(uint32_t lr, uint32_t msp)
{
    SAVED_CONTEXT regs;

    regs.r4 = ((uint32_t *)msp)[-8];
    regs.r5 = ((uint32_t *)msp)[-7];
    regs.r6 = ((uint32_t *)msp)[-6];
    regs.r7 = ((uint32_t *)msp)[-5];
    regs.r8 = ((uint32_t *)msp)[-4];
    regs.r9 = ((uint32_t *)msp)[-3];
    regs.r10 = ((uint32_t *)msp)[-2];
    regs.r11 = ((uint32_t *)msp)[-1];
    regs.control = lr;       // exception LR.

    regs.msp = __get_MSP();
    regs.psp = __get_PSP();

    regs.primask = __get_PRIMASK();
    regs.basepri = __get_BASEPRI();
    regs.faultmask = __get_FAULTMASK();

    regs.fpscr = __get_FPSCR();

	uint32_t stack_pointer = msp;
	uint32_t stack_adj = 8 * sizeof(uint32_t);

	if(lr & (1UL << 2))
	{
		stack_pointer = __get_PSP();
	}

	if((lr & (1UL << 4)) == 0)
	{
		stack_adj += 18 * sizeof(uint32_t);  // 18 FPU regs.
	}

	regs.r0 = ((uint32_t *)stack_pointer)[0];
	regs.r1 = ((uint32_t *)stack_pointer)[1];
	regs.r2 = ((uint32_t *)stack_pointer)[2];
	regs.r3 = ((uint32_t *)stack_pointer)[3];
	regs.r12 = ((uint32_t *)stack_pointer)[4];
	regs.lr = ((uint32_t *)stack_pointer)[5];
	regs.pc = ((uint32_t *)stack_pointer)[6];
	regs.xpsr = ((uint32_t *)stack_pointer)[7];

	if(regs.xpsr & (1UL << 9))
	{
		stack_adj += 1 * sizeof(uint32_t);
	}
	regs.sp = stack_pointer + stack_adj;

	rtos_disable_int();
    bk_wdt_feed();
#if (CONFIG_INT_AON_WDT)
    bk_int_aon_wdt_feed();
#endif
    bk_set_printf_sync(true);
	dump_prologue();
    arch_dump_cpu_registers(0, &regs);
}

static void rtos_dump_system(void)
{
#if CONFIG_DEBUG_FIRMWARE || CONFIG_DUMP_ENABLE
    BK_LOG_FLUSH();
    bk_set_printf_sync(true);

    BK_DUMP_OUT("***********************************************************************************************\r\n");
    BK_DUMP_OUT("***********************************user except handler begin***********************************\r\n");
    BK_DUMP_OUT("***********************************************************************************************\r\n");

    bk_wdt_feed();
#if (CONFIG_INT_AON_WDT)
    bk_int_aon_wdt_feed();
#endif

    if(NULL != s_wifi_dump_func) {
        s_wifi_dump_func();
    }

    if(NULL != s_ble_dump_func) {
        s_ble_dump_func();
    }

    rtos_dump_plat_sys_mems();

#if CONFIG_FREERTOS && CONFIG_MEM_DEBUG
    os_dump_memory_stats(0, 0, NULL);
#endif

    rtos_dump_backtrace();
    rtos_dump_task_list();
#if CONFIG_FREERTOS
    rtos_dump_task_runtime_stats();
#endif

    BK_DUMP_OUT("***********************************************************************************************\r\n");
    BK_DUMP_OUT("************************************user except handler end************************************\r\n");
    BK_DUMP_OUT("***********************************************************************************************\r\n");
#endif //CONFIG_DEBUG_FIRMWARE || CONFIG_DUMP_ENABLE
}

#define CHECK_TASK_WDT_INTERRUPT (0x13)

uint32_t     g_wdt_handler_lr;

static void user_except_handler(uint32_t reset_reason, SAVED_CONTEXT *regs)
{
    if (0 == g_enter_exception) {
        // Make sure the interrupt is disable
        uint32_t int_level = rtos_disable_int();

        /* Handled Trap */
        g_enter_exception = 1;

        // if it is a task WDT assert!
        if((regs->xpsr & 0x1FF) == CHECK_TASK_WDT_INTERRUPT)  // it can be used for any interrupts if LR is saved at entrance of ISR.
        {
            if(g_wdt_handler_lr & (1UL << 2))
            {
                uint32_t stack_pointer = __get_PSP();
                uint32_t stack_adj = 8 * sizeof(uint32_t);

                if((g_wdt_handler_lr & (1UL << 4)) == 0)
                {
                    stack_adj += 18 * sizeof(uint32_t);  // 18 FPU regs.
                }

                regs->r0 = ((uint32_t *)stack_pointer)[0];
                regs->r1 = ((uint32_t *)stack_pointer)[1];
                regs->r2 = ((uint32_t *)stack_pointer)[2];
                regs->r3 = ((uint32_t *)stack_pointer)[3];
                regs->r12 = ((uint32_t *)stack_pointer)[4];
                regs->lr = ((uint32_t *)stack_pointer)[5];
                regs->pc = ((uint32_t *)stack_pointer)[6];
                regs->xpsr = ((uint32_t *)stack_pointer)[7];

                if(regs->xpsr & (1UL << 9))
                {
                    stack_adj += 1 * sizeof(uint32_t);
                }
                regs->sp = stack_pointer + stack_adj;
            }
        }

        bk_set_printf_sync(true);
        dump_prologue();
        arch_dump_cpu_registers(ECAUSE_ASSERT, regs);

        rtos_dump_system();
        dump_epilogue();

#if CONFIG_SYS_CPU0
        bk_reboot_ex(reset_reason);
#endif
        while(g_enter_exception);

        rtos_enable_int(int_level);
    } else {
		dump_epilogue();
#if CONFIG_SYS_CPU0
        bk_wdt_force_reboot();
#endif
    }

}

///1. Save to stack is better
///2. Some regs already saved in stack
static void store_cpu_regs(uint32_t mcause, SAVED_CONTEXT *regs) {
    regs->r0 = __get_R0();
    regs->r1 = __get_R1();
    regs->r2 = __get_R2();
    regs->r3 = __get_R3();
    regs->r4 = __get_R4();
    regs->r5 = __get_R5();
    regs->r6 = __get_R6();
    regs->r7 = __get_R7();
    regs->r8 = __get_R8();
    regs->r9 = __get_R9();
    regs->r10 = __get_R10();
    regs->r11 = __get_R11();
    regs->r12 = __get_R12();
    regs->sp = __get_SP();
    regs->lr = __get_LR();
    regs->pc = __get_PC();

    regs->xpsr = __get_xPSR();
    regs->msp = __get_MSP();
    regs->psp = __get_PSP();

    regs->primask = __get_PRIMASK();
    regs->basepri = __get_BASEPRI();
    regs->faultmask = __get_FAULTMASK();

    regs->control = __get_CONTROL();
    regs->fpscr = __get_FPSCR();
}

extern void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id);
extern volatile const uint8_t build_version[];

void bk_system_dump(void) 
{
    SAVED_CONTEXT regs = {0};
    store_cpu_regs(ECAUSE_ASSERT, &regs);
    uint32_t int_level = rtos_disable_int();

    bk_wdt_feed();
#if (CONFIG_INT_AON_WDT)
    bk_int_aon_wdt_feed();
#endif

    BK_DUMP_OUT("build time => %s !\r\n", build_version);

    user_except_handler(RESET_SOURCE_CRASH_ASSERT, &regs);

    bk_set_jtag_mode(CPU_ID, 0);
    rtos_enable_int(int_level);
}

void user_except_handler_ex(uint32_t reset_reason, uint32_t lr, uint32_t sp)
{
    if (0 == g_enter_exception) {
        dump_context(lr, sp);
        // Make sure the interrupt is disable
        uint32_t int_level = rtos_disable_int();

        /* Handled Trap */
        g_enter_exception = 1;

        BK_DUMP_OUT("build time => %s !\r\n", build_version); 

        rtos_dump_system();

#if CONFIG_CM_BACKTRACE
        cm_backtrace_fault(lr, sp);
#endif

        dump_epilogue();

#if CONFIG_SYS_CPU0
        bk_reboot_ex(reset_reason);
#endif
        while(g_enter_exception);

        rtos_enable_int(int_level);
    } else {
		dump_epilogue();
#if CONFIG_SYS_CPU0
        bk_misc_set_reset_reason(reset_reason);
        bk_wdt_force_reboot();
#endif
    }

}

__attribute__((used, section(".null_trap_handler"))) \
void bk_null_trap_handler(void) {
    BK_ASSERT(0);
}
