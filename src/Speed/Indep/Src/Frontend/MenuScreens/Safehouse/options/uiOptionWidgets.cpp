#include "uiOptionWidgets.hpp"
#include "Speed/Indep/Src/Camera/CameraInfo.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionsScreen.hpp"

#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionsController.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionsMain.hpp"
#include "Speed/Indep/Src/Input/IOModule.h"
#include "Speed/Indep/Src/Misc/Joystick.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"

#ifdef EA_PLATFORM_GAMECUBE
#include "Speed/GameCube/bWare/GameCube/dolphinsdk/include/dolphin/os.h"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/Database/VehicleDB.hpp"
#endif

struct FEString;
struct FEObject;
#include "Speed/Indep/Src/FEng/FEImage.h"
#include "Speed/Indep/Src/Generated/LanguageHashes.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"
#include "Speed/Indep/Src/Online/VoiceCore.hpp"
#endif

void FEngSetInvisible(const char* pkg_name, unsigned int obj_hash);
void FEngSetVisible(const char* pkg_name, unsigned int obj_hash);

// [fe3] POVTypes ya esta en Camera/CameraInfo.hpp

void MemcardEnter(const char* from, const char* to, unsigned int op,
                  void (*pTermFunc)(void*), void* pTermFuncParam,
                  unsigned int msgSuccess, unsigned int msgFailed);

void OMAudio::React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1,
                    unsigned int param2) {
    if (data != 0xC407210) {
        return;
    }
    FEDatabase->GetOptionsSettings()->CurrentCategory = OC_AUDIO;
}

void OMVideo::React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1,
                    unsigned int param2) {
    if (data != 0xC407210) {
        return;
    }
    FEDatabase->GetOptionsSettings()->CurrentCategory = OC_VIDEO;
}

void OMGameplay::React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1,
                       unsigned int param2) {
    if (data != 0xC407210) {
        return;
    }
    FEDatabase->GetOptionsSettings()->CurrentCategory = OC_GAMEPLAY;
}

void OMPlayer::React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1,
                     unsigned int param2) {
    if (data != 0xC407210) {
        return;
    }
    FEDatabase->GetOptionsSettings()->CurrentCategory = OC_PLAYER;
}

void OMController::React(const char* pkg_name, unsigned int data, FEObject* obj,
                         unsigned int param1, unsigned int param2) {
    if (data != 0xC407210) {
        return;
    }
    FEDatabase->GetOptionsSettings()->CurrentCategory = OC_CONTROLS;
}

void OMEATrax::React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1,
                     unsigned int param2) {
    if (data != 0xC407210) {
        return;
    }
    FEDatabase->GetOptionsSettings()->CurrentCategory = OC_EATRAX;
}

void OMOnline::React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1,
                      unsigned int param2) {
    if (data != 0xC407210) {
        return;
    }
    FEDatabase->GetOptionsSettings()->CurrentCategory = OC_ONLINE;
}

void OMTrailers::React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1,
                        unsigned int param2) {
    if (data != 0xC407210) {
        return;
    }
    FEDatabase->GetOptionsSettings()->CurrentCategory = OC_TRAILERS;
}

void OMCredits::React(const char* pkg_name, unsigned int data, FEObject* obj, unsigned int param1,
                      unsigned int param2) {
    if (data != 0xC407210) {
        return;
    }
    FEDatabase->GetOptionsSettings()->CurrentCategory = OC_CREDITS;
}

void AOMasterVol::Act(const char* parent_pkg, unsigned int data) {
    UpdateSlider(data);
    FEDatabase->GetAudioSettings()->MasterVol = GetValue();
    g_pEAXSound->UpdateVolumes(FEDatabase->GetAudioSettings(), GetValue());
    Update(data);
}

void AOMasterVol::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x26782C3E);
    SetValue(FEDatabase->GetAudioSettings()->MasterVol);
    DrawSlider();
}

void AOMasterVol::SetInitialValues() {
    SetSliderValues(0.0f, 1.0f, 0.1f, FEDatabase->GetAudioSettings()->MasterVol);
}

void AOSFXMasterVol::Act(const char* parent_pkg, unsigned int data) {
    UpdateSlider(data);
    FEDatabase->GetAudioSettings()->SoundEffectsVol = GetValue();
    g_pEAXSound->UpdateVolumes(FEDatabase->GetAudioSettings(), GetValue());
    Update(data);
}

void AOSFXMasterVol::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0xFD487543);
    SetValue(FEDatabase->GetAudioSettings()->SoundEffectsVol);
    DrawSlider();
}

void AOSFXMasterVol::SetInitialValues() {
    SetSliderValues(0.0f, 1.0f, 0.1f, FEDatabase->GetAudioSettings()->SoundEffectsVol);
}

