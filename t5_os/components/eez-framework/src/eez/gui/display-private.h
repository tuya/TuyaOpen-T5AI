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

#include <eez/gui/font.h>

namespace eez {
namespace gui {
namespace display {

void initDriver();

#if defined(EEZ_PLATFORM_STM32)
typedef uint16_t *VideoBuffer;
#endif
#if defined(EEZ_PLATFORM_SIMULATOR) || defined(__EMSCRIPTEN__)
typedef uint32_t *VideoBuffer;
#endif

enum DisplayState {
    OFF,
    TURNING_ON,
    ON,
    TURNING_OFF
};

extern DisplayState g_displayState;

extern VideoBuffer g_syncedBuffer;
void syncBuffer();

void copySyncedBufferToScreenshotBuffer();

extern uint16_t g_fc, g_bc;
extern uint8_t g_opacity;

extern gui::font::Font g_font;

void drawStrInit();
void drawGlyph(const uint8_t *src, uint32_t srcLineOffset, int x, int y, int width, int height);

static const int NUM_BUFFERS = 6;

struct RenderBuffer {
    VideoBuffer bufferPointer;
    VideoBuffer previousBuffer;
    int x;
    int y;
    int width;
    int height;
    bool withShadow;
    uint8_t opacity;
    int xOffset;
    int yOffset;
    gui::Rect *backdrop;
};
extern RenderBuffer g_renderBuffers[NUM_BUFFERS];

void setBufferPointer(VideoBuffer buffer);

extern bool g_dirty;
inline void clearDirty() { g_dirty = false; }
inline void setDirty() { g_dirty = true; }
inline bool isDirty() { return g_dirty; }

extern bool g_screenshotAllocated;

#ifdef GUI_CALC_FPS
static const size_t NUM_FPS_VALUES = 60;
extern uint32_t g_fpsValues[NUM_FPS_VALUES];
void calcFPS();
#endif

} // namespace display
} // namespace gui
} // namespace eez
