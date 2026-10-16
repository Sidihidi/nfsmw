#include "Speed/Indep/Src/EAXSound/EAXFrontEnd.hpp"
#include "Speed/Indep/Src/EAXSound/EAXCommon.hpp"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/EAXSound/SND_GEN/FE_AEMS.h"
#include "Speed/Indep/Src/EAXSound/SND_GEN/MAIN_AEMS.h"
#include "Speed/Indep/Src/Generated/Messages/MMiscSound.h"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Misc/Hermes.h"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/EAXSound/Dynamic_Mixer/NFSMixShape.hpp"
#endif

extern float g_SliderValue;
int Debug_Common_FE_OFF = 0;
#ifdef EA_BUILD_A124
bool DEBUG_PRINT_FE_SND = false;
float g_fTableTurnSpeed = 1.0f;
extern Slope TablePitch;
#endif

// Nombre de cada sonido del frontend para depurar. Esta en el fuente de todas las versiones (el DWARF de GameCube
// la da con rango 0xFFFFFFFF: el enlazador la quita), pero solo la A124 la conserva (EAXFrontEnd.cpp:50-151 de
// su .mdebug; las cadenas y el orden de los case son los de su tabla de saltos).
char *GetFESoundName(eMenuSoundTriggers id) {
    switch (id) {
    case UISND_NONE:
        return "UISND_NONE";
    case UISND_COMMON_UP:
        return "UISND_COMMON_UP";
    case UISND_COMMON_DOWN:
        return "UISND_COMMON_DOWN";
    case UISND_COMMON_LEFT:
        return "UISND_COMMON_LEFT";
    case UISND_COMMON_RIGHT:
        return "UISND_COMMON_RIGHT";
    case UISND_COMMON_SELECT:
        return "UISND_COMMON_SELECT";
    case UISND_COMMON_BACK:
        return "UISND_COMMON_BACK";
    case UISND_COMMON_START_PAUSE:
        return "UISND_COMMON_START_PAUSE";
    case UISND_COMMON_WRONG:
        return "UISND_COMMON_WRONG";
    case UISND_COMMON_DLGBOX_IN:
        return "UISND_COMMON_DLGBOX_IN";
    case UISND_COMMON_DLGBOX_OUT:
        return "UISND_COMMON_DLGBOX_OUT";
    case UISND_COMMON_SCROLL_START:
        return "UISND_COMMON_SCROLL_START";
    case UISNN_HUD_ENGAGE_MAIL_POPUP:
        return "UISNN_HUD_ENGAGE_MAIL_POPUP";
    case UISND_HUD_ENGAGE_PHONE:
        return "UISND_HUD_ENGAGE_PHONE";
    case UISND_HUD_ENGAGE_DPAD:
        return "UISND_HUD_ENGAGE_DPAD";
    case UISND_HUD_ENGAGE_MAIL:
        return "UISND_HUD_ENGAGE_MAIL";
    case UISND_MILESTONE_REWARD_SYMBOL_ON:
        return "UISND_MILESTONE_REWARD_SYMBOL_ON";
    case UISND_MILESTONE_REWARD_SYMBOL_MOVE:
        return "UISND_MILESTONE_REWARD_SYMBOL_MOVE";
    case UISND_ENTER_TRIGGER:
        return "UISND_ENTER_TRIGGER";
    case UISND_COMMON_MAX_NUM:
        return "UISND_COMMON_MAX_NUM";
    case UISND_CARSEL_BASSLOOP_RESERVED00:
        return "UISND_CARSEL_BASSLOOP_RESERVED00";
    case UISND_CARSEL_BASSLOOP_RESERVED01:
        return "UISND_CARSEL_BASSLOOP_RESERVED01";
    case UISND_CARSEL_CAMROTATE:
        return "UISND_CARSEL_CAMROTATE";
    case UISND_CARSEL_CAMROTATE_RESERVED00:
        return "UISND_CARSEL_CAMROTATE_RESERVED00";
    case UISND_CARSEL_CAMROTATE_RESERVED01:
        return "UISND_CARSEL_CAMROTATE_RESERVED01";
    case UISND_CUST_INST_PAINT:
        return "UISND_CUST_INST_PAINT";
    case UISND_CUST_PAINT_COLOUR_LEFT:
        return "UISND_CUST_PAINT_COLOUR_LEFT";
    case UISND_CUST_PAINT_COLOUR_RIGHT:
        return "UISND_CUST_PAINT_COLOUR_RIGHT";
    case UISND_CUST_PAINT_TYPE_LEFT:
        return "UISND_CUST_PAINT_TYPE_LEFT";
    case UISND_CUST_PAINT_TYPE_RIGHT:
        return "UISND_CUST_PAINT_TYPE_RIGHT";
    case UISND_UGNEW_KBTYPE:
        return "UISND_UGNEW_KBTYPE";
    case UISND_UGNEW_ENTER:
        return "UISND_UGNEW_ENTER";
    case UISND_UGNEW_DELETE:
        return "UISND_UGNEW_DELETE";
    case UISND_CUST_INST_EXHAUST:
        return "UISND_CUST_INST_EXHAUST";
    case UISND_CUST_INST_GENERIC:
        return "UISND_CUST_INST_GENERIC";
    case UISND_CUST_INST_TURBO:
        return "UISND_CUST_INST_TURBO";
    case UISND_CUST_INST_NOS:
        return "UISND_CUST_INST_NOS";
    case UISND_CUST_INST_TRANSMISSION:
        return "UISND_CUST_INST_TRANSMISSION";
    case UISND_CUST_INST_TIRES:
        return "UISND_CUST_INST_TIRES";
    case UISND_EA_MSGR_OPEN:
        return "UISND_EA_MSGR_OPEN";
    case UISND_EA_MSGR_LOGOFF:
        return "UISND_EA_MSGR_LOGOFF";
    case UISND_EA_MSGR_CHAT_REQ:
        return "UISND_EA_MSGR_CHAT_REQ";
    case UISND_EA_MSGR_MAIL_RECEIVE:
        return "UISND_EA_MSGR_MAIL_RECEIVE";
    case UISND_EA_MSGR_CHALLENGE_REQ:
        return "UISND_EA_MSGR_CHALLENGE_REQ";
    case UISND_MAIN_MENU:
        return "UISND_MAIN_MENU";
    case UISND_MAIN_SUB:
        return "UISND_MAIN_SUB";
    case UISND_IMPOUNDED:
        return "UISND_IMPOUNDED";
    case UISND_RAPSHEET_LOGIN:
        return "UISND_RAPSHEET_LOGIN";
    case UISND_RAPSHEET_LOGIN2:
        return "UISND_RAPSHEET_LOGIN2";
    case UISND_RAPSHEET_MAIN:
        return "UISND_RAPSHEET_MAIN";
    case UISND_RAPSHEET_SUMMARY:
        return "UISND_RAPSHEET_SUMMARY";
    case UISND_RAPSHEET_VEHICLE:
        return "UISND_RAPSHEET_VEHICLE";
    case UISND_RAPSHEET_SELECT:
        return "UISND_RAPSHEET_SELECT";
    case UISND_RAPSHEET_BACKUP:
        return "UISND_RAPSHEET_BACKUP";
    case UISND_RAPSHEET_MOVE_BAR_UP:
        return "UISND_RAPSHEET_MOVE_BAR_UP";
    case UISND_RAPSHEET_MOVE_BAR_DOWN:
        return "UISND_RAPSHEET_MOVE_BAR_DOWN";
    case UISND_RAPSHEET_CTS:
        return "UISND_RAPSHEET_CTS";
    case UISND_RAPSHEET_INFRAC:
        return "UISND_RAPSHEET_INFRAC";
    case UISND_RAPSHEET_PD:
        return "UISND_RAPSHEET_PD";
    case UISND_RAPSHEET_RANKINGS:
        return "UISND_RAPSHEET_RANKINGS";
    case UISND_RAPSHEET_RANKING_DETAIL:
        return "UISND_RAPSHEET_RANKING_DETAIL";
    case UISND_RAPSHEET_TEP:
        return "UISND_RAPSHEET_TEP";
    case UISND_RAPSHEET_EXIT:
        return "UISND_RAPSHEET_EXIT";
    case UISND_MAIN_MENU_ENTER:
        return "UISND_MAIN_MENU_ENTER";
    case UISND_MAIN_MENU_EXIT:
        return "UISND_MAIN_MENU_EXIT";
    case UISND_MAIN_SUB_ENTER:
        return "UISND_MAIN_SUB_ENTER";
    case UISND_MAIN_SUB_EXIT:
        return "UISND_MAIN_SUB_EXIT";
    case UISND_MC_MAIN_ENTER:
        return "UISND_MC_MAIN_ENTER";
    case UISND_BLACKLIST_ENTER:
        return "UISND_BLACKLIST_ENTER";
    case UISND_BLACKLIST_EXIT:
        return "UISND_BLACKLIST_EXIT";
    case UISND_BIO_ENTER:
        return "UISND_BIO_ENTER";
    case UISND_BIO_EXIT:
        return "UISND_BIO_EXIT";
    case UISND_BIO_TO_RIVALCAR_EXIT:
        return "UISND_BIO_TO_RIVALCAR_EXIT";
    case UISND_CUST_MAIN_ENTER:
        return "UISND_CUST_MAIN_ENTER";
    case UISND_CUST_ENTER:
        return "UISND_CUST_ENTER";
    case UISND_CUST_EXIT:
        return "UISND_CUST_EXIT";
    case UISND_SHOWCASE_ENTER:
        return "UISND_SHOWCASE_ENTER";
    case UISND_SHOWCASE_EXIT:
        return "UISND_SHOWCASE_EXIT";
    case UISND_QUICKRACE_BRIEF_ENTER:
        return "UISND_QUICKRACE_BRIEF_ENTER";
    case UISND_QUICK_GAMBLE_BLIP:
        return "UISND_QUICK_GAMBLE_BLIP";
    case UISND_RACESHEET_ENTER:
        return "UISND_RACESHEET_ENTER";
    case UISND_RACESHEET_EXIT:
        return "UISND_RACESHEET_EXIT";
    case UISND_TRACK_SELECT_ENTER:
        return "UISND_TRACK_SELECT_ENTER";
    case UISND_QUICK_BRIEF_EXIT:
        return "UISND_QUICK_BRIEF_EXIT";
    case UISND_QUICK_RACE_CAR_ON:
        return "UISND_QUICK_RACE_CAR_ON";
    case UISND_CAR_SEL_TO_SHOWCASE:
        return "UISND_CAR_SEL_TO_SHOWCASE";
    case UISND_CHAL_SER_ENTER:
        return "UISND_CHAL_SER_ENTER";
    case UISND_CAR_SELECT_ENTER:
        return "UISND_CAR_SELECT_ENTER";
    case UISND_SAFEH_MARK_CONGRATS_OFF:
        return "UISND_SAFEH_MARK_CONGRATS_OFF";
    case UISND_RANDOMIZE_BUTTON:
        return "UISND_RANDOMIZE_BUTTON";
    case UISND_SAFEH_MARK_ENTER:
        return "UISND_SAFEH_MARK_ENTER";
    case UISND_SAFEH_MARK_SPIN_COIN:
        return "UISND_SAFEH_MARK_SPIN_COIN";
    case UISND_CHAL_SER_EXIT:
        return "UISND_CHAL_SER_EXIT";
    case UISND_MAP_LOCK_TARGER:
        return "UISND_MAP_LOCK_TARGER";
    case UISND_MAP_ZOOM_IN:
        return "UISND_MAP_ZOOM_IN";
    case UISND_MAP_ZOOM_OUT:
        return "UISND_MAP_ZOOM_OUT";
    case UISND_MAP_REPOSITION:
        return "UISND_MAP_REPOSITION";
    case UISND_OPTION_MENU_ENTER:
        return "UISND_OPTION_MENU_ENTER";
    case UISND_OPTION_MENU_EXIT:
        return "UISND_OPTION_MENU_EXIT";
    default:
        return "UNKNOWN SOUND";
    }
}

