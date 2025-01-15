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

namespace eez {
namespace gui {

struct Hooks {
    int (*getExtraLongTouchAction)();
    float (*getDefaultAnimationDuration)();
    void (*executeExternalAction)(const WidgetCursor &widgetCursor, int16_t actionId, void *param);
    void (*externalData)(int16_t id, DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value);
    OnTouchFunctionType (*getWidgetTouchFunction)(const WidgetCursor &widgetCursor);
    Page *(*getPageFromId)(int pageId);
    void (*setFocusCursor)(const WidgetCursor& cursor, int16_t dataId);
    void (*stateManagment)();
    bool (*activePageHasBackdrop)();
    void (*executeActionThread)();
    bool (*isEventHandlingDisabled)();
    int (*overrideStyle)(const WidgetCursor &widgetCursor, int styleId);
    uint16_t (*overrideStyleColor)(const WidgetCursor &widgetCursor, const Style *style);
    uint16_t (*overrideActiveStyleColor)(const WidgetCursor &widgetCursor, const Style *style);
    uint16_t (*transformColor)(uint16_t color);
    bool (*styleGetSmallerFont)(font::Font &font);
    uint8_t (*getDisplayBackgroundLuminosityStep)();
    uint8_t (*getSelectedThemeIndex)();
    void (*turnOnDisplayStart)();
    void (*turnOnDisplayTick)();
    void (*turnOffDisplayStart)();
    void (*turnOffDisplayTick)();
    void (*toastMessagePageOnEncoder)(ToastMessagePage *toast, int counter);
#if OPTION_TOUCH_CALIBRATION
    void (*onEnterTouchCalibration)();
    void (*onTouchCalibrationOk)();
    void (*onTouchCalibrationCancel)();
    void (*onTouchCalibrationConfirm)();
    void (*getTouchScreenCalibrationParams)(
        int16_t &touchScreenCalTlx, int16_t &touchScreenCalTly,
        int16_t &touchScreenCalBrx, int16_t &touchScreenCalBry,
        int16_t &touchScreenCalTrx, int16_t &touchScreenCalTry
    );
    void (*setTouchScreenCalibrationParams)(
        int16_t touchScreenCalTlx, int16_t touchScreenCalTly,
        int16_t touchScreenCalBrx, int16_t touchScreenCalBry,
        int16_t touchScreenCalTrx, int16_t touchScreenCalTry
    );
#endif
    void (*onGuiQueueMessage)(uint8_t type, int16_t param);
};

extern Hooks g_hooks;

} // namespace gui
} // namespace eez
