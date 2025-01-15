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

#include <eez/core/assets.h>

#if EEZ_OPTION_GUI
#include <eez/gui/gui.h>
using namespace eez::gui;
#endif

namespace eez {
namespace flow {

#if defined(__EMSCRIPTEN__)
extern uint32_t g_wasmModuleId;
#endif

struct FlowState;

unsigned start(Assets *assets);
void tick();
void stop();

bool isFlowStopped();

#if EEZ_OPTION_GUI
FlowState *getPageFlowState(Assets *assets, int16_t pageIndex, const WidgetCursor &widgetCursor);
#else
FlowState *getPageFlowState(Assets *assets, int16_t pageIndex);
#endif
int getPageIndex(FlowState *flowState);

Value getGlobalVariable(uint32_t globalVariableIndex);
Value getGlobalVariable(Assets *assets, uint32_t globalVariableIndex);
void setGlobalVariable(uint32_t globalVariableIndex, const Value &value);
void setGlobalVariable(Assets *assets, uint32_t globalVariableIndex, const Value &value);

#if EEZ_OPTION_GUI
FlowState *getUserWidgetFlowState(FlowState *flowState, uint16_t userWidgetWidgetComponentIndex, int16_t pageId);
void executeFlowAction(const WidgetCursor &widgetCursor, int16_t actionId, void *param);
void dataOperation(int16_t dataId, DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value);
int16_t getNativeVariableId(const WidgetCursor &widgetCursor);
#endif

void setDebuggerMessageSubsciptionFilter(uint32_t filter);
void onDebuggerClientConnected();
void onDebuggerClientDisconnected();

void onArrayValueFree(ArrayValue *arrayValue);
void onFreeMQTTConnection(ArrayValue *mqttConnectionValue);

void executeScpi();
void flushToDebuggerMessage();

} // flow
} // eez
