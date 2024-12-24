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

struct UpDownWidget : public Widget {
    AssetsPtr<const char> downButtonText;
	AssetsPtr<const char> upButtonText;
    int16_t buttonsStyle;
    int16_t min;
    int16_t max;
};

enum UpDownWidgetSegment {
    UP_DOWN_WIDGET_SEGMENT_TEXT,
    UP_DOWN_WIDGET_SEGMENT_DOWN_BUTTON,
    UP_DOWN_WIDGET_SEGMENT_UP_BUTTON
};

struct UpDownWidgetState : public WidgetState {
	WidgetStateFlags flags;
	Value data;
	UpDownWidgetSegment segment;
    Value min;
    Value max;

	static WidgetCursor g_selectedWidget;

	bool updateState() override;
    void render() override;

	bool hasOnTouch() override;
	void onTouch(const WidgetCursor &widgetCursor, Event &touchEvent) override;
	bool hasOnKeyboard() override;
	bool onKeyboard(const WidgetCursor &widgetCursor, uint8_t key, uint8_t mod) override;

private:
	void upDown(const WidgetCursor &widgetCursor, UpDownWidgetSegment segment_);
};

} // namespace gui
} // namespace eez
