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

////////////////////////////////////////////////////////////////////////////////

enum LVGL_ACTIONS {
    CHANGE_SCREEN,
    PLAY_ANIMATION,
    SET_PROPERTY
};

struct LVGLComponent_ActionType {
    uint32_t action;
};

////////////////////////////////////////////////////////////////////////////////

struct LVGLComponent_ChangeScreen_ActionType : public LVGLComponent_ActionType {
    int32_t screen;
    uint32_t fadeMode;
    uint32_t speed;
    uint32_t delay;
};

////////////////////////////////////////////////////////////////////////////////

#define ANIMATION_PROPERTY_POSITION_X 0
#define ANIMATION_PROPERTY_POSITION_Y 1
#define ANIMATION_PROPERTY_WIDTH 2
#define ANIMATION_PROPERTY_HEIGHT 3
#define ANIMATION_PROPERTY_OPACITY 4
#define ANIMATION_PROPERTY_IMAGE_ANGLE 5
#define ANIMATION_PROPERTY_IMAGE_ZOOM

#define ANIMATION_ITEM_FLAG_RELATIVE (1 << 0)
#define ANIMATION_ITEM_FLAG_INSTANT (1 << 1)

#define ANIMATION_PATH_LINEAR 0
#define ANIMATION_PATH_EASE_IN 1
#define ANIMATION_PATH_EASE_OUT 2
#define ANIMATION_PATH_EASE_IN_OUT 3
#define ANIMATION_PATH_OVERSHOOT 4
#define ANIMATION_PATH_BOUNCE 5

struct LVGLComponent_PlayAnimation_ActionType : public LVGLComponent_ActionType {
    int32_t target;
    uint32_t property;
    int32_t start;
    int32_t end;
    uint32_t delay;
    uint32_t time;
    uint32_t flags;
    uint32_t path;
};

////////////////////////////////////////////////////////////////////////////////

struct LVGLComponent_SetProperty_ActionType : public LVGLComponent_ActionType {
    int32_t target;
    uint32_t property;
    AssetsPtr<uint8_t> value;
    int32_t textarea;
    uint32_t animated;
};

struct LVGLComponent : public Component {
    ListOfAssetsPtr<LVGLComponent_ActionType> actions;
};

////////////////////////////////////////////////////////////////////////////////

} // flow
} // eez
