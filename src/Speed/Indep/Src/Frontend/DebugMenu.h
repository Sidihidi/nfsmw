#ifndef FRONTEND_DEBUGMENU_H
#define FRONTEND_DEBUGMENU_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/bWare/Inc/bList.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Src/AI/aireflectedtypes.h"

class MenuWidget;
class DebugMenu;

enum PursuitState {
    STATE_IDLE = 0,
    STATE_SEEN = 1,
    STATE_PURSUIT = 2,
    STATE_SEARCHING = 3,
};

// total size: 0x4
class IMenuRender {
  public:
    virtual void renderOverlay(int, int, int, int, float, unsigned int) = 0;
    virtual void print(int, int, unsigned int, const char *) = 0;
    virtual void getStringSize(const char *, int *, int *) = 0;
};

// total size: 0xC
struct DebugMenuAlloc : public bTNode<DebugMenuAlloc> {
    DebugMenu *menu; // offset 0x8, size 0x4
};

// total size: 0x34
class DebugMenu : public bNode {
  public:
    static DebugMenu *create(char *name, DebugMenu *parent);
    static void destroy();
    static void StartNewList() {
        mAllocations = new ("DebugMenuList", 0) bTList<DebugMenuAlloc>;
    }
    static void renderBackground();
    static void render();
    static IMenuRender *getRenderer() {
        return mRenderer;
    }
    char *GetName() {
        return mName;
    }
    int GetCurrentSelection() {
        return mCurrentSelection;
    }
    void SetWidgetToRenderFrom(int);
    void SetCurrentSelection(int);
    // Nombre del parametro no probado: ninguna expansion de la A124 lo deja en un registro.
    static void setRenderer(IMenuRender *renderer) {
        mRenderer = renderer;
    }
    DebugMenu *GetParent();
    static void changeMenu(DebugMenu *menu) {
        mCurrentMenu = menu;
    }
    static void SetCurrentMenu();
    static void addWidgetToNewestMenu(MenuWidget *widget);
    static void endNewestMenu();
    static void activate() {
        mActive = true;
    }
    static void deactivate() {
        mActive = false;
    }
    static bool isActive() {
        return mActive;
    }
    static void eventLeft(bool repeat);
    static void eventRight(bool repeat);
    static void eventUp();
    static void eventDown();
    static void eventSelect();
    static void eventCancel();
    // Nombre del parametro no probado: todas las llamadas de la A124 pasan una constante.
    static void SetPursuitState(PursuitState s) {
        mCurPursuitState = s;
    }
    static PursuitState GetPursuitState();
    static void SetPursuitLevel(int l) {
        mCurPursuitLevel = l;
    }
    static void SetPursuitFormation(FormationType f) {
        mCurPursuitForm = f;
    }
    static void PursuitPrintf();
    static void SoakTimer();
    void addWidget(MenuWidget *widget);
    MenuWidget *GetWidgetWithFocus();
    void RemoveAllFocus();

    static bool mDoPursuitPrintf;
    static bool mDoSoakTimer;
    static unsigned int mSoakTicker;

  private:
    DebugMenu();
    ~DebugMenu();
    void actionLeft(bool repeat);
    void actionRight(bool repeat);
    void actionUp();
    void actionDown();
    void actionSelect();
    void actionCancel();
    void doRender(IMenuRender *renderer);
    void doRenderBackground(IMenuRender *renderer);
    bool selectionFitsOnScreen();
    MenuWidget *getWidget(int n);

    DebugMenu *mParent;        // offset 0x8, size 0x4
    int mOriginX;              // offset 0xC, size 0x4
    int mOriginY;              // offset 0x10, size 0x4
    int mWidth;                // offset 0x14, size 0x4
    int mHeight;               // offset 0x18, size 0x4
    uint32 mBackgroundColor;   // offset 0x1C, size 0x4
    int mCurrentSelection;     // offset 0x20, size 0x4
    int mWidgetToRenderFrom;   // offset 0x24, size 0x4
    char *mName;               // offset 0x28, size 0x4
    bTList<MenuWidget> mItems; // offset 0x2C, size 0x8

    static bool mActive;
    static IMenuRender *mRenderer;
    static DebugMenu *mRootMenu;
    static DebugMenu *mCurrentMenu;
    static DebugMenu *mNewestMenu;
    static bTList<DebugMenuAlloc> *mAllocations;
    static PursuitState mCurPursuitState;
    static int mCurPursuitLevel;
    static FormationType mCurPursuitForm;
};

// DebugMenu.cpp; DebugMenuLoad (NFSDebugMenu.cpp) los usa.
extern char *gLastCurMenuName;
extern bool GameSessionIDPrintf;

// NFSDebugMenu.cpp
void ExitDebugMenu();

// ScreenPrintf.cpp (zFe2): el arbol aun no tiene su cabecera; nombres de los parametros del .mdebug.
void ScreenPrintf(int x, int y, const char *fmt, ...);
void ScreenPrintf(int x, int y, float duration, const char *fmt, ...);
void ScreenPrintf(int x, int y, unsigned int color, const char *fmt, ...);
void ScreenPrintf(int x, int y, float duration, unsigned int color, const char *fmt, ...);
void ScreenShadowPrintf(int x, int y, const char *fmt, ...);
void CancelScreenPrintf(int x, int y);
void ResetScreenPrintf();

struct FEString;

// total size: 0xC (stabs de zFe de la A124); GetStringSize (NFSDebugMenu.cpp) pide uno libre.
struct ScreenPrintItem {
    FEString *pFEString; // offset 0x0, size 0x4
    short PosX;          // offset 0x4, size 0x2
    short PosY;          // offset 0x6, size 0x2
    float Lifetime;      // offset 0x8, size 0x4
};

ScreenPrintItem *GetUnusedScreenPrintItem();

#endif
