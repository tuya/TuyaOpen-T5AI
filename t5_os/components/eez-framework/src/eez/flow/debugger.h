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

#include <eez/flow/private.h>

namespace eez {
namespace flow {

extern bool g_debuggerIsConnected;

enum {
    DEBUGGER_MODE_RUN,
    DEBUGGER_MODE_DEBUG,
};
extern int g_debuggerMode;

bool canExecuteStep(FlowState *&flowState, unsigned &componentIndex);

void onStarted(Assets *assets);
void onStopped();

void onAddToQueue(FlowState *flowState, int sourceComponentIndex, int sourceOutputIndex, unsigned targetComponentIndex, int targetInputIndex);
void onRemoveFromQueue();

void onValueChanged(const Value *pValue);

void onFlowStateCreated(FlowState *flowState);
void onFlowStateDestroyed(FlowState *flowState);
void onFlowStateTimelineChanged(FlowState *flowState);

void onFlowError(FlowState *flowState, int componentIndex, const char *errorMessage);

void onComponentExecutionStateChanged(FlowState *flowState, int componentIndex);
void onComponentAsyncStateChanged(FlowState *flowState, int componentIndex);

void logInfo(FlowState *flowState, unsigned componentIndex, const char *message);

void logScpiCommand(FlowState *flowState, unsigned componentIndex, const char *cmd);
void logScpiQuery(FlowState *flowState, unsigned componentIndex, const char *query);
void logScpiQueryResult(FlowState *flowState, unsigned componentIndex, const char *resultText, size_t resultTextLen);

void onPageChanged(int previousPageId, int activePageId, bool activePageIsFromStack = false, bool previousPageIsStillOnStack = false);

void processDebuggerInput(char *buffer, uint32_t length);



} // flow
} // eez