EAXFrontEnd::EAXFrontEnd() {
    int i;
    int Index;

    for (Index = 0; Index < NUM_CAR_INDEXS; Index++) {
        for (i = 0; i < NUM_DRIVE_ON_STATES; i++) {
            this->m_pDriveOnOffSampleHandle[Index][i] = nullptr;
            this->DriveOnFadeOut[Index][i].Initialize(1.0f, 1.0f, 100, LINEAR);
            this->IsEnding[Index][i] = false;
        }
        this->DriveONCarState[Index] = DRIVE_ON_NONE;
    }

    this->m_pSFXOBJ_FEHUD = nullptr;

    {
        for (int k = 0; k < 4; k++) {
            this->m_hydraulicsControls[k] = nullptr;
            this->m_hydraulicsBounce[k] = nullptr;
        }
    }

#ifdef EA_BUILD_A124
    this->m_pPlayLoopFrontEndBASSLoopHandle = nullptr;
    this->m_pPlayLoopFrontEndSampleHandle = nullptr;
#else
    this->m_pPlayRapSheet = nullptr;
#endif
}

EAXFrontEnd::~EAXFrontEnd() {
    for (int k = 0; k < 4; k++) {
        delete this->m_hydraulicsControls[k];
        this->m_hydraulicsBounce[k] = nullptr;
        delete this->m_hydraulicsControls[k];
        this->m_hydraulicsControls[k] = nullptr;
    }

#ifndef EA_BUILD_A124
    delete this->m_pPlayRapSheet;
    this->m_pPlayRapSheet = nullptr;
#endif
}

