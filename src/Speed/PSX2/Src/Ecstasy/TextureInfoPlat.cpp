#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"
#include "Speed/PSX2/Src/Ecstasy/TextureInfoPlat.hpp"

extern SlotPool *eAnimTextureSlotPool;

void TextureInfoPlatInterface::SetPlatInfo(TextureInfoPlatInfo *info) {
    PlatInfo = info;
}

void TextureInfoPlatInterface::Init() {
    TextureInfoPlatInfo *plat_info = GetPlatInfo();

    plat_info->pActiveTexture = 0;
    plat_info->PaletteVramAddress = 0;
    plat_info->ImageVramAddress = 0;
}

void TextureInfoPlatInterface::Close() {}

void *TextureInfoPlatInterface::LockImage(TextureLockType lock_type) {
    return static_cast<TextureInfo *>(this)->ImageData;
}

void TextureInfoPlatInterface::UnlockImage(void *image_lock) {}

void *TextureInfoPlatInterface::LockPalette(TextureLockType lock) {
    return static_cast<TextureInfo *>(this)->PaletteData;
}

void TextureInfoPlatInterface::UnlockPalette(void *palette_lock) {}

// Los stabs de la A124 dan a CreateAnimData un solo local, `uint32 *val` (la imagen en val[0] y la paleta en val[1]),
// y a SetAnimData ninguno.
void *TextureInfoPlatInterface::CreateAnimData() {
    uint32 *val = static_cast<uint32 *>(bOMalloc(eAnimTextureSlotPool));

    val[0] = reinterpret_cast<uint32>(static_cast<TextureInfo *>(this)->ImageData);
    val[1] = reinterpret_cast<uint32>(static_cast<TextureInfo *>(this)->PaletteData);
    return val;
}

void TextureInfoPlatInterface::ReleaseAnimData(void *anim_data) {
    bFree(eAnimTextureSlotPool, anim_data);
}

void TextureInfoPlatInterface::SetAnimData(void *anim_data) {
    static_cast<TextureInfo *>(this)->ImageData = reinterpret_cast<void *>(static_cast<uint32 *>(anim_data)[0]);
    static_cast<TextureInfo *>(this)->PaletteData = reinterpret_cast<void *>(static_cast<uint32 *>(anim_data)[1]);
}
