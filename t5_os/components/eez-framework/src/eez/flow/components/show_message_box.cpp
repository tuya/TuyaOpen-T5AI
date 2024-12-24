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

#if EEZ_OPTION_GUI

#include <eez/flow/flow.h>
#include <eez/flow/components.h>
#include <eez/flow/flow_defs_v3.h>
#include <eez/flow/expression.h>

#include <eez/gui/gui.h>

namespace eez {
namespace flow {

const uint8_t MESSAGE_BOX_TYPE_INFO = 1;
const uint8_t MESSAGE_BOX_TYPE_ERROR = 2;
const uint8_t MESSAGE_BOX_TYPE_QUESTION = 3;

struct ShowMessagePageActionComponent : public Component {
	uint8_t type;
};

struct ShowMessagePageComponentExecutionState : public ComponenentExecutionState {
	FlowState *flowState;
    unsigned componentIndex;
};

ShowMessagePageComponentExecutionState *g_executionState;

void infoMessageCallback() {
    auto flowState = g_executionState->flowState;
    auto componentIndex = g_executionState->componentIndex;
    g_executionState = nullptr;

    deallocateComponentExecutionState(flowState, componentIndex);

    propagateValueThroughSeqout(flowState, componentIndex);
}

void errorMessageCallback(int userParam) {
    auto flowState = g_executionState->flowState;
    auto componentIndex = g_executionState->componentIndex;
    g_executionState = nullptr;

    deallocateComponentExecutionState(flowState, componentIndex);

    propagateValueThroughSeqout(flowState, componentIndex);
}

void questionCallback(void *userParam, unsigned buttonIndex) {
    auto executionState = (ShowMessagePageComponentExecutionState *)userParam;

    auto flowState = executionState->flowState;
    auto componentIndex = executionState->componentIndex;

    deallocateComponentExecutionState(flowState, componentIndex);

    auto component = flowState->flow->components[componentIndex];

    if (buttonIndex < component->outputs.count - 1) {
        propagateValue(flowState, componentIndex, 1 + buttonIndex);
    }
}

void executeShowMessageBoxComponent(FlowState *flowState, unsigned componentIndex) {
	auto component = (ShowMessagePageActionComponent *)flowState->flow->components[componentIndex];

    Value messageValue;
    if (!evalProperty(flowState, componentIndex, defs_v3::SHOW_MESSAGE_BOX_ACTION_COMPONENT_PROPERTY_MESSAGE, messageValue, "Failed to evaluate Message in ShowMessageBox")) {
        return;
    }

    auto executionState = allocateComponentExecutionState<ShowMessagePageComponentExecutionState>(flowState, componentIndex);
    executionState->flowState = flowState;
    executionState->componentIndex = componentIndex;

	if (component->type == MESSAGE_BOX_TYPE_INFO) {
        g_executionState = executionState;
		getAppContextFromId(APP_CONTEXT_ID_DEVICE)->infoMessage(messageValue.getString(), infoMessageCallback, "Close");
	} else if (component->type == MESSAGE_BOX_TYPE_ERROR) {
        g_executionState = executionState;
		getAppContextFromId(APP_CONTEXT_ID_DEVICE)->errorMessageWithAction(messageValue, errorMessageCallback, "Close", 0);
	} else if (component->type == MESSAGE_BOX_TYPE_QUESTION) {
        Value buttonsValue;
        if (!evalProperty(flowState, componentIndex, defs_v3::SHOW_MESSAGE_BOX_ACTION_COMPONENT_PROPERTY_BUTTONS, buttonsValue, "Failed to evaluate Buttons in ShowMessageBox")) {
            return;
        }

        if (!buttonsValue.isArray()) {
            throwError(flowState, componentIndex, "Buttons in ShowMessageBox is not an array");
            return;
        }

        auto buttonsArray = buttonsValue.getArray();
        for (uint32_t i = 0; i < buttonsArray->arraySize; i++) {
            if (!buttonsArray->values[i].isString()) {
                char errorMessage[256];
                snprintf(errorMessage, sizeof(errorMessage), "Element at index %d is not a string in Buttons array in ShowMessageBox", (int)i);
                throwError(flowState, componentIndex, errorMessage);
                return;
            }
        }

        getAppContextFromId(APP_CONTEXT_ID_DEVICE)->questionDialog(messageValue, buttonsValue, executionState, questionCallback);
    }
}

} // namespace flow
} // namespace eez

#endif // EEZ_OPTION_GUI
