#include "Speed/Indep/Src/Frontend/DebugMenuWidgets.h"
#include "Speed/Indep/Src/Frontend/DebugMenu.h"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"

MenuWidget::MenuWidget(const char *n, int h, int w, void (*f)())
    : height(h), width(w), name(n), bCanRepeat(true), bExitMenuAfterSelected(false), textColor(0xffffffff), callbackFunction(f) {}

MenuWidget::~MenuWidget() {}

void MenuWidget::onGainFocus() {
    bHasFocus = true;
    textColor = 0xffffff00;
}

void MenuWidget::onLoseFocus() {
    bHasFocus = false;
    textColor = 0xffffffff;
}

void MenuWidget::initializeSize(IMenuRender *renderer) {
    int x;
    int y;

    renderer->getStringSize(getName(), &x, &y);
    setHeight(y);
    setWidth(valuesXOffset + valuesWidth);
}

void MenuWidget::render(IMenuRender *renderer, int x, int y) {
    renderer->print(x, y, getTextColor(), getName());
}

void MenuWidget::onAction(actions action, bool repeat) {
    if (repeat && !canRepeat()) {
        return;
    }
    if (action == ACTION_SELECT) {
        if (callbackFunction) {
            callbackFunction();
        }
        if (bExitMenuAfterSelected) {
            ExitDebugMenu();
        }
    }
}

StringListWidget::StringListWidget(char *name, StringListParam *values, int n, int initial_value)
    : MenuWidget(name, 20, 0, 0), strings(values), currentValue(initial_value), maxValue(n) {}

StringListWidget::~StringListWidget() {}

void StringListWidget::onAction(actions action, bool repeat) {
    MenuWidget::onAction(action, false);
    if (repeat && !canRepeat()) {
        return;
    }
    switch (action) {
        case ACTION_LEFT:
            if (currentValue > 0) {
                currentValue--;
            }
            break;
        case ACTION_RIGHT:
            if (currentValue < maxValue - 1) {
                currentValue++;
            }
            break;
    }
    strings[currentValue].f();
}

void StringListWidget::render(IMenuRender *renderer, int x, int y) {
    MenuWidget::render(renderer, x, y);
    renderer->print(x + valuesXOffset, y, getTextColor(), strings[currentValue].string);
}

FloatWidget::FloatWidget(char *name, float *var, float rmin, float rmax, float inc, void (*f)())
    : MenuWidget(name, 20, 0, f), rangeMin(rmin), rangeMax(rmax), increment(inc), theVar(var) {}

FloatWidget::~FloatWidget() {}

void FloatWidget::onAction(actions action, bool repeat) {
    MenuWidget::onAction(action, false);
    if (repeat && !canRepeat()) {
        return;
    }
    switch (action) {
        case ACTION_LEFT:
            if (*theVar - increment >= rangeMin) {
                *theVar -= increment;
            }
            break;
        case ACTION_RIGHT:
            if (*theVar + increment <= rangeMax) {
                *theVar += increment;
            }
            break;
    }
}

void FloatWidget::render(IMenuRender *renderer, int x, int y) {
    MenuWidget::render(renderer, x, y);
    char buf[128];

    bSPrintf(buf, "%f", *theVar);
    renderer->print(x + valuesXOffset, y, getTextColor(), buf);
}

BoolWidget::BoolWidget(const char *name, bool *var, void (*f)()) : MenuWidget(name, 20, 0, f), theVar(var) {}

BoolWidget::~BoolWidget() {}

void BoolWidget::onAction(actions action, bool repeat) {
    MenuWidget::onAction(action, false);
    if (repeat && !canRepeat()) {
        return;
    }
    switch (action) {
        case ACTION_LEFT:
        case ACTION_RIGHT:
            *theVar = !*theVar;
            break;
    }
}

void BoolWidget::render(IMenuRender *renderer, int x, int y) {
    MenuWidget::render(renderer, x, y);
    if (*theVar) {
        renderer->print(x + valuesXOffset, y, getTextColor(), "yes");
    } else {
        renderer->print(x + valuesXOffset, y, getTextColor(), "no");
    }
}

ActionWidget::ActionWidget(char *name, void (*f)()) : MenuWidget(name, 20, 0, f) {}

SubmenuWidget::SubmenuWidget(char *name, DebugMenu *var) : MenuWidget(name, 20, 0, 0), theMenu(var) {}

SubmenuWidget::~SubmenuWidget() {}

void SubmenuWidget::onGainFocus() {
    MenuWidget::onGainFocus();
    setTextColor(0xffffff00);
}

void SubmenuWidget::onAction(actions action, bool repeat) {
    MenuWidget::onAction(action, false);
    if (repeat && !canRepeat()) {
        return;
    }
    if (action == ACTION_SELECT) {
        DebugMenu::changeMenu(theMenu);
    }
}

IntWidget::IntWidget(char *name, int *var, int rmin, int rmax, int inc, void (*f)())
    : MenuWidget(name, 20, 0, f), rangeMin(rmin), rangeMax(rmax), increment(inc), theVar(var) {}

IntWidget::~IntWidget() {}

void IntWidget::onAction(actions action, bool repeat) {
    MenuWidget::onAction(action, false);
    if (repeat && !canRepeat()) {
        return;
    }
    switch (action) {
        case ACTION_LEFT:
            if (*theVar - increment >= rangeMin) {
                *theVar -= increment;
            }
            break;
        case ACTION_RIGHT:
            if (*theVar + increment <= rangeMax) {
                *theVar += increment;
            }
            break;
    }
}

void IntWidget::render(IMenuRender *renderer, int x, int y) {
    MenuWidget::render(renderer, x, y);
    char buf[128];

    bSPrintf(buf, "%d", *theVar);
    renderer->print(x + valuesXOffset, y, getTextColor(), buf);
}
