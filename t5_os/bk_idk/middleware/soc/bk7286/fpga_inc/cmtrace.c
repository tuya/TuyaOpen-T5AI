#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cmtrace.h"

#ifndef FIRMWARE_NAME
#define FIRMWARE_NAME "cmtrace"
#endif

int bk_printf(const char *fmt, ...);

#define cmt_println(...)            { bk_printf(__VA_ARGS__); bk_printf("\r\n"); }
#define CMT_CPU_TYPE                CMT_CPU_CM33
#define CMT_OS_TYPE                 CMT_OS_NONE
#define CMT_DUMP_STACK_INFO_ENABLE  1

#define CMT_CPU_CM0                 0
#define CMT_CPU_CM3                 3
#define CMT_CPU_CM4                 4
#define CMT_CPU_CM7                 7
#define CMT_CPU_CM33                33

#define CMT_OS_NONE                 0
#define CMT_OS_RTT                  1
#define CMT_OS_UCOSII               2
#define CMT_OS_UCOSIII              3
#define CMT_OS_FREERTOS             4
#define CMT_OS_RTX5                 5

#if defined(__ARMCC_VERSION)
    #define CMT_CSTACK_BLOCK_NAME   ARM_LIB_STACK
    #define CMT_CODE_SECTION_NAME   ER_ROM
#elif defined(__ICCARM__)
    #define CMT_CSTACK_BLOCK_NAME   "CSTACK"
    #define CMT_CODE_SECTION_NAME   ".text"
#elif defined(__GNUC__)
    #define CMT_CSTACK_BLOCK_START  __StackLimit//_sstack
    #define CMT_CSTACK_BLOCK_END    __StackTop//_estack
    #define CMT_CODE_SECTION_START  _stext
    #define CMT_CODE_SECTION_END    _etext
#else
    #error "not supported compiler"
#endif

/* supported function call stack max depth, default is 16 */
#ifndef CMT_CALL_STACK_MAX_DEPTH
#define CMT_CALL_STACK_MAX_DEPTH    16
#endif

#define CMT_SYSHND_CTRL             (*(volatile unsigned int*)  (0xE000ED24u)) /* system handler control and state register */
#define CMT_NVIC_MFSR               (*(volatile unsigned char*) (0xE000ED28u)) /* memory management fault status register */
#define CMT_NVIC_BFSR               (*(volatile unsigned char*) (0xE000ED29u)) /* bus fault status register */
#define CMT_NVIC_UFSR               (*(volatile unsigned short*)(0xE000ED2Au)) /* usage fault status register */
#define CMT_NVIC_HFSR               (*(volatile unsigned int*)  (0xE000ED2Cu)) /* hard fault status register */
#define CMT_NVIC_DFSR               (*(volatile unsigned short*)(0xE000ED30u)) /* debug fault status register */
#define CMT_NVIC_MMAR               (*(volatile unsigned int*)  (0xE000ED34u)) /* memory management fault address register */
#define CMT_NVIC_BFAR               (*(volatile unsigned int*)  (0xE000ED38u)) /* bus fault manage address register */
#define CMT_NVIC_AFSR               (*(volatile unsigned short*)(0xE000ED3Cu)) /* auxiliary fault status register */

struct cmt_hard_fault_regs
{
    struct
    {
        unsigned int r0;  // Register R0
        unsigned int r1;  // Register R1
        unsigned int r2;  // Register R2
        unsigned int r3;  // Register R3
        unsigned int r12; // Register R12
        unsigned int lr;  // Link register
        unsigned int pc;  // Program counter
        union
        {
            unsigned int value;
            struct
            {
                #if(CMT_CPU_TYPE == CMT_CPU_CM33)
                unsigned int IPSR : 9;  // Interrupt Program Status register (IPSR)
                unsigned int EPSR : 18; // Execution Program Status register (EPSR)
                unsigned int APSR : 5;  // Application Program Status register (APSR)
                #else
                unsigned int IPSR : 8;  // Interrupt Program Status register (IPSR)
                unsigned int EPSR : 19; // Execution Program Status register (EPSR)
                unsigned int APSR : 5;  // Application Program Status register (APSR)
                #endif
            } bits;
        } psr; // Program status register.
    } saved;

