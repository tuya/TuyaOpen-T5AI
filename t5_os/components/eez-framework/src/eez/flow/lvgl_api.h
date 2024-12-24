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

#include <math.h>

#if defined(EEZ_FOR_LVGL)

#include <stdint.h>

#if defined(EEZ_FOR_LVGL)
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

typedef void (*ActionExecFunc)(lv_event_t * e);
void eez_flow_init(const uint8_t *assets, uint32_t assetsSize, lv_obj_t **objects, size_t numObjects, const ext_img_desc_t *images, size_t numImages, ActionExecFunc *actions);

void eez_flow_tick();

bool eez_flow_is_stopped();

extern int16_t g_currentScreen;

void loadScreen(int index);

void flowOnPageLoaded(unsigned pageIndex);

// if flowState is nullptr then userWidgetComponentIndexOrPageIndex is page index
void *getFlowState(void *flowState, unsigned userWidgetComponentIndexOrPageIndex);

void flowPropagateValue(void *flowState, unsigned componentIndex, unsigned outputIndex);

const char *evalTextProperty(void *flowState, unsigned componentIndex, unsigned propertyIndex, const char *errorMessage);
int32_t evalIntegerProperty(void *flowState, unsigned componentIndex, unsigned propertyIndex, const char *errorMessage);
bool evalBooleanProperty(void *flowState, unsigned componentIndex, unsigned propertyIndex, const char *errorMessage);
const char *evalStringArrayPropertyAndJoin(void *flowState, unsigned componentIndex, unsigned propertyIndex, const char *errorMessage, const char *separator);

void assignStringProperty(void *flowState, unsigned componentIndex, unsigned propertyIndex, const char *value, const char *errorMessage);
void assignIntegerProperty(void *flowState, unsigned componentIndex, unsigned propertyIndex, int32_t value, const char *errorMessage);
void assignBooleanProperty(void *flowState, unsigned componentIndex, unsigned propertyIndex, bool value, const char *errorMessage);

float eez_linear(float x);
float eez_easeInQuad(float x);
float eez_easeOutQuad(float x);
float eez_easeInOutQuad(float x);
float eez_easeInCubic(float x);
float eez_easeOutCubic(float x);
float eez_easeInOutCubic(float x);
float eez_easeInQuart(float x);
float eez_easeOutQuart(float x);
float eez_easeInOutQuart(float x);
float eez_easeInQuint(float x);
float eez_easeOutQuint(float x);
float eez_easeInOutQuint(float x);
float eez_easeInSine(float x);
float eez_easeOutSine(float x);
float eez_easeInOutSine(float x);
float eez_easeInExpo(float x);
float eez_easeOutExpo(float x);
float eez_easeInOutExpo(float x);
float eez_easeInCirc(float x);
float eez_easeOutCirc(float x);
float eez_easeInOutCirc(float x);
float eez_easeInBack(float x);
float eez_easeOutBack(float x);
float eez_easeInOutBack(float x);
float eez_easeInElastic(float x);
float eez_easeOutElastic(float x);
float eez_easeInOutElastic(float x);
float eez_easeOutBounce(float x);
float eez_easeInBounce(float x);
float eez_easeOutBounce(float x);
float eez_easeInOutBounce(float x);

float getTimelinePosition(void *flowState);

extern int g_eezFlowLvlgMeterTickIndex;

#ifdef __cplusplus
}
#endif

#endif // defined(EEZ_FOR_LVGL)
