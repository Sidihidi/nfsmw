#ifndef ONLINE_ONLINECFG_H
#define ONLINE_ONLINECFG_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include <types.h>

#include "Speed/Indep/Src/Gameplay/GRace.h"
// OnlineRaceParameters (TheOnlineRaceSettings) es la class de RaceDB.hpp (DWARF: RaceDB.hpp:157); con una copia
// propia aqui, zOnline la decoraba como struct en MSVC (?GetRaceSettings@...PAUOnlineRaceParameters@@).
#include "Speed/Indep/Src/Frontend/Database/RaceDB.hpp"
#ifdef EA_PLATFORM_WIN32
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#endif

struct OnlineCfg {
    static void ReadConfigFile(const char *filename);

  private:
    static void ProcessSetting(const char *attribute, const char *value);
};

// TODO right place for these?
enum eOnlineGameDetails { eHOSTING = 0, eJOINING = 1, eVIEW_PLAYER = 2, eVIEW_GAME = 3 };

enum eOnlineConnectingType {
    eOnlineConnecting_ISP = 0,
    eOnlineConnecting_GAMEROOM = 1,
    eOnlineConnecting_QuickMatchSearch = 2,
    eOnlineConnecting_LobbyMatchSearch = 3
};

enum eOnlineDisconnectPerc {
    OLS_DISCONNECT_PERC_5 = 0,
    OLS_DISCONNECT_PERC_10 = 1,
    OLS_DISCONNECT_PERC_15 = 2,
    OLS_DISCONNECT_PERC_20 = 3,
    OLS_DISCONNECT_PERC_25 = 4,
    OLS_DISCONNECT_PERC_50 = 5,
    OLS_DISCONNECT_PERC_ANY = 6
};

// UIOnlineQuickJoin.cpp; la usan tambien las opciones de la carrera online (UIQRTrackOptions::Setup).
eOnlineDisconnectPerc GetDiconnectPercentageThreshold(const int32 &nDisconnectPercentage);

enum eOnlinePrevPage { ePrevPage_None = 0, ePrevPage_LobbyRoom = 1 };

// total size: 0x114
struct cOnlineSettings {
    friend struct OnlineCfg;

    enum eOnlineState {
        OLS_NETWORK_UNINITIALIZED = 0,
        OLS_CONNECTED_TO_ISP = 1,
        OLS_LOBBY_LOGGED_IN = 2,
        OLS_LOBBY_IDLE = 2,
        OLS_LOBBY_REQUEST_PENDING = 3,
        OLS_LOBBY_SEND_RACE_RESULTS_WAIT = 4,
        OLS_LOBBY_SENT_RACE_RESULTS = 5
    };

