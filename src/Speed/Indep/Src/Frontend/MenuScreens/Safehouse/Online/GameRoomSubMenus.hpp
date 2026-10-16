#ifndef FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_GAMEROOMSUBMENUS_H
#define FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_GAMEROOMSUBMENUS_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Frontend/Database/VehicleDB.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/Slider.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feScrollerina.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiQRCarSelect.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiQRTrackSelect.hpp"
#include "Speed/Indep/Src/Gameplay/GRace.h"
#include "Speed/Indep/bWare/Inc/bList.hpp"

// total size: 0x158
class UIOLGameRoomHost : public MenuScreen {
  public:
    // total size: 0x1C
    class OptionScrollerDatum : public ScrollerDatum {
      public:
        uint32 msgHash; // offset 0x18, size 0x4

        OptionScrollerDatum() : msgHash(0) {}
        ~OptionScrollerDatum() override {}
    };

  private:
    Scrollerina optionScroller; // offset 0x2C, size 0xC8
    FEScrollBar m_scollbar;     // offset 0xF4, size 0x64

  public:
    UIOLGameRoomHost(ScreenConstructorData *sd);
    ~UIOLGameRoomHost() override;

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;

  private:
    void Setup();
};

// total size: 0x1E4
class OL_GameRoomCarSelect : public MenuScreen {
  private:
    bTList<SelectableCar> CarsList;    // offset 0x2C, size 0x8
    SelectableCar *pCurrentNode;       // offset 0x34, size 0x4
    uint32 pCurrentCarHandle;          // offset 0x38, size 0x4
    uint32 m_currentCarHandle;         // offset 0x3C, size 0x4
    FEPlayerCarDB *m_stable;           // offset 0x40, size 0x4
    Scrollerina m_optionList;          // offset 0x44, size 0xC8
    uint32 m_filter;                   // offset 0x10C, size 0x4
    int m_index;                       // offset 0x110, size 0x4
    int m_total;                       // offset 0x114, size 0x4
    TwoStageSlider AccelerationSlider; // offset 0x118, size 0x44
    TwoStageSlider TopSpeedSlider;     // offset 0x15C, size 0x44
    TwoStageSlider HandlingSlider;     // offset 0x1A0, size 0x44

  public:
    OL_GameRoomCarSelect(ScreenConstructorData *sd);
    ~OL_GameRoomCarSelect() override;

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;

  private:
    void BuildCarList();
    void ClearCarList();
    void Setup();
    void UpdateCarChoiceDisplay();
    void UpdateCarListName();
    void EditOption(int direction);
    void BuildRideAndUpdate();
    void InitStatsSliders();
    void UpdateSliders();
    void FilterNext();
    void FilterPrev();
    bool LookupCurrentList(int);
};

// total size: 0x18
class SelectableMode : public bTNode<SelectableMode> {
  public:
    GRace::Type m_type;   // offset 0x8, size 0x4
    uint32 m_textHash;    // offset 0xC, size 0x4
    uint32 m_textureHash; // offset 0x10, size 0x4

    SelectableMode(GRace::Type modetype) : m_type(modetype) {}
    virtual ~SelectableMode() {}
};

enum eGameOption {
    eGameOption_MODE = 0,
    eGameOption_TRACK = 1,
    eGameOption_MINPLAYERS = 2,
    eGameOption_RANKED = 3,
    eGameOption_LENGTH = 4,
    eGameOption_LAPS = 5,
    eGameOption_DIR = 6,
    eGameOption_NOS = 7,
    eGameOption_PERF = 8,
    eGameOption_COLL = 9,
};

// total size: 0x12C
class EditGameDetails : public MenuScreen {
  private:
    Scrollerina m_optionList;                 // offset 0x2C, size 0xC8
    int mTempMinPlayers;                      // offset 0xF4, size 0x4
    bTList<SelectableMode> m_modelist;        // offset 0xF8, size 0x8
    bTList<SelectableTrack> m_CircuitTracks;  // offset 0x100, size 0x8
    bTList<SelectableTrack> m_SprintTracks;   // offset 0x108, size 0x8
    bTList<SelectableTrack> m_DragTracks;     // offset 0x110, size 0x8
    SelectableMode *m_currentModePtr;         // offset 0x118, size 0x4
    SelectableTrack *m_currentCircuitTrack;   // offset 0x11C, size 0x4
    SelectableTrack *m_currentSprintTrack;    // offset 0x120, size 0x4
    SelectableTrack *m_currentDragTrack;      // offset 0x124, size 0x4
    SelectableTrack *m_currentSelectableTrack; // offset 0x128, size 0x4

  public:
    EditGameDetails(ScreenConstructorData *sd);
    ~EditGameDetails() override;

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;

  private:
    void Setup();
    void SetGameMode();
    void UpdateEditPosition();
    void ChangeSelectedItem(int direction);
    void FixUpOptionsForModeSwitch();
    void UpdateTrackDisplay();
};

#endif
