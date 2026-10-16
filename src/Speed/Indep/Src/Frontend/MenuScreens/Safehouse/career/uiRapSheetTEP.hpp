#ifndef __UI_RAP_SHEET_TEP_HPP__
#define __UI_RAP_SHEET_TEP_HPP__

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feUIWidgetMenu.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"

class uiRapSheetTEP : public UIWidgetMenu {
  public:
    uiRapSheetTEP(ScreenConstructorData *sd);
    ~uiRapSheetTEP() override {};
    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;

  private:
    void Setup() override;

    uint32 button_pressed; // offset 0x138, size 0x4
#ifdef EA_PLATFORM_WIN32
    // PC: el ctor original pone a 0 un u32 en +0x128 (r23: fndiff ctor
    // 124/12c/128 frente a 124/128; GC/PS2 no lo tienen, ctor GC al 100%).
    uint32 m_unknown128;
#endif
    int num_pursuits;      // offset 0x13C, size 0x4
};

#endif
