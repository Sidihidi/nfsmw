#include "Speed/Indep/Src/Frontend/MoviePlayer/MoviePlayer.hpp"
#include "Speed/Indep/Src/Frontend/FEngRender.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/PSX2/Src/Ecstasy/EcstasyE.hpp"

TextureInfoPlatInfo MovieTextureInfoPlatInfo;

// La de EcstasyE.cpp en bloques (sin el * 256) y siempre en PSMCT32.
static uint32 GetPS2VramSize(int width, int height, int bit_depth) {
    PS2VramInfo *info = &PS2VramInfoPSMCT32;

    int num_pages_x = (width + info->PixelsPerPageX - 1) / info->PixelsPerPageX;
    int num_pages_y = (height + info->PixelsPerPageY - 1) / info->PixelsPerPageY;
    int total_num_pages = num_pages_x * num_pages_y;
    int total_num_blocks = total_num_pages * 32;

    if (total_num_pages != 0) {
        int page_pixels_x = width - (num_pages_x - 1) * info->PixelsPerPageX;
        int page_pixels_y = height - (num_pages_y - 1) * info->PixelsPerPageY;
        int num_blocks_x = (page_pixels_x + info->PixelsPerBlockX - 1) / info->PixelsPerBlockX;
        int num_blocks_y = (page_pixels_y + info->PixelsPerBlockY - 1) / info->PixelsPerBlockY;
        int block_index = (num_blocks_x - 1) + (num_blocks_y - 1) * info->BlocksPerPageX;

        total_num_blocks = info->BlockNumbers[block_index] + total_num_blocks - 31;
    }

    return total_num_blocks;
}

// Un macrobloque a su sitio. Los stabs de la A124 dan un inline con sus propios dest, y, dw e i dentro de
// DescramblePS2MovieFrame (sin el, la suma del puntero sale con los operandos al reves); el nombre es nuestro.
static inline void DescrambleBlock(uint32 *dest, uint32 *src, int x, int y, int dw) {
    for (int i = 0; i < 16; i++) {
        bMemCpy(&dest[(y + i) * dw + x], src, 64);
        src += 16;
    }
}

// El decodificador deja la imagen en macrobloques de 16x16 pixeles seguidos, columna a columna.
static void DescramblePS2MovieFrame(uint32 *dest, SHAPE *shape) {
    int frame_width = SHAPE_width(shape);
    int frame_height = SHAPE_height(shape);
    uint32 *src = (uint32 *)SHAPE_data(shape);

    for (int j = 0; j < frame_width / 16; j++) {
        for (int i = 0; i < frame_height / 16; i++) {
            DescrambleBlock(dest, &src[((frame_height / 16) * j + i) * 256], j * 16, i * 16, frame_width);
        }
    }
}

void MoviePlayer::FillInTextureInfo(uint32 *frame_address, TextureInfo *texture_info, SHAPE *shape) {
    DescramblePS2MovieFrame(frame_address, shape);
    texture_info->SetPlatInfo(&MovieTextureInfoPlatInfo);
    int width = SHAPE_width(shape);
    int height = SHAPE_height(shape);
    int bit_depth = SHAPE_depth(shape);
    texture_info->Setup("MovieTexture", frame_address, width, height, 0x20, 0, 0, 0, 0, 0);

    TextureInfoPlatInfo *plat_info = texture_info->GetPlatInfo();
    plat_info->PixelStorageMode = 0;
    plat_info->ClutPixelStorageMode = 0;
    plat_info->TextureFx = 0;
    plat_info->MMag = 1;
    plat_info->MMin = 2;
    plat_info->MipMapK = 0;
    plat_info->MipMapL = 0;
    plat_info->ImageInfos[0].Width = width;
    plat_info->ImageInfos[0].Height = height;
    plat_info->ImageInfos[0].TBP = 0;
    plat_info->ImageInfos[0].TBW = next_power_of_2(width) >> 6;
    plat_info->ImageInfos[0].IsSwizzled = 0;
    plat_info->ImageVramBlocksRequired = GetPS2VramSize(width, height, 32);
    plat_info->PaletteVramBlocksRequired = 0;
    plat_info->AlphaBits = 0;
    plat_info->AlphaFix = 0x80;
    plat_info->pActiveTexture = 0;
    plat_info->PaletteVramAddress = 0;
    plat_info->ImageVramAddress = 0;
#ifndef EA_BUILD_A124
    texture_info->ApplyAlphaSorting = 1;
#endif
}

int RCMP_GetMaxFramesOutStanding() {
    return 1;
}
