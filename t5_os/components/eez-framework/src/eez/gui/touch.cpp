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

#if !defined(EEZ_FOR_LVGL)

#include <eez/core/os.h>

#include <eez/gui/gui.h>
#include <eez/gui/touch_filter.h>
#include <eez/gui/thread.h>

#include <eez/platform/touch.h>

////////////////////////////////////////////////////////////////////////////////

namespace eez {
namespace gui {
namespace touch {

static Event g_lastEvent;

static int g_calibratedX = -1;
static int g_calibratedY = -1;
static bool g_calibratedPressed = false;

static int g_filteredX = -1;
static int g_filteredY = -1;
static bool g_filteredPressed = false;

////////////////////////////////////////////////////////////////////////////////

void tick() {
	bool pressed;
	int x;
	int y;
	mcu::touch::read(pressed, x, y);

#if defined(EEZ_PLATFORM_STM32) && !defined(EEZ_PLATFORM_STM32F469I_DISCO)
	g_calibratedPressed = pressed;
	g_calibratedX = x;
	g_calibratedY = y;
	transform(g_calibratedX, g_calibratedY);

	g_filteredX = g_calibratedX;
	g_filteredY = g_calibratedY;
	g_filteredPressed = filter(g_calibratedPressed, g_filteredX, g_filteredY);

	pressed = g_filteredPressed;
	x = g_filteredX;
	y = g_filteredY;
#endif

	onTouchEvent(pressed, x, y, g_lastEvent);
}

Event &getLastEvent() {
    return g_lastEvent;
}

} // namespace touch

void data_touch_calibrated_x(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value) {
    if (operation == DATA_OPERATION_GET) {
        value = touch::g_calibratedX;
    }
}

void data_touch_calibrated_y(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value) {
    if (operation == DATA_OPERATION_GET) {
        value = touch::g_calibratedY;
    }
}

void data_touch_calibrated_pressed(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value) {
    if (operation == DATA_OPERATION_GET) {
        value = touch::g_calibratedPressed;
    }
}

void data_touch_filtered_x(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value) {
    if (operation == DATA_OPERATION_GET) {
        value = touch::g_filteredX;
    }
}

void data_touch_filtered_y(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value) {
    if (operation == DATA_OPERATION_GET) {
        value = touch::g_filteredY;
    }
}

void data_touch_filtered_pressed(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value) {
    if (operation == DATA_OPERATION_GET) {
        value = touch::g_filteredPressed;
    }
}

} // namespace gui
} // namespace eez

#endif

#endif // EEZ_OPTION_GUI
