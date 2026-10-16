#ifndef PSX2_ECSTASY_EVISUALTREATMENT_H
#define PSX2_ECSTASY_EVISUALTREATMENT_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/World/VisualTreatment.h"
#include "Speed/PSX2/Src/Ecstasy/EcstasyE.hpp"

// La parte de PS2 del tratamiento visual (psx2/src/ecstasy/eVisualTreatment.cpp de la A124). El layout y los metodos
// salen de los stabs (scripts/clase_stabs.py): cuatro paletas de 256 entradas detras de la base.
struct IVisualTreatmentPlat : public IVisualTreatment {
    // total size: 0x1200
    u_long128 DesaturationPalette[64]; // offset 0x200, size 0x400
    u_long128 BlackBloomPalette[64];   // offset 0x600, size 0x400
    u_long128 ColorBloomPalette[64];   // offset 0xA00, size 0x400
    unsigned int DetailPalette[256];   // offset 0xE00, size 0x400

    IVisualTreatmentPlat();
    ~IVisualTreatmentPlat();

    void RenderMWVisualLook(eView *view, eDmaChain *dma_chain, eDmaChain *tex_chain);
    void Desaturate(eRenderTarget *screenTarget, eDmaChain *dma_chain);
    void ApplyBlackBloom(eRenderTarget *screenTarget, eRenderTarget *downSample, eDmaChain *dma_chain);
    void ApplyColorBloom(eRenderTarget *screenTarget, eRenderTarget *downSample, eDmaChain *dma_chain);
    void CreateDetailMap(eRenderTarget *screenTarget, eRenderTarget *detailMap, eDmaChain *dma_chain);
    void ApplyDetailMap(eRenderTarget *screenTarget, eRenderTarget *detailMap, eDmaChain *dma_chain);
    void ApplyPulseBrightness(eRenderTarget *screenTarget, eDmaChain *dma_chain);
    unsigned int Radialblur(eRenderTarget *screenTarget, eDmaChain *dma_chain);
};

extern IVisualTreatmentPlat *pVisualTreatmentPlat;

// La curva de los visuallook (World/VisualTreatment.cpp), que los efectos muestrean para hacer sus paletas.
float GetValueFromSpline(float value, bMatrix4 *curve);

#endif
