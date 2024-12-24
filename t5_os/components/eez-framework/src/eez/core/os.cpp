/*
 * eez-framework
 *
 * MIT License
 * Copyright 2024 Envox d.o.o.
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <eez/conf-internal.h>

#if defined(EEZ_PLATFORM_STM32)
#include <main.h>
#endif

#if defined(EEZ_PLATFORM_ESP32)
#include <esp_timer.h>
#endif

#if defined(EEZ_PLATFORM_PICO)
#include "pico/stdlib.h"
#endif

#if defined(EEZ_PLATFORM_RASPBERRY)
#include <circle/timer.h>
#endif

#if defined(EEZ_FOR_LVGL)
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#endif

#if defined(__EMSCRIPTEN__)
#include <sys/time.h>
#endif

#include <eez/core/os.h>

namespace eez {

uint32_t millis() {
#if defined(EEZ_PLATFORM_STM32)
	return HAL_GetTick();
#elif defined(__EMSCRIPTEN__)
	return (uint32_t)emscripten_get_now();
#elif defined(EEZ_PLATFORM_SIMULATOR)
	return osKernelGetTickCount();
#elif defined(EEZ_PLATFORM_ESP32)
	return (unsigned long) (esp_timer_get_time() / 1000ULL);
#elif defined(EEZ_PLATFORM_PICO)
    auto abs_time = get_absolute_time();
    return to_ms_since_boot(abs_time);
#elif defined(EEZ_PLATFORM_RASPBERRY)
    unsigned nStartTicks = CTimer::Get()->GetClockTicks();
    return nStartTicks / 1000;
#elif defined(EEZ_FOR_LVGL)
    return lv_tick_get();
#else
    #error "Missing millis implementation";
#endif
}

} // namespace eez
