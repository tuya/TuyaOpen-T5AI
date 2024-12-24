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

#include <eez/core/sound.h>
#include <eez/core/util.h>

#include <eez/gui/gui.h>
#include <eez/gui/widgets/slider.h>

namespace eez {
namespace gui {

bool SliderWidgetState::updateState() {
    WIDGET_STATE_START(SliderWidget);

    WIDGET_STATE(value, get(widgetCursor, widget->data));
    WIDGET_STATE(minValue, get(widgetCursor, widget->min));
    WIDGET_STATE(maxValue, get(widgetCursor, widget->max));

    WIDGET_STATE_END()
}

void SliderWidgetState::render() {
    const WidgetCursor &widgetCursor = g_widgetCursor;
    auto widget = (const ButtonWidget *)widgetCursor.widget;
    const Style *style = getStyle(widget->style);

	drawRectangle(
		widgetCursor.x, widgetCursor.y, widgetCursor.w, widgetCursor.h,
		nullptr,
		false,
		false
	);

    auto x = widgetCursor.x + style->paddingLeft;
    auto y = widgetCursor.y + style->paddingTop;
    auto w = widgetCursor.w - style->paddingLeft - style->paddingRight;
    auto h = widgetCursor.h - style->paddingTop - style->paddingBottom;

    auto barX = x + h / 2;
    auto barW = w - h;
    auto barH = (h * 8) / 20;
	auto barY = y + (h - barH) / 2;
	auto barBorderRadius = barH / 2;

    double knobRelativePosition = (value.toDouble() - minValue.toDouble()) / (maxValue.toDouble() - minValue.toDouble());
    if (knobRelativePosition < 0) knobRelativePosition = 0;
    else if (knobRelativePosition > 1.0f) knobRelativePosition = 1.0f;

	int knobPosition = (int)round(barX + knobRelativePosition * (barW - 1));

    auto knobRadius = h / 2;
    auto knobX = knobPosition;
    auto knobY = y;
    auto knobW = h;
    auto knobH = h;

	display::setColor(style->borderColor);

    display::AggDrawing aggDrawing;
    display::aggInit(aggDrawing);

    // render bar
    display::setBackColor(style->backgroundColor);
	display::fillRoundedRect(aggDrawing, barX - barBorderRadius, barY, barX + barW + barBorderRadius - 1, barY + barH - 1, style->borderSizeLeft, barBorderRadius, false, true,
		x, y, x + w - 1, y + h - 1);

    // render knob
	display::setBackColor(style->color);
	display::fillRoundedRect(aggDrawing, knobX - knobRadius, knobY, knobX - knobRadius + knobW - 1, knobY + knobH - 1, style->borderSizeLeft, knobRadius, false, true,
		x, y, x + w - 1, y + h - 1);
}

bool SliderWidgetState::hasOnTouch() {
    return true;
}

void SliderWidgetState::onTouch(const WidgetCursor &widgetCursor, Event &touchEvent) {
	auto widget = (const ButtonWidget *)widgetCursor.widget;
	const Style *style = getStyle(widget->style);

	double x = widgetCursor.x + style->paddingLeft;
    double w = widgetCursor.w - style->paddingLeft - style->paddingRight;
    double h = widgetCursor.h - style->paddingTop - style->paddingBottom;

    double knobW = h;

    double barX = x + h / 2.0;
    double barW = w - h;

    if (touchEvent.type == EVENT_TYPE_TOUCH_DOWN) {
		double knobRelativePosition = (value.toDouble() - minValue.toDouble()) / (maxValue.toDouble() - minValue.toDouble());
		if (knobRelativePosition < 0) knobRelativePosition = 0;
		else if (knobRelativePosition > 1.0f) knobRelativePosition = 1.0f;

		double knobPosition = barX + knobRelativePosition * (barW - 1);

		if (abs(touchEvent.x - knobPosition) < knobW * 2) {
            dragging = true;
            dragOrigin = touchEvent.x;

            dragStartPosition = knobPosition;
        }
    } else if (touchEvent.type == EVENT_TYPE_TOUCH_MOVE) {
		if (dragging) {
			double knobPosition = dragStartPosition + (touchEvent.x - dragOrigin);
			double knobRelativePosition = (knobPosition - barX) / (barW - 1);

			double min = minValue.toDouble();
			double max = maxValue.toDouble();
			double newValue = min + knobRelativePosition * (max - min);
			if (newValue < min) newValue = min;
			else if (newValue > max) newValue = max;

			Value tempValue = value;
			assignValue(tempValue, Value(newValue, VALUE_TYPE_DOUBLE));

			set(widgetCursor, widgetCursor.widget->data, tempValue);
		}
	} else if (touchEvent.type == EVENT_TYPE_TOUCH_UP) {
		dragging = false;
	}
}


} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
