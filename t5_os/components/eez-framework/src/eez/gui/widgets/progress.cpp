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

#include <eez/core/util.h>

#include <eez/gui/gui.h>
#include <eez/gui/widgets/progress.h>

namespace eez {
namespace gui {

static const uint8_t PROGRESS_WIDGET_ORIENTATION_HORIZONTAL = 0;
// static const uint8_t PROGRESS_WIDGET_ORIENTATION_VERTICAL = 1;

bool ProgressWidgetState::updateState() {
    WIDGET_STATE_START(ProgressWidget);

    WIDGET_STATE(data, get(widgetCursor, widget->data));
    WIDGET_STATE(min, get(widgetCursor, widget->min));
    WIDGET_STATE(max, get(widgetCursor, widget->max));

    WIDGET_STATE_END()
}

void ProgressWidgetState::render() {
    const WidgetCursor &widgetCursor = g_widgetCursor;

    auto widget = (const ProgressWidget *)widgetCursor.widget;
    const Style* style = getStyle(widget->style);

    drawRectangle(widgetCursor.x, widgetCursor.y, widgetCursor.w, widgetCursor.h, style);

    int percentFrom;
    int percentTo;

    if (widgetCursor.flowState) {
        float fmin = min.toFloat();
        float fmax = max.toFloat();
        float value = data.toFloat();
        percentFrom = 0;
        percentTo = (value - fmin) * 100.0f / (fmax - fmin);
    } else {
        if (data.getType() == VALUE_TYPE_RANGE) {
            percentFrom = data.getRangeFrom();
            percentTo = data.getRangeTo();
        } else {
            percentFrom = 0;
            percentTo = data.getInt();
        }
    }

    percentFrom = clamp(percentFrom, 0, 100.0f);
    percentTo = clamp(percentTo, 0, 100.0f);
    if (percentFrom > percentTo) {
        percentFrom = percentTo;
    }

    auto isHorizontal = widget->orientation == PROGRESS_WIDGET_ORIENTATION_HORIZONTAL;
    if (isHorizontal) {
        auto xFrom = percentFrom * widgetCursor.w / 100;
        auto xTo = percentTo * widgetCursor.w / 100;
        if (g_isRTL) {
            drawRectangle(widgetCursor.x + widgetCursor.w - xTo, widgetCursor.y, xTo - xFrom, widgetCursor.h, style, true);
        } else {
            drawRectangle(widgetCursor.x + xFrom, widgetCursor.y, xTo - xFrom, widgetCursor.h, style, true);
        }
    } else {
        auto yFrom = percentFrom * widgetCursor.h / 100;
        auto yTo = percentTo * widgetCursor.h / 100;
        drawRectangle(widgetCursor.x, widgetCursor.y + widgetCursor.h - yTo, widgetCursor.w, yTo - yFrom, style, true);
    }
}

} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
