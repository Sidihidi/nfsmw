#ifndef FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_STATSMAINMENU_H
#define FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_STATSMAINMENU_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEIconScrollerMenu.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"

// total size: 0x170
class StatsMainMenu : public IconScrollerMenu {
  public:
    StatsMainMenu(ScreenConstructorData *sd);
    ~StatsMainMenu() override {}

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

  private:
    void Setup() override;

    dialog_handle m_dialogHandle; // offset 0x16C, size 0x4
};

#endif
