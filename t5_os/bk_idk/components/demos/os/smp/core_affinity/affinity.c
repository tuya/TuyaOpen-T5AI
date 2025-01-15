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

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Library includes. */
#include <stdio.h>
#include "gpio_driver.h"
#include "bk_gpio.h"

/* Priorities at which the tasks are created. */
#define	logPrintf_TASK_PRIORITY		        ( tskIDLE_PRIORITY + 3)
#define mainQUEUE_RECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define	mainQUEUE_SEND_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )

/* The rate at which data is sent to the queue.  The 200ms value is converted
to ticks using the portTICK_PERIOD_MS constant. */
#define mainQUEUE_SEND_FREQUENCY_MS			( 1000 / portTICK_PERIOD_MS )

/* The number of items the queue can hold.  This is 1 as the receive task
will remove items as they are added, meaning the send task should always find
the queue empty. */
#define mainQUEUE_LENGTH					( 1 )

/* The LED toggled by the Rx task. */
#define mainTASK_LED						( GPIO_15 )

/* disable delay for swd debugging, enable delay for testing effectiveness*/
#define CONFIG_DELAY_ENABLE                   0

/*-----------------------------------------------------------*/

/*
 * Called by main when mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 1 in
 * main.c.
 */
#define GPIO_UP(id) *(volatile uint32_t*) (SOC_AON_GPIO_REG_BASE + ((id) << 2)) = 2
#define GPIO_DOWN(id) *(volatile uint32_t*) (SOC_AON_GPIO_REG_BASE + ((id) << 2)) = 0

#define DEBUG_GPIO_UP         GPIO_UP
#define DEBUG_GPIO_DOWN       GPIO_DOWN

/*
 * The tasks as described in the comments at the top of this file.
 */
static void prvQueueReceiveTask( void *pvParameters );
static void prvQueueSendTask( void *pvParameters );
static void prvLogTask( void *pvParameters );

/*-----------------------------------------------------------*/

/* The queue used by both tasks. */
static QueueHandle_t xQueue = NULL;
TaskHandle_t xTxHandle;
TaskHandle_t xRxHandle;
TaskHandle_t xLogHandle;

void core_affinity_test(void)
{
	UBaseType_t uxCoreAffinityMask;

    os_printf(" Starting core_affinity.\n");
	BK_ASSERT(configUSE_CORE_AFFINITY);
	BK_ASSERT(configNUM_CORES > 1);

    /* Create the queue. */
	xQueue = xQueueCreate( mainQUEUE_LENGTH, sizeof( uint32_t ) );
	if( xQueue != NULL )
	{
		/* Start the two tasks as described in the comments at the top of this
		file. */
		xTaskCreate( prvQueueReceiveTask,				/* The function that implements the task. */
					"Rx", 								/* The text name assigned to the task - for debug only as it is not used by the kernel. */
					configMINIMAL_STACK_SIZE, 			/* The size of the stack to allocate to the task. */
					NULL, 								/* The parameter passed to the task - not used in this case. */
					mainQUEUE_RECEIVE_TASK_PRIORITY, 	/* The priority assigned to the task. */
					&(xRxHandle) );						/* The task handle*/
		uxCoreAffinityMask = (1 << 0);
		vTaskCoreAffinitySet(xRxHandle, uxCoreAffinityMask);
		
		xTaskCreate( prvQueueSendTask, 
			"TX", 
			configMINIMAL_STACK_SIZE, 
			NULL, 
			mainQUEUE_SEND_TASK_PRIORITY, 
			&(xTxHandle) );
		uxCoreAffinityMask = (1 << 1);
		vTaskCoreAffinitySet(xTxHandle, uxCoreAffinityMask);

		xTaskCreate( prvLogTask, 
			"LOG", 
			configMINIMAL_STACK_SIZE, 
			NULL, 
			logPrintf_TASK_PRIORITY, 
			&(xLogHandle) );
		uxCoreAffinityMask = (1 << 2); /* select core id:2*/
		vTaskCoreAffinitySet(xLogHandle, uxCoreAffinityMask);
	}
}

/*-----------------------------------------------------------*/
static void prvQueueSendTask( void *pvParameters )
{
#if (CONFIG_DELAY_ENABLE)
TickType_t xNextWakeTime;
#endif
const unsigned long ulValueToSend = 100UL;

	/* Remove compiler warning about unused parameter. */
	( void ) pvParameters;

#if (CONFIG_DELAY_ENABLE)
	/* Initialise xNextWakeTime - this only needs to be done once. */
	xNextWakeTime = xTaskGetTickCount();
#endif

	for( ;; )
	{
		DEBUG_GPIO_UP(GPIO12);
		DEBUG_GPIO_DOWN(GPIO12);

		/* Send to the queue - causing the queue receive task to unblock and
		toggle the LED.  0 is used as the block time so the sending operation
		will not block - it shouldn't need to block as the queue should always
		be empty at this point in the code. */
		os_printf("[%d]xQueueSend\r\n", portGET_CORE_ID());
		xQueueSend( xQueue, &ulValueToSend, 0U );
	}
}

/*-----------------------------------------------------------*/
static void prvQueueReceiveTask( void *pvParameters )
{
unsigned long ulReceivedValue;
const unsigned long ulExpectedValue = 100UL;

	/* Remove compiler warning about unused parameter. */
	( void ) pvParameters;

	for( ;; )
	{
		/* Wait until something arrives in the queue - this task will block
		indefinitely provided INCLUDE_vTaskSuspend is set to 1 in
		FreeRTOSConfig.h. */
		xQueueReceive( xQueue, &ulReceivedValue, portMAX_DELAY );
		os_printf("[%d]xQueueReceiver\n", portGET_CORE_ID());

		DEBUG_GPIO_UP(GPIO13);
		DEBUG_GPIO_DOWN(GPIO13);

		/*  To get here something must have been received from the queue, but
		is it the expected value?  If it is, toggle the LED. */
		if( ulReceivedValue == ulExpectedValue )
		{
			ulReceivedValue = 0U;
		}
	}
}
/*-----------------------------------------------------------*/
static void prvLogTask( void *pvParameters )
{
	/* Remove compiler warning about unused parameter. */
	( void ) pvParameters;

	for( ;; )
	{
		os_printf("prvLogTask at the core[%d]\r\n", portGET_CORE_ID());
		rtos_delay_milliseconds(500);

		DEBUG_GPIO_UP(GPIO28);
		DEBUG_GPIO_DOWN(GPIO28);
	}
}
/*-----------------------------------------------------------*/

