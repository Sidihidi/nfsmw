#ifndef FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_UIONLINEMAINMENU_H
#define FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_UIONLINEMAINMENU_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEIconScrollerMenu.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"

// total size: 0x174
class UIOnlineMainMenu : public IconScrollerMenu {
  public:
    UIOnlineMainMenu(ScreenConstructorData *sd);
    ~UIOnlineMainMenu() override {}

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

  private:
    void Setup() override;

    bool m_exitingOnline;         // offset 0x16C, size 0x4
    dialog_handle m_dialogHandle; // offset 0x170, size 0x4
};

// Nace en UIOnlineMainMenu.cpp (GSYM entre MMOnlineSelectCar y MMOnlineRankingSubMenu); StatsMainMenu y las
// pantallas de clasificaciones lo leen y lo escriben.
extern bool EnterRankingsMenu;

#endif
