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

#include <eez/flow/components/switch.h>

namespace eez {
namespace flow {

void executeSwitchComponent(FlowState *flowState, unsigned componentIndex) {
    auto component = (SwitchActionComponent *)flowState->flow->components[componentIndex];

    for (uint32_t testIndex = 0; testIndex < component->tests.count; testIndex++) {
        auto test = component->tests[testIndex];

        char strMessage[256];
        snprintf(strMessage, sizeof(strMessage), "Failed to evaluate test condition no. %d in Switch", (int)(testIndex + 1));

        Value conditionValue;
        if (!evalExpression(flowState, componentIndex, test->condition, conditionValue, strMessage)) {
            return;
        }

        int err;
        bool result = conditionValue.toBool(&err);
        if (err) {
            char strMessage[256];
            snprintf(strMessage, sizeof(strMessage), "Failed to convert test condition no. %d to boolean in Switch\n", (int)(testIndex + 1));
            throwError(flowState, componentIndex, strMessage);
            return;
        }

        if (result) {
            snprintf(strMessage, sizeof(strMessage), "Failed to evaluate test output value no. %d in Switch", (int)(testIndex + 1));

            Value outputValue;
            if (!evalExpression(flowState, componentIndex, test->outputValue, outputValue, strMessage)) {
                return;
            }

            propagateValue(flowState, componentIndex, test->outputIndex, outputValue);

            break;
        }
    }

	propagateValueThroughSeqout(flowState, componentIndex);
}

} // namespace flow
} // namespace eez
