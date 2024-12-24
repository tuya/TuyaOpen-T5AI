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

#include <eez/flow/watch_list.h>
#include <eez/flow/components.h>
#include <eez/flow/debugger.h>

namespace eez {
namespace flow {

void executeWatchVariableComponent(FlowState *flowState, unsigned componentIndex);

struct WatchListNode {
    FlowState *flowState;
    unsigned componentIndex;

    WatchListNode *prev;
    WatchListNode *next;
};

struct WatchList {
    WatchListNode *first;
    WatchListNode *last;
};

static WatchList g_watchList;

WatchListNode *watchListAdd(FlowState *flowState, unsigned componentIndex) {
    auto node = (WatchListNode *)alloc(sizeof(WatchListNode), 0x00864d67);

    node->prev = g_watchList.last;
    if (g_watchList.last != 0) {
        g_watchList.last->next = node;
    }
    g_watchList.last = node;

    if (g_watchList.first == 0) {
        g_watchList.first = node;
    }

    node->next = 0;

    node->flowState = flowState;
    node->componentIndex = componentIndex;

    incRefCounterForFlowState(flowState);

    return node;
}

void watchListRemove(WatchListNode *node) {
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        g_watchList.first = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    } else {
        g_watchList.last = node->prev;
    }

    free(node);
}

void visitWatchList() {
    for (auto node = g_watchList.first; node; ) {
        auto nextNode = node->next;

        if (canExecuteStep(node->flowState, node->componentIndex)) {
            executeWatchVariableComponent(node->flowState, node->componentIndex);
        }

        // If the only reason why flow state is still active is because of this watch then we can remove it.
        decRefCounterForFlowState(node->flowState);
        if (canFreeFlowState(node->flowState)) {
            // remove this watch
            freeFlowState(node->flowState);
            watchListRemove(node);
        } else {
            incRefCounterForFlowState(node->flowState);
        }

        node = nextNode;
    }
}

void watchListReset() {
    for (auto node = g_watchList.first; node;) {
        auto nextNode = node->next;
        watchListRemove(node);
        node = nextNode;
    }
}

} // namespace flow
} // namespace eez
