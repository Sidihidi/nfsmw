#ifndef FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_UIONLINEQUICKJOIN_H
#define FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_UIONLINEQUICKJOIN_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEIconScrollerMenu.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/Online/LobbyGameSessions.hpp"

enum eQuickJoinPlayerOption {
    GS_CANCEL = 1,
    GS_CREATE_GAME = 2,
    GS_SEARCH_AGAIN = 3,
};

// total size: 0x5C
class UIOnlineGameSessionSearch : public MenuScreen {
  private:
    bool dialogactive;                    // offset 0x2C, size 0x4
    bool sessionstarted;                  // offset 0x30, size 0x4
    bool searchactive;                    // offset 0x34, size 0x4
    bool searcherror;                     // offset 0x38, size 0x4
    Timer searchtimeout;                  // offset 0x3C, size 0x4
    bool m_bWaitingToExit;                // offset 0x40, size 0x4
    bool m_bWaitingForJoinResult;         // offset 0x44, size 0x4
    bool m_bJoinSessionOK;                // offset 0x48, size 0x4
    int m_numSessions;                    // offset 0x4C, size 0x4
    dialog_handle m_dialogHandle;         // offset 0x50, size 0x4
    // const: el constructor de la A124 compara m_eSearchSection despues de dos llamadas con el valor que leyo para
    // inicializarlo, sin volver a cargarlo (solo pasa si el miembro no puede cambiar). Los stabs no guardan el const.
    const eOnlineConnectingType m_eSearchSection; // offset 0x54, size 0x4
    int m_nDiscPercCBRetCode;             // offset 0x58, size 0x4
    static bool m_bConnectionStatusOK;

  public:
    UIOnlineGameSessionSearch(ScreenConstructorData *sd, const eOnlineConnectingType &eSearchSection);
    ~UIOnlineGameSessionSearch() override;

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;
    void UpdateCallback(LobbyGameSessionsN::SessionStatusCode code, const LobbyApiUserSetT *session);
    void JoinSessionCallback(LobbyApiMsgT *msg);
    static void ConnectionCoreCB(ConnApiRefT *connapi, ConnApiCbInfoT *cbinfo, void *context);
    int32 GetDisconnectPercentage(int32 *pDisconnectPercentage);
    static void GetExtendedStatsCB(LobbyApiMsgT *msg, void *context);

  private:
    void joinsession(const LobbyApiUserSetT *session);
    void checksessions();
    void startsearch();
    void createsession();
    uint32 pick_random_event(GRace::Type &gamemode);
    // Segundos que se busca partida antes de ofrecer crear una (la A124 lo expande en startsearch: 5.0f).
    static float search_timeout() {
        return 5.0f;
    }
    static void FindSessionsCB(LobbyApiMsgT *msg, void *context);
};

// total size: 0x174
class UIOnlineQuickJoin : public IconScrollerMenu {
  public:
    UIOnlineQuickJoin(ScreenConstructorData *sd);
    ~UIOnlineQuickJoin() override {}

    void NotificationMessage(u32 msg, FEObject *obj, u32 param1, u32 param2) override;

  private:
    void Setup() override;

    bool m_boLaunchMatchList; // offset 0x16C, size 0x4
    int m_dialogHandle;       // offset 0x170, size 0x4
};

// Definida en UIOnlineQuickJoin.cpp despues de startsearch, que la registra como callback de LobbyGameSessions.
void FindMatchUpdateCB(LobbyGameSessionsN::SessionStatusCode code, const LobbyApiUserSetT *session, void *context);

// CreateOLConnectingScreen (FEPackageData.cpp) la llama para las busquedas de partida.
MenuScreen *CreateOnlineGameSessionSearch(ScreenConstructorData *sd, const eOnlineConnectingType &eSearchSection);

#endif
