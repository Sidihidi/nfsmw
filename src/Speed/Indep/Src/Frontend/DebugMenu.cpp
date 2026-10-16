#include "Speed/Indep/Src/Frontend/DebugMenu.h"
#include "Speed/Indep/Src/Frontend/DebugMenuWidgets.h"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Online/LobbyGameSessions.hpp"
#include "Speed/Indep/bWare/Inc/bDebug.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"

float DebugMenuTransparencyLevel = 1.0f;
bool DebugMenu::mActive = false;
IMenuRender *DebugMenu::mRenderer = 0;
DebugMenu *DebugMenu::mRootMenu = 0;
DebugMenu *DebugMenu::mCurrentMenu = 0;
DebugMenu *DebugMenu::mNewestMenu = 0;
bTList<DebugMenuAlloc> *DebugMenu::mAllocations = 0;
bool DebugMenu::mDoPursuitPrintf = false;
PursuitState DebugMenu::mCurPursuitState = STATE_IDLE;
FormationType DebugMenu::mCurPursuitForm = (FormationType)0;
int DebugMenu::mCurPursuitLevel = 0;
bool DebugMenu::mDoSoakTimer = false;
unsigned int DebugMenu::mSoakTicker = 0;
char *gLastCurMenuName = 0;
int gLastCurSelection = 0;

DebugMenu *DebugMenu::create(char *name, DebugMenu *parent) {
    DebugMenu *newMenu = new ("DebugMenu", 0) DebugMenu();
    // La A124 no deja stab de esta local (copia del resultado del new); nombre tomado de destroy.
    DebugMenuAlloc *tAlloc = new ("DebugMenuAlloc", 0) DebugMenuAlloc();

    newMenu->mParent = parent;
    newMenu->mName = name;
    newMenu->mBackgroundColor = 0;
    newMenu->mOriginX = 100;
    newMenu->mOriginY = 65;
    newMenu->mWidth = 440;
    newMenu->mHeight = 200;
    newMenu->mCurrentSelection = -1;
    newMenu->mWidgetToRenderFrom = -1;
    mNewestMenu = newMenu;
    tAlloc->menu = newMenu;
    mAllocations->AddTail(tAlloc);

    if (parent) {
        parent->addWidget(new ("SubmenuWidget", 0) SubmenuWidget(name, newMenu));
    } else if (!mRootMenu) {
        mRootMenu = newMenu;
        mCurrentMenu = newMenu;
    }
    return newMenu;
}

void DebugMenu::destroy() {
    DebugMenu **tempPtrs;
    int i;
    DebugMenuAlloc *tAlloc;

    gLastCurMenuName = mCurrentMenu->GetName();
    gLastCurSelection = mCurrentMenu->GetCurrentSelection();

    tempPtrs = new ("DebugMenuTemp", 0) DebugMenu *[mAllocations->CountElements()];
    i = 0;
    for (tAlloc = mAllocations->GetHead(); tAlloc != mAllocations->EndOfList(); tAlloc = tAlloc->GetNext()) {
        tempPtrs[i] = tAlloc->menu;
        i++;
    }

    for (i--; i != 0; i--) {
        delete tempPtrs[i];
    }
    delete tempPtrs[0];
    mRootMenu = 0;

    delete tempPtrs;

    mAllocations->DeleteAllElements();
    delete mAllocations;

    delete mRenderer;
    mRenderer = 0;
    mNewestMenu = 0;
    mCurrentMenu = 0;
}

DebugMenu::DebugMenu() {
    mParent = 0;
}

DebugMenu::~DebugMenu() {
    mItems.DeleteAllElements();
}

void DebugMenu::RemoveAllFocus() {
    MenuWidget *widget;

    for (widget = mItems.GetHead(); widget != mItems.EndOfList(); widget = widget->GetNext()) {
        if (widget->hasFocus()) {
            widget->onLoseFocus();
        }
    }
}

void DebugMenu::SetCurrentMenu() {
    for (bTList<DebugMenuAlloc>::iterator i = mAllocations->begin(); i != mAllocations->end(); i++) {
        DebugMenu *tMenu = (*i)->menu;

        if (!bStrCmp(tMenu->GetName(), gLastCurMenuName)) {
            mCurrentMenu = tMenu;
            MenuWidget *widget = tMenu->getWidget(gLastCurSelection);

            while (bStrCmp(mCurrentMenu->GetWidgetWithFocus()->getName(), widget->getName())) {
                eventDown();
            }
            return;
        }
    }
}

MenuWidget *DebugMenu::getWidget(int n) {
    if (n < mItems.CountElements()) {
        MenuWidget *widget = mItems.GetHead();

        for (int i = 0; i < n; i++) {
            widget = widget->GetNext();
        }
        return widget;
    }
    return 0;
}

