#include "Speed/Indep/Src/Frontend/MoviePlayer/MoviePlayer.hpp"
#include "Speed/GameCube/Src/G.hpp"
#include "realcore/system.h"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/SubTitle.hpp"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/World/CarLoader.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/Src/World/TrackStreamer.hpp"
#include "Speed/Indep/Src/Misc/Joylog.hpp"
#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/Indep/Src/Misc/DemoDisc.hpp"
#include "Speed/Indep/Src/Misc/Main.hpp"
#include "Speed/Indep/Src/Frontend/DebugMenu.h"

// File: speed/indep/src/frontend/MoviePlayer/MoviePlayer.cpp
// total size: 0x8
// Decl: speed/indep/src/frontend/MoviePlayer/MoviePlayer.cpp:141
struct ShapeMemoryAllocator : public EA::Allocator::IAllocator {
  public:
    ShapeMemoryAllocator() : mRefcount(1) {} // Decl: speed/indep/src/frontend/MoviePlayer/MoviePlayer.cpp:143
    ~ShapeMemoryAllocator() override {} // Decl: speed/indep/src/frontend/MoviePlayer/MoviePlayer.cpp:144

    void *Alloc(size_t size, const EA::TagValuePair &flags) override;
    void *Alloc(size_t size);

  private:
    int mRefcount; // offset 0x4, size 0x4, Decl: speed/indep/src/frontend/MoviePlayer/MoviePlayer.cpp:152

    void Free(void *pBlock, size_t size) override; // Decl: speed/indep/src/frontend/MoviePlayer/MoviePlayer.cpp:228

    int AddRef() override; // Decl: speed/indep/src/frontend/MoviePlayer/MoviePlayer.cpp:240

    int Release() override; // Decl: speed/indep/src/frontend/MoviePlayer/MoviePlayer.cpp:245
};

#define MOVIE_TEXTUREINFO_COUNT 3 // :258

// TODO D:/env/egami/realgraph/6/source/shape/cmn/allocator.cpp
namespace RealShape {
class GraphObject {
  public:
    static void SetAllocator(EA::Allocator::IAllocator *);
};
} // namespace RealShape

TextureInfo MovieTextureInfo;

#ifdef EA_PLATFORM_GAMECUBE
// Solo GameCube: ni las dos PS2 ni PC la tienen; alli el asignador de shapes y RCMP_PlayerAllocAlign van directos a
// la memoria del TrackStreamer o a bMalloc.
void *GamecubeMaybeAllocateFromCarLoader(int size, const char *name, int alloc_params) {
    bool track_stream_pool_exists = TheTrackStreamer.HasMemoryPool();
    if ((track_stream_pool_exists && size <= bLargestMalloc(7)) || size <= bLargestMalloc(0)) {
        return nullptr;
    }
    TheCarLoader.MakeSpaceInPool(size);
    void *ptr = bMalloc(size, name, __LINE__, (CarLoaderMemoryPoolNumber & 0xf) | alloc_params);
    if (ptr != nullptr) {
        return ptr;
    }
    return nullptr;
}
#endif

MoviePlayer *gMoviePlayer = nullptr;
// gMovieStartTime, MoviePlayer_Bypass y MoviePlayer_Play no estan en la A124 (ni en su mapa ni en sus stabs).
#ifndef EA_BUILD_A124
int gMovieStartTime = 0xFFFFFFFF;
#endif

// El DWARF de GC la lista entre gMovieStartTime e IsMovieTimerPrintf (estripada: solo la lee el Play de la A124).
bool FMVScreenPrintf = false;
bool IsMovieTimerPrintf = false;
ShapeMemoryAllocator gShapeMemoryAllocator;

