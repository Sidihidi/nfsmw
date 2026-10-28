#include "Speed/Indep/Src/EAXSound/CARSFX/SFXObj_Reverb.hpp"
#include "Speed/Indep/Src/EAXSound/CARSFX/SFXObj_Enums.hpp"
#include "Speed/Indep/Src/EAXSound/EAXAemsManager.h"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/World/ParameterMaps.hpp"

// gcdatos17: en las DOS PS2 ReverbAccessor va delante del TypeInfo de SFXObj_Reverb,
// no detras de csfxedit como en las demas versiones. A124: 0x5e5448 = zEAXSound2
// .data +0xb98, delante de _13SFXObj_Reverb$s_TypeInfo (+0xbb8 = 0x5e5468); SLUS: los
// 28 B del bloque de ceros de +0xbb8 = 0x541d58, tambien delante del TypeInfo de
// +0xbd8. Los stabs lo confirman por el orden de declaracion: ReverbAccessor es
// Decl: 25 y ReverbZoneCrossMap/csfxedit Decl: 45/61. En el fichero hay dos objetos
// con ctor (este y m_EchoAllocs) y siguen en el mismo orden entre si, asi que el
// init de la unidad no cambia; solo se mueve .data.
#ifdef EA_PLATFORM_PLAYSTATION2
ParameterAccessor ReverbAccessor("Reverb"); // size: 0x1C, address: 0x8045E564, Decl: 25
#endif

DEFINE_CREATABLE(0x20100, SFXObj_Reverb, SndBase);

// size: 0x30, address: 0x80418214, Decl: 45
eREVERBFX ReverbZoneCrossMap[12] = {
    RVRB_CARSHOW_SML,       RVRB_CARSHOW_BASS, RVRB_CARSHOW_BASS_2, RVRB_GARAGE,   RVRB_GARAGE_SML, RVRB_SIMPLE_TUNNEL,
    RVRB_SIMPLE_TUNNEL_SML, RVRB_HILLS_CLOSE,  RVRB_HILLS,          RVRB_CITYOPEN, RVRB_CITYDENSE,  RVRB_ALLEY,
};

// size: 0x30, address: 0x80418244, Decl: 61
char *csfxedit[12] = {
    "City_Dense.fx",        "Alley.fx",       "City_Open.fx", "Garage.fx",    "Garage_sml.fx", "Simple_Tunnel.fx",
    "Simple_Tunnel_Sml.fx", "Hills_Close.fx", "Hills.fx",     "City_Open.fx", "City_Dense.fx", "Alley.fx",
};

#ifndef EA_PLATFORM_PLAYSTATION2
ParameterAccessor ReverbAccessor("Reverb"); // size: 0x1C, address: 0x8045E564, Decl: 25
#endif

Snd::GlobalFxProcessor *SFXObj_Reverb::m_pFXEditModule[2]; // Decl: 79

char *SFXObj_Reverb::m_pFXEditPatch[12]; // Decl: 88

void *SFXObj_Reverb::m_EchoBuffer = nullptr;   // Decl: 90
void *SFXObj_Reverb::m_UnusedBuffer = nullptr; // Decl: 91
bool SFXObj_Reverb::bUnavailable = true;       // Decl: 92

SFXObj_Reverb::ReverbStructure SFXObj_Reverb::m_EchoAllocs[4]; // Decl: 94

SFXObj_Reverb::SFXObj_Reverb() : CARSFX() {
#ifdef EA_PLATFORM_PLAYSTATION2
    if (this->m_EchoBuffer == nullptr) {
        this->m_EchoBuffer = gAudioMemoryManager.AllocateMemory(0xA000, "ReverbBuffer", false);
        this->m_UnusedBuffer = this->m_EchoBuffer;
        this->bUnavailable = false;
    }
#endif

    for (int n = 0; n < NUM_ELEMENTS(this->m_pFXEditPatch); n++) {
        this->m_pFXEditPatch[n] = nullptr;
    }
}

SFXObj_Reverb::~SFXObj_Reverb() {
    Destroy();
}

int SFXObj_Reverb::GetController(int Index) {
#ifdef EA_PLATFORM_PLAYSTATION2
    return Index == 0 ? 6 : -1;
#else
    return Index != 0 ? -1 : 6;
#endif
}

void SFXObj_Reverb::AttachController(SFXCTL *psfxctl) {
    if (psfxctl->GetObjectIndex() == 6) {
        m_pTunnelCtl = static_cast<SFXCTL_Tunnel *>(psfxctl);
    }
}

void SFXObj_Reverb::SetupLoadData() {
    for (int n = 0; n < NUM_ELEMENTS(csfxedit); n++) {
        LoadAsset(Attrib::StringKey(csfxedit[n]), SNDPATH_FXEDIT, SDT_GENERIC_DATA, eBANK_SLOT_NONE, true);
    }
}

void SFXObj_Reverb::SetupSFX(CSTATE_Base *_StateBase) {
    SndBase::SetupSFX(_StateBase);
    if (this->m_pStateBase->m_InstNum == 0) {
        this->m_pFXEditModule[0] = nullptr;
        this->m_pFXEditModule[1] = nullptr;
    }
}

