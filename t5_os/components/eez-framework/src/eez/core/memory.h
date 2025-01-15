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

#include <eez/conf-internal.h>

#include <stdint.h>

namespace eez {

#ifdef CONF_MEMORY_BEGIN
    static uint8_t * const MEMORY_BEGIN = (uint8_t *)CONF_MEMORY_BEGIN;
#else
    #if defined(EEZ_FOR_LVGL)
        static uint8_t * const MEMORY_BEGIN = 0;
    #else
        #if defined(EEZ_PLATFORM_STM32)
            static uint8_t * const MEMORY_BEGIN = (uint8_t *)0xc0000000u;
        #endif
        #if defined(EEZ_PLATFORM_SIMULATOR) || defined(__EMSCRIPTEN__)
            extern uint8_t g_memory[];
            static uint8_t * const MEMORY_BEGIN = g_memory;
        #endif
    #endif
#endif

#ifdef CONF_MEMORY_SIZE
    static const uint32_t MEMORY_SIZE = CONF_MEMORY_SIZE;
#else
    #if defined(EEZ_FOR_LVGL)
    #else
        #if defined(EEZ_PLATFORM_STM32)
            #if CONF_OPTION_FPGA
                static const uint32_t MEMORY_SIZE = 32 * 1024 * 1024;
            #elif defined(EEZ_PLATFORM_STM32F469I_DISCO)
                static const uint32_t MEMORY_SIZE = 16 * 1024 * 1024;
            #else
                static const uint32_t MEMORY_SIZE = 8 * 1024 * 1024;
            #endif
        #endif
        #if defined(EEZ_PLATFORM_SIMULATOR) || defined(__EMSCRIPTEN__)
            static const uint32_t MEMORY_SIZE = 64 * 1024 * 1024;
        #endif
    #endif
#endif


extern uint8_t *ALLOC_BUFFER;
extern uint32_t ALLOC_BUFFER_SIZE;

#if !defined(EEZ_FOR_LVGL)
    extern uint8_t *DECOMPRESSED_ASSETS_START_ADDRESS;
    #if defined(CONF_MAX_DECOMPRESSED_ASSETS_SIZE)
        static const uint32_t MAX_DECOMPRESSED_ASSETS_SIZE = CONF_MAX_DECOMPRESSED_ASSETS_SIZE;
    #else
        #if defined(EEZ_PLATFORM_STM32)
            static const uint32_t MAX_DECOMPRESSED_ASSETS_SIZE = 2 * 1024 * 1024;
        #endif
        #if defined(EEZ_PLATFORM_SIMULATOR) || defined(__EMSCRIPTEN__)
            static const uint32_t MAX_DECOMPRESSED_ASSETS_SIZE = 8 * 1024 * 1024;
        #endif
    #endif
#endif

#if !defined(EEZ_FOR_LVGL)
    extern uint8_t *FLOW_TO_DEBUGGER_MESSAGE_BUFFER;
    #if defined(EEZ_FOR_LVGL)
        static const uint32_t FLOW_TO_DEBUGGER_MESSAGE_BUFFER_SIZE = 32 * 1024;
    #else
        #if defined(EEZ_PLATFORM_STM32)
            static const uint32_t FLOW_TO_DEBUGGER_MESSAGE_BUFFER_SIZE = 32 * 1024;
        #endif
        #if defined(EEZ_PLATFORM_SIMULATOR) || defined(__EMSCRIPTEN__)
            static const uint32_t FLOW_TO_DEBUGGER_MESSAGE_BUFFER_SIZE = 1024 * 1024;
        #endif
    #endif
#endif

#if EEZ_OPTION_GUI
    extern uint8_t *VRAM_BUFFER1_START_ADDRESS;
    extern uint8_t *VRAM_BUFFER2_START_ADDRESS;

    #if EEZ_OPTION_GUI_ANIMATIONS
        extern uint8_t *VRAM_ANIMATION_BUFFER1_START_ADDRESS;
        extern uint8_t *VRAM_ANIMATION_BUFFER2_START_ADDRESS;
    #endif

    #ifndef NUM_AUX_BUFFERS
        #define NUM_AUX_BUFFERS 6
    #endif

    extern uint8_t *VRAM_AUX_BUFFER_START_ADDRESSES[NUM_AUX_BUFFERS];

    extern uint8_t* SCREENSHOOT_BUFFER_START_ADDRESS;

    extern uint8_t* GUI_STATE_BUFFER;
#endif

void initMemory();
void initAssetsMemory();
void initOtherMemory();
uint8_t *allocBuffer(uint32_t size);

} // eez