// total size: 0x8
struct MoveVolumeInfo {
    // Members
    const char *MovieBaseName; // offset 0x0, size 0x4
    int Volume;                // offset 0x4, size 0x4
};
// Cada version trae su tabla: 38 entradas en GameCube y 40 en las dos PS2, con otros volumenes (y en la A124
// otras tres peliculas al final). GetMovieCategoryVolume recorre la tabla entera (NUM_ELEMENTS): 0x26 vueltas en
// GameCube y 0x28 en las dos PS2.
MoveVolumeInfo MovieVolumeArray[] = {
#ifdef EA_BUILD_A124
    {"MOVIES\\attract_movie", 90},
    {"MOVIES\\drag_tutorial", 90},
    {"MOVIES\\ealogo", 80},
    {"MOVIES\\intro_movie", 45},
    {"MOVIES\\sprint_tutorial", 90},
    {"MOVIES\\tollbooth_tutorial", 95},
    {"MOVIES\\bounty_tutorial", 95},
    {"MOVIES\\pursuit_tutorial", 95},
    {"MOVIES\\speedtrap_tutorial", 95},
    {"MOVIES\\psa", 90},
    {"MOVIES\\blacklist_01", 95},
    {"MOVIES\\blacklist_02", 95},
    {"MOVIES\\blacklist_03", 95},
    {"MOVIES\\blacklist_04", 95},
    {"MOVIES\\blacklist_05", 95},
    {"MOVIES\\blacklist_06", 95},
    {"MOVIES\\blacklist_07", 95},
    {"MOVIES\\blacklist_08", 95},
    {"MOVIES\\blacklist_09", 95},
    {"MOVIES\\blacklist_10", 95},
    {"MOVIES\\blacklist_11", 95},
    {"MOVIES\\blacklist_12", 95},
    {"MOVIES\\blacklist_13", 95},
    {"MOVIES\\blacklist_14", 95},
    {"MOVIES\\blacklist_15", 95},
    {"MOVIES\\storyfmv_bla134", 85},
    {"MOVIES\\storyfmv_bus12", 85},
    {"MOVIES\\storyfmv_cro06_coh06a", 85},
    {"MOVIES\\storyfmv_dda01", 85},
    {"MOVIES\\storyfmv_epi138", 85},
    {"MOVIES\\storyfmv_her136", 85},
    {"MOVIES\\storyfmv_pin11", 85},
    {"MOVIES\\storyfmv_pol17_mot21", 85},
    {"MOVIES\\storyfmv_rap30", 85},
    {"MOVIES\\storyfmv_raz08", 85},
    {"MOVIES\\storyfmv_roc02", 85},
    {"MOVIES\\storyfmv_saf25", 85},
    {"MOVIES\\MarvNM-480", 85},
    {"MOVIES\\NBALive-2006-n", 85},
    {"MOVIES\\SSX-OT-n-480", 85},
#elif defined(EA_PLATFORM_PLAYSTATION2)
    {"MOVIES\\attract_movie", 100},
    {"MOVIES\\drag_tutorial", 85},
    {"MOVIES\\ealogo", 80},
    {"MOVIES\\intro_movie", 85},
    {"MOVIES\\sprint_tutorial", 85},
    {"MOVIES\\tollbooth_tutorial", 85},
    {"MOVIES\\bounty_tutorial", 85},
    {"MOVIES\\pursuit_tutorial", 85},
    {"MOVIES\\speedtrap_tutorial", 85},
    {"MOVIES\\psa", 75},
    {"MOVIES\\blacklist_01", 65},
    {"MOVIES\\blacklist_02", 65},
    {"MOVIES\\blacklist_03", 65},
    {"MOVIES\\blacklist_04", 65},
    {"MOVIES\\blacklist_05", 65},
    {"MOVIES\\blacklist_06", 65},
    {"MOVIES\\blacklist_07", 65},
    {"MOVIES\\blacklist_08", 65},
    {"MOVIES\\blacklist_09", 65},
    {"MOVIES\\blacklist_10", 65},
    {"MOVIES\\blacklist_11", 65},
    {"MOVIES\\blacklist_12", 65},
    {"MOVIES\\blacklist_13", 65},
    {"MOVIES\\blacklist_14", 65},
    {"MOVIES\\blacklist_15", 65},
    {"MOVIES\\storyfmv_bla134", 127},
    {"MOVIES\\storyfmv_bus12", 127},
    {"MOVIES\\storyfmv_cro06_coh06a", 127},
    {"MOVIES\\storyfmv_dda01", 127},
    {"MOVIES\\storyfmv_epi138", 127},
    {"MOVIES\\storyfmv_her136", 127},
    {"MOVIES\\storyfmv_pin11", 127},
    {"MOVIES\\storyfmv_pol17_mot21", 127},
    {"MOVIES\\storyfmv_rap30", 127},
    {"MOVIES\\storyfmv_raz08", 127},
    {"MOVIES\\storyfmv_roc02", 127},
    {"MOVIES\\storyfmv_saf25", 127},
    {"MOVIES\\storyfmv_rac01", 127},
    {"MOVIES\\xsell_SSX", 20},
    {"MOVIES\\xsell_black", 85},
#else
    {"MOVIES\\attract_movie", 65},
    {"MOVIES\\drag_tutorial", 55},
    {"MOVIES\\ealogo", 60},
    {"MOVIES\\intro_movie", 55},
    {"MOVIES\\sprint_tutorial", 55},
    {"MOVIES\\tollbooth_tutorial", 55},
    {"MOVIES\\bounty_tutorial", 55},
    {"MOVIES\\pursuit_tutorial", 55},
    {"MOVIES\\speedtrap_tutorial", 55},
    {"MOVIES\\psa", 50},
    {"MOVIES\\blacklist_01", 45},
    {"MOVIES\\blacklist_02", 45},
    {"MOVIES\\blacklist_03", 45},
    {"MOVIES\\blacklist_04", 45},
    {"MOVIES\\blacklist_05", 45},
    {"MOVIES\\blacklist_06", 45},
    {"MOVIES\\blacklist_07", 45},
    {"MOVIES\\blacklist_08", 45},
    {"MOVIES\\blacklist_09", 45},
    {"MOVIES\\blacklist_10", 45},
    {"MOVIES\\blacklist_11", 45},
    {"MOVIES\\blacklist_12", 45},
    {"MOVIES\\blacklist_13", 45},
    {"MOVIES\\blacklist_14", 45},
    {"MOVIES\\blacklist_15", 45},
    {"MOVIES\\storyfmv_bla134", 80},
    {"MOVIES\\storyfmv_bus12", 80},
    {"MOVIES\\storyfmv_cro06_coh06a", 80},
    {"MOVIES\\storyfmv_dda01", 80},
    {"MOVIES\\storyfmv_epi138", 80},
    {"MOVIES\\storyfmv_her136", 80},
    {"MOVIES\\storyfmv_pin11", 80},
    {"MOVIES\\storyfmv_pol17_mot21", 80},
    {"MOVIES\\storyfmv_rap30", 80},
    {"MOVIES\\storyfmv_raz08", 80},
    {"MOVIES\\storyfmv_roc02", 80},
    {"MOVIES\\storyfmv_saf25", 80},
    {"MOVIES\\storyfmv_rac01", 80},
#endif
};



