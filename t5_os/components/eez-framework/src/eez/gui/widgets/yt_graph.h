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

enum {
	YT_GRAPH_UPDATE_METHOD_SCROLL,
	YT_GRAPH_UPDATE_METHOD_SCAN_LINE,
	YT_GRAPH_UPDATE_METHOD_STATIC
};

struct YTGraphWidget : public Widget {
};


struct YTGraphWidgetState : public WidgetState {
	WidgetStateFlags flags;
	Value data;
    uint32_t refreshCounter;
    uint8_t iChannel;
    uint8_t ytGraphUpdateMethod;
    uint32_t numHistoryValues;
    uint32_t historyValuePosition;
    uint32_t cursorPosition;
    uint8_t *bookmarks;
    bool showLabels;
    int8_t selectedValueIndex;
    bool valueIsVisible[MAX_NUM_OF_Y_VALUES];
    float valueDiv[MAX_NUM_OF_Y_VALUES];
    float valueOffset[MAX_NUM_OF_Y_VALUES];

	uint32_t previousHistoryValuePosition;
	bool refreshBackground;

    bool updateState() override;
    void render() override;

	bool hasOnTouch() override;
	void onTouch(const WidgetCursor &widgetCursor, Event &touchEvent) override;
};

} // gui
} // eez
