#ifndef PSX2_ECSTASY_TEXTURE_INFO_PLAT_H
#define PSX2_ECSTASY_TEXTURE_INFO_PLAT_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/bWare/Inc/bList.hpp"

// El layout sale de los stabs de la A124 (scripts/clase_stabs.py).
struct TexturePlatImageInfo {
    // total size: 0x8
    short Width;       // offset 0x0, size 0x2
    short Height;      // offset 0x2, size 0x2
    short TBP;         // offset 0x4, size 0x2
    signed char TBW;   // offset 0x6, size 0x1
    signed char IsSwizzled; // offset 0x7, size 0x1
};

struct TextureInfoPlatInfo {
    // total size: 0x54
    short ImageVramBlocksRequired;   // offset 0x0, size 0x2
    short PaletteVramBlocksRequired; // offset 0x2, size 0x2
    signed char PixelStorageMode;    // offset 0x4, size 0x1
    signed char ClutPixelStorageMode; // offset 0x5, size 0x1
    signed char TextureFx;           // offset 0x6, size 0x1
    signed char AlphaBits;           // offset 0x7, size 0x1
    signed char AlphaFix;            // offset 0x8, size 0x1
    signed char MMag;                // offset 0x9, size 0x1
    signed char MMin;                // offset 0xA, size 0x1
    signed char MipMapL;             // offset 0xB, size 0x1
    short MipMapK;                   // offset 0xC, size 0x2
    signed char AlphaUsage;          // offset 0xE, size 0x1
    signed char Special;             // offset 0xF, size 0x1
    TexturePlatImageInfo ImageInfos[7]; // offset 0x10, size 0x38
    void *pActiveTexture;            // offset 0x48, size 0x4
    void *pStackedActiveTexture;     // offset 0x4C, size 0x4
    short PaletteVramAddress;        // offset 0x50, size 0x2
    short ImageVramAddress;          // offset 0x52, size 0x2
};

void eUnSwizzle8bitPalette(unsigned int *palette);
void eSwizzle8bitPalette(unsigned int *palette);

#endif
