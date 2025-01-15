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

namespace eez {
namespace gui {

struct Rect {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
};

inline bool operator != (const Rect& r1, const Rect& r2) {
    return r1.x != r2.x || r1.y != r2.y || r1.w != r2.w || r1.h != r2.h;
}

inline bool operator == (const Rect& r1, const Rect& r2) {
    return !(r1 != r2);
}

struct PointF {
    float x;
    float y;
};

struct RectangleF {
    float left;
    float top;
    float right;
    float bottom;
};

bool clipSegment(RectangleF r, PointF &p1, PointF &p2);

} // namespace gui
} // namespace eez
