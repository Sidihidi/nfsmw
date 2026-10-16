#ifndef FRONTEND_DEBUGMENUWIDGETS_H
#define FRONTEND_DEBUGMENUWIDGETS_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/bWare/Inc/bList.hpp"

class IMenuRender;
class DebugMenu;

// total size: 0x2C
class MenuWidget : public bTNode<MenuWidget> {
  public:
    enum actions {
        ACTION_LEFT = 0,
        ACTION_RIGHT = 1,
        ACTION_SELECT = 2,
        ACTION_CANCEL = 3,
    };

    // La A124 no tiene simbolo para ninguno de los dos: constantes que el codigo pliega. Solo la suma
    // valuesXOffset + valuesWidth (350, en initializeSize) y valuesXOffset (300, en los render) estan probadas.
    static const int valuesXOffset = 300;
    static const int valuesWidth = 50;

    int getHeight() {
        return height;
    }
    int getWidth() {
        return width;
    }
    void setHeight(int h) {
        height = h;
    }
    void setWidth(int w) {
        width = w;
    }
    virtual void render(IMenuRender *renderer, int x, int y);
    virtual void onGainFocus();
    virtual void onLoseFocus();
    virtual void onAction(actions action, bool repeat);
    bool hasFocus() {
        return bHasFocus;
    }
    bool canRepeat() {
        return bCanRepeat;
    }
    void enableRepeats() {
        bCanRepeat = true;
    }
    void disableRepeats() {
        bCanRepeat = false;
    }
    void initializeSize(IMenuRender *renderer);
    MenuWidget(const char *n, int h, int w, void (*f)());
    virtual ~MenuWidget();
    const char *getName() {
        return name;
    }
    void SetExitMenuAfterSelected(bool exit) {
        bExitMenuAfterSelected = exit;
    }

  protected:
    unsigned int getTextColor() {
        return textColor;
    }
    void setTextColor(unsigned int c) {
        textColor = c;
    }

  private:
    int height;                  // offset 0x8, size 0x4
    int width;                   // offset 0xC, size 0x4
    const char *name;            // offset 0x10, size 0x4
    bool bHasFocus;              // offset 0x14, size 0x4
    bool bCanRepeat;             // offset 0x18, size 0x4
    bool bExitMenuAfterSelected; // offset 0x1C, size 0x4
    unsigned int textColor;      // offset 0x20, size 0x4
    void (*callbackFunction)();  // offset 0x24, size 0x4
};

// total size: 0x8
struct StringListParam {
    char *string; // offset 0x0, size 0x4
    void (*f)();  // offset 0x4, size 0x4
};

// total size: 0x38
class StringListWidget : public MenuWidget {
  public:
    virtual void onAction(actions action, bool repeat);
    virtual void render(IMenuRender *renderer, int x, int y);
    StringListWidget(char *name, StringListParam *values, int n, int initial_value);
    virtual ~StringListWidget();

  private:
    StringListParam *strings; // offset 0x2C, size 0x4
    int currentValue;         // offset 0x30, size 0x4
    int maxValue;             // offset 0x34, size 0x4
};

// total size: 0x3C
class FloatWidget : public MenuWidget {
  public:
    virtual void onAction(actions action, bool repeat);
    virtual void render(IMenuRender *renderer, int x, int y);
    FloatWidget(char *name, float *var, float rmin, float rmax, float inc, void (*f)());
    virtual ~FloatWidget();

  private:
    float rangeMin;  // offset 0x2C, size 0x4
    float rangeMax;  // offset 0x30, size 0x4
    float increment; // offset 0x34, size 0x4
    float *theVar;   // offset 0x38, size 0x4
};

// total size: 0x3C
class IntWidget : public MenuWidget {
  public:
    virtual void onAction(actions action, bool repeat);
    virtual void render(IMenuRender *renderer, int x, int y);
    IntWidget(char *name, int *var, int rmin, int rmax, int inc, void (*f)());
    virtual ~IntWidget();

  private:
    int rangeMin;  // offset 0x2C, size 0x4
    int rangeMax;  // offset 0x30, size 0x4
    int increment; // offset 0x34, size 0x4
    int *theVar;   // offset 0x38, size 0x4
};

// total size: 0x30
class BoolWidget : public MenuWidget {
  public:
    virtual void onAction(actions action, bool repeat);
    virtual void render(IMenuRender *renderer, int x, int y);
    BoolWidget(const char *name, bool *var, void (*f)());
    virtual ~BoolWidget();

  private:
    bool *theVar; // offset 0x2C, size 0x4
};

// total size: 0x2C
class ActionWidget : public MenuWidget {
  public:
    ActionWidget(char *name, void (*f)());
};

// total size: 0x30
class SubmenuWidget : public MenuWidget {
  public:
    virtual void onGainFocus();
    virtual void onAction(actions action, bool repeat);
    SubmenuWidget(char *name, DebugMenu *var);
    virtual ~SubmenuWidget();

  private:
    DebugMenu *theMenu; // offset 0x2C, size 0x4
};

#endif
