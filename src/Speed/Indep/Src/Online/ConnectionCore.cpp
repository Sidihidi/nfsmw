#include "LobbyCore.hpp"
#include "VoiceCore.hpp"

#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Misc/Config.h"

extern "C" {
ConnApiRefT *ConnApiCreate(const char *sessionName, int gamePort, int maxClients,
                           ConnApiCallbackT *callback, void *userData);
void ConnApiDestroy(ConnApiRefT *connapi);
int ConnApiHost(ConnApiRefT *connapi, ConnApiUserInfoT *userInfo, int numClients, int sessionID);
int ConnApiConnect(ConnApiRefT *connapi, ConnApiUserInfoT *userInfo, int numClients, int sessionID);
int ConnApiAddClient(ConnApiRefT *connapi, ConnApiUserInfoT *userInfo);
ConnApiClientListT *ConnApiGetClientList(ConnApiRefT *connapi);
int ConnApiRemoveClient(ConnApiRefT *connapi, const char *clientName, int clientIndex);
int ConnApiOnline(ConnApiRefT *connapi, const char *name, DirtyAddrT *dirtyAddr);
void ConnApiDisconnect(ConnApiRefT *connapi);
int ConnApiStatus2(ConnApiRefT *connapi, int selector, void *buffer, int bufferSize);
#ifdef EA_PLATFORM_XENON
void NetConnSleep(int milliseconds); // ResetSession_HaveMutex (en el EGM, el Sleep del XDK: 0x828501A8)
#endif
int ConnApiControl(ConnApiRefT *connapi, int control, int value, int value2, void *pValue);
}

#ifdef EA_PLATFORM_XENON
// Xbox 360: el entero de .data 0x82A81DA4 (justo tras OnlineEnabled); solo lo leen ConnectionCore::Init y
// LobbyCore::Init, que con el alargan los plazos de conexion (probablemente una opcion de depuracion). Sin nombre en
// ningun binario y sin definir en el arbol.
extern int XenonUnknown82A81DA4;
#endif

ConnectionCore *TheConnectionCore;

ConnectionCore::ConnectionCore() {
    connapi = nullptr;
#ifndef EA_PLATFORM_WIN32 // PC (ConnectionCore::Instance, 0x79F090, lo expande): no lo asigna
    TheConnectionCore = this;
#endif
    connapiCallback = nullptr;
    callbackContext = nullptr;
    numConnectedPlayers = 0;
    isOnline = false;
}

ConnectionCore::~ConnectionCore() { Reset(); }

ConnectionCore &ConnectionCore::Instance() {
    static ConnectionCore theConnectionCore;
    return theConnectionCore;
}

void ConnectionCore::Init(int maxNumPlayers, ConnApiCallbackT *cbfunc, void *context) {
    char sessionname[64];

    networkMutex.Lock("ConnectionCore::Init");
    if (connapi) {
        networkMutex.Unlock("ConnectionCore::Init");
        return;
    }

    bSPrintf(sessionname, "%s-%s-%s", OLGetProductName(), OLGetPlatform(), OLGetProductYear());
    connapiCallback = cbfunc;
    callbackContext = context;
    numConnectedPlayers = 0;
    isOnline = false;
    VoiceCore::mInstance->Startup();
    VoiceCore::mInstance->SetHeadsetPort(FEDatabase->PlayerJoyports[0]);
    connapi = ConnApiCreate(sessionname, 3658, maxNumPlayers, ConnApiCallback, nullptr);
#ifdef EA_PLATFORM_XENON // EGM 0x8258221C
    ConnApiControl(connapi, 'type', 1, 0, nullptr);
    if (XenonUnknown82A81DA4) {
        ConnApiControl(connapi, 'time', 3600000, 0, nullptr);
    }
    ConnApiControl(connapi, 'mwid', 243, 0, nullptr);
#else
    ConnApiControl(connapi, 'mwid', 100, 0, nullptr);
#endif
    ConnApiControl(connapi, 'mout', 16, 0, nullptr);
    networkMutex.Unlock("ConnectionCore::Init");
}

