#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Ecstasy/EcstasyData.hpp"
#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/Indep/Src/Misc/ResourceLoader.hpp"
#include "Speed/Indep/bWare/Inc/bChunk.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/PSX2/Src/Ecstasy/TextureInfoPlat.hpp"
#include "Speed/PSX2/Src/Ecstasy/eStripData.hpp"

// El logaritmo en base 2 por el exponente del flotante: se eleva f a una potencia que deje el exponente cerca de 127
// para no perder decimales.
float Log2(float f) {
    int n = *reinterpret_cast<int *>(&f);
    int nLog2 = ((n >> 23) & 0xFF) - 127;
    int nMaxPower = nLog2 > 0 ? nLog2 + 1 : nLog2 - 1;
    float fMaxPower = bAbs(127.0f / nMaxPower);
    float f2 = f;

    for (int i = 1; i < bFloatToInt(fMaxPower); i++) {
        f2 *= f;
    }

    n = *reinterpret_cast<int *>(&f2);
    nLog2 = ((n >> 23) & 0xFF) - 127;

    return nLog2 / fMaxPower;
}

// La K del mipmap del GS (en dieciseisavos) para una densidad de texeles, con el sesgo de la textura.
int16 ComputeK(float fTexelDensity, TextureInfo *pTextureInfo) {
    TextureInfoPlatInfo *plat_info = pTextureInfo->GetPlatInfo();
    int mipmap_l = plat_info->MipMapL;
    float fTextureK = ((plat_info->MipMapK << 20) >> 20) * (1.0f / 16.0f);
    float result = bSqrt(fTexelDensity);

    if (result > 0.0f) {
        float fK = Log2(1.0f / result);

        fK *= (1 << mipmap_l);

        fK = fTextureK - fK;
        fK += 4.7f;

        return bFloatToInt(fK * 16.0f);
    }

    return 0;
}

// La K del mipmap de cada tira: la densidad de texeles de su material corregida por lo inclinada que esta respecto a la
// camara (su normal media, Nz y Nt).
void eSolidPlatInfo::CalcAutoK(eSolid *solid) {
    if (!solid || AreChunksBeingMoved()) {
        return;
    }

    eTextureEntry *texture_table = solid->pTextureTable;
    eStripEntry *strip_entry_table = StripEntryTable;
    uint8 *strip_data_start = StripDataStart;
    int num_strips = NumStrips;

    if (!strip_entry_table || !texture_table) {
        return;
    }

    for (int n = 0; n < num_strips; n++) {
        eStripEntry *strip_entry = &strip_entry_table[n];
        TextureInfo *texture_info = texture_table[strip_entry->TextureNumber].pTextureInfo;

        if (texture_info) {
            float vNormalZ = strip_entry->Nz * (1.0f / 127.0f);

            vNormalZ = bSqrt(1.0f - vNormalZ * vNormalZ);

            float fBias = 0.8f;

            fBias = (bMax(0.0f, bCos(static_cast<unsigned short>(static_cast<unsigned char>(strip_entry->Nt) << 8))) * 0.4f + 0.6f) * fBias;
            vNormalZ = bMax(0.001f, fBias * vNormalZ + 1.0f - fBias);

            float fTexelDensity = strip_entry->MaterialDensity * (texture_info->Width * texture_info->Height);
            int16 k = ComputeK(fTexelDensity / vNormalZ, texture_info);

            reinterpret_cast<eStripData *>(strip_data_start + strip_entry->DataQWordOffset * 16)->SetMipMapK(k);
        }
    }
}

int eSolidPlatInterface::LoaderPlatChunks(bChunk *chunk) {
    bChunk *end = chunk->GetLastChunk();
    eSolidPlatInfo *plat_info = 0;

    for (bChunk *c = chunk->GetFirstChunk(); c < end; c = c->GetNext()) {
        switch (c->GetID()) {
        case 0x134a00:
            plat_info = reinterpret_cast<eSolidPlatInfo *>(c->GetAlignedData(16));
            SetPlatInfo(plat_info);
            break;
        case 0x134a01:
            plat_info->StripEntryTable = reinterpret_cast<eStripEntry *>(c->GetAlignedData(16));
            break;
        case 0x134a02:
            plat_info->StripDataStart = reinterpret_cast<uint8 *>(c->GetAlignedData(16));
            break;
        }
    }
    return 1;
}

int eSolidPlatInterface::UnloaderPlatChunks(bChunk *chunk) {
    SetPlatInfo(0);
    return 1;
}

int eSolidPlatInterface::FixPlatInfo() {
    return 1;
}

int eSolidPlatInterface::UnFixPlatInfo() {
    return 1;
}

void eSolidPlatInterface::SetSmoothVertex(uint32 vertex_offset, float nx, float ny, float nz) {
    eSolidPlatInfo *plat_info = GetPlatInfo();
    unsigned int strip = vertex_offset & 0xffff;
    unsigned int vertex = vertex_offset >> 16;
    eStripEntry *entry = &plat_info->StripEntryTable[strip];

    if (plat_info->NumStrips < strip) {
        return;
    }
    if (entry->NumVerts < vertex) {
        return;
    }
    if (!(entry->Flags & 0x4000)) {
        return;
    }
    reinterpret_cast<eStripData *>(plat_info->StripDataStart + entry->DataQWordOffset * 16)->SetNORM(vertex, nx, ny, nz);
}

int eLoadSolidListPlatChunks(bChunk *chunk) {
    bChunk *end = chunk->GetLastChunk();

    for (bChunk *c = chunk->GetFirstChunk(); c < end; c = c->GetNext()) {
    }
    return 1;
}

int eUnloadSolidListPlatChunks(bChunk *chunk) {
    bChunk *end = chunk->GetLastChunk();

    for (bChunk *c = chunk->GetFirstChunk(); c < end; c = c->GetNext()) {
    }
    return 1;
}

void eSolidPlatNotifyTextureLoading(eSolid *solid) {
    eSolidPlatInfo *plat_info = solid->GetPlatInfo();

    if (plat_info) {
        plat_info->CalcAutoK(solid);
    }
}

void AutoKit(eSolid *solid) {
    eSolidPlatInfo *plat_info = solid->GetPlatInfo();

    if (plat_info) {
        plat_info->CalcAutoK(solid);
    }
}
