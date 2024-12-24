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

struct RollerWidget : public Widget {
	int16_t min;
	int16_t max;
    int16_t text;
    int16_t selectedValueStyle;
    int16_t unselectedValueStyle;
    uint16_t componentIndex;
};

struct RollerWidgetState : public WidgetState {
    Value data;

	int minValue = 1;
	int maxValue = 100;

    int textHeight;

    bool isDragging = false;
    bool isRunning = false;

    float position = 0;
    float velocity = 0;

    int dragOrigin = 0;
    int dragStartPosition = 0;
    int dragPosition = 0;

    bool updateState() override;
    void render() override;

    bool hasOnTouch() override;
	void onTouch(const WidgetCursor &widgetCursor, Event &touchEvent) override;

private:
    void updatePosition();
    bool isMoving();
    void applyDragForce();
    void applySnapForce();
    void applyEdgeForce();
    void applyForce(float force);
};


} // namespace gui
} // namespace eez
