/*
 * eez-framework
 *
 * MIT License
 * Copyright 2024 Envox d.o.o.
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <eez/conf-internal.h>

#ifndef PAGE_ID_ASYNC_OPERATION_IN_PROGRESS
#define PAGE_ID_ASYNC_OPERATION_IN_PROGRESS 0
#endif

#ifndef COLOR_ID_BOOKMARK
#define COLOR_ID_BOOKMARK 0
#endif

#ifndef MAX_KEYPAD_TEXT_LENGTH
#define MAX_KEYPAD_TEXT_LENGTH 128
#endif

#ifndef MAX_KEYPAD_LABEL_LENGTH
#define MAX_KEYPAD_LABEL_LENGTH 64
#endif

#include <eez/core/assets.h>
#include <eez/gui/display.h>

enum {
    FIRST_INTERNAL_PAGE_ID = 32000,
    INTERNAL_PAGE_ID_SELECT_FROM_ENUM,
    INTERNAL_PAGE_ID_TOAST_MESSAGE,
    INTERNAL_PAGE_ID_MENU_WITH_BUTTONS,
    INTERNAL_PAGE_ID_QUESTION
};

enum InternalActionsEnum {
    FIRST_INTERNAL_ACTION_ID = 32000,
    ACTION_ID_INTERNAL_SELECT_ENUM_ITEM,
    ACTION_ID_INTERNAL_DIALOG_CLOSE,
    ACTION_ID_INTERNAL_TOAST_ACTION,
    ACTION_ID_INTERNAL_TOAST_ACTION_WITHOUT_PARAM,
    ACTION_ID_INTERNAL_MENU_WITH_BUTTONS,
    ACTION_ID_INTERNAL_QUESTION_PAGE_BUTTON
};

namespace eez {
namespace gui {

////////////////////////////////////////////////////////////////////////////////

extern WidgetCursor g_activeWidget;
extern bool g_isBlinkTime;

////////////////////////////////////////////////////////////////////////////////

void guiInit();
void guiTick();

WidgetCursor &getFoundWidgetAtDown();
void setFoundWidgetAtDown(WidgetCursor &widgetCursor);
void clearFoundWidgetAtDown();
bool isFocusWidget(const WidgetCursor &widgetCursor);
void refreshScreen();
inline bool isPageInternal(int pageId) { return pageId > FIRST_INTERNAL_PAGE_ID; }

bool isExternalPageOnStack();
void removeExternalPagesFromTheStack();

inline int getWidgetAction(const WidgetCursor &widgetCursor) {
    if (widgetCursor.widget->type == WIDGET_TYPE_INPUT) {
        if (widgetCursor.widget->action == ACTION_ID_NONE) {
		    return ACTION_ID_EDIT;
        }
    }
	return widgetCursor.widget->action;
}

void executeAction(const WidgetCursor &widgetCursor, int actionId, void *param = nullptr);
void executeInternalAction(int actionId);

AppContext *getAppContextFromId(int16_t id);

extern const char *g_discardMessage;

extern void (*loadMainAssets)(const uint8_t *assets, uint32_t assetsSize);
extern Assets *&g_mainAssets;

void setOverrideStyleRule(int16_t fromStyle, int16_t toStyle);
int overrideStyle(const WidgetCursor &widgetCursor, int styleId);

} // namespace gui
} // namespace eez

#include <eez/gui/app_context.h>
#include <eez/gui/update.h>
#include <eez/gui/overlay.h>
#include <eez/gui/font.h>
#include <eez/gui/draw.h>
#include <eez/gui/touch.h>
#include <eez/gui/page.h>
#include <eez/gui/hooks.h>

#define DATA_OPERATION_FUNCTION(id, operation, widgetCursor, value) (id >= 0 ? g_dataOperationsFunctions[id](operation, widgetCursor, value) : g_hooks.externalData(id, operation, widgetCursor, value))