unsigned int *RCMPDecodeBuffer = nullptr;

#ifndef EA_BUILD_A124
bool MoviePlayer_Bypass() {
    return bGetTickerDifference(gMovieStartTime) > 5000.0f;
}

void MoviePlayer_Play() {
    if (gMoviePlayer != nullptr) {
        gMovieStartTime = bGetTicker();
        gMoviePlayer->Play();
    }
}
#endif

void *ShapeMemoryAllocator::Alloc(size_t size, const EA::TagValuePair &flags) {
    const char *name = "";
    int allocation_params = BMEMORY_TOP_BIT;
    int offset = 0;
    const EA::TagValuePair *p = &flags;
    while (p != nullptr) {
        unsigned int tag = p->mTag;
        switch (tag) {
        case 1:
            name = static_cast<const char *>(p->mValue.mPointer);
            break;
        case 2:
            allocation_params |= BMEMORY_ALIGNMENT(p->mValue.mInt);
            break;
        case 3: {
            int value = p->mValue.mInt;
            allocation_params |= BMEMORY_ALIGNMENT_OFFSET(value);
            offset = value;
            break;
        }
        case 4:
            allocation_params &= ~BMEMORY_TOP_BIT;
            break;
        }
        p = p->mNext;
    }
#ifdef EA_PLATFORM_GAMECUBE
    void *maybe = GamecubeMaybeAllocateFromCarLoader(size, name, allocation_params);
    if (maybe != nullptr) {
        return maybe;
    }
#endif
    if (!TheTrackStreamer.HasMemoryPool()) {
        return bMalloc(size, name, 0, allocation_params);
    }
    return TheTrackStreamer.AllocateUserMemory(size, "shape_mem", offset);
}

void ShapeMemoryAllocator::Free(void *pBlock, size_t size) {
    if (!TheTrackStreamer.HasMemoryPool()
#ifndef EA_PLATFORM_PLAYSTATION2 // las dos PS2 no comprueban IsUserMemory; GameCube y PC si
        || (pBlock != nullptr && !TheTrackStreamer.IsUserMemory(pBlock))
#endif
    ) {
        bFree(pBlock);
    } else {
        TheTrackStreamer.FreeUserMemory(pBlock);
    }
}

