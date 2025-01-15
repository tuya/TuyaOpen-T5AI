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

#if defined(EEZ_PLATFORM_SIMULATOR) || defined(__EMSCRIPTEN__)

#if EEZ_OPTION_GUI

#include <math.h>

#if !defined(__EMSCRIPTEN__)
#include <SDL.h>
#endif

#include <eez/core/encoder.h>
#include <eez/core/keyboard.h>
#include <eez/core/os.h>

#include <eez/gui/gui.h>

#include <eez/platform/simulator/events.h>

namespace eez {

namespace platform {
namespace simulator {

int g_mouseX;
int g_mouseY;
bool g_mouseButton1IsPressed;

void readEvents() {
#if !defined(__EMSCRIPTEN__)
    int yMouseWheel = 0;
    bool mouseButton2IsUp = false;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
            if (event.button.button == 1) {
                if (event.type == SDL_MOUSEBUTTONDOWN) {
                    g_mouseButton1IsPressed = true;
                } else if (event.type == SDL_MOUSEBUTTONUP) {
                    g_mouseButton1IsPressed = false;
                }
            }
            if (event.button.button == 2) {
                if (event.type == SDL_MOUSEBUTTONUP) {
                    mouseButton2IsUp = true;
                }
            }
        } else if (event.type == SDL_MOUSEWHEEL) {
            yMouseWheel += event.wheel.y;
        } else if (event.type == SDL_KEYDOWN) {
            keyboard::onKeyboardEvent(&event.key);
        } else if (event.type == SDL_WINDOWEVENT)  {
            if (event.window.event == SDL_WINDOWEVENT_SHOWN || event.window.event == SDL_WINDOWEVENT_RESTORED) {
                eez::gui::refreshScreen();
            }
        }

        if (event.type == SDL_QUIT) {
            eez::shutdown();
        }
    }

    SDL_GetMouseState(&g_mouseX, &g_mouseY);

    // for web simulator
    if (yMouseWheel >= 100 || yMouseWheel <= -100) {
        yMouseWheel /= 100;
    }

#if OPTION_ENCODER
    mcu::encoder::write(yMouseWheel, mouseButton2IsUp);
#endif
#endif
}

bool isMiddleButtonPressed() {
#if !defined(__EMSCRIPTEN__)
    int x;
    int y;
	osDelay(1000);
    auto buttons = SDL_GetMouseState(&x, &y);
    return buttons & SDL_BUTTON(2) ? true : false;
#else
    return false;
#endif
}

} // namespace simulator
} // namespace platform
} // namespace eez

#if defined(__EMSCRIPTEN__)

EM_PORT_API(void) onPointerEvent(int x, int y, int pressed) {
    using namespace eez::platform::simulator;

    g_mouseX = x;
    g_mouseY = y;
    g_mouseButton1IsPressed = pressed;
}

EM_PORT_API(void) onMouseWheelEvent(double yMouseWheel, int clicked) {
#if OPTION_ENCODER
    if (yMouseWheel >= 100 || yMouseWheel <= -100) {
        yMouseWheel /= 100;
    }
    eez::mcu::encoder::write(round(yMouseWheel), clicked);
#endif
}

#endif

#endif

#endif // defined(EEZ_PLATFORM_SIMULATOR)
