#ifndef FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_UIONLINEVIEWTRACK_H
#define FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_UIONLINEVIEWTRACK_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/quickrace/uiQRTrackSelect.hpp"

// total size: 0x11C
class uiOnlineViewTrack : public UIQRTrackSelect {
  public:
    uiOnlineViewTrack(ScreenConstructorData *sd);
    ~uiOnlineViewTrack() override {}

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;
    void Setup() override;

  private:
    void SetCurrentTrack();
};

// Definido en uiOnlineMain.cpp.
extern const char *g_GameRoomName;

#endif
