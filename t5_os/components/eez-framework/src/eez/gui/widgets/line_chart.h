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

namespace eez {
namespace gui {

#define Y_AXIS_RANGE_OPTION_FIXED 0
#define Y_AXIS_RANGE_OPTION_FLOATING 1

struct LineChartWidget : public Widget {
    int16_t title;

    int16_t showTitle;
    int16_t showLegend;
    int16_t showXAxis;
    int16_t showYAxis;
    int16_t showGrid;

    int16_t yAxisRangeOption;
    int16_t yAxisRangeFrom;
    int16_t yAxisRangeTo;

    int16_t marginLeft;
    int16_t marginTop;
    int16_t marginRight;
    int16_t marginBottom;

    int16_t marker;

    int16_t titleStyle;
    int16_t legendStyle;
	int16_t xAxisStyle;
    int16_t yAxisStyle;
    int16_t markerStyle;

    uint16_t componentIndex;
};

struct LineChartWidgetState : public WidgetState {
    WidgetStateFlags flags;
    Value title;
    Value showTitleValue;
    Value showLegendValue;
    Value showXAxisValue;
    Value showYAxisValue;
    Value showGridValue;
    Value yAxisRangeFrom;
    Value yAxisRangeTo;
    Value markerValue;

    bool updateState() override;
	void render() override;
};

} // namespace gui
} // namespace eez
