/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configBK_ASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "sdkconfig.h"

#if CONFIG_FREERTOS_SMP
#include "smp.h"
#endif

#if CONFIG_FREERTOS_V9
#define FreeRTOS_VERSION_MAJOR                      9
#elif CONFIG_FREERTOS_V10 || CONFIG_FREERTOS_SMP
#define FreeRTOS_VERSION_MAJOR                      10
#endif

// Modified by TUYA Start
#ifndef CONFIG_FREERTOS_USE_RECURSIVE_MUTEXES
#define CONFIG_FREERTOS_USE_RECURSIVE_MUTEXES       1
#endif
// Modified by TUYA End

/* Tick */
#define configCPU_CLOCK_HZ                          CONFIG_CPU_FREQ_HZ
#if CONFIG_FREERTOS_TICK_RATE_HZ
#define configTICK_RATE_HZ                          (CONFIG_FREERTOS_TICK_RATE_HZ)
#else
#define configTICK_RATE_HZ                          ( ( TickType_t ) 500 )
#endif
#define configUSE_16_BIT_TICKS                      0
#define configUSE_WATCHDOG_TICK                     32000

/* Priorities */
#define configKERNEL_INTERRUPT_PRIORITY             255

/* Timers */
#define configUSE_TIMERS                            ( 1 )
// Modified by TUYA Start
#define configTIMER_TASK_PRIORITY                   ( 9 )
// Modified by TUYA End
#define configTIMER_QUEUE_LENGTH                    ( 16 )
#define configTIMER_TASK_STACK_DEPTH                ( ( unsigned short ) (3072 / sizeof( portSTACK_TYPE )) )

/* Task */
#if CONFIG_FREERTOS_V10
#define configUSE_TICKLESS_IDLE                     CONFIG_FREERTOS_USE_TICKLESS_IDLE
#endif // CONFIG_FREERTOS_V10

#if CONFIG_FREERTOS_RTT_MONITOR
#define configUSE_SEGGER_SYSTEM_VIEWER_HEAP_EVENTS 1
#define INCLUDE_pxTaskGetStackStart               (1)
#endif

#if CONFIG_FREERTOS_SMP
#define configUSE_PORT_OPTIMISED_TASK_SELECTION    0
#define configNUMBER_OF_CORES                      CONFIG_CPU_CNT
#define configUSE_MINIMAL_IDLE_HOOK                1
#define configUSE_TICKLESS_IDLE                    CONFIG_FREERTOS_USE_TICKLESS_IDLE
#define portCRITICAL_NESTING_IN_TCB                1
#define configUSE_CORE_AFFINITY                    1
#define configRUN_MULTIPLE_PRIORITIES              1
#if CONFIG_FREERTOS_ALLOW_OS_API_IN_IRQ_DISABLED
#define configUSE_OS_API_IN_IRQ_DISABLED           1
#endif

/* configTICK_CORE indicates which core should handle the SysTick interrupts */
#define configTICK_CORE                            CPU0_CORE_ID

/*TODO: this macro definition will be removed in the near future, and this is only for freertos smp branch*/
#define configNUM_CORES                            configNUMBER_OF_CORES
#define configUSE_TIME_SLICING                     0
#define configUSE_TASK_PREEMPTION_DISABLE          1
#endif // CONFIG_FREERTOS_SMP

#if CONFIG_FREERTOS_POSIX
#define configUSE_APPLICATION_TASK_TAG             1
#define configSUPPORT_STATIC_ALLOCATION            1
#endif

// Modified by TUYA Start
#define configMAX_PRIORITIES                        ( 11 )
// Modified by TUYA End
#define configUSE_PREEMPTION                        1
#define configMINIMAL_STACK_SIZE                    ( ( unsigned short ) (768/2) )
#define configMAX_TASK_NAME_LEN                     ( 16 )
#define configIDLE_SHOULD_YIELD                     1
#define configUSE_CO_ROUTINES                       0
#define configMAX_CO_ROUTINE_PRIORITIES             ( 2 )

/* Hooks */
#define configUSE_IDLE_HOOK                         1
#define configUSE_TICK_HOOK                         0
#define configUSE_MALLOC_FAILED_HOOK                ( 1 )

/* Memory allocation definitions. */
#if CONFIG_FREERTOS_V10 || CONFIG_FREERTOS_SMP
#define configSUPPORT_STATIC_ALLOCATION			   1
#define configSUPPORT_DYNAMIC_ALLOCATION		   1
#endif

#if CONFIG_SOC_BK7256XX
#define configDYNAMIC_HEAP_SIZE                     0
#else
#define configDYNAMIC_HEAP_SIZE                     1
#endif

#if (CONFIG_CUSTOMIZE_HEAP_SIZE)
#define configTOTAL_HEAP_SIZE                       ( ( size_t ) (CONFIG_CUSTOMIZE_HEAP_SIZE) )
#elif (CONFIG_SYS_CPU1)
#define configTOTAL_HEAP_SIZE                       ( ( size_t ) ( 20 * 1024 ) )
#else
#define configTOTAL_HEAP_SIZE                       ( ( size_t ) ( 212 * 1024 ) )
#endif
/* Queue & Semaphore & Mutex */
#define configQUEUE_REGISTRY_SIZE                   0
#define configUSE_COUNTING_SEMAPHORES               1
#define configUSE_MUTEXES                           1

#if (CONFIG_FREERTOS_USE_QUEUE_SETS)
#define configUSE_QUEUE_SETS                        1
#endif
#if CONFIG_FREERTOS_POSIX || CONFIG_FREERTOS_USE_RECURSIVE_MUTEXES
#define configUSE_RECURSIVE_MUTEXES                  1
#endif


