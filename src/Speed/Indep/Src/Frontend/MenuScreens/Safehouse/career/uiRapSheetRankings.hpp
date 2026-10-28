#ifndef __UI_RAP_SHEET_RANKINGS_HPP__
#define __UI_RAP_SHEET_RANKINGS_HPP__

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/Database/RaceDB.hpp"

class uiRapSheetRankings : public MenuScreen {
  public:
    uiRapSheetRankings(ScreenConstructorData *sd);
    ~uiRapSheetRankings() override {};
    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;
    void RefreshHeader();

    static bool career_view;

  private:
    void Setup();
    void PrintRanking(uint32 fe_rank, uint32 button_hash, ePursuitDetailTypes type);

    uint32 button_pressed; // offset 0x2C, size 0x4
    uint32 init_button;    // offset 0x30, size 0x4
#ifdef EA_PLATFORM_WIN32
    uint32 highlighted_button; // offset 0x34, size 0x4 (speed.exe: the mouse; Create allocates 0x38)
#endif
};

#endif