void AOInteractiveMusicMode::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        FEDatabase->GetAudioSettings()->InteractiveMusicMode =
            static_cast<int>(FEDatabase->GetAudioSettings()->InteractiveMusicMode == 0);
    }
    Update(data);
}

void AOInteractiveMusicMode::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0xA3DBB390);
    unsigned int hash = FEDatabase->GetAudioSettings()->InteractiveMusicMode != 0 ? LANGUAGE_COMMON_ON : LANGUAGE_COMMON_OFF;
    FEngSetLanguageHash(GetDataObject(), hash);
}

void AOEATraxMusicMode::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        FEDatabase->GetAudioSettings()->EATraxMode =
            static_cast<int>(FEDatabase->GetAudioSettings()->EATraxMode == 0);
    }
    Update(data);
}

void AOEATraxMusicMode::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0xDCFB6B36);
    unsigned int hash = FEDatabase->GetAudioSettings()->EATraxMode != 0 ? LANGUAGE_COMMON_ON : LANGUAGE_COMMON_OFF;
    FEngSetLanguageHash(GetDataObject(), hash);
}

void AOCarVol::Act(const char* parent_pkg, unsigned int data) {
    UpdateSlider(data);
    FEDatabase->GetAudioSettings()->CarVol = GetValue();
    g_pEAXSound->UpdateVolumes(FEDatabase->GetAudioSettings(), GetValue());
    Update(data);
}

void AOCarVol::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x218E4B08);
    SetValue(FEDatabase->GetAudioSettings()->CarVol);
    DrawSlider();
}

void AOCarVol::SetInitialValues() {
    SetSliderValues(0.0f, 1.0f, 0.1f, FEDatabase->GetAudioSettings()->CarVol);
}

void AOEngineVol::Act(const char* parent_pkg, unsigned int data) {
    UpdateSlider(data);
    FEDatabase->GetAudioSettings()->EngineVol = GetValue();
    g_pEAXSound->UpdateVolumes(FEDatabase->GetAudioSettings(), GetValue());
    Update(data);
}

void AOEngineVol::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0xA2B1F888);
    SetValue(FEDatabase->GetAudioSettings()->EngineVol);
    DrawSlider();
}

void AOEngineVol::SetInitialValues() {
    SetSliderValues(0.0f, 1.0f, 0.1f, FEDatabase->GetAudioSettings()->EngineVol);
}

void AOSpeechVol::Act(const char* parent_pkg, unsigned int data) {
    UpdateSlider(data);
    FEDatabase->GetAudioSettings()->SpeechVol = GetValue();
    g_pEAXSound->UpdateVolumes(FEDatabase->GetAudioSettings(), GetValue());
    Update(data);
}

void AOSpeechVol::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x9E5FB82A);
    SetValue(FEDatabase->GetAudioSettings()->SpeechVol);
    DrawSlider();
}

void AOSpeechVol::SetInitialValues() {
    SetSliderValues(0.0f, 1.0f, 0.1f, FEDatabase->GetAudioSettings()->SpeechVol);
}

void AOFEMusicVol::Act(const char* parent_pkg, unsigned int data) {
    UpdateSlider(data);
    float value = GetValue();
#ifndef EA_BUILD_A124
    if (bEqual(0.0f, value, 0.001f)) {
        value = 0.0f;
    }
#endif
    FEDatabase->GetAudioSettings()->FEMusicVol = value;
    g_pEAXSound->UpdateVolumes(FEDatabase->GetAudioSettings(), GetValue());
    Update(data);
}

void AOFEMusicVol::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x418E681D);
    SetValue(FEDatabase->GetAudioSettings()->FEMusicVol);
    DrawSlider();
}

void AOFEMusicVol::SetInitialValues() {
    SetSliderValues(0.0f, 1.0f, 0.1f, FEDatabase->GetAudioSettings()->FEMusicVol);
}

void AOIGMusicVol::Act(const char* parent_pkg, unsigned int data) {
    UpdateSlider(data);
    FEDatabase->GetAudioSettings()->IGMusicVol = GetValue();
    g_pEAXSound->UpdateVolumes(FEDatabase->GetAudioSettings(), GetValue());
    Update(data);
}

void AOIGMusicVol::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0xDF21EAC2);
    SetValue(FEDatabase->GetAudioSettings()->IGMusicVol);
    DrawSlider();
}

void AOIGMusicVol::SetInitialValues() {
    SetSliderValues(0.0f, 1.0f, 0.1f, FEDatabase->GetAudioSettings()->IGMusicVol);
}

