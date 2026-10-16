#ifndef FRONTEND_MENUSCREENS_SAFEHOUSE_CAREER_UIREPSHEETRIVALSTREAMER_H
#define FRONTEND_MENUSCREENS_SAFEHOUSE_CAREER_UIREPSHEETRIVALSTREAMER_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/FEng/FEObject.h"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"

#ifdef EA_BUILD_A124
// La A124 es otra version (0x44 con el vtable al final, stabs de zFe): carga las texturas del bin con la API uint32 y
// las recibe en TexturesLoadedCallback, que las pasa a la instancia unica (pInstance). LoadAllTextures, los
// Rival/Tag/BGLoadCallback y los SetRival/Tag/BGLoaded estan en sus stabs pero el ELF no los conserva.
class uiRepSheetRivalStreamer {
  public:
    uiRepSheetRivalStreamer(const char *pkg_name, bool in_game);
    virtual ~uiRepSheetRivalStreamer();

    static void MakeSpaceInPoolCallbackBridge(int param) {
        reinterpret_cast<uiRepSheetRivalStreamer *>(param)->MakeSpaceInPoolCallback();
    }

    void MakeSpaceInPoolCallback();
    void Init(uint32 the_bin, FEImage *the_rival, FEImage *the_tag, FEImage *the_bg);
    void SetPackLoaded();

  protected:
    void UnloadAllTextures();
    int CalcTexturesToLoad(uint32 *temp, int bin);
    static void PackLoadCallback(uint32 screenPtr);
    static void TexturesLoadedCallback(uint32 bin);
    void SetTexturesLoaded(int bin);

    static uiRepSheetRivalStreamer *pInstance;

    int MemPoolNum;                 // offset 0x0, size 0x4
    const char *pkg_name;           // offset 0x4, size 0x4
    bool bTexPackLoaded;            // offset 0x8, size 0x4
    bool bInGame;                   // offset 0xC, size 0x4
    bool bLoading;                  // offset 0x10, size 0x4
    int NumLoadedTextures;          // offset 0x14, size 0x4
    unsigned int LoadedTextures[3]; // offset 0x18, size 0xC
    FEImage *Rival;                 // offset 0x24, size 0x4
    FEImage *Tag;                   // offset 0x28, size 0x4
    FEImage *BG;                    // offset 0x2C, size 0x4
    uint32 BGHash;                  // offset 0x30, size 0x4
    uint32 TagHash;                 // offset 0x34, size 0x4
    uint32 RivalHash;               // offset 0x38, size 0x4
    uint32 Bin;                     // offset 0x3C, size 0x4
};
#else
// total size: 0x3C
// r56-fe: los tres bridges con el cuerpo EN CLASE: el objetivo los emite en la
// cola de finish_file (zFe.o 0x0291D8..), no en el cuerpo.
class uiRepSheetRivalStreamer {
  public:
    static void MakeSpaceInPoolCallbackBridge(int param) {
        reinterpret_cast<uiRepSheetRivalStreamer *>(param)->MakeSpaceInPoolCallback();
    }

    static void TexturePackLoadedCallbackBridge(void *param) {
        static_cast<uiRepSheetRivalStreamer *>(param)->TexturePackLoadedCallback();
    }

    static void TexturesLoadedCallbackBridge(void *param) {
        static_cast<uiRepSheetRivalStreamer *>(param)->TexturesLoadedCallback();
    }

    uiRepSheetRivalStreamer(const char *pkg_name, bool in_game);

    virtual ~uiRepSheetRivalStreamer();

    void MakeSpaceInPoolCallback();
  protected:
    void TexturePackLoadedCallback();
  public:
    void Init(unsigned int the_bin, FEImage *the_rival, FEImage *the_tag, FEImage *the_bg);
  protected:
    void LoadTextures();

    void UnloadTextures();

    int CalcTexturesToLoad(unsigned int *temp, int bin);

    void TexturesLoadedCallback();

    int MemPoolNum;                   // offset 0x0, size 0x4
    bool bInGame;                     // offset 0x4, size 0x1
    bool bMakeSpaceInPoolComplete;    // offset 0x8, size 0x1
    const char *pkg_name;             // offset 0xC, size 0x4
    int DesiredBin;                   // offset 0x10, size 0x4
    int LoadedBin;                    // offset 0x14, size 0x4
    bool LoadingInProgress;           // offset 0x18, size 0x1
    int NumLoadedTextures;            // offset 0x1C, size 0x4
    unsigned int LoadedTextures[3];   // offset 0x20, size 0xC
    FEImage *Rival;                   // offset 0x2C, size 0x4
    FEImage *Tag;                     // offset 0x30, size 0x4
    FEImage *BG;                      // offset 0x34, size 0x4
};
#endif

#endif
