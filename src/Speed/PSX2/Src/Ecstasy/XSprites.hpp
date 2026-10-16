#ifndef PSX2_ECSTASY_XSPRITES_H
#define PSX2_ECSTASY_XSPRITES_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/PSX2/Src/xSparks.h"
#include "Speed/Indep/bWare/Inc/bMath.hpp"

struct TextureInfo;
class eView;

// total size: 0x30
struct SpriteDef {
    TextureInfo *texture_info; // offset 0x0, size 0x4
    unsigned int color;        // offset 0x4, size 0x4
    float width;               // offset 0x8, size 0x4
    bVector3 startPos;         // offset 0x10, size 0x10
    bVector3 EndPosPos;        // offset 0x20, size 0x10
};

// total size: 0x3850
struct XSpriteManager {
    // Vacio, pero declarado: la A124 emite _$_14XSpriteManager (solo el delete del destructor) para el global.
    ~XSpriteManager() {}

    void AddSpark(const NGParticle &particle, TextureInfo *CurrentTexture);
    void RenderAll(eView *view);

    unsigned int position;        // offset 0x0, size 0x4
    SpriteDef XSpriteBuffer[300]; // offset 0x10, size 0x3840
};

extern XSpriteManager NGSpriteManager;

#endif
