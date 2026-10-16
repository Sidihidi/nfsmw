#ifndef PC_ECSTASY_TEXTURE_INFO_PLAT_H
#define PC_ECSTASY_TEXTURE_INFO_PLAT_H

#include "Speed/Indep/bWare/Inc/bList.hpp"

struct IDirect3DTexture9;

// El registro de PC de BCHUNK_SPEED_TEXTURE_PACK_PLAT_INFO_TABLE (0x33310005) mide 0x20: lo dicen el exe (Texture.cpp
// divide el tamano del chunk entre 0x20 y TextureLoadedStreamingEntryCallback resta 0x7C + 0x20) y los datos de la
// instalacion de PC (cada 0x33310005 lleva un registro de 0x20 por cada TextureInfo de 0x7C de 0x33310004).
// total size: 0x20
struct TextureInfoPlatInfo : public bTNode<TextureInfoPlatInfo> {
    unsigned int Unknown8;                 // offset 0x8, size 0x4 (1 en los datos de PC)
    unsigned int UnknownC;                 // offset 0xC, size 0x4 (5 en los datos de PC)
    unsigned int Unknown10;                // offset 0x10, size 0x4 (6 en los datos de PC)
    unsigned int Format;                   // offset 0x14, size 0x4 (D3DFORMAT: 'DXT1', 'DXT3', 'DXT5', 0x15, 0x29...)
    struct IDirect3DTexture9 *pD3DTexture; // offset 0x18, size 0x4 (lo crea TextureInfoPlatInterface::Init)
    unsigned int Unknown1C;                // offset 0x1C, size 0x4
};

// En PC son funciones vacias (un `ret`): la paleta no se baraja.
void eUnSwizzle8bitPalette(unsigned int *palette);
void eSwizzle8bitPalette(unsigned int *palette);

#endif
