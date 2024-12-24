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

#include <eez/core/os.h>

#include <eez/flow/components.h>
#include <eez/flow/flow_defs_v3.h>
#include <eez/flow/expression.h>
#include <eez/flow/private.h>
#include <eez/flow/queue.h>
#include <eez/flow/debugger.h>

#if EEZ_OPTION_GUI
#include <eez/gui/gui.h>
using namespace eez::gui;
#endif

namespace eez {
namespace flow {

struct AnimateComponenentExecutionState : public ComponenentExecutionState {
    float startPosition;
    float endPosition;
    float speed;
    uint32_t startTimestamp;
};

void executeAnimateComponent(FlowState *flowState, unsigned componentIndex) {
	auto state = (AnimateComponenentExecutionState *)flowState->componenentExecutionStates[componentIndex];
	if (!state) {
        Value fromValue;
        if (!evalProperty(flowState, componentIndex, defs_v3::ANIMATE_ACTION_COMPONENT_PROPERTY_FROM, fromValue, "Failed to evaluate From in Animate")) {
            return;
        }

        Value toValue;
        if (!evalProperty(flowState, componentIndex, defs_v3::ANIMATE_ACTION_COMPONENT_PROPERTY_TO, toValue, "Failed to evaluate To in Animate")) {
            return;
        }

        Value speedValue;
        if (!evalProperty(flowState, componentIndex, defs_v3::ANIMATE_ACTION_COMPONENT_PROPERTY_SPEED, speedValue, "Failed to evaluate Speed in Animate")) {
            return;
        }

        float from = fromValue.toFloat();
        float to = toValue.toFloat();
        float speed = speedValue.toFloat();

        if (speed == 0) {
            flowState->timelinePosition = to;
            onFlowStateTimelineChanged(flowState);

            propagateValueThroughSeqout(flowState, componentIndex);
        } else {
		    state = allocateComponentExecutionState<AnimateComponenentExecutionState>(flowState, componentIndex);

            state->startPosition = from;
            state->endPosition = to;
            state->speed = speed;
            state->startTimestamp = millis();

            if (!addToQueue(flowState, componentIndex, -1, -1, -1, true)) {
                return;
            }
        }
    } else {
        float currentTime;

        if (state->startPosition < state->endPosition) {
            currentTime = state->startPosition + state->speed * (millis() - state->startTimestamp) / 1000.0f;
            if (currentTime >= state->endPosition) {
                currentTime = state->endPosition;
            }
        } else {
            currentTime = state->startPosition - state->speed * (millis() - state->startTimestamp) / 1000.0f;
            if (currentTime <= state->endPosition) {
                currentTime = state->endPosition;
            }
        }

        flowState->timelinePosition = currentTime;
        onFlowStateTimelineChanged(flowState);

        if (currentTime == state->endPosition) {
            deallocateComponentExecutionState(flowState, componentIndex);
            propagateValueThroughSeqout(flowState, componentIndex);
        } else {
            if (!addToQueue(flowState, componentIndex, -1, -1, -1, true)) {
                return;
            }
        }
    }
}

} // namespace flow
} // namespace eez
