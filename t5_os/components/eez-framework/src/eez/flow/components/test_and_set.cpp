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
#include <eez/core/util.h>

#include <eez/flow/components.h>
#include <eez/flow/flow_defs_v3.h>
#include <eez/flow/expression.h>
#include <eez/flow/queue.h>

namespace eez {
namespace flow {

void executeTestAndSetComponent(FlowState *flowState, unsigned componentIndex) {
    Value dstValue;
    if (!evalAssignableProperty(flowState, componentIndex, defs_v3::TEST_AND_SET_ACTION_COMPONENT_PROPERTY_VARIABLE, dstValue, "Failed to evaluate Variable in TestAndSet")) {
        return;
    }

    if (dstValue.getValue().type != VALUE_TYPE_BOOLEAN) {
        throwError(flowState, componentIndex, "Variable in TestAndSet must be of type Boolean");
        return;
    }

    // waits for the variable to be false, then sets it to true and propagates through the Seqout

    // if the variable is false, set it to true and propagate through the Seqout
    if (!dstValue.getValue().getBoolean()) {
        assignValue(flowState, componentIndex, dstValue, Value(true, VALUE_TYPE_BOOLEAN));
        propagateValueThroughSeqout(flowState, componentIndex);
    } else {
        // otherwise, wait, i.e. add the component to the queue, so that it is executed again
        addToQueue(flowState, componentIndex, -1, -1, -1, true);
    }
}

} // namespace flow
} // namespace eez
