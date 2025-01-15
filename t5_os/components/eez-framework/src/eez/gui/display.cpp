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

#if EEZ_OPTION_GUI

#include <stdio.h>
#include <string.h>

#include <eez/core/utf8.h>

#include <eez/core/util.h>

#if OPTION_KEYBOARD
#include <eez/core/keyboard.h>
#endif

#if OPTION_MOUSE
#include <eez/core/mouse.h>
#endif

#include <eez/gui/gui.h>
#include <eez/gui/thread.h>

#include <eez/gui/display-private.h>

#define CONF_BACKDROP_OPACITY 128

using namespace eez::gui;

namespace eez {
namespace gui {
namespace display {

DisplayState g_displayState;

VideoBuffer g_renderBuffer1;
VideoBuffer g_renderBuffer2;

VideoBuffer g_syncedBuffer;
VideoBuffer g_renderBuffer;

#if EEZ_OPTION_GUI_ANIMATIONS
VideoBuffer g_animationBuffer1;
VideoBuffer g_animationBuffer2;
VideoBuffer g_animationBuffer;
#endif

bool g_takeScreenshot;

uint16_t g_fc, g_bc;
uint8_t g_opacity = 255;

gui::font::Font g_font;

static uint8_t g_colorCache[256][4];

#define FLOAT_TO_COLOR_COMPONENT(F) ((F) < 0 ? 0 : (F) > 255 ? 255 : (uint8_t)(F))
#define RGB_TO_HIGH_BYTE(R, G, B) (((R) & 248) | (G) >> 5)
#define RGB_TO_LOW_BYTE(R, G, B) (((G) & 28) << 3 | (B) >> 3)

static const uint16_t *g_themeColors;
static uint32_t g_themeColorsCount;
static const uint16_t *g_colors;

bool g_dirty;

RenderBuffer g_renderBuffers[NUM_BUFFERS];
static VideoBuffer g_mainBufferPointer;
static int g_numBuffersToDraw;

bool g_screenshotAllocated;

////////////////////////////////////////////////////////////////////////////////

void init() {
    onLuminocityChanged();
    onThemeChanged();

    g_renderBuffer1 = (VideoBuffer)VRAM_BUFFER1_START_ADDRESS;
    g_renderBuffer2 = (VideoBuffer)VRAM_BUFFER2_START_ADDRESS;

#if EEZ_OPTION_GUI_ANIMATIONS
    g_animationBuffer1 = (VideoBuffer)VRAM_ANIMATION_BUFFER1_START_ADDRESS;
    g_animationBuffer2 = (VideoBuffer)VRAM_ANIMATION_BUFFER2_START_ADDRESS;
#endif

    for (size_t i = 0; i < NUM_AUX_BUFFERS; i++) {
        g_renderBuffers[i].bufferPointer = (VideoBuffer)(VRAM_AUX_BUFFER_START_ADDRESSES[i]);
    }

    initDriver();

    // start with the black screen
    setColor(0, 0, 0);
    g_renderBuffer = g_renderBuffer1;
    fillRect(0, 0, getDisplayWidth() - 1, getDisplayHeight() - 1);
    g_renderBuffer = g_renderBuffer2;
    fillRect(0, 0, getDisplayWidth() - 1, getDisplayHeight() - 1);

#if EEZ_OPTION_GUI_ANIMATIONS
    g_animationBuffer = g_animationBuffer1;
#endif

    g_syncedBuffer = g_renderBuffer1;
    syncBuffer();
}

void turnOn() {
    if (g_displayState != ON && g_displayState != TURNING_ON) {
		g_hooks.turnOnDisplayStart();
    }
}

bool isOn() {
    return g_displayState == ON || g_displayState == TURNING_ON;
}

void turnOff() {
    if (g_displayState != OFF && g_displayState != TURNING_OFF) {
		g_hooks.turnOffDisplayStart();
    }
}

////////////////////////////////////////////////////////////////////////////////

#ifdef GUI_CALC_FPS
bool g_calcFpsEnabled;
bool g_drawFpsGraphEnabled;
uint32_t g_fpsValues[NUM_FPS_VALUES];
uint32_t g_fpsAvg;
static uint32_t g_fpsTotal;
static uint32_t g_lastTimeFPS;

void calcFPS() {
    // calculate last FPS value
	g_fpsTotal -= g_fpsValues[0];

	for (size_t i = 1; i < NUM_FPS_VALUES; i++) {
		g_fpsValues[i - 1] = g_fpsValues[i];
	}

	uint32_t time = millis();
	auto diff = time - g_lastTimeFPS;

	auto fps = diff ? 1000 / diff : 0;
    if (fps > 60) {
        fps = 60;
    }
    g_fpsValues[NUM_FPS_VALUES - 1] = fps;

	g_fpsTotal += g_fpsValues[NUM_FPS_VALUES - 1];
	g_fpsAvg = g_fpsTotal / NUM_FPS_VALUES;
}

void drawFpsGraph(int x, int y, int w, int h, const Style *style) {
	int x1 = x;
	int y1 = y;
	int x2 = x + w - 1;
	int y2 = y + h - 1;
	drawBorderAndBackground(x1, y1, x2, y2, style, style->backgroundColor);

	x1++;
	y1++;
	x2--;
	y2--;

	bool isRed = false;
	display::setColor(style->color);

	x = x1;
	for (size_t i = 0; i < NUM_FPS_VALUES && x <= x2; i++, x++) {
		int y = y2 - g_fpsValues[i] * (y2 - y1) / 60;
		if (y < y1) {
			y = y1;
		}

		if (g_fpsValues[i] < 40) {
			if (!isRed) {
				display::setColor16(COLOR_RED);
				isRed = true;
			}
		} else {
			if (isRed) {
				display::setColor(style->color);
				isRed = false;
			}
		}

		display::drawVLine(x, y, y2 - y);
	}
}
#endif

////////////////////////////////////////////////////////////////////////////////

#if EEZ_OPTION_GUI_ANIMATIONS
static void finishAnimation() {
    g_animationState.enabled = false;

    if (g_renderBuffer == g_renderBuffer1) {
        g_renderBuffer = g_renderBuffer2;
        bitBlt(g_renderBuffer1, 0, 0, getDisplayWidth() - 1, getDisplayHeight() - 1);
    } else {
        g_renderBuffer = g_renderBuffer1;
        bitBlt(g_renderBuffer2, 0, 0, getDisplayWidth() - 1, getDisplayHeight() - 1);
    }

    g_syncedBuffer = g_renderBuffer1;
    syncBuffer();
}
#endif

#if EEZ_OPTION_GUI_ANIMATIONS
void animate(Buffer startBuffer, void (*callback)(float t, VideoBuffer bufferOld, VideoBuffer bufferNew, VideoBuffer bufferDst), float duration) {
    if (g_animationState.enabled) {
        display::finishAnimation();
    }

    g_animationState.enabled = true;
    g_animationState.startTime = 0;
    g_animationState.duration = duration != -1 ? duration : g_hooks.getDefaultAnimationDuration();
    g_animationState.startBuffer = startBuffer;
    g_animationState.callback = callback;
    g_animationState.easingRects = remapOutQuad;
    g_animationState.easingOpacity = remapOutCubic;
}

static void animateStep() {
    uint32_t time = millis();
    if (time == 0) {
        time = 1;
    }
    if (g_animationState.startTime == 0) {
        g_animationState.startTime = time;
    }
    float t = (time - g_animationState.startTime) / (1000.0f * g_animationState.duration);
    if (t < 1.0f) {
		if (g_syncedBuffer == g_animationBuffer1) {
			g_animationBuffer = g_animationBuffer2;
		} else {
			g_animationBuffer = g_animationBuffer1;
		}

        if (g_renderBuffer == g_renderBuffer1) {
            g_animationState.callback(t, g_renderBuffer2, g_renderBuffer1, g_animationBuffer);
        } else {
            g_animationState.callback(t, g_renderBuffer1, g_renderBuffer2, g_animationBuffer);
        }

        g_syncedBuffer = g_animationBuffer;
        syncBuffer();
    } else {
    	finishAnimation();
    }
}
#endif

void update() {
    if (g_displayState == TURNING_ON) {
		g_hooks.turnOnDisplayTick();
    } else if (g_displayState == TURNING_OFF) {
		g_hooks.turnOffDisplayTick();
    } else if (g_displayState == OFF) {
#if EEZ_OPTION_GUI_ANIMATIONS
        if (g_animationState.enabled) {
            display::finishAnimation();
        }
#endif
        osDelay(16);
        sendMessageToGuiThread(GUI_QUEUE_MESSAGE_TYPE_DISPLAY_VSYNC, 0, 0);
        return;
    }

#ifdef GUI_CALC_FPS
	g_lastTimeFPS = millis();
#endif

    display::beginRendering();
    updateScreen();
    display::endRendering();

#ifdef GUI_CALC_FPS
    if (g_calcFpsEnabled) {
        calcFPS();
    }
#endif

#if EEZ_OPTION_GUI_ANIMATIONS
    if (!g_screenshotAllocated && g_animationState.enabled) {
        animateStep();
    } else {
#endif
        g_syncedBuffer = g_renderBuffer;
		syncBuffer();

        if (g_takeScreenshot) {
            copySyncedBufferToScreenshotBuffer();

            g_takeScreenshot = false;
            g_screenshotAllocated = true;
        }
#if EEZ_OPTION_GUI_ANIMATIONS
    }
#endif
}

const uint8_t *takeScreenshot() {
#ifdef __EMSCRIPTEN__
    copySyncedBufferToScreenshotBuffer();
#else
    while (g_screenshotAllocated) {
    }

	g_takeScreenshot = true;

	do {
		osDelay(0);
	} while (g_takeScreenshot);

#endif

    return SCREENSHOOT_BUFFER_START_ADDRESS;
}

void releaseScreenshot() {
    g_screenshotAllocated = false;
}

////////////////////////////////////////////////////////////////////////////////

VideoBuffer getBufferPointer() {
    return g_renderBuffer;
}

void setBufferPointer(VideoBuffer buffer) {
    g_renderBuffer = buffer;
}

void beginRendering() {
    if (g_syncedBuffer == g_renderBuffer1) {
        g_renderBuffer = g_renderBuffer2;
    } else if (g_syncedBuffer == g_renderBuffer2) {
        g_renderBuffer = g_renderBuffer1;
    }

    clearDirty();

    g_mainBufferPointer = getBufferPointer();
    g_numBuffersToDraw = 0;
}

static int g_maxNumBuffersToDraw = 0;

int beginBufferRendering() {
    int bufferIndex = g_numBuffersToDraw++;
    if (g_numBuffersToDraw > g_maxNumBuffersToDraw) {
        g_maxNumBuffersToDraw = g_numBuffersToDraw;
        printf("maxNumBuffersToDraw %d\n", g_maxNumBuffersToDraw);
    }
	g_renderBuffers[bufferIndex].previousBuffer = getBufferPointer();
    setBufferPointer(g_renderBuffers[bufferIndex].bufferPointer);
    return bufferIndex;
}

void endBufferRendering(int bufferIndex, int x, int y, int width, int height, bool withShadow, uint8_t opacity, int xOffset, int yOffset, Rect *backdrop) {
    RenderBuffer &renderBuffer = g_renderBuffers[bufferIndex];

	renderBuffer.x = x;
	renderBuffer.y = y;
	renderBuffer.width = width;
	renderBuffer.height = height;
	renderBuffer.withShadow = withShadow;
	renderBuffer.opacity = opacity;
	renderBuffer.xOffset = xOffset;
	renderBuffer.yOffset = yOffset;
	renderBuffer.backdrop = backdrop;

    setBufferPointer(renderBuffer.previousBuffer);
}

void endRendering() {
    setBufferPointer(g_mainBufferPointer);

#if OPTION_KEYBOARD
    if (keyboard::isDisplayDirty()) {
    	setDirty();
    }
#endif

#if OPTION_MOUSE
    if (mouse::isDisplayDirty()) {
    	setDirty();
    }
#endif

#if defined(GUI_CALC_FPS)
    if (g_drawFpsGraphEnabled) {
	    setDirty();
    }
#endif

    if (isDirty()) {
        for (int bufferIndex = 0; bufferIndex < g_numBuffersToDraw; bufferIndex++) {
            RenderBuffer &renderBuffer = g_renderBuffers[bufferIndex];

            int sx = renderBuffer.x;
            int sy = renderBuffer.y;

            int x1 = renderBuffer.x + renderBuffer.xOffset;
            int y1 = renderBuffer.y + renderBuffer.yOffset;
            int x2 = x1 + renderBuffer.width - 1;
            int y2 = y1 + renderBuffer.height - 1;

            if (renderBuffer.backdrop) {
                // opacity backdrop
                auto savedOpacity = setOpacity(CONF_BACKDROP_OPACITY);
                setColor(COLOR_ID_BACKDROP);
                fillRect(renderBuffer.backdrop->x, renderBuffer.backdrop->y, renderBuffer.backdrop->x + renderBuffer.backdrop->w - 1, renderBuffer.backdrop->y + renderBuffer.backdrop->h - 1);
                setOpacity(savedOpacity);
            }

            if (renderBuffer.withShadow) {
                drawShadow(x1, y1, x2, y2);
            }

            bitBlt(g_renderBuffers[bufferIndex].bufferPointer, nullptr, sx, sy, x2 - x1 + 1, y2 - y1 + 1, x1, y1, renderBuffer.opacity);
        }

#if defined(GUI_CALC_FPS)
        if (g_drawFpsGraphEnabled) {
            drawFpsGraph(getDisplayWidth() - 64 - 4, 4, 64, 32, getStyle(STYLE_ID_FPS_GRAPH));
        }
#endif

#if OPTION_KEYBOARD
        keyboard::updateDisplay();
#endif

#if OPTION_MOUSE
        mouse::updateDisplay();
#endif
    } else {
        if (g_syncedBuffer == g_renderBuffer1) {
            bitBlt(g_renderBuffer1, 0, 0, getDisplayWidth() - 1, getDisplayHeight() - 1);
        } else if (g_syncedBuffer == g_renderBuffer2) {
            bitBlt(g_renderBuffer2, 0, 0, getDisplayWidth() - 1, getDisplayHeight() - 1);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

uint32_t color16to32(uint16_t color, uint8_t opacity) {
    uint32_t color32;
    ((uint8_t *)&color32)[0] = COLOR_TO_R(color);
    ((uint8_t *)&color32)[1] = COLOR_TO_G(color);
    ((uint8_t *)&color32)[2] = COLOR_TO_B(color);
    ((uint8_t *)&color32)[3] = opacity;
    return color32;
}

uint16_t color32to16(uint32_t color) {
    auto pcolor = (uint8_t *)&color;
    return RGB_TO_COLOR(pcolor[0], pcolor[1], pcolor[1]);
}

uint32_t blendColor(uint32_t fgColor, uint32_t bgColor) {
    uint8_t *fg = (uint8_t *)&fgColor;
    uint8_t *bg = (uint8_t *)&bgColor;

    float alphaMult = fg[3] * bg[3] / 255.0f;
    float alphaOut = fg[3] + bg[3] - alphaMult;

    float r = (fg[0] * fg[3] + bg[0] * bg[3] - bg[0] * alphaMult) / alphaOut;
    float g = (fg[1] * fg[3] + bg[1] * bg[3] - bg[1] * alphaMult) / alphaOut;
    float b = (fg[2] * fg[3] + bg[2] * bg[3] - bg[2] * alphaMult) / alphaOut;

    r = clamp(r, 0.0f, 255.0f);
    g = clamp(g, 0.0f, 255.0f);
    b = clamp(b, 0.0f, 255.0f);

    uint32_t result;
    uint8_t *presult = (uint8_t *)&result;
    presult[0] = (uint8_t)r;
    presult[1] = (uint8_t)g;
    presult[2] = (uint8_t)b;
    presult[3] = (uint8_t)alphaOut;

    return result;
}

void onThemeChanged() {
    auto selectedThemeIndex = g_hooks.getSelectedThemeIndex();
    g_themeColors = getThemeColors(selectedThemeIndex);
    g_themeColorsCount = getThemeColorsCount(selectedThemeIndex);
    g_colors = getColors();
}

void onLuminocityChanged() {
    // invalidate cache
    for (int i = 0; i < 256; ++i) {
        g_colorCache[i][0] = 0;
        g_colorCache[i][1] = 0;
        g_colorCache[i][2] = 0;
        g_colorCache[i][3] = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////

#define swap(type, i, j) {type t = i; i = j; j = t;}

void rgbToHsl(float r, float g, float b, float &h, float &s, float &l) {
    r /= 255;
    g /= 255;
    b /= 255;

    float min = r;
    float mid = g;
    float max = b;

    if (min > mid) {
        swap(float, min, mid);
    }
    if (mid > max) {
        swap(float, mid, max);
    }
    if (min > mid) {
        swap(float, min, mid);
    }

    l = (max + min) / 2;

    if (max == min) {
        h = s = 0; // achromatic
    } else {
        float d = max - min;
        s = l > 0.5 ? d / (2 - max - min) : d / (max + min);

        if (max == r) {
            h = (g - b) / d + (g < b ? 6 : 0);
        } else if (max == g) {
            h = (b - r) / d + 2;
        } else if (max == b) {
            h = (r - g) / d + 4;
        }

        h /= 6;
    }
}

float hue2rgb(float p, float q, float t) {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1.0f/6) return p + (q - p) * 6 * t;
    if (t < 1.0f/2) return q;
    if (t < 2.0f/3) return p + (q - p) * (2.0f/3 - t) * 6;
    return p;
}

void hslToRgb(float h, float s, float l, float &r, float &g, float &b) {
    if (s == 0) {
        r = g = b = l; // achromatic
    } else {
        float q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;

        r = hue2rgb(p, q, h + 1.0f/3);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1.0f/3);
    }

    r *= 255;
    g *= 255;
    b *= 255;
}

void adjustColor(uint16_t &c) {
    if (g_hooks.getDisplayBackgroundLuminosityStep() == DISPLAY_BACKGROUND_LUMINOSITY_STEP_DEFAULT) {
        return;
    }

	uint8_t ch = c >> 8;
	uint8_t cl = c & 0xFF;

    int i = (ch & 0xF0) | (cl & 0x0F);
    if (ch == g_colorCache[i][0] && cl == g_colorCache[i][1]) {
        // cache hit!
		c = (g_colorCache[i][2] << 8) | g_colorCache[i][3];
        return;
    }

    uint8_t r, g, b;
    r = ch & 248;
    g = ((ch << 5) | (cl >> 3)) & 252;
    b = cl << 3;

    float h, s, l;
    rgbToHsl(r, g, b, h, s, l);

    float a = l < 0.5 ? l : 1 - l;
    if (a > 0.3f) {
        a = 0.3f;
    }
    float lmin = l - a;
    float lmax = l + a;

    float lNew = remap((float)g_hooks.getDisplayBackgroundLuminosityStep(),
        (float)DISPLAY_BACKGROUND_LUMINOSITY_STEP_MIN,
        lmin,
        (float)DISPLAY_BACKGROUND_LUMINOSITY_STEP_MAX,
        lmax);

    float floatR, floatG, floatB;
    hslToRgb(h, s, lNew, floatR, floatG, floatB);

    r = FLOAT_TO_COLOR_COMPONENT(floatR);
    g = FLOAT_TO_COLOR_COMPONENT(floatG);
    b = FLOAT_TO_COLOR_COMPONENT(floatB);

    uint8_t chNew = RGB_TO_HIGH_BYTE(r, g, b);
    uint8_t clNew = RGB_TO_LOW_BYTE(r, g, b);

    // store new color in the cache
    g_colorCache[i][0] = ch;
    g_colorCache[i][1] = cl;
    g_colorCache[i][2] = chNew;
    g_colorCache[i][3] = clNew;

	c = (chNew << 8) | clNew;
}

uint16_t getColor16FromIndex(uint16_t color) {
    color = g_hooks.transformColor(color);
	return color < g_themeColorsCount ? g_themeColors[color] : g_colors[color - g_themeColorsCount];
}

void setColor(uint8_t r, uint8_t g, uint8_t b) {
    g_fc = RGB_TO_COLOR(r, g, b);
	adjustColor(g_fc);
}

void setColor16(uint16_t color) {
    g_fc = color;
    adjustColor(g_fc);
}

void setColor(uint16_t color, bool ignoreLuminocity) {
    g_fc = getColor16FromIndex(color);
    if (!ignoreLuminocity) {
        adjustColor(g_fc);
    }
}

uint16_t getColor() {
    return g_fc;
}

void setBackColor(uint8_t r, uint8_t g, uint8_t b) {
    g_bc = RGB_TO_COLOR(r, g, b);
	adjustColor(g_bc);
}

void setBackColor(uint16_t color, bool ignoreLuminocity) {
	g_bc = getColor16FromIndex(color);
    if (!ignoreLuminocity) {
	    adjustColor(g_bc);
    }
}

uint16_t getBackColor() {
    return g_bc;
}

uint8_t setOpacity(uint8_t opacity) {
    uint8_t savedOpacity = g_opacity;
    g_opacity = opacity;
    return savedOpacity;
}

uint8_t getOpacity() {
    return g_opacity;
}

////////////////////////////////////////////////////////////////////////////////

void drawHLine(int x, int y, int l) {
    fillRect(x, y, x + l, y);
}

void drawVLine(int x, int y, int l) {
    fillRect(x, y, x, y + l);
}

void drawRect(int x1, int y1, int x2, int y2) {
    drawHLine(x1, y1, x2 - x1);
    drawHLine(x1, y2, x2 - x1);
    drawVLine(x1, y1, y2 - y1);
    drawVLine(x2, y1, y2 - y1);
}

void drawFocusFrame(int x, int y, int w, int h) {
    int lineWidth = MIN(MIN(3, w), h);

    setColor16(RGB_TO_COLOR(255, 0, 255));

    // top
    fillRect(x, y, x + w - 1, y + lineWidth - 1);

    // left
    fillRect(x, y + lineWidth, x + lineWidth - 1, y + h - lineWidth - 1);

    // right
    fillRect(x + w - lineWidth, y + lineWidth, x + w - 1, y + h - lineWidth - 1);

    // bottom
    fillRect(x, y + h - lineWidth, x + w - 1, y + h - 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////

void aggInit(AggDrawing& aggDrawing) {
	aggDrawing.rbuf.attach((uint8_t *)getBufferPointer(), getDisplayWidth(), getDisplayHeight(), getDisplayWidth() * DISPLAY_BPP / 8);
	aggDrawing.graphics.attach(aggDrawing.rbuf.buf(), aggDrawing.rbuf.width(), aggDrawing.rbuf.height(), aggDrawing.rbuf.stride());
}

void drawRoundedRect(
    AggDrawing& aggDrawing,
    int x1, int y1, int x2, int y2,
    int lineWidth,
	int rtlx, int rtly, int rtrx, int rtry,
	int rbrx, int rbry, int rblx, int rbly
) {
    fillRoundedRect(
        aggDrawing,
        x1, y1, x2, y2,
        lineWidth,
        rtlx, rtly, rtrx, rtry,
	    rbrx, rbry, rblx, rbly,
        true, false
    );
}

void fillRect(
    int x1, int y1, int x2, int y2,
    int clip_x1, int clip_y1, int clip_x2, int clip_y2
) {
    if (clip_x1 != -1) {
        x1 = MAX(x1, clip_x1);
        x2 = MIN(x2, clip_x2);
        y1 = MAX(y1, clip_y1);
        y2 = MIN(y2, clip_y2);
    }
    fillRect(x1, y1, x2, y2);
}

void fillRoundedRect(
    AggDrawing& aggDrawing,
    int x1, int y1, int x2, int y2,
    int lineWidth,
    int rtlx, int rtly, int rtrx, int rtry,
    int rbrx, int rbry, int rblx, int rbly,
    bool drawLine, bool fill,
    int clip_x1, int clip_y1, int clip_x2, int clip_y2
) {
#ifdef CONF_FAST_ROUND_RECT
	if (
		rtlx == rtly && rtly == rtrx && rtrx == rtry && rtry == rbrx && rbrx == rbry && rbry == rblx && rblx == rbly // all radiuses are the same
		// && clip_x1 == -1 // no clipping
	) {
		int r = rtlx;
		int border = lineWidth;

		if (border == 0) {
			drawLine = 0;
		}

		int w = x2 - x1 + 1;
		int h = y2 - y1 + 1;

		int x = MIN(w, h);
		if (r > x / 2.0f) {
			r = floorf(x / 2.0f);
		}

		int r_inner = r - border;

		int xc1 = x2 - r + 1;
		int yc1 = y1 + r;

		int xc2 = x1 + r;
		int yc2 = y1 + r;

		int xc3 = x1 + r;
		int yc3 = y2 - r + 1;

		int xc4 = x2 - r + 1;
		int yc4 = y2 - r + 1;

		auto fc_save = g_fc;

		uint8_t fc[3] = { COLOR_TO_R(g_fc), COLOR_TO_G(g_fc), COLOR_TO_B(g_fc) };
		uint8_t bc[3] = { COLOR_TO_R(g_bc), COLOR_TO_G(g_bc), COLOR_TO_B(g_bc) };

		float op = g_opacity / 255.0f;
		float a1_op;
		float a2_op;
		float a3_op;
		float r1, g1, b1;
		uint8_t dest_r, dest_g, dest_b;

		#define DRAW_PIXEL(x, y, c1, a1, c2, a2) \
			getPixel(x, y, &dest_r, &dest_g, &dest_b); \
			a1_op = a1 * op; \
			a2_op = a2 * op; \
			a3_op = 1 - (a1 + a2); \
			r1 = (c1)[0] * a1_op + (c2)[0] * a2_op + dest_r * a3_op; \
			g1 = (c1)[1] * a1_op + (c2)[1] * a2_op + dest_g * a3_op; \
			b1 = (c1)[2] * a1_op + (c2)[2] * a2_op + dest_b * a3_op; \
			g_fc = RGB_TO_COLOR((int)r1, (int)g1, (int)b1); \
			if (clip_x1 == -1 || (x >= clip_x1 && x <= clip_x2 && y >= clip_y1 && y <= clip_y2)) drawPixel(x, y) \

		#define DRAW_4(x, y, a1, a2) \
			DRAW_PIXEL(xc1  + (x)     , yc1 -  (y)     , drawLine ? fc : bc, (a1), bc, (a2)); \
			DRAW_PIXEL(xc2 - ((x) + 1), yc2 -  (y)     , drawLine ? fc : bc, (a1), bc, (a2)); \
			DRAW_PIXEL(xc3 - ((x) + 1), yc3 + ((y) - 1), drawLine ? fc : bc, (a1), bc, (a2)); \
			DRAW_PIXEL(xc4  + (x)     , yc4 + ((y) - 1), drawLine ? fc : bc, (a1), bc, (a2)); \

		display::startPixelsDraw();

		int ffd = roundf(r / sqrtf(2.0f));
		for (int x = 0; x < ffd; x++) {
			float yr = sqrtf(r * r - (x + 0.5f) * (x + 0.5f));
			int y = ceilf(yr);
			float a1 = 1 - (y - yr);

			float yr_inner = drawLine && x < r_inner ? sqrtf(r_inner * r_inner - (x + 0.5f) * (x + 0.5f)) : 0;
			int y_inner = ceilf(yr_inner);
			float a2 = 1 - (y_inner - yr_inner);

			if (y > 0) {
                DRAW_4(x, y, a1, 0);
            }
			DRAW_4(y - 1, x + 1, a1, 0);

			for (y = y - 1; y > y_inner; y--) {
				DRAW_4(x, y, 1.0, 0);
				DRAW_4(y - 1, x + 1, 1.0, 0);
			}

			if (y > 0) {
				DRAW_4(x, y, 1 - a2, fill ? a2 : 0);
				DRAW_4(y - 1, x + 1, 1 - a2, fill ? a2 : 0);

                if (fill) {
                    for (y = y - 1; y > 0; y--) {
                        DRAW_4(x, y, 0.0, 1.0);
                        DRAW_4(y - 1, x + 1, 0.0, 1.0);
                    }
                }
			}
		}

		display::endPixelsDraw();

		g_fc = g_bc;

		// background
		if (fill) {
			if (drawLine) {
				fillRect(x1 + r, y1 + border, x2 - r, y2 - border, clip_x1, clip_y1, clip_x2, clip_y2); // from top to bottom
				fillRect(x1 + border, y1 + r, x1 + r - 1, y2 - r, clip_x1, clip_y1, clip_x2, clip_y2); // left
				fillRect(x2 - r + 1, y1 + r, x2 - border, y2 - r, clip_x1, clip_y1, clip_x2, clip_y2); // right
			} else {
				fillRect(x1 + r, y1, x2 - r, y2); // from top to bottom
				fillRect(x1, y1 + r, x1 + r - 1, y2 - r, clip_x1, clip_y1, clip_x2, clip_y2); // left
				fillRect(x2 - r + 1, y1 + r, x2, y2 - r, clip_x1, clip_y1, clip_x2, clip_y2); // right
			}
		}

		g_fc = fc_save;

		// border
		if (drawLine) {
			fillRect(x1 + r, y1, x2 - r, y1 + border - 1, clip_x1, clip_y1, clip_x2, clip_y2); // top
			fillRect(x1 + r, y2 - border + 1, x2 - r, y2, clip_x1, clip_y1, clip_x2, clip_y2); // bottom
			fillRect(x1, y1 + r, x1 + border - 1, y2 - r, clip_x1, clip_y1, clip_x2, clip_y2); // left
			fillRect(x2 - border + 1, y1 + r, x2, y2 - r, clip_x1, clip_y1, clip_x2, clip_y2); // right
		}
	} else {
#endif
        // use AGG, slower

        auto &graphics = aggDrawing.graphics;

        if (clip_x1 != -1) {
            graphics.clipBox(clip_x1, clip_y1, clip_x2 + 1, clip_y2 + 1);
        } else {
            graphics.clipBox(x1, y1, x2 + 1, y2 + 1);
        }
        graphics.masterAlpha(g_opacity / 255.0);
        graphics.translate(x1, y1);
        graphics.lineWidth(lineWidth);
        if (lineWidth > 0 && drawLine) {
            graphics.lineColor(COLOR_TO_R(g_fc), COLOR_TO_G(g_fc), COLOR_TO_B(g_fc));
        } else {
            graphics.noLine();
        }
        if (fill) {
            graphics.fillColor(COLOR_TO_R(g_bc), COLOR_TO_G(g_bc), COLOR_TO_B(g_bc));
        } else {
            graphics.noFill();
        }
        auto w = x2 - x1 + 1;
        auto h = y2 - y1 + 1;
        graphics.roundedRect(
            lineWidth / 2.0, lineWidth / 2.0, w - lineWidth, h - lineWidth,
            rtlx, rtly, rtrx, rtry, rbrx, rbry, rblx, rbly
        );

        graphics.translate(-x1, -y1);
        graphics.clipBox(0, 0, aggDrawing.rbuf.width(), aggDrawing.rbuf.height());
#ifdef CONF_FAST_ROUND_RECT
    }
#endif
}

void fillRoundedRect(
    AggDrawing& aggDrawing,
	int x1, int y1, int x2, int y2,
	int lineWidth,
	int r,
	bool drawLine, bool fill,
	int clip_x1, int clip_y1, int clip_x2, int clip_y2
) {
	fillRoundedRect(aggDrawing, x1, y1, x2, y2, lineWidth, r, r, r, r, r, r, r, r, drawLine, fill, clip_x1, clip_y1, clip_x2, clip_y2);
}

////////////////////////////////////////////////////////////////////////////////

static int8_t measureGlyph(int32_t encoding) {
    auto glyph = g_font.getGlyph(encoding);
    if (!glyph)
        return 0;

    return glyph->dx;
}

int8_t measureGlyph(int32_t encoding, gui::font::Font &font) {
    auto glyph = font.getGlyph(encoding);
    if (!glyph)
        return 0;

    return glyph->dx;
}

int measureStr(const char *text, int textLength, gui::font::Font &font, int max_width) {
    g_font = font;

    int width = 0;

    if (textLength == -1) {
        while (true) {
            utf8_int32_t encoding;
            text = utf8codepoint(text, &encoding);
            if (!encoding) {
                break;
            }
            int glyph_width = measureGlyph(encoding);
            if (max_width > 0 && width + glyph_width > max_width) {
                return max_width;
            }
            width += glyph_width;
        }
    } else {
        for (int i = 0; i < textLength; ++i) {
            utf8_int32_t encoding;
            text = utf8codepoint(text, &encoding);
            if (!encoding) {
                break;
            }
            int glyph_width = measureGlyph(encoding);
            if (max_width > 0 && width + glyph_width > max_width) {
                return max_width;
            }
            width += glyph_width;
        }
    }

    return width;
}

void drawStr(const char *text, int textLength, int x, int y, int clip_x1, int clip_y1, int clip_x2, int clip_y2, gui::font::Font &font, int cursorPosition) {
    g_font = font;

    drawStrInit();

    if (textLength == -1) {
        textLength = utf8len(text);
    }

    int xCursor = x;

    int i;

    for (i = 0; i < textLength; ++i) {
        utf8_int32_t encoding;
        text = utf8codepoint(text, &encoding);
        if (!encoding) {
            break;
        }

        if (i == cursorPosition) {
            xCursor = x;
        }

        auto x1 = x;
        auto y1 = y;

        auto glyph = g_font.getGlyph(encoding);
        if (glyph) {
            int x_glyph = x1 + glyph->x;
            int y_glyph = y1 + g_font.getAscent() - (glyph->y + glyph->height);

            // draw glyph pixels
            int iStartByte = 0;
            if (x_glyph < clip_x1) {
                int dx_off = clip_x1 - x_glyph;
                iStartByte = dx_off;
                x_glyph = clip_x1;
            }

			if (iStartByte < glyph->width) {
				int offset = 0;
				int glyphHeight = glyph->height;
				if (y_glyph < clip_y1) {
					int dy_off = clip_y1 - y_glyph;
					offset += dy_off * glyph->width;
					glyphHeight -= dy_off;
					y_glyph = clip_y1;
				}

				int width;
				if (x_glyph + (glyph->width - iStartByte) - 1 > clip_x2) {
					width = clip_x2 - x_glyph + 1;
				} else {
					width = (glyph->width - iStartByte);
				}

				int height;
				if (y_glyph + glyphHeight - 1 > clip_y2) {
					height = clip_y2 - y_glyph + 1;
				} else {
					height = glyphHeight;
				}

				if (width > 0 && height > 0) {
					drawGlyph(glyph->pixels + offset + iStartByte, glyph->width - width, x_glyph, y_glyph, width, height);
				}
			}

			x += glyph->dx;
		}
    }

    if (i == cursorPosition) {
        xCursor = x;
    }

    if (cursorPosition != -1 && xCursor - CURSOR_WIDTH / 2 >= clip_x1 && xCursor + CURSOR_WIDTH / 2 - 1 <= clip_x2) {
        auto d = MAX(((clip_y2 - clip_y1) - font.getHeight()) / 2, 0);
        fillRect(xCursor - CURSOR_WIDTH / 2, clip_y1 + d, xCursor + CURSOR_WIDTH / 2 - 1, clip_y2 - d);
    }

    setDirty();
}

int getCharIndexAtPosition(int xPos, const char *text, int textLength, int x, int y, int clip_x1, int clip_y1, int clip_x2,int clip_y2, gui::font::Font &font) {
    if (textLength == -1) {
        textLength = utf8len(text);
    }

    int i;

    for (i = 0; i < textLength; ++i) {
        utf8_int32_t encoding;
        text = utf8codepoint(text, &encoding);
        if (!encoding) {
            break;
        }

        auto glyph = font.getGlyph(encoding);
        auto dx = 0;
        if (glyph) {
            dx = glyph->dx;
        }
        if (xPos < x + dx / 2) {
            return i;
        }
        x += dx;
    }

    return i;
}

int getCursorXPosition(int cursorPosition, const char *text, int textLength, int x, int y, int clip_x1, int clip_y1, int clip_x2,int clip_y2, gui::font::Font &font) {
    if (textLength == -1) {
        textLength = utf8len(text);
    }

    for (int i = 0; i < textLength; ++i) {
        utf8_int32_t encoding;
        text = utf8codepoint(text, &encoding);
        if (!encoding) {
            break;
        }

        if (i == cursorPosition) {
            return x;
        }

        auto glyph = font.getGlyph(encoding);
        if (glyph) {
            x += glyph->dx;
        }
    }

    return x;
}

} // namespace display
} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