void ConnectionCore::Reset() {
    networkMutex.Lock("ConnectionCore::Reset");
    if (connapi) {
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON) // PC (0x79E100) y Xbox 360 (0x825822D0): el "CE" del lobby antes de destruir connapi
        LobbyUsers::Instance().SetWin32UnknownCE();
#endif
        ConnApiDestroy(connapi);
    }
    connapi = nullptr;
    connapiCallback = nullptr;
    callbackContext = nullptr;
    numConnectedPlayers = 0;
    isOnline = false;
#ifndef EA_PLATFORM_WIN32
    VoiceCore::mInstance->Shutdown();
#endif
    networkMutex.Unlock("ConnectionCore::Reset");
}

void ConnectionCore::SetCallback(ConnApiCallbackT *cbfunc, void *context) {
    bool mutexLocked = false;
    if (connapi) {
        networkMutex.Lock("ConnectionCore::SetCallback");
        mutexLocked = true;
    }
    connapiCallback = cbfunc;
    callbackContext = context;
    if (mutexLocked == true) {
        networkMutex.Unlock("ConnectionCore::SetCallback");
    }
}

void ConnectionCore::HostSession(int sessionID, int connectionType) {
    ConnApiUserInfoT myUserInfo;

    networkMutex.Lock("ConnectionCore::HostSession");
    MaybeGoOnline();
    if (SkipFE) {
        myUserInfo.uAddr = NetworkCore::MyIPAddress();
        myUserInfo.uLocalAddr = myUserInfo.uAddr;
        bMemCpy(&myUserInfo.DirtyAddr, &NetworkCore::MyDirtyAddr(), sizeof(myUserInfo.DirtyAddr));
        bStrCpy(myUserInfo.strName, FEDatabase->OnlineSettings.GetLobbyPersona());
    } else {
        BuildUserInfo(myUserInfo, *LobbyUsers::Instance().GetMyUserRecord());
    }
    ConnApiControl(connapi, 'type', connectionType, 0, nullptr);
    ConnApiHost(connapi, &myUserInfo, 1, sessionID);
    networkMutex.Unlock("ConnectionCore::HostSession");
}

void ConnectionCore::JoinSession(ConnApiUserInfoT &hostInfo, int sessionID, int connectionType) {
    ConnApiUserInfoT userInfo[2];

    networkMutex.Lock("ConnectionCore::JoinSession");
    MaybeGoOnline();
    bMemCpy(userInfo, &hostInfo, sizeof(hostInfo));
    if (SkipFE) {
        userInfo[1].uAddr = NetworkCore::MyIPAddress();
        userInfo[1].uLocalAddr = userInfo[1].uAddr;
        bMemCpy(&userInfo[1].DirtyAddr, &NetworkCore::MyDirtyAddr(), sizeof(userInfo[1].DirtyAddr));
        bStrCpy(userInfo[1].strName, FEDatabase->OnlineSettings.GetLobbyPersona());
    } else {
        BuildUserInfo(userInfo[1], *LobbyUsers::Instance().GetMyUserRecord());
    }
    ConnApiControl(connapi, 'type', connectionType, 0, nullptr);
    ConnApiConnect(connapi, userInfo, 2, sessionID);
    networkMutex.Unlock("ConnectionCore::JoinSession");
}

void ConnectionCore::JoinSession(LobbyApiPlayerT &hostInfo, int sessionID, void *strSess,
                                 int connectionType) {
    ConnApiUserInfoT userInfo[2];

    networkMutex.Lock("ConnectionCore::JoinSession");
    MaybeGoOnline();
    BuildUserInfo(userInfo[0], hostInfo);
    LobbyApiUserT *myUser = LobbyUsers::Instance().GetMyUserRecord();
    BuildUserInfo(userInfo[1], *myUser);
    ConnApiControl(connapi, 'type', connectionType, 0, nullptr);
    ConnApiControl(connapi, 'sess', 0, 0, strSess);
    ConnApiConnect(connapi, userInfo, 2, sessionID);
    networkMutex.Unlock("ConnectionCore::JoinSession");
}

