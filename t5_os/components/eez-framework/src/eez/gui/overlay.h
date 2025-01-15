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

namespace eez {
namespace gui {

enum OverlayVisibiliy {
	OVERLAY_MINIMIZED = (1 << 0),
	OVERLAY_HIDDEN = (1 << 1)
};

struct WidgetOverride {
    bool isVisible;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
};

struct Overlay {
    int x;
    int y;
    int width;
    int height;

    int state; // if 0 then overlay is not visible
    WidgetOverride *widgetOverrides;

    bool moved = false;

	int visibility = 0;

    int xOffsetMinimized;
    int yOffsetMinimized;

	int xOffsetMaximized;
	int yOffsetMaximized;

	int xOffsetOnTouchDown;
    int yOffsetOnTouchDown;

    int xOnTouchDown;
    int yOnTouchDown;
};

bool isOverlay(const WidgetCursor &widgetCursor);
Overlay *getOverlay(const WidgetCursor &widgetCursor);
void getOverlayOffset(const WidgetCursor &widgetCursor, int &xOffset, int &yOffset);
void dragOverlay(Event &touchEvent);

extern int g_xOverlayOffset;
extern int g_yOverlayOffset;

} // namespace gui
} // namespace eez