    /* 0x000 */ int iNumPlayers;
    // const en la A124: _15cOnlineSettings$MaxOnlinePlayers vive en .rodata (0x55c97a, vale 4) y EE-GCC adelanta su
    // lectura por encima de los stores (EditGameDetails::ChangeSelectedItem).
#ifdef EA_PLATFORM_XENON
    // Xbox 360: con valor en la clase; LobbyGameSessions::CreateSession ("SIZE") y LobbyGames ("MAXSIZE") pasan el 4
    // como inmediato (`li r6, 4`) y el EGM no tiene el dato en .rdata.
    static const uint8 MaxOnlinePlayers = 4;
#else
    static const uint8 MaxOnlinePlayers;
#endif
    /* 0x004 */ uint8 MinOnlinePlayers;
    /* 0x008 */ eOnlineGameDetails GameDetails;
    /* 0x00c */ bool AnyCarClassAllowed;
    /* 0x010 */ bool RankedGame;
    /* 0x014 */ bool UseNOS;
    /* 0x018 */ bool CollisionDetection;
    /* 0x01c */ bool Passworded;
    /* 0x020 */ char Password[13];
    /* 0x02d */ char VerifyPassword[13];
    /* 0x03c */ bool PerformanceMatching;
    /* 0x040 */ eOnlineDisconnectPerc DisconnectPerc;
    /* 0x044 */ eOnlineDisconnectPerc CurrPlayerDisconnectPerc;
    /* 0x048 */ GRace::Type RaceMode;
    /* 0x04c */ bool QuickJoinRace;
    /* 0x050 */ bool EmailInformation;
    /* 0x054 */ bool ShareEmailAddress;
    /* 0x058 */ char CountryCode[3];
    /* 0x05c */ bool IsUserHostingLANServer;
    /* 0x060 */ bool IsPrivateRoom;
    /* 0x064 */ bool ShowPlayerNames;
    /* 0x068 */ bool ShowBadConnectionWarning;
    /* 0x06c */ bool AppearOffline;
    /* 0x070 */ bool ShowForeignReg;
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    // PC (speed.exe) y Xbox 360: un bool en +0x44 (cOnlineSettings::Default lo pone a 0 justo tras ShowForeignReg, +0x43;
    // EGM 0x822EE018). NO es inErrorState: SetState no lo borra y BuddyConnectCB/doprocessing no lo leen; solo lo
    // escriben Default y LobbyGameSessions::Win32Unknown79A1D0 (true desde el callback de la sala, false desde
    // EndOnlineRace) y lo lee UpdateSessionFlags como el bit 0x4000 de la sesion, que FindSessions de PC filtra.
    // Probablemente "carrera en curso" de la sesion. Nombre NEUTRO: ningun binario lo da. Tras lastState no hay
    // inErrorState en estas dos (NetworkUserName empieza en +0x70 en PC: SaveToBuffer).
    bool win32_unknown_0x44;
#endif
#ifdef EA_PLATFORM_XENON
    // Xbox 360: SetState (0x822EE1F8) pone a 0 SelectedGameRoomListOption en +0x4C y Default deja SelectedConnectingScreen
    // y ePrevPage en +0x50/+0x54: 4 B mas aqui, que ni Default ni SetState tocan. Sin nombre en ningun binario.
    int xenon_unknown_0x48;
#endif
    /* 0x074 */ int SelectedGameRoomListOption;
    /* 0x078 */ int SelectedConnectingScreen;
    /* 0x07c */ eOnlinePrevPage ePrevPage;
#ifdef EA_PLATFORM_WIN32
    int win32_unknown_0x54; // PC: Default lo pone a 1 (+0x54 en speed.exe); nombre nuestro
#endif
#ifdef EA_PLATFORM_XENON
    // Xbox 360: Default los pone a 3 y a false tras RestoreDefaultSettings, donde PC pone a 1 su win32_unknown_0x54
    // (probablemente el mismo campo); el EGM los lee en FEDatabase + 0x190/0x194. Sin nombre en ningun binario.
    int xenon_unknown_0x58;
    bool xenon_unknown_0x5c;
#endif

  private:
    friend struct BuddyCore;

    /* 0x080 */ OnlineRaceParameters TheOnlineRaceSettings;
    /* 0x08c */ eOnlineState onlineState;
    /* 0x090 */ eOnlineState lastState;
#if !defined(EA_PLATFORM_WIN32) && !defined(EA_PLATFORM_XENON)
    /* 0x094 */ bool inErrorState;
#endif
    /* 0x098 */ char NetworkUserName[20];
    /* 0x0ac */ char NetworkPersona[20];
    /* 0x0c0 */ char NetworkPasswd[20];
    /* 0x0d4 */ char LANUserName[20];
    /* 0x0e8 */ char LANPersona[20];
    /* 0x0fc */ char LANPasswd[20];
    /* 0x110 */ bool bLoggedInToBuddyServer;
#ifdef EA_PLATFORM_WIN32
    // PC: Default limpia 21 B tras bLoggedInToBuddyServer (+0xE8 en BuddyConnectCB de speed.exe) de una sola
    // pasada; el sizeof total es 0x100 (operator==: repe cmpsd de 0x40 dwords).
    char win32_unknown_0xe9[21];
#endif
    static char LobbyServerAddr[32];
    static uint16 LobbyServerPort;
#ifdef EA_PLATFORM_WIN32
    // PC: el par del servidor en LAN, tras el de internet en el .data de speed.exe (0x8F42EC "127.0.0.1" y
    // 0x8F430C 9900; LobbyServerAddr/Port estan en 0x8F42C8/0x8F42E8). Solo los leen GetLobbyServerAddr
    // (0x56E440) y GetLobbyServerPort (0x56E460) cuando FEDatabase->IsLANMode(); nombres nuestros.
    static char LANServerAddr[32];
    static uint16 LANServerPort;
#endif