void ConnectionCore::LeaveSession() {
#ifdef EA_PLATFORM_WIN32 // PC (0x790A60): sin clientes no coge el mutex
    if (!connapi || ConnApiGetClientList(connapi)->iNumClients <= 0) {
        return;
    }
#endif
    networkMutex.Lock("ConnectionCore::LeaveSession");
    ResetSession_HaveMutex();
    networkMutex.Unlock("ConnectionCore::LeaveSession");
}

void ConnectionCore::AddPlayer(ConnApiUserInfoT &userInfo) {
    networkMutex.Lock("ConnectionCore::AddPlayer");
    if (bStrCmp(userInfo.strName, FEDatabase->OnlineSettings.GetLobbyPersona()) != 0) {
        ConnApiAddClient(connapi, &userInfo);
    }
    networkMutex.Unlock("ConnectionCore::AddPlayer");
}

void ConnectionCore::AddPlayer(LobbyApiUserT &userInfo) {
    ConnApiUserInfoT realUserInfo;

    networkMutex.Lock("ConnectionCore::AddPlayer");
    BuildUserInfo(realUserInfo, userInfo);
    if (bStrCmp(realUserInfo.strName, FEDatabase->OnlineSettings.GetLobbyPersona()) != 0) {
        ConnApiAddClient(connapi, &realUserInfo);
    }
    networkMutex.Unlock("ConnectionCore::AddPlayer");
}

void ConnectionCore::AddPlayer(LobbyApiPlayerT &userInfo) {
    networkMutex.Lock("ConnectionCore::AddPlayer");
    AddPlayer_HaveMutex(userInfo);
    networkMutex.Unlock("ConnectionCore::AddPlayer");
}

void ConnectionCore::UpdatePlayers(const LobbyApiPlayT &game) {
    bool madeChanges;

    networkMutex.Lock("ConnectionCore::UpdatePlayers");
    ConnApiClientListT *clientList = ConnApiGetClientList(connapi);
    if (!clientList) {
        networkMutex.Unlock("ConnectionCore::UpdatePlayers");
        return;
    }
    if (game.iIdent < 0 || game.iCount == 0) {
        ResetSession_HaveMutex();
        networkMutex.Unlock("ConnectionCore::UpdatePlayers");
        return;
    }

    madeChanges = false;
    for (int i = 0; i < clientList->iNumClients; i++) {
        bool foundPlayer = false;
        for (int j = 0; j < game.iCount; j++) {
            if (bStrCmp(clientList->Clients[i].UserInfo.strName, game.aOpponents[j].strPers) == 0) {
                foundPlayer = true;
                break;
            }
        }
        if (!foundPlayer) {
            madeChanges = true;
#ifndef EA_PLATFORM_WIN32 // PC (UpdatePlayers 0x7989E0): sin VoiceCore y con el "CE" del lobby
            VoiceCore::mInstance->RemovePlayer(clientList->Clients[i].UserInfo.strName);
#endif
            ConnApiRemoveClient(connapi, nullptr, i);
            NetConnIdle();
            i--;
            clientList = ConnApiGetClientList(connapi);
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON) // Xbox 360 tambien (EGM 0x8258323C)
            LobbyUsers::Instance().SetWin32UnknownCE();
#endif
        }
    }

    for (int i = 0; i < game.iCount; i++) {
        bool addPlayer = true;
        for (int j = 0; j < clientList->iNumClients; j++) {
            if (bStrCmp(clientList->Clients[j].UserInfo.strName, game.aOpponents[i].strPers) == 0) {
                addPlayer = false;
                break;
            }
        }
        if (addPlayer) {
            madeChanges = true;
            AddPlayer_HaveMutex(game.aOpponents[i]);
        }
    }

    if (madeChanges == true) {
        UpdateNumConnectedPlayers();
    }
    networkMutex.Unlock("ConnectionCore::UpdatePlayers");
}

