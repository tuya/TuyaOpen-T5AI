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
#include <eez/flow/operations.h>
#include <eez/flow/private.h>
#include <eez/flow/hooks.h>

#include <eez/gui/gui.h>

namespace eez {
namespace flow {

struct ShowKeyboardActionComponent : public Component {
	uint8_t password;
};

void executeShowKeyboardComponent(FlowState *flowState, unsigned componentIndex) {
	auto component = (ShowKeyboardActionComponent *)flowState->flow->components[componentIndex];

    Value labelValue;
    if (!evalProperty(flowState, componentIndex, defs_v3::SHOW_KEYBOARD_ACTION_COMPONENT_PROPERTY_LABEL, labelValue, "Failed to evaluate Label in ShowKeyboard")) {
        return;
    }

    Value initialTextValue;
    if (!evalProperty(flowState, componentIndex, defs_v3::SHOW_KEYBOARD_ACTION_COMPONENT_PROPERTY_INITAL_TEXT, initialTextValue, "Failed to evaluate InitialText in ShowKeyboard")) {
        return;
    }

    Value minCharsValue;
    if (!evalProperty(flowState, componentIndex, defs_v3::SHOW_KEYBOARD_ACTION_COMPONENT_PROPERTY_MIN_CHARS, minCharsValue, "Failed to evaluate MinChars in ShowKeyboard")) {
        return;
    }

    Value maxCharsValue;
    if (!evalProperty(flowState, componentIndex, defs_v3::SHOW_KEYBOARD_ACTION_COMPONENT_PROPERTY_MAX_CHARS, maxCharsValue, "Failed to evaluate MaxChars in ShowKeyboard")) {
        return;
    }

	static FlowState *g_showKeyboardFlowState;
	static unsigned g_showKeyboardComponentIndex;

	g_showKeyboardFlowState = flowState;
	g_showKeyboardComponentIndex = componentIndex;

	startAsyncExecution(flowState, componentIndex);

	auto onOk = [](char *value) {
		propagateValue(g_showKeyboardFlowState, g_showKeyboardComponentIndex, 0, Value::makeStringRef(value, -1, 0x87d32fe2));
		getAppContextFromId(APP_CONTEXT_ID_DEVICE)->popPage();
		endAsyncExecution(g_showKeyboardFlowState, g_showKeyboardComponentIndex);
	};

	auto onCancel = []() {
		propagateValue(g_showKeyboardFlowState, g_showKeyboardComponentIndex, 1);
		getAppContextFromId(APP_CONTEXT_ID_DEVICE)->popPage();
		endAsyncExecution(g_showKeyboardFlowState, g_showKeyboardComponentIndex);
	};

	const char *label = labelValue.getString();
	if (label && *label) {
		labelValue = op_add(labelValue, Value(": "));
	}

	showKeyboardHook(labelValue, initialTextValue, minCharsValue, maxCharsValue, component->password, onOk, onCancel);
}

} // namespace flow
} // namespace eez

#endif // EEZ_OPTION_GUI
