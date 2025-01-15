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
#include <eez/gui/widgets/qr_code.h>

#include <eez/libs/qrcodegen/qrcodegen.h>

namespace eez {
namespace gui {

bool QRCodeWidgetState::updateState() {
    WIDGET_STATE_START(QRCodeWidget);

    WIDGET_STATE(data, get(widgetCursor, widget->data));

    WIDGET_STATE_END()
}

uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];

void QRCodeWidgetState::render() {
    const WidgetCursor &widgetCursor = g_widgetCursor;

    auto widget = (const QRCodeWidget *)widgetCursor.widget;
    const Style *style = getStyle(widget->style);

    // Make QR code
    qrcodegen_Ecc errCorLvl;
    if (widget->errorCorrection == 0) errCorLvl = qrcodegen_Ecc_LOW;
    else if (widget->errorCorrection == 1) errCorLvl = qrcodegen_Ecc_MEDIUM;
    else if (widget->errorCorrection == 2) errCorLvl = qrcodegen_Ecc_QUARTILE;
    else errCorLvl = qrcodegen_Ecc_HIGH;

	qrcodegen_encodeText(data.getString(), tempBuffer, qrcode, errCorLvl, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);

    display::setColor(style->backgroundColor);
    display::fillRect(widgetCursor.x, widgetCursor.y, widgetCursor.x + widgetCursor.w - 1, widgetCursor.y + widgetCursor.h - 1);

    int size = qrcodegen_getSize(qrcode);
    int border = 1;

    double sizePx = 1.0 * MIN(widgetCursor.w, widgetCursor.h) / (size + 2 * border);

    double xPadding = (widgetCursor.w - sizePx * size) / 2;
    double yPadding = (widgetCursor.h - sizePx * size) / 2;

	display::AggDrawing aggDrawing;
	display::aggInit(aggDrawing);
	auto &graphics = aggDrawing.graphics;

    auto color16 = display::getColor16FromIndex(style->color);

    graphics.resetPath();

    for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
            if (qrcodegen_getModule(qrcode, x, y)) {
                graphics.moveTo(
                    widgetCursor.x + xPadding + x * sizePx,
                    widgetCursor.y + yPadding + y * sizePx
                );
                graphics.horLineRel(sizePx);
                graphics.verLineRel(sizePx);
                graphics.horLineRel(-sizePx);
                graphics.closePolygon();
            }
		}
	}

    graphics.fillColor(COLOR_TO_R(color16), COLOR_TO_G(color16), COLOR_TO_B(color16));
    graphics.noLine();
    graphics.drawPath();
}

} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
