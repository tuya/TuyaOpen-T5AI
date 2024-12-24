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

#include <assert.h>
#include <math.h>
#include <string.h>

#include <eez/core/assets.h>
#include <eez/core/os.h>
#include <eez/core/debug.h>
#include <eez/core/action.h>

#if OPTION_MOUSE
#include <eez/core/mouse.h>
#endif

#include <eez/core/sound.h>
#include <eez/core/util.h>

#include <eez/gui/gui.h>

#include <eez/flow/flow.h>

#define CONF_GUI_BLINK_TIME 400 // 400ms

namespace eez {
namespace gui {

void (*loadMainAssets)(const uint8_t* assets, uint32_t assetsSize) = eez::loadMainAssets;
Assets*& g_mainAssets = eez::g_mainAssets;

bool g_isBlinkTime;
static bool g_wasBlinkTime;

////////////////////////////////////////////////////////////////////////////////

void guiInit() {
#ifndef GUI_SKIP_LOAD_MAIN_ASSETS
    if (!g_isMainAssetsLoaded) {
        loadMainAssets(assets, sizeof(assets));
    }
#endif

    if (g_mainAssets->flowDefinition) {
        flow::start(g_mainAssets);
    }

    display::init();
}

void guiTick() {
    g_wasBlinkTime = g_isBlinkTime;
    g_isBlinkTime = (millis() % (2 * CONF_GUI_BLINK_TIME)) > CONF_GUI_BLINK_TIME;

    if (g_mainAssets->flowDefinition) {
        flow::tick();
    }

	g_hooks.stateManagment();
}

////////////////////////////////////////////////////////////////////////////////

bool isInternalAction(int actionId) {
    return actionId > FIRST_INTERNAL_ACTION_ID;
}

void executeAction(const WidgetCursor &widgetCursor, int actionId, void *param) {
    if (actionId == ACTION_ID_NONE) {
        return;
    }

    sound::playClick();

	g_hooks.executeActionThread();

    if (isInternalAction(actionId)) {
        executeInternalAction(actionId);
    } else {
        if (actionId >= 0) {
            g_actionExecFunctions[actionId]();
        } else {
			g_hooks.executeExternalAction(widgetCursor, actionId, param);
        }
    }
}

void popPage() {
	getAppContextFromId(APP_CONTEXT_ID_DEVICE)->popPage();
}

// from InternalActionsEnum
static ActionExecFunc g_internalActionExecFunctions[] = {
    0,
    // ACTION_ID_INTERNAL_SELECT_ENUM_ITEM
	SelectFromEnumPage::selectEnumItem,

    // ACTION_ID_INTERNAL_DIALOG_CLOSE
    popPage,

    // ACTION_ID_INTERNAL_TOAST_ACTION
    ToastMessagePage::executeAction,

    // ACTION_ID_INTERNAL_TOAST_ACTION_WITHOUT_PARAM
    ToastMessagePage::executeActionWithoutParam,

    // ACTION_ID_INTERNAL_MENU_WITH_BUTTONS
    MenuWithButtonsPage::executeAction,

    // ACTION_ID_INTERNAL_QUESTION_PAGE_BUTTON
    QuestionPage::executeAction
};

void executeInternalAction(int actionId) {
    g_internalActionExecFunctions[actionId - FIRST_INTERNAL_ACTION_ID]();
}

bool isFocusWidget(const WidgetCursor &widgetCursor) {
    return widgetCursor.appContext->isFocusWidget(widgetCursor);
}

bool isExternalPageOnStack() {
	return getAppContextFromId(APP_CONTEXT_ID_DEVICE)->isExternalPageOnStack();
}

void removeExternalPagesFromTheStack() {
	return getAppContextFromId(APP_CONTEXT_ID_DEVICE)->removeExternalPagesFromTheStack();
}

struct OverrideStyleRule {
    int16_t fromStyle;
    int16_t toStyle;
};
static OverrideStyleRule g_overrideStyleRules[10];

void setOverrideStyleRule(int16_t fromStyle, int16_t toStyle) {
    for (size_t i = 0; i < sizeof(g_overrideStyleRules) / sizeof(OverrideStyleRule); i++) {
        if (g_overrideStyleRules[i].fromStyle == STYLE_ID_NONE) {
            g_overrideStyleRules[i].fromStyle = fromStyle;
            g_overrideStyleRules[i].toStyle = toStyle;
        } else if (g_overrideStyleRules[i].fromStyle == fromStyle) {
            g_overrideStyleRules[i].toStyle = toStyle;
            break;
        }
    }
}

int overrideStyle(const WidgetCursor &widgetCursor, int styleId) {
    if (g_overrideStyleRules[0].fromStyle != STYLE_ID_NONE) {
        for (size_t i = 0; i < sizeof(g_overrideStyleRules) / sizeof(OverrideStyleRule); i++) {
            if (g_overrideStyleRules[i].fromStyle == STYLE_ID_NONE) {
                break;
            }
            if (g_overrideStyleRules[i].fromStyle == styleId) {
                styleId = g_overrideStyleRules[i].toStyle;
                break;
            }
        }
    }
    if (g_hooks.overrideStyle) {
        return g_hooks.overrideStyle(widgetCursor, styleId);
    }
    return styleId;
}

} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
