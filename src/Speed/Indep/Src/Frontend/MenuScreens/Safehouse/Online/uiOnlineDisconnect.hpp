#ifndef FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_UIONLINEDISCONNECT_H
#define FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_UIONLINEDISCONNECT_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Online/LobbyCore.hpp"
#include "Speed/Indep/Src/Online/OnlineCfg.hpp"

// total size: 0x3C
class CUIOnlineDisconnect : public MenuScreen {
  public:
    static bool mIsHostInGameDisconnect;

  private:
    bool m_bReconnectInProgress;     // offset 0x2C, size 0x4
    bool m_bReconnectSuccessful;     // offset 0x30, size 0x4
    bool m_bDisconnectDialogPending; // offset 0x34, size 0x4
    bool m_bDisconnectDialogUp;      // offset 0x38, size 0x4

    static CUIOnlineDisconnect *m_pUIOnlineDisconnect_Instance;
    static cOnlineSettings::eOnlineState m_FinalReconnectState;
    static bool m_bNotificationPending;

  public:
    CUIOnlineDisconnect(ScreenConstructorData *sd);
    ~CUIOnlineDisconnect() override;

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

    static void ShowDialog(LobbyCoreN::OnlineDisconnectReason Reason, uint32 Code, uint32 Label);
    static void SetDisconnectNotificationPending() {
        m_bNotificationPending = true;
    }
    static void ClearDisconnectNotificationPending() {
        m_bNotificationPending = false;
    }
    static bool IsDisconnectNotificationPending() {
        return m_bNotificationPending;
    }
    static void SetReconnectFinalState();
    static bool IsReconnectable(uint32 DisconnectErrorMsgHash);
    static uint32 GetDisconnectMessage(LobbyCoreN::OnlineDisconnectReason Reason, uint32 Code, uint32 Label);
    static void DebugPrintf(const char *fmt, ...) {}

    void DisconnectDialogConstructed();
    void DisconnectDialogGone();
    void DoDisconnectBackToMain();
    void DoReconnect();
    void ReconnectSuccessful();
    void ReconnectUnSuccessful();
};

void ExitOnlineMode(const char *currentPackageName);

#endif
