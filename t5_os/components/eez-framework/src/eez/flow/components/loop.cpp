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

struct LoopComponenentExecutionState : public ComponenentExecutionState {
    Value dstValue;
    Value toValue;

    Value currentValue;
};

void executeLoopComponent(FlowState *flowState, unsigned componentIndex) {
    auto component = flowState->flow->components[componentIndex];

    auto loopComponentExecutionState = (LoopComponenentExecutionState *)flowState->componenentExecutionStates[componentIndex];

    // restart loop if entered through "start" input
    static const unsigned START_INPUT_INDEX = 0;
    auto startInputIndex = component->inputs[START_INPUT_INDEX];
    if (flowState->values[startInputIndex].type != VALUE_TYPE_UNDEFINED) {
        if (loopComponentExecutionState) {
            deallocateComponentExecutionState(flowState, componentIndex);
            loopComponentExecutionState = nullptr;
        }
    } else {
        if (!loopComponentExecutionState) {
            return;
        }
    }

    Value stepValue;
    if (!evalProperty(flowState, componentIndex, defs_v3::LOOP_ACTION_COMPONENT_PROPERTY_STEP, stepValue, "Failed to evaluate Step in Loop")) {
        return;
    }

    Value currentValue;

    if (!loopComponentExecutionState) {
        Value dstValue;
        if (!evalAssignableProperty(flowState, componentIndex, defs_v3::LOOP_ACTION_COMPONENT_PROPERTY_VARIABLE, dstValue, "Failed to evaluate Variable in Loop")) {
            return;
        }

        Value fromValue;
        if (!evalProperty(flowState, componentIndex, defs_v3::LOOP_ACTION_COMPONENT_PROPERTY_FROM, fromValue, "Failed to evaluate From in Loop")) {
            return;
        }

        Value toValue;
        if (!evalProperty(flowState, componentIndex, defs_v3::LOOP_ACTION_COMPONENT_PROPERTY_TO, toValue, "Failed to evaluate To in Loop")) {
            return;
        }

        loopComponentExecutionState = allocateComponentExecutionState<LoopComponenentExecutionState>(flowState, componentIndex);
        loopComponentExecutionState->dstValue = dstValue;
        loopComponentExecutionState->toValue = toValue;

		currentValue = fromValue;
    } else {
        if (loopComponentExecutionState->dstValue.getType() == VALUE_TYPE_FLOW_OUTPUT) {
            currentValue = op_add(loopComponentExecutionState->currentValue, stepValue);
        } else {
            currentValue = op_add(loopComponentExecutionState->dstValue, stepValue);
        }
    }

    if (loopComponentExecutionState->dstValue.getType() == VALUE_TYPE_FLOW_OUTPUT) {
        loopComponentExecutionState->currentValue = currentValue;
    } else {
        assignValue(flowState, componentIndex, loopComponentExecutionState->dstValue, currentValue);
    }

    bool condition;
    if (stepValue.toDouble(nullptr) > 0) {
        condition = op_great(currentValue, loopComponentExecutionState->toValue).toBool();
    } else {
        condition = op_less(currentValue, loopComponentExecutionState->toValue).toBool();
    }

    if (condition) {
        // done
        deallocateComponentExecutionState(flowState, componentIndex);
        propagateValue(flowState, componentIndex, 1);
    } else {
        if (loopComponentExecutionState->dstValue.getType() == VALUE_TYPE_FLOW_OUTPUT) {
            assignValue(flowState, componentIndex, loopComponentExecutionState->dstValue, currentValue);
        }
        propagateValueThroughSeqout(flowState, componentIndex);
    }
}

} // namespace flow
} // namespace eez
