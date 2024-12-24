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

#include <eez/flow/private.h>

namespace eez {
namespace gui {

struct InputWidget : public Widget {
	uint16_t flags;
	int16_t min;
	int16_t max;
    int16_t precision;
	int16_t unit;
	uint16_t componentIndex;
};

struct InputWidgetState : public WidgetState {
	WidgetStateFlags flags;
	Value data;

    bool updateState() override;
	void render() override;
};

struct InputWidgetExecutionState : public flow::ComponenentExecutionState {
	Value min;
	Value max;
    Value precision;
	Unit unit;
};

static const uint16_t INPUT_WIDGET_TYPE_TEXT = 0x0001;
static const uint16_t INPUT_WIDGET_TYPE_NUMBER = 0x0002;
static const uint16_t INPUT_WIDGET_PASSWORD_FLAG = 0x0100;

Value getInputWidgetMin(const gui::WidgetCursor &widgetCursor);
Value getInputWidgetMax(const gui::WidgetCursor &widgetCursor);
Value getInputWidgetPrecision(const gui::WidgetCursor &widgetCursor);
Unit getInputWidgetUnit(const gui::WidgetCursor &widgetCursor);

Value getInputWidgetData(const gui::WidgetCursor &widgetCursor, const Value &dataValue);

} // namespace gui
} // namespace eez