void AOAudioMode::Act(const char* parent_pkg, unsigned int data) {
    int mode = FEDatabase->GetAudioSettings()->AudioMode;
    if (data == 0x9120409E) {
        mode--;
        if (mode < 0) {
#ifdef EA_PLATFORM_WIN32
            mode = 1;
#else
            mode = 2;
#endif
        }
    } else if (data == 0xB5971BF1) {
        mode++;
#ifdef EA_PLATFORM_WIN32
        if (mode > 1) {
#else
        if (mode > 2) {
#endif
            mode = 0;
        }
    }
    if (FEDatabase->GetAudioSettings()->AudioMode != mode) {
#ifdef EA_PLATFORM_GAMECUBE
        OSSetSoundMode(mode != 0);
#endif
    }
    FEDatabase->GetAudioSettings()->AudioMode = mode;
    Update(data);
}

void AOAudioMode::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x2881AB87);
    unsigned int hash = 0;
    int mode = FEDatabase->GetAudioSettings()->AudioMode;
    switch (mode) {
    case 0:
        hash = 0xC50FA35F;
        break;
    case 1:
        hash = 0x55DA8BF8;
        break;
#ifndef EA_PLATFORM_WIN32 // PC solo tiene los dos primeros modos (como Act)
    case 2:
        hash = 0xF6FAFF24;
        break;
#endif
    }
    FEngSetLanguageHash(GetDataObject(), hash);
}

#ifdef EA_BUILD_A124
void OOShowPlayers::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        FEDatabase->GetOnlineSettings()->ShowPlayerNames = !FEDatabase->GetOnlineSettings()->ShowPlayerNames;
    }
    Update(data);
}

void OOShowPlayers::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x0B660729);
    if (FEDatabase->GetOnlineSettings()->ShowPlayerNames) {
        FEngSetLanguageHash(GetDataObject(), LANGUAGE_COMMON_ON);
    } else {
        FEngSetLanguageHash(GetDataObject(), LANGUAGE_COMMON_OFF);
    }
}

void OOAppearOnline::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        UIOptionsScreen::mTempAppearOffline = !UIOptionsScreen::mTempAppearOffline;
        if (UIOptionsScreen::mTempAppearOffline == true) {
            DialogInterface::ShowOneButton(parent_pkg, "", static_cast<eDialogTitle>(1), LANGUAGE_COMMON_OK, 0xFFFFFFFF, 0xFFFFFFFF,
                                           GetLocalizedString(0x66886589));
        }
    }
    Update(data);
}

void OOAppearOnline::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x7BF4E178);
    if (UIOptionsScreen::mTempAppearOffline) {
        FEngSetLanguageHash(GetDataObject(), LANGUAGE_COMMON_YES);
    } else {
        FEngSetLanguageHash(GetDataObject(), LANGUAGE_COMMON_NO);
    }
}

void OOBadConnectionWarning::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        FEDatabase->GetOnlineSettings()->ShowBadConnectionWarning = !FEDatabase->GetOnlineSettings()->ShowBadConnectionWarning;
    }
    Update(data);
}

void OOBadConnectionWarning::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0xD1009B34);
    if (FEDatabase->GetOnlineSettings()->ShowBadConnectionWarning) {
        FEngSetLanguageHash(GetDataObject(), LANGUAGE_COMMON_ON);
    } else {
        FEngSetLanguageHash(GetDataObject(), LANGUAGE_COMMON_OFF);
    }
}

void OOVoiceSpeaker::Act(const char* parent_pkg, unsigned int data) {
    if (VoiceCore::instance().IsHeadsetConnected()) {
        if (data == 0x9120409E || data == 0xB5971BF1) {
            VoiceCore::instance().SetSpeakerState(!VoiceCore::instance().GetSpeakerState());
            if (volumewidget) {
                if (VoiceCore::instance().GetSpeakerState()) {
                    if (!volumewidget->IsEnabled()) {
                        volumewidget->Enable();
                    }
                } else if (volumewidget->IsEnabled()) {
                    volumewidget->Disable();
                }
                volumewidget->Draw();
            }
        }
        BlinkArrows(data);
    }
    Draw();
}

void OOVoiceSpeaker::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x52BCEECC);
    if (VoiceCore::instance().GetSpeakerState()) {
        FEngSetLanguageHash(GetDataObject(), 0x86CC69B7);
    } else {
        FEngSetLanguageHash(GetDataObject(), 0x1E1C73AA);
    }
}

void OOVoiceVolume::Act(const char* parent_pkg, unsigned int data) {
    UpdateSlider(data);
    VoiceCore::instance().SetPlaybackVolume(GetValue());
    Update(data);
}

void OOVoiceVolume::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0xD2822102);
    SetValue(VoiceCore::instance().GetPlaybackVolume());
    DrawSlider();
}

void OOVoiceVolume::SetInitialValues() {
    SetSliderValues(0.0f, 1.0f, 0.1f, VoiceCore::instance().GetPlaybackVolume());
}

