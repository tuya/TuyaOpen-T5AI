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

#if EEZ_OPTION_GUI_ANIMATIONS

namespace eez {
namespace gui {

enum Buffer {
    BUFFER_OLD,
    BUFFER_NEW,
    BUFFER_SOLID_COLOR
};

enum Opacity {
    OPACITY_SOLID,
    OPACITY_FADE_IN,
    OPACITY_FADE_OUT
};

enum Position {
    POSITION_TOP_LEFT,
    POSITION_TOP,
    POSITION_TOP_RIGHT,
    POSITION_LEFT,
    POSITION_CENTER,
    POSITION_RIGHT,
    POSITION_BOTTOM_LEFT,
    POSITION_BOTTOM,
    POSITION_BOTTOM_RIGHT
};

struct AnimationState {
    bool enabled;
    uint32_t startTime;
    float duration;
    Buffer startBuffer;
    void (*callback)(float t, VideoBuffer bufferOld, VideoBuffer bufferNew, VideoBuffer bufferDst);
    float (*easingRects)(float x, float x1, float y1, float x2, float y2);
    float (*easingOpacity)(float x, float x1, float y1, float x2, float y2);
};

struct AnimRect {
    Buffer buffer;
    Rect srcRect;
    Rect dstRect;
    uint16_t color;
    Opacity opacity;
    Position position;
};

extern AnimationState g_animationState;

#define MAX_ANIM_RECTS 10

extern AnimRect g_animRects[MAX_ANIM_RECTS];

void animateOpen(const Rect &srcRect, const Rect &dstRect);
void animateClose(const Rect &srcRect, const Rect &dstRect);
void animateRects(AppContext *appContext, Buffer startBuffer, int numRects, float duration = -1, const Rect *clipRect = nullptr);

} // gui
} // eez

#endif // EEZ_OPTION_GUI_ANIMATIONS
