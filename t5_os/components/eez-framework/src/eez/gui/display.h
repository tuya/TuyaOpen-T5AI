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

#include <agg2d.h>
#include <agg_rendering_buffer.h>

#if defined(EEZ_PLATFORM_STM32)
	typedef uint16_t *VideoBuffer;
#endif
#if defined(EEZ_PLATFORM_SIMULATOR) || defined(__EMSCRIPTEN__)
	typedef uint32_t *VideoBuffer;
#endif

#if EEZ_OPTION_GUI_ANIMATIONS
#include <eez/gui/animation.h>
#endif

#include <eez/gui/font.h>
#include <eez/gui/geometry.h>
#include <eez/gui/image.h>

static const int CURSOR_WIDTH = 2;

namespace eez {
namespace gui {
namespace display {

#define TRANSPARENT_COLOR_INDEX 0xFFFF

#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x0400
#define COLOR_BLUE 0x001F

// C: rrrrrggggggbbbbb
#define RGB_TO_COLOR(R, G, B) (uint16_t((R)&0xF8) << 8) | (uint16_t((G)&0xFC) << 3) | (((B)&0xF8) >> 3)

#define COLOR_TO_R(C) (uint8_t(((C) >> 11) << 3))
#define COLOR_TO_G(C) (uint8_t((((C) >> 5) << 2) & 0xFF))
#define COLOR_TO_B(C) (uint8_t(((C) << 3) & 0xFF))

extern VideoBuffer g_renderBuffer;

void init();

void turnOn();
void turnOff();
bool isOn();

void onThemeChanged();
void onLuminocityChanged();
void updateBrightness();

void update();

#if EEZ_OPTION_GUI_ANIMATIONS
void animate(Buffer startBuffer, void (*callback)(float t, VideoBuffer bufferOld, VideoBuffer bufferNew, VideoBuffer bufferDst), float duration = -1);
#endif

void beginRendering();
int beginBufferRendering();
void endBufferRendering(int bufferIndex, int x, int y, int width, int height, bool withShadow, uint8_t opacity, int xOffset, int yOffset, gui::Rect *backdrop);
void endRendering();

VideoBuffer getBufferPointer();

const uint8_t *takeScreenshot();
void releaseScreenshot();

#ifdef GUI_CALC_FPS
extern bool g_calcFpsEnabled;
extern bool g_drawFpsGraphEnabled;
extern uint32_t g_fpsAvg;
void drawFpsGraph(int x, int y, int w, int h, const Style *style);
#endif


uint32_t color16to32(uint16_t color, uint8_t opacity = 255);
uint16_t color32to16(uint32_t color);
uint32_t blendColor(uint32_t fgColor, uint32_t bgColor);

inline int getDisplayWidth() { return DISPLAY_WIDTH; }
inline int getDisplayHeight() { return DISPLAY_HEIGHT;  }

uint16_t getColor16FromIndex(uint16_t color);

void setColor(uint8_t r, uint8_t g, uint8_t b);
void setColor16(uint16_t color16);
void setColor(uint16_t color, bool ignoreLuminocity = false);
uint16_t getColor();

void setBackColor(uint8_t r, uint8_t g, uint8_t b);
void setBackColor(uint16_t color, bool ignoreLuminocity = false);
uint16_t getBackColor();

uint8_t setOpacity(uint8_t opacity);
uint8_t getOpacity();

void getPixel(int x, int y, uint8_t *r, uint8_t *g, uint8_t *b);

// these are the basic drawing operations
void startPixelsDraw();
void drawPixel(int x, int y);
void drawPixel(int x, int y, uint8_t opacity);
void endPixelsDraw();
void fillRect(int x1, int y1, int x2, int y2);
void bitBlt(int x1, int y1, int x2, int y2, int x, int y);
void drawBitmap(Image *image, int x, int y);

// used by animation
void fillRect(void *dst, int x1, int y1, int x2, int y2);
void bitBlt(void *src, int x1, int y1, int x2, int y2);
void bitBlt(void *src, void *dst, int x1, int y1, int x2, int y2);
void bitBlt(void *src, void *dst, int sx, int sy, int sw, int sh, int dx, int dy, uint8_t opacity); // also used for buffer rendering (see endRendering)

// these are implemented by calling basic drawing operations
void drawHLine(int x, int y, int l);
void drawVLine(int x, int y, int l);
void drawRect(int x1, int y1, int x2, int y2);
void drawFocusFrame(int x, int y, int w, int h);

// AGG based drawing
struct AggDrawing {
    AggDrawing() {
        startPixelsDraw();
    }
    ~AggDrawing() {
        endPixelsDraw();
    }
    agg::rendering_buffer rbuf;
	Agg2D graphics;
};

void aggInit(AggDrawing& aggDrawing);

void drawRoundedRect(
	AggDrawing &aggDrawing,
	int x1, int y1, int x2, int y2,
	int lineWidth,
	int rtlx, int rtly, int rtrx, int rtry,
	int rbrx, int rbry, int rblx, int rbly
);

void fillRoundedRect(
	AggDrawing &aggDrawing,
	int x1, int y1, int x2, int y2,
	int lineWidth,
	int rtlx, int rtly, int rtrx, int rtry,
	int rbrx, int rbry, int rblx, int rbly,
	bool drawLine, bool fill,
	int clip_x1 = -1, int clip_y1 = -1, int clip_x2 = -1, int clip_y2 = -1
);

void fillRoundedRect(
	AggDrawing &aggDrawing,
	int x1, int y1, int x2, int y2,
	int lineWidth,
	int r,
	bool drawLine, bool fill,
	int clip_x1 = -1, int clip_y1 = -1, int clip_x2 = -1, int clip_y2 = -1
);

void drawStr(const char *text, int textLength, int x, int y, int clip_x1, int clip_y1, int clip_x2, int clip_y2, gui::font::Font &font, int cursorPosition);
int getCharIndexAtPosition(int xPos, const char *text, int textLength, int x, int y, int clip_x1, int clip_y1, int clip_x2,int clip_y2, gui::font::Font &font);
int getCursorXPosition(int cursorPosition, const char *text, int textLength, int x, int y, int clip_x1, int clip_y1, int clip_x2,int clip_y2, gui::font::Font &font);
int8_t measureGlyph(int32_t encoding, gui::font::Font &font);
int measureStr(const char *text, int textLength, gui::font::Font &font, int max_width = 0);

} // namespace display
} // namespace gui
} // namespace eez
