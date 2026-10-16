// The online settings of the front end database. Only the PS2 prototype (SLES-53558-A124) has them: its .mdebug
// puts frontend/database/onlinedb.cpp in zFe2, between FEDatabase.cpp and RaceDB.cpp.
#include "Speed/Indep/Src/Online/OnlineCfg.hpp"
#include "Speed/Indep/Src/Online/VoiceCore.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/World/RaceParameters.hpp"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

void cOnlineSettings::Default() {
    TheOnlineRaceSettings.Default();

    if (SkipFE) {
        RaceMode = GRace::kRaceType_Circuit;
        TheOnlineRaceSettings.TrackDirection = TheRaceParameters.TrackDirection;
        TheOnlineRaceSettings.NumLaps = TheRaceParameters.NumLapsInRace;
    }

    iNumPlayers = 1;
    MinOnlinePlayers = 2;
    GameDetails = static_cast<eOnlineGameDetails>(3);
    onlineState = OLS_NETWORK_UNINITIALIZED;
    lastState = OLS_NETWORK_UNINITIALIZED;
    inErrorState = false;
    AnyCarClassAllowed = true;
    RankedGame = true;
    UseNOS = false;
    CollisionDetection = true;

    QuickJoinRace = false;
    Passworded = false;
    EmailInformation = false;
    ShareEmailAddress = false;
    IsPrivateRoom = false;
    ShowForeignReg = false;
    DisconnectPerc = static_cast<eOnlineDisconnectPerc>(6);
    PerformanceMatching = true;
    RestoreDefaultSettings();

    bMemSet(Password, 0, sizeof(Password));
    bMemSet(VerifyPassword, 0, sizeof(VerifyPassword));
    bMemSet(NetworkUserName, 0, sizeof(NetworkUserName));
    bMemSet(NetworkPersona, 0, sizeof(NetworkPersona));
    bMemSet(NetworkPasswd, 0, sizeof(NetworkPasswd));
    bMemSet(LANUserName, 0, sizeof(LANUserName));
    bMemSet(LANPersona, 0, sizeof(LANPersona));
    bMemSet(CountryCode, 0, sizeof(CountryCode));

    SelectedConnectingScreen = 0;
    ePrevPage = static_cast<eOnlinePrevPage>(0);
}

void cOnlineSettings::RestoreDefaultSettings() {
    ShowPlayerNames = true;
    ShowBadConnectionWarning = true;
    AppearOffline = false;
}

bool cOnlineSettings::operator==(const cOnlineSettings &rhs) const {
    return bMemCmp(this, &rhs, sizeof(cOnlineSettings)) == 0;
}

cOnlineSettings::eOnlineState cOnlineSettings::SetState(eOnlineState newState) {
    char textstate[2][64];

    inErrorState = false;

    for (int i = 0; i < 2; i++) {
        switch (i == 0 ? onlineState : newState) {
            case OLS_NETWORK_UNINITIALIZED:
                bStrCpy(textstate[i], "OLS_NETWORK_UNINITIALIZED");
                break;
            case OLS_CONNECTED_TO_ISP:
                bStrCpy(textstate[i], "OLS_CONNECTED_TO_ISP");
                break;
            case OLS_LOBBY_IDLE:
                bStrCpy(textstate[i], "OLS_LOBBY_IDLE");
                break;
            case OLS_LOBBY_REQUEST_PENDING:
                bStrCpy(textstate[i], "OLS_LOBBY_REQUEST_PENDING");
                break;
            case OLS_LOBBY_SEND_RACE_RESULTS_WAIT:
                bStrCpy(textstate[i], "OLS_LOBBY_SEND_RACE_RESULTS_WAIT");
                break;
            case OLS_LOBBY_SENT_RACE_RESULTS:
                bStrCpy(textstate[i], "OLS_LOBBY_SENT_RACE_RESULTS");
                break;
        }
    }

    lastState = onlineState;
    onlineState = newState;
    SelectedGameRoomListOption = 0;
    return lastState;
}

char *cOnlineSettings::GetLobbyAccountName() {
    return FEDatabase->IsLANMode() ? LANUserName : NetworkUserName;
}

