#ifndef PSX2_ECSTASY_ELIGHTPLAT_H
#define PSX2_ECSTASY_ELIGHTPLAT_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// Los datos de DMA de un material (los stabs de la A124: 0x60 bytes); AttachDmaDataListMain los manda tal cual.
struct eLightMaterialPlatInfo {
    // total size: 0x60
    unsigned int feDmaData[24]; // offset 0x0, size 0x60
};

struct eLightMaterialPlatInterface {
    // total size: 0x4 (los stabs de la A124)
    void *PlatInfo; // offset 0x0, size 0x4

    void CreatePlatInfo();
    void UpdatePlatInfo();
};

void elInitPlat();

// r77 (PS2): la capa de GameCube declara PlatConvertColor (eLightPlat.hpp:50)
// y eLight.hpp, que es Indep, la usa; el stub de PSX2 no la tenia. Mismo hueco
// que la r71e tapo para Xenon. OJO al escribir su cuerpo: la version de
// GameCube invierte los bytes (ARGB -> BGRA) porque es big-endian, y PS2 es
// LITTLE-endian, asi que no puede ser la misma. Aqui solo va la declaracion:
// hace falta para compilar, y el cuerpo llegara con el match de PS2.
unsigned int PlatConvertColor(unsigned int colour);

#endif
