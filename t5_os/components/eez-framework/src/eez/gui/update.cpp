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

#include <eez/core/debug.h>
#include <eez/core/os.h>

#include <eez/gui/gui.h>
#include <eez/gui/widgets/containers/app_view.h>

namespace eez {
namespace gui {

static bool g_refreshScreen;
static Widget *g_rootWidget;

WidgetState *g_widgetStateStart;
WidgetState *g_widgetStateEnd;

bool g_widgetStateStructureChanged;

WidgetCursor g_widgetCursor;

void refreshScreen() {
	g_refreshScreen = true;
}

void updateScreen() {
	if (!g_rootWidget) {
		static AppViewWidget g_rootAppViewWidget;

		g_rootWidget = &g_rootAppViewWidget;

		g_rootWidget->type = WIDGET_TYPE_APP_VIEW;
		g_rootWidget->data = DATA_ID_NONE;
		g_rootWidget->action = ACTION_ID_NONE;
		g_rootWidget->x = 0;
		g_rootWidget->y = 0;
		g_rootWidget->width = display::getDisplayWidth();
		g_rootWidget->height = display::getDisplayHeight();
		g_rootWidget->style = 0;
	}

	if (g_refreshScreen) {
		g_refreshScreen = false;

		if (g_widgetStateStart) {
			// invalidate widget states
			freeWidgetStates(g_widgetStateStart);
			g_widgetStateStart = nullptr;
		}

        // auto bufferIndex = display::beginBufferRendering();
        // display::setColor(0, 0, 0);
        // display::fillRect(0, 0, display::getDisplayWidth(), display::getDisplayHeight());
        // display::endBufferRendering(bufferIndex, 0, 0, display::getDisplayWidth(), display::getDisplayHeight(), false, 255, 0, 0, nullptr);
	}

	bool hasPreviousState = g_widgetStateStart != nullptr;
	g_widgetStateStart = (WidgetState *)GUI_STATE_BUFFER;

    g_isActiveWidget = false;

	g_widgetCursor = WidgetCursor();
	g_widgetCursor.assets = g_mainAssets;
	g_widgetCursor.appContext = getRootAppContext();
	g_widgetCursor.widget = g_rootWidget;
	g_widgetCursor.currentState = g_widgetStateStart;
	g_widgetCursor.refreshed = !hasPreviousState;
	g_widgetCursor.hasPreviousState = hasPreviousState;

	g_foundWidgetAtDownInvalid = false;

    g_widgetCursor.x = 0;
    g_widgetCursor.y = 0;
    g_widgetCursor.w = g_rootWidget->width;
    g_widgetCursor.h = g_rootWidget->height;

    if (g_mainAssets->assetsType != ASSETS_TYPE_DASHBOARD) {
        enumWidget();
    }

	g_widgetStateEnd = g_widgetCursor.currentState;
	g_widgetStateStructureChanged = !g_widgetCursor.hasPreviousState;

	if (g_foundWidgetAtDownInvalid) {
		WidgetCursor widgetCursor;
		setFoundWidgetAtDown(widgetCursor);
	}
}

void enumRootWidget() {
	if (!g_widgetStateStart  || g_refreshScreen) {
		return;
	}

    g_isActiveWidget = false;

	g_widgetCursor = WidgetCursor();
	g_widgetCursor.assets = g_mainAssets;
	g_widgetCursor.appContext = getRootAppContext();
	g_widgetCursor.widget = g_rootWidget;
	g_widgetCursor.currentState = g_widgetStateStart;
	g_widgetCursor.refreshed = false;
	g_widgetCursor.hasPreviousState = true;

    g_widgetCursor.x = 0;
    g_widgetCursor.y = 0;
    g_widgetCursor.w = g_rootWidget->width;
    g_widgetCursor.h = g_rootWidget->height;

    enumWidget();
}

} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
