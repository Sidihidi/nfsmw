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
#if !defined(EA_PLATFORM_WIN32) && !defined(EA_PLATFORM_XENON)
    inErrorState = false;
#endif
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
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    // PC: en speed.exe el store a +0x44 va tras el de ShowForeignReg (+0x43); Xbox 360 igual (EGM 0x822EE0C4).
    win32_unknown_0x44 = false;
#endif
    DisconnectPerc = static_cast<eOnlineDisconnectPerc>(6);
    PerformanceMatching = true;
    RestoreDefaultSettings();
#ifdef EA_PLATFORM_WIN32
    win32_unknown_0x54 = 1; // PC: Default de speed.exe pone a 1 el entero de +0x54
#endif
#ifdef EA_PLATFORM_XENON
    // Xbox 360 (EGM 0x822EE0D0): los dos campos sin nombre de +0x58/+0x5C, tras RestoreDefaultSettings.
    xenon_unknown_0x58 = 3;
    xenon_unknown_0x5c = false;
#endif

    bMemSet(Password, 0, sizeof(Password));
    bMemSet(VerifyPassword, 0, sizeof(VerifyPassword));
    bMemSet(NetworkUserName, 0, sizeof(NetworkUserName));
    bMemSet(NetworkPersona, 0, sizeof(NetworkPersona));
    bMemSet(NetworkPasswd, 0, sizeof(NetworkPasswd));
    bMemSet(LANUserName, 0, sizeof(LANUserName));
    bMemSet(LANPersona, 0, sizeof(LANPersona));
    bMemSet(CountryCode, 0, sizeof(CountryCode));

#ifdef EA_PLATFORM_WIN32
    // PC: Default de speed.exe limpia tambien la cadena de +0xE9.
    bMemSet(win32_unknown_0xe9, 0, sizeof(win32_unknown_0xe9));
#endif

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
#ifdef EA_PLATFORM_WIN32
    // PC (speed.exe): la forma corta, expandida en los llamantes (cola de ChallengeAcceptGameCB); sin el
    // volcado de depuracion del prototipo ni el borrado de inErrorState.
    lastState = onlineState;
    onlineState = newState;
    SelectedGameRoomListOption = 0;
    return lastState;
#else
    char textstate[2][64];

#ifndef EA_PLATFORM_XENON
    // Xbox 360: el SetState del EGM (0x822EE1F8) no borra inErrorState.
    inErrorState = false;
#endif

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
#endif
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
#ifdef EA_PLATFORM_WIN32
    // PC (speed.exe 0x56E440): en LAN devuelve el bufer de 0x8F42EC ("127.0.0.1").
    return FEDatabase->IsLANMode() ? LANServerAddr : LobbyServerAddr;
#else
    return LobbyServerAddr;
#endif
}

uint16 cOnlineSettings::GetLobbyServerPort() {
#ifdef EA_PLATFORM_WIN32
    // PC (speed.exe 0x56E460): en LAN devuelve el puerto de 0x8F430C (9900).
    return FEDatabase->IsLANMode() ? LANServerPort : LobbyServerPort;
#else
    return LobbyServerPort;
#endif
}

#ifdef EA_PLATFORM_WIN32
// PC (speed.exe 0x56E480, 0x56E4A0 y 0x56E4D0, en este orden tras GetLobbyServerPort): nombres NEUTROS.
void cOnlineSettings::Win32Unknown56E480(const char *addr) {
    if (addr) {
        bStrCpy(LANServerAddr, addr);
    }
}

void cOnlineSettings::SetWin32Unknown0xe9(const char *value) {
    if (value) {
        bStrNCpy(win32_unknown_0xe9, value, 21);
        bToUpper(win32_unknown_0xe9);
    }
}

char *cOnlineSettings::GetWin32Unknown0xe9() {
    return win32_unknown_0xe9;
}
#endif

char *cOnlineSettings::SaveToBuffer(char *buf) {
    char *maxbuf = buf + GetSaveBufferSize();
    unsigned int voip_state;
    float voip_volume = 0.0f;

#ifdef EA_PLATFORM_WIN32
    // PC (speed.exe 0x57FFB0): los dos valores de voip se guardan a cero, sin consultar VoiceCore (no hay
    // ninguna reubicacion a VoiceCore y el bufer se acota en buf+0x36).
    voip_volume = 0.0f;
    voip_state = 0;
#else
    voip_state = VoiceCore::instance().GetSpeakerState();
    voip_volume = VoiceCore::instance().GetPlaybackVolume();
#endif

    buf = SaveSomeData(buf, &voip_state, 4, maxbuf);
    buf = SaveSomeData(buf, &voip_volume, 4, maxbuf);

#ifdef EA_PLATFORM_WIN32
    // PC guarda cada bool en 1 B (los tres stores de byte desde +0x40/+0x41/+0x42; total 54, no 63).
    buf = SaveSomeData(buf, &ShowPlayerNames, 1, maxbuf);
    buf = SaveSomeData(buf, &ShowBadConnectionWarning, 1, maxbuf);
    buf = SaveSomeData(buf, &AppearOffline, 1, maxbuf);
#else
    buf = SaveSomeData(buf, &ShowPlayerNames, 4, maxbuf);
    buf = SaveSomeData(buf, &ShowBadConnectionWarning, 4, maxbuf);
    buf = SaveSomeData(buf, &AppearOffline, 4, maxbuf);
#endif

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

#ifdef EA_PLATFORM_WIN32
    // PC: como al guardar, cada bool ocupa 1 B (total 54).
    buf = LoadSomeData(&ShowPlayerNames, buf, 1, maxbuf);
    buf = LoadSomeData(&ShowBadConnectionWarning, buf, 1, maxbuf);
    buf = LoadSomeData(&AppearOffline, buf, 1, maxbuf);
#else
    buf = LoadSomeData(&ShowPlayerNames, buf, 4, maxbuf);
    buf = LoadSomeData(&ShowBadConnectionWarning, buf, 4, maxbuf);
    buf = LoadSomeData(&AppearOffline, buf, 4, maxbuf);
#endif

    buf = LoadSomeData(NetworkUserName, buf, sizeof(NetworkUserName), maxbuf);
    buf = LoadSomeData(NetworkPasswd, buf, sizeof(NetworkPasswd), maxbuf);

    buf = LoadSomeData(CountryCode, buf, sizeof(CountryCode), maxbuf);

    VoiceCore::instance().SetSpeakerState(voip_state != 0.0f);

    VoiceCore::instance().SetPlaybackVolume(voip_volume);

    return buf;
}

int32 cOnlineSettings::GetSaveBufferSize() {
#ifdef EA_PLATFORM_WIN32
    return 54; // PC: 4+4+1+1+1+20+20+3 (SaveToBuffer de speed.exe acota en buf+0x36)
#else
    return 63;
#endif
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
