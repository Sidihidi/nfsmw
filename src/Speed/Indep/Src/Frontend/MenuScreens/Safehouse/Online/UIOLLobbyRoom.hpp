#ifndef FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_UIOLLOBBYROOM_H
#define FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_UIOLLOBBYROOM_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feScrollerina.hpp"
#include "Speed/Indep/Src/Online/LobbyGameSessions.hpp"

enum eSORTCATEGORY {
    eSORTCATEGORY_NAME = 1,
    eSORTCATEGORY_NUM_PLAYERS = 2,
    eSORTCATEGORY_PING = 3,
};

enum eLobbyButtons {
    JOIN_BUTTON = 0,
    CREATE_BUTTON = 1,
    REFRESH_BUTTON = 2,
    FILTER_BUTTON = 3,
    NUM_LOBBY_BUTTONS = 4,
};

// total size: 0x220
class UIOLLobbyRoom : public MenuScreen {
  public:
    // total size: 0x40
    class LobbyScrollerDatum : public ScrollerDatum {
      public:
        int sessionIdent;     // offset 0x18, size 0x4
        char sessionName[36]; // offset 0x1C, size 0x24

        LobbyScrollerDatum() : sessionIdent(-1) {
            sessionName[0] = '\0';
        }
        ~LobbyScrollerDatum() override {}
    };

    static bool sExittingOnline;

  private:
    Scrollerina GameScroller;                           // offset 0x2C, size 0xC8
    Scrollerina TitleScroller;                          // offset 0xF4, size 0xC8
    bool bAllowScroll;                                  // offset 0x1BC, size 0x4
    bool haveRunFindSessions;                           // offset 0x1C0, size 0x4
    int numFailedJoinAttempts;                          // offset 0x1C4, size 0x4
    dialog_handle msgHandle;                            // offset 0x1C8, size 0x4
#ifdef EA_PLATFORM_XENON
    // EGM: un segundo dialogo en +0x198 (FindSessions guarda aqui el suyo, 0x82279150; NotificationMessage lo
    // cierra, 0x82277688). El ctor no lo toca. Ningun binario da el nombre.
    dialog_handle xenon_unknown_0x198;
#endif
    eLobbyButtons curOption;                            // offset 0x1CC, size 0x4
    bool doBadConnectionTest;                           // offset 0x1D0, size 0x4
    static const char *m_password;
    bool quickjoin;                                     // offset 0x1D4, size 0x4
    bool redrawSessionsList;                            // offset 0x1D8, size 0x4
    bool doSwitchToGameRoomScreen;                      // offset 0x1DC, size 0x4
    bool doDismissDialog;                               // offset 0x1E0, size 0x4
    struct {
        bool showDialog;                                // offset 0x0, size 0x4
        uint32 dialogTitle;                             // offset 0x4, size 0x4
        uint32 buttonPressedMessage;                    // offset 0x8, size 0x4
        uint32 buttonCancelMessage;                     // offset 0xC, size 0x4
        const char *dialogText;                         // offset 0x10, size 0x4
    } delayedDialogDetails;                             // offset 0x1E4, size 0x14
    bool bNameAscending;                                // offset 0x1F8, size 0x4
    bool bTypeAscending;                                // offset 0x1FC, size 0x4
    bool bRacersAscending;                              // offset 0x200, size 0x4
    bool bPingAscending;                                // offset 0x204, size 0x4
    bool bCourseAscending;                              // offset 0x208, size 0x4
    bool m_bDisableMouseInput;                          // offset 0x20C, size 0x4
    int m_LobbyApiMsgT_code;                            // offset 0x210, size 0x4
    bool m_bPerformJoinSession;                         // offset 0x214, size 0x4
    int m_currentSortColumn;                            // offset 0x218, size 0x4
    eOnlineDisconnectPerc m_PlayerDisconnectMinThreshold; // offset 0x21C, size 0x4
#ifdef EA_PLATFORM_XENON
    int xenon_unknown_0x1d4; // EGM: el ctor lo pone a cero el ultimo (0x822771A0); 0x822785F4 lo escribe
#endif

  public:
    UIOLLobbyRoom(ScreenConstructorData *sd);
    ~UIOLLobbyRoom() override;

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;

    static void SetPassword(const char *);

  private:
    void Setup();
    void PopulateSessionsList();
    void DrawSessionsList();
    void MaybeDrawSessionsList();
    void JoinSession();
    int32 FindSessions();
    int32 CreateSession();
    void SetConnectionQualityGood(int row);
    void SetConnectionQualityAverage(int row);
    void SetConnectionQualityBad(int row);
    void SetConnectionQualityUnknown(int row);
    void TurnOffPingCircles(int row);
    void MaybeFindSessions();
    void MaybeUpdateServerUsageStats();
    void MaybeDismissDialog();
    static void JoinSessionCB(LobbyApiMsgT *msg, void *context);
    static void SessionUpdateCB(LobbyGameSessionsN::SessionStatusCode code, const LobbyApiUserSetT *session, void *context);
    static void FindSessionsCB(LobbyApiMsgT *msg, void *context);
    void ShowCarSwitchConfirmDialog();
    void SwitchToOnlineCareerCar();
    void JoinButton();
    void RefreshButton();
    void SetDelayedDialog(uint32 titleHash, uint32 pressMessage, uint32 cancelMessage, const char *text);
    void MaybeShowDelayedDialog();
    void PerformJoinSession();
    void SwitchSortColumn();
    void DisplaySingleSessionInformation();
};

// Definido en uiOnlineFilters.cpp (detras de uiOLLobbyRoom.cpp en la unidad zFe).
extern FilterGameSessionParamsT OnlineSessionFilter;

#endif