char *cOnlineSettings::GetLobbyPersona() {
    return FEDatabase->IsLANMode() ? LANPersona : NetworkPersona;
}

char *cOnlineSettings::GetLobbyPassword() {
    return FEDatabase->IsLANMode() ? LANPasswd : NetworkPasswd;
}

void cOnlineSettings::SetLobbyAccountName(const char *name) {
    if (name) {
        bStrNCpy(FEDatabase->IsLANMode() ? LANUserName : NetworkUserName, name, 16);
    }
}

void cOnlineSettings::SetLobbyPersona(const char *persona) {
    if (persona) {
        bStrNCpy(FEDatabase->IsLANMode() ? LANPersona : NetworkPersona, persona, 16);
    }
}

void cOnlineSettings::SetLobbyPassword(const char *password) {
    if (password) {
        bStrNCpy(FEDatabase->IsLANMode() ? LANPasswd : NetworkPasswd, password, 16);
    }
}

char *cOnlineSettings::GetLobbyServerAddr() {
    return LobbyServerAddr;
}

uint16 cOnlineSettings::GetLobbyServerPort() {
    return LobbyServerPort;
}

char *cOnlineSettings::SaveToBuffer(char *buf) {
    char *maxbuf = buf + GetSaveBufferSize();
    unsigned int voip_state;
    float voip_volume = 0.0f;

    voip_state = VoiceCore::instance().GetSpeakerState();
    voip_volume = VoiceCore::instance().GetPlaybackVolume();

    buf = SaveSomeData(buf, &voip_state, 4, maxbuf);
    buf = SaveSomeData(buf, &voip_volume, 4, maxbuf);

    buf = SaveSomeData(buf, &ShowPlayerNames, 4, maxbuf);
    buf = SaveSomeData(buf, &ShowBadConnectionWarning, 4, maxbuf);
    buf = SaveSomeData(buf, &AppearOffline, 4, maxbuf);

    buf = SaveSomeData(buf, NetworkUserName, sizeof(NetworkUserName), maxbuf);
    buf = SaveSomeData(buf, NetworkPasswd, sizeof(NetworkPasswd), maxbuf);

    buf = SaveSomeData(buf, CountryCode, sizeof(CountryCode), maxbuf);

    return buf;
}

char *cOnlineSettings::LoadFromBuffer(char *buf) {
    char *maxbuf = buf + GetSaveBufferSize();
    unsigned int voip_volume = 0;
    float voip_state = 0.0f;

    buf = LoadSomeData(&voip_state, buf, 4, maxbuf);
    buf = LoadSomeData(&voip_volume, buf, 4, maxbuf);

    buf = LoadSomeData(&ShowPlayerNames, buf, 4, maxbuf);
    buf = LoadSomeData(&ShowBadConnectionWarning, buf, 4, maxbuf);
    buf = LoadSomeData(&AppearOffline, buf, 4, maxbuf);

    buf = LoadSomeData(NetworkUserName, buf, sizeof(NetworkUserName), maxbuf);
    buf = LoadSomeData(NetworkPasswd, buf, sizeof(NetworkPasswd), maxbuf);

    buf = LoadSomeData(CountryCode, buf, sizeof(CountryCode), maxbuf);

    VoiceCore::instance().SetSpeakerState(voip_state != 0.0f);

    VoiceCore::instance().SetPlaybackVolume(voip_volume);

    return buf;
}

int32 cOnlineSettings::GetSaveBufferSize() {
    return 63;
}

OnlineCreateUserSettings::OnlineCreateUserSettings() {
    Initialize();
}

void OnlineCreateUserSettings::Initialize() {
    UserName[0] = '\0';
    Password[0] = '\0';
    EmailAddress[0] = '\0';
    ConfirmPassword[0] = '\0';
    ParentalEmail[0] = '\0';

    EmailInformProducts = true;
    EmailInformPartners = true;
    TermsOfService = false;
    BirthMonth = 1;
    BirthDay = 1;
    BirthYear = 2003;
    Gender = eMale;
}
