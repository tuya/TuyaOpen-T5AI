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

#define CONF_GUI_PAGE_NAVIGATION_STACK_SIZE 10

#define APP_CONTEXT_ID_DEVICE 0
#define APP_CONTEXT_ID_SIMULATOR_FRONT_PANEL 1

namespace eez {
namespace gui {

class Page;

struct PageOnStack {
    int pageId = PAGE_ID_NONE;
    Page *page = nullptr;
    int displayBufferIndex = -1;
    float timelinePosition;
};

class ToastMessagePage;

class AppContext {
    friend struct AppViewWidgetState;

public:
	Rect rect;

    AppContext();

    virtual void stateManagment();

    void showPage(int pageId);
    void pushPage(int pageId, Page *page = nullptr);
    void popPage();
    void removePageFromStack(int pageId);

    int getActivePageStackPointer() {
        return m_updatePageIndex != -1 ? m_updatePageIndex : m_pageNavigationStackPointer;
    }

    int getActivePageId() {
        return m_pageNavigationStack[getActivePageStackPointer()].pageId;
    }

    Page *getActivePage() {
        return m_pageNavigationStack[getActivePageStackPointer()].page;
    }

    bool isActivePageInternal();

    int getPreviousPageId() {
        int index = getActivePageStackPointer();
        return index == 0 ? PAGE_ID_NONE : m_pageNavigationStack[index - 1].pageId;
    }

    void replacePage(int pageId, Page *page = nullptr);

    Page *getPage(int pageId);
    bool isPageOnStack(int pageId);
    bool isExternalPageOnStack();
	void removeExternalPagesFromTheStack();

    int getNumPagesOnStack() {
        return m_pageNavigationStackPointer + 1;
    }

    virtual bool isFocusWidget(const WidgetCursor &widgetCursor);

    virtual bool isBlinking(const WidgetCursor &widgetCursor, int16_t id);

    virtual void onPageTouch(const WidgetCursor &foundWidget, Event &touchEvent);

    virtual bool testExecuteActionOnTouchDown(int action);

    virtual bool isAutoRepeatAction(int action);

    virtual bool isWidgetActionEnabled(const WidgetCursor &widgetCursor);

    virtual int getLongTouchActionHook(const WidgetCursor &widgetCursor);

    void infoMessage(const char *message);
    void infoMessage(Value value);
    void infoMessage(const char *message, void (*action)(), const char *actionLabel);
    void errorMessage(const char *message, bool autoDismiss = false);
    void errorMessage(Value value);
    void errorMessageWithAction(Value value, void (*action)(int param), const char *actionLabel, int actionParam);
    void errorMessageWithAction(const char *message, void (*action)(), const char *actionLabel);

    void yesNoDialog(int yesNoPageId, const char *message, void (*yes_callback)(), void (*no_callback)(), void (*cancel_callback)());
    void yesNoDialog(int yesNoPageId, Value value, void(*yes_callback)(), void(*no_callback)(), void(*cancel_callback)());
    void questionDialog(Value message, Value buttons, void *userParam, void (*callback)(void *userParam, unsigned buttonIndex));

	// TODO these should be private
	void(*m_dialogYesCallback)();
	void(*m_dialogNoCallback)();
	void(*m_dialogCancelCallback)();
	void(*m_dialogLaterCallback)();

    virtual int getMainPageId() = 0;

    void getBoundingRect(Rect &rect);

protected:
    PageOnStack m_pageNavigationStack[CONF_GUI_PAGE_NAVIGATION_STACK_SIZE];
    int m_pageNavigationStackPointer = 0;
    int m_updatePageIndex;

    uint32_t m_showPageTime;

    virtual void onPageChanged(int previousPageId, int activePageId, bool activePageIsFromStack, bool previousPageIsStillOnStack);

    void doShowPage(int index, Page *page, int previousPageId, bool activePageIsFromStack, bool previousPageIsStillOnStack);
    void setPage(int pageId);

    void updatePage(int i, WidgetCursor &widgetCursor);
    virtual void pageRenderCustom(int i, WidgetCursor &widgetCursor);

    void getPageRect(int pageId, const Page *page, int &x, int &y, int &w, int &h);
    bool isPageFullyCovered(int pageNavigationStackIndex);

    virtual bool canExecuteActionWhenTouchedOutsideOfActivePage(int pageId, int action);

    void pushToastMessage(ToastMessagePage *toastMessage);
};

AppContext *getRootAppContext();

} // namespace gui
} // namespace eez
