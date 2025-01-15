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
#include <eez/flow/components/call_action.h>

namespace eez {
namespace flow {

struct OutputActionComponent : public Component {
	uint8_t outputIndex;
};

void executeOutputComponent(FlowState *flowState, unsigned componentIndex) {
    auto component = (OutputActionComponent *)flowState->flow->components[componentIndex];

	if (!flowState->parentFlowState) {
		throwError(flowState, componentIndex, "No parentFlowState in Output\n");
		return;
	}

	if (!flowState->parentComponent) {
		throwError(flowState, componentIndex, "No parentComponent in Output\n");
		return;
	}

    auto inputIndex = component->inputs[0];
    if (inputIndex >= flowState->flow->componentInputs.count) {
        throwError(flowState, componentIndex, "Invalid input index in Output\n");
		return;
	}

    auto value = flowState->values[inputIndex];

    auto callActionComponent = (CallActionActionComponent *)flowState->parentComponent;

    uint8_t parentComponentOutputIndex = callActionComponent->outputsStartIndex + component->outputIndex;

    if (parentComponentOutputIndex >= flowState->parentComponent->outputs.count) {
        throwError(flowState, componentIndex, "Output action component, invalid output index\n");
		return;
    }

    propagateValue(flowState->parentFlowState, flowState->parentComponentIndex, parentComponentOutputIndex, value);
}

} // namespace flow
} // namespace eez