void OOVoiceVolume::Enable() {
    FESliderWidget::Enable();
    FEngSetScript(GetTitleObject(), FEHASH_UNHIGHLIGHT, true);
    if (GetBacking()) {
        FEngSetScript(GetBacking(), FEHASH_UNHIGHLIGHT, true);
    }
}

void OOVoiceVolume::Disable() {
    FESliderWidget::Disable();
    FEngSetScript(GetTitleObject(), FEHASH_DISABLE, true);
    if (GetBacking()) {
        FEngSetScript(GetBacking(), FEHASH_DISABLE, true);
    }
}
#endif

void VOWideScreen::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        FEDatabase->GetVideoSettings()->WideScreen = !FEDatabase->GetVideoSettings()->WideScreen;
    }
    Update(data);
}

void VOWideScreen::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0xD3588630);
    unsigned int hash = FEDatabase->GetVideoSettings()->WideScreen ? LANGUAGE_COMMON_ON : LANGUAGE_COMMON_OFF;
    FEngSetLanguageHash(GetDataObject(), hash);
}

void VOScreenPosHoriz::Act(const char* parent_pkg, unsigned int data) {
    UpdateSlider(data);
    FEDatabase->GetVideoSettings()->ScreenOffsetX = GetValue();
    Update(data);
}

void VOScreenPosHoriz::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0xA9CFE581);
    SetValue(FEDatabase->GetVideoSettings()->ScreenOffsetX);
    DrawSlider();
}

void VOScreenPosHoriz::SetInitialValues() {
    SetSliderValues(-15.0f, 15.0f, 1.0f, FEDatabase->GetVideoSettings()->ScreenOffsetX);
}

void VOScreenPosVert::Act(const char* parent_pkg, unsigned int data) {
    UpdateSlider(data);
    FEDatabase->GetVideoSettings()->ScreenOffsetY = GetValue();
    Update(data);
}

void VOScreenPosVert::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x375B62F1);
    SetValue(FEDatabase->GetVideoSettings()->ScreenOffsetY);
    DrawSlider();
}

void VOScreenPosVert::SetInitialValues() {
    SetSliderValues(-15.0f, 15.0f, 1.0f, FEDatabase->GetVideoSettings()->ScreenOffsetY);
}

void VOScreenScale::Act(const char* parent_pkg, unsigned int data) {
    UpdateSlider(data);
    FEDatabase->GetVideoSettings()->FEScale = GetValue();
    Update(data);
}

void VOScreenScale::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x39EF3A6C);
    SetValue(FEDatabase->GetVideoSettings()->FEScale);
    DrawSlider();
}

void VOScreenScale::SetInitialValues() {
    SetSliderValues(1.0f, 1.125f, 0.0125f, FEDatabase->GetVideoSettings()->FEScale);
}

void VOTimeOfDay::Act(const char* parent_pkg, unsigned int data) {
    UpdateSlider(data);
    Update(data);
}

void VOTimeOfDay::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x999D1383);
    DrawSlider();
}

void VOTimeOfDay::SetInitialValues() {}

void GODamage::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        FEDatabase->GetGameplaySettings()->Damage = !FEDatabase->GetGameplaySettings()->Damage;
    }
    Update(data);
}

void GODamage::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x1582ADFF);
    unsigned int hash = FEDatabase->GetGameplaySettings()->Damage ? LANGUAGE_COMMON_ON : LANGUAGE_COMMON_OFF;
    FEngSetLanguageHash(GetDataObject(), hash);
}

void GOAutoSave::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        FEDatabase->GetGameplaySettings()->AutoSaveOn = !FEDatabase->GetGameplaySettings()->AutoSaveOn;
        if (FEDatabase->GetGameplaySettings()->AutoSaveOn) {
            MemcardEnter(parent_pkg, parent_pkg, 0xA1, nullptr, nullptr, 0, 0);
        }
    }
    Update(data);
}

void GOAutoSave::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0xD1056C88);
    unsigned int hash = FEDatabase->GetGameplaySettings()->AutoSaveOn ? LANGUAGE_COMMON_ON : LANGUAGE_COMMON_OFF;
    FEngSetLanguageHash(GetDataObject(), hash);
}

void GOJumpCams::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        FEDatabase->GetGameplaySettings()->JumpCam = !FEDatabase->GetGameplaySettings()->JumpCam;
    }
    Update(data);
}

void GOJumpCams::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0xF26A5CBF);
    unsigned int hash = FEDatabase->GetGameplaySettings()->JumpCam ? LANGUAGE_COMMON_ON : LANGUAGE_COMMON_OFF;
    FEngSetLanguageHash(GetDataObject(), hash);
}

void GORearview::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        FEDatabase->GetGameplaySettings()->RearviewOn = !FEDatabase->GetGameplaySettings()->RearviewOn;
    }
    Update(data);
}

