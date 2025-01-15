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

#include <eez/gui/gui.h>
#include <eez/gui/widgets/containers/container.h>

namespace eez {
namespace gui {

bool isOverlay(const  WidgetCursor &widgetCursor) {
    if (widgetCursor.widget->type != WIDGET_TYPE_CONTAINER) {
        return false;
    }
    auto containerWidget = (const ContainerWidget *)widgetCursor.widget;
    return containerWidget->overlay != DATA_ID_NONE;
}

Overlay *getOverlay(const WidgetCursor &widgetCursor) {
    if (widgetCursor.widget->type != WIDGET_TYPE_CONTAINER) {
        return nullptr;
    }
    auto containerWidget = (const ContainerWidget *)widgetCursor.widget;
    if (containerWidget->overlay == DATA_ID_NONE) {
        return nullptr;
    }
    Value overlayValue;
    DATA_OPERATION_FUNCTION(containerWidget->overlay, DATA_OPERATION_GET_OVERLAY_DATA, widgetCursor, overlayValue);
    if (overlayValue.getType() != VALUE_TYPE_POINTER) {
        return nullptr;
    }
    return (Overlay *)overlayValue.getVoidPointer();
}

void getOverlayOffset(const WidgetCursor &widgetCursor, int &xOffset, int &yOffset) {
    Overlay *overlay = getOverlay(widgetCursor);
    if (overlay) {
		auto &overlayXOffset = (overlay->visibility & OVERLAY_MINIMIZED) != 0 ? overlay->xOffsetMinimized : overlay->xOffsetMaximized;
		auto &overlayYOffset = (overlay->visibility & OVERLAY_MINIMIZED) != 0 ? overlay->yOffsetMinimized : overlay->yOffsetMaximized;

        if (!overlay->moved && overlay->state) {
			overlayXOffset = overlay->x - widgetCursor.widget->x;
			overlayYOffset = overlay->y - widgetCursor.widget->y;
        }

        int x1 = 0;
        int y1 = 0;
        int x2 = 0;
        int y2 = 0;
        expandRectWithShadow(x1, y1, x2, y2);

        int x = widgetCursor.x + overlayXOffset;
        if (x + x1 < widgetCursor.appContext->rect.x) {
            x = widgetCursor.appContext->rect.x - x1;
        }
        if (x + overlay->width + x2 > widgetCursor.appContext->rect.x + widgetCursor.appContext->rect.w) {
            x = widgetCursor.appContext->rect.x + widgetCursor.appContext->rect.w - overlay->width - x2;
        }

        int y = widgetCursor.y + overlayYOffset;
        if (y + y1 < widgetCursor.appContext->rect.y) {
            y = widgetCursor.appContext->rect.y - y1;
        }
        if (y + overlay->height + y2 > widgetCursor.appContext->rect.y + widgetCursor.appContext->rect.h) {
            y = widgetCursor.appContext->rect.y + widgetCursor.appContext->rect.h - overlay->height - y2;
        }

        xOffset = overlayXOffset = x - widgetCursor.x;
        yOffset = overlayYOffset = y - widgetCursor.y;
    } else {
        xOffset = 0;
        yOffset = 0;
    }
}

void dragOverlay(Event &touchEvent) {
    Overlay *overlay = getOverlay(getFoundWidgetAtDown());
    if (overlay) {
		auto &overlayXOffset = (overlay->visibility & OVERLAY_MINIMIZED) != 0 ? overlay->xOffsetMinimized : overlay->xOffsetMaximized;
		auto &overlayYOffset = (overlay->visibility & OVERLAY_MINIMIZED) != 0 ? overlay->yOffsetMinimized : overlay->yOffsetMaximized;
		if (touchEvent.type == EVENT_TYPE_TOUCH_DOWN) {
            overlay->xOnTouchDown = touchEvent.x;
            overlay->yOnTouchDown = touchEvent.y;
            overlay->xOffsetOnTouchDown = overlayXOffset;
            overlay->yOffsetOnTouchDown = overlayYOffset;
        } else if (touchEvent.type == EVENT_TYPE_TOUCH_MOVE) {
            overlay->moved = true;
			overlayXOffset = overlay->xOffsetOnTouchDown + touchEvent.x - overlay->xOnTouchDown;
			overlayYOffset = overlay->yOffsetOnTouchDown + touchEvent.y - overlay->yOnTouchDown;
        }
    }
}

} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
