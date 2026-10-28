#ifndef PC_ONLINE_PLATFORMNETWORKCORE_HPP
#define PC_ONLINE_PLATFORMNETWORKCORE_HPP

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include <types.h>

// La clase base de NetworkCore en PC (speed.exe): su vtable 0x8B68A0 tiene un solo
// slot (el destructor virtual, 0x78FC70) y ctor/dtor (0x78AFF0/0x78B000) que solo
// fijan el vptr, sin datos miembro - la misma forma que la de PS2. El destructor de
// NetworkCore (0x7929B0) deja el vptr en esta vtable al terminar, y en X360 la
// cadena "PlatformNetworkCore::Destroy() has been called!!" (NFS.exe) prueba el
// nombre de la clase compartida. Los cuerpos de PC viven en la zona de zOnline
// (0x78B010, 0x78B020, 0x78B040) y aun no tienen TU en la config de PC.
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
