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

#include <stdio.h>
#include <string.h>

#include <eez/core/util.h>

#include <eez/gui/gui.h>
#include <eez/gui/widgets/text.h>

#define IGNORE_LUMINOSITY_FLAG 1

static const size_t MAX_TEXT_LEN = 128;

namespace eez {
namespace gui {

void TextWidget_autoSize(TextWidget& widget) {
    const Style *style = getStyle(widget.style);
    font::Font font = styleGetFont(style);
    widget.width = style->borderSizeLeft + style->paddingLeft + display::measureStr(static_cast<const char *>(widget.text), -1, font, 0) + style->borderSizeRight + style->paddingRight;
    widget.height = style->borderSizeTop + style->paddingTop + font.getHeight() + style->borderSizeBottom + style->paddingBottom;
}

bool TextWidgetState::updateState() {
    WIDGET_STATE_START(TextWidget);

    const Style *style = getStyle(overrideStyle(widgetCursor, widget->style));

    WIDGET_STATE(flags.active, g_isActiveWidget);
    WIDGET_STATE(flags.focused, isFocusWidget(widgetCursor));

    WIDGET_STATE(flags.blinking, g_isBlinkTime && styleIsBlink(style));

    const char *text = widget->text ? static_cast<const char *>(widget->text) : nullptr;
	WIDGET_STATE(data, !(text && text[0]) && widget->data ? get(widgetCursor, widget->data) : 0);

    WIDGET_STATE_END()
}

void TextWidgetState::render() {
    const WidgetCursor &widgetCursor = g_widgetCursor;

    auto widget = (const TextWidget *)widgetCursor.widget;
    const Style *style = getStyle(overrideStyle(widgetCursor, widget->style));
    const char *text = widget->text ? static_cast<const char *>(widget->text) : nullptr;

    uint16_t overrideColor                 = flags.focused ? style->focusColor           : g_hooks.overrideStyleColor(widgetCursor, style);
    uint16_t overrideBackgroundColor       = flags.focused ? style->focusBackgroundColor : style->backgroundColor;
    uint16_t overrideActiveColor           = flags.focused ? style->focusBackgroundColor : g_hooks.overrideActiveStyleColor(widgetCursor, style);
    uint16_t overrideActiveBackgroundColor = flags.focused ? style->focusColor           : style->activeBackgroundColor;

    bool ignoreLuminosity = (widget->flags & IGNORE_LUMINOSITY_FLAG) != 0;
    if (text && text[0]) {
        drawText(
            text, -1,
            widgetCursor.x, widgetCursor.y, widgetCursor.w, widgetCursor.h,
            style,
            flags.active, flags.blinking, ignoreLuminosity,
            &overrideColor, &overrideBackgroundColor, &overrideActiveColor, &overrideActiveBackgroundColor
        );
    } else if (widget->data) {
        if (data.isString()) {
            if (data.getOptions() & STRING_OPTIONS_FILE_ELLIPSIS) {
                const char *fullText = data.getString();
                int fullTextLength = strlen(fullText);
                font::Font font = styleGetFont(style);
                int fullTextWidth = display::measureStr(fullText, fullTextLength, font);
                if (fullTextWidth <= widgetCursor.w) {
                    drawText(
                        fullText, fullTextLength,
                        widgetCursor.x, widgetCursor.y, widgetCursor.w, widgetCursor.h,
                        style,
                        flags.active, flags.blinking, ignoreLuminosity,
                        &overrideColor, &overrideBackgroundColor, &overrideActiveColor, &overrideActiveBackgroundColor
                    );
                } else {
                    char text[MAX_TEXT_LEN + 1];
                    int ellipsisWidth = display::measureStr("...", 3, font);
                    int width = ellipsisWidth;
                    int textLength = 3;
                    int iLeft = 0;
                    int iRight = strlen(fullText) - 1;
                    while (iLeft < iRight && textLength < (int)MAX_TEXT_LEN) {
                        int widthLeft = display::measureGlyph(fullText[iLeft], font);
                        if (width + widthLeft > widgetCursor.w) {
                            break;
                        }
                        width += widthLeft;
                        iLeft++;
                        textLength++;

                        int widthRight = display::measureGlyph(fullText[iRight], font);
                        if (width + widthRight > widgetCursor.w) {
                            break;
                        }
                        width += widthRight;
                        iRight--;
                        textLength++;
                    }

                    memcpy(text, fullText, iLeft);
                    text[iLeft] = 0;
                    stringAppendString(text, sizeof(text), "...");
                    stringAppendString(text, sizeof(text), fullText + iRight + 1);

                    drawText(
                        text, textLength,
                        widgetCursor.x, widgetCursor.y, widgetCursor.w, widgetCursor.h,
                        style,
                        flags.active, flags.blinking, ignoreLuminosity,
                        &overrideColor, &overrideBackgroundColor, &overrideActiveColor, &overrideActiveBackgroundColor
                    );
                }

            } else {
                const char *str = data.getString();
                drawText(
                    str ? str : "", -1,
                    widgetCursor.x, widgetCursor.y, widgetCursor.w, widgetCursor.h,
                    style,
                    flags.active, flags.blinking, ignoreLuminosity,
                    &overrideColor, &overrideBackgroundColor, &overrideActiveColor, &overrideActiveBackgroundColor
                );
            }
        } else {
            char text[MAX_TEXT_LEN + 1];
            data.toText(text, sizeof(text));
            drawText(
                text, -1,
                widgetCursor.x, widgetCursor.y, widgetCursor.w, widgetCursor.h,
                style,
                flags.active, flags.blinking, ignoreLuminosity,
                &overrideColor, &overrideBackgroundColor, &overrideActiveColor, &overrideActiveBackgroundColor,
                data.getType() == VALUE_TYPE_FLOAT
            );
        }
    }
}

} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