#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON) // PC (0x79E160) y Xbox 360 (0x825833B0), sin llamantes directos: fuera de linea
void ConnectionCore::RemovePlayer(char *playerName) {
#else
inline void ConnectionCore::RemovePlayer(char *playerName) {
#endif
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON) // PC (0x79E160): quitarme a mi o al primer cliente cierra la sesion; si no, "CE" al lobby
    if (!connapi) {
        return;
    }
    networkMutex.Lock("ConnectionCore::RemovePlayer");
    ConnApiClientListT *clientList = ConnApiGetClientList(connapi);
    if (!clientList) {
        networkMutex.Unlock("ConnectionCore::RemovePlayer");
        return;
    }
    // Dos llamadas (MSVC las junta en una): con `||` expande ResetSession_HaveMutex y el original la llama.
    if (bStrCmp(playerName, FEDatabase->OnlineSettings.GetLobbyPersona()) == 0) {
        ResetSession_HaveMutex();
    } else if (bStrCmp(playerName, clientList->Clients[0].UserInfo.strName) == 0) {
        ResetSession_HaveMutex();
    } else {
#ifdef EA_PLATFORM_XENON // EGM 0x825834C4: y lo saca del chat de voz
        VoiceCore::mInstance->RemovePlayer(playerName);
#endif
        ConnApiRemoveClient(connapi, playerName, 0);
        LobbyUsers::Instance().SetWin32UnknownCE();
        UpdateNumConnectedPlayers();
    }
    networkMutex.Unlock("ConnectionCore::RemovePlayer");
#else
    networkMutex.Lock("ConnectionCore::RemovePlayer");
    ConnApiClientListT *clientList = ConnApiGetClientList(connapi);
    if (clientList && bStrCmp(playerName, FEDatabase->OnlineSettings.GetLobbyPersona()) != 0) {
        for (int i = 0; i < clientList->iNumClients; i++) {
            if (bStrCmp(clientList->Clients[i].UserInfo.strName, playerName) == 0) {
                VoiceCore::mInstance->RemovePlayer(clientList->Clients[i].UserInfo.strName);
                ConnApiRemoveClient(connapi, nullptr, i);
                UpdateNumConnectedPlayers();
                break;
            }
        }
    }
    networkMutex.Unlock("ConnectionCore::RemovePlayer");
#endif
}

#ifdef EA_PLATFORM_XENON
// Xbox 360 (EGM 0x82583518, sin llamantes directos): fuera de linea; quitar al primer cliente (o a mi) cierra la sesion
void ConnectionCore::RemovePlayer(int index) {
    if (!connapi) {
        return;
    }
    networkMutex.Lock("ConnectionCore::RemovePlayer");
    ConnApiClientListT *clientList = ConnApiGetClientList(connapi);
    if (!clientList) {
        networkMutex.Unlock("ConnectionCore::RemovePlayer");
        return;
    }
    if (index == 0) {
        ResetSession_HaveMutex();
    } else if (bStrCmp(clientList->Clients[index].UserInfo.strName, FEDatabase->OnlineSettings.GetLobbyPersona()) == 0) {
        ResetSession_HaveMutex();
    } else {
        VoiceCore::mInstance->RemovePlayer(clientList->Clients[index].UserInfo.strName);
        ConnApiRemoveClient(connapi, nullptr, index);
        UpdateNumConnectedPlayers();
    }
    networkMutex.Unlock("ConnectionCore::RemovePlayer");
}
#else
inline void ConnectionCore::RemovePlayer(int index) {
    networkMutex.Lock("ConnectionCore::RemovePlayer");
    ConnApiClientListT *clientList = ConnApiGetClientList(connapi);
    if (clientList && index >= 0 && index < clientList->iNumClients &&
        bStrCmp(clientList->Clients[index].UserInfo.strName,
                FEDatabase->OnlineSettings.GetLobbyPersona()) != 0) {
        VoiceCore::mInstance->RemovePlayer(clientList->Clients[index].UserInfo.strName);
        ConnApiRemoveClient(connapi, nullptr, index);
        UpdateNumConnectedPlayers();
    }
    networkMutex.Unlock("ConnectionCore::RemovePlayer");
}
#endif