void GORearview::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x85A6CE05);
    unsigned int hash = FEDatabase->GetGameplaySettings()->RearviewOn ? LANGUAGE_COMMON_ON : LANGUAGE_COMMON_OFF;
    FEngSetLanguageHash(GetDataObject(), hash);
}

void GOSpeedoUnits::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        unsigned char units = FEDatabase->GetGameplaySettings()->SpeedoUnits;
        if (units == 0) {
            FEDatabase->GetGameplaySettings()->SpeedoUnits = 1;
        } else if (units == 1) {
            FEDatabase->GetGameplaySettings()->SpeedoUnits = 0;
        }
    }
    Update(data);
}

void GOSpeedoUnits::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x01E19173);
    unsigned int hash = FEDatabase->GetGameplaySettings()->SpeedoUnits == 0 ? 0xFBD74FC5 : 0xAF70E736;
    FEngSetLanguageHash(GetDataObject(), hash);
}

void GORacingMiniMap::Act(const char* parent_pkg, unsigned int data) {
    int mode = FEDatabase->GetGameplaySettings()->RacingMiniMapMode;
    if (data == 0x9120409E) {
        mode--;
        if (mode < 0) {
            mode = 2;
        }
    } else if (data == 0xB5971BF1) {
        mode++;
        if (mode > 2) {
            mode = 0;
        }
    }
    FEDatabase->GetGameplaySettings()->RacingMiniMapMode = static_cast<unsigned char>(mode);
    Update(data);
}

void GORacingMiniMap::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x9FA5EC9E);
    unsigned int hash = 0;
    unsigned char mode = FEDatabase->GetGameplaySettings()->RacingMiniMapMode;
    switch (mode) {
    case 1:
        hash = 0xF4B00E99;
        break;
    case 0:
        hash = 0xF75595F2;
        break;
    case 2:
        hash = LANGUAGE_COMMON_OFF;
        break;
    }
    FEngSetLanguageHash(GetDataObject(), hash);
}

void GOExploringMiniMap::Act(const char* parent_pkg, unsigned int data) {
    int mode = FEDatabase->GetGameplaySettings()->ExploringMiniMapMode;
    if (data == 0x9120409E) {
        mode--;
        if (mode < 0) {
            mode = 2;
        }
    } else if (data == 0xB5971BF1) {
        mode++;
        if (mode > 2) {
            mode = 0;
        }
    }
    FEDatabase->GetGameplaySettings()->ExploringMiniMapMode = static_cast<unsigned char>(mode);
    Update(data);
}

void GOExploringMiniMap::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0xC6269082);
    unsigned int hash = 0;
    unsigned char mode = FEDatabase->GetGameplaySettings()->ExploringMiniMapMode;
    switch (mode) {
    case 1:
        hash = 0xF4B00E99;
        break;
    case 0:
        hash = 0xF75595F2;
        break;
    case 2:
        hash = LANGUAGE_COMMON_OFF;
        break;
    }
    FEngSetLanguageHash(GetDataObject(), hash);
}

void POTransmission::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        int player = GetPlayerToEditForOptions();
        int trans = FEDatabase->GetPlayerSettings(player)->Transmission;
        if (trans == 0) {
            trans = 1;
        } else if (trans == 1) {
            trans = 0;
        }
        player = GetPlayerToEditForOptions();
        FEDatabase->GetPlayerSettings(player)->Transmission = trans;
    }
    Update(data);
}

void POTransmission::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0xD31407E7);
    unsigned int hash = 0;
    int player = GetPlayerToEditForOptions();
    unsigned char trans = FEDatabase->GetPlayerSettings(player)->Transmission;
    switch (trans) {
    case 0:
        hash = LANGUAGE_COMMON_AUTO;
        break;
    case 1:
        hash = LANGUAGE_COMMON_MANUAL;
        break;
    }
    FEngSetLanguageHash(GetDataObject(), hash);
}

void POTransPrompt::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        int player = GetPlayerToEditForOptions();
        FEDatabase->GetPlayerSettings(player)->TransmissionPromptOn =
            !FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions())->TransmissionPromptOn;
    }
    Update(data);
}

void POTransPrompt::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x50E2172B);
    int player = GetPlayerToEditForOptions();
    unsigned int hash = FEDatabase->GetPlayerSettings(player)->TransmissionPromptOn ? LANGUAGE_COMMON_ON : LANGUAGE_COMMON_OFF;
    FEngSetLanguageHash(GetDataObject(), hash);
}

