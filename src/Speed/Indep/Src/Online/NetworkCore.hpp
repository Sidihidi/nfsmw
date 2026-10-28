#ifndef ONLINE_NETWORK_CORE_HPP
#define ONLINE_NETWORK_CORE_HPP

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/bWare/Inc/bList.hpp"
#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/PSX2/Src/Online/PlatformNetworkCore.hpp"
#elif defined(EA_PLATFORM_WIN32)
#include "Speed/PC/Src/Online/PlatformNetworkCore.hpp"
#elif defined(EA_PLATFORM_XENON)
#include "Speed/Xenon/Src/Online/PlatformNetworkCore.hpp"
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

// La base PlatformNetworkCore existe tambien en PC: el ctor de NetworkCore en
// speed.exe (0x792990) fija el vptr 0x8B6CFC y su dtor (0x7929B0) lo deja en la
// vtable de la base (0x8B68A0) tras destruir procFuncList. En GC no hay capa
// online y la clase compila sin base, como hasta ahora.
class NetworkCore
#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
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
#ifdef EA_PLATFORM_XENON
    int XenonUnknown825748B8(); // NetworkCore.cpp
#endif

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
#ifdef EA_PLATFORM_XENON
    // EGM: el ctor (0x825741C8) los pone a cero y la funcion de estado de Xenon (0x825748B8), que pregunta a
    // NetConnStatus por 'plug', 'onln' y 'conn' y mira '-dup', los lee y los escribe. Ningun binario da los nombres:
    // probablemente el cable, la sesion de Xbox Live y la conexion.
    bool xenon_unknown_0x10;
    bool xenon_unknown_0x11;
    bool xenon_unknown_0x12;
#endif
    static NetworkCore mInstance;
};

#endif
