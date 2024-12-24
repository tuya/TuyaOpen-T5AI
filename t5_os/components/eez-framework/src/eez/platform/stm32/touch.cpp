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

#if defined(EEZ_PLATFORM_STM32) && !defined(EEZ_FOR_LVGL)

#include <string.h>
#include <stdint.h>

#include <i2c.h>

#include <eez/core/os.h>
#include <eez/core/debug.h>

#include <eez/gui/gui.h>
#include <eez/gui/touch.h>

#ifdef EEZ_PLATFORM_STM32F469I_DISCO
#include "stm32469i_discovery_ts.h"
#else
#define TSC2007IPW
//#define AR1021
#endif

#ifndef TOUCH_DEVICE_ADDRESS
#if defined(TSC2007IPW)
#define TOUCH_DEVICE_ADDRESS (0x48 << 1)
#endif

#if defined(AR1021)
#define TOUCH_DEVICE_ADDRESS (0x4D << 1)
#endif
#endif

#ifndef TOUCH_DEVICE_HANDLE
#define TOUCH_DEVICE_HANDLE hi2c1
#endif

namespace eez {
namespace mcu {
namespace touch {

static const int CONF_TOUCH_PRESSED_DEBOUNCE_TIMEOUT_MS = 10;
static const int CONF_TOUCH_NOT_PRESSED_DEBOUNCE_TIMEOUT_MS = 25;

static const int CONF_TOUCH_Z1_THRESHOLD = 50;

static const uint8_t  X_DATA_ID = 0b11000010;
static const uint8_t  Y_DATA_ID = 0b11010010;
static const uint8_t Z1_DATA_ID = 0b11100010;

enum State {
    STATE_NOT_PRESSED,
    STATE_PRESSED,
    STATE_DEBOUNCE_PRESSED,
    STATE_DEBOUNCE_NOT_PRESSED
};

enum Event {
    EVENT_PRESSED,
    EVENT_NOT_PRESSED,
    EVENT_DEBOUNCE_TIMEOUT
};

static State g_state;
static uint32_t g_debounceTimeout;

static int16_t g_lastXData = -1;
static int16_t g_lastYData = -1;
static int16_t g_lastZ1Data = 0;

void touchMeasure() {
#if defined(TSC2007IPW)
    static int g_errorCounter = 0;
    bool isError = false;

    vTaskSuspendAll();

    uint8_t result[4];

    if (HAL_I2C_Master_Transmit(&TOUCH_DEVICE_HANDLE, TOUCH_DEVICE_ADDRESS, (uint8_t *)&Z1_DATA_ID, 1, 5) != HAL_OK) {
        isError = true;
    }
    if (HAL_I2C_Master_Receive(&TOUCH_DEVICE_HANDLE, TOUCH_DEVICE_ADDRESS, result, 2, 5) != HAL_OK) {
        isError = true;
    }

    g_lastZ1Data = (((int16_t)result[0]) << 3) | ((int16_t)result[1]);

    if (g_lastZ1Data > CONF_TOUCH_Z1_THRESHOLD) {
        if (HAL_I2C_Master_Transmit(&TOUCH_DEVICE_HANDLE, TOUCH_DEVICE_ADDRESS, (uint8_t *)&X_DATA_ID, 1, 5) != HAL_OK) {
            isError = true;
        }
        if (HAL_I2C_Master_Receive(&TOUCH_DEVICE_HANDLE, TOUCH_DEVICE_ADDRESS, result, 2, 5) != HAL_OK) {
            isError = true;
        }

        if (HAL_I2C_Master_Transmit(&TOUCH_DEVICE_HANDLE, TOUCH_DEVICE_ADDRESS, (uint8_t *)&Y_DATA_ID, 1, 5) != HAL_OK) {
            isError = true;
        }
        if (HAL_I2C_Master_Receive(&TOUCH_DEVICE_HANDLE, TOUCH_DEVICE_ADDRESS, result + 2, 2, 5) != HAL_OK) {
            isError = true;
        }
    }

    xTaskResumeAll();

    if (isError) {
        if (g_errorCounter < 5) {
            g_errorCounter++;
            DebugTrace("Touch controller error detected\n");
        }
    }

    if (g_lastZ1Data > CONF_TOUCH_Z1_THRESHOLD) {
        g_lastXData  = (((int16_t)result[0]) << 3) | ((int16_t)result[1]);
        g_lastYData  = (((int16_t)result[2]) << 3) | ((int16_t)result[3]);
    }
#endif

#if defined(AR1021)
    vTaskSuspendAll();

    uint8_t result[5];
    memset(result, 0, 5);
    HAL_I2C_Master_Receive(&TOUCH_DEVICE_HANDLE, TOUCH_DEVICE_ADDRESS, result, 5, 5);

    xTaskResumeAll();

    g_lastZ1Data = result[0] & 1 ? CONF_TOUCH_Z1_THRESHOLD + 1 : 0;
    if (g_lastZ1Data > CONF_TOUCH_Z1_THRESHOLD) {
        g_lastXData = ((int16_t)result[1]) | (((int16_t)result[2]) << 7);
        g_lastYData = ((int16_t)result[3]) | (((int16_t)result[4]) << 7);
    }
#endif

#if defined(EEZ_PLATFORM_STM32F469I_DISCO)
    TS_StateTypeDef TS_State;
    if (BSP_TS_GetState(&TS_State) == TS_OK) {
    	if (TS_State.touchDetected) {
    		g_lastZ1Data = CONF_TOUCH_Z1_THRESHOLD + 1;
    		g_lastXData = TS_State.touchX[0];
    		g_lastYData = TS_State.touchY[0];
    	} else {
    		g_lastZ1Data = 0;
    	}
    }
#endif
}

static void setTimeout(uint32_t &timeout, uint32_t timeoutDuration) {
    timeout = millis() + timeoutDuration;
    if (timeout == 0) {
        timeout = 1;
    }
}

static void clearTimeout(uint32_t &timeout) {
    timeout = 0;
}

static void setState(State state) {
    if (state != g_state) {
        g_state = state;
    }
}

static void stateTransition(Event event) {
    if (g_state == STATE_NOT_PRESSED) {
        if (event == EVENT_PRESSED) {
            setState(STATE_DEBOUNCE_PRESSED);
            setTimeout(g_debounceTimeout, CONF_TOUCH_PRESSED_DEBOUNCE_TIMEOUT_MS);
        }
    } else if (g_state == STATE_PRESSED) {
        if (event == EVENT_NOT_PRESSED) {
            setState(STATE_DEBOUNCE_NOT_PRESSED);
            setTimeout(g_debounceTimeout, CONF_TOUCH_NOT_PRESSED_DEBOUNCE_TIMEOUT_MS);
        }
    } else if (g_state == STATE_DEBOUNCE_PRESSED) {
        if (event == EVENT_NOT_PRESSED) {
            setState(STATE_NOT_PRESSED);
            clearTimeout(g_debounceTimeout);
        } else if (event == EVENT_DEBOUNCE_TIMEOUT) {
            setState(STATE_PRESSED);
        }
    } else if (g_state == STATE_DEBOUNCE_NOT_PRESSED) {
        if (event == EVENT_PRESSED) {
            setState(STATE_PRESSED);
            clearTimeout(g_debounceTimeout);
        } else if (event == EVENT_DEBOUNCE_TIMEOUT) {
            setState(STATE_NOT_PRESSED);
        }
    }
}

static void testTimeoutEvent(uint32_t &timeout, Event timeoutEvent) {
    if (timeout && (int32_t)(millis() - timeout) >= 0) {
        clearTimeout(timeout);
        stateTransition(timeoutEvent);
    }
}

void read(bool &isPressed, int &x, int &y) {
    touchMeasure();

    stateTransition(g_lastZ1Data > CONF_TOUCH_Z1_THRESHOLD ? EVENT_PRESSED : EVENT_NOT_PRESSED);

    testTimeoutEvent(g_debounceTimeout, EVENT_DEBOUNCE_TIMEOUT);

    isPressed = g_state == STATE_PRESSED;
    x = g_lastXData;
    y = g_lastYData;
}

} // namespace touch
} // namespace mcu

namespace gui {

using namespace mcu::touch;

void data_touch_raw_x(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value) {
    if (operation == DATA_OPERATION_GET) {
        value = (int)g_lastXData;
    }
}

void data_touch_raw_y(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value) {
    if (operation == DATA_OPERATION_GET) {
        value = (int)g_lastYData;
    }
}

void data_touch_raw_z1(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value) {
    if (operation == DATA_OPERATION_GET) {
        value = (int)g_lastZ1Data;
    }
}

void data_touch_raw_pressed(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value) {
    if (operation == DATA_OPERATION_GET) {
        value = g_state == STATE_PRESSED;
    }
}

} // namespace gui


} // namespace eez

#endif // EEZ_PLATFORM_STM32
