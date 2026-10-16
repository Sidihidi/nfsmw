#ifndef FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_UIONLINEMAIN_H
#define FRONTEND_MENUSCREENS_SAFEHOUSE_ONLINE_UIONLINEMAIN_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/FEng/FEString.h"
#include "Speed/Indep/Src/Frontend/FEngFont.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feScrollerina.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/Online/LobbyGameSessions.hpp"
#include "Speed/Indep/Src/Physics/PhysicsInfo.hpp"

class RideInfo;
class FECarRecord;

enum eGameRoomListOption {
    eGameRoomListOption_HOST = 0,
    eGameRoomListOption_CLIENT = 1,
    eGameRoomListOption_CAR_SELECT = 2,
};

// total size: 0x48
class PlayerScrollerDatum : public ScrollerDatum {
  public:
    uint32 carNameHash;                          // offset 0x18, size 0x4
    Physics::Info::Performance carPerformance;   // offset 0x1C, size 0xC
    bool hasCompletedCareer;                     // offset 0x28, size 0x4
    bool makingChanges;                          // offset 0x2C, size 0x4
    char name[16];                               // offset 0x30, size 0x10
    uint32 points;                               // offset 0x40, size 0x4
    bool m_bValidData;                           // offset 0x44, size 0x4

    PlayerScrollerDatum()
        : carNameHash(0), hasCompletedCareer(false), makingChanges(false), points(0xffffffff), m_bValidData(false) {
        name[0] = '\0';
    }
    ~PlayerScrollerDatum() override {}
};

// total size: 0x11FC
class UIOLGameRoom : public MenuScreen {
  public:
    // total size: 0x1084
    class ChatBuffer {
      public:
        FEObject *mpArrow1;      // offset 0x0, size 0x4
        FEObject *mpArrow2;      // offset 0x4, size 0x4
        FEScrollBar mScrollBar;  // offset 0x8, size 0x64

      private:
        FEString *mpChatBody;    // offset 0x6C, size 0x4
        FEngFont *mpFont;        // offset 0x70, size 0x4
        int mWidth;              // offset 0x74, size 0x4
        int mFlags;              // offset 0x78, size 0x4
        int mScroll;             // offset 0x7C, size 0x4
        int mLines;              // offset 0x80, size 0x4
        char mBuffer[4096];      // offset 0x84, size 0x1000

      public:
        ChatBuffer(const char *packagename);
        ~ChatBuffer() {}
        void Receive(char *message);
        void ScrollChatBodyUp();
        void ScrollChatBodyDown();
        char *GetChatBuffer() {
            return mBuffer;
        }

      private:
        int WordWrap(char *message);
        void AddLine(char *message, int size);
        void Display();
    };

    static PlayerScrollerDatum *mSelectedPlayerDatum;
    static int mGameCountdown;

