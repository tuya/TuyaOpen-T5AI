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

#include <math.h>

#include <eez/core/alloc.h>
#include <eez/core/util.h>

#include <eez/gui/gui.h>
#include <eez/gui/data.h>

#include <eez/gui/widgets/input.h>

#include <eez/flow/private.h>

namespace eez {
namespace gui {

static const size_t MAX_TEXT_LEN = 128;

void getInputWidgetParams(const gui::WidgetCursor &widgetCursor, Value &min, Value &max, Value &precision, Unit &unit) {
	auto widget = (const InputWidget*)widgetCursor.widget;

	auto unitValue = get(widgetCursor, widget->unit);
	unit = getUnitFromName(unitValue.toString(0x5049bd52).getString());

	auto minValue = get(widgetCursor, widget->min);
	auto maxValue = get(widgetCursor, widget->max);
	auto precisionValue = get(widgetCursor, widget->precision);

	float factor = getUnitFactor(unit);
	auto baseUnit = getBaseUnit(unit);

	min = Value(minValue.toFloat() * factor, baseUnit);
	max = Value(maxValue.toFloat() * factor, baseUnit);
	precision = Value(precisionValue.toFloat() * factor, baseUnit);
}

Value getInputWidgetMin(const gui::WidgetCursor &widgetCursor) {
	auto widget = (const InputWidget*)widgetCursor.widget;

	auto flowState = widgetCursor.flowState;
	if (flowState) {
		auto inputWidgetExecutionState = (InputWidgetExecutionState *)flowState->componenentExecutionStates[widget->componentIndex];
		if (inputWidgetExecutionState) {
			return inputWidgetExecutionState->min;
		}
	}

	auto unitValue = get(widgetCursor, widget->unit);
	auto unit = getUnitFromName(unitValue.toString(0x5049bd52).getString());
	auto baseUnit = getBaseUnit(unit);

	auto value = get(widgetCursor, widget->min);
	float factor = getUnitFactor(unit);
	return Value(value.toFloat() * factor, baseUnit);
}

Value getInputWidgetMax(const gui::WidgetCursor &widgetCursor) {
	auto widget = (const InputWidget*)widgetCursor.widget;

	auto flowState = widgetCursor.flowState;
	if (flowState) {
		auto inputWidgetExecutionState = (InputWidgetExecutionState *)flowState->componenentExecutionStates[widget->componentIndex];
		if (inputWidgetExecutionState) {
			return inputWidgetExecutionState->max;
		}
	}

	auto unitValue = get(widgetCursor, widget->unit);
	auto unit = getUnitFromName(unitValue.toString(0x5049bd52).getString());
	auto baseUnit = getBaseUnit(unit);

	auto value = get(widgetCursor, widget->max);
	float factor = getUnitFactor(unit);
	return Value(value.toFloat() * factor, baseUnit);
}

Value getInputWidgetPrecision(const gui::WidgetCursor &widgetCursor) {
	auto widget = (const InputWidget*)widgetCursor.widget;

	auto flowState = widgetCursor.flowState;
	if (flowState) {
		auto inputWidgetExecutionState = (InputWidgetExecutionState *)flowState->componenentExecutionStates[widget->componentIndex];
		if (inputWidgetExecutionState) {
			return inputWidgetExecutionState->precision;
		}
	}

	auto unitValue = get(widgetCursor, widget->unit);
	auto unit = getUnitFromName(unitValue.toString(0x5049bd52).getString());
	auto baseUnit = getBaseUnit(unit);

	auto value = get(widgetCursor, widget->precision);
	float factor = getUnitFactor(unit);
	return Value(value.toFloat() * factor, baseUnit);
}

Unit getInputWidgetUnit(const gui::WidgetCursor &widgetCursor) {
	auto widget = (const InputWidget*)widgetCursor.widget;

	auto flowState = widgetCursor.flowState;
	if (flowState) {
		auto inputWidgetExecutionState = (InputWidgetExecutionState *)flowState->componenentExecutionStates[widget->componentIndex];
		if (inputWidgetExecutionState) {
			return inputWidgetExecutionState->unit;
		}
	}

	auto unitValue = get(widgetCursor, widget->unit);
	if (unitValue.isString()) {
		return getUnitFromName(unitValue.getString());
	} else {
		return UNIT_NONE;
	}
}

Value getInputWidgetData(const gui::WidgetCursor &widgetCursor, const Value &dataValue) {
	auto widget = (const InputWidget*)widgetCursor.widget;

	if (widget->flags & INPUT_WIDGET_TYPE_NUMBER) {
		auto unit = getInputWidgetUnit(widgetCursor);
		float factor = getUnitFactor(unit);
		auto baseUnit = getBaseUnit(unit);
    	return Value(dataValue.toFloat() * factor, baseUnit);
	} else {
        return Value::makeStringRef(dataValue.getString(), -1, 0xa3d4f1d4);
	}
}

bool InputWidgetState::updateState() {
    WIDGET_STATE_START(InputWidget);

	const Style *style = getStyle(overrideStyle(widgetCursor, widget->style));

	WIDGET_STATE(flags.active, g_isActiveWidget);
	WIDGET_STATE(flags.focused, isFocusWidget(widgetCursor));
	WIDGET_STATE(flags.blinking, g_isBlinkTime && styleIsBlink(style));
	WIDGET_STATE(data, get(widgetCursor, widget->data));

    WIDGET_STATE_END()
}

void InputWidgetState::render() {
    const WidgetCursor &widgetCursor = g_widgetCursor;

	auto widget = (const InputWidget*)widgetCursor.widget;

	if (widgetCursor.flowState) {
		auto inputWidgetExecutionState = (InputWidgetExecutionState *)widgetCursor.flowState->componenentExecutionStates[widget->componentIndex];
		if (!inputWidgetExecutionState) {
			inputWidgetExecutionState = eez::flow::allocateComponentExecutionState<InputWidgetExecutionState>(widgetCursor.flowState, widget->componentIndex);
		}

		getInputWidgetParams(
			widgetCursor,
			inputWidgetExecutionState->min,
			inputWidgetExecutionState->max,
			inputWidgetExecutionState->precision,
			inputWidgetExecutionState->unit
		);
	}

	const Style *style = getStyle(overrideStyle(widgetCursor, widget->style));

	uint16_t overrideColor                 = flags.focused ? style->focusColor           : g_hooks.overrideStyleColor(widgetCursor, style);
	uint16_t overrideBackgroundColor       = flags.focused ? style->focusBackgroundColor : style->backgroundColor;
	uint16_t overrideActiveColor           = flags.focused ? style->focusBackgroundColor : g_hooks.overrideActiveStyleColor(widgetCursor, style);
	uint16_t overrideActiveBackgroundColor = flags.focused ? style->focusColor           : style->activeBackgroundColor;

	char text[MAX_TEXT_LEN + 1];
	data.toText(text, sizeof(text));

	if ((widget->flags & INPUT_WIDGET_TYPE_TEXT) && (widget->flags & INPUT_WIDGET_PASSWORD_FLAG)) {
		for (int i = 0; text[i]; i++) {
			text[i] = '*';
		}
	}

	drawText(
		text, -1,
		widgetCursor.x, widgetCursor.y, widgetCursor.w, widgetCursor.h,
		style,
		flags.active, flags.blinking, false,
		&overrideColor, &overrideBackgroundColor, &overrideActiveColor, &overrideActiveBackgroundColor,
		true // useSmallerFontIfDoesNotFit
	);
}

} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