void EAXFrontEnd::AttachSFXOBJ(SFX_Base *psfx, eSFXOBJ_MAIN_TYPES sfxtype) {
    if (sfxtype == SFXOBJ_FEHUD) {
        this->m_pSFXOBJ_FEHUD = psfx;
    }
}

void EAXFrontEnd::Initialize() {}

void EAXFrontEnd::Stop(eMenuSoundTriggers etrigger) {
#ifndef EA_BUILD_A124
    if (IsSoundEnabled == 0) {
        return;
    }
    if (this->m_pPlayRapSheet && this->m_pPlayRapSheet->GetId() == etrigger) {
        delete this->m_pPlayRapSheet;
    }
#endif
}

int EAXFrontEnd::Play(eMenuSoundTriggers etrigger) {
    if (IsSoundEnabled == 0) {
        return -1;
    }

    if (this->m_pSFXOBJ_FEHUD == nullptr) {
        return 0;
    }
    int nvol = this->m_pSFXOBJ_FEHUD->GetDMixOutput(2, DMX_VOL);

    g_pEAXSound->SetCsisName(this->m_pSFXOBJ_FEHUD);

#ifndef EA_BUILD_A124
    int testID = static_cast<int>(etrigger) + 23;

    // La A124 no tiene la hoja de antecedentes: solo la rama del sonido normal (su asm no compara etrigger).
    if (static_cast<unsigned int>(etrigger - 80) < 16 && testID != 0x6E && testID != 0x6F) {
        delete this->m_pPlayRapSheet;
        this->m_pPlayRapSheet = new Csis::PlayFrontEndSample_RS(etrigger, nvol, 0x1000, 0);
    } else
#endif
    {
        int getref;

        this->m_pPlayFrontEndSampleHandle = new Csis::PlayFrontEndSample(etrigger, nvol, 0x1000, 0);
        getref = this->m_pPlayFrontEndSampleHandle->GetRefCount();
        delete this->m_pPlayFrontEndSampleHandle;
        this->m_pPlayFrontEndSampleHandle = nullptr;
    }

    return 0;
}

