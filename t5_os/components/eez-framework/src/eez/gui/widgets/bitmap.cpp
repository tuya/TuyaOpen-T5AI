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

#include <eez/core/os.h>
#include <eez/core/util.h>

#include <eez/gui/gui.h>
#include <eez/gui/widgets/bitmap.h>

namespace eez {
namespace gui {

bool BitmapWidgetState::updateState() {
    WIDGET_STATE_START(BitmapWidget);

    WIDGET_STATE(flags.active, g_isActiveWidget);
    WIDGET_STATE(data, widget->data ? getBitmapImage(widgetCursor, widget->data) : get(widgetCursor, widget->data));

    WIDGET_STATE_END()
}

void BitmapWidgetState::render() {
    const WidgetCursor &widgetCursor = g_widgetCursor;

    auto widget = (const BitmapWidget *)widgetCursor.widget;
    const Style* style = getStyle(widget->style);

    const Bitmap *bitmap = nullptr;

    if (widget->data) {
        if (data.getType() == VALUE_TYPE_POINTER) {
            auto image = (Image *)data.getVoidPointer();
            if (image) {
                drawBitmap(image, widgetCursor.x, widgetCursor.y, widgetCursor.w, widgetCursor.h, style, flags.active);
            }
            return;
        } else {
            bitmap = getBitmap(data.getInt());
        }
    } else if (widget->bitmap != 0) {
        bitmap = getBitmap(widget->bitmap);
    }

    if (bitmap) {
        Image image;

        image.width = bitmap->w;
        image.height = bitmap->h;
        image.bpp = bitmap->bpp;
        image.lineOffset = 0;
        image.pixels = (uint8_t *)bitmap->pixels;

        drawBitmap(&image, widgetCursor.x, widgetCursor.y, widgetCursor.w, widgetCursor.h, style, flags.active);
    }
}

} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
