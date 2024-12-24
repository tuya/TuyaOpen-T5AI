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

struct ScrollBarWidget : public Widget {
    int16_t thumbStyle;
    int16_t buttonsStyle;
    AssetsPtr<const char> leftButtonText;
	AssetsPtr<const char> rightButtonText;
};

enum ScrollBarWidgetSegment {
    SCROLL_BAR_WIDGET_SEGMENT_UNINITIALIZED,
    SCROLL_BAR_WIDGET_SEGMENT_NONE,
    SCROLL_BAR_WIDGET_SEGMENT_TRACK_LEFT,
    SCROLL_BAR_WIDGET_SEGMENT_TRACK_RIGHT,
    SCROLL_BAR_WIDGET_SEGMENT_THUMB,
    SCROLL_BAR_WIDGET_SEGMENT_LEFT_BUTTON,
    SCROLL_BAR_WIDGET_SEGMENT_RIGHT_BUTTON
};

struct ScrollBarWidgetState : public WidgetState {
	WidgetStateFlags flags;
    int size;
    int position;
    int pageSize;

    static WidgetCursor g_selectedWidget;

	bool updateState() override;
    void render() override;

    bool hasOnTouch() override;
	void onTouch(const WidgetCursor &widgetCursor, Event &touchEvent) override;
	bool hasOnKeyboard() override;
	bool onKeyboard(const WidgetCursor &widgetCursor, uint8_t key, uint8_t mod) override;
};

} // namespace gui
} // namespace eez
