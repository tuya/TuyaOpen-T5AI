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

#include <eez/gui/gui.h>

namespace eez {
namespace gui {

////////////////////////////////////////////////////////////////////////////////

enum KeypadMode {
    KEYPAD_MODE_LOWERCASE,
    KEYPAD_MODE_UPPERCASE,
    KEYPAD_MODE_SYMBOL
};

class Keypad : public Page {
  public:
    AppContext *getAppContext() { return m_appContext;  }

	void start(AppContext *appContext, const char *label, const char *text, int minChars_, int maxChars_, bool isPassword_, void(*ok)(char *), void(*cancel)(), void(*setDefault)());

    void pageAlloc();
    void pageFree();

    void key();
    virtual void key(char ch);
    virtual void space();
    virtual void back();
    virtual void clear();
    virtual void sign();
    virtual void unit();
    virtual bool isOkEnabled();
    virtual void ok();
    virtual void cancel();
    virtual bool canSetDefault();
    virtual void setDefault();

    virtual void getKeypadText(char *text, size_t count);
    Value getKeypadTextValue();

    void insertChar(char c);

    virtual Unit getSwitchToUnit() {
        return UNIT_UNKNOWN;
    }

    KeypadMode m_keypadMode;

    virtual int getCursorPosition();
    virtual void setCursorPosition(int cursorPosition);

    int getXScroll(const WidgetCursor &widgetCursor);

    const char *getKeypadLabel();

protected:
    AppContext *m_appContext;

    char m_label[MAX_KEYPAD_LABEL_LENGTH + 1];
    char m_keypadText[MAX_KEYPAD_TEXT_LENGTH + 2];
    int m_cursorPosition;
    int m_xScroll;
    int m_minChars;
    int m_maxChars;

    void init(AppContext *appContext, const char *label);

private:
    bool m_isPassword;
    uint32_t m_lastKeyAppendTimeMs;

    void (*m_okCallback)(char *);
    void (*m_cancelCallback)();
    void (*m_setDefaultCallback)();
};

////////////////////////////////////////////////////////////////////////////////

enum {
    NUMERIC_KEYPAD_OPTION_ACTION_NONE,
    NUMERIC_KEYPAD_OPTION_ACTION_MAX,
    NUMERIC_KEYPAD_OPTION_ACTION_MIN,
    NUMERIC_KEYPAD_OPTION_ACTION_DEF,
    NUMERIC_KEYPAD_OPTION_USER = NUMERIC_KEYPAD_OPTION_ACTION_DEF + 1
};

struct NumericKeypadOptions {
    NumericKeypadOptions();

	int pageId;

    int slotIndex;
	int subchannelIndex;

    Unit editValueUnit;

    bool allowZero;
    float min;
    float max;
    float def;

    struct {
        unsigned checkWhileTyping : 1;
        unsigned signButtonEnabled : 1;
        unsigned dotButtonEnabled : 1;
        unsigned unitChangeEnabled: 1;
    } flags;

    int option1Action;
    const char *option1ButtonText;

    int option2Action;
    const char *option2ButtonText;

    int option3Action;
    const char *option3ButtonText;

    float encoderPrecision;

	void enableMaxButton();
	void enableMinButton();
	void enableDefButton();
};

enum NumericKeypadState {
    START,
    EMPTY,
    D0,
    D1,
    DOT,
    D2,
    D3,

    BEFORE_DOT,
    AFTER_DOT
};

class NumericKeypad : public Keypad {
public:
    void init(
        AppContext *appContext,
        const char *label,
        const Value &value,
        NumericKeypadOptions &options,
        void (*okFloat)(float),
        void (*okUint32)(uint32_t),
        void (*cancel)()
    );

    bool isEditing();

    Unit getEditUnit();

    void getKeypadText(char *text, size_t count) override;
    virtual bool getText(char *text, size_t count);

    void key(char ch) override;
    void space() override;
    void caps();
    void back() override;
    void clear() override;
    void sign() override;
    void unit() override;
    bool isOkEnabled() override;
    void ok() override;
    void cancel() override;

    void setMaxValue();
    void setMinValue();
    void setDefValue();

	virtual bool isMicroAmperAllowed();
	virtual bool isAmperAllowed();

    Unit getSwitchToUnit() override;

	virtual void showLessThanMinErrorMessage();
	virtual void showGreaterThanMaxErrorMessage();

    virtual int getCursorPosition() override;
    virtual void setCursorPosition(int cursorPosition) override;

    virtual float getPrecision();

    NumericKeypadOptions m_options;

protected:
    Value m_startValue;
    NumericKeypadState m_state;
    void (*m_okFloatCallback)(float);
    void (*m_okUint32Callback)(uint32_t);
    void (*m_cancelCallback)();

    void appendEditUnit(char *text, size_t maxTextLength);
    double getValue();
    char getDotSign();

    void toggleEditUnit();
    bool checkNumSignificantDecimalDigits();
    void digit(int d);
    void dot();
    void reset();
};

// optionActionIndex: possible values are 1, 2 and 3 (for three possible optional actions)
void executeNumericKeypadOptionHook(int optionActionIndex);

////////////////////////////////////////////////////////////////////////////////

Keypad *getActiveKeypad();
NumericKeypad *getActiveNumericKeypad();

void onKeypadTextTouch(const WidgetCursor &widgetCursor, Event &touchEvent);

} // namespace gui
} // namespace eez