int EAXFrontEnd::Play(void *peventst) {
    if (IsSoundEnabled == 0) {
        return -1;
    }

    if (this->m_pSFXOBJ_FEHUD) {
        if (!this->m_pSFXOBJ_FEHUD->GetOutputBlockPtr()) {
            return 0;
        }

        if (peventst) {
            PlayFrontEndSampleSt *pst = static_cast<PlayFrontEndSampleSt *>(peventst);
            int nvol = pst->volume;

            int DMixVol = this->m_pSFXOBJ_FEHUD->GetDMixOutput(2, DMX_VOL);
            nvol = nvol * DMixVol >> 15;

            g_pEAXSound->SetCsisName(this->m_pSFXOBJ_FEHUD);
            this->m_pPlayFrontEndSampleHandle = new Csis::PlayFrontEndSample(pst->id, nvol, pst->pitch, pst->azimuth);
            if (this->m_pPlayFrontEndSampleHandle) {
                delete this->m_pPlayFrontEndSampleHandle;
            }
            this->m_pPlayFrontEndSampleHandle = nullptr;
        }
    }

    return -1;
}

void EAXFrontEnd::Update(void *peventst) {
    this->UpdateDriveOn();
#ifdef EA_BUILD_A124
    if (this->m_pPlayLoopFrontEndBASSLoopHandle) {
        this->m_pPlayLoopFrontEndBASSLoopHandle->SetVolume(this->m_pSFXOBJ_FEHUD->GetDMixOutput(2, DMX_VOL));
        this->m_pPlayLoopFrontEndBASSLoopHandle->SetPitch(static_cast<int>(TablePitch.GetValue(g_fTableTurnSpeed)));
        this->m_pPlayLoopFrontEndBASSLoopHandle->CommitMemberData();
    }
#endif
}

