#ifndef FRONTEND_MENUSCREENS_SAFEHOUSE_CAREER_UIREPSHEETBOUNTY_H
#define FRONTEND_MENUSCREENS_SAFEHOUSE_CAREER_UIREPSHEETBOUNTY_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include <types.h>

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feArrayScrollerMenu.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiTrackMapStreamer.hpp"
#endif

struct UITrackMapStreamer;
struct FEMultiImage;

// total size: 0x178
struct uiRepSheetBounty : public ArrayScrollerMenu {
    bool bIsInGame;                    // offset 0xE8, size 0x1
    // La A124 lo lleva POR VALOR: sus stabs lo ponen en 0xE8 con 1728 bits y TrackMap en 0x1C0; el constructor
    // (0x23B9B8) llama a __18UITrackMapStreamer con this+0xE8 y el destructor (0x2820C0) a _$_18UITrackMapStreamer.
#ifdef EA_BUILD_A124
    UITrackMapStreamer TrackMapStreamer;
#else
    UITrackMapStreamer* TrackMapStreamer; // offset 0xEC, size 0x4
#endif
    FEMultiImage* TrackMap;            // offset 0xF0, size 0x4
    char orderedList[128];             // offset 0xF4, size 0x80
    // La A124 no lo tiene: acaba en orderedList (sizeof 0x244 = 0x1C4 + 0x80).
#ifndef EA_BUILD_A124
    bool tutorialPlaying;              // offset 0x174, size 0x1
#endif

    uiRepSheetBounty(ScreenConstructorData* sd);
    ~uiRepSheetBounty() override {
#ifndef EA_BUILD_A124
        delete TrackMapStreamer;
#endif
    }

    eMenuSoundTriggers NotifySoundMessage(u32 msg, eMenuSoundTriggers maybe) override;
    void NotificationMessage(u32 msg, FEObject* obj, u32 param1,
                             u32 param2) override;
  private:
    void Setup();
    void RefreshTrack();
    void RefreshHeader() override;
};

struct BountyDatum : public ArrayDatum {
    int index; // offset 0x24, size 0x4

    BountyDatum(unsigned int hash, unsigned int desc, unsigned int idx)
        : ArrayDatum(hash, desc) //
        , index(idx) {
    }
    ~BountyDatum() override {}
    void NotificationMessage(u32 msg, FEObject* pObj, u32 param1, u32 param2) override;
};

#endif
