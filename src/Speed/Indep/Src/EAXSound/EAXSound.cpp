#include "./EAXSOund.hpp"
#ifdef EA_PLATFORM_GAMECUBE
#include <dolphin/mtx.h>
#endif
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
void g_AdjustVolPause(int &Volume);

void DestroySoundDriver();

#include "Speed/Indep/Src/EAXSound/EAXAemsManager.h"
#include "Speed/Indep/Src/EAXSound/STICH_Playback.h"
#include "Speed/Indep/Src/EAXSound/EAXCar.hpp"
#include "Speed/Indep/Src/EAXSound/EAXCarState.hpp"
#include "Speed/Indep/Src/EAXSound/EAXSND8Wrapper.hpp"
#include "Speed/Indep/Src/EAXSound/NFSMixMaster.hpp"
#include "Speed/Indep/Src/EAXSound/Dynamic_Mixer/NFSLiveLink.hpp"
#include "Speed/Indep/Src/EAXSound/SimStates/EAX_HeliState.hpp"
#include "Speed/Indep/Src/EAXSound/CARSFX/SFXObj_NISStream.hpp"
#include "Speed/Indep/Src/EAXSound/CARSFX/SFXObj_Pathfinder.hpp"
#include "Speed/Indep/Src/EAXSound/CARSFX/SFXObj_Reverb.hpp"
#include "Speed/Indep/Src/EAXSound/SndCamera.hpp"
#include "Speed/Indep/Src/EAXSound/SoundConn.h"
#include "Speed/Indep/Src/EAXSound/Stream/EAXS_StreamManager.h"
#include "Speed/Indep/Src/EAXSound/Stream/EAXS_StreamChannel.h"
#include "Speed/Indep/Src/EAXSound/Stream/SpeechManager.hpp"
#include "Speed/Indep/Src/EAXSound/States/Managers/STATEMGR_AICar.hpp"
#include "Speed/Indep/Src/EAXSound/States/Managers/STATEMGR_CarState.hpp"
#include "Speed/Indep/Src/EAXSound/States/Managers/STATEMGR_Base.hpp"
#include "Speed/Indep/Src/EAXSound/States/Managers/STATEMGR_Collision.hpp"
#include "Speed/Indep/Src/EAXSound/States/Managers/STATEMGR_CopCar.hpp"
#include "Speed/Indep/Src/EAXSound/States/Managers/STATEMGR_DriveBy.hpp"
#include "Speed/Indep/Src/EAXSound/States/Managers/STATEMGR_Enviro.hpp"
#include "Speed/Indep/Src/EAXSound/States/Managers/STATEMGR_Helicopter.hpp"
#include "Speed/Indep/Src/EAXSound/States/Managers/STATEMGR_Main.hpp"
#include "Speed/Indep/Src/EAXSound/States/Managers/STATEMGR_Music.hpp"
#include "Speed/Indep/Src/EAXSound/States/Managers/STATEMGR_PlayerCar.hpp"
#include "Speed/Indep/Src/EAXSound/States/Managers/STATEMGR_TrafficCar.hpp"
#include "Speed/Indep/Src/EAXSound/States/Managers/STATEMGR_Truck.hpp"
#include "Speed/Indep/Src/EAXSound/sfxctl/SFXCTL_NISReving.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEMenuScreen.hpp"
#include "Speed/Indep/Src/Gameplay/GRace.h"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Generated/Messages/MControlPathfinder.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/engine.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/engineaudio.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/pvehicle.h"
#include "Speed/Indep/Src/Main/Scheduler.h"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Misc/QueuedFile.hpp"
#include "Speed/Indep/Src/Sim/Simulation.h"
#include "Speed/Indep/Src/World/WorldConn.h"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bDebug.hpp"
#include <new>

#include "Speed/Indep/Src/EAXSound/EAXCommon.hpp"
#include "Speed/Indep/Src/EAXSound/EAXFrontEnd.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Frontend/DebugMenu.h" // ScreenPrintf
#endif

Attrib::StringKey crcEngineAudio("EngineAudio");
Attrib::StringKey crcAudioSystem("AudioSystem");
Attrib::StringKey crcMostWanted("MostWanted");
Attrib::StringKey crcDrivetrain("Drivetrain");
Attrib::StringKey crcSkidParams("SkidParams");
Attrib::StringKey crcTurboSFX("TurboSFX");
Attrib::StringKey crcAccelTrans("AccelTrans");
Attrib::StringKey crcShiftPattern("ShiftPattern");
Attrib::StringKey crcSweetener("Sweetener");
Attrib::StringKey crcEnglish("English");
Attrib::StringKey crcCarHitWall("CarHitWall");
Attrib::StringKey crcLicensedMusic("LicensedMusic");
Attrib::StringKey crcMusic("Music");
EAXAemsManager gAEMSMgr;
stSndDataLoadParams g_SndAssetList[48];
Slope TablePitch(0.0f, 1.0f, 3500.0f, 4500.0f);
Slope BreakingPitchVsSpeed(3000.0f, 5000.0f, 30.0f, 100.0f);

unsigned int SoundRandomSeed = 0;
float g_SliderValue = -1.0f;
float g_fMasterSFXVolume = 1.0f;
int g_iMasterSFXVolume = 0x7FFF;
extern int g_DMIX_DummyInputBlock[];
char *g_pcsCSISAllocString = "CSISSOUND";
extern char csCSISdebug[];
// .bss:0x8045DDCC, 0x20 B. Estaba declarada `extern` y sin definir.
char csCSISdebug[32];
bool bIsMapInQueuedFileLoad = false;
unsigned int g_ActiveCtlStates = 0;
extern unsigned int g_laststartanimid;
extern bool g_bWasLastNISaStart;
bool RUN_SOUND_STATE = true;
unsigned int g_ActiveSFXStates = 0;
unsigned int g_PrevActiveCtlStates = 0;
unsigned int g_PrevActiveSFXStates = 0;
// Una entrada por estado de eSNDCTLSTATE: 15 en la A124 (sin STOP_MUSIC ni GAMESTARTRACE, y con otras mascaras para
// FE_UPSCREEN, FE_SMS_MESSAGE y FADEOUT), 17 en SLUS (sin SYSTEM_HUD) y 18 en GameCube (.data de cada binario).
unsigned int g_CtlStateActions[MAX_SNDCTL_STATES] = {
    0xFFFFFFFF, 63, 0, 28,
#ifdef EA_BUILD_A124
    184, 61,
#else
    190, 188,
#endif
    1145, 289, 512, 51, 2104, 127,
#ifndef EA_BUILD_A124
    1, 127,
#endif
    56,
#ifdef EA_BUILD_A124
    0,
#else
    0x1000,
#endif
    0xFFFFFFFF,
#ifndef EA_PLATFORM_PLAYSTATION2
    0xFFFFFFFF,
#endif
};
extern char *csfxedit[];
bool bHasStartNewGameOccured = false;
bool gbHasStartNewGamePlayBeenProcessed = false;
bool bIsAnFEToIngameTransition = false;
bool bHasDataLoadOccured = false;
int gIsPauseForPause = 0;
EAXSound *g_pEAXSound = nullptr;
void *NullPointer = nullptr;
int gnHasStartLoadFEBeenProcessed = 0;
int gb_DORESTART_RACE = 0;
int gb_Is321 = 0;
extern int SkipFE;
extern eLanguages SkipFELanguage;
int DisableSoundUpdate = 0;
extern bool gbAudioInterruptsWorldDataRead;
extern bool gbWorldDataBlocksAudioRead;
extern bool bReadCallbackToggle;
// .bss:0x8045DDBC / 0x8045DDC0 / 0x8045DDC4, 4 B cada una: declaradas
// `extern` y sin definir en todo el arbol.
bool gbAudioInterruptsWorldDataRead;
bool gbWorldDataBlocksAudioRead;
bool bReadCallbackToggle;
#ifdef EA_BUILD_A124
char prevSoundFile[50];
#endif
bool bStreamBlockState = false;
bool bAudioInterrupt = false;
bool bStreamReadTiming = false;
unsigned int uStreamBlockTicks = 0;
unsigned int uStreamReadTicks = 0;
unsigned int uAudioInterruptTicks = 0;
extern int GameFlowSndState[];
// .bss:0x8045DD78, 0x3C B = 15 enteros (EAXSound.cpp:1002 hace bMemSet de 0x3C).
int GameFlowSndState[15];
// r75: fuera extern SNDSYS_service duplicado; snd/sndo.h ya lo declara (via EAXS_StreamChannel.h).
void RegisterStates();
void RegisterSFX();

int bStrICmp(const char *s1, const char *s2);
void SoundPause(bool on, eSNDPAUSE_REASON reason);

#ifdef EA_BUILD_A124
// Solo la A124 (lineas 269-284 de su EAXSound.cpp): el nombre de cada motivo de pausa.
char *GetPauseReasonName(eSNDPAUSE_REASON state) {
    switch (state) {
        case ePAUSE_ERROR:
            return "ePAUSE_ERROR";
        case eSNDPAUSE_PAUSEMENU:
            return "eSNDPAUSE_PAUSEMENU";
        case eSNDPAUSE_SIMSTATE_IDLE:
            return "eSNDPAUSE_SIMSTATE_IDLE";
        case eSNDPAUSE_SIMSTATE_INIT:
            return "eSNDPAUSE_SIMSTATE_INIT";
        case eSNDPAUSE_SIMSTATE_ACTIVE:
            return "eSNDPAUSE_SIMSTATE_ACTIVE";
        case eSNDPAUSE_MEMCARD:
            return "eSNDPAUSE_MEMCARD";
        case eSNDPAUSE_ONLINE:
            return "eSNDPAUSE_ONLINE";
        case eSNDPAUSE_MOVIE:
            return "eSNDPAUSE_MOVIE";
        case eSNDPAUSE_QUITTOFE:
            return "eSNDPAUSE_QUITTOFE";
        case eSNDPAUSE_NISON:
            return "eSNDPAUSE_NISON";
        case eSNDPAUSE_PHOTOFINISH:
            return "eSNDPAUSE_PHOTOFINISH";
        case eSNDPAUSE_SMS_MESSAGE:
            return "eSNDPAUSE_SMS_MESSAGE";
    }
    return "UNKNOWN PAUSE REASON";
}
#endif

bool g_EAXIsPaused() {
#ifdef EA_BUILD_A124 // la A124 aun lleva la pausa en EAXSound
    return g_pEAXSound->m_bIsPaused;
#else
    // Los estados que paran el sonido; las dos PS2 no tienen SNDSTATE_SYSTEM_HUD (SLUS: 0x1483B).
    if ((g_ActiveCtlStates & SEND_STATE(SNDSTATE_OFF)) ||
        (g_ActiveCtlStates & SEND_STATE(SNDSTATE_PAUSE)) ||
        (g_ActiveCtlStates & SEND_STATE(SNDSTATE_FE)) ||
        (g_ActiveCtlStates & SEND_STATE(SNDSTATE_FE_UPSCREEN)) ||
        (g_ActiveCtlStates & SEND_STATE(SNDSTATE_FE_SMS_MESSAGE)) ||
        (g_ActiveCtlStates & SEND_STATE(SNDSTATE_FMV)) ||
        (g_ActiveCtlStates & SEND_STATE(SNDSTATE_MINILOAD)) ||
        (g_ActiveCtlStates & SEND_STATE(SNDSTATE_ERROR))
#ifndef EA_PLATFORM_PLAYSTATION2
        || (g_ActiveCtlStates & SEND_STATE(SNDSTATE_SYSTEM_HUD))
#endif
        ) {
        return true;
    }
    return false;
#endif
}