void DebugMenu::actionUp() {
    if (mCurrentSelection > 0) {
        MenuWidget *widget = getWidget(mCurrentSelection);
        widget->onLoseFocus();
        mCurrentSelection--;
        widget = getWidget(mCurrentSelection);
        widget->onGainFocus();
        if (mCurrentSelection < mWidgetToRenderFrom) {
            mWidgetToRenderFrom = mCurrentSelection;
        }
    }
}

void DebugMenu::actionDown() {
    if (mCurrentSelection < mItems.CountElements() - 1) {
        MenuWidget *widget = getWidget(mCurrentSelection);
        widget->onLoseFocus();
        mCurrentSelection++;
        widget = getWidget(mCurrentSelection);
        widget->onGainFocus();
        while (!selectionFitsOnScreen()) {
            mWidgetToRenderFrom++;
        }
    }
}

void DebugMenu::actionLeft(bool repeat) {
    if (mCurrentSelection >= 0) {
        MenuWidget *widget = getWidget(mCurrentSelection);
        widget->onAction(MenuWidget::ACTION_LEFT, repeat);
    }
}

void DebugMenu::actionRight(bool repeat) {
    if (mCurrentSelection >= 0) {
        MenuWidget *widget = getWidget(mCurrentSelection);
        widget->onAction(MenuWidget::ACTION_RIGHT, repeat);
    }
}

void DebugMenu::actionSelect() {
    if (mCurrentSelection >= 0) {
        MenuWidget *widget = getWidget(mCurrentSelection);
        widget->onAction(MenuWidget::ACTION_SELECT, false);
    }
}

void DebugMenu::actionCancel() {
    if (mParent) {
        mCurrentMenu = mParent;
    }
}

void DebugMenu::eventUp() {
    if (mCurrentMenu) {
        mCurrentMenu->actionUp();
    }
}

void DebugMenu::eventDown() {
    if (mCurrentMenu) {
        mCurrentMenu->actionDown();
    }
}

void DebugMenu::eventLeft(bool repeat) {
    if (mCurrentMenu) {
        mCurrentMenu->actionLeft(repeat);
    }
}

void DebugMenu::eventRight(bool repeat) {
    if (mCurrentMenu) {
        mCurrentMenu->actionRight(repeat);
    }
}

void DebugMenu::eventSelect() {
    if (mCurrentMenu) {
        mCurrentMenu->actionSelect();
    }
}

void DebugMenu::eventCancel() {
    if (mCurrentMenu) {
        mCurrentMenu->actionCancel();
    }
}

void DebugMenu::addWidget(MenuWidget *widget) {
    widget->initializeSize(mRenderer);
    mItems.AddTail(widget);

    if (mCurrentSelection == -1) {
        mCurrentSelection = 0;
        mWidgetToRenderFrom = 0;
        widget->onGainFocus();
    }
}

void DebugMenu::addWidgetToNewestMenu(MenuWidget *widget) {
    if (mNewestMenu) {
        mNewestMenu->addWidget(widget);
    }
}

void DebugMenu::endNewestMenu() {
    if (mNewestMenu) {
        DebugMenu *parent = mNewestMenu->mParent;

        if (parent) {
            mNewestMenu = parent;
        }
    }
}

void OnlineScreenPrintf() {}

bool LatencyScreenPrintf = false;
GameSession *mySession = 0;
bool GameSessionIDPrintf = false;

void SessionIDPrintf() {
    mySession = LobbyGameSessions::Instance().GetMySession();
    if (mySession) {
        ScreenPrintf(-300, -120, "Session ID: %s", mySession->strName);
    } else {
        ScreenPrintf(-300, -120, "Session ID: none");
    }
}

void DebugMenu::render() {
    if (mCurrentMenu && isActive()) {
        DoScreenPrintf = 0;
        mCurrentMenu->doRender(mRenderer);
    }
    if (mDoPursuitPrintf) {
        PursuitPrintf();
    }
    if (GameSessionIDPrintf) {
        SessionIDPrintf();
    }
    if (LatencyScreenPrintf) {
        OnlineScreenPrintf();
    }
    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;
        mSoakTicker = bGetTicker();
    }
    SoakTimer();
}

void DebugMenu::renderBackground() {
    if (mCurrentMenu && isActive()) {
        mCurrentMenu->doRenderBackground(mRenderer);
    }
}

void DebugMenu::doRenderBackground(IMenuRender *renderer) {
    renderer->renderOverlay(mOriginX - 5, mOriginY - 5, mWidth + 10, mHeight + 10, DebugMenuTransparencyLevel, 0xffff);
    renderer->renderOverlay(mOriginX, mOriginY, mWidth, mHeight, DebugMenuTransparencyLevel, mBackgroundColor);
}

