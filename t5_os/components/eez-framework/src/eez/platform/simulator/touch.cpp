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

#if defined(EEZ_PLATFORM_SIMULATOR) || defined(__EMSCRIPTEN__)

#if EEZ_OPTION_GUI

#include <eez/platform/touch.h>

#include <eez/platform/simulator/events.h>

#include <eez/gui/gui.h>

#if OPTION_MOUSE
#include <eez/core/mouse.h>
#endif

using namespace eez::platform::simulator;

namespace eez {
namespace mcu {
namespace touch {

void read(bool &isPressed, int &x, int &y) {
    readEvents();

    using namespace eez::gui;

#if OPTION_MOUSE
    if (mouse::isMouseEnabled()) {
        mouse::onMouseEvent(g_mouseButton1IsPressed, g_mouseX, g_mouseY);

        isPressed = false;
        x = 0;
        y = 0;
    }
    else {
#endif
		isPressed = g_mouseButton1IsPressed;
        x = g_mouseX;
        y = g_mouseY;
#if OPTION_MOUSE
	}
#endif
}

} // namespace touch
} // namespace mcu

namespace gui {

void data_touch_raw_x(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value) {
    if (operation == DATA_OPERATION_GET) {
        value = g_mouseX;
    }
}

void data_touch_raw_y(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value) {
    if (operation == DATA_OPERATION_GET) {
        value = g_mouseY;
    }
}

void data_touch_raw_z1(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value) {
    if (operation == DATA_OPERATION_GET) {
        value = 0;
    }
}

void data_touch_raw_pressed(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value) {
    if (operation == DATA_OPERATION_GET) {
        value = g_mouseButton1IsPressed;
    }
}

} // namespace gui

} // namespace eez

#endif // EEZ_OPTION_GUI

#endif // defined(EEZ_PLATFORM_SIMULATOR)