#ifdef EA_PLATFORM_WIN32
// PC (0x78C240): sin connapi no coge el mutex (como GetPlayer(int)); los dos ceros salen por el mismo sitio
int ConnectionCore::GetNumPlayers() {
    if (connapi) {
        networkMutex.Lock("ConnectionCore::GetNumPlayers");
        ConnApiClientListT *clientList = ConnApiGetClientList(connapi);
        if (!clientList) {
            networkMutex.Unlock("ConnectionCore::GetNumPlayers");
        } else {
            int tmp = clientList->iNumClients;
            networkMutex.Unlock("ConnectionCore::GetNumPlayers");
            return tmp;
        }
    }
    return 0;
}
#else
int ConnectionCore::GetNumPlayers() {
    int tmp;

    networkMutex.Lock("ConnectionCore::GetNumPlayers");
    ConnApiClientListT *clientList = ConnApiGetClientList(connapi);
    if (!clientList) {
        networkMutex.Unlock("ConnectionCore::GetNumPlayers");
        tmp = 0;
    } else {
        tmp = clientList->iNumClients;
        networkMutex.Unlock("ConnectionCore::GetNumPlayers");
    }
    return tmp;
}
#endif

int ConnectionCore::GetNumConnectedPlayers() { return numConnectedPlayers; }

inline ConnApiClientT *ConnectionCore::GetPlayer(char *name) {
#ifdef EA_PLATFORM_WIN32 // PC (0x78C2B0, la llama LobbyUsers::SetWin32UnknownCE): sin clientes sale antes
    networkMutex.Lock("ConnectionCore::GetPlayer");
    ConnApiClientListT *clientList = ConnApiGetClientList(connapi);
    if (!clientList || clientList->iNumClients == 0) {
        networkMutex.Unlock("ConnectionCore::GetPlayer");
        return nullptr;
    }
    ConnApiClientT *player = nullptr;
    char *realName = name ? name : FEDatabase->OnlineSettings.GetLobbyPersona();
    for (int i = 0; i < clientList->iNumClients; i++) {
        if (bStrCmp(clientList->Clients[i].UserInfo.strName, realName) == 0) {
            player = &clientList->Clients[i];
            break;
        }
    }
    networkMutex.Unlock("ConnectionCore::GetPlayer");
    return player;
#else
    ConnApiClientT *player = nullptr;
    char *realName = name;

    networkMutex.Lock("ConnectionCore::GetPlayer");
    ConnApiClientListT *clientList = ConnApiGetClientList(connapi);
    if (!realName) {
        realName = FEDatabase->OnlineSettings.GetLobbyPersona();
    }
    if (clientList) {
        for (int i = 0; i < clientList->iNumClients; i++) {
            if (bStrCmp(clientList->Clients[i].UserInfo.strName, realName) == 0) {
                player = &clientList->Clients[i];
                break;
            }
        }
    }
    networkMutex.Unlock("ConnectionCore::GetPlayer");
    return player;
#endif
}

ConnApiClientT *ConnectionCore::GetPlayer(int index) {
#ifdef EA_PLATFORM_WIN32 // PC (0x78C380): sin connapi sale antes de coger el mutex
    if (!connapi) {
        return nullptr;
    }
#endif
    networkMutex.Lock("ConnectionCore::GetPlayer");
    ConnApiClientListT *clientList = ConnApiGetClientList(connapi);
    if (!clientList || clientList->iNumClients == 0 || index > clientList->iNumClients) {
        networkMutex.Unlock("ConnectionCore::GetPlayer");
        return nullptr;
    }
    ConnApiClientT *player = &clientList->Clients[index];
    networkMutex.Unlock("ConnectionCore::GetPlayer");
    return player;
}