/* Utilities */
#define configUSE_TRACE_FACILITY                    1

#define configUSE_ALTERNATIVE_API                   0
//Set configCHECK_FOR_STACK_OVERFLOW to 3 to add ISR stack checking.
#define configCHECK_FOR_STACK_OVERFLOW              3
#if CONFIG_FREERTOS_V9
#define configGENERATE_RUN_TIME_STATS               0
#endif
#if CONFIG_FREERTOS_V10
#define configGENERATE_RUN_TIME_STATS               1
#elif CONFIG_FREERTOS_SMP
#define configGENERATE_RUN_TIME_STATS               1
#define configGENERATE_RUN_TIME_STATS_PER_CORE      1
#endif
#define configUSE_IDLE_SLEEP_HOOK                   1
#define configUSE_STATS_FORMATTING_FUNCTIONS        1

#if (configGENERATE_RUN_TIME_STATS == 1)
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
#define portGET_RUN_TIME_COUNTER_VALUE()            bk_get_tick()
#endif

#if CONFIG_FREERTOS_POSIX
#define configUSE_POSIX_ERRNO                       1
#endif
/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet                    1
#define INCLUDE_uxTaskPriorityGet                   1
#define INCLUDE_uxTaskGetStackHighWaterMark         1
#define INCLUDE_vTaskDelete                         1
#define INCLUDE_vTaskCleanUpResources               0
#define INCLUDE_vTaskSuspend                        1
#define INCLUDE_vTaskDelayUntil                     1
#define INCLUDE_vTaskDelay                          1
#define INCLUDE_xTaskAbortDelay                     1
#define INCLUDE_xTaskGetCurrentTaskHandle           1
#define INCLUDE_xTimerPendFunctionCall              1

#if CONFIG_FREERTOS_V10 || CONFIG_FREERTOS_SMP
#define INCLUDE_xTaskGetHandle                      1
#define INCLUDE_eTaskGetState                       1
#define INCLUDE_xSemaphoreGetMutexHolder            1
#endif

#if CONFIG_FREERTOS_POSIX
#define INCLUDE_xQueueGetMutexHolder                1
#endif

#define configNUM_THREAD_LOCAL_STORAGE_POINTERS    3

#define configMAX_SYSCALL_INTERRUPT_PRIORITY        191 /* equivalent to 0xb0, or priority 11. */

#define configBK_FREERTOS                           1

#define configASSERT(cond)         BK_ASSERT(cond)

#if CONFIG_SOC_BK7256XX
/****** Hardware/compiler specific settings. *******************************************/
#define configISR_STACK_SIZE_WORDS 1024
#define configLIST_VOLATILE        volatile

/* The following constant describe the hardware */
/* Enable Hardware Stack Protection and Recording mechanism. */
#define configHSP_ENABLE			0

/* Hardware stack protection for ISR can't support statically allocated IRQ_STACK */
#if(configHSP_ENABLE == 1 && configISR_STACK_SIZE_WORDS == 0)
#define configISR_HSP_ENABLE			1
#endif

/* Record the highest address of stack. */
#if(configHSP_ENABLE == 1 )
#ifndef configRECORD_STACK_HIGH_ADDRESS
#define configRECORD_STACK_HIGH_ADDRESS		1
#endif
#endif
/* Andes RTOS Tracer specifics. */

/* Enable Andes RTOS Tracer */
#define configUSE_ANDES_TRACER				0

/* Buffer size recording event data in target.
Members except log buffer in uncached buffer used at most 6 cache line space. */
#define configTRACER_LOG_BUFFER_SIZE		( 4096 - ( 6 * ( tracerCACHE_LINE_SIZE ) ) )

/* Maximum number of RTOS tasks info can be kept in target */
#define configTRACER_MAX_NUM_TASK_INFO		32

#if( configUSE_ANDES_TRACER == 1 && configUSE_TRACE_FACILITY != 1 )
	/* Andes RTOS Tracer needs configUSE_TRACE_FACILITY. Forcely enable it. */
	#undef configUSE_TRACE_FACILITY
	#define configUSE_TRACE_FACILITY		1
#endif

#if( configUSE_ANDES_TRACER == 1 )
	/* Enlarge stack size in Andes RTOS Tracer */
	#undef configMINIMAL_STACK_SIZE
	#define configMINIMAL_STACK_SIZE		( 640 )

	/* Define TIMER_TASK_STACK_DEPTH again because MINIMAL_STACK_SIZE changes */
	#undef configTIMER_TASK_STACK_DEPTH
	#define configTIMER_TASK_STACK_DEPTH	( configMINIMAL_STACK_SIZE + 256 )
#endif

/* Andes RTOS Tracer header. */
// #include "tracer.h"

#define configMTIME_BASE_ADDRESS		( 0xE6000000 )
#define configMTIMECMP_BASE_ADDRESS		( 0xE6000008 )

#endif // #if CONFIG_SOC_BK7256XX

#if (CONFIG_ARCH_CM33 == 1)
#define configENABLE_FPU                   1
#define configENABLE_MPU                   0
#define configENABLE_TRUSTZONE             0
#define configMINIMAL_SECURE_STACK_SIZE    (1024)

#if CONFIG_SPE
#define configRUN_FREERTOS_SECURE_ONLY     1
#else
#define configRUN_FREERTOS_SECURE_ONLY     0
#endif
#endif

/*Trace Recorder for Tracealyzer v4.6.6*/
#if CONFIG_FREERTOS_TRACE
#include "trcRecorder.h"
#endif

#endif /* FREERTOS_CONFIG_H */
