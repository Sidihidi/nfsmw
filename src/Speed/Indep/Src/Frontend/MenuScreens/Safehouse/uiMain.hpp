#ifndef _UIMAIN
#define _UIMAIN

#include <types.h>

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEIconScrollerMenu.hpp"

// total size: 0x170
struct UIMain : public IconScrollerMenu {
    bool m_bStatsShowing; // offset 0x16C, size 0x1

    UIMain(ScreenConstructorData* sd);
    ~UIMain() override {}

    void NotificationMessage(u32 msg, FEObject* obj, u32 param1, u32 param2) override;
  private:
    void Setup() override;
    void UpdateProfileData();
};

// Definido en uiMain.cpp; lo usan uiOptionsMain.cpp, uiQRModeSelect.cpp (hoy con extern local) y UIEATerms.
extern const char *gOnlineMainMenu;

#endif
