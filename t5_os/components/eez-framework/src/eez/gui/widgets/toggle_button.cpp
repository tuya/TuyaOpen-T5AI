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
#include <eez/gui/widgets/toggle_button.h>

namespace eez {
namespace gui {

bool ToggleButtonWidgetState::updateState() {
    WIDGET_STATE_START(ToggleButtonWidget);

    WIDGET_STATE(flags.active, g_isActiveWidget);
    WIDGET_STATE(flags.enabled, get(widgetCursor, widget->data).getInt() ? 1 : 0);

    WIDGET_STATE_END()
}

void ToggleButtonWidgetState::render() {
    const WidgetCursor &widgetCursor = g_widgetCursor;

    auto widget = (const ToggleButtonWidget *)widgetCursor.widget;
    const Style* style = getStyle(widget->style);
    const Style* checkedStyle = getStyle(widget->checkedStyle);

    auto &text = flags.enabled ? widget->text2 : widget->text1;
    drawText(
        text ? static_cast<const char *>(text) : "",
        -1,
        widgetCursor.x, widgetCursor.y, widgetCursor.w, widgetCursor.h,
        flags.enabled ? checkedStyle : style,
        flags.active
    );
}

bool ToggleButtonWidgetState::hasOnTouch() {
    return true;
}

void ToggleButtonWidgetState::onTouch(const WidgetCursor &widgetCursor, Event &touchEvent) {
	if (touchEvent.type == EVENT_TYPE_TOUCH_UP) {
        set(widgetCursor, widgetCursor.widget->data, get(widgetCursor, widgetCursor.widget->data).getInt() ? 0 : 1);

        if (widgetCursor.widget->action != ACTION_ID_NONE) {
            executeAction(widgetCursor, widgetCursor.widget->action);
        } else {
            sound::playClick();
        }
	}
}

} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
