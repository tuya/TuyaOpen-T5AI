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

#include <math.h>
#include <stdlib.h>

#include <eez/core/sound.h>

#include <eez/gui/gui.h>
#include <eez/gui/widgets/list_graph.h>

namespace eez {
namespace gui {

bool ListGraphWidgetState::updateState() {
    WIDGET_STATE_START(ListGraphWidget);

    WIDGET_STATE(data, get(widgetCursor, widget->data));
    WIDGET_STATE(cursorData, get(widgetCursor, widget->cursorData));

    WIDGET_STATE_END()
}

void ListGraphWidgetState::render() {
    const WidgetCursor &widgetCursor = g_widgetCursor;

    auto widget = (const ListGraphWidget *)widgetCursor.widget;

    const Style* style = getStyle(widget->style);
    const Style* y1Style = getStyle(widget->y1Style);
    const Style* y2Style = getStyle(widget->y2Style);
    const Style* cursorStyle = getStyle(widget->cursorStyle);

    int iCursor = cursorData.getInt();
    int iRow = iCursor / 3;

    // draw background
    display::setColor(style->backgroundColor);
    display::fillRect(widgetCursor.x, widgetCursor.y, widgetCursor.x + widgetCursor.w - 1, widgetCursor.y + widgetCursor.h - 1);

    int dwellListLength = getFloatListLength(widgetCursor, widget->dwellData);
    if (dwellListLength > 0) {
        float *dwellList = getFloatList(widgetCursor, widget->dwellData);

        const Style *styles[2] = { y1Style, y2Style };

        int listLength[2] = { getFloatListLength(widgetCursor, widget->y1Data),
                                getFloatListLength(widgetCursor, widget->y2Data) };

        float *list[2] = { getFloatList(widgetCursor, widget->y1Data),
                            getFloatList(widgetCursor, widget->y2Data) };

        float min[2] = {
            getMin(widgetCursor, widget->y1Data).getFloat(),
            getMin(widgetCursor, widget->y2Data).getFloat()
        };

        float max[2] = {
            getMax(widgetCursor, widget->y1Data).getFloat(),
            getMax(widgetCursor, widget->y2Data).getFloat()
        };

        int maxListLength = getFloatListLength(widgetCursor, widget->data);

        float dwellSum = 0;
        for (int i = 0; i < maxListLength; ++i) {
            if (i < dwellListLength) {
                dwellSum += dwellList[i];
            } else {
                dwellSum += dwellList[dwellListLength - 1];
            }
        }

        float currentDwellSum = 0;
        int xPrev = widgetCursor.x;
        int yPrev[2];
        for (int i = 0; i < maxListLength; ++i) {
            currentDwellSum +=
                i < dwellListLength ? dwellList[i] : dwellList[dwellListLength - 1];
            int x1 = xPrev;
            int x2;
            if (i == maxListLength - 1) {
                x2 = widgetCursor.x + widgetCursor.w - 1;
            } else {
                x2 = widgetCursor.x + int(currentDwellSum * widgetCursor.w / dwellSum);
            }
            if (x2 < x1)
                x2 = x1;
            if (x2 >= widgetCursor.x + widgetCursor.w)
                x2 = widgetCursor.x + widgetCursor.w - 1;

            if (i == iRow) {
                display::setColor(cursorStyle->backgroundColor);
                display::fillRect(x1, widgetCursor.y, x2 - 1,
                    widgetCursor.y + widgetCursor.h - 1);
            }


            for (int k = 0; k < 2; ++k) {
                int j = iCursor % 3 == 2 ? k : 1 - k;

                if (listLength[j] > 0) {
                    display::setColor(styles[j]->color);

                    float value = i < listLength[j] ? list[j][i] : list[j][listLength[j] - 1];
                    int y = int((value - min[j]) * widgetCursor.h / (max[j] - min[j]));
                    if (y < 0)
                        y = 0;
                    if (y >= widgetCursor.h)
                        y = widgetCursor.h - 1;

                    y = widgetCursor.y + (widgetCursor.h - 1) - y;

                    if (i > 0 && abs(yPrev[j] - y) > 1) {
                        if (yPrev[j] < y) {
                            display::drawVLine(x1, yPrev[j] + 1, y - yPrev[j] - 1);
                        } else {
                            display::drawVLine(x1, y, yPrev[j] - y - 1);
                        }
                    }

                    yPrev[j] = y;

                    display::drawHLine(x1, y, x2 - x1);
                }
            }

            xPrev = x2;
        }
    }
}

bool ListGraphWidgetState::hasOnTouch() {
    return true;
}

void ListGraphWidgetState::onTouch(const WidgetCursor &widgetCursor, Event &touchEvent) {
    if (touchEvent.type == EVENT_TYPE_TOUCH_DOWN || touchEvent.type == EVENT_TYPE_TOUCH_MOVE) {
        auto widget = (const ListGraphWidget *)widgetCursor.widget;

        if (touchEvent.x < widgetCursor.x || touchEvent.x >= widgetCursor.x + widgetCursor.w) {
            return;
        }
        if (touchEvent.y < widgetCursor.y || touchEvent.y >= widgetCursor.y + widgetCursor.h) {
            return;
        }

        int dwellListLength = getFloatListLength(widgetCursor, widget->dwellData);
        if (dwellListLength > 0) {
            int iCursor = -1;

            float *dwellList = getFloatList(widgetCursor, widget->dwellData);

            int maxListLength = getFloatListLength(widgetCursor, widget->data);

            float dwellSum = 0;
            for (int i = 0; i < maxListLength; ++i) {
                if (i < dwellListLength) {
                    dwellSum += dwellList[i];
                } else {
                    dwellSum += dwellList[dwellListLength - 1];
                }
            }

            float currentDwellSum = 0;
            int xPrev = widgetCursor.x;
            for (int i = 0; i < maxListLength; ++i) {
                currentDwellSum +=
                    i < dwellListLength ? dwellList[i] : dwellList[dwellListLength - 1];
                int x1 = xPrev;
                int x2;
                if (i == maxListLength - 1) {
                    x2 = widgetCursor.x + widgetCursor.w - 1;
                } else {
                    x2 = widgetCursor.x + int(currentDwellSum * widgetCursor.w / dwellSum);
                }
                if (x2 < x1)
                    x2 = x1;
                if (x2 >= widgetCursor.x + widgetCursor.w)
                    x2 = widgetCursor.x + widgetCursor.w - 1;

                if (touchEvent.x >= x1 && touchEvent.x < x2) {
                    int iCurrentCursor =
                        get(widgetCursor, widget->cursorData).getInt();
                    iCursor = i * 3 + iCurrentCursor % 3;
                    break;
                }
            }

            if (iCursor >= 0) {
                set(widgetCursor, widget->cursorData, Value(iCursor));
                if (touchEvent.type == EVENT_TYPE_TOUCH_DOWN) {
                    sound::playClick();
                }
            }
        }
    }
}

} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
