#include "uiRepSheetRivalStreamer.hpp"
#include "Speed/Indep/Src/Ecstasy/Texture.hpp"

#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/World/TrackStreamer.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"

struct FEObject;

void FEngSetTextureHash(FEImage *image, unsigned int hash);
unsigned int FEngHashString(const char *format, ...);
struct TextureInfo;
TextureInfo *GetTextureInfo(unsigned int hash, int, int);

#ifdef EA_BUILD_A124
uiRepSheetRivalStreamer *uiRepSheetRivalStreamer::pInstance;

uiRepSheetRivalStreamer::uiRepSheetRivalStreamer(const char *name, bool in_game)
    : MemPoolNum(0), pkg_name(name), bTexPackLoaded(false), bInGame(in_game), bLoading(false), TagHash(0), RivalHash(0) {
    pInstance = this;
    if (bInGame) {
        MemPoolNum = 7;
        TheTrackStreamer.MakeSpaceInPool(0x60000, MakeSpaceInPoolCallbackBridge, reinterpret_cast<int>(this));
    } else {
        eLoadStreamingTexturePack("Global\\Rivals.bin", PackLoadCallback, reinterpret_cast<uint32>(this), 0);
    }
}

uiRepSheetRivalStreamer::~uiRepSheetRivalStreamer() {
    if (bInGame) {
        TheTrackStreamer.WaitForCurrentLoadingToComplete();
    }
    WaitForResourceLoadingComplete();
    UnloadAllTextures();
    eUnloadStreamingTexturePack("Global\\Rivals.bin");
    if (MemPoolNum == 7) {
        TheTrackStreamer.RefreshLoading();
    }
}

void uiRepSheetRivalStreamer::MakeSpaceInPoolCallback() {
    eLoadStreamingTexturePack("Global\\Rivals.bin", PackLoadCallback, reinterpret_cast<uint32>(this), 0);
}

void uiRepSheetRivalStreamer::Init(uint32 the_bin, FEImage *the_rival, FEImage *the_tag, FEImage *the_bg) {
    Tag = the_tag;
    Bin = the_bin;
    BG = the_bg;
    Rival = the_rival;
    FEngSetInvisible(reinterpret_cast<FEObject *>(Rival));
    FEngSetInvisible(reinterpret_cast<FEObject *>(Tag));
    FEngSetInvisible(reinterpret_cast<FEObject *>(BG));
    if (!bLoading) {
        eUnloadStreamingTexture(LoadedTextures, NumLoadedTextures);
        eWaitForStreamingTexturePackLoading("Global\\Rivals.bin");
        LoadedTextures[0] = LoadedTextures[1] = LoadedTextures[2] = 0;
        bLoading = true;
        uint32 temp[3];
        memset(temp, 0, sizeof(temp));
        int count = CalcTexturesToLoad(temp, the_bin);
        eLoadStreamingTexture(temp, count, TexturesLoadedCallback, Bin, MemPoolNum);
    }
}
#else
uiRepSheetRivalStreamer::uiRepSheetRivalStreamer(const char *name, bool in_game) {
    pkg_name = name;
    MemPoolNum = 0;
    bInGame = in_game;
    DesiredBin = -1;
    LoadedBin = -1;
    LoadingInProgress = true;
    bMakeSpaceInPoolComplete = false;
    NumLoadedTextures = 0;
    Rival = nullptr;
    Tag = nullptr;
    BG = nullptr;
    if (bInGame) {
        MemPoolNum = 7;
        TheTrackStreamer.DisableZoneSwitching();
#ifdef EA_PLATFORM_PLAYSTATION2
        TheTrackStreamer.MakeSpaceInPool(0x60000, MakeSpaceInPoolCallbackBridge, reinterpret_cast<int>(this));
#else
        TheTrackStreamer.MakeSpaceInPool(0x30000, MakeSpaceInPoolCallbackBridge, reinterpret_cast<int>(this));
#endif
    } else {
        eLoadStreamingTexturePack("Global\\Rivals.bin", TexturePackLoadedCallbackBridge, this, 0);
    }
}

