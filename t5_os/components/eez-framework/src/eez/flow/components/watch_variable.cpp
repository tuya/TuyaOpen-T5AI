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

#include <stdio.h>

#include <eez/core/alloc.h>
#include <eez/core/os.h>

#include <eez/flow/components.h>
#include <eez/flow/flow_defs_v3.h>
#include <eez/flow/expression.h>
#include <eez/flow/queue.h>
#include <eez/flow/watch_list.h>

namespace eez {
namespace flow {

struct WatchVariableComponenentExecutionState : public ComponenentExecutionState {
	Value value;
    WatchListNode *node;
};

void executeWatchVariableComponent(FlowState *flowState, unsigned componentIndex) {
	auto watchVariableComponentExecutionState = (WatchVariableComponenentExecutionState *)flowState->componenentExecutionStates[componentIndex];

    Value value;
    if (!evalProperty(flowState, componentIndex, defs_v3::WATCH_VARIABLE_ACTION_COMPONENT_PROPERTY_VARIABLE, value, "Failed to evaluate Variable in WatchVariable")) {
        return;
    }

	if (!watchVariableComponentExecutionState) {
        watchVariableComponentExecutionState = allocateComponentExecutionState<WatchVariableComponenentExecutionState>(flowState, componentIndex);
        watchVariableComponentExecutionState->value = value;
        watchVariableComponentExecutionState->node = watchListAdd(flowState, componentIndex);

        propagateValue(flowState, componentIndex, 1, value);
	} else {
		if (value != watchVariableComponentExecutionState->value) {
            watchVariableComponentExecutionState->value = value;
			propagateValue(flowState, componentIndex, 1, value);
		}
	}
}

} // namespace flow
} // namespace eez
