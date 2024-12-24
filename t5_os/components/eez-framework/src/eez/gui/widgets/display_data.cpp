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

#include <string.h>

#include <eez/core/os.h>
#include <eez/core/util.h>

#include <eez/gui/gui.h>
#include <eez/gui/widgets/display_data.h>

namespace eez {
namespace gui {

enum {
    DISPLAY_OPTION_ALL = 0,
    DISPLAY_OPTION_INTEGER = 1,
    DISPLAY_OPTION_FRACTION = 2,
    DISPLAY_OPTION_FRACTION_AND_UNIT = 3,
    DISPLAY_OPTION_UNIT = 4,
    DISPLAY_OPTION_INTEGER_AND_FRACTION = 5
};

int findStartOfFraction(char *text) {
    int i;
    for (i = 0; text[i] && (text[i] == '<' || text[i] == ' ' || text[i] == '-' || (text[i] >= '0' && text[i] <= '9')); i++);
    return i;
}

int findStartOfUnit(char *text, int i) {
    for (i = 0; text[i] && (text[i] == '<' || text[i] == ' ' || text[i] == '-' || (text[i] >= '0' && text[i] <= '9') || text[i] == '.'); i++);
    return i;
}

bool DisplayDataWidgetState::updateState() {
    WIDGET_STATE_START(DisplayDataWidget);

    const Style *style = getStyle(overrideStyle(widgetCursor, widget->style));

    WIDGET_STATE(flags.active, g_isActiveWidget);
    WIDGET_STATE(flags.focused, isFocusWidget(widgetCursor));
    WIDGET_STATE(flags.blinking, g_isBlinkTime && (styleIsBlink(style) || isBlinking(widgetCursor, widget->data)));

    uint32_t refreshRate;
    if (widgetCursor.flowState) {
        refreshRate = get(widgetCursor, widget->refreshRate).toInt32(nullptr);
    } else {
        refreshRate = getTextRefreshRate(widgetCursor, widget->data);
    }

    bool refreshData = true;
    auto newData = get(widgetCursor, widget->data);
    auto currentTime = millis();
    if (hasPreviousState && data != newData) {
        if (refreshRate != 0 && currentTime - dataRefreshLastTime < refreshRate) {
            refreshData = false;
        }
    }
    if (refreshData) {
        WIDGET_STATE(data, newData);
        dataRefreshLastTime = currentTime;
    }

    WIDGET_STATE(color,                 flags.focused ? style->focusColor           : getColor(widgetCursor, widget->data, style));
    WIDGET_STATE(backgroundColor,       flags.focused ? style->focusBackgroundColor : getBackgroundColor(widgetCursor, widget->data, style));
    WIDGET_STATE(activeColor,           flags.focused ? style->focusBackgroundColor : getActiveColor(widgetCursor, widget->data, style));
    WIDGET_STATE(activeBackgroundColor, flags.focused ? style->focusColor           : getActiveBackgroundColor(widgetCursor, widget->data, style));

    bool cursorVisible = millis() % (2 * CONF_GUI_TEXT_CURSOR_BLINK_TIME_MS) < CONF_GUI_TEXT_CURSOR_BLINK_TIME_MS;
    WIDGET_STATE(cursorPosition, cursorVisible ? getTextCursorPosition(widgetCursor, widget->data) : -1);

    WIDGET_STATE(xScroll, getXScroll(widgetCursor));

    WIDGET_STATE_END()
}

void DisplayDataWidgetState::render() {
    const WidgetCursor &widgetCursor = g_widgetCursor;

    auto widget = (const DisplayDataWidget *)widgetCursor.widget;
    const Style *style = getStyle(overrideStyle(widgetCursor, widget->style));

    char text[64];
    data.toText(text, sizeof(text));

    char *start = text;

    int length = -1;

    if (widget->displayOption != DISPLAY_OPTION_ALL) {
        if (data.getType() == VALUE_TYPE_FLOAT) {
            if (widget->displayOption == DISPLAY_OPTION_INTEGER) {
                int i = findStartOfFraction(text);
                text[i] = 0;
            } else if (widget->displayOption == DISPLAY_OPTION_FRACTION) {
                int i = findStartOfFraction(text);
                start = text + i;
            } else if (widget->displayOption == DISPLAY_OPTION_FRACTION_AND_UNIT) {
                int i = findStartOfFraction(text);
                int k = findStartOfUnit(text, i);
                if (i < k) {
                    start = text + i;
                    text[k] = 0;
                }
                else {
                    stringCopy(text, sizeof(text), ".0");
                }
            } else if (widget->displayOption == DISPLAY_OPTION_UNIT) {
                int i = findStartOfUnit(text, 0);
                start = text + i;
            } else if (widget->displayOption == DISPLAY_OPTION_INTEGER_AND_FRACTION) {
                int i = findStartOfUnit(text, 0);
                text[i] = 0;
            }

            // trim left
            while (*start && *start == ' ') {
                start++;
            }

            // trim right
            length = strlen(start);
            if (length > 0 && start[length - 1] == ' ') {
                length--;
            }
        } else {
            if (
                widget->displayOption != DISPLAY_OPTION_INTEGER &&
                widget->displayOption != DISPLAY_OPTION_INTEGER_AND_FRACTION
            ) {
                *text = 0;
            }
        }
    }

    drawText(
        start, length,
        widgetCursor.x, widgetCursor.y, widgetCursor.w, widgetCursor.h,
        style,
        flags.active, flags.blinking, false,
        &color, &backgroundColor, &activeColor, &activeBackgroundColor,
        data.getType() == VALUE_TYPE_FLOAT
#if OPTION_KEYPAD
        || widget->data == DATA_ID_KEYPAD_EDIT_UNIT
#endif
        , // useSmallerFontIfDoesNotFit
        cursorPosition,
        xScroll
    );
}

int DISPLAY_DATA_getCharIndexAtPosition(int xPos, const WidgetCursor &widgetCursor) {
    auto widget = (const DisplayDataWidget *)widgetCursor.widget;

	const Style *style = getStyle(overrideStyle(widgetCursor, widget->style));

    char text[64];
    Value data = get(widgetCursor, widget->data);
    data.toText(text, sizeof(text));

    char *start = text;

    if (widget->displayOption == DISPLAY_OPTION_INTEGER) {
        int i = findStartOfFraction(text);
        text[i] = 0;
    } else if (widget->displayOption == DISPLAY_OPTION_FRACTION) {
        int i = findStartOfFraction(text);
        start = text + i;
    } else if (widget->displayOption == DISPLAY_OPTION_FRACTION_AND_UNIT) {
        int i = findStartOfFraction(text);
        int k = findStartOfUnit(text, i);
        if (i < k) {
            start = text + i;
            text[k] = 0;
        } else {
            stringCopy(text, sizeof(text), ".0");
        }
    } else if (widget->displayOption == DISPLAY_OPTION_UNIT) {
        int i = findStartOfUnit(text, 0);
        start = text + i;
    } else if (widget->displayOption == DISPLAY_OPTION_INTEGER_AND_FRACTION) {
        int i = findStartOfUnit(text, 0);
        text[i] = 0;
    }

    // trim left
    while (*start && *start == ' ') {
        start++;
    }

    // trim right
    int length = strlen(start);
    while (length > 0 && start[length - 1] == ' ') {
        length--;
    }

    int xScroll = getXScroll(widgetCursor);

    return getCharIndexAtPosition(xPos, start, length, widgetCursor.x - xScroll, widgetCursor.y, widgetCursor.w, widgetCursor.h, style);
}

int DISPLAY_DATA_getCursorXPosition(int cursorPosition, const WidgetCursor &widgetCursor) {
    auto widget = (const DisplayDataWidget *)widgetCursor.widget;

	const Style *style = getStyle(overrideStyle(widgetCursor, widget->style));

    char text[64];
    Value data = get(widgetCursor, widget->data);
    data.toText(text, sizeof(text));

    char *start = text;

    if (widget->displayOption == DISPLAY_OPTION_INTEGER) {
        int i = findStartOfFraction(text);
        text[i] = 0;
    } else if (widget->displayOption == DISPLAY_OPTION_FRACTION) {
        int i = findStartOfFraction(text);
        start = text + i;
    } else if (widget->displayOption == DISPLAY_OPTION_FRACTION_AND_UNIT) {
        int i = findStartOfFraction(text);
        int k = findStartOfUnit(text, i);
        if (i < k) {
            start = text + i;
            text[k] = 0;
        } else {
            stringCopy(text, sizeof(text), ".0");
        }
    } else if (widget->displayOption == DISPLAY_OPTION_UNIT) {
        int i = findStartOfUnit(text, 0);
        start = text + i;
    } else if (widget->displayOption == DISPLAY_OPTION_INTEGER_AND_FRACTION) {
        int i = findStartOfUnit(text, 0);
        text[i] = 0;
    }

    // trim left
    while (*start && *start == ' ') {
        start++;
    }

    // trim right
    int length = strlen(start);
    while (length > 0 && start[length - 1] == ' ') {
        length--;
    }

    return getCursorXPosition(cursorPosition, start, length, widgetCursor.x, widgetCursor.y, widgetCursor.w, widgetCursor.h, style);
}

} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
