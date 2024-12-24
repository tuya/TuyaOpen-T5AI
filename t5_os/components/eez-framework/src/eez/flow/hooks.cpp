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

#include <assert.h>
#include <math.h>

#include <eez/flow/hooks.h>

#include <eez/core/util.h>

#if EEZ_OPTION_GUI
#include <eez/gui/gui.h>
#endif

#include <chrono>

namespace eez {
namespace flow {

static void replacePage(int16_t pageId, uint32_t animType, uint32_t speed, uint32_t delay) {
#if EEZ_OPTION_GUI
	eez::gui::getAppContextFromId(APP_CONTEXT_ID_DEVICE)->replacePage(pageId);
#endif
}

static void showKeyboard(Value label, Value initialText, Value minChars, Value maxChars, bool isPassword, void(*onOk)(char *), void(*onCancel)()) {
}

static void showKeypad(Value label, Value initialValue, Value min, Value max, Unit unit, void(*onOk)(float), void(*onCancel)()) {
}

static void stopScript() {
	assert(false);
}

static void scpiComponentInit() {
}

static void startToDebuggerMessage() {
}

static void writeDebuggerBuffer(const char *buffer, uint32_t length) {
}

static void finishToDebuggerMessage() {
}

static void onDebuggerInputAvailable() {
}

void (*replacePageHook)(int16_t pageId, uint32_t animType, uint32_t speed, uint32_t delay) = replacePage;
void (*showKeyboardHook)(Value label, Value initialText, Value minChars, Value maxChars, bool isPassword, void(*onOk)(char *), void(*onCancel)()) = showKeyboard;
void (*showKeypadHook)(Value label, Value initialValue, Value min, Value max, Unit unit, void(*onOk)(float), void(*onCancel)()) = showKeypad;
void (*stopScriptHook)() = stopScript;

void (*scpiComponentInitHook)() = scpiComponentInit;

void (*startToDebuggerMessageHook)() = startToDebuggerMessage;
void (*writeDebuggerBufferHook)(const char *buffer, uint32_t length) = writeDebuggerBuffer;
void (*finishToDebuggerMessageHook)() = finishToDebuggerMessage;
void (*onDebuggerInputAvailableHook)() = onDebuggerInputAvailable;

void (*executeDashboardComponentHook)(uint16_t componentType, int flowStateIndex, int componentIndex) = nullptr;

void (*onArrayValueFreeHook)(ArrayValue *arrayValue) = nullptr;

#if defined(EEZ_FOR_LVGL)
static lv_obj_t *getLvglObjectFromIndex(int32_t index) {
    return 0;
}

static const void *getLvglImageByName(const char *name) {
    return 0;
}

static void executeLvglAction(int actionIndex) {
}

lv_obj_t *(*getLvglObjectFromIndexHook)(int32_t index) = getLvglObjectFromIndex;
const void *(*getLvglImageByNameHook)(const char *name) = getLvglImageByName;
void (*executeLvglActionHook)(int actionIndex) = executeLvglAction;
#endif

double getDateNowDefaultImplementation() {
    using namespace std::chrono;
    milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    return (double)ms.count();
}

double (*getDateNowHook)() = getDateNowDefaultImplementation;

void (*onFlowErrorHook)(FlowState *flowState, int componentIndex, const char *errorMessage) = nullptr;

} // namespace flow
} // namespace eez

