#ifndef ONLINE_NETWORK_CORE_HPP
#define ONLINE_NETWORK_CORE_HPP

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/bWare/Inc/bList.hpp"
#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/PSX2/Src/Online/PlatformNetworkCore.hpp"
#endif

struct NetGameLinkRefT;

extern int NetworkUseLobbies;

struct NetGamePacketSyncT {
    uint32 repl;
    uint32 echo;
    int16 late;
};

typedef struct {
    struct {
        int size;
        uint16 len;
        uint8 kind;
        uint8 pad;
    } head;
    union {
        struct {
            int16 count;
        } echo;
        struct {
            int ident;
            int kind;
            int size;
            uint8 data[200];
        } strm;
        uint8 data[512];
    } body;
    struct {
        NetGamePacketSyncT sync;
        uint8 kind;
    } tail;
} NetGamePacketT;

struct DirtyAddrT {
    char strMachineAddr[64];
};

// DirtySock NetGameLink: estado del enlace. Ninguna build trae sus stabs; la A124 solo lee la latencia en +0xC
// (UIOLGameRoom::NotificationMessage la pasa a LobbyGameSessions::SetSessionLatency). Los tres campos previos llevan
// los nombres de DirtySock sin verificar.
struct NetGameLinkStatT {
    int32 isopen;
    int32 isconn;
    uint32 conntime;
    int32 late;
};

extern "C" {
NetGameLinkStatT *NetGameLinkStatus(NetGameLinkRefT *link);
}

// La base PlatformNetworkCore solo esta reconstruida en PS2 (Speed/PSX2); en PC el
// subsistema online vive todavia en los unassigned de SPEED_EXE_1_3 y su base no se
// ha levantado. Mientras, NetworkCore compila en PC sin la base (nadie la instancia:
// zOnline solo enlaza fuentes en la A124).
class NetworkCore
#ifdef EA_PLATFORM_PLAYSTATION2
    : public PlatformNetworkCore
#endif
{
  public:
    typedef int (*ProcFunc)(void *context);

    struct ProcFuncNode : public bTNode<ProcFuncNode> {
        ProcFuncNode(ProcFunc function, void *functionContext)
            : func(function), //
              context(functionContext) {}

        ProcFunc func;
        void *context;
    };

    NetworkCore();
    ~NetworkCore() override;

    static NetworkCore &Instance() { return mInstance; }
    bool Init();
    static int StaticInit();
    void Destroy();
    int Send(NetGameLinkRefT *pLinkRef, char *data, int numBytes, bool isReliable);
    bool Recv(NetGameLinkRefT *pLinkRef, NetGamePacketT &sGamePacketInbound, bool &reliable);
    void Flush(NetGameLinkRefT *const *pLinkRef, int numEntries);
    static void DoNetworkProcessing();

    static uint32 MyIPAddress();
    static char *MyIPAddressText();
    static DirtyAddrT &MyDirtyAddr();
    uint8 GetNetworkVersionNumber();

    int RegisterProcessingFunc(ProcFunc func, void *context);
    void UnregisterProcessingFunc(ProcFunc func, void *context);
    uint32 GetTime();

    bool IsInitialized();

  private:
    static void NetworkTimerFunc();
    static int NetworkThreadFunc(void *param);
    static int TestThreadStarvation();

    static uint32 myIPAddress;
    bool networkIsInitialized;
    static int msecPerThreadInterval;
    bTList<ProcFuncNode> procFuncList;
    static NetworkCore mInstance;
};

#endif
