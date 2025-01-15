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

#if EEZ_OPTION_GUI

#include <eez/core/os.h>

#include <eez/gui/gui.h>
#include <eez/gui/thread.h>

#include <eez/flow/flow.h>
#include <eez/flow/hooks.h>

namespace eez {
namespace gui {

void mainLoop(void *);

#ifndef GUI_THREAD_STACK_SIZE
#define GUI_THREAD_STACK_SIZE 12 * 1024
#endif

EEZ_THREAD_DECLARE(gui, Normal, GUI_THREAD_STACK_SIZE);

EEZ_MESSAGE_QUEUE_DECLARE(gui, {
    uint8_t type;
    union {
        uint32_t param;

        struct {
            AppContext *appContext;
            int pageId;
            Page *page;
        } changePage;

        Event touchEvent;
    };
});

void startThread() {
	EEZ_MESSAGE_QUEUE_CREATE(gui, 20);
	EEZ_THREAD_CREATE(gui, mainLoop);

#ifdef __EMSCRIPTEN__
    eez::gui::guiInit();
#endif
}

void oneIter();

void mainLoop(void *) {
#ifdef __EMSCRIPTEN__
	oneIter();
#else

    guiInit();

    while (1) {

#ifdef EEZ_PLATFORM_SIMULATOR
    if (g_shutdown) {
        break;
    }
#endif

        oneIter();
    }
#endif
}

void processGuiQueue(uint32_t timeout) {
#ifdef __EMSCRIPTEN__
    while (true) {
#endif
    guiMessageQueueObject obj;
    if (!EEZ_MESSAGE_QUEUE_GET(gui, obj, timeout)) {
        return;
    }

    uint8_t type = obj.type;

    if (type == GUI_QUEUE_MESSAGE_TYPE_DISPLAY_VSYNC) {
        display::update();
    } else if (type == GUI_QUEUE_MESSAGE_TYPE_TOUCH_EVENT) {
        processTouchEvent(obj.touchEvent);
    } else if (type == GUI_QUEUE_MESSAGE_TYPE_SHOW_PAGE) {
        obj.changePage.appContext->showPage(obj.changePage.pageId);
    } else if (type == GUI_QUEUE_MESSAGE_TYPE_PUSH_PAGE) {
        obj.changePage.appContext->pushPage(obj.changePage.pageId, obj.changePage.page);
    } else if (type == GUI_QUEUE_MESSAGE_REFRESH_SCREEN) {
        refreshScreen();
    } else if (type == GUI_QUEUE_MESSAGE_UNLOAD_EXTERNAL_ASSETS) {
        unloadExternalAssets();
    } else if (type == GUI_QUEUE_MESSAGE_DEBUGGER_CLIENT_CONNECTED) {
        flow::onDebuggerClientConnected();
    } else if (type == GUI_QUEUE_MESSAGE_DEBUGGER_CLIENT_DISCONNECTED) {
        flow::onDebuggerClientDisconnected();
    } else if (type == GUI_QUEUE_MESSAGE_DEBUGGER_INPUT_AVAILABLE) {
        flow::onDebuggerInputAvailableHook();
    } else {
        g_hooks.onGuiQueueMessage(type, obj.param);
    }
#ifdef __EMSCRIPTEN__
    }
#endif
}

void oneIter() {
	processGuiQueue(100);
    guiTick();
}

void sendMessageToGuiThread(uint8_t messageType, uint32_t messageParam, uint32_t timeoutMillisec) {
    guiMessageQueueObject obj;
    obj.type = messageType;
    obj.param = messageParam;
	EEZ_MESSAGE_QUEUE_PUT(gui, obj, timeoutMillisec);
}

void sendTouchEventToGuiThread(Event &touchEvent) {
	touchEvent.time = millis();

    guiMessageQueueObject obj;
    obj.type = GUI_QUEUE_MESSAGE_TYPE_TOUCH_EVENT;
    obj.touchEvent = touchEvent;
	EEZ_MESSAGE_QUEUE_PUT(gui, obj, 0);
}

bool pushPageInGuiThread(AppContext *appContext, int pageId, Page *page) {
    if (!isGuiThread()) {
        guiMessageQueueObject obj;
        obj.type = GUI_QUEUE_MESSAGE_TYPE_PUSH_PAGE;
        obj.changePage.appContext = appContext;
        obj.changePage.pageId = pageId;
        obj.changePage.page = page;
        EEZ_MESSAGE_QUEUE_PUT(gui, obj, osWaitForever);
        return true;
    }
    return false;
}

bool showPageInGuiThread(AppContext *appContext, int pageId) {
    if (!isGuiThread()) {
        guiMessageQueueObject obj;
        obj.type = GUI_QUEUE_MESSAGE_TYPE_SHOW_PAGE;
        obj.changePage.appContext = appContext;
        obj.changePage.pageId = pageId;
        EEZ_MESSAGE_QUEUE_PUT(gui, obj, osWaitForever);
        return true;
    }
    return false;
}

bool isGuiThread() {
    return osThreadGetId() == g_guiTaskHandle;
}

void suspendGuiThread() {
#if !defined(EEZ_PLATFORM_SIMULATOR) && !defined(__EMSCRIPTEN__)
	vTaskSuspend((TaskHandle_t)g_guiTaskHandle);
#endif
}

void resumeGuiThread() {
#if !defined(EEZ_PLATFORM_SIMULATOR) && !defined(__EMSCRIPTEN__)
	vTaskResume((TaskHandle_t)g_guiTaskHandle);
#endif
}

} // namespace gui
} // namespace eez

#endif // EEZ_OPTION_GUI