  public:
#if defined(EA_BUILD_A124) || defined(EA_PLATFORM_WIN32)
    // Inline in the prototype and on PC: the constructor of cFrontendDatabase calls Default directly.
    cOnlineSettings() {
        Default();
    }
#else
    cOnlineSettings();
#endif
    void Default();
    void RestoreDefaultSettings();
    bool operator==(const cOnlineSettings &rhs) const;
    eOnlineState SetState(eOnlineState state);
    eOnlineState GetState() {
        return onlineState;
    }
    eOnlineState GetLastState();
    void SetErrorState();
#if !defined(EA_PLATFORM_WIN32) && !defined(EA_PLATFORM_XENON)
    // Inline en la A124 (OnlineDB.hpp:125; UIOnlineLogin::BuddyConnectCB lo expande: lw 148(settings)). PC y Xbox 360
    // no tienen inErrorState (ver win32_unknown_0x44).
    bool IsInErrorState() {
        return inErrorState;
    }
#endif
    OnlineRaceParameters *GetRaceSettings() { return &TheOnlineRaceSettings; }
    char *GetLobbyAccountName();
    char *GetLobbyPersona();
    char *GetLobbyPassword();
    void SetLobbyAccountName(const char *name);
    void SetLobbyPersona(const char *persona);
    void SetLobbyPassword(const char *password);
    char *GetLobbyServerAddr();
    uint16 GetLobbyServerPort();
#ifdef EA_PLATFORM_WIN32
    // PC (speed.exe 0x56E4D0: `lea eax, [ecx + 0xE9]`): devuelve win32_unknown_0xe9; LobbyLogin::DoContextLogin lo
    // manda como REGKEY (probablemente la clave de registro del juego; 0x56E4A0 la copia con bStrNCpy de 21 y
    // bToUpper). Nombre NEUTRO: ningun binario lo da.
    char *GetWin32Unknown0xe9();
    // PC (0x56E4A0): la copia con bStrNCpy de 21 y bToUpper (desde UIOnlineLogin::DoLobbyLoginProcessing). NEUTRO.
    void SetWin32Unknown0xe9(const char *value);
    // PC (0x56E480): copia la direccion en LANServerAddr (bStrCpy, si no es nula). Nombre NEUTRO.
    void Win32Unknown56E480(const char *addr);
#endif
    char *SaveToBuffer(char *buf);
    char *LoadFromBuffer(char *buf);
    int32 GetSaveBufferSize();
    char *GetGamePassword();
#ifdef EA_PLATFORM_WIN32
    void SetGamePassword(const char *pwd) {
        bStrNCpy(Password, pwd, 12);
    }
    void SetGameVerifyPassword(const char *pwd) {
        bStrNCpy(VerifyPassword, pwd, 12);
    }
#else
    void SetGamePassword(const char *pwd);
    void SetGameVerifyPassword(const char *pwd);
#endif
    // Inline en la A124 (OnlineDB.hpp:192-193; UIOnlineLogin::BuddyConnectCB y DoLobbyLoginProcessing los expanden).
    // La A124 lo declara SetBuddyServerLoggedIn(bool); el nombre del parametro no queda en los stabs.
    void SetBuddyServerLoggedIn(bool bLoggedIn) {
        bLoggedInToBuddyServer = bLoggedIn;
    }
    bool IsLoggedInToBuddyServer() {
        return bLoggedInToBuddyServer;
    }
};

void AllocOnlineMemoryPool();
void FreeOnlineMemoryPool();

enum OnlineGender { eMale = 0, eFemale = 1, eNum_Gender = 2 };

struct OnlineCreateUserSettings { // 0xe8
    /* 0x00 */ char UserName[17];
    /* 0x11 */ char Password[17];
    /* 0x22 */ char ConfirmPassword[17];
    /* 0x33 */ char EmailAddress[51];
    /* 0x66 */ char ParentalEmail[51];
    /* 0x99 */ char TruncatedLongEmail[51];
    /* 0xcc */ bool EmailInformProducts;
    /* 0xd0 */ bool EmailInformPartners;
    /* 0xd4 */ int BirthMonth;
    /* 0xd8 */ int BirthDay;
    /* 0xdc */ int BirthYear;
    /* 0xe0 */ bool TermsOfService;
    /* 0xe4 */ OnlineGender Gender;

#if defined(EA_BUILD_A124) || defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    OnlineCreateUserSettings();
#endif
    void Initialize();
};

#endif