uiRepSheetRivalStreamer::~uiRepSheetRivalStreamer() {
    if (bInGame) {
        if (!bMakeSpaceInPoolComplete) {
            TheTrackStreamer.WaitForCurrentLoadingToComplete();
        }
        TheTrackStreamer.EnableZoneSwitching();
        TheTrackStreamer.RefreshLoading();
    }
    eWaitForStreamingTexturePackLoading("Global\\Rivals.bin");
    UnloadTextures();
    eUnloadStreamingTexturePack("Global\\Rivals.bin");
}

void uiRepSheetRivalStreamer::MakeSpaceInPoolCallback() {
    bMakeSpaceInPoolComplete = true;
    eLoadStreamingTexturePack("Global\\Rivals.bin", TexturePackLoadedCallbackBridge, this, 0);
}

void uiRepSheetRivalStreamer::TexturePackLoadedCallback() {
    LoadingInProgress = false;
    LoadTextures();
}

void uiRepSheetRivalStreamer::Init(unsigned int the_bin, FEImage *the_rival, FEImage *the_tag, FEImage *the_bg) {
    DesiredBin = the_bin;
    Rival = the_rival;
    Tag = the_tag;
    BG = the_bg;
    FEngSetInvisible(reinterpret_cast<FEObject *>(Rival));
    FEngSetInvisible(reinterpret_cast<FEObject *>(Tag));
    FEngSetInvisible(reinterpret_cast<FEObject *>(BG));
    if (!LoadingInProgress) {
        LoadTextures();
    }
}

void uiRepSheetRivalStreamer::LoadTextures() {
    if (LoadedBin != DesiredBin) {
        if (NumLoadedTextures != 0) {
            UnloadTextures();
        }
        LoadingInProgress = true;
        LoadedBin = DesiredBin;
        NumLoadedTextures = CalcTexturesToLoad(LoadedTextures, DesiredBin);
        eLoadStreamingTexture(LoadedTextures, NumLoadedTextures, TexturesLoadedCallbackBridge, this, MemPoolNum);
    }
}

void uiRepSheetRivalStreamer::UnloadTextures() {
    eUnloadStreamingTexture(LoadedTextures, NumLoadedTextures);
    NumLoadedTextures = 0;
    LoadedBin = -1;
}
#endif

int uiRepSheetRivalStreamer::CalcTexturesToLoad(unsigned int *temp, int bin) {
    int index = 0;
    if (Rival != nullptr) {
        if (bInGame) {
            temp[index++] = FEngHashString("RIVAL_%02d_IG", bin);
        } else {
            temp[index++] = FEngHashString("RIVAL_%02d", bin);
        }
    }
    if (Tag != nullptr) {
        if (bInGame) {
            temp[index++] = FEngHashString("RIVAL_%02d_IG_GRAF", bin);
        } else {
            temp[index++] = FEngHashString("RIVAL_%02d_GRAF", bin);
        }
    }
    if (BG != nullptr) {
        if (bInGame) {
            temp[index++] = FEngHashString("RIVAL_%02d_IG_BG", bin);
        } else {
            temp[index++] = FEngHashString("RIVAL_%02d_BG", bin);
        }
    }
    return index;
}

#ifdef EA_BUILD_A124
void uiRepSheetRivalStreamer::UnloadAllTextures() {
    if (Rival) {
        FEngSetInvisible(reinterpret_cast<FEObject *>(Rival));
    }
    if (Tag) {
        FEngSetInvisible(reinterpret_cast<FEObject *>(Tag));
    }
    if (BG) {
        FEngSetInvisible(reinterpret_cast<FEObject *>(BG));
    }

    if (TagHash) {
        eUnloadStreamingTexture(TagHash);
    }
    if (RivalHash) {
        eUnloadStreamingTexture(RivalHash);
    }
    if (BGHash) {
        eUnloadStreamingTexture(BGHash);
    }
}

