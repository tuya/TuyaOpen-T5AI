/*
 * eez-framework
 *
 * MIT License
 * Copyright 2024 Envox d.o.o.
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <stdint.h>

#if defined(EEZ_FOR_LVGL)

uint32_t osKernelGetTickCount(void);

#else

#include "cmsis_os2.h"

#if defined(EEZ_PLATFORM_STM32)
#include "FreeRTOS.h"
#include "task.h"
#endif

#define EEZ_THREAD_DECLARE(NAME, PRIORITY, STACK_SIZE) \
    osThreadId_t g_##NAME##TaskHandle; \
    const osThreadAttr_t g_##NAME##TaskAttributes = { \
        #NAME, \
		0, \
		0, \
		0, \
		0, \
        STACK_SIZE, \
        osPriority##PRIORITY, \
		0, \
		0, \
    }
#define EEZ_THREAD_CREATE(NAME, THREAD_FUNC) g_##NAME##TaskHandle = osThreadNew(THREAD_FUNC, nullptr, &g_##NAME##TaskAttributes);
#define EEZ_THREAD_TERMINATE(NAME) osThreadTerminate(g_##NAME##TaskHandle)

#define EEZ_MESSAGE_QUEUE_DECLARE(NAME, OBJECT_DEF) \
    struct NAME##MessageQueueObject OBJECT_DEF; \
    osMessageQueueId_t g_##NAME##MessageQueueId
#define EEZ_MESSAGE_QUEUE_CREATE(NAME, QUEUE_SIZE) g_##NAME##MessageQueueId = osMessageQueueNew(QUEUE_SIZE, sizeof(NAME##MessageQueueObject), nullptr)
#define EEZ_MESSAGE_QUEUE_GET(NAME, OBJ, TIMEOUT) (osMessageQueueGet(g_##NAME##MessageQueueId, &OBJ, nullptr, TIMEOUT) == osOK)
#define EEZ_MESSAGE_QUEUE_PUT(NAME, OBJ, TIMEOUT) osMessageQueuePut(g_##NAME##MessageQueueId, &OBJ, 0, TIMEOUT)

#define EEZ_MUTEX_DECLARE(NAME) \
    osMutexId_t g_##NAME##mutexId;\
    const osMutexAttr_t g_##NAME##mutexAttr = { \
        #NAME, \
        osMutexRecursive | osMutexPrioInherit, \
        NULL, \
        0U \
    }
#define EEZ_MUTEX_CREATE(NAME) g_##NAME##mutexId = osMutexNew(&g_##NAME##mutexAttr)
#define EEZ_MUTEX_WAIT(NAME, TIMEOUT) osMutexAcquire(g_##NAME##mutexId, TIMEOUT) == osOK
#define EEZ_MUTEX_RELEASE(NAME) osMutexRelease(g_##NAME##mutexId)

#endif

#if defined(__EMSCRIPTEN__)
#ifndef EM_PORT_API
#	if defined(__EMSCRIPTEN__)
#		include <emscripten.h>
#		if defined(__cplusplus)
#			define EM_PORT_API(rettype) extern "C" rettype EMSCRIPTEN_KEEPALIVE
#		else
#			define EM_PORT_API(rettype) rettype EMSCRIPTEN_KEEPALIVE
#		endif
#	else
#		if defined(__cplusplus)
#			define EM_PORT_API(rettype) extern "C" rettype
#		else
#			define EM_PORT_API(rettype) rettype
#		endif
#	endif
#endif
#else
#    define EM_PORT_API(rettype) rettype
#endif

namespace eez {

enum TestResult {
	TEST_NONE,
	TEST_FAILED,
	TEST_OK,
	TEST_CONNECTING,
	TEST_SKIPPED,
	TEST_WARNING
};

uint32_t millis();

extern bool g_shutdown;
void shutdown();

} // namespace eez
