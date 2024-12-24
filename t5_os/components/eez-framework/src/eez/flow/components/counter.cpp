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

#include <eez/flow/components.h>
#include <eez/flow/flow_defs_v3.h>
#include <eez/flow/expression.h>
#include <eez/flow/operations.h>

namespace eez {
namespace flow {

struct CounterComponenentExecutionState : public ComponenentExecutionState {
    int counter;
};

void executeCounterComponent(FlowState *flowState, unsigned componentIndex) {
    auto counterComponenentExecutionState = (CounterComponenentExecutionState *)flowState->componenentExecutionStates[componentIndex];

    if (!counterComponenentExecutionState) {
        Value counterValue;
        if (!evalProperty(flowState, componentIndex, defs_v3::COUNTER_ACTION_COMPONENT_PROPERTY_COUNT_VALUE, counterValue, "Failed to evaluate countValue in Counter")) {
            return;
        }

        counterComponenentExecutionState = allocateComponentExecutionState<CounterComponenentExecutionState>(flowState, componentIndex);
        counterComponenentExecutionState->counter = counterValue.getInt();
    }

    if (counterComponenentExecutionState->counter > 0) {
        counterComponenentExecutionState->counter--;
        propagateValueThroughSeqout(flowState, componentIndex);
    } else {
        // done
        deallocateComponentExecutionState(flowState, componentIndex);
        propagateValue(flowState, componentIndex, 1);
    }
}

} // namespace flow
} // namespace eez