void uiRepSheetRivalStreamer::PackLoadCallback(uint32 screenPtr) {
    reinterpret_cast<uiRepSheetRivalStreamer *>(screenPtr)->SetPackLoaded();
}

void uiRepSheetRivalStreamer::TexturesLoadedCallback(uint32 bin) {
    pInstance->SetTexturesLoaded(bin);
}

void uiRepSheetRivalStreamer::SetPackLoaded() {
    if (eIsStreamingTexturePackLoaded("Global\\Rivals.bin")) {
        bTexPackLoaded = true;
        bLoading = true;
        uint32 temp[3];
        memset(temp, 0, sizeof(temp));
        int count = CalcTexturesToLoad(temp, Bin);
        eLoadStreamingTexture(temp, count, TexturesLoadedCallback, Bin, MemPoolNum);
    }
}

void uiRepSheetRivalStreamer::SetTexturesLoaded(int bin) {
    if (bin != Bin) {
        uint32 temp[3];
        memset(temp, 0, sizeof(temp));
        int count = CalcTexturesToLoad(temp, bin);
        eUnloadStreamingTexture(temp, count);
        bLoading = true;
        count = CalcTexturesToLoad(temp, Bin);
        eLoadStreamingTexture(temp, count, TexturesLoadedCallback, Bin, MemPoolNum);
        return;
    }
    NumLoadedTextures = CalcTexturesToLoad(LoadedTextures, bin);
    bLoading = false;
    int index = 0;
    if (Rival != nullptr) {
        cFEng::Get()->QueuePackageMessage(0xC0942E85, pkg_name, nullptr);
        GetTextureInfo(LoadedTextures[0], 0, 0);
        FEngSetTextureHash(Rival, LoadedTextures[index++]);
        FEngSetVisible(reinterpret_cast<FEObject *>(Rival));
    }
    if (Tag != nullptr) {
        cFEng::Get()->QueuePackageMessage(0x8C9D4547, pkg_name, nullptr);
        FEngSetTextureHash(Tag, LoadedTextures[index++]);
        FEngSetVisible(reinterpret_cast<FEObject *>(Tag));
    }
    if (BG != nullptr) {
        cFEng::Get()->QueuePackageMessage(0xD22B95D0, pkg_name, nullptr);
        FEngSetTextureHash(BG, LoadedTextures[index]);
        FEngSetVisible(reinterpret_cast<FEObject *>(BG));
    }
}
#else
void uiRepSheetRivalStreamer::TexturesLoadedCallback() {
    int idx;
    LoadingInProgress = false;
    if (LoadedBin != DesiredBin) {
        LoadTextures();
        return;
    }
    idx = 0;
    if (Rival != nullptr) {
        cFEng::Get()->QueuePackageMessage(0xC0942E85, pkg_name, nullptr);
        TextureInfo *ti = GetTextureInfo(LoadedTextures[0], 0, 0);
        FEngSetTextureHash(Rival, LoadedTextures[idx++]);
        FEngSetVisible(reinterpret_cast<FEObject *>(Rival));
    }
    if (Tag != nullptr) {
        cFEng::Get()->QueuePackageMessage(0x8C9D4547, pkg_name, nullptr);
        FEngSetTextureHash(Tag, LoadedTextures[idx]);
        idx++;
        FEngSetVisible(reinterpret_cast<FEObject *>(Tag));
    }
    if (BG == nullptr) {
        return;
    }
    cFEng::Get()->QueuePackageMessage(0xD22B95D0, pkg_name, nullptr);
    FEngSetTextureHash(BG, LoadedTextures[idx]);
    FEngSetVisible(reinterpret_cast<FEObject *>(BG));
}
#endif
