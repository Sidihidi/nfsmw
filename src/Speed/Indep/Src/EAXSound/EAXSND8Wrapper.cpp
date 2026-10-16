#include "Speed/Indep/Src/EAXSound/EAXSND8Wrapper.hpp"
#include <types.h>
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"
#include "Speed/Indep/bWare/Inc/bMemory.hpp"


#include "Speed/Indep/Src/EAXSound/PF_iallocatorimpl.h"
#include "Speed/Indep/Libs/snd/9/source/library/cmn/sndenum.h"
#include "snd/sndo.h"

#include "Speed/Indep/Src/EAXSound/CSISAllocator.h"

extern int nCsisSlotPoolSizes[];
// .bss:0x8045DDB4 y 0x8045DDB8, 4 B cada una: un solo elemento, y el codigo
// de este fichero solo usa el indice 0.
SlotPool *pCsisSlotPools[1];
int nCsisSlotPoolSizes[1];
extern int AudioMemoryPool;
int MAIN_SAMPLERATE = 0x7D00;
// r75: fuera extern SNDSYS_service duplicado; snd/sndo.h ya lo declara (include propio).

CSISCoreAllocator g_CSISCoreAllocator;

EAXSND8Wrapper::EAXSND8Wrapper() {
    this->m_pSoundHeap = nullptr;
    this->m_pStreamBuff = nullptr;
    pCsisSlotPools[0] = nullptr;
    nCsisSlotPoolSizes[0] = 0x88;
}

EAXSND8Wrapper::~EAXSND8Wrapper() {
    bDeleteSlotPool(pCsisSlotPools[0]);
    pCsisSlotPools[0] = nullptr;
}

// Como EAXSound::RestoreDriver: en el fuente de todas las versiones y solo enlazada en la A124 (EAXSND8Wrapper.cpp:113).
void EAXSND8Wrapper::RestoreSoundDriver() {
    Snd::System::Restore();
}

bool EAXSND8Wrapper::Initialize() {
    if (IsSoundEnabled == 0) {
        IsAudioStreamingEnabled = IsSpeechEnabled = IsNISAudioEnabled = IsSoundEnabled;
        return false;
    }

    pCsisSlotPools[0] =
        static_cast<SlotPool *>(bNewSlotPool(nCsisSlotPoolSizes[0], 0x100, "AUD:Csis SlotPools", AudioMemoryPool));
    Csis::System::SetAllocator(static_cast<EA::Allocator::ICoreAllocator *>(static_cast<void *>(&g_CSISCoreAllocator)));
    Csis::System::Init();
    Snd::System::VectorToCsisMutex();
    Snd::System::VectorToReal6();
    Snd::System::SetMaxBanks(0x20);

    this->m_nHeapSize = 0x33000;
    this->m_pSoundHeap = gAudioMemoryManager.AllocateMemoryChar(this->m_nHeapSize, "SND Heap", false);
    Snd::System::SetOutputSampleRate(Snd::DEVICE_MAIN, MAIN_SAMPLERATE);

    this->SetAudioRenderMode(this->m_eLastAudioMode = this->m_eCurrentAudioMode = this->GetDefaultPlatformAudioMode());

    Snd::System::SetVoices(Snd::DEVICE_MAIN, 8);
    Snd::System::SetOutputSampleRate(Snd::DEVICE_IOP, 32000);
    Snd::System::SetOutputSampleRate(Snd::DEVICE_MAIN, MAIN_SAMPLERATE);
    Snd::System::SetSndInitsAram(true);
    Snd::Memory::SetHeap(Snd::DEVICE_MAIN, this->m_pSoundHeap, this->m_nHeapSize);
    Snd::Memory::SetHeapThreshold(Snd::DEVICE_MAIN, 1.0f);
    Snd::System::Init(0x90600);
    return true;
}

void EAXSND8Wrapper::ReInit() {
    if (IsSoundEnabled == 0) {
#ifndef EA_BUILD_A124
        IsAudioStreamingEnabled = IsSpeechEnabled = IsNISAudioEnabled = IsSoundEnabled;
#endif
    } else {
        AudioSettings *settings = g_pEAXSound->GetCurAudioSettings();
        eSndAudioMode mode = static_cast<eSndAudioMode>(settings->AudioMode);
        this->m_eCurrentAudioMode = mode;
        this->SetSnd8RenderMode(mode);
        Snd::System::ReInit();
    }
}

void EAXSND8Wrapper::STUPID() {}

void EAXSND8Wrapper::Update() {
    this->STUPID();
}