void DebugMenu::doRender(IMenuRender *renderer) {
    int currentY;
    int i = 0;
    MenuWidget *widget;
    MenuWidget *titleWidget;
    int titleHeight = 0;
    char subMenuTitle[256];

    if (mParent) {
        titleWidget = mParent->GetWidgetWithFocus();
        if (titleWidget) {
            bSPrintf(subMenuTitle, "[%s]", titleWidget->getName());
            titleHeight = titleWidget->getHeight();
        }
    } else {
        int x;
        int y;

        bSNPrintf(subMenuTitle, 256, "[CL %s]", BuildVersionChangelistName);
        mRenderer->getStringSize(subMenuTitle, &x, &y);
        titleHeight = y;
    }
    mRenderer->print(mOriginX + 5, mOriginY, 0xff00ff00, subMenuTitle);

    currentY = titleHeight;
    for (widget = mItems.GetHead(); widget != mItems.EndOfList(); widget = widget->GetNext()) {
        if (currentY + widget->getHeight() > mHeight) {
            return;
        }
        if (i >= mWidgetToRenderFrom) {
            widget->render(mRenderer, mOriginX + 20, mOriginY + currentY);
            currentY += widget->getHeight();
        }
        i++;
    }
}

const char *GetFormationStr(FormationType f) {
    switch (f) {
        case PIT:
            return "PIT";
        case BOX_IN:
            return "BOX_IN";
        case ROLLING_BLOCK:
            return "ROLLING_BLOCK";
        case FOLLOW:
            return "FOLLOW";
        case STAGGER_FOLLOW:
            return "STAGGER_FOLLOW";
        case HELI_PURSUIT:
            return "HELI_PURSUIT";
        case HERD:
            return "HERD";
        default:
            return "NONE";
    }
}

void DebugMenu::PursuitPrintf() {
    switch (mCurPursuitState) {
        case STATE_IDLE:
            ScreenPrintf(-300, -115, "Pursuit: IDLE - %d", mCurPursuitLevel);
            break;
        case STATE_SEEN:
            ScreenPrintf(-300, -130, 2.0f, "SEEN");
            ScreenPrintf(-300, -115, "Pursuit: IDLE - %d", mCurPursuitLevel);
            break;
        case STATE_PURSUIT:
            ScreenPrintf(-300, -115, "Pursuit: PURSUIT - %d", mCurPursuitLevel);
            break;
        case STATE_SEARCHING:
            ScreenPrintf(-300, -115, "Pursuit: SEARCHING - %d", mCurPursuitLevel);
            break;
    }
    ScreenPrintf(-300, -100, "Formation: %s", GetFormationStr(mCurPursuitForm));
}

void DebugMenu::SoakTimer() {
    static int ten_seconds = 0;
    static int minutes = 0;
    static int hours = 0;
    float seconds = bGetTickerDifference(mSoakTicker) * 0.001f;

    if (seconds >= 10.0f) {
        if (ten_seconds == 5) {
            minutes++;
            if (minutes >= 60) {
                hours++;
                minutes = 0;
            }
            ten_seconds = 0;
            seconds = 0.0f;
            mSoakTicker = bGetTicker();
        } else {
            ten_seconds++;
            seconds = 0.0f;
            mSoakTicker = bGetTicker();
        }
    }
    if (mDoSoakTimer) {
        ScreenPrintf(0, -215, "Soak Timer");
        ScreenPrintf(0, -200, "%02d : %02d : %d%f", hours, minutes, ten_seconds, seconds);
    }
}

bool DebugMenu::selectionFitsOnScreen() {
    if (mCurrentSelection == -1 || mWidgetToRenderFrom == -1) {
        return true;
    }
    MenuWidget *widget;
    MenuWidget *titleWidget;
    int i = 0;
    int currentY = 0;

    if (mParent) {
        titleWidget = mParent->GetWidgetWithFocus();
        if (titleWidget) {
            currentY = titleWidget->getHeight();
        }
    } else {
        char menuTitle[128];
        int x;
        int y;

        bSNPrintf(menuTitle, 128, "[CL %s]", BuildVersionChangelistName);
        mRenderer->getStringSize(menuTitle, &x, &y);
        currentY = y;
    }
    for (widget = mItems.GetHead(); widget != mItems.EndOfList(); widget = widget->GetNext()) {
        if (currentY + widget->getHeight() > mHeight) {
            return false;
        }
        if (mCurrentSelection == i) {
            return true;
        }
        if (i >= mWidgetToRenderFrom) {
            currentY += widget->getHeight();
        }
        i++;
    }
    return false;
}

MenuWidget *DebugMenu::GetWidgetWithFocus() {
    MenuWidget *widget;

    for (widget = mItems.GetHead(); widget != mItems.EndOfList(); widget = widget->GetNext()) {
        if (widget->hasFocus()) {
            return widget;
        }
    }
    return 0;
}