void ConnectionCore::BuildUserInfo(ConnApiUserInfoT &dest, const LobbyApiUserT &src) {
    dest.uAddr = src.addr;
    dest.uLocalAddr = src.uLocalAddr;
    bMemCpy(&dest.DirtyAddr, &src.MachineAddr, sizeof(dest.DirtyAddr));
    bStrCpy(dest.strName, src.name);
}

void ConnectionCore::BuildUserInfo(ConnApiUserInfoT &dest, const LobbyApiPlayerT &src) {
    dest.uAddr = src.uAddr;
    dest.uLocalAddr = src.uLocalAddr;
    bStrCpy(dest.DirtyAddr.strMachineAddr, src.strMachineAddr);
    bStrCpy(dest.strName, src.strPers);
}

void ConnectionCore::UpdateNumConnectedPlayers() {
    numConnectedPlayers = 0;
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON) // PC (0x78C490) y Xbox 360 (0x82583C00): NetConnIdle y solo los clientes que hay
    NetConnIdle();
#endif
    ConnApiClientListT *clientList = ConnApiGetClientList(connapi);
    if (clientList) {
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
        for (int i = 0; i < clientList->iNumClients; i++) {
#else
        for (int i = 0; i < clientList->iMaxClients; i++) {
#endif
            if (clientList->Clients[i].GameInfo.eStatus == CONNAPI_STATUS_ACTV) {
                numConnectedPlayers++;
            }
        }
    }
}

inline bool ConnectionCore::IsSessionStarted() {
    ConnApiClientListT *clientList = ConnApiGetClientList(connapi);
    return clientList && clientList->iNumClients > 0;
}

void ConnectionCore::MaybeGoOnline() {
    if (!isOnline) {
        if (SkipFE) {
            ConnApiOnline(connapi, FEDatabase->OnlineSettings.GetLobbyPersona(),
                          &NetworkCore::MyDirtyAddr());
        } else {
            LobbyApiUserT *me = LobbyUsers::Instance().GetMyUserRecord();
            ConnApiOnline(connapi, me->name, &me->MachineAddr);
        }
        isOnline = true;
    }
}

void ConnectionCore::AddPlayer_HaveMutex(const LobbyApiPlayerT &userInfo) {
    ConnApiUserInfoT realUserInfo;

    BuildUserInfo(realUserInfo, userInfo);
    if (bStrCmp(realUserInfo.strName, FEDatabase->OnlineSettings.GetLobbyPersona()) != 0) {
        ConnApiAddClient(connapi, &realUserInfo);
    }
}

void ConnectionCore::ResetSession_HaveMutex() {
    if (connapi) {
        ConnApiClientListT *clientList = ConnApiGetClientList(connapi);
        if (clientList && clientList->iNumClients != 0) {
            ConnApiDisconnect(connapi);
#ifdef EA_PLATFORM_XENON // EGM 0x82583E44: espera a que 'ingm' deje de valer 1
            while (ConnApiStatus2(connapi, 'ingm', nullptr, 0) == 1) {
                NetConnIdle();
                NetConnSleep(8);
            }
#endif
            numConnectedPlayers = 0;
#ifndef EA_PLATFORM_WIN32 // PC (LeaveSession 0x790A60): sin VoiceCore
            VoiceCore::mInstance->RemoveAllPlayers();
#endif
        }
    }
}