eSndAudioMode EAXSND8Wrapper::SetAudioModeFromMemoryCard(eSndAudioMode mode) {
    if (IsSoundEnabled == 0) {
#ifndef EA_BUILD_A124
        IsAudioStreamingEnabled = IsSpeechEnabled = IsNISAudioEnabled = IsSoundEnabled;
#endif
        return mode;
    }

#ifdef EA_PLATFORM_GAMECUBE
    this->m_eCurrentAudioMode = this->GetDefaultPlatformAudioMode();
    switch (this->m_eCurrentAudioMode) {
    case AUDIO_MODE_MIN:
        mode = AUDIO_MODE_MIN;
        break;
    case AUDIO_MODE_STEREO:
        if (mode == AUDIO_MODE_MIN) {
            mode = AUDIO_MODE_STEREO;
        }
        break;
    default:
        mode = this->m_eCurrentAudioMode;
        break;
    }

    switch (mode) {
    case AUDIO_MODE_MIN:
        OSSetSoundMode(0);
        break;
    case AUDIO_MODE_STEREO:
        OSSetSoundMode(1);
        break;
    case AUDIO_MODE_MAX:
        OSSetSoundMode(1);
        break;
    default:
        break;
    }

    this->m_eCurrentAudioMode = mode;
    return mode;
#else
    // Las dos PS2 descartan el resultado y guardan el modo pedido (medido en
    // A124 y SLUS: llamada sin usar v0 y `sw mode,0x14(this)`).
    this->GetDefaultPlatformAudioMode();
    this->m_eCurrentAudioMode = mode;
    return mode;
#endif
}

eSndAudioMode EAXSND8Wrapper::SetAudioRenderMode(eSndAudioMode mode) {
    if (IsSoundEnabled == 0) {
#ifndef EA_BUILD_A124
        IsAudioStreamingEnabled = IsSpeechEnabled = IsNISAudioEnabled = IsSoundEnabled;
#endif
        return mode;
    }

#ifdef EA_PLATFORM_GAMECUBE
    switch (mode) {
    case AUDIO_MODE_MIN:
        OSSetSoundMode(0);
        break;
    case AUDIO_MODE_STEREO:
        OSSetSoundMode(1);
        break;
    case AUDIO_MODE_MAX:
        OSSetSoundMode(1);
        break;
    default:
        break;
    }
#endif

    this->m_eCurrentAudioMode = mode;
    this->SetSnd8RenderMode(mode);
    return this->m_eCurrentAudioMode;
}

eSndAudioMode EAXSND8Wrapper::SetSnd8RenderMode(eSndAudioMode mode) {
    if (IsSoundEnabled == 0) {
#ifndef EA_BUILD_A124
        IsAudioStreamingEnabled = IsSpeechEnabled = IsNISAudioEnabled = IsSoundEnabled;
#endif
        return mode;
    }

    switch (mode) {
    case AUDIO_MODE_MIN:
#ifndef EA_BUILD_A124 // la A124 pasa a mono sin pasar por estereo
        Snd::System::SetOutputMode(Snd::OUTPUTMODE_STEREO);
        SNDSYS_service();
#endif
        Snd::System::SetOutputMode(Snd::OUTPUTMODE_MONO);
        break;
    case AUDIO_MODE_STEREO:
        Snd::System::SetOutputMode(Snd::OUTPUTMODE_PROLOGIC2);
        break;
    case AUDIO_MODE_MAX:
        Snd::System::SetOutputMode(Snd::OUTPUTMODE_PROLOGIC2);
        break;
    default:
        break;
    }

    return mode;
}

eSndAudioMode EAXSND8Wrapper::GetDefaultPlatformAudioMode() {
    if (!IsSoundEnabled) {
        return AUDIO_MODE_STEREO;
    }
#ifdef EA_PLATFORM_GAMECUBE
    eSndAudioMode mode = AUDIO_MODE_STEREO;
    if (OSGetSoundMode() == 0) {
        mode = AUDIO_MODE_MIN;
    }
#elif defined(EA_PLATFORM_XENON) || defined(EA_PLATFORM_WIN32)
    eSndAudioMode mode = AUDIO_MODE_STEREO; // NFS.exe 0x822E9B48: AudioSettings::Default guarda 1; speed.exe 0x4B9240: mov eax, 1
#else
    eSndAudioMode mode = AUDIO_MODE_MAX;
#endif
    return mode;
}

inline void *PF_Allocator::Alloc(unsigned int size, const EA::TagValuePair &flags) {
    (void)flags;
    return gAudioMemoryManager.AllocateMemory(size, "AUD: Pathfinder alloc", true);
}