void EAXSound::START_321Countdown() {
    int id;
    SFXObj_Pathfinder *ppf;
#ifndef EA_BUILD_A124
    SFXObj_NISStream *pnis;
#endif

    if (!IsSoundEnabled || !IsAudioStreamingEnabled) {
        return;
    }
    id = 0x40010010;
    ppf = static_cast<SFXObj_Pathfinder *>(this->GetSFXBase_Object(id));
    if (ppf) {
        ppf->Set321(true);
    }
#ifndef EA_BUILD_A124
    id = 0x40000050;
    pnis = static_cast<SFXObj_NISStream *>(this->GetSFXBase_Object(id));
    if (pnis) {
        pnis->StartNIS();
    }
#endif
}

bool EAXSound::AreResourceLoadsPending() {
    return gAEMSMgr.AreResourceLoadsPending();
}

void EAXSound::QueueNISButtonThrough(unsigned int anim_id, int camera_track_number) {
    int id;
    SFXObj_NISStream *pnis;
    Speech::Module *cop_speech;

    if (!IsSoundEnabled || !IsAudioStreamingEnabled) {
        return;
    }

    id = 0x40000050;
    pnis = static_cast<SFXObj_NISStream *>(this->GetSFXBase_Object(id));
    if (camera_track_number == -1) {
#ifdef EA_BUILD_A124 // la A124 no excluye las dos NIS de salida
        pnis->QueueNISStream(g_laststartanimid, -1, true, false);
#else
        if (g_laststartanimid != 0x0D0E5A9D && g_laststartanimid != 0xCBFF6594) {
            pnis->QueueNISStream(g_laststartanimid, -1, true, false);
        }
#endif
    } else if (g_bWasLastNISaStart) {
        pnis->QueueNISStream(anim_id, camera_track_number, true, false);
    } else {
        pnis->StopStream();
    }

    Speech::Manager::ClearPlayback();
    id = COPSPEECH_MODULE;
    cop_speech = Speech::Manager::GetSpeechModule(id);
    if (cop_speech != nullptr) {
        cop_speech->ReleaseResource();
    }
}

void EAXSound::QueueNISStream(unsigned int anim_id, int camera_track_number, void (*setmstimecb)(unsigned int, int)) {
    int id;
    SFXObj_NISStream *pnis;

    if (IsSoundEnabled && IsAudioStreamingEnabled) {
        id = 0x40000050;
        pnis = static_cast<SFXObj_NISStream *>(this->GetSFXBase_Object(id));
        pnis->QueueNISStream(anim_id, camera_track_number, setmstimecb, false);
    }
}

bool EAXSound::IsNISStreamQueued() {
    int id;
    SFXObj_NISStream *pnis;

    if (!IsSoundEnabled || !IsAudioStreamingEnabled) {
        return true;
    }
    id = 0x40000050;
    pnis = static_cast<SFXObj_NISStream *>(this->GetSFXBase_Object(id));
    return pnis->IsNISStreamReady();
}

void EAXSound::NISFinished() {
    int id;
    SFXObj_NISStream *pnis;

    if (IsSoundEnabled && IsAudioStreamingEnabled) {
        id = 0x40000050;
        pnis = static_cast<SFXObj_NISStream *>(this->GetSFXBase_Object(id));
        pnis->NISActivityDone();
        SoundPause(false, eSNDPAUSE_NISON);
        SetSoundControlState(false, SNDSTATE_NIS_STORY, "EAXSound::NISFinished");
        SetSoundControlState(false, SNDSTATE_NIS_INTRO, "EAXSound::NISFinished");
        SetSoundControlState(false, SNDSTATE_NIS_BLK, "EAXSound::NISFinished");
        SetSoundControlState(false, SNDSTATE_NIS_ARREST, "EAXSound::NISFinished");
    }
}

// NISFinished goes here when implemented

void EAXSound::PlayNIS() {
    int id;
    SFXObj_NISStream *pnis;

    if (IsSoundEnabled && IsAudioStreamingEnabled) {
        id = 0x40000050;
        pnis = static_cast<SFXObj_NISStream *>(this->GetSFXBase_Object(id));
        pnis->StartNIS();
        SoundPause(true, eSNDPAUSE_NISON);
#ifdef EA_BUILD_A124
        // Solo la A124: encadena SetSoundControlState(true, SNDSTATE_NIS_INTRO, "NIS:Type?") con tail-call
        // (A124 0x5c0: SoundPause jal + SetSoundControlState j; SLUS y GC terminan en SoundPause).
        SetSoundControlState(true, SNDSTATE_NIS_INTRO, "NIS:Type?");
#endif
    }
}

