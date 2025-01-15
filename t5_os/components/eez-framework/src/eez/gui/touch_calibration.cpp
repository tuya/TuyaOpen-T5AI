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

#if OPTION_TOUCH_CALIBRATION

#include <eez/core/os.h>
#include <eez/core/sound.h>

#include <eez/gui/gui.h>
#include <eez/gui/touch_filter.h>

#include <eez/gui/touch_calibration.h>

#define CONF_GUI_TOUCH_CALIBRATION_M 17
#define TOUCH_POINT_ACTIVATION_THRESHOLD 200

////////////////////////////////////////////////////////////////////////////////

namespace eez {
namespace gui {

static AppContext *g_appContext;

static uint32_t g_pointStartTime;

static struct {
    int x;
    int y;
} g_points[3];
static int g_currentPoint;

bool isTouchCalibrated() {
#if defined(SKIP_TOUCH_CALIBRATION) || defined(EEZ_PLATFORM_STM32F469I_DISCO)
    return true;
#else
    bool success;

	int16_t touchScreenCalTlx;
	int16_t touchScreenCalTly;
	int16_t touchScreenCalBrx;
	int16_t touchScreenCalBry;
	int16_t touchScreenCalTrx;
	int16_t touchScreenCalTry;

	g_hooks.getTouchScreenCalibrationParams(
		touchScreenCalTlx, touchScreenCalTly,
		touchScreenCalBrx, touchScreenCalBry,
		touchScreenCalTrx, touchScreenCalTry
	);

    success = touch::calibrateTransform(
        touchScreenCalTlx, touchScreenCalTly,
        touchScreenCalBrx, touchScreenCalBry,
        touchScreenCalTrx, touchScreenCalTry,
        CONF_GUI_TOUCH_CALIBRATION_M, display::getDisplayWidth(), display::getDisplayHeight());
    return success;
#endif
}

void startCalibration() {
    touch::resetTransformCalibration();
    g_currentPoint = 0;
    g_pointStartTime = millis();
}

void enterTouchCalibration(AppContext *appContext) {
    g_appContext = appContext;
	g_hooks.onEnterTouchCalibration();
    startCalibration();
}

void touchCalibrationDialogYes() {
	g_hooks.setTouchScreenCalibrationParams(g_points[0].x, g_points[0].y, g_points[1].x, g_points[1].y, g_points[2].x, g_points[2].y);
	g_hooks.onTouchCalibrationOk();
}

void touchCalibrationDialogNo() {
    startCalibration();
}

void touchCalibrationDialogCancel() {
	g_hooks.onTouchCalibrationCancel();
}

void selectTouchCalibrationPoint(Event &touchEvent) {
    g_points[g_currentPoint].x = touchEvent.x;
    g_points[g_currentPoint].y = touchEvent.y;

    g_currentPoint++;
    g_pointStartTime = millis();

    if (g_currentPoint == 3) {
        g_currentPoint = 0;

        bool success = touch::calibrateTransform(
            g_points[0].x, g_points[0].y, g_points[1].x, g_points[1].y, g_points[2].x,
            g_points[2].y, CONF_GUI_TOUCH_CALIBRATION_M, display::getDisplayWidth(), display::getDisplayHeight());

        if (success) {
			g_hooks.onTouchCalibrationConfirm();
        } else {
            startCalibration();
			g_appContext->errorMessage("Received data is invalid due to\nimprecise pointing or\ncommunication problem!", true);
        }
    }
}

bool isTouchPointActivated() {
    return millis() - g_pointStartTime > TOUCH_POINT_ACTIVATION_THRESHOLD;
}

void findActiveWidget() {
    if (g_activeWidget) {
        return;
    }

#if OPTION_TOUCH_CALIBRATION
    const WidgetCursor& widgetCursor = g_widgetCursor;
    if (widgetCursor.appContext->getActivePageId() == PAGE_ID_TOUCH_CALIBRATION) {
        if (widgetCursor.widget->type == WIDGET_TYPE_TEXT) {
            g_activeWidget = widgetCursor;
        }
    }
#endif
}

void onTouchCalibrationPageTouch(const WidgetCursor &foundWidget, Event &touchEvent) {
    if (touchEvent.type == EVENT_TYPE_TOUCH_DOWN) {
        g_pointStartTime = millis();
    } else if (touchEvent.type == EVENT_TYPE_TOUCH_MOVE) {
        if (!g_activeWidget && isTouchPointActivated()) {
			forEachWidget(findActiveWidget);
        }
    } else if (touchEvent.type == EVENT_TYPE_TOUCH_UP) {
        if (isTouchPointActivated()) {
            g_activeWidget = 0;
            selectTouchCalibrationPoint(touchEvent);
        }
    }
}

void data_touch_calibration_point(DataOperationEnum operation, const WidgetCursor& widgetCursor, Value &value) {
    if (operation == DATA_OPERATION_GET) {
        value = Value(g_currentPoint);
    }
}

} // namespace gui
} // namespace eez

#endif

#endif // EEZ_OPTION_GUI
