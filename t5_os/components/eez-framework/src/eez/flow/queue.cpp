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

#include <eez/flow/queue.h>
#include <eez/flow/debugger.h>
#include <eez/flow/flow_defs_v3.h>

namespace eez {
namespace flow {

static const unsigned QUEUE_SIZE = 1000;
static struct {
	FlowState *flowState;
	unsigned componentIndex;
    bool continuousTask;
} g_queue[QUEUE_SIZE];
static unsigned g_queueHead;
static unsigned g_queueTail;
static bool g_queueIsFull = false;
unsigned g_numContinuousTaskInQueue;

void queueReset() {
	g_queueHead = 0;
	g_queueTail = 0;
	g_queueIsFull = false;
    g_numContinuousTaskInQueue = 0;
}

size_t getQueueSize() {
	if (g_queueHead == g_queueTail) {
		if (g_queueIsFull) {
			return QUEUE_SIZE;
		}
		return 0;
	}

	if (g_queueHead < g_queueTail) {
		return g_queueTail - g_queueHead;
	}

	return QUEUE_SIZE - g_queueHead + g_queueTail;
}

bool addToQueue(FlowState *flowState, unsigned componentIndex, int sourceComponentIndex, int sourceOutputIndex, int targetInputIndex, bool continuousTask) {
	if (g_queueIsFull) {
        throwError(flowState, componentIndex, "Execution queue is full\n");
		return false;
	}

	g_queue[g_queueTail].flowState = flowState;
	g_queue[g_queueTail].componentIndex = componentIndex;
    g_queue[g_queueTail].continuousTask = continuousTask;

	g_queueTail = (g_queueTail + 1) % QUEUE_SIZE;

	if (g_queueHead == g_queueTail) {
		g_queueIsFull = true;
	}

    if (!continuousTask) {
        ++g_numContinuousTaskInQueue;
	    onAddToQueue(flowState, sourceComponentIndex, sourceOutputIndex, componentIndex, targetInputIndex);
    }

    incRefCounterForFlowState(flowState);

	return true;
}

bool peekNextTaskFromQueue(FlowState *&flowState, unsigned &componentIndex, bool &continuousTask) {
	if (g_queueHead == g_queueTail && !g_queueIsFull) {
		return false;
	}

	flowState = g_queue[g_queueHead].flowState;
	componentIndex = g_queue[g_queueHead].componentIndex;
    continuousTask = g_queue[g_queueHead].continuousTask;

	return true;
}

void removeNextTaskFromQueue() {
	auto flowState = g_queue[g_queueHead].flowState;
    decRefCounterForFlowState(flowState);

    auto continuousTask = g_queue[g_queueHead].continuousTask;

	g_queueHead = (g_queueHead + 1) % QUEUE_SIZE;
	g_queueIsFull = false;

    if (!continuousTask) {
        --g_numContinuousTaskInQueue;
	    onRemoveFromQueue();
    }
}

bool isInQueue(FlowState *flowState, unsigned componentIndex) {
	if (g_queueHead == g_queueTail && !g_queueIsFull) {
		return false;
	}

    unsigned int it = g_queueHead;
    while (true) {
		if (g_queue[it].flowState == flowState && g_queue[it].componentIndex == componentIndex) {
            return true;
		}

        it = (it + 1) % QUEUE_SIZE;
        if (it == g_queueTail) {
            break;
        }
	}

    return false;
}

} // namespace flow
} // namespace eez
