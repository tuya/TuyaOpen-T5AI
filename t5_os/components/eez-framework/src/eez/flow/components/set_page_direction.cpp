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

#include <eez/flow/components.h>
#include <eez/flow/flow_defs_v3.h>
#include <eez/flow/expression.h>
#include <eez/flow/private.h>

#include <eez/gui/gui.h>
using namespace eez::gui;

namespace eez {
namespace flow {

#define PAGE_DIRECTION_LTR 0
#define PAGE_DIRECTION_RTL 1

struct SetPageDirectionComponent : public Component {
	uint8_t direction;
};

void executeSetPageDirectionComponent(FlowState *flowState, unsigned componentIndex) {
	auto component = (SetPageDirectionComponent *)flowState->flow->components[componentIndex];
    gui::g_isRTL = component->direction == PAGE_DIRECTION_RTL;
    gui::refreshScreen();
    propagateValueThroughSeqout(flowState, componentIndex);
}

} // namespace flow
} // namespace eez

#endif // EEZ_OPTION_GUI