    union
    {
        unsigned int value;
        struct
        {
            unsigned int MEMFAULTACT    : 1; // Read as 1 if memory management fault is active
            unsigned int BUSFAULTACT    : 1; // Read as 1 if bus fault exception is active
            #if(CMT_CPU_TYPE == CMT_CPU_CM33)
            unsigned int HARDFAULTACT   : 1; // Read as 1 if hardfault is active
            #else
            unsigned int UnusedBits1    : 1;
            #endif
            unsigned int USGFAULTACT    : 1; // Read as 1 if usage fault exception is active
            #if(CMT_CPU_TYPE == CMT_CPU_CM33)
            unsigned int SECUREFAULTACT : 1; // Read as 1 if secure fault exception is active
            unsigned int NMIACT         : 1; // Read as 1 if NMI exception is active
            unsigned int UnusedBits2    : 1;
            #else
            unsigned int UnusedBits2    : 3;
            #endif
            unsigned int SVCALLACT      : 1; // Read as 1 if SVC exception is active
            unsigned int MONITORACT     : 1; // Read as 1 if debug monitor exception is active
            unsigned int UnusedBits3    : 1;
            unsigned int PENDSVACT      : 1; // Read as 1 if PendSV exception is active
            unsigned int SYSTICKACT     : 1; // Read as 1 if SYSTICK exception is active
            unsigned int USGFAULTPENDED : 1; // Usage fault pended; usage fault started but was replaced by a higher-priority exception
            unsigned int MEMFAULTPENDED : 1; // Memory management fault pended; memory management fault started but was replaced by a higher-priority exception
            unsigned int BUSFAULTPENDED : 1; // Bus fault pended; bus fault handler was started but was replaced by a higher-priority exception
            unsigned int SVCALLPENDED   : 1; // SVC pended; SVC was started but was replaced by a higher-priority exception
            unsigned int MEMFAULTENA    : 1; // Memory management fault handler enable
            unsigned int BUSFAULTENA    : 1; // Bus fault handler enable
            unsigned int USGFAULTENA    : 1; // Usage fault handler enable
            #if(CMT_CPU_TYPE == CMT_CPU_CM33)
            unsigned int SECUREFAULTENA : 1;   // Secure fault handler enable
            unsigned int SECUREFAULTPENDED : 1;// Secure fault pended; Secure fault handler was started but was replaced by a higher-priority exception
            unsigned int HARDFAULTPENDED   : 1;// Hard fault pended; Hard fault handler was started but was replaced by a higher-priority exception
            #else
            // None
            #endif
        } bits;
    } syshndctrl; // System Handler Control and State Register (0xE000ED24)

    union
    {
        unsigned char value;
        struct
        {
            unsigned char IACCVIOL    : 1; // Instruction access violation
            unsigned char DACCVIOL    : 1; // Data access violation
            unsigned char UnusedBits  : 1;
            unsigned char MUNSTKERR   : 1; // Unstacking error
            unsigned char MSTKERR     : 1; // Stacking error
            unsigned char MLSPERR     : 1; // Floating-point lazy state preservation (M4/M7)
            unsigned char UnusedBits2 : 1;
            unsigned char MMARVALID   : 1; // Indicates the MMAR is valid
        } bits;
    } mfsr;            // Memory Management Fault Status Register (0xE000ED28)

    unsigned int mmar; // Memory Management Fault Address Register (0xE000ED34)

    union
    {
        unsigned char value;
        struct
        {
            unsigned char IBUSERR    : 1; // Instruction access violation
            unsigned char PRECISERR  : 1; // Precise data access violation
            unsigned char IMPREISERR : 1; // Imprecise data access violation
            unsigned char UNSTKERR   : 1; // Unstacking error
            unsigned char STKERR     : 1; // Stacking error
            unsigned char LSPERR     : 1; // Floating-point lazy state preservation (M4/M7)
            unsigned char UnusedBits : 1;
            unsigned char BFARVALID  : 1; // Indicates BFAR is valid
        } bits;
    } bfsr;            // Bus Fault Status Register (0xE000ED29)
    unsigned int bfar; // Bus Fault Manage Address Register (0xE000ED38)

    union
    {
        unsigned short value;
        struct
        {
            unsigned short UNDEFINSTR : 1; // Attempts to execute an undefined instruction
            unsigned short INVSTATE   : 1; // Attempts to switch to an invalid state (e.g., ARM)
            unsigned short INVPC      : 1; // Attempts to do an exception with a bad value in the EXC_RETURN number
            unsigned short NOCP       : 1; // Attempts to execute a coprocessor instruction
            #if(CMT_CPU_TYPE == CMT_CPU_CM33)
            unsigned short STKOF      : 1; // Indicates a stack overflow error has occured
            unsigned short UnusedBits : 3;
            #else
            unsigned short UnusedBits : 4;
            #endif
            unsigned short UNALIGNED  : 1; // Indicates that an unaligned access fault has taken place
            unsigned short DIVBYZERO0 : 1; // Indicates a divide by zero has taken place (can be set only if DIV_0_TRP is set)
        } bits;
    } ufsr; // Usage Fault Status Register (0xE000ED2A)

    union
    {
        unsigned int value;
        struct
        {
            unsigned int UnusedBits  : 1;
            unsigned int VECTBL      : 1; // Indicates hard fault is caused by failed vector fetch
            unsigned int UnusedBits2 : 28;
            unsigned int FORCED      : 1; // Indicates hard fault is taken because of bus fault/memory management fault/usage fault
            unsigned int DEBUGEVT    : 1; // Indicates hard fault is triggered by debug event
        } bits;
    } hfsr; // Hard Fault Status Register (0xE000ED2C)

    union
    {
        unsigned int value;
        struct
        {
            unsigned int HALTED   : 1; // Halt requested in NVIC
            unsigned int BKPT     : 1; // BKPT instruction executed
            unsigned int DWTTRAP  : 1; // DWT match occurred
            unsigned int VCATCH   : 1; // Vector fetch occurred
            unsigned int EXTERNAL : 1; // EDBGRQ signal asserted
        } bits;
    } dfsr;            // Debug Fault Status Register (0xE000ED30)

    unsigned int afsr; // Auxiliary Fault Status Register (0xE000ED3C), Vendor controlled (optional)
};

