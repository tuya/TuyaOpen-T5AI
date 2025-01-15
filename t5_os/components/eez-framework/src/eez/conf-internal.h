/*
 * eez-framework
 *
 * MIT License
 * Copyright 2024 Envox d.o.o.
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef __cplusplus
    #ifdef __has_include
        #if __has_include("eez-framework-conf.h")
            #include "eez-framework-conf.h"
        #else
            #define OPTION_KEYBOARD 0
            #define OPTION_MOUSE 0
            #define OPTION_KEYPAD 0
            #define CUSTOM_VALUE_TYPES
        #endif
    #endif
#endif

#if ARDUINO
    #define EEZ_FOR_LVGL 1
    #ifndef LV_LVGL_H_INCLUDE_SIMPLE
        #define LV_LVGL_H_INCLUDE_SIMPLE
    #endif
    #include <Arduino.h>
#endif

#if EEZ_FOR_LVGL
    #define EEZ_OPTION_GUI 0
#endif

#ifndef EEZ_OPTION_GUI
    #define EEZ_OPTION_GUI 1
#endif

#ifndef OPTION_KEYBOARD
#define OPTION_KEYBOARD 0
#endif

#ifndef OPTION_MOUSE
#define OPTION_MOUSE 0
#endif

#ifndef OPTION_KEYPAD
#define OPTION_KEYPAD 0
#endif

#ifndef CUSTOM_VALUE_TYPES
#define CUSTOM_VALUE_TYPES
#endif

#ifdef __cplusplus
    #if EEZ_OPTION_GUI
        #ifdef __has_include
            #if __has_include("eez-framework-gui-conf.h")
                #include <eez-framework-gui-conf.h>
            #endif
        #endif
    #endif
#endif

#ifndef EEZ_OPTION_GUI_ANIMATIONS
    #define EEZ_OPTION_GUI_ANIMATIONS 1
#endif
