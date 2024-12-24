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

#include <eez/flow/components.h>
#include <eez/flow/flow_defs_v3.h>
#include <eez/flow/expression.h>
#include <eez/flow/components/set_variable.h>

namespace eez {
namespace flow {

void executeSetVariableComponent(FlowState *flowState, unsigned componentIndex) {
    auto component = (SetVariableActionComponent *)flowState->flow->components[componentIndex];

    for (uint32_t entryIndex = 0; entryIndex < component->entries.count; entryIndex++) {
        auto entry = component->entries[entryIndex];

        char strErrorMessage[256];
        snprintf(strErrorMessage, sizeof(strErrorMessage), "Failed to evaluate Variable no. %d in SetVariable", (int)(entryIndex + 1));

        Value dstValue;
        if (!evalAssignableExpression(flowState, componentIndex, entry->variable, dstValue, strErrorMessage)) {
            return;
        }

        snprintf(strErrorMessage, sizeof(strErrorMessage), "Failed to evaluate Value no. %d in SetVariable", (int)(entryIndex + 1));

        Value srcValue;
        if (!evalExpression(flowState, componentIndex, entry->value, srcValue, strErrorMessage)) {
            return;
        }

        assignValue(flowState, componentIndex, dstValue, srcValue);
    }

	propagateValueThroughSeqout(flowState, componentIndex);
}

} // namespace flow
} // namespace eez