#define CMT_ASSERT(EXPR)                                                \
if(!(EXPR))                                                             \
{                                                                       \
    cmt_println("(%s) has assert failed at %s.", #EXPR, __FUNCTION__);  \
    while (1);                                                          \
}

/* ELF(Executable and Linking Format) file extension name for each compiler */
#if defined(__ARMCC_VERSION)
    #define CMT_ELF_FILE_EXTENSION_NAME          ".axf"
#elif defined(__ICCARM__)
    #define CMT_ELF_FILE_EXTENSION_NAME          ".out"
#elif defined(__GNUC__)
    #define CMT_ELF_FILE_EXTENSION_NAME          ".elf"
#else
    #error "not supported compiler"
#endif

#ifndef cmt_println
    #error "cmt_println isn't defined"
#endif

#ifndef CMT_CPU_TYPE
    #error "CMT_CPU_TYPE isn't defined"
#endif

#ifndef CMT_OS_TYPE
    #error "CMT_OS_TYPE isn't defined"
#endif

#if   (CMT_OS_TYPE == CMT_OS_NONE)
#elif (CMT_OS_TYPE == CMT_OS_RTT)
    #include <rtthread.h>
#elif (CMT_OS_TYPE == CMT_OS_UCOSII)
    #include <ucos_ii.h>
#elif (CMT_OS_TYPE == CMT_OS_UCOSIII)
    #include <os.h>
#elif (CMT_OS_TYPE == CMT_OS_FREERTOS)
    #include <FreeRTOS.h>  
    extern uint32_t*vTaskStackAddr(void);/* need to modify the FreeRTOS/tasks source code */
    extern uint32_t vTaskStackSize(void);
    extern char*    vTaskName(void);
#elif (CMT_OS_TYPE == CMT_OS_RTX5)
    #include "rtx_os.h"
#else
    #error "not supported OS type"
#endif

/* include or export for supported cmt_get_msp, cmt_get_psp and cmt_get_sp function */
#if defined(__CC_ARM)
    static __inline __asm uint32_t cmt_get_msp(void)
    {
        mrs r0, msp
        bx lr
    }
    static __inline __asm uint32_t cmt_get_psp(void)
    {
        mrs r0, psp
        bx lr
    }
    static __inline __asm uint32_t cmt_get_sp(void)
    {
        mov r0, sp
        bx lr
    }
#elif defined(__clang__)
    __attribute__( (always_inline) ) static __inline uint32_t cmt_get_msp(void)
    {
        uint32_t result;
        __asm volatile ("mrs %0, msp" : "=r" (result) );
        return (result);
    }
    __attribute__( (always_inline) ) static __inline uint32_t cmt_get_psp(void)
    {
        uint32_t result;
        __asm volatile ("mrs %0, psp" : "=r" (result) );
        return (result);
    }
    __attribute__( (always_inline) ) static __inline uint32_t cmt_get_sp(void)
    {
        uint32_t result;
        __asm volatile ("mov %0, sp" : "=r" (result) );
        return (result);
    }
#elif defined(__ICCARM__)
/* IAR iccarm specific functions */
/* Close Raw Asm Code Warning */  
#pragma diag_suppress=Pe940    
    static uint32_t cmt_get_msp(void)
    {
      __asm("mrs r0, msp");
      __asm("bx lr");        
    }
    static uint32_t cmt_get_psp(void)
    {
      __asm("mrs r0, psp");
      __asm("bx lr");        
    }
    static uint32_t cmt_get_sp(void)
    {
      __asm("mov r0, sp");
      __asm("bx lr");       
    }
#pragma diag_default=Pe940  
#elif defined(__GNUC__)
    __attribute__( ( always_inline ) ) static inline uint32_t cmt_get_msp(void)
    {
        register uint32_t result;
        __asm volatile ("MRS %0, msp\n" : "=r" (result) );
        return(result);
    }
    __attribute__( ( always_inline ) ) static inline uint32_t cmt_get_psp(void)
    {
        register uint32_t result;
        __asm volatile ("MRS %0, psp\n" : "=r" (result) );
        return(result);
    }
    __attribute__( ( always_inline ) ) static inline uint32_t cmt_get_sp(void)
    {
        register uint32_t result;
        __asm volatile ("MOV %0, sp\n" : "=r" (result) );
        return(result);
    }
#else
    #error "not supported compiler"
#endif

#if __STDC_VERSION__ < 199901L
    #error "must be C99 or higher. try to add '-std=c99' to compile parameters"
#endif

#if defined(__ARMCC_VERSION)
    #define SECTION_START(_name_)                Image$$##_name_##$$ZI$$Base
    #define SECTION_END(_name_)                  Image$$##_name_##$$ZI$$Limit
    #define IMAGE_SECTION_START(_name_)          Image$$##_name_##$$Base
    #define IMAGE_SECTION_END(_name_)            Image$$##_name_##$$Limit
    #define CSTACK_BLOCK_START(_name_)           SECTION_START(_name_)
    #define CSTACK_BLOCK_END(_name_)             SECTION_END(_name_)
    #define CODE_SECTION_START(_name_)           IMAGE_SECTION_START(_name_)
    #define CODE_SECTION_END(_name_)             IMAGE_SECTION_END(_name_)

    extern const int CSTACK_BLOCK_START(CMT_CSTACK_BLOCK_NAME);
    extern const int CSTACK_BLOCK_END(CMT_CSTACK_BLOCK_NAME);
    extern const int CODE_SECTION_START(CMT_CODE_SECTION_NAME);
    extern const int CODE_SECTION_END(CMT_CODE_SECTION_NAME);
#elif defined(__ICCARM__)
    #pragma section=CMT_CSTACK_BLOCK_NAME
    #pragma section=CMT_CODE_SECTION_NAME
#elif defined(__GNUC__)
    extern const int CMT_CSTACK_BLOCK_START;
    extern const int CMT_CSTACK_BLOCK_END;
    extern const int CMT_CODE_SECTION_START;
    extern const int CMT_CODE_SECTION_END;
#else
    #error "not supported compiler"
#endif

enum
{
    PRINT_MAIN_STACK_CFG_ERROR,
    PRINT_FIRMWARE_INFO,
    PRINT_ASSERT_ON_THREAD,
    PRINT_ASSERT_ON_HANDLER,
    PRINT_THREAD_STACK_INFO,
    PRINT_MAIN_STACK_INFO,
    PRINT_THREAD_STACK_OVERFLOW,
    PRINT_MAIN_STACK_OVERFLOW,
    PRINT_CALL_STACK_INFO,
    PRINT_CALL_STACK_ERR,
    PRINT_FAULT_ON_THREAD,
    PRINT_FAULT_ON_HANDLER,
    PRINT_REGS_TITLE,
    PRINT_HFSR_VECTBL,
    PRINT_MFSR_IACCVIOL,
    PRINT_MFSR_DACCVIOL,
    PRINT_MFSR_MUNSTKERR,
    PRINT_MFSR_MSTKERR,
    PRINT_MFSR_MLSPERR,
    PRINT_BFSR_IBUSERR,
    PRINT_BFSR_PRECISERR,
    PRINT_BFSR_IMPREISERR,
    PRINT_BFSR_UNSTKERR,
    PRINT_BFSR_STKERR,
    PRINT_BFSR_LSPERR,
    PRINT_UFSR_UNDEFINSTR,
    PRINT_UFSR_INVSTATE,
    PRINT_UFSR_INVPC,
    PRINT_UFSR_NOCP,
    #if(CMT_CPU_TYPE == CMT_CPU_CM33)
    PRINT_UFSR_STKOF,
    #endif
    PRINT_UFSR_UNALIGNED,
    PRINT_UFSR_DIVBYZERO0,
    PRINT_DFSR_HALTED,
    PRINT_DFSR_BKPT,
    PRINT_DFSR_DWTTRAP,
    PRINT_DFSR_VCATCH,
    PRINT_DFSR_EXTERNAL,
    PRINT_MMAR,
    PRINT_BFAR,
};

static const char * const print_info[] =
{
    [PRINT_MAIN_STACK_CFG_ERROR]  = "ERROR: Unable to get the main stack information, please check the configuration of the main stack",
    [PRINT_FIRMWARE_INFO]         = "Firmware name: %s, hardware version: %s, software version: %s",
    [PRINT_ASSERT_ON_THREAD]      = "Assert on thread %s",
    [PRINT_ASSERT_ON_HANDLER]     = "Assert on interrupt or bare metal(no OS) environment",
    [PRINT_THREAD_STACK_INFO]     = "===== Thread stack information =====",
    [PRINT_MAIN_STACK_INFO]       = "====== Main stack information ======",
    [PRINT_THREAD_STACK_OVERFLOW] = "Error: Thread stack(%08x) was overflow",
    [PRINT_MAIN_STACK_OVERFLOW]   = "Error: Main stack(%08x) was overflow",
    [PRINT_CALL_STACK_INFO]       = "Show more call stack info by run: addr2line -e %s%s -a -f %s",
    [PRINT_CALL_STACK_ERR]        = "Dump call stack has an error",
    [PRINT_FAULT_ON_THREAD]       = "Fault on thread %s",
    [PRINT_FAULT_ON_HANDLER]      = "Fault on interrupt or bare metal(no OS) environment",
    [PRINT_REGS_TITLE]            = "=================== Registers information ====================",
    [PRINT_HFSR_VECTBL]           = "Hard fault is caused by failed vector fetch",
    [PRINT_MFSR_IACCVIOL]         = "Memory management fault is caused by instruction access violation",
    [PRINT_MFSR_DACCVIOL]         = "Memory management fault is caused by data access violation",
    [PRINT_MFSR_MUNSTKERR]        = "Memory management fault is caused by unstacking error",
    [PRINT_MFSR_MSTKERR]          = "Memory management fault is caused by stacking error",
    [PRINT_MFSR_MLSPERR]          = "Memory management fault is caused by floating-point lazy state preservation",
    [PRINT_BFSR_IBUSERR]          = "Bus fault is caused by instruction access violation",
    [PRINT_BFSR_PRECISERR]        = "Bus fault is caused by precise data access violation",
    [PRINT_BFSR_IMPREISERR]       = "Bus fault is caused by imprecise data access violation",
    [PRINT_BFSR_UNSTKERR]         = "Bus fault is caused by unstacking error",
    [PRINT_BFSR_STKERR]           = "Bus fault is caused by stacking error",
    [PRINT_BFSR_LSPERR]           = "Bus fault is caused by floating-point lazy state preservation",
    [PRINT_UFSR_UNDEFINSTR]       = "Usage fault is caused by attempts to execute an undefined instruction",
    [PRINT_UFSR_INVSTATE]         = "Usage fault is caused by attempts to switch to an invalid state (e.g., ARM)",
    [PRINT_UFSR_INVPC]            = "Usage fault is caused by attempts to do an exception with a bad value in the EXC_RETURN number",
    [PRINT_UFSR_NOCP]             = "Usage fault is caused by attempts to execute a coprocessor instruction",
    #if(CMT_CPU_TYPE == CMT_CPU_CM33)
    [PRINT_UFSR_STKOF]            = "Usage fault is caused by indicates that a stack overflow (hardware check) has taken place",
    #endif
    [PRINT_UFSR_UNALIGNED]        = "Usage fault is caused by indicates that an unaligned access fault has taken place",
    [PRINT_UFSR_DIVBYZERO0]       = "Usage fault is caused by Indicates a divide by zero has taken place (can be set only if DIV_0_TRP is set)",
    [PRINT_DFSR_HALTED]           = "Debug fault is caused by halt requested in NVIC",
    [PRINT_DFSR_BKPT]             = "Debug fault is caused by BKPT instruction executed",
    [PRINT_DFSR_DWTTRAP]          = "Debug fault is caused by DWT match occurred",
    [PRINT_DFSR_VCATCH]           = "Debug fault is caused by Vector fetch occurred",
    [PRINT_DFSR_EXTERNAL]         = "Debug fault is caused by EDBGRQ signal asserted",
    [PRINT_MMAR]                  = "The memory management fault occurred address is %08x",
    [PRINT_BFAR]                  = "The bus fault occurred address is %08x",
};

static uint32_t code_size = 0;
static uint32_t code_start_addr = 0;
static uint32_t main_stack_size = 0;
static uint32_t main_stack_start_addr = 0;
static bool init_ok = false;
static bool on_fault = false;
static bool stack_is_overflow = false;
static bool on_thread_before_fault = false;
static char call_stack_info[CMT_CALL_STACK_MAX_DEPTH * (8 + 1)] = { 0 };
static struct cmt_hard_fault_regs regs;

void cmtrace_init(void)
{
#if defined(__ARMCC_VERSION)
    main_stack_start_addr = (uint32_t)&CSTACK_BLOCK_START(CMT_CSTACK_BLOCK_NAME);
    main_stack_size = (uint32_t)&CSTACK_BLOCK_END(CMT_CSTACK_BLOCK_NAME) - main_stack_start_addr;
    code_start_addr = (uint32_t)&CODE_SECTION_START(CMT_CODE_SECTION_NAME);
    code_size = (uint32_t)&CODE_SECTION_END(CMT_CODE_SECTION_NAME) - code_start_addr;
#elif defined(__ICCARM__)
    main_stack_start_addr = (uint32_t)__section_begin(CMT_CSTACK_BLOCK_NAME);
    main_stack_size = (uint32_t)__section_end(CMT_CSTACK_BLOCK_NAME) - main_stack_start_addr;
    code_start_addr = (uint32_t)__section_begin(CMT_CODE_SECTION_NAME);
    code_size = (uint32_t)__section_end(CMT_CODE_SECTION_NAME) - code_start_addr;
#elif defined(__GNUC__)
    main_stack_start_addr = (uint32_t)(&CMT_CSTACK_BLOCK_START);
    main_stack_size = (uint32_t)(&CMT_CSTACK_BLOCK_END) - main_stack_start_addr;
    code_start_addr = (uint32_t)(&CMT_CODE_SECTION_START);
    code_size = (uint32_t)(&CMT_CODE_SECTION_END) - code_start_addr;
#else
    #error "not supported compiler"
#endif

    if(main_stack_size == 0)
    {
        cmt_println(print_info[PRINT_MAIN_STACK_CFG_ERROR]);
        return;
    }

    init_ok = true;
}

#if CMT_OS_TYPE
/**
 * @brief Get current thread stack information
 * @param sp stack current pointer
 * @param start_addr stack start address
 * @param size stack size
 */
static void get_cur_thread_stack_info(uint32_t sp, uint32_t* start_addr, uint32_t* size)
{
    CMT_ASSERT(start_addr);
    CMT_ASSERT(size);

#if(CMT_OS_TYPE == CMT_OS_RTT)
    *start_addr = (uint32_t) rt_thread_self()->stack_addr;
    *size = rt_thread_self()->stack_size;
#elif (CMT_OS_TYPE == CMT_OS_UCOSII)
    extern OS_TCB *OSTCBCur;
    *start_addr = (uint32_t) OSTCBCur->OSTCBStkBottom;
    *size = OSTCBCur->OSTCBStkSize * sizeof(OS_STK);
#elif (CMT_OS_TYPE == CMT_OS_UCOSIII)
    extern OS_TCB *OSTCBCurPtr;  
    *start_addr = (uint32_t) OSTCBCurPtr->StkBasePtr;
    *size = OSTCBCurPtr->StkSize * sizeof(CPU_STK_SIZE);
#elif (CMT_OS_TYPE == CMT_OS_FREERTOS)   
    *start_addr = (uint32_t)vTaskStackAddr();
    *size = vTaskStackSize() * sizeof( StackType_t );
#elif (CMT_OS_TYPE == CMT_OS_RTX5)
    osRtxThread_t *thread = osRtxInfo.thread.run.curr;
    *start_addr = (uint32_t)thread->stack_mem;
    *size = thread->stack_size;
#endif
}

static const char* get_cur_thread_name(void)
{
#if(CMT_OS_TYPE == CMT_OS_RTT)
    return rt_thread_self()->name;
#elif (CMT_OS_TYPE == CMT_OS_UCOSII)
    extern OS_TCB* OSTCBCur;

    #if OS_TASK_NAME_SIZE > 0 || OS_TASK_NAME_EN > 0
    return (const char*)OSTCBCur->OSTCBTaskName;
    #else
    return NULL;
    #endif /* OS_TASK_NAME_SIZE > 0 || OS_TASK_NAME_EN > 0 */
#elif (CMT_OS_TYPE == CMT_OS_UCOSIII)
    extern OS_TCB* OSTCBCurPtr;    
    return (const char*)OSTCBCurPtr->NamePtr;
#elif (CMT_OS_TYPE == CMT_OS_FREERTOS)
    return vTaskName();
#elif (CMT_OS_TYPE == CMT_OS_RTX5)
    osThreadId_t id = osThreadGetId();
    return osThreadGetName(id);
#endif
}

#endif

#if CMT_DUMP_STACK_INFO_ENABLE
static void dump_stack(uint32_t stack_start_addr, uint32_t stack_size, uint32_t *stack_pointer)
{
    if(stack_is_overflow)
    {
        cmt_println(print_info[on_thread_before_fault ? PRINT_THREAD_STACK_OVERFLOW : PRINT_MAIN_STACK_OVERFLOW], stack_pointer);

        if((uint32_t)stack_pointer < stack_start_addr)
        {
            stack_pointer = (uint32_t *) stack_start_addr;
        }
        else if((uint32_t) stack_pointer > stack_start_addr + stack_size)
        {
            stack_pointer = (uint32_t *) (stack_start_addr + stack_size);
        }
    }

    cmt_println(print_info[PRINT_THREAD_STACK_INFO]);

    for(; (uint32_t) stack_pointer < stack_start_addr + stack_size; stack_pointer++)
    {
        cmt_println("  addr: %08x    data: %08x", stack_pointer, *stack_pointer);
    }

    cmt_println("====================================");
}
#endif

/* check the disassembly instruction is 'BL' or 'BLX' */
static bool disassembly_ins_is_bl_blx(uint32_t addr)
{
    #define BL_INS_MASK     0xF800
    #define BL_INS_HIGH     0xF800
    #define BL_INS_LOW      0xF000
    #define BLX_INX_MASK    0xFF00
    #define BLX_INX         0x4700

    uint16_t ins1 = *((uint16_t *)addr);
    uint16_t ins2 = *((uint16_t *)(addr + 2));

    if((ins2 & BL_INS_MASK) == BL_INS_HIGH && (ins1 & BL_INS_MASK) == BL_INS_LOW)
    {
        return true;
    }
    else if((ins2 & BLX_INX_MASK) == BLX_INX)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief backtrace function call stack
 * @param buffer call stack buffer
 * @param size buffer size
 * @param sp stack pointer
 * @return depth
 */
static uint32_t call_stack(uint32_t *buffer, uint32_t size, uint32_t sp)
{
    uint32_t stack_start_addr = main_stack_start_addr, pc;
    uint32_t depth = 0, stack_size = main_stack_size;
    bool regs_saved_lr_is_valid = false;

    if(on_fault)
    {
        if(!stack_is_overflow) {
            /* first depth is PC */
            buffer[depth++] = regs.saved.pc;
            /* fix the LR address in thumb mode */
            pc = regs.saved.lr - 1;
            if((pc >= code_start_addr) && (pc <= code_start_addr + code_size) && (depth < CMT_CALL_STACK_MAX_DEPTH)
                    && (depth < size)) {
                buffer[depth++] = pc;
                regs_saved_lr_is_valid = true;
            }
        }

    #if CMT_OS_TYPE
        /* program is running on thread before fault */
        if(on_thread_before_fault) get_cur_thread_stack_info(sp, &stack_start_addr, &stack_size);
    }
    else
    {
        /* OS environment */
        if(cmt_get_sp() == cmt_get_psp()) get_cur_thread_stack_info(sp, &stack_start_addr, &stack_size);
    #endif
    }

    if(stack_is_overflow)
    {
        if(sp < stack_start_addr)
        {
            sp = stack_start_addr;
        }
        else if(sp > stack_start_addr + stack_size)
        {
            sp = stack_start_addr + stack_size;
        }
    }

    /* copy called function address */
    for(; sp < stack_start_addr + stack_size; sp += sizeof(uint32_t))
    {
        /* the *sp value may be LR, so need decrease a word to PC */
        pc = *((uint32_t *) sp) - sizeof(uint32_t);
        /* the Cortex-M using thumb instruction, so the pc must be an odd number */
        if(pc % 2 == 0) continue;
        /* fix the PC address in thumb mode */
        pc = *((uint32_t *) sp) - 1;
        if((pc >= code_start_addr + sizeof(uint32_t)) && (pc <= code_start_addr + code_size) && (depth < CMT_CALL_STACK_MAX_DEPTH)
        /* check the the instruction before PC address is 'BL' or 'BLX' */
        && disassembly_ins_is_bl_blx(pc - sizeof(uint32_t)) && (depth < size))
        {
            /* the second depth function may be already saved, so need ignore repeat */
            if((depth == 2) && regs_saved_lr_is_valid && (pc == buffer[1])) continue;
            buffer[depth++] = pc;
        }
    }

    return depth;
}

static void print_call_stack(uint32_t sp)
{
    uint32_t i;
    uint32_t call_stack_buf[CMT_CALL_STACK_MAX_DEPTH] = {0};
    uint32_t cur_depth = call_stack(call_stack_buf, CMT_CALL_STACK_MAX_DEPTH, sp);

    for(i = 0; i < cur_depth; i++)
    {
        sprintf(call_stack_info + i * (8 + 1), "%08lx", (unsigned long)call_stack_buf[i]);
        call_stack_info[i * (8 + 1) + 8] = ' ';
    }

    if(cur_depth)
    {
        call_stack_info[cur_depth * (8 + 1) - 1] = '\0';
        cmt_println(print_info[PRINT_CALL_STACK_INFO], FIRMWARE_NAME, CMT_ELF_FILE_EXTENSION_NAME, call_stack_info);
    }
    else
    {
        cmt_println(print_info[PRINT_CALL_STACK_ERR]);
    }
}

#if(CMT_CPU_TYPE != CMT_CPU_CM0)
static void fault_diagnosis(void)
{
    if(regs.hfsr.bits.VECTBL) cmt_println(print_info[PRINT_HFSR_VECTBL]);

    if(regs.hfsr.bits.FORCED)
    {
        if(regs.mfsr.value) /* Memory Management Fault */
        {
            if(regs.mfsr.bits.IACCVIOL)  cmt_println(print_info[PRINT_MFSR_IACCVIOL]);
            if(regs.mfsr.bits.DACCVIOL)  cmt_println(print_info[PRINT_MFSR_DACCVIOL]);
            if(regs.mfsr.bits.MUNSTKERR) cmt_println(print_info[PRINT_MFSR_MUNSTKERR]);
            if(regs.mfsr.bits.MSTKERR)   cmt_println(print_info[PRINT_MFSR_MSTKERR]);
            #if(CMT_CPU_TYPE == CMT_CPU_CM4) || (CMT_CPU_TYPE == CMT_CPU_CM7) || (CMT_CPU_TYPE == CMT_CPU_CM33)
            if(regs.mfsr.bits.MLSPERR)   cmt_println(print_info[PRINT_MFSR_MLSPERR]);
            #endif
            if(regs.mfsr.bits.MMARVALID && (regs.mfsr.bits.IACCVIOL || regs.mfsr.bits.DACCVIOL)) cmt_println(print_info[PRINT_MMAR], regs.mmar);
        }

        if(regs.bfsr.value) /* Bus Fault */
        {
            if(regs.bfsr.bits.IBUSERR)   cmt_println(print_info[PRINT_BFSR_IBUSERR]);
            if(regs.bfsr.bits.PRECISERR) cmt_println(print_info[PRINT_BFSR_PRECISERR]);
            if(regs.bfsr.bits.IMPREISERR)cmt_println(print_info[PRINT_BFSR_IMPREISERR]);
            if(regs.bfsr.bits.UNSTKERR)  cmt_println(print_info[PRINT_BFSR_UNSTKERR]);
            if(regs.bfsr.bits.STKERR)    cmt_println(print_info[PRINT_BFSR_STKERR]);
            #if(CMT_CPU_TYPE == CMT_CPU_CM4) || (CMT_CPU_TYPE == CMT_CPU_CM7) || (CMT_CPU_TYPE == CMT_CPU_CM33)
            if(regs.bfsr.bits.LSPERR)    cmt_println(print_info[PRINT_BFSR_LSPERR]);
            #endif
            if(regs.bfsr.bits.BFARVALID && regs.bfsr.bits.PRECISERR) cmt_println(print_info[PRINT_BFAR], regs.bfar);
        }

        if(regs.ufsr.value) /* Usage Fault */
        {
            if(regs.ufsr.bits.UNDEFINSTR)   cmt_println(print_info[PRINT_UFSR_UNDEFINSTR]);
            if(regs.ufsr.bits.INVSTATE)     cmt_println(print_info[PRINT_UFSR_INVSTATE]);
            if(regs.ufsr.bits.INVPC)        cmt_println(print_info[PRINT_UFSR_INVPC]);
            if(regs.ufsr.bits.NOCP)         cmt_println(print_info[PRINT_UFSR_NOCP]);
            #if(CMT_CPU_TYPE == CMT_CPU_CM33)
            if(regs.ufsr.bits.STKOF)        cmt_println(print_info[PRINT_UFSR_STKOF]);
            #endif
            if(regs.ufsr.bits.UNALIGNED)    cmt_println(print_info[PRINT_UFSR_UNALIGNED]);
            if(regs.ufsr.bits.DIVBYZERO0)   cmt_println(print_info[PRINT_UFSR_DIVBYZERO0]);
        }
    }

    if(regs.hfsr.bits.DEBUGEVT) /* Debug Fault */
    {
        if(regs.dfsr.value)
        {
            if(regs.dfsr.bits.HALTED)   cmt_println(print_info[PRINT_DFSR_HALTED]);
            if(regs.dfsr.bits.BKPT)     cmt_println(print_info[PRINT_DFSR_BKPT]);
            if(regs.dfsr.bits.DWTTRAP)  cmt_println(print_info[PRINT_DFSR_DWTTRAP]);
            if(regs.dfsr.bits.VCATCH)   cmt_println(print_info[PRINT_DFSR_VCATCH]);
            if(regs.dfsr.bits.EXTERNAL) cmt_println(print_info[PRINT_DFSR_EXTERNAL]);
        }
    }
}
#endif

#if(CMT_CPU_TYPE == CMT_CPU_CM4) || (CMT_CPU_TYPE == CMT_CPU_CM7) || (CMT_CPU_TYPE == CMT_CPU_CM33)
static uint32_t statck_del_fpu_regs(uint32_t fault_handler_lr, uint32_t sp)
{
    /* the stack has S0~S15 and FPSCR registers when statck_has_fpu_regs is true, double word align */
    return (fault_handler_lr & (1UL << 4)) == 0 ? sp + sizeof(uint32_t) * 18 : sp;
}
#endif

/**
 * @brief backtrace for fault handler
 * @param fault_handler_lr the LR register value on fault handler
 * @param fault_handler_sp the stack pointer on fault handler
 */
void cmtrace_fault(uint32_t fault_handler_lr, uint32_t fault_handler_sp)
{
    uint32_t stack_pointer = fault_handler_sp, saved_regs_addr = stack_pointer;

    const char *regs_name[] = { "R0 ", "R1 ", "R2 ", "R3 ", "R12", "LR ", "PC ", "PSR" };

    #if CMT_DUMP_STACK_INFO_ENABLE
    uint32_t stack_start_addr = main_stack_start_addr;
    uint32_t stack_size = main_stack_size;
    #endif

    CMT_ASSERT(init_ok);
    CMT_ASSERT(!on_fault);

    on_fault = true;

    cmt_println("");

    #if CMT_OS_TYPE

    on_thread_before_fault = fault_handler_lr & (1UL << 2);

    if(on_thread_before_fault) /* check which stack was used before (MSP or PSP) */
    {
        cmt_println(print_info[PRINT_FAULT_ON_THREAD], get_cur_thread_name() != NULL ? get_cur_thread_name() : "NO_NAME");
        saved_regs_addr = stack_pointer = cmt_get_psp();

        #if CMT_DUMP_STACK_INFO_ENABLE
        get_cur_thread_stack_info(stack_pointer, &stack_start_addr, &stack_size);
        #endif
    }
    else
    {
        cmt_println(print_info[PRINT_FAULT_ON_HANDLER]);
    }

    #else

    cmt_println(print_info[PRINT_FAULT_ON_HANDLER]); /* bare metal(no OS) environment */

    #endif

    stack_pointer += sizeof(uint32_t) * 8; /* delete saved R0~R3, R12, LR,PC,xPSR registers space */

    #if(CMT_CPU_TYPE == CMT_CPU_CM4) || (CMT_CPU_TYPE == CMT_CPU_CM7) || (CMT_CPU_TYPE == CMT_CPU_CM33)
    stack_pointer = statck_del_fpu_regs(fault_handler_lr, stack_pointer);
    #endif

    #if CMT_DUMP_STACK_INFO_ENABLE
    if(stack_pointer < stack_start_addr || stack_pointer > stack_start_addr + stack_size) stack_is_overflow = true;
    dump_stack(stack_start_addr, stack_size, (uint32_t *) stack_pointer);
    #endif

    if(!stack_is_overflow) /* the stack frame may be get failed when it is overflow  */
    {
        cmt_println(print_info[PRINT_REGS_TITLE]); /* dump register */

        regs.saved.r0        = ((uint32_t *)saved_regs_addr)[0];  // Register R0
        regs.saved.r1        = ((uint32_t *)saved_regs_addr)[1];  // Register R1
        regs.saved.r2        = ((uint32_t *)saved_regs_addr)[2];  // Register R2
        regs.saved.r3        = ((uint32_t *)saved_regs_addr)[3];  // Register R3
        regs.saved.r12       = ((uint32_t *)saved_regs_addr)[4];  // Register R12
        regs.saved.lr        = ((uint32_t *)saved_regs_addr)[5];  // Link register LR
        regs.saved.pc        = ((uint32_t *)saved_regs_addr)[6];  // Program counter PC
        regs.saved.psr.value = ((uint32_t *)saved_regs_addr)[7];  // Program status word PSR

        cmt_println("  %s: %08x  %s: %08x  %s: %08x  %s: %08x", regs_name[0], regs.saved.r0,
                                                                regs_name[1], regs.saved.r1,
                                                                regs_name[2], regs.saved.r2,
                                                                regs_name[3], regs.saved.r3);
        cmt_println("  %s: %08x  %s: %08x  %s: %08x  %s: %08x", regs_name[4], regs.saved.r12,
                                                                regs_name[5], regs.saved.lr,
                                                                regs_name[6], regs.saved.pc,
                                                                regs_name[7], regs.saved.psr.value);
        cmt_println("==============================================================");
    }

    /* the Cortex-M0 is not support fault diagnosis */
    #if(CMT_CPU_TYPE != CMT_CPU_CM0)
    regs.syshndctrl.value = CMT_SYSHND_CTRL;  // System Handler Control and State Register
    regs.mfsr.value       = CMT_NVIC_MFSR;    // Memory Fault Status Register
    regs.mmar             = CMT_NVIC_MMAR;    // Memory Management Fault Address Register
    regs.bfsr.value       = CMT_NVIC_BFSR;    // Bus Fault Status Register
    regs.bfar             = CMT_NVIC_BFAR;    // Bus Fault Manage Address Register
    regs.ufsr.value       = CMT_NVIC_UFSR;    // Usage Fault Status Register
    regs.hfsr.value       = CMT_NVIC_HFSR;    // Hard Fault Status Register
    regs.dfsr.value       = CMT_NVIC_DFSR;    // Debug Fault Status Register
    regs.afsr             = CMT_NVIC_AFSR;    // Auxiliary Fault Status Register
    fault_diagnosis();
    #endif

    print_call_stack(stack_pointer);
}

void HardFault_Handler(void)
{
    asm("    MOV    r0, lr");
    asm("    MOV    r1, sp");
    asm("    BL     cmtrace_fault");
    asm("_Loop:");
    asm("    BL     _Loop");
}
