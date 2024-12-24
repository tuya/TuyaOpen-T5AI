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

#include <eez/core/os.h>
#include <eez/core/assets.h>
#include <eez/core/value.h>

#if EEZ_OPTION_GUI
#include <eez/gui/gui.h>
using namespace eez::gui;
#endif

namespace eez {
namespace flow {

static const int UNDEFINED_VALUE_INDEX = 0;
static const int NULL_VALUE_INDEX = 1;

#define TRACK_REF_COUNTER_FOR_COMPONENT_STATE(component) \
    !( \
        component->type == defs_v3::COMPONENT_TYPE_INPUT_ACTION || \
        component->type == defs_v3::COMPONENT_TYPE_LOOP_ACTION || \
        component->type == defs_v3::COMPONENT_TYPE_COUNTER_ACTION || \
        component->type == defs_v3::COMPONENT_TYPE_WATCH_VARIABLE_ACTION \
    ) \

struct ComponenentExecutionState {
    uint32_t lastExecutedTime;
    ComponenentExecutionState() : lastExecutedTime(millis()) {}
	virtual ~ComponenentExecutionState() {}
};

struct CatchErrorComponenentExecutionState : public ComponenentExecutionState {
	Value message;
};

struct FlowState {
	uint32_t flowStateIndex;
	Assets *assets;
	FlowDefinition *flowDefinition;
	Flow *flow;
	uint16_t flowIndex;
	bool isAction;
	bool error;

    // Used for freeing flow state. While this is greater than 0, flow state cannot be freed
    // because it is still active. It is active if:
    //   - there is component in the queue
    //   - there is async component
    //   - there is component with execution state (not all components are tracked, check TRACK_REF_COUNTER_FOR_COMPONENT_STATE)
    //   - there is watch component in watch_list
    uint32_t refCounter;

    FlowState *parentFlowState;
	Component *parentComponent;
	int parentComponentIndex;
	Value *values;
	ComponenentExecutionState **componenentExecutionStates;
    bool *componenentAsyncStates;
    unsigned executingComponentIndex;
    float timelinePosition;
#if defined(EEZ_FOR_LVGL)
    int32_t lvglWidgetStartIndex;
#endif
    Value eventValue;

    FlowState *firstChild;
    FlowState *lastChild;
    FlowState *previousSibling;
    FlowState *nextSibling;
};

extern int g_selectedLanguage;
extern FlowState *g_firstFlowState;
extern FlowState *g_lastFlowState;

FlowState *initActionFlowState(int flowIndex, FlowState *parentFlowState, int parentComponentIndex);
FlowState *initPageFlowState(Assets *assets, int flowIndex, FlowState *parentFlowState, int parentComponentIndex);

void incRefCounterForFlowState(FlowState *flowState);
void decRefCounterForFlowState(FlowState *flowState);

bool canFreeFlowState(FlowState *flowState);
void freeFlowState(FlowState *flowState);
void freeAllChildrenFlowStates(FlowState *flowState);

void deallocateComponentExecutionState(FlowState *flowState, unsigned componentIndex);

extern void onComponentExecutionStateChanged(FlowState *flowState, int componentIndex);
template<class T>
T *allocateComponentExecutionState(FlowState *flowState, unsigned componentIndex) {
    if (flowState->componenentExecutionStates[componentIndex]) {
        deallocateComponentExecutionState(flowState, componentIndex);
    }
    auto executionState = ObjectAllocator<T>::allocate(0x72dc3bf4);
    flowState->componenentExecutionStates[componentIndex] = executionState;

    auto component = flowState->flow->components[componentIndex];
    if (TRACK_REF_COUNTER_FOR_COMPONENT_STATE(component)) {
        incRefCounterForFlowState(flowState);
    }

    onComponentExecutionStateChanged(flowState, componentIndex);
    return executionState;
}

void resetSequenceInputs(FlowState *flowState);

void propagateValue(FlowState *flowState, unsigned componentIndex, unsigned outputIndex, const Value &value);
void propagateValue(FlowState *flowState, unsigned componentIndex, unsigned outputIndex); // propagates null value
void propagateValueThroughSeqout(FlowState *flowState, unsigned componentIndex); // propagates null value through @seqout (0-th output)

#if EEZ_OPTION_GUI
void getValue(uint16_t dataId, DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value);
void setValue(uint16_t dataId, const WidgetCursor &widgetCursor, const Value& value);
#endif

void assignValue(FlowState *flowState, int componentIndex, Value &dstValue, const Value &srcValue);

void clearInputValue(FlowState *flowState, int inputIndex);

void startAsyncExecution(FlowState *flowState, int componentIndex);
void endAsyncExecution(FlowState *flowState, int componentIndex);

void executeCallAction(FlowState *flowState, unsigned componentIndex, int flowIndex);

enum FlowEvent {
    FLOW_EVENT_OPEN_PAGE,
    FLOW_EVENT_CLOSE_PAGE,
    FLOW_EVENT_KEYDOWN
};

void onEvent(FlowState *flowState, FlowEvent flowEvent, Value eventValue);

void throwError(FlowState *flowState, int componentIndex, const char *errorMessage);
void throwError(FlowState *flowState, int componentIndex, const char *errorMessage, const char *errorMessageDescription);

void enableThrowError(bool enable);

} // flow
} // eez
