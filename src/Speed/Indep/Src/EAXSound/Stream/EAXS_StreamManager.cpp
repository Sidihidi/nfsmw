#include "Speed/Indep/Src/EAXSound/Stream/EAXS_StreamManager.h"

#include "Speed/Indep/Src/EAXSound/Stream/EAXS_StreamChannel.h"
#include "Speed/Indep/Src/EAXSound/EAXSoundTypes.h"
#include "Speed/Indep/Src/EAXSound/AudioMemoryManager.hpp"
#include "Speed/Indep/Src/World/CarLoader.hpp"
#include "Speed/Indep/Src/World/TrackStreamer.hpp"

#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_WIN32) // las dos PS2 y PC: buffers mayores y 16 trozos para la voz
#define COPSPEECH_STREAMBUFFERSIZE 65536
#define COPSPEECH_MAXCHUNKS 16
#define MUSIC_STREAMBUFFERSIZE 163840
#define NISSFX_STREAMBUFFERSIZE 131072
#else
#define COPSPEECH_STREAMBUFFERSIZE 49152
#define COPSPEECH_MAXCHUNKS 8
#define MUSIC_STREAMBUFFERSIZE 98304
#define NISSFX_STREAMBUFFERSIZE 65536
#endif

#if defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_BUILD_A124) // SLUS: dos bloques mas por canal (sus .data valen 2)
int INCREASE_MUSICSTREAM_BLOCKS = 2;
int INCREASE_NISSFXSTRM_BLOCKS = 2;
#elif !defined(EA_BUILD_A124) // la A124 no los tiene (EAXS_StreamManager.h)
int INCREASE_MUSICSTREAM_BLOCKS = 0;
int INCREASE_NISSFXSTRM_BLOCKS = 0;
#endif
#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_GAMECUBE)
// gcdatos18: en las DOS PS2 el orden es [Songs][gpEAXS_StrmMgr][GN_HACK_STREAM_TYPE]
// (A124: PS2_globals.hpp 0x5e3448/0x5e3454/0x5e3458 = zEAXSound .data
// +0x2008/+0x2014/+0x2018, delante de utickreadcallback; SLUS cierra igual por
// bloques, gcdatos17 §3). Songs es el unico objeto con ctor del fichero, asi que el
// init de la unidad no cambia; solo se mueve .data.
// gcdatos14/17: GN_HACK_STREAM_TYPE, -1 en su .data objetivo (SLUS: el ff ff ff ff
// de +0x1fc8 = 0x53fd10).
// gcdatos19: GC igual (zEAXSound .data: gpEAXS_StrmMgr +0x2d8, el ff ff ff ff de
// +0x2dc y utickreadcallback +0x2e0; el DWARF de GC lo declara int). La nota de
// gcdatos14 que lo daba por ausente en GC era de antes de mirar este hueco.
// PC/X360 conservan la rama #else token a token.
Sound::SongInfoList Songs;
EAXS_StreamManager *gpEAXS_StrmMgr = nullptr;
int GN_HACK_STREAM_TYPE = -1;
#else
EAXS_StreamManager *gpEAXS_StrmMgr = nullptr;

Sound::SongInfoList Songs;
#endif

bool IsWorldDataStreaming(unsigned int strmhandle);
void AssignAudioStreamHandle(unsigned int strmhandle);

bool IsWorldDataStreaming(unsigned int strmhandle) {
    bool streaming = false;

    if (strmhandle != 0) {
        unsigned int poolbase = reinterpret_cast<unsigned int>(gAudioMemoryManager.m_pMemoryPoolMem);
        unsigned int poolend = poolbase + static_cast<unsigned int>(gAudioMemoryManager.m_memoryPoolSize);

        if (strmhandle > poolbase && strmhandle < poolend) {
            if (TheCarLoader.IsLoadingInProgress() != 0 || TheTrackStreamer.IsLoadingInProgressNonRepeatable()) {
                streaming = true;
            }
        }
    } else if (TheCarLoader.IsLoadingInProgress() != 0 || TheTrackStreamer.IsLoadingInProgressNonRepeatable()) {
        streaming = true;
    }

    return streaming;
}

void AssignAudioStreamHandle(unsigned int realstrmhandle) {
    unsigned int nStartAudioMemPool;
    unsigned int nEndAudioMemPool;

    nStartAudioMemPool = reinterpret_cast<unsigned int>(gAudioMemoryManager.m_pMemoryPoolMem);
    nEndAudioMemPool = nStartAudioMemPool + static_cast<unsigned int>(gAudioMemoryManager.m_memoryPoolSize);
    if (realstrmhandle > nStartAudioMemPool && realstrmhandle < nEndAudioMemPool) {
        unsigned int buffstart;
        unsigned int buffsize;
        buffstart = realstrmhandle;
        buffsize = nEndAudioMemPool;
        nEndAudioMemPool = realstrmhandle;
    }
}

