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
namespace flow {

struct LineChartLine {
    AssetsPtr<uint8_t> label;
    uint16_t color;
    uint16_t reserved;
    float width;
    AssetsPtr<uint8_t> value;
};

struct LineChartWidgetComponenent : public Component {
    uint32_t maxPoints;
    AssetsPtr<uint8_t> xValue;
    ListOfAssetsPtr<LineChartLine> lines;
};

#if EEZ_OPTION_GUI

struct Point {
    float x;
    float lines[1];
};

struct LineChartWidgetComponenentExecutionState : public ComponenentExecutionState {
    LineChartWidgetComponenentExecutionState();
    ~LineChartWidgetComponenentExecutionState();

    void init(uint32_t numLines, uint32_t maxPoints);

    uint32_t numLines;
    uint32_t maxPoints;
    uint32_t numPoints;
    uint32_t startPointIndex;

    Value *lineLabels;

    bool updated;

    bool onInputValue(FlowState *flowState, unsigned componentIndex);

    Value getX(int pointIndex);
    void setX(int pointIndex, Value& value);

    float getY(int pointIndex, int lineIndex);
    void setY(int pointIndex, int lineIndex, float value);

private:
    // Data structure where n is no. of points, m is no. of lines, Xi is Value and Yij is float:
    // X1
    // X2
    // Xn
    // Y11 Y12 ... Y1m
    // Y21 Y22 ... Y2m
    // ...
    // Yn1 Yn2 ... Ynm,
    void *data;
};

#endif // EEZ_OPTION_GUI || !defined(EEZ_OPTION_GUI)

} // flow
} // eez