  private:
    Timer m_InfoBoxTimer;            // offset 0x2C, size 0x4
    Scrollerina playerScroller;      // offset 0x30, size 0xC8
    ChatBuffer mChat;                // offset 0xF8, size 0x1084
    bool startButtonEnabled;         // offset 0x117C, size 0x4
    static const int countdownSeconds;
    FEObject *the_type_area;         // offset 0x1180, size 0x4
    dialog_handle msgHandle;         // offset 0x1184, size 0x4
    RideInfo *m_rideInfo;            // offset 0x1188, size 0x4
    uint32 m_carHandle;              // offset 0x118C, size 0x4
    FECarRecord *m_carRecord;        // offset 0x1190, size 0x4
    bool m_viewingModalDisplay;      // offset 0x1194, size 0x4
    bool m_setDimState;              // offset 0x1198, size 0x4
    Timer m_startGameTimer;          // offset 0x119C, size 0x4
    bool m_bTestStartGameTimeOut;    // offset 0x11A0, size 0x4
    bool m_sendPrivateMsg;           // offset 0x11A4, size 0x4
    ScrollerSlot *m_startReadySlot;  // offset 0x11A8, size 0x4
    bool m_isPrivateRoom;            // offset 0x11AC, size 0x4
    int m_numPlayers;                // offset 0x11B0, size 0x4
    bool m_bWaitingToViewCar;        // offset 0x11B4, size 0x4
    uint32 m_slotIndex;              // offset 0x11B8, size 0x4
    bool m_bShowingHostModal;        // offset 0x11BC, size 0x4
    bool m_bShowingClientModal;      // offset 0x11C0, size 0x4
    bool m_bSetSelectionToUser;      // offset 0x11C4, size 0x4
    bool m_bLocallyReady;            // offset 0x11C8, size 0x4
    bool m_bReadyButtonPressed;      // offset 0x11CC, size 0x4
    bool doUpdateSession;            // offset 0x11D0, size 0x4
    bool doDismissDialog;            // offset 0x11D4, size 0x4
    bool doSwitchToLobby;            // offset 0x11D8, size 0x4
    bool m_bChangingSettings;        // offset 0x11DC, size 0x4
    Timer m_hostStartUpdateTimer;    // offset 0x11E0, size 0x4
    bool m_isRaceStarted;            // offset 0x11E4, size 0x4
    struct {
        bool showDialog;             // offset 0x0, size 0x4
        uint32 dialogTitle;          // offset 0x4, size 0x4
        uint32 buttonPressedMessage; // offset 0x8, size 0x4
        uint32 buttonCancelMessage;  // offset 0xC, size 0x4
        const char *dialogText;      // offset 0x10, size 0x4
    } delayedDialogDetails;          // offset 0x11E8, size 0x14

  public:
    UIOLGameRoom(ScreenConstructorData *sd);
    ~UIOLGameRoom() override;

    void NotificationMessage(u32 msg, FEObject *pobj, u32 param1, u32 param2) override;

    static void DisplayGameDetails(MenuScreen *pGameRoom);

  private:
    void Setup();
    void StartGame();
    void LeaveSession();
    void InitializePlayerList();
    void PopulatePlayerList();
    void AddOpenSlotAtIndex(int32 index);
    void AddSessionPlayers();
    void Redraw(bool dim);
    void ScrollPlayerList(int direction);
    void SetConnectionQualityGood(uint32 index);
    void SetConnectionQualityAverage(uint32 index);
    void SetConnectionQualityBad(uint32 index);
    void SetConnectionQualityUnknown(uint32 index);
    void TurnOffPingCircles(uint32 index);
    void MaybeUpdateSession();
    static void SessionUpdateCB(LobbyGameSessionsN::SessionStatusCode code, const LobbyApiUserSetT *session, void *context);
    static void CreateSessionCB(LobbyApiMsgT *msg, void *context);
    static void LeaveSessionCB(LobbyApiMsgT *msg, void *context);
    static void StartGameCB(LobbyApiMsgT *msg, void *context);
    static void GetUserOnlineRecordCB(LobbyApiMsgT *msg, void *context);
    static void ReceiveChatCB(void *online_chat_panel, char *message, bool is_private, bool is_special, bool is_local);
    static void PingPlayerCB(DirtyAddrT *pAddr, uint32 uPing, void *pData);
    void SubmitComplaintCB(LobbyApiMsgT *msg);
    void ShowVOIPStatus();
    void SendChallengeInvite(char *name);
    void CheckForBuddyOnline();
    void SetDelayedDialog(uint32 titleHash, uint32 pressMessage, uint32 cancelMessage, const char *text);
    void MaybeShowDelayedDialog();
    void MaybeDismissDialog();
    void MaybeSwitchToLobby();
    void CheckDisplayOfHostStartCountdown();
    void MaybeCreateGameInSession();
    void ExitToOnlineMain();
    void VerifyMyCar();
    void UpdateInfoBox();
    void ViewPlayersCar(PlayerScrollerDatum *pPlayerDatum);
};

const char *GetTopPackage();
void ShowDisconnectDialog(const char *pkg, LobbyCoreN::OnlineDisconnectReason reason, int code, uint32 label);
void DisconnectCB(LobbyCoreN::OnlineDisconnectReason reason, LobbyApiMsgT *msg, void *context);
void HandleBuddyDisconnect();

#endif
