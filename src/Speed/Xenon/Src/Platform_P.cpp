// Platform_P.cpp: la capa de plataforma de Xbox 360 (el nombre es el del PDB del prototipo de ProStreet para Xbox 360,
// speed\xenon\src\platform_p.cpp, con las mismas funciones vacias en el mismo orden que el EGM: CanRumble,
// GetModelNumVertices, bFunkCallASync, bIsThereACDInTheDrive, eClampTopLeft 0x82494D38, eWaitUntilRenderingDone
// 0x82494D40, eForceBackgroundColour...). Aqui van las que llama la fuente comun y en el EGM no dejan llamada porque
// LTCG expande su cuerpo vacio (ServicePlatform en el MainLoop, EnableInterrupts, InitDisplaySystem...). Las demas
// del bloque (0x82494CE8-0x824952A0: InitPlatform, MyFileMalloc 0x82495030, MyFileFree 0x82495048, los avisos de disco)
// quedan por escribir.
#include "Speed/Indep/Src/Misc/Platform.h"
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Ecstasy/EcstasyE.hpp"
#include "Speed/Xenon/Src/Ecstasy/TextureInfoPlat.hpp"

// El EGM devuelve el segundo argumento (`mr r3, r4`); GameCube devuelve false.
int eClampTopLeft(bool bOnOff, int nUnused) {
    return nUnused;
}

void eForceBackgroundColour(unsigned char r, unsigned char g, unsigned char b, float time, const char *reason) {}

TextureInfo *eGetRenderTargetTextureInfo(int32 name_hash) {
    return 0;
}

void EnableInterrupts() {}

void InitDisplaySystem() {}

void FinishedRenderingFEngLayer() {}

void eUnSwizzle8bitPalette(unsigned int *palette) {}

void eSwizzle8bitPalette(unsigned int *palette) {}

void ServicePlatform() {}