int ShapeMemoryAllocator::AddRef() {
    return ++mRefcount;
}

int ShapeMemoryAllocator::Release() {
#ifdef EA_PLATFORM_WIN32
    // PC emite dec/test/jg (r21); el resto, la forma original.
    int ref = mRefcount - 1;
    mRefcount = ref;
    if (ref > 0) {
        return ref;
    }
    delete this;
    return 0;
#else
    int ref = mRefcount - 1;
    mRefcount = ref;
    if (ref < 1) {
        delete this;
        ref = 0;
    }
    return ref;
#endif
}

void *RCMP_PlayerAllocAlign(const char *name, int size, int alignment, int headersize, int type) {
    if (name == nullptr || *name == '\0') {
        name = "RCMP_Mem";
    }
    size = size + headersize;
#ifdef EA_PLATFORM_GAMECUBE
    void *maybe = GamecubeMaybeAllocateFromCarLoader(size, name, BMEMORY_ALIGNMENT_OFFSET(headersize) | BMEMORY_ALIGNMENT(alignment));
    if (maybe != nullptr) {
        return maybe;
    }
#endif
    if (TheTrackStreamer.HasMemoryPool()) {
        return TheTrackStreamer.AllocateUserMemory(size, name, headersize);
    }
    if (alignment == 0) {
        alignment = 0x80;
    }
    return bMalloc(size, name, 0, BMEMORY_ALIGNMENT_OFFSET(headersize) | BMEMORY_ALIGNMENT(alignment) | BMEMORY_ALLOCATE_FROM_TOP);
}

void RCMP_PlayerFree(void *ptr) {
    if (!TheTrackStreamer.HasMemoryPool()
#ifndef EA_PLATFORM_PLAYSTATION2 // las dos PS2 no comprueban IsUserMemory; GameCube y PC si
        || (ptr != nullptr && !TheTrackStreamer.IsUserMemory(ptr))
#endif
    ) {
        bFree(ptr);
    } else if (ptr != nullptr) {
        TheTrackStreamer.FreeUserMemory(ptr);
    }
}

void MoviePlayer_StartUp() {
    if (gMoviePlayer == nullptr) {
        gMoviePlayer = new ("MoviePlayer", 0) MoviePlayer(0);
    }
}

void MoviePlayer_ShutDown() {
#ifndef EA_BUILD_A124
    gMovieStartTime = 0xFFFFFFFF;
#endif
    if (gMoviePlayer != nullptr) {
        delete gMoviePlayer;
        gMoviePlayer = nullptr;
    }
    TheTrackStreamer.RefreshLoading();
}

MoviePlayer::MoviePlayer(int memClass)
    : fCurFrameNum(0), fStatus(3), fLiveStatus(3), fPlayer(nullptr), CurFrame(nullptr) {
    RCMP::rcmp_sys.m_DefaultMemDir = memClass;
    RCMP::rcmp_sys.AllocMemFunc = RCMP_PlayerAllocAlign;
    RCMP::rcmp_sys.FreeMemFunc = RCMP_PlayerFree;
    RealShape::GraphObject::SetAllocator(&gShapeMemoryAllocator);
    if (TheTrackStreamer.HasMemoryPool()) {
#ifdef EA_PLATFORM_PLAYSTATION2
        TheTrackStreamer.MakeSpaceInPool(0x39D000, true); // las dos PS2
#else
        TheTrackStreamer.MakeSpaceInPool(0x271000, true);
#endif
    }
}

MoviePlayer::~MoviePlayer() {
    if (fPlayer != nullptr) {
        delete fPlayer;
    }
    fPlayer = nullptr;
    RCMP_PlayerFree(RCMPDecodeBuffer);
    RCMPDecodeBuffer = nullptr;
#ifndef EA_PLATFORM_PLAYSTATION2
    PlatFinishMovie();
#endif
}

void MoviePlayer::Init(Settings &newSettings) {
    mSettings = newSettings;
    mSettings.volume = GetMovieCategoryVolume();
    ResetTimer();
    HandleFatalError();
}