void POHandling::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        int player = GetPlayerToEditForOptions();
        unsigned char handling = FEDatabase->GetPlayerSettings(player)->Handling;
        if (handling == 0) {
            handling = 1;
        } else if (handling == 1) {
            handling = 0;
        }
        player = GetPlayerToEditForOptions();
        FEDatabase->GetPlayerSettings(player)->Handling = handling;
    }
    Update(data);
}

void POHandling::Draw() {
    unsigned int hash = 0;
    FEngSetLanguageHash(GetTitleObject(), 0xDF06ED22);
    int player = GetPlayerToEditForOptions();
    unsigned char handling = FEDatabase->GetPlayerSettings(player)->Handling;
    switch (handling) {
    case 0:
        hash = LANGUAGE_COMMON_ON;
        break;
    case 1:
        hash = LANGUAGE_COMMON_OFF;
        break;
    }
    FEngSetLanguageHash(GetDataObject(), hash);
}

void PODriveCam::Act(const char* parent_pkg, unsigned int data) {
    int player = GetPlayerToEditForOptions();
    int cam = static_cast<int>(FEDatabase->GetPlayerSettings(player)->CurCam);
    if (data == 0x9120409E) {
        do {
            cam--;
            if (cam < 0) {
                cam = 6;
            }
        } while (!IsPlayerCameraSelectable(
            GetPOVTypeFromPlayerCamera(static_cast<ePlayerSettingsCameras>(cam))));
    } else if (data == 0xB5971BF1) {
        do {
            cam++;
            if (cam > 6) {
                cam = 0;
            }
        } while (!IsPlayerCameraSelectable(
            GetPOVTypeFromPlayerCamera(static_cast<ePlayerSettingsCameras>(cam))));
    }
    player = GetPlayerToEditForOptions();
    FEDatabase->GetPlayerSettings(player)->CurCam = static_cast<ePlayerSettingsCameras>(cam);
    Update(data);
}

void PODriveCam::Draw() {
    unsigned int hash = 0;
    FEngSetLanguageHash(GetTitleObject(), 0xF6CCDC5F);
    int player = GetPlayerToEditForOptions();
    int cam = static_cast<int>(FEDatabase->GetPlayerSettings(player)->CurCam);
    switch (cam) {
    case 3:
        hash = 0x1EA4CEC2;
        break;
    case 2:
        hash = 0x5AE3441F;
        break;
    case 1:
        hash = 0x414F19D7;
        break;
    case 0:
        hash = 0xC3E9AE58;
        break;
    case 4:
        hash = 0x916039B4;
        break;
    }
    FEngSetLanguageHash(GetDataObject(), hash);
}

void POGauges::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        int player = GetPlayerToEditForOptions();
        FEDatabase->GetPlayerSettings(player)->GaugesOn = !FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions())->GaugesOn;
    }
    Update(data);
}

void POGauges::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0xAC148579);
    int player = GetPlayerToEditForOptions();
    unsigned int hash = FEDatabase->GetPlayerSettings(player)->GaugesOn ? LANGUAGE_COMMON_ON : LANGUAGE_COMMON_OFF;
    FEngSetLanguageHash(GetDataObject(), hash);
}

void POPosition::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        int player = GetPlayerToEditForOptions();
        FEDatabase->GetPlayerSettings(player)->PositionOn = !FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions())->PositionOn;
    }
    Update(data);
}

void POPosition::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x82CD8F92);
    int player = GetPlayerToEditForOptions();
    unsigned int hash = FEDatabase->GetPlayerSettings(player)->PositionOn ? LANGUAGE_COMMON_ON : LANGUAGE_COMMON_OFF;
    FEngSetLanguageHash(GetDataObject(), hash);
}

void POLapInfo::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        int player = GetPlayerToEditForOptions();
        FEDatabase->GetPlayerSettings(player)->LapInfoOn = !FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions())->LapInfoOn;
    }
    Update(data);
}

void POLapInfo::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x969F96A5);
    int player = GetPlayerToEditForOptions();
    unsigned int hash = FEDatabase->GetPlayerSettings(player)->LapInfoOn ? LANGUAGE_COMMON_ON : LANGUAGE_COMMON_OFF;
    FEngSetLanguageHash(GetDataObject(), hash);
}

void POScore::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        int player = GetPlayerToEditForOptions();
        FEDatabase->GetPlayerSettings(player)->ScoreOn = !FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions())->ScoreOn;
    }
    Update(data);
}

void POScore::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0xC03F9F19);
    int player = GetPlayerToEditForOptions();
    unsigned int hash = FEDatabase->GetPlayerSettings(player)->ScoreOn ? LANGUAGE_COMMON_ON : LANGUAGE_COMMON_OFF;
    FEngSetLanguageHash(GetDataObject(), hash);
}

void POSplitTime::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        int type = FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions())->SplitTimeType ? 0 : 4;
        FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions())->SplitTimeType = type;
    }
    Update(data);
}

