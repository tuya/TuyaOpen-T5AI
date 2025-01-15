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

namespace eez {
namespace flow {

void executeIsTrueComponent(FlowState *flowState, unsigned componentIndex) {
	Value srcValue;
	if (!evalProperty(flowState, componentIndex, defs_v3::IS_TRUE_ACTION_COMPONENT_PROPERTY_VALUE, srcValue, "Failed to evaluate Value in IsTrue")) {
		return;
	}

    int err;
    bool result = srcValue.toBool(&err);
    if (err == 0) {
        if (result) {
            propagateValue(flowState, componentIndex, 1, Value(true, VALUE_TYPE_BOOLEAN));
        } else {
            propagateValue(flowState, componentIndex, 2, Value(false, VALUE_TYPE_BOOLEAN));
        }
    } else {
        throwError(flowState, componentIndex, "Failed to convert Value to boolean in IsTrue\n");
        return;
    }

	propagateValueThroughSeqout(flowState, componentIndex);
}

} // namespace flow
} // namespace eez