void EAXSound::PlayUISoundFX(eMenuSoundTriggers etriggertype) {
    if (IsSoundEnabled) {
#ifdef EA_BUILD_A124
        if (DEBUG_PRINT_FE_SND) {
            ScreenPrintf(-300, -80, 15.0f, "Prev FE Sound Played: %s
", prevSoundFile);
            ScreenPrintf(-300, -65, 15.0f, "FE Sound Played: %s
", GetFESoundName(etriggertype));
            bSafeStrCpy(prevSoundFile, GetFESoundName(etriggertype), 50);
        }
#endif
        if (etriggertype < UISND_COMMON_MAX_NUM) {
            if (this->m_pCmnSnd != nullptr) {
                this->m_pCmnSnd->Play(etriggertype);
            }
        } else if (etriggertype < UISND_FRONTEND_MAX_NUM) {
            etriggertype = static_cast<eMenuSoundTriggers>(etriggertype - UISND_COMMON_MAX_NUM);
            if (this->m_pFESnd != nullptr) {
                this->m_pFESnd->Play(etriggertype);
            }
        }
    }
}

// PlayUISoundFX, StopUISoundFX go here when implemented

void EAXSound::StopUISoundFX(eMenuSoundTriggers etriggertype) {
    if (IsSoundEnabled) {
        if (etriggertype < UISND_COMMON_MAX_NUM) {
            if (this->m_pCmnSnd != nullptr) {
                this->m_pCmnSnd->Stop(etriggertype);
            }
        } else {
            etriggertype = static_cast<eMenuSoundTriggers>(etriggertype - UISND_COMMON_MAX_NUM);
            if (this->m_pFESnd != nullptr) {
                this->m_pFESnd->Stop(etriggertype);
            }
        }
    }
}

void EAXSound::SetCsisName(SndBase *psndbase) {
    int ninst = psndbase->GetUniqueID();
    bSPrintf(csCSISdebug, " %s, 0x%x ", psndbase->GetTypeName(), ninst);
    this->SetCsisName(csCSISdebug);
}

void EAXSound::SetCsisName(char *pcsAllocName) {
    g_pcsCSISAllocString = pcsAllocName;
}

EAXSound::EAXSound() {
    int nloop;

    this->m_pcsCsisName = "SOUND";
    this->bPlayCarSounds = true;
    this->mAttributes = nullptr;
    this->mLocalAttr = nullptr;
    this->m_pEAXSND8Wrapper = nullptr;
    this->m_bAudioIsPaused = false;
#ifndef EA_BUILD_A124
    this->m_X360_UI_Override = false;
#endif
    this->m_eSndGameMode = SND_MODE_NONE;
    this->m_prevSndGameMode = SND_MODE_NONE;
    this->m_pStreamManager = nullptr;
    this->m_nStereoUpgradeLevel = 0;
    this->m_bIsPaused = false;
    this->m_pCurAudioSettings = nullptr;
    this->m_nSpeechLoadBankIndex = 0;
    this->m_ePlayerMixMode = EAXS3D_SINGLE_PLAYER_MIX;
    this->m_pNewSongInfoSt = nullptr;
    this->EngineLoadingBlocked = false;
    this->FrameCnt = 0;
    this->m_bIsSpecialUGMovie = false;
    this->m_pCmnSnd = nullptr;
    this->m_pFESnd = nullptr;
    this->m_pSTICH_Playback = nullptr;
    this->m_pNFSLiveLink = nullptr;
    this->m_pNFSMixMaster = nullptr;
    this->bPlayCameraSnapShot = false;
    this->mmsgMRestartRace = nullptr;
#ifndef EA_BUILD_A124
    this->m_bPause_MainFNG = false;
#endif
    this->mEventID = 0;
    this->mData.fEventID = 0xF2D10992; // TODO magic
    nloop = 0;
}

EAXSound::~EAXSound() {
    gSpeechCache.Dump();

    if (this->mAttributes) {
        delete this->mAttributes;
        this->mAttributes = nullptr;
    }

    if (this->mLocalAttr) {
        delete this->mLocalAttr;
        this->mLocalAttr = nullptr;
    }

    if (this->m_pCmnSnd) {
        delete this->m_pCmnSnd;
        this->m_pCmnSnd = nullptr;
    }

    if (this->m_pFESnd) {
        delete this->m_pFESnd;
        this->m_pFESnd = nullptr;
    }

    if (this->m_pEAXSND8Wrapper) {
        delete this->m_pEAXSND8Wrapper;
        this->m_pEAXSND8Wrapper = nullptr;
    }

    if (g_pNISRevMgr) {
        delete g_pNISRevMgr;
    }
}

int *EAXSound::GetPointerCallback(int nid) {
    SndBase *pbs = GetSndBase_Object(nid);
    if (pbs) {
        return pbs->GetOutputBlockPtr() ? pbs->GetOutputBlockPtr() : nullptr;
    }
    return g_DMIX_DummyInputBlock;
}

void EAXSound::SetSFXOutCallback(int nid, int *ptr) {
    SndBase *pbs = GetSndBase_Object(nid);
    if (pbs) {
        pbs->SetOutputsPtr(ptr);
    }
    int *pclear = ptr;
    for (int n = 0; n < 15; n++) {
        *pclear = 0;
        pclear++;
    }
}

bool EAXSound::SetSFXInputCallback(int nid, int *ptr) {
    SndBase *pbs = GetSndBase_Object(nid);
    if (pbs) {
        pbs->SetInputsPtr(ptr);
        if (pbs->GetStateBase()) {
            return pbs->GetStateBase()->IsAttached();
        }
    }
    return false;
}

int EAXSound::GetStateRefCount(int nstate) {
    return m_pStateMgr[nstate] ? m_pStateMgr[nstate]->GetStateObjCount() : 0;
}

void EAXSound::SetSFXBaseObject(SFX_Base *psb, eMAINMAPSTATES estate, int ntype, int instance) {
    switch (estate) {
        case eMM_MAIN:
            switch (ntype) {
                case SFXOBJ_MUSIC:
                    break;
                case SFXOBJ_SPEECH:
                    Speech::Manager::AttachSFXOBJ(COPSPEECH_MODULE, psb, SFXOBJ_SPEECH);
                    break;
                case SFXOBJ_MOVIES:
                    break;
                case SFXOBJ_NISPROJ_STRMS:
                case SFXOBJ_MOMENT_STRMS:
                    Speech::Manager::AttachSFXOBJ(NISSFX_MODULE, psb, static_cast<eSFXOBJ_MAIN_TYPES>(ntype));
                    break;
                case SFXOBJ_FEHUD:
                    if (this->m_pFESnd != nullptr) {
                        this->m_pFESnd->AttachSFXOBJ(psb, SFXOBJ_FEHUD);
                    }
                    if (this->m_pCmnSnd != nullptr) {
                        this->m_pCmnSnd->AttachSFXOBJ(psb, SFXOBJ_FEHUD);
                    }
                    break;
            }
            break;
        case eMM_PLAYERCAR:
        case eMM_AIRACECAR:
        case eMM_TRAFFIC:
        case eMM_ENVIRONMENT:
            break;
        case eMM_COLLISION:
            Speech::Manager::AttachSFXOBJ(NISSFX_MODULE, psb, static_cast<eSFXOBJ_MAIN_TYPES>(ntype));
            break;
        default:
            break;
    }
}

// GetSFXBase_Object, GetSndBase_Object go here when implemented

SFX_Base *EAXSound::GetSFXBase_Object(int nID) {
    SFX_Base *ReturnObj = nullptr;
    int nState = (static_cast<unsigned int>(nID) >> 16) & 0xFF;
    if (m_pStateMgr[nState]) {
        int nInstanceID = (nID >> 11) & 0x1F;
        int SFXID_Number = (nID >> 4) & 0x7F;
        if (!m_pStateMgr[nState]->GetStateObj(nInstanceID)) {
            return nullptr;
        }
        if ((nID & 0xE0000000) == 0x40000000) {
            ReturnObj = static_cast<SFX_Base *>(m_pStateMgr[nState]->GetStateObj(nInstanceID)->GetSFXObject(SFXID_Number));
        }
    }
    return ReturnObj;
}

SndBase *EAXSound::GetSndBase_Object(int nID) {
    SndBase *ReturnObj = nullptr;
    int nState = (static_cast<unsigned int>(nID) >> 16) & 0xFF;
    if (m_pStateMgr[nState]) {
        int nInstanceID = (nID >> 11) & 0x1F;
        int SFXID_Number = (nID >> 4) & 0x7F;
        if (!m_pStateMgr[nState]->GetStateObj(nInstanceID)) {
            return nullptr;
        }
        if ((nID & 0xE0000000) == 0x40000000) {
            ReturnObj = m_pStateMgr[nState]->GetStateObj(nInstanceID)->GetSFXObject(SFXID_Number);
        }
        if ((nID & 0xE0000000) == 0x60000000) {
            ReturnObj = m_pStateMgr[nState]->GetStateObj(nInstanceID)->GetSFXCTLObject(SFXID_Number);
        }
    }
    if (!ReturnObj) {
        return nullptr;
    }
    return ReturnObj;
}

float EAXSound::GetCurMusicVolume() {
    if (this->m_eSndGameMode == SND_FRONTEND) {
        return this->m_pCurAudioSettings->GetMasteredFEMusicVol();
    }
    return this->m_pCurAudioSettings->GetMasteredIGMusicVol();
}

// Las dos PS2 la tienen (EAXSound.cpp:1002-1024 de la A124); en GameCube el enlazador la quita (el DWARF la da con
// la escala de volumen del disco de demostracion, DemoDiscManager, que este cuerpo no lleva).
int EAXSound::GetMasterVolume(eMasterMixChannel eMasterMixChannel) {
    if (this->m_pCurAudioSettings) {
        switch (eMasterMixChannel) {
        case eMASTER_VOL:
            return static_cast<int>(this->m_pCurAudioSettings->MasterVol * 32767.0f);
        case eSPEECH_VOL:
            return static_cast<int>(this->m_pCurAudioSettings->MasterVol * this->m_pCurAudioSettings->GetMasteredSpeechVol() * 32767.0f);
        case eFEMUSIC_VOL:
            return static_cast<int>(this->m_pCurAudioSettings->MasterVol * this->m_pCurAudioSettings->GetMasteredFEMusicVol() * 32767.0f);
        case eMUSIC_VOL:
            return static_cast<int>(this->m_pCurAudioSettings->MasterVol * this->m_pCurAudioSettings->GetMasteredIGMusicVol() * 32767.0f);
        case eSFX_VOL:
            return static_cast<int>(this->m_pCurAudioSettings->MasterVol * this->m_pCurAudioSettings->GetMasteredSoundEffectsVol() * 32767.0f);
        case eENGINE_VOL:
            return static_cast<int>(this->m_pCurAudioSettings->MasterVol * this->m_pCurAudioSettings->GetMasteredCarVol() * 32767.0f);
        case eTIRE_VOL:
            return static_cast<int>(this->m_pCurAudioSettings->MasterVol * this->m_pCurAudioSettings->GetMasteredCarVol() * 32767.0f);
        default:
            return 0x7FFF;
        }
    }

    return 0;
}

void EAXSound::ReInitMasterVolumes() {
    int i;
}

void EAXSound::UpdateVolumes(AudioSettings *paudiosettings, float NewValue) {
    this->m_pCurAudioSettings = paudiosettings;
    this->ReInitMasterVolumes();
    g_fMasterSFXVolume = paudiosettings->AmbientVol;
    g_iMasterSFXVolume = static_cast<int>(paudiosettings->AmbientVol * 32767.0f);
    g_SliderValue = NewValue;
}

unsigned int EAXSound::Random(int range) {
    return bRandom(range, &SoundRandomSeed);
}

float EAXSound::Random(float range) {
    return bRandom(range, &SoundRandomSeed);
}

// En el fuente de todas las versiones (DWARF de GameCube); solo la A124 la conserva al enlazar (EAXSound.cpp:1084-1087).
void EAXSound::RestoreDriver() {
    if (this->m_pEAXSND8Wrapper) {
        this->m_pEAXSND8Wrapper->RestoreSoundDriver();
    }
}

void EAXSound::UpdateSongInfo() {}

void EAXSound::InitializeDriver() {
    if (!IsSoundEnabled) {
        return;
    }

    this->m_pEAXSND8Wrapper =
        new (gAudioMemoryManager.AllocateMemory(sizeof(EAXSND8Wrapper), "EAXSND8Wrapper", false)) EAXSND8Wrapper();
    if (!this->m_pEAXSND8Wrapper->Initialize()) {
        IsSoundEnabled = false;
        return;
    }

    gAEMSMgr.InitSPUram();
    this->m_pNFSMixMaster = new (gAudioMemoryManager.AllocateMemory(sizeof(NFSMixMaster), "NFSMixMaster", false))
        NFSMixMaster();
    this->m_pSTICH_Playback = new (gAudioMemoryManager.AllocateMemory(sizeof(cSTICH_PlayBack), "STICH_PlayBack", false))
        cSTICH_PlayBack();
    new (gAudioMemoryManager.AllocateMemory(sizeof(NIS_RevManager), "NISRevMan", false)) NIS_RevManager();

    for (int n = 0; n < 13; n++) {
        m_pStateMgr[n] = nullptr;
    }

    m_pStateMgr[0] =
        new (gAudioMemoryManager.AllocateMemory(sizeof(CSTATEMGR_Main), "SND: CSTATEMGR_MAIN", false))
            CSTATEMGR_Main();
    m_pStateMgr[0]->Initialize(static_cast<eMAINMAPSTATES>(0));

    m_pStateMgr[1] =
        new (gAudioMemoryManager.AllocateMemory(sizeof(CSTATEMGR_Music), "SND: CSTATEMGR_Music", false))
            CSTATEMGR_Music();
    m_pStateMgr[1]->Initialize(static_cast<eMAINMAPSTATES>(1));

    m_pStateMgr[2] =
        new (gAudioMemoryManager.AllocateMemory(sizeof(CSTATEMGR_PlayerCar), "SND: CSTATEMGR_PlyrCar", false))
            CSTATEMGR_PlayerCar();
    m_pStateMgr[2]->Initialize(static_cast<eMAINMAPSTATES>(2));

    m_pStateMgr[3] =
        new (gAudioMemoryManager.AllocateMemory(sizeof(CSTATEMGR_AICar), "SND: CSTATEMGR_AICar", false))
            CSTATEMGR_AICar();
    m_pStateMgr[3]->Initialize(static_cast<eMAINMAPSTATES>(3));

    m_pStateMgr[4] =
        new (gAudioMemoryManager.AllocateMemory(sizeof(CSTATEMGR_CopCar), "SND: CSTATEMGR_CarState", false))
            CSTATEMGR_CopCar();
    m_pStateMgr[4]->Initialize(static_cast<eMAINMAPSTATES>(4));

    m_pStateMgr[5] =
        new (gAudioMemoryManager.AllocateMemory(sizeof(CSTATEMGR_TrafficCar), "SND: CSTATEMGR_TrafficCar", false))
            CSTATEMGR_TrafficCar();
    m_pStateMgr[5]->Initialize(static_cast<eMAINMAPSTATES>(5));

    m_pStateMgr[6] =
        new (gAudioMemoryManager.AllocateMemory(sizeof(CSTATEMGR_Enviro), "SND: CSTATEMGR_ENVIRO", false))
            CSTATEMGR_Enviro();
    m_pStateMgr[6]->Initialize(static_cast<eMAINMAPSTATES>(6));

    m_pStateMgr[7] =
        new (gAudioMemoryManager.AllocateMemory(sizeof(CSTATEMGR_Collision), "SND: CSTATEMGR_Collision", false))
            CSTATEMGR_Collision();
    m_pStateMgr[7]->Initialize(static_cast<eMAINMAPSTATES>(7));

    m_pStateMgr[8] =
        new (gAudioMemoryManager.AllocateMemory(sizeof(CSTATEMGR_DriveBy), "SND: CSTATEMGR_DriveBy", false))
            CSTATEMGR_DriveBy();
    m_pStateMgr[8]->Initialize(static_cast<eMAINMAPSTATES>(8));

    m_pStateMgr[11] =
        new (gAudioMemoryManager.AllocateMemory(sizeof(CSTATEMGR_Helicopter), "SND: CSTATEMGR_Helicopter", false))
            CSTATEMGR_Helicopter();
    m_pStateMgr[11]->Initialize(static_cast<eMAINMAPSTATES>(11));

    m_pStateMgr[12] =
        new (gAudioMemoryManager.AllocateMemory(sizeof(CSTATEMGR_Truck), "SND: CSTATEMGR_Truck", false))
            CSTATEMGR_Truck();
    m_pStateMgr[12]->Initialize(static_cast<eMAINMAPSTATES>(12));

    this->m_pNFSMixMaster->AssignSFXCallbacks(EAXSound::GetPointerCallback, EAXSound::SetSFXOutCallback,
                                        EAXSound::SetSFXInputCallback, EAXSound::GetStateRefCount,
                                        EAXSound::MixMapReadyCallback);
}

void EAXSound::RefreshLocalAttr() {
    if (this->mLocalAttr != nullptr) {
        delete this->mLocalAttr;
        this->mLocalAttr = nullptr;
    }

    switch ((SkipFE == 0) ? GetCurrentLanguage() : SkipFELanguage) {
    case 1:
        this->mLocalAttr = new Attrib::Gen::audiosystem(this->mAttributes->Locales(1).GetCollectionWithDefault(), 0, nullptr);
        break;
    case 2:
        this->mLocalAttr = new Attrib::Gen::audiosystem(this->mAttributes->Locales(2).GetCollectionWithDefault(), 0, nullptr);
        break;
    case 3:
        this->mLocalAttr = new Attrib::Gen::audiosystem(this->mAttributes->Locales(3).GetCollectionWithDefault(), 0, nullptr);
        break;
    case 4:
        this->mLocalAttr = new Attrib::Gen::audiosystem(this->mAttributes->Locales(4).GetCollectionWithDefault(), 0, nullptr);
        break;
    case 10:
        this->mLocalAttr = new Attrib::Gen::audiosystem(this->mAttributes->Locales(5).GetCollectionWithDefault(), 0, nullptr);
        break;
    case 0:
    default:
        this->mLocalAttr = new Attrib::Gen::audiosystem(this->mAttributes->Locales(0).GetCollectionWithDefault(), 0, nullptr);
        break;
    }
}

void EAXSound::InitializeSoundBootLoad() {
    if (!IsSoundEnabled) {
        return;
    }

    this->mAttributes = new Attrib::Gen::audiosystem(0x7e4b0ed2, 0, nullptr);
    this->RefreshLocalAttr();
    gAEMSMgr.Init();
    // La cache de voz tiene un tamano por build: 0x4B000 en el prototipo A124, 0x32000 en la PS2 de
    // venta y 0x1B000 en GameCube (lui/ori de InitializeSoundBootLoad en cada binario).
#ifdef EA_BUILD_A124
    gSpeechCache.Init(0x4B000);
#elif defined(EA_PLATFORM_PLAYSTATION2)
    gSpeechCache.Init(0x32000);
#else
    gSpeechCache.Init(0x1B000);
#endif
    CSTATEMGR_Base::ClearClassLists();
    RegisterStates();
    RegisterSFX();
}

void EAXSound::StartNewGamePlay() {
    int id;

    if (!IsSoundEnabled) {
        return;
    }

#ifndef EA_BUILD_A124
    SetSoundControlState(true, SNDSTATE_STOP_MUSIC, "RestartRace");
#endif

    if (bHasStartNewGameOccured) {
        if (this->m_pCmnSnd) {
            delete this->m_pCmnSnd;
            this->m_pCmnSnd = nullptr;
        }

#ifndef EA_BUILD_A124 // la A124 no para los canales de stream
        for (int s = 0; s < 4; s++) {
            if (this->m_pStreamManager->GetStreamChannel(s) && s != 1) {
                this->m_pStreamManager->GetStreamChannel(s)->Stop();
                this->m_pStreamManager->GetStreamChannel(s)->PurgeStream();
            }
        }
#endif

        for (int n = 0; n < 13; n++) {
            if (n != 1) {
                if (m_pStateMgr[n]) {
                    m_pStateMgr[n]->ExitWorld();
                }
            } else {
                m_pStateMgr[n]->DisconnectMixMap();
            }
        }

        if (this->m_pNFSMixMaster->IsMixMapReady()) {
            this->m_pNFSMixMaster->DestroyMainMainMap();
        }

        gAEMSMgr.DestroySlots(false);
    }

    if (this->m_ePlayerMixMode != EAXS3D_TWO_PLAYER_MIX) {
        EAXAemsManager::QueueSlots(eBANK_SLOT_PATHFINDER, 1);
    }

    CSTATEMGR_AICar::QueueSlots();
    gAEMSMgr.InitializeSlots(!bHasStartNewGameOccured);
    bHasStartNewGameOccured = true;
    EAXCar::g_TurboInfo = nullptr;
    EAXCar::g_ShiftInfo = nullptr;

    if (!bIsMapInQueuedFileLoad) {
        bIsMapInQueuedFileLoad = true;

        GRaceParameters *race = nullptr;
        if (GRaceStatus::Exists()) {
            race = GRaceStatus::Get().GetRaceParameters();
        } else {
            race = GRaceDatabase::Get().GetStartupRace();
        }

        if (race) {
            GRace::Type rt = race->GetRaceType();
            if (rt == GRace::kRaceType_Drag) {
                this->SetSndGameMode(SND_DRAGRACE);
                if (Sim::GetUserMode() == Sim::USER_SPLIT_SCREEN) {
                    this->m_pNFSMixMaster->CreateMainMainMap(eRACE_TWODRG);
                } else {
                    this->m_pNFSMixMaster->CreateMainMainMap(eRACE_DRAG);
                }
            } else {
                int id = 0x40010010;
                SFXObj_PFEATrax *ppf = static_cast<SFXObj_PFEATrax *>(this->GetSFXBase_Object(id));
                if (ppf) {
                    ppf->RestartRace();
                }
                if (rt == GRace::kRaceType_Challenge) {
                    this->SetSndGameMode(SND_CHALLENGERACE);
                } else {
                    this->SetSndGameMode(SND_STREETRACE);
                }
                if (Sim::GetUserMode() == Sim::USER_SPLIT_SCREEN) {
                    this->m_pNFSMixMaster->CreateMainMainMap(eRACE_TWOCIRC);
                } else {
                    this->m_pNFSMixMaster->CreateMainMainMap(eRACE_CIRCUIT);
                }
            }
        } else {
            this->SetSndGameMode(SND_FREEROAM);
            if (Sim::GetUserMode() == Sim::USER_SPLIT_SCREEN) {
                this->m_pNFSMixMaster->CreateMainMainMap(eRACE_TWOCIRC);
            } else {
                this->m_pNFSMixMaster->CreateMainMainMap(eRACE_CIRCUIT);
            }
        }
    }

    this->InitializeInGame();
    this->m_pNFSMixMaster->InitMixMap(0);

    id = 0x40010010;
    SFXObj_Pathfinder *ppf = static_cast<SFXObj_Pathfinder *>(this->GetSFXBase_Object(id));
#ifdef EA_BUILD_A124
    // La A124 no comprueba ppf antes de elegir la pantalla partida y, fuera de las carreras de aceleracion, activa el
    // 321 del pathfinder (lineas 1402-1419 de su EAXSound.cpp).
    if (this->m_ePlayerMixMode == EAXS3D_TWO_PLAYER_MIX) {
        ppf->SetSplitScreen(true);
    } else {
        ppf->SetSplitScreen(false);
    }

    if (GRaceStatus::Exists() && GRaceStatus::Get().GetRaceParameters()->GetRaceType() != GRace::kRaceType_Drag) {
        if (ppf) {
            ppf->Set321(true);
        }
    }
#else
    if (ppf) {
        if (this->m_ePlayerMixMode == EAXS3D_TWO_PLAYER_MIX) {
            ppf->SetSplitScreen(true);
        } else {
            ppf->SetSplitScreen(false);
        }
    }
#endif

    gbHasStartNewGamePlayBeenProcessed = true;
}

void EAXSound::InitializeInGame() {
    if (!IsSoundEnabled) {
        return;
    }

    int nstate = -1;
    this->m_pCmnSnd = new ("AUD:EAXCommon", 0) EAXCommon();
    this->m_pCmnSnd->Initialize();

    SoundRandomSeed = bRandom(-1);
    if (!bIsAnFEToIngameTransition) {
        nstate = 1;
    }
    bIsAnFEToIngameTransition = false;

    for (int n = 0; n < 13; n++) {
        if (n != nstate && m_pStateMgr[n]) {
            m_pStateMgr[n]->EnterWorld(this->m_eSndGameMode);
        }
    }

    this->mEventID = Scheduler::Get().fSchedule_OncePerGameLoop->AddTask(-0x0D2EF66E, &this->mData, 6, true, 0, 0);
}

void EAXSound::InitializeFrontEnd() {
    if (!IsSoundEnabled) {
        return;
    }

    if (this->mAttributes == nullptr) {
        this->mAttributes = new Attrib::Gen::audiosystem(0x7e4b0ed2, 0, nullptr);
    }

    if (this->mLocalAttr == nullptr) {
        this->mLocalAttr = new Attrib::Gen::audiosystem(this->mAttributes->Locales(0).GetCollectionWithDefault(), 0, nullptr);
    }

    this->m_pCmnSnd = new ("AUD:EAXCommon", 0) EAXCommon();
    this->m_pFESnd = new ("AUD:EAXFrontEnd", 0) EAXFrontEnd();
    this->RefreshLocalAttr();
    Speech::Manager::Init(SPEECH_FRONTEND_MODE);
    SoundRandomSeed = bRandom(-1);
    this->m_pCurAudioSettings = &FEDatabase->CurrentUserProfiles[0]->GetOptions()->TheAudioSettings;
    EAXAemsManager::m_RequiredSlots[3]++;
    gAEMSMgr.InitializeSlots(true);

    if (m_pStateMgr[0] != nullptr) {
        m_pStateMgr[0]->EnterWorld(this->m_eSndGameMode);
    }
    if (m_pStateMgr[1] != nullptr) {
        m_pStateMgr[1]->EnterWorld(this->m_eSndGameMode);
    }

    g_pEAXSound->InitEATRAX();
}

void EAXSound::MixMapReadyCallback() {
    bIsMapInQueuedFileLoad = false;
    for (int n = 0; n < eMM_MAX_MAIN_MIXSTATES; n++) {
        if (m_pStateMgr[n] != nullptr) {
            m_pStateMgr[n]->SafeConnectOrphanObjects();
        }
    }
    g_pEAXSound->AttachPlayerCars();
}

void EAXSound::AttachPlayerCars() {
    int num_attached = 0;
    for (int n = 0; n < EAX_CarState::Count(); n++) {
        if (EAX_CarState::GetList()[n]->GetContext() == Sound::CONTEXT_PLAYER) {
            num_attached++;
            CSTATE_Base *newcar =
                m_pStateMgr[eMM_PLAYERCAR]->GetFreeState(EAX_CarState::GetList()[n]);
            if (newcar != nullptr) {
                newcar->Attach(EAX_CarState::GetList()[n]);
                if (Sim::GetUserMode() == Sim::USER_SPLIT_SCREEN) {
                    if (num_attached == 2) {
                        return;
                    }
                }
                if (Sim::GetUserMode() != Sim::USER_SPLIT_SCREEN && num_attached == 1) {
                    return;
                }
            }
        }
    }
}

void EAXSound::InitEATRAX() {
    this->UpdateSongInfo();
}

void EAXSound::PlayFEMusic(int nIndex) {
    MControlPathfinder(true, 0, 0, 0).Send(UCrc32("Pathfinder5"));
}

#ifdef EA_BUILD_A124
unsigned int GenerateUpgradedEngine(EAX_CarState *pCar);
#else
unsigned int GenerateUpgradedEngine(EAX_CarState *pCar, int playerUpgrade);
#endif

namespace Physics {
namespace Info {

float MaxTorque(const Attrib::Gen::engine &engine, float &atrpm);

} // namespace Info
} // namespace Physics

// Sound::Wheel::Reset and Sound::Engine::Reset are defined inline in EAXCarState.hpp.

#ifdef EA_BUILD_A124
inline EAX_CarState::EAX_CarState(const Attrib::Collection *atr, Sound::Context context, unsigned int wuid)
#else
inline EAX_CarState::EAX_CarState(const Attrib::Collection *atr, Sound::Context context, unsigned int wuid, HSIMABLE__ *handle)
#endif
    : mVel0(0.0f, 0.0f, 0.0f) //
    , mRacePos(0) //
    , mBrake(0.0f) //
    , mEBrake(0.0f) //
    , mNosEmptyFlag(0) //
    , mMovementMode(PHYSICS_MOVEMENT) //
    , mPlayerZone(PLAYER_ZONE_NONE) //
    , mSteering(0) //
    , mAngle(0) //
    , mSirenState(Sound::SIREN_OFF) //
    , mHotPursuit(0) //
    , mAttributes(atr, 0, nullptr) //
    , mWorldID(wuid) //
    , mEngineInfo(static_cast<const Attrib::Collection *>(nullptr), 0, nullptr) //
#ifdef EA_BUILD_A124
    , mContext(context) // la A124 guarda mContext antes que mWorldID: va en la lista
#endif
{
#ifndef EA_BUILD_A124
    static int PlayerUpgrade = 0;
#endif
    float max_torque_rpm;

#ifndef EA_BUILD_A124
    this->mContext = context;
#endif
    this->mSimUpdating = 1;
#ifndef EA_BUILD_A124
    this->mHandle = handle;
#endif
    this->mAssetsLoaded = 0;
    this->mTrailerID = 0;
    this->mNISCarID = -1;
    this->mControlSource = Sound::CONTROL_AI;
    this->mOversteer = 0.0f;
    this->mUndersteer = 0.0f;
    this->mSlipAngle = 0.0f;
    this->mVisualRPM = 0.0f;
    this->mDesiredSpeed = 0.0f;
    this->mVel1 = this->mVel0;
    bIdentity(&this->mMatrix);

    Attrib::Gen::pvehicle vehicleinfo(this->mAttributes);
    Attrib::Gen::engine engine(vehicleinfo.engine(0), 0, nullptr);

#ifndef EA_BUILD_A124
    if (this->mContext == Sound::CONTEXT_PLAYER) {
        int phys_cur_upgrade = this->mAttributes.engine_current();
        int phys_num_upgrades = this->mAttributes.engine_upgrades();

        int base_upgrade = bClamp(4 - phys_num_upgrades, 0, 4);
        int curupgade_offset = bClamp(phys_cur_upgrade, 0, 4);

        PlayerUpgrade = base_upgrade + curupgade_offset;

        PlayerUpgrade = bClamp(PlayerUpgrade, 0, 4);
    }

    this->mEngineInfo.ChangeWithDefault(GenerateUpgradedEngine(this, PlayerUpgrade));
#else
    this->mEngineInfo.ChangeWithDefault(GenerateUpgradedEngine(this));
#endif
    this->mMaxTorque = Physics::Info::MaxTorque(engine, max_torque_rpm);
    this->mMaxRPM = engine.MAX_RPM();
    this->mMinRPM = engine.IDLE();
    this->mRedline = engine.RED_LINE();

    switch (this->mContext) {
    case Sound::CONTEXT_TRAFFIC:
    case Sound::CONTEXT_TRACTOR:
    case Sound::CONTEXT_TRAILER:
        this->mAssetsLoaded = 1;
        break;
    }
}

template class UTL::Collections::Listable<EAX_CarState, EAX_CarState::List::Limit>::List;
template class UTL::Vector<EAX_HeliState *, 16>;

void EAXSound::Update(float t) {
    if (!IsSoundEnabled || t < 0.0f) {
        return;
    }

    if (gnHasStartLoadFEBeenProcessed != 0) {
        gnHasStartLoadFEBeenProcessed--;
        if (gnHasStartLoadFEBeenProcessed < 0) {
            gnHasStartLoadFEBeenProcessed = 0;
        }
        return;
    }

    if (gbHasStartNewGamePlayBeenProcessed != 0) {
        gbHasStartNewGamePlayBeenProcessed = 0;
        return;
    }

    if (DisableSoundUpdate != 0) {
        return;
    }

    if (!bReadCallbackToggle) {
        if (bStreamReadTiming == false) {
            bStreamReadTiming = true;
            bGetTickerDifference(uStreamReadTicks, bGetTicker());
            uStreamReadTicks = bGetTicker();
        }
    } else if (bStreamReadTiming == true) {
        bGetTickerDifference(uStreamReadTicks, bGetTicker());
        uStreamReadTicks = bGetTicker();
        bStreamReadTiming = false;
    }

    if (gbWorldDataBlocksAudioRead == true) {
        if (bStreamBlockState == false) {
            bStreamBlockState = gbWorldDataBlocksAudioRead;
            uStreamBlockTicks = bGetTicker();
        }
    } else if (bStreamBlockState == true) {
        bGetTickerDifference(uStreamBlockTicks, bGetTicker());
        bStreamBlockState = false;
    }

    bool audio_interrupts_wdr = gbAudioInterruptsWorldDataRead;
    if (audio_interrupts_wdr == true) {
        if (bAudioInterrupt == false) {
            uAudioInterruptTicks = bGetTicker();
        }
        bAudioInterrupt = audio_interrupts_wdr;
    } else if (bAudioInterrupt == true) {
        bGetTickerDifference(uAudioInterruptTicks);
        bAudioInterrupt = false;
    } else {
        bAudioInterrupt = false;
    }

    if (gb_DORESTART_RACE != 0) {
        if (this->mEventID != 0) {
            Scheduler::Get().fSchedule_OncePerGameLoop->RemoveTask(this->mEventID);
        }
        EAXCar::g_TurboInfo = nullptr;
        EAXCar::g_ShiftInfo = nullptr;
        this->m_pNFSMixMaster->DestroyMap();
        this->InitializeInGame();
        this->m_pNFSMixMaster->InitMixMap(0);

        Speech::Module *module = Speech::Manager::GetSpeechModule(1);
        if (module != nullptr) {
            module = Speech::Manager::GetSpeechModule(1);
            module->PurgeSpeech();
        }

        SFXObj_PFEATrax *ppf = static_cast<SFXObj_PFEATrax *>(this->GetSFXBase_Object(0x40010010));
        if (ppf != nullptr) {
            ppf->RestartRace();
            if (this->m_ePlayerMixMode == EAXS3D_TWO_PLAYER_MIX) {
                ppf->m_Flags |= 2;
            } else {
                ppf->m_Flags &= ~2u;
            }
        }

        bMemSet(GameFlowSndState, '\0', 0x3C);
        gb_DORESTART_RACE = 0;
#ifndef EA_BUILD_A124
        // La A124 termina el bloque aqui (b + sw gb_DORESTART_RACE); el resto llama a PauseMenu.
        SetSoundControlState(false, SNDSTATE_PAUSE, "PauseMenu");
#endif
    } else {
        if (gb_Is321 != 0) {
#ifdef EA_BUILD_A124
            // Solo la A124: primero encola el NIS del arranque con el hash de "RESTART_FAKE" (bStringHash +
            // QueueNISButtonThrough(hash, -1) antes del Start321Reving).
            this->QueueNISButtonThrough(bStringHash("RESTART_FAKE"), -1);
#endif
            if (g_pNISRevMgr != nullptr) {
                g_pNISRevMgr->Start321Reving();
            }
            gb_Is321 = 0;
        }

        if (GameFlowSndState[13] != 0) {
            Speech::Manager::Deduce();
        }

        gAEMSMgr.Update();
        SndCamera::UpdateCameras();
        this->m_pEAXSND8Wrapper->Update();
        if (g_pNISRevMgr != nullptr) {
            g_pNISRevMgr->Update(t);
        }

        if (this->m_pNFSMixMaster->IsMixMapReady() == true) {
            if (this->m_pSTICH_Playback != nullptr) {
                this->m_pSTICH_Playback->Update(t);
            }
            Speech::Manager::Update(t);

            if (this->GetFrontEnd() != nullptr) {
                this->GetFrontEnd()->Update(nullptr);
            }

            SndBase::m_fRunningTime += t;
            SndBase::m_fDeltaTime = t;
            for (int n = 0; n < 13; n++) {
                CSTATEMGR_Base *mgr = m_pStateMgr[n];
                if (mgr != nullptr) {
                    mgr->UpdateParams(g_EAXIsPaused() ? 0.0f : t);
                }
            }

            if (this->m_pNFSMixMaster != nullptr) {
                this->m_pNFSMixMaster->ProcessMixMap(t, SndCamera::GetCurCamState(0));
            }

            for (int n = 0; n < 13; n++) {
                CSTATEMGR_Base *mgr = m_pStateMgr[n];
                if (mgr != nullptr) {
                    mgr->ProcessUpdate();
                }
            }

            if (this->m_pCmnSnd != nullptr) {
                this->m_pCmnSnd->Update(nullptr);
            }
        }

        this->m_prevSndGameMode = this->m_eSndGameMode;
        if (!this->AreResourceLoadsPending()) {
            CarSoundConn::ForEach(CarSoundConn::SetAssetsLoaded);
        }
        SNDSYS_service();
    }
}

void EAXSound::CommitAssets() {
    CSTATEMGR_CarState::ResolveCarBanks();
    for (CarSoundConn *const *iter = CarSoundConn::GetList().begin();
         iter != CarSoundConn::GetList().end(); ++iter) {
        CarSoundConn *pconn = *iter;
        if (!pconn->mConnected) {
            pconn->mConnected = true;
        }
    }
}

EAXCar *EAXSound::GetPlayerTunerCar(int nindex) {
    if (m_pStateMgr[eMM_PLAYERCAR] != nullptr) {
        return static_cast<EAXCar *>(m_pStateMgr[eMM_PLAYERCAR]->GetStateObj(nindex));
    }
    return nullptr;
}

CSTATE_Helicopter *EAXSound::SpawnHelicopter(EAX_HeliState *pHeli) {
    if (!IsSoundEnabled) {
        return nullptr;
    }
    CSTATE_Base *newheli = nullptr;
    eMAINMAPSTATES eStateMgrType = eMM_HELICOPTER;
    newheli = m_pStateMgr[eStateMgrType]->GetFreeState(pHeli);
    if (newheli != nullptr) {
        newheli->Attach(pHeli);
        return reinterpret_cast<CSTATE_Helicopter *>(newheli);
    }
    return nullptr;
}

void EAXSound::DestroyEAXHeli(EAX_HeliState *pHeli) {
    CSTATE_Base *newheli;
    eMAINMAPSTATES eStateMgrType;
    if (IsSoundEnabled) {
        eStateMgrType = eMM_HELICOPTER;
        newheli = m_pStateMgr[eStateMgrType]->GetStateObj(pHeli);
        if (newheli != nullptr) {
            newheli->Detach();
        }
    }
}

void EAXSound::DestroyEAXCar(EAX_CarState *pCar) {
    if (!IsSoundEnabled) {
        return;
    }
    CSTATE_Base *attachedcar = nullptr;
    switch (pCar->GetContext()) {
    case Sound::CONTEXT_PLAYER:
        if (m_pStateMgr[eMM_PLAYERCAR] != nullptr) {
            attachedcar = m_pStateMgr[eMM_PLAYERCAR]->GetStateObj(pCar);
        }
        break;
    case Sound::CONTEXT_AIRACER:
        if (m_pStateMgr[eMM_AIRACECAR] != nullptr) {
            attachedcar = m_pStateMgr[eMM_AIRACECAR]->GetStateObj(pCar);
        }
        break;
    case Sound::CONTEXT_COP:
        if (m_pStateMgr[eMM_COPCAR] != nullptr) {
            attachedcar = m_pStateMgr[eMM_COPCAR]->GetStateObj(pCar);
        }
        break;
    case Sound::CONTEXT_TRAFFIC:
        if (m_pStateMgr[eMM_TRAFFIC] != nullptr) {
            attachedcar = m_pStateMgr[eMM_TRAFFIC]->GetStateObj(pCar);
        }
        break;
    // Las dos PS2 saltan con una tabla de 7 entradas (hasta CONTEXT_TRAILER), las tres ultimas al camion.
    case Sound::CONTEXT_ONLINE:
    case Sound::CONTEXT_TRACTOR:
    case Sound::CONTEXT_TRAILER:
    default:
        if (m_pStateMgr[eMM_TRUCK] != nullptr) {
            attachedcar = m_pStateMgr[eMM_TRUCK]->GetStateObj(pCar);
        }
        break;
    }
    if (attachedcar != nullptr) {
        attachedcar->Detach();
    }
    CSTATEMGR_CarState::DestroyCar(pCar);
}

void EAXSound::LoadFrontEndSoundBanks(void (*callback)(int), int callback_param) {
    if (!IsSoundEnabled) {
        return;
    }

#ifndef EA_BUILD_A124
    if (IsSpeechEnabled) {
        gSpeechCache.Validate();
    }
#endif

    gIsPauseForPause = 0;
    bSyncTaskRun();
    gnHasStartLoadFEBeenProcessed = 2;
    g_pEAXSound->m_prevSndGameMode = g_pEAXSound->m_eSndGameMode;
    g_pEAXSound->m_eSndGameMode = SND_FRONTEND;
    gAEMSMgr.ResetBankLoadParams();
    this->m_pStreamManager = new ("EAXS_StreamManager", false) EAXS_StreamManager();
    this->m_pStreamManager->InitializeStreams(SNDGM_FRONTEND);

#if defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_BUILD_A124)
    g_ActiveCtlStates = 0;
    g_ActiveSFXStates = 0;
    g_PrevActiveCtlStates = 0;
    g_PrevActiveSFXStates = 0;
#elif !defined(EA_BUILD_A124)
    if ((g_ActiveCtlStates & 0x20000) == 0) {
        g_ActiveCtlStates = 0;
        g_ActiveSFXStates = 0;
        g_PrevActiveCtlStates = 0;
        g_PrevActiveSFXStates = 0;
    }
#endif

    this->InitializeFrontEnd();

    this->m_pCmnSnd->Initialize();
    this->m_pFESnd->Initialize();

    this->m_pNFSMixMaster->CreateMainMainMap(eRACE_CIRCUIT);
    this->m_pNFSMixMaster->InitMixMap(0);

    gAEMSMgr.m_ExternalLoadCallback = callback;
    gAEMSMgr.m_ExternalLoadCallbackParam = callback_param;

    this->ReInitMasterVolumes();
    bSyncTaskRun();

#ifndef EA_BUILD_A124
    if (IsSpeechEnabled) {
        gSpeechCache.Validate();
    }
#endif
}

void EAXSound::UnloadFrontEndSoundBanks() {
    if (!IsSoundEnabled) {
        return;
    }
    int n;

#ifndef EA_BUILD_A124
    if (IsSpeechEnabled) {
        gSpeechCache.Validate();
    }
#endif

    gIsPauseForPause = 0;
    while (g_pEAXSound->AreResourceLoadsPending()) {
        g_pEAXSound->Update(0.1f);
        ServiceQueuedFiles();
    }

    Speech::Manager::Destroy();

    if (this->GetFrontEnd()) {
        this->GetFrontEnd()->DestroyAllDriveOnSnds();
    }

    for (n = 0; n < 13; n++) {
        if (m_pStateMgr[n]) {
            m_pStateMgr[n]->ExitWorld();
        }
    }

    delete this->m_pStreamManager;
    this->m_pStreamManager = nullptr;

    if (this->m_pNFSMixMaster) {
        this->m_pNFSMixMaster->DestroyMainMainMap();
    }

    if (this->m_pNFSLiveLink) {
        this->m_pNFSLiveLink->bMonitorChannel = false;
    }

    if (this->m_pFESnd) {
        delete this->m_pFESnd;
        this->m_pFESnd = nullptr;
    }

    if (this->m_pCmnSnd) {
        delete this->m_pCmnSnd;
        this->m_pCmnSnd = nullptr;
    }

    while (gAEMSMgr.m_nEndOfList != 0) {
        gAEMSMgr.UnloadSndData(0);
    }

    gAEMSMgr.DestroySlots(false);
    gAEMSMgr.ResetBankLoadParams();
    bSyncTaskRun();

#ifndef EA_BUILD_A124
    if (IsSpeechEnabled) {
        gSpeechCache.Validate();
    }
#endif
}

void g_LoadSndAsset(Attrib::StringKey filename, eSNDDATAPATH path, eSNDDATATYPE datatype) {
    stSndAssetQueue requeststruct;
    requeststruct.pThis = nullptr;
    requeststruct.pCar = nullptr;

    requeststruct.Asset.FileName = filename;
    requeststruct.Asset.DataPath = path;
    requeststruct.Asset.eDataType = datatype;
    gAEMSMgr.QueueFileLoad(requeststruct, eBANK_SLOT_NONE);
}

void LoadCommonIngameFiles() {
    g_LoadSndAsset(g_pEAXSound->GetAttributes().AEMS_FEBanks(1), SNDPATH_GLOBAL, SDT_AEMS_ASYNCSPUMEM);
    g_LoadSndAsset(g_pEAXSound->GetAttributes().AEMS_EnvBanks(), SNDPATH_INGAME, SDT_AEMS_ASYNCSPUMEM);
    g_LoadSndAsset(g_pEAXSound->GetAttributes().AEMS_MiscBanks(5), SNDPATH_INGAME, SDT_AEMS_ASYNCSPUMEM);
    g_LoadSndAsset(g_pEAXSound->GetAttributes().AEMS_MiscBanks(4), SNDPATH_INGAME, SDT_AEMS_ASYNCSPUMEM);
    g_LoadSndAsset(g_pEAXSound->GetAttributes().AEMS_MiscBanks(3), SNDPATH_INGAME, SDT_AEMS_ASYNCSPUMEM);
    g_LoadSndAsset(g_pEAXSound->GetAttributes().AEMS_MiscBanks(1), SNDPATH_INGAME, SDT_AEMS_ASYNCSPUMEM);
    g_LoadSndAsset(g_pEAXSound->GetAttributes().AEMS_RNBanks(), SNDPATH_INGAME, SDT_AEMS_ASYNCSPUMEM);
    g_LoadSndAsset(Attrib::StringKey("SIREN_MB.abk"), SNDPATH_INGAME, SDT_AEMS_ASYNCSPUMEM);
    g_LoadSndAsset(g_pEAXSound->GetAttributes().AEMS_StitchBanks(0), SNDPATH_INGAME, SDT_AEMS_ASYNCSPUMEM);
    g_LoadSndAsset(g_pEAXSound->GetAttributes().AEMS_StitchBanks(2), SNDPATH_INGAME, SDT_AEMS_ASYNCSPUMEM);
    g_LoadSndAsset(g_pEAXSound->GetAttributes().AEMS_StitchBanks(1), SNDPATH_INGAME, SDT_AEMS_ASYNCSPUMEM);
    g_LoadSndAsset(g_pEAXSound->GetAttributes().AEMS_MiscBanks(2), SNDPATH_INGAME, SDT_AEMS_ASYNCSPUMEM);
    g_LoadSndAsset(g_pEAXSound->GetAttributes().AEMS_WNBanks(0), SNDPATH_INGAME, SDT_AEMS_ASYNCSPUMEM);
    g_LoadSndAsset(g_pEAXSound->GetAttributes().AEMS_MiscBanks(6), SNDPATH_ENGINE, SDT_AEMS_ASYNCSPUMEM);
    g_LoadSndAsset(g_pEAXSound->GetAttributes().AEMS_MiscBanks(7), SNDPATH_ENGINE, SDT_AEMS_ASYNCSPUMEM);

    for (int n = 0; n < 12; n++) {
        g_LoadSndAsset(Attrib::StringKey(csfxedit[n]), SNDPATH_FXEDIT, SDT_GENERIC_DATA);
    }
}

void EAXSound::StopSND11() {}

void EAXSound::StartSND11() {}

void EAXSound::LoadInGameSoundBanks(void (*callback)(int), int callback_param) {
    if (IsSoundEnabled) {
#ifndef EA_BUILD_A124
        if (IsSpeechEnabled) {
            gSpeechCache.Validate();
        }
#endif
        gIsPauseForPause = 0;
        bIsAnFEToIngameTransition = true;
        this->m_pStreamManager = new ("EAXS_StreamManager", false) EAXS_StreamManager();

        if (FEDatabase->IsSplitScreenMode()) {
            this->m_pStreamManager->InitializeStreams(SNDGM_SPLITSCREEN);
            this->m_ePlayerMixMode = EAXS3D_TWO_PLAYER_MIX;
        } else {
            this->m_pStreamManager->InitializeStreams(SNDGM_FREEROAM);
            this->m_ePlayerMixMode = EAXS3D_SINGLE_PLAYER_MIX;
        }

        bHasDataLoadOccured = true;
        bHasStartNewGameOccured = false;
        this->m_pCurAudioSettings = FEDatabase->GetAudioSettings();
        this->m_pEAXSND8Wrapper->ReInit();

        g_pEAXSound->SetSndGameMode(SND_STREETRACE);
        gAEMSMgr.ResetBankLoadParams();
        CSTATEMGR_CarState::ResetCarBanks();
        bMemSet(g_SndAssetList, 0, sizeof(g_SndAssetList));
        this->RefreshLocalAttr();

        if (this->m_ePlayerMixMode == EAXS3D_TWO_PLAYER_MIX) {
            IsSpeechEnabled = false;
            Speech::Manager::Init(SPEECH_SPLITSCREEN_MODE);
        } else {
            IsSpeechEnabled = true;
            Speech::Manager::Init(SPEECH_GAME_MODE);
        }

        LoadCommonIngameFiles();

#if defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_BUILD_A124)
        g_ActiveCtlStates = 0;
        g_ActiveSFXStates = 0;
        g_PrevActiveCtlStates = 0;
        g_PrevActiveSFXStates = 0;
#elif !defined(EA_BUILD_A124)
        if ((g_ActiveCtlStates & 0x20000) == 0) {
            g_ActiveCtlStates = 0;
            g_ActiveSFXStates = 0;
            g_PrevActiveCtlStates = 0;
            g_PrevActiveSFXStates = 0;
        }
#endif

        gAEMSMgr.m_ExternalLoadCallback = callback;
        gAEMSMgr.m_ExternalLoadCallbackParam = callback_param;

#ifndef EA_BUILD_A124
        if (IsSpeechEnabled) {
            gSpeechCache.Validate();
        }
#endif
    }
}

void EAXSound::CloseSound() {
    bHasDataLoadOccured = false;

    for (int n = 0; n < 13; n++) {
        if (m_pStateMgr[n] != nullptr) {
            m_pStateMgr[n]->ExitWorld();
        }
    }

    while (g_pEAXSound->AreResourceLoadsPending()) {
        g_pEAXSound->Update(0.1f);
        ServiceQueuedFiles();
    }

    if (this->m_pNFSMixMaster != nullptr) {
        this->m_pNFSMixMaster->DestroyMainMainMap();
    }

    if (g_pNISRevMgr != nullptr) {
        g_pNISRevMgr->CloseNIS();
    }

    bSyncTaskRun();
}

void EAXSound::UnLoadInGameSoundBanks() {
    if (!IsSoundEnabled) {
        return;
    }

    IsSpeechEnabled = true;
    gIsPauseForPause = 0;
    while (g_pEAXSound->AreResourceLoadsPending()) {
        g_pEAXSound->Update(0.1f);
        ServiceQueuedFiles();
    }

    bHasDataLoadOccured = false;
    if (this->mmsgMRestartRace != nullptr) {
        Hermes::Handler::Destroy(this->mmsgMRestartRace);
        this->mmsgMRestartRace = nullptr;
    }

#ifndef EA_BUILD_A124
    if (IsSpeechEnabled) {
        gSpeechCache.Validate();
    }
#endif

    Speech::Manager::Destroy();
    bSyncTaskRun();

    if (this->m_pNFSMixMaster != nullptr) {
        this->m_pNFSMixMaster->DestroyMainMainMap();
    }

    if (m_pStateMgr[0] != nullptr) {
        m_pStateMgr[0]->ExitWorld();
    }

    delete this->m_pStreamManager;
    this->m_pStreamManager = nullptr;

    for (int n = 0; n < 12; n++) {
        if (SFXObj_Reverb::m_pFXEditPatch[n] != nullptr) {
            gAudioMemoryManager.FreeMemory(SFXObj_Reverb::m_pFXEditPatch[n]);
        }
        SFXObj_Reverb::m_pFXEditPatch[n] = nullptr;
    }

    if (this->m_pCmnSnd != nullptr) {
        delete this->m_pCmnSnd;
        this->m_pCmnSnd = nullptr;
    }

    while (gAEMSMgr.m_nEndOfList != 0) {
        gAEMSMgr.UnloadSndData(0);
    }

    gAEMSMgr.DestroySlots(true);
    gAEMSMgr.ResetBankLoadParams();
    bSyncTaskRun();
}

// LoadInGameSoundBanks, etc. go here when implemented

#ifdef EA_BUILD_A124
// La pausa a mano de la A124 (lineas 2827-3117 de su EAXSound.cpp); la version final la lleva SetSoundControlState.
void EAXSound::EnterPauseMenu(eSNDPAUSE_REASON pause_reason) {
    if (!IsSoundEnabled) {
        return;
    }

    m_LastPauseReason = pause_reason;
    GetSFXBase_Object(0x40000010); // la A124 lo pide y no usa el resultado

    switch (pause_reason) {
        case eSNDPAUSE_PAUSEMENU:
            gIsPauseForPause++;
            // sin break: el menu de pausa cuenta y luego pausa como los demas motivos
        case ePAUSE_ERROR:
        default:
            m_bIsPaused = true;
            t_Paused = t_CurTime;
            m_bAudioIsPaused = true;
            if (m_pStreamManager) {
                if (m_pStreamManager->GetStreamChannel(1)) {
                    m_pStreamManager->GetStreamChannel(1)->Pause();
                }
                if (m_pStreamManager->GetStreamChannel(0)) {
                    m_pStreamManager->GetStreamChannel(0)->Pause();
                }
                if (m_pStreamManager->GetStreamChannel(2)) {
                    m_pStreamManager->GetStreamChannel(2)->Pause();
                }
            }
            break;

        case eSNDPAUSE_PHOTOFINISH:
            gIsPauseForPause++;
            if (m_pStreamManager) {
                if (m_pStreamManager->GetStreamChannel(1)) {
                    m_pStreamManager->GetStreamChannel(1)->Pause();
                }
                if (m_pStreamManager->GetStreamChannel(0)) {
                    m_pStreamManager->GetStreamChannel(0)->Pause();
                }
                if (m_pStreamManager->GetStreamChannel(2)) {
                    m_pStreamManager->GetStreamChannel(2)->Pause();
                }
            }
            m_bIsPaused = true;
            t_Paused = t_CurTime;
            m_bAudioIsPaused = true;
            break;

        case eSNDPAUSE_NISON:
            if (m_pStreamManager) {
                if (m_pStreamManager->GetStreamChannel(1)) {
                    m_pStreamManager->GetStreamChannel(1)->Pause();
                }
                if (m_pStreamManager->GetStreamChannel(0)) {
                    m_pStreamManager->GetStreamChannel(0)->Resume();
                }
                if (m_pStreamManager->GetStreamChannel(2)) {
                    m_pStreamManager->GetStreamChannel(2)->Resume();
                }
            }
            break;

        case eSNDPAUSE_SIMSTATE_INIT:
        case eSNDPAUSE_MEMCARD:
            if (m_pStreamManager) {
                if (m_pStreamManager->GetStreamChannel(1)) {
                    m_pStreamManager->GetStreamChannel(1)->Pause();
                }
                if (m_pStreamManager->GetStreamChannel(0)) {
                    m_pStreamManager->GetStreamChannel(0)->Pause();
                }
                if (m_pStreamManager->GetStreamChannel(2)) {
                    m_pStreamManager->GetStreamChannel(2)->Pause();
                }
            }
            m_bIsPaused = true;
            t_Paused = t_CurTime;
            m_bAudioIsPaused = true;
            break;

        case eSNDPAUSE_SIMSTATE_IDLE: {
            if (m_pStreamManager) {
                if (m_pStreamManager->GetStreamChannel(1)) {
                    m_pStreamManager->GetStreamChannel(1)->Pause();
                }
                if (m_pStreamManager->GetStreamChannel(0)) {
                    m_pStreamManager->GetStreamChannel(0)->Pause();
                }
            }
            // Asi en el binario: con un NIS, !INIS::Exists() es falso y nunca pregunta IsLoaded ni IsPlaying.
            INIS *nis = INIS::Get();
            if (!nis || (!INIS::Exists() && !nis->IsLoaded() && !nis->IsPlaying())) {
                m_bIsPaused = true;
                t_Paused = t_CurTime;
                m_bAudioIsPaused = true;
            }
            break;
        }

        case eSNDPAUSE_ONLINE:
            m_bIsPaused = true;
            t_Paused = t_CurTime;
            m_bAudioIsPaused = true;
            if (m_pStreamManager) {
                if (m_pStreamManager->GetStreamChannel(1)) {
                    m_pStreamManager->GetStreamChannel(1)->Pause();
                }
                if (m_pStreamManager->GetStreamChannel(0)) {
                    m_pStreamManager->GetStreamChannel(0)->Pause();
                }
                if (m_pStreamManager->GetStreamChannel(2)) {
                    m_pStreamManager->GetStreamChannel(2)->Pause();
                }
            }
            break;

        case eSNDPAUSE_MOVIE:
            if (m_pStreamManager) {
                if (m_pStreamManager->GetStreamChannel(1)) {
                    m_pStreamManager->GetStreamChannel(1)->Pause();
                }
                if (m_pStreamManager->GetStreamChannel(0)) {
                    m_pStreamManager->GetStreamChannel(0)->Pause();
                }
                if (m_pStreamManager->GetStreamChannel(2)) {
                    m_pStreamManager->GetStreamChannel(2)->Pause();
                }
            }
            break;

        case eSNDPAUSE_SMS_MESSAGE:
            if (m_pStreamManager) {
                if (m_pStreamManager->GetStreamChannel(1)) {
                    m_pStreamManager->GetStreamChannel(1)->Pause();
                }
                if (m_pStreamManager->GetStreamChannel(0)) {
                    m_pStreamManager->GetStreamChannel(0)->Resume();
                }
                if (m_pStreamManager->GetStreamChannel(2)) {
                    m_pStreamManager->GetStreamChannel(2)->Pause();
                }
            }
            break;
    }
}

void EAXSound::ExitPauseMenu(eSNDPAUSE_REASON pause_reason) {
    if (!IsSoundEnabled) {
        return;
    }

    if (pause_reason == eSNDPAUSE_PAUSEMENU || pause_reason == eSNDPAUSE_PHOTOFINISH) {
        gIsPauseForPause--;
    }

    if (gIsPauseForPause > 0) {
        return;
    }

    gIsPauseForPause = 0;
    if (pause_reason == eSNDPAUSE_SMS_MESSAGE) {
        return;
    }

    this->m_bIsPaused = false;
    this->m_bAudioIsPaused = false;
    if (pause_reason == eSNDPAUSE_QUITTOFE) {
        if (this->m_pStreamManager->GetStreamChannel(1)) {
            this->m_pStreamManager->GetStreamChannel(1)->PurgeStream();
        }
        if (this->m_pStreamManager->GetStreamChannel(0)) {
            this->m_pStreamManager->GetStreamChannel(0)->PurgeStream();
        }
        if (this->m_pStreamManager->GetStreamChannel(2)) {
            this->m_pStreamManager->GetStreamChannel(2)->PurgeStream();
        }
    }

    if (this->m_pStreamManager->GetStreamChannel(1)) {
        this->m_pStreamManager->GetStreamChannel(1)->Resume();
    }
    if (this->m_pStreamManager->GetStreamChannel(0)) {
        this->m_pStreamManager->GetStreamChannel(0)->Resume();
    }
    if (this->m_pStreamManager->GetStreamChannel(2)) {
        this->m_pStreamManager->GetStreamChannel(2)->Resume();
    }
}
#endif

void EAXSound::ReStartRace(bool bIs321) {
    if (!IsSoundEnabled) {
        return;
    }

#ifndef EA_BUILD_A124
    SetSoundControlState(true, SNDSTATE_STOP_MUSIC, "RestartRace");
#endif

    for (int s = 0; s < 4; s++) {
        if (this->m_pStreamManager->GetStreamChannel(s) != nullptr) {
#ifdef EA_BUILD_A124
            // La A124 no para el canal 1, lo pausa, y no reanuda ninguno.
            if (s != 1) {
                this->m_pStreamManager->GetStreamChannel(s)->Stop();
                this->m_pStreamManager->GetStreamChannel(s)->PurgeStream();
            } else {
                this->m_pStreamManager->GetStreamChannel(s)->Pause();
            }
#else
            this->m_pStreamManager->GetStreamChannel(s)->Stop();
            this->m_pStreamManager->GetStreamChannel(s)->PurgeStream();
            this->m_pStreamManager->GetStreamChannel(s)->Resume();
#endif
        }
    }

    if (this->m_pCmnSnd != nullptr) {
        delete this->m_pCmnSnd;
        this->m_pCmnSnd = nullptr;
    }

    for (int n = 0; n < 13; n++) {
        if (n != 1) {
            if (m_pStateMgr[n] != nullptr) {
                m_pStateMgr[n]->ExitWorld();
            }
        } else {
            m_pStateMgr[1]->DisconnectMixMap();
        }
    }

    gb_DORESTART_RACE = 1;
    gb_Is321 = bIs321;
}

void InitializeSoundDriver() {
#ifdef EA_PLATFORM_PLAYSTATION2
    int size = IsSoundEnabled ? 0x158000 : 0x4000;
#elif defined(EA_PLATFORM_WIN32)
    int size = IsSoundEnabled ? 0x567000 : 0x4000; // PC: neg/sbb/and 0x563000/add 0x4000 en el exe
#else
    int size = IsSoundEnabled ? 0x11F000 : 0x4000;
#endif
#if defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_BUILD_A124)
    size += (INCREASE_MUSICSTREAM_BLOCKS + INCREASE_NISSFXSTRM_BLOCKS) * STREAMBLOCK_SIZE;
#endif

    gAudioMemoryManager.InitMemoryPool(
#ifndef EA_BUILD_A124
        AUD_MAIN_MEM_POOL,
#endif
        size);
    g_pEAXSound = new ("AUD:EAXSound") EAXSound();
    g_pEAXSound->InitializeDriver();
}

void InitializeSoundLoad() {
    g_pEAXSound->InitializeSoundBootLoad();
}

void LoadAemsFrontEnd(void (*callback)(int), int callback_param) {
    if (IsSoundEnabled == 1) {
        g_pEAXSound->LoadFrontEndSoundBanks(callback, callback_param);
    } else {
        callback(callback_param);
    }
}

void UnloadAemsFrontEnd() {
    if (IsSoundEnabled == 1) {
        g_pEAXSound->UnloadFrontEndSoundBanks();
    }
}

void SoundPause(bool bpause, eSNDPAUSE_REASON esndpause) {
#ifdef EA_BUILD_A124 // la A124 aun pausa a mano cuando no corre la maquina de estados
    // El return es del original: sin el, EE-GCC salta a las ultimas llamadas con j y el original las hace con jal.
    if (RUN_SOUND_STATE) {
        return;
    }

    if (bpause) {
        g_pEAXSound->EnterPauseMenu(esndpause);
        g_pEAXSound->Update(0.015f);
    } else {
        g_pEAXSound->ExitPauseMenu(esndpause);
    }
#endif
}

#ifdef EA_BUILD_A124
// La A124 compara el nombre del paquete con bStrICmp, sin la tabla de claves ni el switch (lineas 3407-3460 de su
// EAXSound.cpp); Pause_Options.fng se compara dos veces.
void FESoundControl(bool bOn, const char *name) {
    if (g_pEAXSound == nullptr) {
        return;
    }

    if (g_pEAXSound->GetSndGameMode() != SND_FRONTEND && g_pEAXSound->GetSndGameMode() != SND_LOADING_SCREEN) {
        if (!bStrICmp(name, "Pause_Main.fng") || !bStrICmp(name, "Pause_Options.fng")) {
            SetSoundControlState(bOn, SNDSTATE_PAUSE, "PauseMenu");
        } else if (!bStrICmp(name, "InGamePhotoMaster.fng")) {
            SetSoundControlState(bOn, SNDSTATE_PAUSE, "PhotoScreen");
        } else if (!bStrICmp(name, "Pause_Options.fng")) {
        } else if (!bStrICmp(name, "EA_Trax.fng") || !bStrICmp(name, "FadeScreen.fng")) {
        } else if (!bStrICmp(name, "SixDaysLater.fng")) {
            SetSoundControlState(bOn, SNDSTATE_PAUSE, "SixDaysLater");
        } else {
            SetSoundControlState(bOn, SNDSTATE_FE_UPSCREEN, "IG FE Screen");
        }
    }
}
#else
void FESoundControl(bool bOn, const char *name) {
    if (g_pEAXSound == nullptr) {
        return;
    }

    unsigned int key = Attrib::StringHash32(name);
    Attrib::StringKey FengList[37] = {
        "Pause_Main.fng",
        "Pause_Options.fng",
        "Pause_Controller.fng",
        "InGamePhotoMaster.fng",
        "EA_Trax.fng",
        "FadeScreen.fng",
        "SMS_Message.fng",
        "BUSTED_OVERLAY.fng",
        "SixDaysLater.fng",
        "InGame_MC_Main_GC.fng",
        "InGameAnyMovie.fng",
        "Pause_Performance_Tuning.fng",
        "InGame_MC_Main.fng",
        "InGameDialog.fng",
        "WS_InGameAnyMovie.fng",
        "InGameAnyMovie.fng",
        "InGameAnyTutorial.fng",
        "FEAnyMovie.fng",
        "WS_FEAnyMovie.fng",
        "FEAnyTutorial.fng",
        "InGameAnyTutorial.fng",
        "LS_EALogo.fng",
        "LS_EA_hidef.fng",
        "LS_PSA.fng",
        "MW_LS_IntroFMV.fng",
        "MW_LS_AttractFMV.fng",
        "WS_LS_EALogo.fng",
        "WS_LS_EA_hidef.fng",
        "WS_LS_PSA.fng",
        "WS_LS_IntroFMV.fng",
        "WS_MW_LS_AttractFMV.fng",
        "PostRace_MilestoneRewards.fng",
        "PostRace_Pursuit.fng",
        "Game_StartRace",
        "InGameMilestones.fng",
        "InGameBounty.fng",
        "InGameRaceSheet.fng",
    };

    bStringHash(name);
    int index = -1;
    int namehash = 0;
    do {
        if (key == FengList[namehash].GetHash32()) {
            index = namehash;
            break;
        }
        int n = namehash + 1;
        namehash = n;
    } while (namehash < 37);

    if (g_pEAXSound->GetSndGameMode() == SND_FRONTEND) {
        goto FRONTEND_FMV_STATE;
    }

    switch (index) {
        case 0:
        case 1:
        case 2:
        case 11:
#ifndef EA_BUILD_A124 // la alpha 124 no tiene m_bPause_MainFNG
            g_pEAXSound->m_bPause_MainFNG = bOn;
#endif
            SetSoundControlState(bOn, SNDSTATE_PAUSE, name);
            return;
        case 3:
            SetSoundControlState(bOn, SNDSTATE_PAUSE, name);
            return;
        case 4:
        case 5:
        case 6:
        case 7:
            return;
        case 8:
            SetSoundControlState(bOn, SNDSTATE_PAUSE, name);
            SetSoundControlState(bOn, SNDSTATE_OFF, name);
            return;
        case 9:
        case 12:
#ifndef EA_BUILD_A124
            if (g_pEAXSound->m_bPause_MainFNG) {
                return;
            }
#endif
            SetSoundControlState(bOn, SNDSTATE_PAUSE, name);
            return;
        case 10:
            goto FMV_STATE;
        case 13:
            return;
        case 16:
        case 20:
            SetSoundControlState(bOn, SNDSTATE_FMV, name);
            goto STOP_MUSIC;
        case 31:
        case 32:
            goto FE_SMS_MESSAGE;
        case 33:
            goto STOP_MUSIC;
        case 34:
        case 35:
        case 36:
            goto PURGE_SPEECH;
        case 14:
        case 15:
        case 17:
        case 18:
        case 19:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
            break;
        default:
            goto FE_UPSCREEN;
    }

FMV_STATE: if (bOn) {
        SetSoundControlState(bOn, SNDSTATE_FMV, name);
        return;
    }
    SetSoundControlState(false, SNDSTATE_FMV, name);
    SetSoundControlState(true, SNDSTATE_STOP_MUSIC, name);
    return;

FE_SMS_MESSAGE: SetSoundControlState(bOn, SNDSTATE_FE_SMS_MESSAGE, name);
    return;

STOP_MUSIC: SetSoundControlState(bOn, SNDSTATE_STOP_MUSIC, name);
    return;

PURGE_SPEECH: if (Speech::Manager::GetSpeechModule(1) != nullptr) {
        Speech::Manager::GetSpeechModule(1)->PurgeSpeech();
    }
    goto FE_UPSCREEN;

FE_UPSCREEN: SetSoundControlState(bOn, SNDSTATE_FE_UPSCREEN, name);
    return;

FRONTEND_FMV_STATE: if (index == 10 || index >= 14) {
        SetSoundControlState(bOn, SNDSTATE_FMV, "name");
    }
}
#endif

#ifdef EA_BUILD_A124
void SetSoundControlState(bool bON, eSNDCTLSTATE esndstate, char *Reason)
#else
void SetSoundControlState(bool bON, eSNDCTLSTATE esndstate, const char *Reason)
#endif
{
    if (!RUN_SOUND_STATE || g_pEAXSound == nullptr || !IsSoundEnabled) {
        return;
    }

#ifndef EA_BUILD_A124
    if (g_pEAXSound->m_bPause_MainFNG && !bON && esndstate != SNDSTATE_MINILOAD && esndstate <= SNDSTATE_FE_SMS_MESSAGE &&
        esndstate > SNDSTATE_NIS_ARREST) {
        return;
    }
#endif

    if (((g_ActiveCtlStates & (1 << esndstate)) && bON) || (!(g_ActiveCtlStates & (1 << esndstate)) && !bON)) {
        return;
    }

    g_PrevActiveCtlStates = g_ActiveCtlStates;
    g_PrevActiveSFXStates = g_ActiveSFXStates;

    if (!bON) {
        g_ActiveCtlStates &= ~(1 << esndstate);
    } else {
        g_ActiveCtlStates |= 1 << esndstate;
    }

    g_ActiveSFXStates = 0;
    for (int n = 0; n < MAX_SNDCTL_STATES; n++) {
        if ((g_ActiveCtlStates & (1 << n)) != 0) {
            g_ActiveSFXStates |= g_CtlStateActions[n];
        }
    }

    for (int state = 0; state < MAX_SFX_STATE; state++) {
        if ((g_ActiveSFXStates & (1 << state)) != (g_PrevActiveSFXStates & (1 << state)) &&
            g_pEAXSound->GetStreamManager()) {
            if ((g_ActiveSFXStates & (1 << state)) != 0) {
                switch (state) {
                    case 0:
                        if (g_pEAXSound->GetStreamManager()->GetStreamChannel(1)) {
                            g_pEAXSound->GetStreamManager()->GetStreamChannel(1)->Pause();
                        }
                        break;
                    case 1:
                        if (g_pEAXSound->GetStreamManager()->GetStreamChannel(0)) {
                            g_pEAXSound->GetStreamManager()->GetStreamChannel(0)->Pause();
                        }
                        break;
                    case 2:
                        if (g_pEAXSound->GetStreamManager()->GetStreamChannel(2)) {
                            g_pEAXSound->GetStreamManager()->GetStreamChannel(2)->Pause();
                        }
                        break;
                }
            } else {
                switch (state) {
                    case 0:
                        if (g_pEAXSound->GetStreamManager()->GetStreamChannel(1)) {
                            g_pEAXSound->GetStreamManager()->GetStreamChannel(1)->Resume();
                        }
                        break;
                    case 1:
                        if (g_pEAXSound->GetStreamManager()->GetStreamChannel(0)) {
                            g_pEAXSound->GetStreamManager()->GetStreamChannel(0)->Resume();
                        }
                        break;
                    case 2:
                        if (g_pEAXSound->GetStreamManager()->GetStreamChannel(2)) {
                            g_pEAXSound->GetStreamManager()->GetStreamChannel(2)->Resume();
                        }
                        break;
                }
            }
        }
    }

#ifndef EA_PLATFORM_PLAYSTATION2 // ni la A124 ni SLUS actualizan el sonido al entrar en SNDSTATE_ERROR
    if (esndstate == SNDSTATE_ERROR) {
        g_pEAXSound->Update(0.1f);
    }
#endif
}

void LoadAemsInGame(void (*callback)(int), int callback_param) {
    if (IsSoundEnabled == 1) {
        g_pEAXSound->LoadInGameSoundBanks(callback, callback_param);
    } else {
        callback(callback_param);
    }
}

void UnloadAemsInGame() {
    if (IsSoundEnabled == 1) {
        g_pEAXSound->UnLoadInGameSoundBanks();
    }
}

void CloseSound() {
    if (IsSoundEnabled) {
        g_pEAXSound->CloseSound();
    }
}

eSndAudioMode EAXSound::GetDefaultPlatformAudioMode() {
    return this->m_pEAXSND8Wrapper->GetDefaultPlatformAudioMode();
}

eSndAudioMode EAXSound::SetAudioModeFromMemoryCard(eSndAudioMode mode) {
    return this->m_pEAXSND8Wrapper->SetAudioModeFromMemoryCard(mode);
}
