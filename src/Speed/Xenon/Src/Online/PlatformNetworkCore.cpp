// La capa de plataforma de NetworkCore en Xbox 360: en el EGM va en zOnline entre OnlineMutex y NetworkCore (el sitio
// de psx2/src/online/PlatformNetworkCore.cpp en la A124), 0x82574058-0x825741C0.
#include "Speed/Xenon/Src/Online/PlatformNetworkCore.hpp"

extern "C" {
void NetConnStartup(const char *params);
void NetConnShutdown(int32 flags);
}

PlatformNetworkCore::PlatformNetworkCore() {}

PlatformNetworkCore::~PlatformNetworkCore() {}

// EGM 0x825740C8: un solo NetConnStatus y los cuatro estados que cuentan como conectado (Xbox Live incluido).
bool PlatformNetworkCore::IsOnline() {
    int status = NetConnStatus('star', 0, 0);
    return status == '+isp' || status == '+onl' || status == '+xbl' || status == '-dup';
}

// En X360 no hay programa de configuracion de red que lanzar.
void PlatformNetworkCore::ExecNetGui() {}

// EGM 0x82574148: NetConnStartup con la cadena vacia y NetConnIdle.
bool PlatformNetworkCore::Init() {
    NetConnStartup("");
    NetConnIdle();
    return true;
}

// EGM 0x82574178. La .rdata conserva "PlatformNetworkCore::Destroy() has been called!!\n" (0x820A7A30) sin referencia:
// el aviso de depuracion no queda en el codigo.
void PlatformNetworkCore::Destroy() { NetConnShutdown(1); }
