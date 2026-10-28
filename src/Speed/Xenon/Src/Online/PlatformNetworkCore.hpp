#ifndef XENON_ONLINE_PLATFORMNETWORKCORE_HPP
#define XENON_ONLINE_PLATFORMNETWORKCORE_HPP

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include <types.h>

// La base de NetworkCore en Xbox 360. El EGM tiene la cadena "PlatformNetworkCore::Destroy() has been called!!" en la
// .rdata de zOnline, entre las de OnlineMutex y las de NetworkCore: el mismo sitio que psx2/src/online/
// PlatformNetworkCore.cpp en la A124. La forma de la clase es la de PS2 y PC (un destructor virtual, sin datos).
extern "C" {
int32 NetConnStatus(int32 kind, int32 data, void *buffer);
void NetConnIdle();
void RpcDone();
}

class PlatformNetworkCore {
  public:
    PlatformNetworkCore();
    virtual ~PlatformNetworkCore();

    bool IsOnline();
    void ExecNetGui();

  protected:
    bool Init();
    void Destroy();
};

#endif
