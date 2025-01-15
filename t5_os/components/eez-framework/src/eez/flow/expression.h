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

#include <eez/core/util.h>
#include <eez/flow/private.h>

namespace eez {
namespace flow {

static const size_t STACK_SIZE = 20;

struct EvalStack {
	FlowState *flowState;
	int componentIndex;
	const int32_t *iterators;

	Value stack[STACK_SIZE];
	size_t sp = 0;

    char errorMessage[512];

	bool push(const Value &value) {
		if (sp >= STACK_SIZE) {
			throwError(flowState, componentIndex, "Evaluation stack is full\n");
			return false;
		}
		stack[sp++] = value;
		return true;
	}

	bool push(Value *pValue) {
		if (sp >= STACK_SIZE) {
			return false;
		}
		stack[sp++] = Value(pValue, VALUE_TYPE_VALUE_PTR);
		return true;
	}

	Value pop() {
        if (sp == 0) {
            return Value::makeError();
        }
		return stack[--sp];
	}

    void setErrorMessage(const char *str) {
        stringCopy(errorMessage, sizeof(errorMessage), str);
    }
};

#if EEZ_OPTION_GUI
bool evalExpression(FlowState *flowState, int componentIndex, const uint8_t *instructions, Value &result, const char *errorMessage, int *numInstructionBytes = nullptr, const int32_t *iterators = nullptr, eez::gui::DataOperationEnum operation = eez::gui::DATA_OPERATION_GET);
#else
bool evalExpression(FlowState *flowState, int componentIndex, const uint8_t *instructions, Value &result, const char *errorMessage, int *numInstructionBytes = nullptr, const int32_t *iterators = nullptr);
#endif
bool evalAssignableExpression(FlowState *flowState, int componentIndex, const uint8_t *instructions, Value &result, const char *errorMessage, int *numInstructionBytes = nullptr, const int32_t *iterators = nullptr);

#if EEZ_OPTION_GUI
bool evalProperty(FlowState *flowState, int componentIndex, int propertyIndex, Value &result, const char *errorMessage, int *numInstructionBytes = nullptr, const int32_t *iterators = nullptr, eez::gui::DataOperationEnum operation = eez::gui::DATA_OPERATION_GET);
#else
bool evalProperty(FlowState *flowState, int componentIndex, int propertyIndex, Value &result, const char *errorMessage, int *numInstructionBytes = nullptr, const int32_t *iterators = nullptr);
#endif
bool evalAssignableProperty(FlowState *flowState, int componentIndex, int propertyIndex, Value &result, const char *errorMessage, int *numInstructionBytes = nullptr, const int32_t *iterators = nullptr);

} // flow
} // eez
