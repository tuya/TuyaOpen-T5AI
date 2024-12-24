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
#include <eez/gui/display-private.h>
#if OPTION_KEYPAD
#include <eez/gui/keypad.h>
#endif
#include <eez/gui/touch_calibration.h>

#include <eez/flow/flow.h>

namespace eez {
namespace gui {

static int getExtraLongTouchAction() {
    return ACTION_ID_NONE;
}

static float getDefaultAnimationDuration() {
    return 0;
}

static void executeExternalAction(const WidgetCursor &widgetCursor, int16_t actionId, void *param) {
    flow::executeFlowAction(widgetCursor, actionId, param);
}

static void externalData(int16_t id, DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value) {
    flow::dataOperation(id, operation, widgetCursor, value);
}

static OnTouchFunctionType getWidgetTouchFunctionHook(const WidgetCursor &widgetCursor) {
#if OPTION_KEYPAD
	auto data = widgetCursor.widget->data < 0 ? (g_mainAssets->flowDefinition ? flow::getNativeVariableId(widgetCursor) : DATA_ID_NONE) : widgetCursor.widget->data;
	if (data == DATA_ID_KEYPAD_TEXT) {
        return eez::gui::onKeypadTextTouch;
    }
#endif
    return nullptr;
}

static Page *getPageFromId(int pageId) {
    return nullptr;
}

static void setFocusCursor(const WidgetCursor& cursor, int16_t dataId) {
}

static void stateManagment() {
#if defined(EEZ_PLATFORM_SIMULATOR) || defined(__EMSCRIPTEN__)
	getAppContextFromId(APP_CONTEXT_ID_SIMULATOR_FRONT_PANEL)->stateManagment();
#endif
	getAppContextFromId(APP_CONTEXT_ID_DEVICE)->stateManagment();
}

static bool activePageHasBackdrop() {
	if (getAppContextFromId(APP_CONTEXT_ID_DEVICE)->getActivePageId() == INTERNAL_PAGE_ID_TOAST_MESSAGE) {
        ToastMessagePage *page = (ToastMessagePage *)getAppContextFromId(APP_CONTEXT_ID_DEVICE)->getActivePage();
        return page->hasAction();
	}
	return true;
}

static void executeActionThread() {
    // why is this required?
    osDelay(1);
}

static bool isEventHandlingDisabled() {
    return false;
}

static uint16_t overrideStyleColor(const WidgetCursor &widgetCursor, const Style *style) {
    return style->color;
}

static uint16_t overrideActiveStyleColor(const WidgetCursor &widgetCursor, const Style *style) {
    return style->activeColor;
}

static uint16_t transformColor(uint16_t color) {
    return color;
}

static bool styleGetSmallerFont(font::Font &font) {
    return false;
}

static uint8_t getDisplayBackgroundLuminosityStep() {
    return DISPLAY_BACKGROUND_LUMINOSITY_STEP_DEFAULT;
}

static uint8_t getSelectedThemeIndex() {
    return eez::gui::THEME_ID_DEFAULT;
}

static void turnOnDisplayStart() {
    display::g_displayState = display::ON;
}

static void turnOnDisplayTick() {
}

static void turnOffDisplayStart() {
    display::g_displayState = display::OFF;
}

static void turnOffDisplayTick() {
}

static void toastMessagePageOnEncoder(ToastMessagePage *toast, int counter) {
	toast->appContext->popPage();
}

#if OPTION_TOUCH_CALIBRATION
static void onEnterTouchCalibration() {
	auto appContext = getAppContextFromId(APP_CONTEXT_ID_DEVICE);
    appContext->replacePage(PAGE_ID_TOUCH_CALIBRATION);
}

static void onTouchCalibrationOk() {
	auto appContext = getAppContextFromId(APP_CONTEXT_ID_DEVICE);
    appContext->popPage();
	appContext->infoMessage("Touch screen is calibrated.");
	appContext->showPage(appContext->getMainPageId());
}

static void onTouchCalibrationCancel() {
	auto appContext = getAppContextFromId(APP_CONTEXT_ID_DEVICE);
    appContext->showPage(appContext->getMainPageId());
}

static void onTouchCalibrationConfirm() {
	auto appContext = getAppContextFromId(APP_CONTEXT_ID_DEVICE);
    appContext->yesNoDialog(PAGE_ID_TOUCH_CALIBRATION_YES_NO, "Save changes?", touchCalibrationDialogYes, touchCalibrationDialogNo, nullptr);
}

static void getTouchScreenCalibrationParams(
    int16_t &touchScreenCalTlx, int16_t &touchScreenCalTly,
    int16_t &touchScreenCalBrx, int16_t &touchScreenCalBry,
    int16_t &touchScreenCalTrx, int16_t &touchScreenCalTry
) {
}

static void setTouchScreenCalibrationParams(
    int16_t touchScreenCalTlx, int16_t touchScreenCalTly,
    int16_t touchScreenCalBrx, int16_t touchScreenCalBry,
    int16_t touchScreenCalTrx, int16_t touchScreenCalTry
) {
}
#endif

static void onGuiQueueMessage(uint8_t type, int16_t param) {
}

////////////////////////////////////////////////////////////////////////////////

Hooks g_hooks = {
    getExtraLongTouchAction,
    getDefaultAnimationDuration,
    executeExternalAction,
    externalData,
	getWidgetTouchFunctionHook,
    getPageFromId,
    setFocusCursor,
    stateManagment,
    activePageHasBackdrop,
    executeActionThread,
    isEventHandlingDisabled,
    nullptr,
    overrideStyleColor,
    overrideActiveStyleColor,
    transformColor,
    styleGetSmallerFont,
    getDisplayBackgroundLuminosityStep,
    getSelectedThemeIndex,
    turnOnDisplayStart,
    turnOnDisplayTick,
    turnOffDisplayStart,
    turnOffDisplayTick,
    toastMessagePageOnEncoder,
#if OPTION_TOUCH_CALIBRATION
    onEnterTouchCalibration,
    onTouchCalibrationOk,
    onTouchCalibrationCancel,
    onTouchCalibrationConfirm,
    getTouchScreenCalibrationParams,
    setTouchScreenCalibrationParams,
#endif
    onGuiQueueMessage
};

} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