void MoviePlayer::ResetTimer() {
    mTicker = 0;
    mTickerFirstTime = true;
    mMoviePaused = false;
    mili_seconds = 0;
    seconds = 0;
    minutes = 0;
    milliseconds = 0.0f;
    prevMilliseconds = 0.0f;
}

void MoviePlayer::Play() {
#ifdef EA_BUILD_A124
    if (FMVScreenPrintf) {
        ScreenPrintf(-300, -80, 5.0f, "FMV: [%s]\n", mSettings.filename);
    }
#endif
    if (SkipMovies) {
        cFEng::Get()->QueueGameMessage(0xc3960eb9, nullptr, 0xff);
        return;
    }
    RCMP::AV_PLAYER::LOAD_ENUM loadType = mSettings.preload ? RCMP::AV_PLAYER::PRELOAD : RCMP::AV_PLAYER::STREAM;
    RCMP::AV_PLAYER::SOUND_ENUM soundType = !mSettings.sound ? RCMP::AV_PLAYER::SOUND_OFF : RCMP::AV_PLAYER::SOUND_ON;
    fPlayer = new RCMP::AV_PLAYER(mSettings.filename, mSettings.bufferSize, loadType, soundType);
    HandleFatalError();
    if (fPlayer == nullptr) {
        fStatus = 2;
        fLiveStatus = 2;
        return;
    }
#ifdef EA_PLATFORM_PLAYSTATION2
    DoWithMainMemoryStack((void *)GetFirstFrameBridge, 0, 0);
#else
    GetFirstFrame();
#endif
    if (mSettings.sound) {
        fPlayer->SetVol(mSettings.volume);
    }
    fCurFrameNum++;
    fPlayer->Pause();
#ifndef EA_BUILD_A124
    if (CurFrame == nullptr) {
        cFEng::Get()->QueueGameMessage(0xc3960eb9, nullptr, 0xff);
        return;
    }
#endif
#ifdef EA_PLATFORM_PLAYSTATION2
    if (RCMPDecodeBuffer == nullptr) {
        RealShape::Shape *shape = CurFrame->GetShape();
        int size = shape->GetWidth() * shape->GetHeight() * 4;
        RCMPDecodeBuffer = static_cast<unsigned int *>(RCMP_PlayerAllocAlign("RCMPDecodeBuffer", size, 0x80, 0, 0));
    }
#else
    PlatSetFirstMovieFrame(&MovieTextureInfo, CurFrame->GetShape(), mSettings.type == 0);
#endif
    SubTitler::NotifyFirstFrame();
    fStatus = 5;
    fLiveStatus = 5;
    fPlayer->UnPause();
#ifdef EA_BUILD_A124
    SoundPause(true, eSNDPAUSE_MOVIE);
    SetSoundControlState(true, SNDSTATE_FMV, "MoviePlay");
#endif
}

void MoviePlayer::Stop() {
    fStatus = 1;
    fLiveStatus = 1;
    ResetTimer();
#ifdef EA_BUILD_A124
    SoundPause(false, eSNDPAUSE_MOVIE);
    SetSoundControlState(false, SNDSTATE_FMV, "MoviePlay");
#endif
}

char *const MoviePlayer::GetMovieFilename() {
    return mSettings.filename;
}

int MoviePlayer::GetMovieCategoryVolume() {
    int volume = 0x7F;
    for (int i = 0; i < NUM_ELEMENTS(MovieVolumeArray); i++) {
        if (bStrNICmp(MovieVolumeArray[i].MovieBaseName, mSettings.filename, bStrLen(MovieVolumeArray[i].MovieBaseName)) == 0) {
            volume = MovieVolumeArray[i].Volume;
#ifndef EA_BUILD_A124
            if (ShutJosieUp && bStrNICmp(MovieVolumeArray[i].MovieBaseName, "MOVIES\\psa", bStrLen(MovieVolumeArray[i].MovieBaseName)) == 0) {
                volume = 0;
            }
#endif
        }
    }
    return volume;
}

#ifdef EA_PLATFORM_PLAYSTATION2
// 0x70000000 es el scratchpad del EE: se guarda en StackCopy mientras la funcion corre con la pila en la memoria
// principal, y se devuelve despues.
void MoviePlayer::UpdateBridgeFunction() {
    bMemCpy(gMoviePlayer->StackCopy, (void *)0x70000000, sizeof(gMoviePlayer->StackCopy));
    gMoviePlayer->UpdateFunction();
    bMemCpy((void *)0x70000000, gMoviePlayer->StackCopy, sizeof(gMoviePlayer->StackCopy));
}