void POSplitTime::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x084BC378);
    unsigned int hash = 0;
    int player = GetPlayerToEditForOptions();
    unsigned char splitTime = FEDatabase->GetPlayerSettings(player)->SplitTimeType;
    switch (splitTime) {
    case 0:
        hash = LANGUAGE_COMMON_ON;
        break;
    case 1:
        hash = 0xC44D3943;
        break;
    case 2:
        hash = 0x17FAFC32;
        break;
    case 3:
        hash = 0x1EA459F8;
        break;
    case 4:
        hash = LANGUAGE_COMMON_OFF;
        break;
    }
    FEngSetLanguageHash(GetDataObject(), hash);
}

void POLeaderBoard::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        int player = GetPlayerToEditForOptions();
        FEDatabase->GetPlayerSettings(player)->LeaderboardOn = !FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions())->LeaderboardOn;
    }
    Update(data);
}

void POLeaderBoard::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0xC93FBFB5);
    int player = GetPlayerToEditForOptions();
    unsigned int hash = FEDatabase->GetPlayerSettings(player)->LeaderboardOn ? LANGUAGE_COMMON_ON : LANGUAGE_COMMON_OFF;
    FEngSetLanguageHash(GetDataObject(), hash);
}

void COVibration::Act(const char* parent_pkg, unsigned int data) {
#ifdef EA_PLATFORM_PLAYSTATION2 // las dos PS2 solo conmutan el Rumble y hacen vibrar el mando; sin flechas
    if (data == 0x9120409E || data == 0xB5971BF1) {
        FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions())->Rumble =
            !FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions())->Rumble;
        if (FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions())->Rumble) {
            InputDevice* device = IOModule::GetIOModule().GetDevice(
                FEDatabase->GetPlayersJoystickPort(
#ifdef EA_BUILD_A124 // el prototipo hace vibrar el mando del jugador 0
                    0
#else
                    GetPlayerToEditForOptions()
#endif
                    ));
            if (device != nullptr) {
                device->StartVibration();
            }
        }
    }
    Update(data);
#else
    if (data == 0x9120409E || data == 0xB5971BF1) {
        if (data == 0x9120409E) {
            int player = GetPlayerToEditForOptions();
            FEDatabase->GetPlayerSettings(player)->Rumble = 0;
            FEngSetInvisible(parent_pkg, 0xBFF41BD9);
            FEngSetInvisible(parent_pkg, 0x7BCD6703);
            FEngSetInvisible(GetLeftImage());
            FEngSetVisible(parent_pkg, 0xBEE65E8C);
            FEngSetVisible(parent_pkg, 0x7C51B6D6);
            FEngSetVisible(GetRightImage());
        } else {
            int player = GetPlayerToEditForOptions();
            if (FEDatabase->GetPlayerSettings(player)->Rumble) {
                return;
            }
            player = GetPlayerToEditForOptions();
            FEDatabase->GetPlayerSettings(player)->Rumble = 1;
            FEngSetInvisible(parent_pkg, 0xBEE65E8C);
            FEngSetInvisible(parent_pkg, 0x7C51B6D6);
            FEngSetInvisible(GetRightImage());
            FEngSetVisible(parent_pkg, 0xBFF41BD9);
            FEngSetVisible(parent_pkg, 0x7BCD6703);
            FEngSetVisible(GetLeftImage());
        }
        int player = GetPlayerToEditForOptions();
        if (FEDatabase->GetPlayerSettings(player)->Rumble) {
            InputDevice* device = IOModule::GetIOModule().GetDevice(
                FEDatabase->GetPlayersJoystickPort(GetPlayerToEditForOptions()));
            if (device != nullptr) {
                device->StartVibration();
            }
        }
    }
    Update(data);
#endif
}

void COVibration::Draw() {
#ifdef EA_PLATFORM_PLAYSTATION2
    FEngSetLanguageHash(GetTitleObject(), 0x28A690A2);
#else
    FEngSetLanguageHash(GetTitleObject(), 0xDDDC5E1B);
#endif
    FEString* dataObj = GetDataObject();
    int player = GetPlayerToEditForOptions();
    unsigned int hash = LANGUAGE_COMMON_OFF;
    if (FEDatabase->GetPlayerSettings(player)->Rumble) {
        hash = LANGUAGE_COMMON_ON;
    }
    FEngSetLanguageHash(dataObj, hash);
}

