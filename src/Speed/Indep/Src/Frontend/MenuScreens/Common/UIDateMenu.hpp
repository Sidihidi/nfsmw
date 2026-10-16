#ifndef FRONTEND_MENUSCREENS_COMMON_UIDATEMENU_H
#define FRONTEND_MENUSCREENS_COMMON_UIDATEMENU_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feWidget.hpp"

class FEString;

// total size: 0x70
class UIDateMenu : public MenuScreen {
  public:
    ~UIDateMenu() override {
        pCurrentDate = nullptr;
    }

    static UIDateMenu *GetInstance() {
        return pCurrentDate;
    }

    int GetDay() {
        return Day;
    }

    int GetMonth() {
        return Month;
    }

    int GetYear() {
        return Year;
    }

    UIDateMenu(ScreenConstructorData *sd);

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;

    // Declaradas en el original (stabs de la A124) y sin cuerpo en ninguna version.
    void GenerateDateString(char *);
    void GenerateDateString(FEString *);

  protected:
    void UpdateDate(int ArrowIndex);

  private:
    void ScrollHorizontal(eScrollDir dir);
    void ScrollVertical(eScrollDir dir);

  protected:
    static UIDateMenu *pCurrentDate; // size: 0x4

    FEString *pWidgets[3];    // offset 0x2C, size 0xC
    FEString *pDay;           // offset 0x38, size 0x4
    FEString *pMonth;         // offset 0x3C, size 0x4
    FEString *pYear;          // offset 0x40, size 0x4
    FEDateWidget *pWidget;    // offset 0x44, size 0x4
    int Index;                // offset 0x48, size 0x4
    int Day;                  // offset 0x4C, size 0x4
    int Month;                // offset 0x50, size 0x4
    unsigned short Year;      // offset 0x54, size 0x2
    unsigned short m_MaxYear; // offset 0x56, size 0x2
    FEObject *pBlinkies[6];   // offset 0x58, size 0x18
};

#endif
