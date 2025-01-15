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

#include <assert.h>

#include <eez/core/memory.h>

#if defined(EEZ_FOR_LVGL)
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#endif

namespace eez {

#if (defined(EEZ_PLATFORM_SIMULATOR) || defined(__EMSCRIPTEN__)) && !defined(EEZ_FOR_LVGL)
uint8_t g_memory[MEMORY_SIZE] = { 0 };
#endif

uint8_t *DECOMPRESSED_ASSETS_START_ADDRESS;
uint8_t *FLOW_TO_DEBUGGER_MESSAGE_BUFFER;

#if EEZ_OPTION_GUI
    uint8_t *VRAM_BUFFER1_START_ADDRESS;
    uint8_t *VRAM_BUFFER2_START_ADDRESS;

    #if EEZ_OPTION_GUI_ANIMATIONS
        uint8_t *VRAM_ANIMATION_BUFFER1_START_ADDRESS;
        uint8_t *VRAM_ANIMATION_BUFFER2_START_ADDRESS;
    #endif

    uint8_t *VRAM_AUX_BUFFER_START_ADDRESSES[NUM_AUX_BUFFERS];

    uint8_t *SCREENSHOOT_BUFFER_START_ADDRESS;

    uint8_t *GUI_STATE_BUFFER;
#endif

uint8_t *ALLOC_BUFFER = 0;
uint32_t ALLOC_BUFFER_SIZE = 0;

void initMemory() {
    initAssetsMemory();
    initOtherMemory();
}

void initAssetsMemory() {
#if defined(EEZ_FOR_LVGL)
#if defined(LV_MEM_SIZE)
    ALLOC_BUFFER_SIZE = LV_MEM_SIZE;
#endif
#else
    ALLOC_BUFFER = MEMORY_BEGIN;
    ALLOC_BUFFER_SIZE = MEMORY_SIZE;
    DECOMPRESSED_ASSETS_START_ADDRESS = allocBuffer(MAX_DECOMPRESSED_ASSETS_SIZE);
#endif
}

void initOtherMemory() {
#if !defined(EEZ_FOR_LVGL)
    FLOW_TO_DEBUGGER_MESSAGE_BUFFER = allocBuffer(FLOW_TO_DEBUGGER_MESSAGE_BUFFER_SIZE);
#endif

#if EEZ_OPTION_GUI
    uint32_t VRAM_BUFFER_SIZE = DISPLAY_WIDTH * DISPLAY_HEIGHT * DISPLAY_BPP / 8;

    VRAM_BUFFER1_START_ADDRESS = allocBuffer(VRAM_BUFFER_SIZE);
    VRAM_BUFFER2_START_ADDRESS = allocBuffer(VRAM_BUFFER_SIZE);

    for (size_t i = 0; i < NUM_AUX_BUFFERS; i++) {
        VRAM_AUX_BUFFER_START_ADDRESSES[i] = allocBuffer(VRAM_BUFFER_SIZE);
    }

#if EEZ_OPTION_GUI_ANIMATIONS
    VRAM_ANIMATION_BUFFER1_START_ADDRESS = allocBuffer(VRAM_BUFFER_SIZE);
    VRAM_ANIMATION_BUFFER2_START_ADDRESS = allocBuffer(VRAM_BUFFER_SIZE);
    SCREENSHOOT_BUFFER_START_ADDRESS = VRAM_ANIMATION_BUFFER1_START_ADDRESS;
#else
    SCREENSHOOT_BUFFER_START_ADDRESS = allocBuffer(VRAM_BUFFER_SIZE * 2);
#endif

    GUI_STATE_BUFFER = allocBuffer(GUI_STATE_BUFFER_SIZE);
#endif
}

uint8_t *allocBuffer(uint32_t size) {
#if defined(EEZ_FOR_LVGL)
    return (uint8_t *)lv_mem_alloc(size);
#else
    size = ((size + 1023) / 1024) * 1024;

    auto buffer = ALLOC_BUFFER;

    assert(ALLOC_BUFFER_SIZE > size);
    ALLOC_BUFFER += size;
    ALLOC_BUFFER_SIZE -= size;

    return buffer;
#endif
}

} // eez
