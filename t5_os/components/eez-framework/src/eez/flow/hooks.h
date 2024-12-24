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

#include <stdint.h>
#include <stdlib.h>

#include <eez/core/value.h>
#include <eez/flow/private.h>

#if defined(EEZ_FOR_LVGL)
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#endif

namespace eez {
namespace flow {

extern void (*replacePageHook)(int16_t pageId, uint32_t animType, uint32_t speed, uint32_t delay);
extern void (*showKeyboardHook)(Value label, Value initialText, Value minChars, Value maxChars, bool isPassword, void(*onOk)(char *), void(*onCancel)());
extern void (*showKeypadHook)(Value label, Value initialValue, Value min, Value max, Unit unit, void(*onOk)(float), void(*onCancel)());
extern void (*stopScriptHook)();

extern void (*scpiComponentInitHook)();

extern void (*startToDebuggerMessageHook)();
extern void (*writeDebuggerBufferHook)(const char *buffer, uint32_t length);
extern void (*finishToDebuggerMessageHook)();
extern void (*onDebuggerInputAvailableHook)();

extern void (*executeDashboardComponentHook)(uint16_t componentType, int flowStateIndex, int componentIndex);

extern void (*onArrayValueFreeHook)(ArrayValue *arrayValue);

#if defined(EEZ_FOR_LVGL)
extern lv_obj_t *(*getLvglObjectFromIndexHook)(int32_t index);
extern const void *(*getLvglImageByNameHook)(const char *name);
extern void (*executeLvglActionHook)(int actionIndex);
#endif

extern double (*getDateNowHook)();

extern void (*onFlowErrorHook)(FlowState *flowState, int componentIndex, const char *errorMessage);

} // flow
} // eez