void MoviePlayer::GetFirstFrameBridge() {
    bMemCpy(gMoviePlayer->StackCopy, (void *)0x70000000, sizeof(gMoviePlayer->StackCopy));
    gMoviePlayer->GetFirstFrame();
    bMemCpy((void *)0x70000000, gMoviePlayer->StackCopy, sizeof(gMoviePlayer->StackCopy));
}
#endif

void MoviePlayer::GetFirstFrame() {
    CurFrame = fPlayer->GetFirstFrame(RCMP_GetMaxFramesOutStanding(), GetMillisecondsPerFrame() * 2);
}

void MoviePlayer::Update() {
    if (fStatus == 5) {
#ifdef EA_PLATFORM_PLAYSTATION2
        TheDemoDiscManager.RegisterActivity();
        DoWithMainMemoryStack((void *)UpdateBridgeFunction, (int)this, 0);
#else
        UpdateFunction();
#endif
        int movie_done = (fLiveStatus != 5);
        if (Joylog::IsReplaying()) {
            int joylog_movie_done = Joylog::GetData(4, JOYLOG_CHANNEL_MOVIE_PLAYER_STATUS);
            if (joylog_movie_done != 0 && movie_done == 0) {
                while (fLiveStatus != 0) {
                    UpdateFunction();
                }
            }
            movie_done = joylog_movie_done;
        }
        Joylog::AddData(movie_done, 4, JOYLOG_CHANNEL_MOVIE_PLAYER_STATUS);
        if (movie_done != 0) {
            fStatus = fLiveStatus;
            eWaitUntilRenderingDone();
            cFEng::Get()->QueueGameMessage(0xc3960eb9, nullptr, 0xff);
            SoundPause(false, eSNDPAUSE_MOVIE);
            SetSoundControlState(false, SNDSTATE_PAUSE, "movie_done");
            if (fPlayer != nullptr) {
                delete fPlayer;
            }
            fPlayer = nullptr;
            RCMP_PlayerFree(RCMPDecodeBuffer);
            RCMPDecodeBuffer = nullptr;
            ResetTimer();
        }
        HandleFatalError();
    }
}

void MoviePlayer::UpdateFunction() {
    static int recurse = 0;
    if (recurse == 0) {
        int finished = 0;
        recurse = 1;
        bool MovieFinished = false;
        SYNCTASK_run();
        THREAD_yield(0);
        if (fPlayer->IsTimeForDecode() && CurFrame != nullptr) {
            fPlayer->GetDecoder()->ReleaseFrame(CurFrame);
            float goalFrame = fPlayer->GetGoalFrame();
            CurFrame = fPlayer->GetFrame(goalFrame);
        } else {
            MovieFinished = true;
        }
        if (CurFrame == nullptr) {
            finished = fPlayer->IsAudioFinished();
        } else {
            RealShape::Shape *shape = CurFrame->GetShape();
            if (!MovieFinished) {
                this->FillInTextureInfo(RCMPDecodeBuffer, &MovieTextureInfo, shape);
            }
        }
        HandleFatalError();
        if (finished != 0) {
            eWaitUntilRenderingDone();
            fLiveStatus = 0;
        }
        recurse = 0;
    }
}

uint32 MoviePlayer::GetMillisecondsPerFrame() {
    if (GetVideoMode() == 0) {
        return 20;
    }
    return 16;
}

void MoviePlayer::HandleFatalError() {}

TextureInfo *MoviePlayer::GetTexture() {
    if (fStatus == 5) {
        return &MovieTextureInfo;
    }
    return GetTextureInfo(0, 1, 0);
}

bool GiveTheMoviePlayerBandwidth() {
#ifdef EA_PLATFORM_WIN32
    // PC emite cmp/test por igualdad (r21: 39,6 -> 98,9 %); el resto, la forma original.
    if (gMoviePlayer != nullptr) {
        int status = gMoviePlayer->GetStatus();
        if (status == 3 || status == 4 || status == 5) {
            return true;
        }
    }
    return false;
#else
    bool result = false;
    if (gMoviePlayer != nullptr) {
        int status = gMoviePlayer->GetStatus();
        status -= 3;
        result = static_cast<unsigned int>(status) < 3u;
    }
    return result;
#endif
}