// TODO
void SFXObj_Reverb::InitSFX() {
    SndBase::InitSFX();

    for (int n = 0; n < 12; n++) {
        int index = gAEMSMgr.IsAssetLoaded(Attrib::StringKey(csfxedit[n]));

        if (index < 0) {
            this->Disable();
            return;
        }

        m_pFXEditPatch[n] = static_cast<char *>(g_SndAssetList[index].pmem);
    }

    this->Enable();
    if (g_pEAXSound->GetSndGameMode() == SND_FRONTEND) {
        if (this->m_pFXEditModule[0] != nullptr) {
            this->m_pFXEditModule[0]->Release();
            this->m_pFXEditModule[0] = nullptr;
        }
#ifdef EA_PLATFORM_XENON
        Snd::GlobalFxProcessor::CreateInstance(Snd::DEVICE_MAIN, &m_pFXEditModule[0]);
#else
        Snd::GlobalFxProcessor::CreateInstance(Snd::DEVICE_MAIN, 0, &m_pFXEditModule[0]);
#endif
        m_pFXEditModule[0]->SetCustom(m_pFXEditPatch[3]);
    } else {
        if (this->m_pFXEditModule[0] != nullptr) {
            m_pFXEditModule[0]->Release();
            m_pFXEditModule[0] = nullptr;
        }
#ifdef EA_PLATFORM_XENON
        Snd::GlobalFxProcessor::CreateInstance(Snd::DEVICE_MAIN, &m_pFXEditModule[0]);
#else
        Snd::GlobalFxProcessor::CreateInstance(Snd::DEVICE_MAIN, 0, &m_pFXEditModule[0]);
#endif
        this->m_pFXEditModule[0]->SetCustom(m_pFXEditPatch[8]);
        m_pTunnelCtl->SetCurrentReverbType(RVRB_HILLS, 0);
    }
}

void SFXObj_Reverb::UpdateParams(float t) {
    if (IsEnabled()) {
        if (m_pTunnelCtl->m_IsLeadCar) {
            if (g_pEAXSound->GetSndGameMode() != SND_FRONTEND && g_pEAXSound->GetSndGameMode() != SND_CARSHOW) {
                int ndmixverb = this->GetDMixOutput(0, DMX_VOL);
                this->m_pTunnelCtl->m_AEMSWetVol = ndmixverb * m_pTunnelCtl->m_AEMSWetVol >> 15;
                this->m_pTunnelCtl->m_GinsuWetVol = ndmixverb * m_pTunnelCtl->m_GinsuWetVol >> 15;
            }
        }
    }
}

// TODO
void SFXObj_Reverb::ProcessUpdate() {
    if (IsEnabled() && m_pTunnelCtl->m_IsLeadCar && g_pEAXSound->GetSndGameMode() != SND_FRONTEND && g_pEAXSound->GetSndGameMode() != SND_CARSHOW &&
        m_pTunnelCtl->bIsReadyForSwitch && (m_pFXEditModule[0] != nullptr)) {
        SNDSYS_service();
        m_pFXEditModule[0]->Reset();
        m_pFXEditModule[0]->SetCustom(m_pFXEditPatch[m_pTunnelCtl->m_ReverbType]);
    }
}

// TODO
void SFXObj_Reverb::Destroy() {
    if (this->m_pFXEditModule[0] != nullptr) {
        this->m_pFXEditModule[0]->Release();
        this->m_pFXEditModule[0] = nullptr;
    }

    if (this->m_pFXEditModule[1] != nullptr) {
        m_pFXEditModule[1]->Release();
        m_pFXEditModule[1] = nullptr;
    }

    for (int n = 0; n < NUM_ELEMENTS(this->m_pFXEditPatch); n++) {
        this->m_pFXEditPatch[n] = nullptr;
    }

    if (this->m_EchoBuffer != nullptr) {
        gAudioMemoryManager.FreeMemory(this->m_EchoBuffer);
        this->m_EchoBuffer = nullptr;
    }
}

static const int DEBUG_REVERB_BUFFER = 0; // size: 0x4, Decl: 367

// STRIPPED en GameCube (Range 0xFFFFFFFF en el DWARF); las dos PS2 la tienen
void *SFXObj_Reverb::AllocReverbBuffer(char *name, int size) {
    int index;
    unsigned int unalignedaddr;
    void *ptr;

    if (m_EchoBuffer == nullptr) {
        return nullptr;
    }
    if (bUnavailable) {
        return nullptr;
    }
    if (size + (static_cast<char *>(m_UnusedBuffer) - static_cast<char *>(m_EchoBuffer)) > 0xA000) {
        return nullptr;
    }
    index = -1;
    for (int n = 0; n < NUMBER_REVERB_ALLOCS && index == -1; n++) {
        if (m_EchoAllocs[n].Alloc == nullptr) {
            index = n;
        }
    }
    if (index == -1) {
        return nullptr;
    }
    unalignedaddr = reinterpret_cast<unsigned int>(m_UnusedBuffer);
    ptr = reinterpret_cast<void *>((unalignedaddr + 15) & ~15);
    m_UnusedBuffer = static_cast<char *>(ptr) + size;
    m_EchoAllocs[index].Alloc = ptr;
    m_EchoAllocs[index].Size = size;
    return ptr;
}

// STRIPPED en GameCube (Range 0xFFFFFFFF en el DWARF); las dos PS2 la tienen
bool SFXObj_Reverb::FreeReverbBuffer(void *ptr) {
    int index = -1;
    bool FoundAlloc;
    bool OtherAllocsStillExists;

    FoundAlloc = OtherAllocsStillExists = false;

    for (int n = 0; n < NUMBER_REVERB_ALLOCS; n++) {
        if (m_EchoAllocs[n].Alloc == ptr) {
            index = n;
            FoundAlloc = true;
        } else if (m_EchoAllocs[n].Alloc != nullptr) {
            OtherAllocsStillExists = true;
        }
    }
    if (!FoundAlloc) {
        return false;
    }
    if (OtherAllocsStillExists) {
        bUnavailable = true;
    } else {
        bUnavailable = false;
        m_UnusedBuffer = m_EchoBuffer;
    }
    m_EchoAllocs[index].Clear();
    return true;
}
