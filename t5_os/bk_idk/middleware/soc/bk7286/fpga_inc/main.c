#include <stdint.h>
#include <string.h>
#include "bkreg.h"
#include "bkstdio.h"
#include "drv_gpio.h"
#include "drv_uart.h"
#include "drv_system.h"
#include "FreeRTOS.h"
#include "task.h"
#include "shell.h"
#include "cmtrace.h"

#define FREE_RTOS_ENABLE    (1)
#if     FREE_RTOS_ENABLE

void freertos_task_shell(void* pvParameters)
{
    while(1)
    {
        shell_process();
        system_peri_cpu0_halt();
    }
}

#endif

void vApplicationIdleHook(void)
{
    vPortYield();
    system_peri_cpu0_halt();
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char * pcTaskName)
{
    bk_printf("[ERROR]: Stack of task <%s> overflowed\n", pcTaskName);
}

void vApplicationMallocFailedHook(void)
{
    bk_printf("[ERROR]: Malloc failed\n");
}

void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer, StackType_t** ppxIdleTaskStackBuffer, uint32_t* pulIdleTaskStackSize )
{
    static StaticTask_t xIdleTaskTCB;
    static StackType_t  uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer, StackType_t** ppxTimerTaskStackBuffer, uint32_t* pulTimerTaskStackSize)
{
    static StaticTask_t xTimerTaskTCB;
    static StackType_t  uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    *ppxTimerTaskTCBBuffer   = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}

int main(void)
{
    REG_WDT_CONFIG = 0x5A0000;
	REG_WDT_CONFIG = 0xA50000;
	REG_AONWDT_CONFIG = 0x5A0000;
	REG_AONWDT_CONFIG = 0xA50000;

    #if CHIPID == 7236
    REG_PMU_0x00 &= ~(1 << 31);
    REG_FLASH_0x0A |= (1 << 4);//DUAL-LINE
    system_init();
    #endif

    gpio_config(GPIO10, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_FUNC1);
    gpio_config(GPIO11, GPIO_INPUT,  GPIO_PULL_UP, GPIO_PERI_FUNC1);
    system_peri_clk_enable(SYS_PERI_CLK_UART0);
    system_peri_cpu0_irq_enable(SYS_PERI_IRQ_UART0);

    uart_init(PRINT_UART_PORT, 115200, UART_DATA_BITS_8, UART_STOP_BITS_1, UART_PARITY_NONE);

    bk_printf("Welcome to use BEKEN's chip BK7286!\n");
    bk_printf("Firmware build @ %s %s\n", __TIME__, __DATE__);

    gpio_config(GPIO20, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_FUNC2);//SWC
    gpio_config(GPIO21, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_PERI_FUNC2);//SWD

    //(*(volatile unsigned long*)(0x44000400 + 0x00 * 4)) = 2;  //FIXME@TODO@GPIO
    //(*(volatile unsigned long*)(0x44820000 + 0x07 * 4)) = '2';//FIXME@TODO@UART

    cmtrace_init();

    bk_printf("\n%s", SHELL_PROMPT_SYMBOL);

    //bk_uart_rx_cmd("isp");

    #if FREE_RTOS_ENABLE

    xTaskCreate(freertos_task_shell,
                (const char*)"SHELL",
                configMINIMAL_STACK_SIZE * 80,
                NULL,
                5,
                NULL);

    vTaskStartScheduler();

    #else

    while(1)
    {
        shell_process();
    }

    #endif

    return 0;
}

void __aeabi_assert(const char* type, const char* file, int line)
{
    bk_printf("[ASSERT]: %s:%d, %s\n", file, line, type);
}