void COVibration::UnsetFocus() {
    const u32 FEObj_ArrowMainLeft = 0xBFF41BD9;
    const u32 FEObj_LEFTARROW0 = 0x7BCD6703;
    const u32 FEObj_ArrowMainRight = 0xBEE65E8C;
    const u32 FEObj_RIGHTARROW0 = 0x7C51B6D6;
    FEngSetVisible("Pause_Controller.fng", FEObj_ArrowMainLeft);
    FEngSetVisible("Pause_Controller.fng", FEObj_LEFTARROW0);
    FEngSetVisible(GetLeftImage());
    FEngSetVisible("Pause_Controller.fng", FEObj_ArrowMainRight);
    FEngSetVisible("Pause_Controller.fng", FEObj_RIGHTARROW0);
    FEngSetVisible(GetRightImage());
    FEToggleWidget::UnsetFocus();
}

void COVibration::SetFocus(const char* parent_pkg) {
    int player = GetPlayerToEditForOptions();
    if (FEDatabase->GetPlayerSettings(player)->Rumble) {
        const u32 FEObj_ArrowMainRight = 0xBEE65E8C;
        const u32 FEObj_RIGHTARROW0 = 0x7C51B6D6;
        FEngSetInvisible("Pause_Controller.fng", FEObj_ArrowMainRight);
        FEngSetInvisible("Pause_Controller.fng", FEObj_RIGHTARROW0);
        FEngSetInvisible(GetRightImage());
    } else {
        const u32 FEObj_ArrowMainLeft = 0xBFF41BD9;
        const u32 FEObj_LEFTARROW0 = 0x7BCD6703;
        FEngSetInvisible("Pause_Controller.fng", FEObj_ArrowMainLeft);
        FEngSetInvisible("Pause_Controller.fng", FEObj_LEFTARROW0);
        FEngSetInvisible(GetLeftImage());
    }
    FEToggleWidget::SetFocus(parent_pkg);
}

void CODriveWithAnalog::Act(const char* parent_pkg, unsigned int data) {
    if (data == 0x9120409E || data == 0xB5971BF1) {
        int player = GetPlayerToEditForOptions();
        FEDatabase->GetPlayerSettings(player)->DriveWithAnalog =
            !FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions())->DriveWithAnalog;
    }
    Update(data);
}

void CODriveWithAnalog::Draw() {
    FEngSetLanguageHash(GetTitleObject(), 0x436668C6);
    FEString* dataObj = GetDataObject();
    int player = GetPlayerToEditForOptions();
    unsigned int hash = LANGUAGE_COMMON_NO;
    if (FEDatabase->GetPlayerSettings(player)->DriveWithAnalog) {
        hash = LANGUAGE_COMMON_YES;
    }
    FEngSetLanguageHash(dataObj, hash);
}

void COConfig::Act(const char* parent_pkg, unsigned int data) {
    int player = GetPlayerToEditForOptions();
    int config = static_cast<int>(FEDatabase->GetPlayerSettings(player)->Config);
    player = GetPlayerToEditForOptions();
    bool isAnalogSwiched = FEDatabase->GetPlayerSettings(player)->DriveWithAnalog;
#ifdef EA_BUILD_A124
    int max = IsJoystickTypeWheel(static_cast<JoystickPort>(UIOptionsController::PortToConfigure)) ? 1 : MAX_CONFIG;
    if (data == 0x9120409E) {
        config--;
        if (config < 0) {
            isAnalogSwiched = !isAnalogSwiched;
            config = max;
        }
    } else if (data == 0xB5971BF1) {
        config++;
        if (config > max) {
            isAnalogSwiched = !isAnalogSwiched;
            config = 0;
        }
    }
#else
    if (UIOptionsController::isWheelConfig) {
        config = 0;
        isAnalogSwiched = true;
    } else {
        int max = MAX_CONFIG;
        if (data == 0x9120409E) {
            config--;
            if (config < 0) {
                isAnalogSwiched = !isAnalogSwiched;
                config = max;
            }
        } else if (data == 0xB5971BF1) {
            config++;
            if (config > max) {
                isAnalogSwiched = !isAnalogSwiched;
                config = 0;
            }
        }
    }
#endif
    player = GetPlayerToEditForOptions();
    FEDatabase->GetPlayerSettings(player)->DriveWithAnalog = isAnalogSwiched;
    player = GetPlayerToEditForOptions();
    FEDatabase->GetPlayerSettings(player)->Config = static_cast<eControllerConfig>(config);
    cFEng::Get()->QueueGameMessage(0x92B703B5, parent_pkg, 0xFF);
    Update(data);
}

void COConfig::Draw() {
    int player = GetPlayerToEditForOptions();
    int config = static_cast<int>(FEDatabase->GetPlayerSettings(player)->Config);
    player = GetPlayerToEditForOptions();
    int val = config + 1;
    if (!FEDatabase->GetPlayerSettings(player)->DriveWithAnalog) {
        val = config + 6;
    }
    FEngSetLanguageHash(GetTitleObject(), 0xBA7306AA);
    FEPrintf(GetDataObject(), "%d", val);
}