void ConnectionCore::ConnApiCallback(ConnApiRefT *connapi, ConnApiCbInfoT *cbinfo, void *context) {
    ConnApiClientListT *clientList = ConnApiGetClientList(connapi);
    ConnectionCore &conncore = Instance();
    ConnApiClientT &client = clientList->Clients[cbinfo->iClientId];

#ifdef EA_PLATFORM_XENON // EGM 0x82583EF4: en los eventos de juego recuenta y manda el "CE" al lobby
    if (cbinfo->eType == CONNAPI_CBTYPE_GAMEEVENT) {
        conncore.UpdateNumConnectedPlayers();
        LobbyUsers::Instance().SetWin32UnknownCE();
    }
#else
    if (cbinfo->eType == CONNAPI_CBTYPE_GAMEEVENT) {
        if (cbinfo->eNewStatus == CONNAPI_STATUS_ACTV) {
            conncore.numConnectedPlayers++;
        } else if (cbinfo->eNewStatus == CONNAPI_STATUS_DISC &&
                   static_cast<unsigned int>(cbinfo->eOldStatus - CONNAPI_STATUS_ACTV) < 2) {
            conncore.numConnectedPlayers--;
        }
    }
#endif

    if (!SkipFE && cbinfo->eType == CONNAPI_CBTYPE_VOIPEVENT) {
        if (cbinfo->eNewStatus == CONNAPI_STATUS_ACTV) {
#ifdef EA_PLATFORM_XENON // EGM 0x82583EC0: el canal de voz del cliente, sin mirar antes IsInVOIPChat
            VoiceCore::mInstance->AddPlayer(client.iVoipConnId, client.UserInfo.strName);
#else
            if (!VoiceCore::mInstance->IsInVOIPChat(client.UserInfo.strName, nullptr)) {
                VoiceCore::mInstance->AddPlayer(client.UserInfo.strName);
            }
#endif
#ifdef EA_PLATFORM_XENON // EGM 0x82583F74: dos comparaciones
        } else if (cbinfo->eNewStatus == CONNAPI_STATUS_DISC &&
                   (cbinfo->eOldStatus == CONNAPI_STATUS_ACTV || cbinfo->eOldStatus == CONNAPI_STATUS_CLSE)) {
#else
        } else if (cbinfo->eNewStatus == CONNAPI_STATUS_DISC &&
                   static_cast<unsigned int>(cbinfo->eOldStatus - CONNAPI_STATUS_ACTV) < 2) {
#endif
            VoiceCore::mInstance->RemovePlayer(client.UserInfo.strName);
        }
    }

    if (cbinfo->eType == CONNAPI_CBTYPE_SESSEVENT && cbinfo->eNewStatus == CONNAPI_STATUS_ACTV) {
#ifdef EA_PLATFORM_XENON // EGM 0x82583FB0: tambien el 'nonc' de la sesion, en "PLATPARAMS"
        char strSess[128];
        bMemSet(strSess, 0, sizeof(strSess));
        ConnApiStatus2(conncore.connapi, 'sess', strSess, sizeof(strSess));
        char nonce[16];
        bMemSet(nonce, 0, sizeof(nonce));
        ConnApiStatus2(conncore.connapi, 'nonc', nonce, sizeof(nonce));
        char buf[256];
        bMemSet(buf, 0, sizeof(buf));
        TagFieldSetString(buf, sizeof(buf), "SESS", strSess);
        TagFieldSetString(buf, sizeof(buf), "PLATPARAMS", nonce);
#else
        char strSess[128];
        ConnApiStatus2(conncore.connapi, 'sess', strSess, sizeof(strSess));
        char buf[144] = "";
        TagFieldSetString(buf, sizeof(buf), "SESS", strSess);
#endif
        lobbyMutex.Lock("ConnectionCore::ConnApiCallback");
        LobbyCore::Instance().QueueCommand('gset', buf, LobbyCore::DefaultCB, nullptr, nullptr,
                                           nullptr, false);
        lobbyMutex.Unlock("ConnectionCore::ConnApiCallback");
        LobbyGameSessions::Instance().SettingsHaveChanged();
    }

    if (conncore.connapiCallback) {
        conncore.connapiCallback(connapi, cbinfo, conncore.callbackContext);
    }
}

inline void ConnectionCore::FrontEndCB(ConnApiRefT *connapi, ConnApiCbInfoT *cbinfo,
                                       void *context) {}
