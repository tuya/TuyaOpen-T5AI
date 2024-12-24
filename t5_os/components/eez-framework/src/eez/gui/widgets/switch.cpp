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
#include <eez/gui/widgets/switch.h>

static const uint32_t ANIMATION_DURATION = 150;

namespace eez {
namespace gui {

bool SwitchWidgetState::updateState() {
    WIDGET_STATE_START(SwitchWidget);

	if (changeStartTime != 0) {
		if (millis() - changeStartTime > ANIMATION_DURATION) {
			changeStartTime = 0;
		}
		hasPreviousState = false;
	}

    WIDGET_STATE(data, widget->data ? get(widgetCursor, widget->data) : 0);

    WIDGET_STATE_END()
}

void SwitchWidgetState::render() {
    const WidgetCursor &widgetCursor = g_widgetCursor;
    auto widget = (const ButtonWidget *)widgetCursor.widget;
    const Style *style = getStyle(widget->style);

	drawRectangle(
		widgetCursor.x, widgetCursor.y, widgetCursor.w, widgetCursor.h,
		nullptr,
		false,
		false
	);

    auto enabled = data.getInt() ? true : false;

	float t;
	if (changeStartTime) {
		t = 1.0f * (millis() - changeStartTime) / ANIMATION_DURATION;
		if (t > 1.0f) {
			t = 1.0f;
		}
		if (!enabled) {
			t = 1.0f - t;
		}
	} else {
		t = enabled ? 1.0f : 0;
	}

    auto x = widgetCursor.x + style->paddingLeft;
	auto y = widgetCursor.y + style->paddingTop;
	auto w =
        widgetCursor.w -
        style->paddingLeft -
        style->paddingRight;
	auto h =
        widgetCursor.h -
        style->paddingTop -
        style->paddingBottom;

    display::AggDrawing aggDrawing;
    display::aggInit(aggDrawing);

    display::setColor(style->borderColor);

    int pos = (int)floorf(x + t * (w - 1));

	display::setBackColor(style->backgroundColor);
    display::fillRoundedRect(
		aggDrawing,
		x, y, x + w - 1, y + h - 1,
		style->borderSizeLeft, h, true, true,
        pos, y, x + w - 1, y + h - 1
	);

	display::setBackColor(style->activeBackgroundColor);
	display::fillRoundedRect(
		aggDrawing,
		x, y, x + w - 1, y + h - 1,
		style->borderSizeLeft, h, true, true,
		x, y, pos - 1, y + h - 1
	);

    // draw knob
	y += 2 + style->borderSizeLeft;
	h -= 2 * (2 + style->borderSizeLeft);

	auto xDisabled = x + 1 + style->borderSizeLeft;
	auto xEnabled = x + w - h - (1 + style->borderSizeLeft);
    x = (int)round(xDisabled + t * (xEnabled - xDisabled));

    w = h;

    display::setBackColor(style->color);
    display::fillRoundedRect(aggDrawing, x, y, x + w - 1, y + h - 1, 1, h, false, true);
}

bool SwitchWidgetState::hasOnTouch() {
    return true;
}

void SwitchWidgetState::onTouch(const WidgetCursor &widgetCursor, Event &touchEvent) {
	if (touchEvent.type == EVENT_TYPE_TOUCH_UP) {
        set(widgetCursor, widgetCursor.widget->data, get(widgetCursor, widgetCursor.widget->data).getInt() ? 0 : 1);

        if (widgetCursor.widget->action != ACTION_ID_NONE) {
            executeAction(widgetCursor, widgetCursor.widget->action);
        } else {
            sound::playClick();
        }

		changeStartTime = millis();
		if (changeStartTime == 0) {
			changeStartTime = 1;
		}
	}
}


} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