EAXS_StreamManager::EAXS_StreamManager() {
    this->m_nNumStreamsAdded = 0;
    this->m_nWDRCount = 0;
    this->m_bIsWDRStreaming = false;
    this->m_bWasWDRStreaming = false;
    this->m_nTickStartZoneChange = 0;

    for (int i = 0; i < 4; i++) {
        this->m_pStrmCh[i] = nullptr;
    }
}

EAXS_StreamManager::~EAXS_StreamManager() {
    for (int i = 0; i < 4; i++) {
        if (this->m_pStrmCh[i] != nullptr) {
            delete this->m_pStrmCh[i];
            this->m_pStrmCh[i] = nullptr;
        }
    }
}

void EAXS_StreamManager::InitializeStreams(eGAMEMODE gamemode) {
    gpEAXS_StrmMgr = this;

    switch (gamemode) {
        case SNDGM_SPLITSCREEN:
            this->m_pStrmCh[STYPE_MUSIC] = new ("AUD:PF MUSIC Stream Channel", STRMCHANNEL_LINE) EAXS_StreamChannel();
            this->m_pStrmCh[STYPE_MUSIC]->InitParams(this);
            this->m_pStrmCh[STYPE_MUSIC]->InitChannel(8, 32,
                                                      MUSIC_STREAMBUFFERSIZE + INCREASE_MUSICSTREAM_BLOCKS * STREAMBLOCK_SIZE,
                                                      STYPE_MUSIC);

            this->m_pStrmCh[STYPE_NISSFX] = new ("AUD:NIS/SFX Stream Channel", STRMCHANNEL_LINE) EAXS_StreamChannel();
            this->m_pStrmCh[STYPE_NISSFX]->InitParams(this);
            this->m_pStrmCh[STYPE_NISSFX]->InitChannel(8, 32,
                                                       NISSFX_STREAMBUFFERSIZE + INCREASE_NISSFXSTRM_BLOCKS * STREAMBLOCK_SIZE,
                                                       STYPE_NISSFX);
            break;

        case SNDGM_FREEROAM:
        case SNDGM_RACE:
        case SNDGM_FRONTEND:
        default:
            this->m_pStrmCh[STYPE_COPSPEECH] = new ("AUD:Speech Stream Channel", STRMCHANNEL_LINE) EAXS_StreamChannel();
            this->m_pStrmCh[STYPE_COPSPEECH]->InitParams(this);
            this->m_pStrmCh[STYPE_COPSPEECH]->InitChannel(COPSPEECH_MAXCHUNKS, 32, COPSPEECH_STREAMBUFFERSIZE, STYPE_COPSPEECH);

            this->m_pStrmCh[STYPE_MUSIC] = new ("AUD:PF MUSIC Stream Channel", STRMCHANNEL_LINE) EAXS_StreamChannel();
            this->m_pStrmCh[STYPE_MUSIC]->InitParams(this);
            this->m_pStrmCh[STYPE_MUSIC]->InitChannel(8, 32,
                                                      MUSIC_STREAMBUFFERSIZE + INCREASE_MUSICSTREAM_BLOCKS * STREAMBLOCK_SIZE,
                                                      STYPE_MUSIC);

            this->m_pStrmCh[STYPE_NISSFX] = new ("AUD:NIS/SFX Stream Channel", STRMCHANNEL_LINE) EAXS_StreamChannel();
            this->m_pStrmCh[STYPE_NISSFX]->InitParams(this);
            this->m_pStrmCh[STYPE_NISSFX]->InitChannel(8, 32,
                                                       NISSFX_STREAMBUFFERSIZE + INCREASE_NISSFXSTRM_BLOCKS * STREAMBLOCK_SIZE,
                                                       STYPE_NISSFX);
            break;
    }
}

EAXS_StreamChannel *EAXS_StreamManager::GetStreamChannel(int nchannel) {
    if (nchannel < 4) {
        return this->m_pStrmCh[nchannel];
    }
    return nullptr;
}

void EAXS_StreamManager::AddStreamChannel(EAXS_StreamChannel *pstrmchannel, eSTRMTYPE strmtype) {
    if (this->m_pStrmCh[strmtype] == nullptr) {
        this->m_pStrmCh[strmtype] = pstrmchannel;
    }
}

void EAXS_StreamManager::RemoveStreamChannel(eSTRMTYPE strmtype) {}
