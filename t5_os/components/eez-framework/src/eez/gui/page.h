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

#include <eez/gui/widgets/button.h>
#include <eez/gui/widgets/rectangle.h>
#include <eez/gui/widgets/text.h>

namespace eez {
namespace gui {

extern Value g_alertMessage;

////////////////////////////////////////////////////////////////////////////////

class Page {
public:
    virtual ~Page() {
    }

    virtual void pageAlloc();
    virtual void pageFree();

    virtual void pageWillAppear();

    virtual void onEncoder(int counter);
    virtual void onEncoderClicked();
    virtual Unit getEncoderUnit();

    virtual int getDirty();
    virtual void set();
    virtual void discard();

    virtual bool showAreYouSureOnDiscard();
};

////////////////////////////////////////////////////////////////////////////////

class SetPage : public Page {
public:
    virtual void edit();

protected:
    int16_t editDataId;

    static void onSetValue(float value);
    virtual void setValue(float value);
};

////////////////////////////////////////////////////////////////////////////////

class InternalPage : public Page {
public:
    virtual void updateInternalPage() = 0;
	virtual WidgetCursor findWidgetInternalPage(int x, int y, bool clicked) = 0;
    virtual bool canClickPassThrough();
    virtual bool closeIfTouchedOutside();

    int x;
    int y;
    int width;
    int height;

protected:
    Widget widget;
};

////////////////////////////////////////////////////////////////////////////////

enum ToastType {
    INFO_TOAST,
    ERROR_TOAST
};

class ToastMessagePage : public InternalPage {
    friend class AppContext;

    static ToastMessagePage *findFreePage();

public:
    static ToastMessagePage *create(AppContext *appContext, ToastType type, const char *message, bool autoDismiss = false);
    static ToastMessagePage *create(AppContext *appContext, ToastType type, Value message);
    static ToastMessagePage *create(AppContext *appContext, ToastType type, Value message, void (*action)(int param), const char *actionLabel, int actionParam);
    static ToastMessagePage *create(AppContext *appContext, ToastType type, const char *message, void (*action)(), const char *actionLabel);

    void init(AppContext *appContext, ToastType type, const Value& message);

    void pageFree();

    void onEncoder(int counter);
    void onEncoderClicked();

    void updateInternalPage();
    WidgetCursor findWidgetInternalPage(int x, int y, bool clicked);
    bool canClickPassThrough();
    bool closeIfTouchedOutside();

    bool hasAction() {
      return actionWidget.action != 0;
    }

    static void executeAction();
    static void executeActionWithoutParam();

	AppContext *appContext;
    Value messageValue;

private:
    ToastType type;

    static const size_t MAX_MESSAGE_LENGTH = 200;
    char messageBuffer[MAX_MESSAGE_LENGTH];

    const char *actionLabel;
    void (*action)(int param);
    int actionParam;
    void (*actionWithoutParam)();

    RectangleWidget containerRectangleWidget;
    TextWidget line1Widget;
    TextWidget line2Widget;
    TextWidget line3Widget;
    ButtonWidget actionWidget;

    WidgetCursor lastActiveWidget;
};

////////////////////////////////////////////////////////////////////////////////

class SelectFromEnumPage : public InternalPage {
public:
    static void pushSelectFromEnumPage(
        AppContext *appContext,
        const EnumItem *enumItems,
        uint16_t currentValue,
        bool (*disabledCallback)(uint16_t value),
        void (*onSet)(uint16_t),
        bool smallFont = false,
        bool showRadioButtonIcon = true
    );

    static void pushSelectFromEnumPage(
        AppContext *appContext,
        void (*enumDefinitionFunc)(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value),
        uint16_t currentValue,
        bool (*disabledCallback)(uint16_t value),
        void (*onSet)(uint16_t),
        bool smallFont = false,
        bool showRadioButtonIcon = true
    );

    static const EnumItem *getActiveSelectEnumDefinition();
    static void selectEnumItem();
    static void popSelectFromEnumPage();

    void init(
        AppContext *appContext_,
        const EnumItem *enumDefinition_,
        uint16_t currentValue_,
    	bool (*disabledCallback_)(uint16_t value),
        void (*onSet_)(uint16_t),
        bool smallFont_,
        bool showRadioButtonIcon_
    );

    void init(
        AppContext *appContext_,
        void (*enumDefinitionFunc)(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value),
        uint16_t currentValue_,
        bool (*disabledCallback_)(uint16_t value),
        void (*onSet_)(uint16_t),
        bool smallFont_,
        bool showRadioButtonIcon_
    );

    void init();

    void updateInternalPage();
    WidgetCursor findWidgetInternalPage(int x, int y, bool clicked);

    const EnumItem *getEnumDefinition() {
        return enumDefinition;
    }

    AppContext *appContext;

private:
    const EnumItem *enumDefinition;
    void (*enumDefinitionFunc)(DataOperationEnum operation, const WidgetCursor &widgetCursor, Value &value);

    int numItems;
    int numColumns;
    int itemWidth;
    int itemHeight;

    int activeItemIndex;

    bool dirty;

    uint16_t getValue(int i);
    bool getLabel(int i, char *text, int count);

    uint16_t currentValue;
    bool (*disabledCallback)(uint16_t value);
    void (*onSet)(uint16_t);

    bool smallFont;
    bool showRadioButtonIcon;

    bool calcSize();
    void findPagePosition();

    bool isDisabled(int i);

    void getItemPosition(int itemIndex, int &x, int &y);
    void getItemLabel(int itemIndex, char *text, int count);

	void doSelectEnumItem();
};

////////////////////////////////////////////////////////////////////////////////

enum MenuType {
    MENU_TYPE_BUTTON
};

static const size_t MAX_MENU_ITEMS = 4;

void showMenu(AppContext *appContext, const char *message, MenuType menuType, const char **menuItems, void(*callback)(int));

class MenuWithButtonsPage : public InternalPage {
public:
    static MenuWithButtonsPage *create(AppContext *appContext, const char *message, const char **menuItems, void (*callback)(int));

    void updateInternalPage();
    WidgetCursor findWidgetInternalPage(int x, int y, bool clicked);

    static void executeAction();

private:
    AppContext *m_appContext;
    RectangleWidget m_containerRectangleWidget;
    TextWidget m_messageTextWidget;
    TextWidget m_buttonTextWidgets[MAX_MENU_ITEMS];
    size_t m_numButtonTextWidgets;
    void (*m_callback)(int);

    void init(AppContext *appContext, const char *message, const char **menuItems, void(*callback)(int));
};

////////////////////////////////////////////////////////////////////////////////

class QuestionPage : public InternalPage {
public:
    static QuestionPage *create(AppContext *appContext, const Value &message, const Value &buttons, void *userParam, void (*callback)(void *userParam, unsigned buttonIndex));

    void updateInternalPage();
    WidgetCursor findWidgetInternalPage(int x, int y, bool clicked);

    static void executeAction();

private:
    AppContext *m_appContext;
    Value m_message;
    Value m_buttons;
    void *m_userParam;
    void (*m_callback)(void *userParam, unsigned buttonIndex);

    RectangleWidget m_containerRectangleWidget;
    TextWidget m_messageTextWidget;
    TextWidget m_buttonTextWidgets[MAX_MENU_ITEMS];
    size_t m_numButtonTextWidgets;

    void init(AppContext *appContext, const Value &message, const Value &buttons, void *userParam, void (*callback)(void *userParam, unsigned buttonIndex));
};

} // namespace gui
} // namespace eez
