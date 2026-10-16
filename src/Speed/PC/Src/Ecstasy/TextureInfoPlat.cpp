#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bMemory.hpp"
#include "Speed/PC/Src/Ecstasy/D3D9Min.hpp"
#include "Speed/PC/Src/Ecstasy/TextureInfoPlat.hpp"

// Los nombres de las variables de este fichero son nuestros: el exe de PC no los trae.
extern IDirect3DDevice9 *g_D3DDevice;
extern int AnimatedTextureEnable;
extern bool eNoTextures;
extern int eTextureMipSkip;

SlotPool *eAnimTextureSlotPool = 0;
bTList<TextureInfoPlatInfo> TextureInfoPlatInfoList;
bPList<IDirect3DTexture9> eTextureReleaseList;

void eUnSwizzle8bitPalette(unsigned int *palette) {}

void eSwizzle8bitPalette(unsigned int *palette) {}

void TextureInfoPlatInterface::SetPlatInfo(TextureInfoPlatInfo *info) {
    this->PlatInfo = info;
}

void TextureInfoPlatInterface::Init() {
    TextureInfo *texture_info = static_cast<TextureInfo *>(this);
    TextureInfoPlatInfo *plat_info = GetPlatInfo();
    IDirect3DTexture9 **d3d_texture = &plat_info->pD3DTexture;

    if (*d3d_texture) {
        Close();
    }
    if (bStrNCmp(texture_info->DebugName, "SFX_TRACKBARRIER", 16) == 0 && texture_info->ApplyAlphaSorting) {
        texture_info->ApplyAlphaSorting = 2;
    }
    unsigned char *image_data = static_cast<unsigned char *>(texture_info->ImageData);
    if (!image_data) {
        return;
    }
    D3DFORMAT format = static_cast<D3DFORMAT>(plat_info->Format);
    int allocated = 0;
    int image_size = texture_info->BaseImageSize;
    unsigned char *data = image_data;
    if (format == D3DFMT_P8) {
        unsigned int *palette = static_cast<unsigned int *>(texture_info->PaletteData);
        data = static_cast<unsigned char *>(operator new(texture_info->ImageSize * sizeof(unsigned int)));
        unsigned int *dest = reinterpret_cast<unsigned int *>(data);

        allocated = 1;
        for (int mip = 0; mip < texture_info->NumMipMapLevels; mip++) {
            int w = texture_info->Width >> mip;
            int h = texture_info->Height >> mip;
            for (int i = 0; i < w * h; i++) {
                *dest++ = palette[*image_data++];
            }
        }
        format = D3DFMT_A8R8G8B8;
        image_size *= sizeof(unsigned int);
        texture_info->ImageCompressionType = TEXCOMP_32BIT;
    }
    if (eNoTextures) {
        *d3d_texture = nullptr;
        return;
    }
    int skip = 0;
    bool keep_mips = false;
    if (texture_info->pTexturePack && texture_info->pTexturePack->GetNameHash() == 0x3e871f1) {
        keep_mips = true;
    }
    if (eTextureMipSkip > 0 && !keep_mips) {
        if (texture_info->NumMipMapLevels > eTextureMipSkip) {
            skip = eTextureMipSkip;
        } else if (texture_info->NumMipMapLevels > 1) {
            skip = texture_info->NumMipMapLevels - 1;
        }
        if (skip != 0) {
            texture_info->Width = texture_info->Width / (skip * 2);
            texture_info->Height = texture_info->Height / (skip * 2);
        }
    }
    g_D3DDevice->CreateTexture(texture_info->Width, texture_info->Height, texture_info->NumMipMapLevels - skip, 0, format, D3DPOOL_MANAGED,
                               d3d_texture, nullptr);
    if (!*d3d_texture) {
        return;
    }
    TextureInfoPlatInfoList.AddTail(plat_info);
    unsigned char *src = data;
    int num_levels = texture_info->NumMipMapLevels;
    if (skip > 0) {
        num_levels -= skip;
        do {
            src += image_size;
            image_size /= 4;
        } while (--skip);
    }
    for (int level = 0; level < num_levels; level++) {
        D3DLOCKED_RECT locked_rect;

        (*d3d_texture)->LockRect(level, &locked_rect, nullptr, 0);
        int level_size = image_size >> (level * 2);
        bMemCpy(locked_rect.pBits, src, level_size);
        (*d3d_texture)->UnlockRect(level);
        src += level_size;
    }
    if (allocated) {
        operator delete(data);
    }
}

void TextureInfoPlatInterface::Close() {
    TextureInfoPlatInfo *plat_info = GetPlatInfo();

    if (plat_info->pD3DTexture) {
        eTextureReleaseList.AddTail(plat_info->pD3DTexture);
        plat_info->pD3DTexture = nullptr;
        plat_info->Remove();
    }
}

void *TextureInfoPlatInterface::LockImage(TextureLockType lock) {
    D3DLOCKED_RECT locked_rect;
    TextureInfoPlatInfo *plat_info = this->PlatInfo;

    plat_info->pD3DTexture->LockRect(0, &locked_rect, nullptr, 0);
    return locked_rect.pBits;
}

void TextureInfoPlatInterface::UnlockImage(void *image_lock) {
    TextureInfoPlatInfo *plat_info = this->PlatInfo;

    plat_info->pD3DTexture->UnlockRect(0);
}

void *TextureInfoPlatInterface::LockPalette(TextureLockType lock) {
    return static_cast<TextureInfo *>(this)->GetPaletteData();
}

void TextureInfoPlatInterface::UnlockPalette(void *palette_lock) {}

void *TextureInfoPlatInterface::CreateAnimData() {
    TextureInfoPlatInfo *plat_info = GetPlatInfo();
    IDirect3DTexture9 **anim_data = static_cast<IDirect3DTexture9 **>(bOMalloc(eAnimTextureSlotPool));

    *anim_data = plat_info->pD3DTexture;
    return anim_data;
}

void TextureInfoPlatInterface::ReleaseAnimData(void *anim_data) {
    bFree(eAnimTextureSlotPool, anim_data);
}

void TextureInfoPlatInterface::SetAnimData(void *anim_data) {
    if (AnimatedTextureEnable) {
        GetPlatInfo()->pD3DTexture = *static_cast<IDirect3DTexture9 **>(anim_data);
    }
}
