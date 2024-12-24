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

#include <eez/core/alloc.h>
#include <eez/core/os.h>

#include <eez/flow/components.h>
#include <eez/flow/flow.h>
#include <eez/flow/flow_defs_v3.h>
#include <eez/flow/queue.h>
#include <eez/flow/components/input.h>
#include <eez/flow/components/lvgl_user_widget.h>

#if defined(EEZ_FOR_LVGL)

namespace eez {
namespace flow {

struct LVGLUserWidgetComponent : public Component {
	int16_t flowIndex;
	uint8_t inputsStartIndex;
	uint8_t outputsStartIndex;
    int32_t widgetStartIndex;
};

LVGLUserWidgetExecutionState *createUserWidgetFlowState(FlowState *flowState, unsigned userWidgetWidgetComponentIndex) {
    auto component = (LVGLUserWidgetComponent *)flowState->flow->components[userWidgetWidgetComponentIndex];
    auto userWidgetFlowState = initPageFlowState(flowState->assets, component->flowIndex, flowState, userWidgetWidgetComponentIndex);
    userWidgetFlowState->lvglWidgetStartIndex = component->widgetStartIndex;
    auto userWidgetWidgetExecutionState = allocateComponentExecutionState<LVGLUserWidgetExecutionState>(flowState, userWidgetWidgetComponentIndex);
    userWidgetWidgetExecutionState->flowState = userWidgetFlowState;
    return userWidgetWidgetExecutionState;
}

void executeLVGLUserWidgetComponent(FlowState *flowState, unsigned componentIndex) {
    auto component = (LVGLUserWidgetComponent *)flowState->flow->components[componentIndex];

    auto userWidgetWidgetExecutionState = (LVGLUserWidgetExecutionState *)flowState->componenentExecutionStates[componentIndex];
    if (!userWidgetWidgetExecutionState) {
        userWidgetWidgetExecutionState = createUserWidgetFlowState(flowState, componentIndex);
    }
    auto userWidgetFlowState = userWidgetWidgetExecutionState->flowState;

    for (
        unsigned userWidgetComponentIndex = 0;
        userWidgetComponentIndex < userWidgetFlowState->flow->components.count;
        userWidgetComponentIndex++
    ) {
        auto userWidgetComponent = userWidgetFlowState->flow->components[userWidgetComponentIndex];
        if (userWidgetComponent->type == defs_v3::COMPONENT_TYPE_INPUT_ACTION) {
            auto inputActionComponentExecutionState = (InputActionComponentExecutionState *)userWidgetFlowState->componenentExecutionStates[userWidgetComponentIndex];
            if (inputActionComponentExecutionState) {
                Value value;
                if (getCallActionValue(userWidgetFlowState, userWidgetComponentIndex, value)) {
                    if (inputActionComponentExecutionState->value != value) {
                        addToQueue(userWidgetWidgetExecutionState->flowState, userWidgetComponentIndex, -1, -1, -1, false);
                        inputActionComponentExecutionState->value = value;
                    }
                } else {
                    return;
                }
            }
        } else if (userWidgetComponent->type == defs_v3::COMPONENT_TYPE_START_ACTION) {
            Value value;
            if (getCallActionValue(userWidgetFlowState, userWidgetComponentIndex, value)) {
                if (value.getType() != VALUE_TYPE_UNDEFINED) {
                    addToQueue(userWidgetWidgetExecutionState->flowState, userWidgetComponentIndex, -1, -1, -1, false);
                }
            } else {
                return;
            }
        }
    }
}

} // namespace flow
} // namespace eez


#else

namespace eez {
namespace flow {

void executeLVGLUserWidgetComponent(FlowState *flowState, unsigned componentIndex) {
    throwError(flowState, componentIndex, "Not implemented");
}

} // namespace flow
} // namespace eez

#endif
