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

struct BarGraphWidget : public Widget {
    int16_t textStyle;
    int16_t line1Data;
    int16_t line1Style;
    int16_t line2Data;
    int16_t line2Style;
    int16_t min;
    int16_t max;
    int16_t refreshRate;
    uint8_t orientation; // BAR_GRAPH_ORIENTATION_...
};

struct BarGraphWidgetState : public WidgetState {
	WidgetStateFlags flags;
	Value data;
    uint16_t color;
    uint16_t backgroundColor;
    uint16_t activeColor;
    uint16_t activeBackgroundColor;
    Value line1Data;
    Value line2Data;
    Value textData;
    Value min;
    Value max;
    Value refreshRate;

    uint32_t textDataRefreshLastTime;

    bool updateState() override;
    void render() override;
};

} // namespace gui
} // namespace eez
