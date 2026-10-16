#include "Speed/Indep/Src/Misc/TestHooks/JuiceHooks/JuiceHooks.h"
#include "Speed/PSX2/Src/Online/PS2Isp.hpp"

extern "C" void NetConnStartup(const char *params);
extern "C" int NetConnQuery(const char *name, NetConfigRecT *configs, int count);
extern "C" int NetConnConnect(NetConfigRecT *configs, int flags);
extern "C" int printf(const char *format, ...);
extern "C" char *NetConnMAC();
extern "C" ProtoAriesRefT *ProtoAriesCreate(int memSize);
extern "C" long SocketInTextGetAddr(const char *host);
extern "C" int ProtoAriesConnect(ProtoAriesRefT *ref, int flags, long address, int port);
extern "C" int ProtoAriesPeek(ProtoAriesRefT *ref, int *kind, int *code, char **data);
extern "C" int ProtoAriesRecv(ProtoAriesRefT *ref, int *kind, int *channel, char *buf, int size);
extern "C" int ProtoAriesSend(ProtoAriesRefT *ref, int length, int channel, char *buf, int size);
extern "C" long ProtoAriesStatus(void *ref, int selector);
extern "C" int ProtoAriesUnconnect(void *ref);
extern "C" void ProtoAriesDestroy(void *ref);
extern "C" int NetConnStatus(int kind, int data, void *buffer);
extern "C" void NetConnShutdown(int flags);
extern "C" void NetConnIdle();
extern "C" char *SocketInAddrGetText(long address);

namespace Juice {

IExtension *IExtension::sCurrentExtension;
JuiceDirtyNet *JuiceDirtyNet::mInstance;

JuiceDirtyNet *JuiceDirtyNet::Instance() {
    static JuiceDirtyNet dirty;
    return &dirty;
}

JuiceDirtyNet::JuiceDirtyNet() {}

JuiceDirtyNet::~JuiceDirtyNet() {}

int JuiceDirtyNet::Connect(const char *host, int port) {
    mAries = ProtoAriesCreate(0x1000);
    return ProtoAriesConnect(mAries, 0, SocketInTextGetAddr(host), port);
}

int JuiceDirtyNet::Initialize() {
    NetConfigRecT configs[4];
    int connected = 1;
    NetConnStartup("-nosecure");
    printf("Loading network configuration");
    int cnt = NetConnQuery("mc0:", configs, 4);
    if (cnt == 0) {
        printf("\nNo network configurations found on mc0\n");
        GameHook::Instance()->DisableJuice();
        return 0;
    } else if (cnt < 0) {
        printf("\nError occured loading network configurations\n");
        GameHook::Instance()->DisableJuice();
        return 0;
    }
    printf("\nConfiguration Loaded Starting Juice\n");
    int error = NetConnConnect(configs, 0);
    printf("\nNetConnConnect returned: %d\n", error);
    if (error < 0) {
        connected = 0;
        GameHook::Instance()->DisableJuice();
    }
    return connected;
}

char *JuiceDirtyNet::GetMac() {
    return NetConnMAC();
}

int JuiceDirtyNet::IsConnected() {
    PeekHdr(0);
    return ProtoAriesStatus(mAries, 0x73746174) == 3;
}

int JuiceDirtyNet::Disconnect() {
    return ProtoAriesUnconnect(mAries);
}

int JuiceDirtyNet::PeekHdr(char *data) {
    int kind = 0;
    int code = 0;
    int result = ProtoAriesPeek(mAries, &kind, &code, &data);
    return result == 0 ? 1 : result;
}

int JuiceDirtyNet::Recv(char *buf, int *channel, int size) {
    int dummyKind = 0;
    return ProtoAriesRecv(mAries, &dummyKind, channel, buf, size);
}

int JuiceDirtyNet::Send(int length, int channel, char *sendBuffer) {
    int ret_val;
    unsigned int timeDiff = 0;
    unsigned int startTime = GameHook::Instance()->GetCurrentSystemTime();
    do {
        NetConnIdle();
        ret_val = ProtoAriesSend(mAries, length, channel, sendBuffer, length);
        if (ret_val < 0) {
            NetConnIdle();
            IExtension::sCurrentExtension->ThreadYield(5);
            NetConnIdle();
        }
        NetConnIdle();
        timeDiff += GameHook::Instance()->GetTimeElapsed(startTime);
    } while (ret_val < 0 || timeDiff > 3000);
    return ret_val;
}

void JuiceDirtyNet::TearDown() {
    NetConnShutdown(0);
}

int JuiceDirtyNet::IsEnabled() {
    return NetConnStatus(0x6f6e6c6e, 0, 0);
}

void JuiceDirtyNet::MaintainConn() {
    NetConnIdle();
}

void JuiceDirtyNet::FreeEverything() {
    ProtoAriesDestroy(mAries);
}

char *JuiceDirtyNet::GetLocalIpAddress() {
    long address = 0;
    unsigned int elapsed = 0;
    unsigned int startTime = GameHook::Instance()->GetCurrentSystemTime();
    if (NetConnStatus(0x706c7567, 0, 0) != 0) {
        while (address == 0) {
            address = NetConnStatus(0x61646472, 0, 0);
            if (NetConnStatus(0x706c7567, 0, 0) == 0 || elapsed > 3000) {
                break;
            }
            elapsed += GameHook::Instance()->GetTimeElapsed(startTime);
        }
    }
    return SocketInAddrGetText(address);
}

}
