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

#include <eez/flow/components/input.h>
#include <eez/flow/components/call_action.h>
#include <eez/flow/flow_defs_v3.h>

namespace eez {
namespace flow {

bool getCallActionValue(FlowState *flowState, unsigned componentIndex, Value &value) {
	auto component = flowState->flow->components[componentIndex];

	if (!flowState->parentFlowState) {
		throwError(flowState, componentIndex, "No parentFlowState in Input\n");
		return false;
	}

	if (!flowState->parentComponent) {
		throwError(flowState, componentIndex, "No parentComponent in Input\n");
		return false;
	}

    auto callActionComponent = (CallActionActionComponent *)flowState->parentComponent;

    uint8_t callActionComponentInputIndex = callActionComponent->inputsStartIndex;
    if (component->type == defs_v3::COMPONENT_TYPE_INPUT_ACTION) {
        auto inputActionComponent = (InputActionComponent *)component;
        callActionComponentInputIndex += inputActionComponent->inputIndex;
    } else {
        callActionComponentInputIndex -= 1;
    }

    if (callActionComponentInputIndex >= callActionComponent->inputs.count) {
        throwError(flowState, componentIndex, "Invalid input index in Input\n");
        return false;
    }

    auto &parentComponentInputs = callActionComponent->inputs;
    auto parentFlowInputIndex = parentComponentInputs[callActionComponentInputIndex];

    auto parentFlow = flowState->flowDefinition->flows[flowState->parentFlowState->flowIndex];
    if (parentFlowInputIndex >= parentFlow->componentInputs.count) {
        throwError(flowState, componentIndex, "Invalid input index of parent component in Input\n");
        return false;
    }

    value = flowState->parentFlowState->values[parentFlowInputIndex];
    return true;
}

void executeInputComponent(FlowState *flowState, unsigned componentIndex) {
	Value value;
    if (getCallActionValue(flowState, componentIndex, value)) {
        auto inputActionComponentExecutionState = (InputActionComponentExecutionState *)flowState->componenentExecutionStates[componentIndex];
        if (!inputActionComponentExecutionState) {
            inputActionComponentExecutionState = allocateComponentExecutionState<InputActionComponentExecutionState>(flowState, componentIndex);
        }

        propagateValue(flowState, componentIndex, 0, value);
        inputActionComponentExecutionState->value = value;
    }
}

} // namespace flow
} // namespace eez