void *EAXFrontEnd::GetEventPointer(int neventindex) {
    return nullptr;
}

void EAXFrontEnd::UpdateDriveOn() {}

void EAXFrontEnd::SetFEDrivingCarState(bVector3 *, bVector3 *, Camera *, int) {}

void EAXFrontEnd::DestroyAllDriveOnSnds() {}

EAXCommon::EAXCommon() {
    this->mMsgMiscSound =
        Hermes::Handler::Create<MMiscSound, EAXCommon, EAXCommon>(this, &EAXCommon::MsgPlayMiscSound, "Snd", 0);
    this->m_pSFXOBJ_FEHUD = nullptr;
    this->m_pRadar = nullptr;
}

EAXCommon::~EAXCommon() {
    if (this->mMsgMiscSound) {
        Hermes::Handler::Destroy(this->mMsgMiscSound);
    }
}

void EAXCommon::AttachSFXOBJ(SFX_Base *psfx, eSFXOBJ_MAIN_TYPES sfxtype) {
    if (sfxtype == SFXOBJ_FEHUD) {
        this->m_pSFXOBJ_FEHUD = psfx;
    }
}

void EAXCommon::MsgPlayMiscSound(const MMiscSound &message) {
    if (message.GetSoundID() == 0) {
        if (this->m_pRadar) {
            delete this->m_pRadar;
        }

        g_pEAXSound->SetCsisName("FE Radar");
        this->m_pRadar = new Csis::FX_Radar(0, this->m_pSFXOBJ_FEHUD->GetDMixOutput(3, DMX_VOL), 0, 0, 0);
    }
}

void EAXCommon::Initialize() {}

void EAXCommon::Stop(eMenuSoundTriggers etrigger) {}

int EAXCommon::Play(eMenuSoundTriggers etrigger) {
    int nvol;

    if (!IsSoundEnabled) return -1;
    if (Debug_Common_FE_OFF) return -1;

    if (!this->m_pSFXOBJ_FEHUD) return -1;
#ifndef EA_BUILD_A124
    if (!this->m_pSFXOBJ_FEHUD->GetOutputBlockPtr()) return 0;
#endif

    nvol = 0;
    nvol = this->m_pSFXOBJ_FEHUD->GetDMixOutput(1, DMX_VOL);
    if (g_SliderValue > -1.0f) {
        int CurSliderVol = bClamp(static_cast<int>(g_SliderValue * 32767.0f), 0, 0x7FFF);

#ifdef EA_BUILD_A124
        // La A124 pasa el volumen del deslizador por la curva de mezcla (su asm llama a GetCurveOutput).
        nvol = nvol * NFSMixShape::GetCurveOutput(SHAPE_UP_ONE_MIN_EQPWR, CurSliderVol, false) >> 15;
#else
        nvol = nvol * CurSliderVol >> 15;
#endif
        g_SliderValue = -1.0f;
    }

#ifndef EA_BUILD_A124
    if (etrigger == UISND_ENTER_TRIGGER) {
        this->m_pSFXOBJ_FEHUD->SetDMIX_Input(4, 0x7FFF);
    }
#endif

#if defined(EA_PLATFORM_PLAYSTATION2)
    // PS2 (la A124 y SLUS): los sonidos comunes empiezan 30 muestras mas alla en su banco.
    etrigger = static_cast<eMenuSoundTriggers>(etrigger + 30);
#endif
    g_pEAXSound->SetCsisName(this->m_pSFXOBJ_FEHUD);
    this->m_pPlayCommonSampleHandle = new Csis::PlayCommonSample(static_cast<int>(etrigger), nvol, 0x1000, 0);
    if (this->m_pPlayCommonSampleHandle) {
        delete this->m_pPlayCommonSampleHandle;
    }
    this->m_pPlayCommonSampleHandle = nullptr;
    return 0;
}

int EAXCommon::Play(void *peventst) {
    if (IsSoundEnabled == 0) {
        return -1;
    }

    if (this->m_pSFXOBJ_FEHUD) {
        if (!this->m_pSFXOBJ_FEHUD->GetOutputBlockPtr()) {
            return 0;
        }

        if (peventst) {
            PlayCommonSampleSt *pst = static_cast<PlayCommonSampleSt *>(peventst);
            int nvol = pst->volume;

            int DMixVol = this->m_pSFXOBJ_FEHUD->GetDMixOutput(1, DMX_VOL);
            nvol = nvol * DMixVol >> 15;

            g_pEAXSound->SetCsisName(this->m_pSFXOBJ_FEHUD);
            this->m_pPlayCommonSampleHandle = new Csis::PlayCommonSample(pst->id, nvol, pst->pitch, pst->azimuth);
            if (this->m_pPlayCommonSampleHandle) {
                delete this->m_pPlayCommonSampleHandle;
            }
            this->m_pPlayCommonSampleHandle = nullptr;
        }
    }

    return -1;
}

void EAXCommon::Update(void *peventst) {
    (void)peventst;

    if (this->m_pRadar) {
        int refCount = this->m_pRadar->GetRefCount();

        if (refCount <= 1) {
            delete this->m_pRadar;
            this->m_pRadar = nullptr;
        }
    }

#ifndef EA_BUILD_A124
    if (this->m_pSFXOBJ_FEHUD) {
        this->m_pSFXOBJ_FEHUD->SetDMIX_Input(4, 0);
    }
#endif
}

void *EAXCommon::GetEventPointer(int neventindex) {
    return nullptr;
}
