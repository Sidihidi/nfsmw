#include "uiOptionsScreen.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEImages.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEStrings.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/feDialogBox.hpp"

#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionWidgets.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/options/uiOptionsMain.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Generated/Events/EUnPause.hpp"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Sim/Simulation.h"
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"
#include "Speed/Indep/Src/Generated/LanguageHashes.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Misc/TestHooks/JuiceHooks/JuiceHooks.h"
#include "Speed/Indep/Src/Online/VoiceCore.hpp"
#endif

void FEngSetTextureHash(FEImage* image, unsigned int texture_hash);
int FEngSNPrintf(char* buffer, int buf_size, const char* fmt, ...);
// [fe3] const char* GetLocalizedString(unsigned int hash);

extern EAXSound* g_pEAXSound;

// [fe3] DialogInterface viene de MenuScreens/Common/DialogInterface.hpp

// [fe3] FEngSetTextureHash(pkg,hash,tex) ya es inline en FEngInterfaceFEImages.hpp

int UIOptionsScreen::PlayerToEdit = 0;
#ifdef EA_BUILD_A124
bool UIOptionsScreen::mTempAppearOffline = false;
#endif

int GetPlayerToEditForOptions() {
    return UIOptionsScreen::PlayerToEdit;
}

void SetPlayerToEditForOptions(int player) {
    UIOptionsScreen::PlayerToEdit = player;
}

UIOptionsScreen::UIOptionsScreen(ScreenConstructorData* sd)
    : UIWidgetMenu(sd) //
    , mCalledFromPauseMenu(sd->Arg != 0) //
    , NeedsColorCal(false) //
    , OriginalAudioSettings(nullptr) //
    , OriginalVideoSettings(nullptr) //
    , OriginalGameplaySettings(nullptr) //
    , OriginalPlayerSettings(nullptr)
#ifdef EA_BUILD_A124
    , OriginalOnlineSettings(nullptr)
#endif
{
#ifdef EA_BUILD_A124
    OriginalSpeakerState = VoiceCore::instance().GetSpeakerState();
    OriginalSpeakerVolume = VoiceCore::instance().GetSpeakerVolume();
    OriginalPlaybackVolume = VoiceCore::instance().GetPlaybackVolume();
#endif
    if (mCalledFromPauseMenu) {
        iMaxWidgetsOnScreen = 10;
    } else {
        iMaxWidgetsOnScreen = 9;
    }

#ifdef EA_BUILD_A124
    // La A124 rotula la cabecera por categoria y avisa a Juice de las opciones con las que se entra.
    FEString* subtitle = FEngFindString(GetPackageName(), 0x7326E44C);
    switch (FEDatabase->GetOptionsSettings()->CurrentCategory) {
    case OC_AUDIO:
    case OC_VIDEO:
    case OC_PC_ADV_DISPLAY:
    case OC_GAMEPLAY:
        FEngSetInvisible(subtitle);
        break;
    case OC_PLAYER:
        FEngSetLanguageHash(subtitle, PlayerToEdit == 0 ? LANGUAGE_COMMON_PLAYER_1 : LANGUAGE_COMMON_PLAYER_2);
        break;
    }
    JuiceStatsDB::Instance()->SetOptions(*FEDatabase->GetOptionsSettings());
#else
    if (FEDatabase->GetOptionsSettings()->CurrentCategory == OC_PLAYER &&
        Sim::GetUserMode() == Sim::USER_SPLIT_SCREEN) {
        cFEng::Get()->QueuePackageMessage(0x7DB7B6D7, GetPackageName(), 0);
        const char* pkg = GetPackageName();
        FEngSetLanguageHash(pkg, 0x53BF826D,
                            GetPlayerToEditForOptions() == 0 ? LANGUAGE_COMMON_PLAYER_1 : LANGUAGE_COMMON_PLAYER_2);
    }
#endif

    Setup();
}

UIOptionsScreen::~UIOptionsScreen() {
#ifdef EA_BUILD_A124
    JuiceStatsDB::Instance()->CompareOptions(FEDatabase->GetOptionsSettings());
#endif
    delete OriginalAudioSettings;
    delete OriginalVideoSettings;
    delete OriginalGameplaySettings;
    delete OriginalPlayerSettings;
#ifdef EA_BUILD_A124
    delete OriginalOnlineSettings;
#endif
}

void UIOptionsScreen::NotificationMessage(u32 msg, FEObject* pobj, u32 param1,
                                          u32 param2) {
    UIWidgetMenu::NotificationMessage(msg, pobj, param1, param2);

    switch (msg) {
#ifdef EA_BUILD_A124
    // La A124 refresca el VOIP al cambiar de opcion online; es su primer caso (el codigo va delante del de
    // 0x911AB364).
    case 0xC98356BA:
        if (FEDatabase->GetOptionsSettings()->CurrentCategory == OC_ONLINE) {
            UpdateOnlineVoipOptions();
        }
        break;
#endif
    case 0x911AB364:
        if (OptionsDidNotChange()) {
            cFEng::Get()->QueuePackageMessage(0x587C018B, GetPackageName(), 0);
        } else {
            const char* pkg_name = GetPackageName();
            const char* prompt = GetLocalizedString(0xE9CB802F);
            DialogInterface::ShowTwoButtons(pkg_name,
                                            mCalledFromPauseMenu ? "InGameDialog.fng" : "Dialog.fng",
                                            static_cast<eDialogTitle>(1), LANGUAGE_COMMON_YES, LANGUAGE_COMMON_NO,
                                            0x775DBA97, 0x34DC1BCF, 0x34DC1BCF,
                                            static_cast<eDialogFirstButtons>(1), prompt);
        }
        break;
    case 0x775DBA97:
        RestoreOriginals();
        MemoryCard::GetInstance()->SetCardRemovedWithAutoSaveEnabled(false);
        cFEng::Get()->QueuePackageMessage(0x587C018B, GetPackageName(), 0);
        break;
    case 0xC519BFC4: {
        const char* pkg_name = GetPackageName();
        const char* prompt = GetLocalizedString(0x8AEF5AE8);
        DialogInterface::ShowTwoButtons(pkg_name,
                                        mCalledFromPauseMenu ? "InGameDialog.fng" : "Dialog.fng",
                                        static_cast<eDialogTitle>(1), LANGUAGE_COMMON_YES, LANGUAGE_COMMON_NO,
                                        0xD05FC3A3, 0x34DC1BCF, 0x34DC1BCF,
                                        static_cast<eDialogFirstButtons>(1), prompt);
        break;
    }
    case 0xD9FEEC59:
    case 0x5073EF13:
        if (FEDatabase->GetOptionsSettings()->CurrentCategory == OC_PLAYER) {
            cFEng* fe = cFEng::Get();
            unsigned int snd = (msg == 0x5073EF13) ? 0x6B283007 : 0xF4B32D4D;
            fe->QueueSoundMessage(snd, GetPackageName());
            if (!OptionsDidNotChange()) {
                char buf[128];
                const char* fmt = GetLocalizedString(0xBA463431);
                FEngSNPrintf(buf, 128, fmt, GetPlayerToEditForOptions() + 1);
                DialogInterface::ShowTwoButtons(GetPackageName(),
                                                mCalledFromPauseMenu ? "InGameDialog.fng" : "Dialog.fng",
                                                static_cast<eDialogTitle>(1), LANGUAGE_COMMON_YES, LANGUAGE_COMMON_NO,
                                                0x9A5AD46D, 0xA2A07AC4, 0x34DC1BCF,
                                                static_cast<eDialogFirstButtons>(1), buf);
            } else {
                cFEng::Get()->QueueGameMessage(0x9A5AD46D, 0, 0xFF);
            }
        }
        break;
    case 0xA2A07AC4:
        TogglePlayer(true);
        break;
    case 0x9A5AD46D:
        TogglePlayer(false);
        break;
    case 0xD05FC3A3:
        {
            OptionsSettings* options_settings = FEDatabase->GetOptionsSettings();
            // La A124 entra en la tarjeta sin mirar la categoria.
            if (!FEDatabase->GetGameplaySettings()->AutoSaveOn
#ifndef EA_BUILD_A124
                && options_settings->CurrentCategory == OC_GAMEPLAY
#endif
            ) {
                MemcardEnter(GetPackageName(), GetPackageName(), 0xA1, 0, 0, 0, 0);
            }
        }
        RestoreDefaults();
        break;
    case 0xE1FDE1D1: {
        bool dirty = false;
        if (FEDatabase->IsOptionsDirty() || !OptionsDidNotChange()) {
            dirty = true;
        }
        FEDatabase->SetOptionsDirty(dirty);

        if (mCalledFromPauseMenu) {
            cFEng::Get()->QueuePackageSwitch("Pause_Main.fng", 1, 0, false);
        } else if (FEDatabase->IsOnlineMode()) {
            cFEng::Get()->QueuePackageSwitch("OL_MAIN.fng", 0, 0, false);
        } else {
            cFEng::Get()->QueuePackageSwitch("MainMenu_Sub.fng", 0, 0, false);
        }

        OptionsSettings* options_settings = FEDatabase->GetOptionsSettings();
        if (options_settings->CurrentCategory != OC_AUDIO) {
            return;
        }
        g_pEAXSound->UpdateVolumes(FEDatabase->GetAudioSettings(), -1.0f);
        break;
    }
    case 0xB5AF2461:
        new EUnPause();
        break;
    case 0x7E998E5E:
        if (FEDatabase->GetOptionsSettings()->CurrentCategory == OC_GAMEPLAY) {
            ClearWidgets();
            SetupGameplay();
            SetInitialOption(0);
        }
#ifndef EA_BUILD_A124 // la A124 no redibuja las demas categorias
        else {
            for (int i = 0; i < Options.CountElements(); i++) {
                Options.GetNode(i)->Draw();
            }
        }
#endif
        break;
    case 0x34DC1BCF:
    case 0x72619778:
    case 0x406415E3:
    case 0xC519BFC3:
#ifndef EA_BUILD_A124
    case 0xC98356BA:
#endif
        break;
    }
}

#ifdef EA_BUILD_A124
void UIOptionsScreen::UpdateOnlineVoipOptions() {
    if (!VoiceCore::instance().IsHeadsetConnected()) {
        if (!volumeoption->IsEnabled()) {
            volumeoption->Enable();
        }
    } else if (!VoiceCore::instance().GetSpeakerState()) {
        if (volumeoption->IsEnabled()) {
            volumeoption->Disable();
        }
    }
    volumeoption->Draw();
}
#endif

void UIOptionsScreen::Setup() {
    ClearWidgets();
    NeedsColorCal = false;
    mInitialAutoSaveValue = FEDatabase->GetGameplaySettings()->AutoSaveOn;

    FEngSetInvisible(GetPackageName(), 0xE54C30BE);
    FEngSetInvisible(GetPackageName(), 0x8E1BEA84);
    FEngSetInvisible(GetPackageName(), 0x608BB8C8);
    FEngSetInvisible(GetPackageName(), 0x444969FD);
    FEngSetInvisible(GetPackageName(), 0x444969FE);

    eOptionsCategory curCat = FEDatabase->GetOptionsSettings()->CurrentCategory;
    switch (curCat) {
    case OC_AUDIO:
        SetupAudio();
        break;
    case OC_VIDEO:
        SetupVideo();
        break;
    case OC_GAMEPLAY:
        SetupGameplay();
        break;
    case OC_PLAYER:
        SetupPlayer();
        FEngSetVisible(GetPackageName(), 0x444969FD);
        FEngSetVisible(GetPackageName(), 0x444969FE);
        break;
    case OC_ONLINE:
        SetupOnline();
        break;
    }

    SetInitialOption(0);
}

void UIOptionsScreen::SetupAudio() {
    FEngSetTextureHash(GetPackageName(), 0x8007B4C, 0xF37AF144);

    if (mCalledFromPauseMenu) {
        FEngSetLanguageHash(GetPackageName(), 0x42ADB44C, 0xB1426DFA);
    } else {
        FEngSetLanguageHash(GetPackageName(), 0x42ADB44C, 0x3932C2E4);
    }

    AddSliderOption(new ("AOSFXMasterVol", 0) AOSFXMasterVol(true), true);
    AddSliderOption(new ("AOCarVol", 0) AOCarVol(true), true);
    AddSliderOption(new ("AOSpeechVol", 0) AOSpeechVol(true), true);
    AddSliderOption(new ("AOFEMusicVol", 0) AOFEMusicVol(true), true);
    AddSliderOption(new ("AOIGMusicVol", 0) AOIGMusicVol(true), true);
    AddToggleOption(new ("AOInteractiveMusicMode", 0) AOInteractiveMusicMode(true), true);
    AddToggleOption(new ("AOEATraxMusicMode", 0) AOEATraxMusicMode(true), true);

    if (TheGameFlowManager.IsInFrontend()) {
        AddToggleOption(new ("AOAudioMode", 0) AOAudioMode(true), true);
    }

    AudioSettings* temp = new ("AudioSettings", 0) AudioSettings();
    OriginalAudioSettings = temp;
    *OriginalAudioSettings = *FEDatabase->GetAudioSettings();
}

void UIOptionsScreen::SetupVideo() {
    FEngSetTextureHash(GetPackageName(), 0x8007B4C, 0x8A006328);

    if (mCalledFromPauseMenu) {
        FEngSetLanguageHash(GetPackageName(), 0x42ADB44C, 0xD94EA03F);
    } else {
        FEngSetLanguageHash(GetPackageName(), 0x42ADB44C, 0x48478029);
    }

    AddToggleOption(new ("VOWideScreen", 0) VOWideScreen(true), true);
#ifdef EA_PLATFORM_PLAYSTATION2
    AddSliderOption(new ("VOScreenPosHoriz", 0) VOScreenPosHoriz(true), true);
    AddSliderOption(new ("VOScreenPosVert", 0) VOScreenPosVert(true), true);
#endif

    VideoSettings* temp = new ("VideoSettings", 0) VideoSettings();
    OriginalVideoSettings = temp;
    *OriginalVideoSettings = *FEDatabase->GetVideoSettings();

    FEngSetScript(GetPackageName(), 0xAD6B204F, FEHASH_APPEAR, true);
}

void UIOptionsScreen::SetupGameplay() {
    FEngSetTextureHash(GetPackageName(), 0x8007B4C, 0x4DF98FB2);

    if (mCalledFromPauseMenu) {
        FEngSetLanguageHash(GetPackageName(), 0x42ADB44C, 0x3936D9F8);
    } else {
        FEngSetLanguageHash(GetPackageName(), 0x42ADB44C, 0x01CCE8C2);
    }

    bool split = ShouldShowAutoSave();
    if (split) {
        AddToggleOption(new ("GOAutoSave", 0) GOAutoSave(true), true);
    }

    if (Sim::GetUserMode() != Sim::USER_SPLIT_SCREEN) {
        AddToggleOption(new ("GOJumpCams", 0) GOJumpCams(true), true);
    }

    AddToggleOption(new ("GODamage", 0) GODamage(true), true);
    AddToggleOption(new ("GORearview", 0) GORearview(true), true);
    AddToggleOption(new ("GOSpeedoUnits", 0) GOSpeedoUnits(true), true);

#ifndef EA_BUILD_A124 // la A124 pone los minimapas tambien en pantalla partida
    bool is_split = Sim::GetUserMode() == Sim::USER_SPLIT_SCREEN;
    if (!mCalledFromPauseMenu) is_split = false;
    if (!is_split) {
#endif
        if (!FEDatabase->IsOnlineMode() && !FEDatabase->IsLANMode()) {
            AddToggleOption(new ("GOExploringMiniMap", 0) GOExploringMiniMap(true), true);
        }
        AddToggleOption(new ("GORacingMiniMap", 0) GORacingMiniMap(true), true);
#ifndef EA_BUILD_A124
    }
#endif

    if (OriginalGameplaySettings == nullptr) {
        GameplaySettings* temp = new ("GameplaySettings", 0) GameplaySettings();
            OriginalGameplaySettings = temp;
        *OriginalGameplaySettings = *FEDatabase->GetGameplaySettings();
    }
}

void UIOptionsScreen::SetupPlayer() {
    FEngSetTextureHash(GetPackageName(), 0x8007B4C, 0xD708EFEF);

    if (mCalledFromPauseMenu) {
        FEngSetLanguageHash(GetPackageName(), 0x42ADB44C, 0xD9DC2F12);
    } else {
        FEngSetLanguageHash(GetPackageName(), 0x42ADB44C, 0xC055165F);
    }

#ifndef EA_BUILD_A124 // la A124 no lanza el script de aparicion
    FEngSetScript(GetPackageName(), 0x8A41F5B9, FEHASH_APPEAR, true);
#endif

    FEngSetLanguageHash(GetPackageName(), 0x53BF826D,
                        GetPlayerToEditForOptions() == 0 ? LANGUAGE_COMMON_PLAYER_1 : LANGUAGE_COMMON_PLAYER_2);

    if (!GRaceStatus::Exists() || GRaceStatus::Get().GetRaceType() != GRace::kRaceType_Drag) {
        AddToggleOption(new ("POTransmission", 0) POTransmission(true), true);
    }
#ifdef EA_BUILD_A124 // solo la A124 ofrece el aviso de cambio de marcha
    AddToggleOption(new ("POTransPrompt", 0) POTransPrompt(true), true);
#endif

    AddToggleOption(new ("PODriveCam", 0) PODriveCam(true), true);
    AddToggleOption(new ("POGauges", 0) POGauges(true), true);
    AddToggleOption(new ("POPosition", 0) POPosition(true), true);
    AddToggleOption(new ("POSplitTime", 0) POSplitTime(true), true);
    AddToggleOption(new ("POScore", 0) POScore(true), true);

    if (!GRaceStatus::Exists() || (!GRaceStatus::IsDragRace() && !GRaceStatus::IsTollboothRace())) {
        AddToggleOption(new ("POLeaderBoard", 0) POLeaderBoard(true), true);
    }

    PlayerSettings* temp = new ("PlayerSettings", 0) PlayerSettings();
    OriginalPlayerSettings = temp;
    *OriginalPlayerSettings =
        *FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions());
}

void UIOptionsScreen::SetupOnline() {
    if (mCalledFromPauseMenu) {
        FEngSetLanguageHash(GetPackageName(), 0x42ADB44C, 0x966C856D);
    } else {
        FEngSetLanguageHash(GetPackageName(), 0x42ADB44C, 0xE463B5F7);
    }
#ifdef EA_BUILD_A124
    volumeoption = new ("OOVoiceVolume", 0) OOVoiceVolume(true);
    AddSliderOption(volumeoption, true);
    AddToggleOption(new ("OOBadConnectionWarning", 0) OOBadConnectionWarning(true), true);
    FEngSetLanguageHash(GetPackageName(), 0x42ADB44C, 0x22B1C2E6);
    OriginalOnlineSettings = new ("OnlineSettings", 0) cOnlineSettings();
    *OriginalOnlineSettings = *FEDatabase->GetOnlineSettings();
#endif
}

void UIOptionsScreen::RestoreDefaults() {
    bool bOldAutoSaveVal;

    eOptionsCategory curCat = FEDatabase->GetOptionsSettings()->CurrentCategory;
    switch (curCat) {
    case OC_AUDIO:
        FEDatabase->GetAudioSettings()->Default();
        break;
    case OC_VIDEO:
        FEDatabase->GetVideoSettings()->Default();
        break;
    case OC_GAMEPLAY:
        bOldAutoSaveVal = FEDatabase->GetGameplaySettings()->AutoSaveOn;
        FEDatabase->GetGameplaySettings()->Default();
        if (!ShouldShowAutoSave()) {
            FEDatabase->GetGameplaySettings()->AutoSaveOn = bOldAutoSaveVal;
        }
        break;
    case OC_PLAYER:
        FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions())->DefaultFromOptionsScreen();
        break;
#ifdef EA_BUILD_A124
    case OC_ONLINE:
        FEDatabase->GetOnlineSettings()->RestoreDefaultSettings();
        break;
#endif
    }

    for (int i = 0; i < Options.CountElements(); i++) {
        Options.GetNode(i)->Draw();
    }
}

bool UIOptionsScreen::OptionsDidNotChange() {
    eOptionsCategory curCat = FEDatabase->GetOptionsSettings()->CurrentCategory;
    switch (curCat) {
    case OC_AUDIO:
        return *FEDatabase->GetAudioSettings() == *OriginalAudioSettings;
    case OC_VIDEO:
        return *FEDatabase->GetVideoSettings() == *OriginalVideoSettings;
    case OC_GAMEPLAY:
        return *FEDatabase->GetGameplaySettings() == *OriginalGameplaySettings;
    case OC_PLAYER:
        return *FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions()) ==
               *OriginalPlayerSettings;
#ifdef EA_BUILD_A124
    case OC_ONLINE:
        return *FEDatabase->GetOnlineSettings() == *OriginalOnlineSettings &&
               mTempAppearOffline == FEDatabase->GetOnlineSettings()->AppearOffline;
#endif
    default:
        return false;
    }
}

void UIOptionsScreen::RestoreOriginals() {
    eOptionsCategory curCat = FEDatabase->GetOptionsSettings()->CurrentCategory;
    switch (curCat) {
    case OC_AUDIO:
        *FEDatabase->GetAudioSettings() = *OriginalAudioSettings;
        break;
    case OC_VIDEO:
        *FEDatabase->GetVideoSettings() = *OriginalVideoSettings;
        break;
    case OC_GAMEPLAY:
        *FEDatabase->GetGameplaySettings() = *OriginalGameplaySettings;
        break;
    case OC_PLAYER:
        *FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions()) = *OriginalPlayerSettings;
        break;
#ifdef EA_BUILD_A124
    case OC_ONLINE:
        *FEDatabase->GetOnlineSettings() = *OriginalOnlineSettings;
        // Una sola lectura de VoiceCore::mInstance para las tres llamadas: una referencia local (GCC no da stab
        // a las referencias locales en registro, y la A124 no trae ninguno aqui).
        VoiceCore &voice = VoiceCore::instance();
        voice.SetSpeakerState(OriginalSpeakerState);
        voice.SetSpeakerVolume(OriginalSpeakerVolume);
        voice.SetPlaybackVolume(OriginalPlaybackVolume);
        break;
#endif
    }
}

void UIOptionsScreen::TogglePlayer(bool revert_changes) {
    if (revert_changes) {
        RestoreOriginals();
    }

    SetPlayerToEditForOptions(GetPlayerToEditForOptions() == 0);

    if (FEDatabase->GetOptionsSettings()->CurrentCategory == OC_PLAYER) {
        *OriginalPlayerSettings =
            *FEDatabase->GetPlayerSettings(GetPlayerToEditForOptions());

        const char* pkg = GetPackageName();
        unsigned int lang;
        if (GetPlayerToEditForOptions() == 0) {
            lang = LANGUAGE_COMMON_PLAYER_1;
        } else {
            lang = LANGUAGE_COMMON_PLAYER_2;
        }
        FEngSetLanguageHash(pkg, 0x53BF826D, lang);
    }

    for (int i = 0; i < Options.CountElements(); i++) {
        Options.GetNode(i)->Draw();
    }
}

bool UIOptionsScreen::ShouldShowAutoSave() {
    bool result = false;
    if (!GRaceStatus::Exists() ||
        GRaceStatus::Get().GetRaceContext() == GRace::kRaceContext_Career
#ifndef EA_BUILD_A124 // la A124 no autoguarda en las series de desafios
        || (GRaceStatus::Get().GetRaceParameters() &&
         GRaceStatus::Get().GetRaceParameters()->GetIsChallengeSeriesRace())
#endif
        ) {
        if (IsMemcardEnabled && IsAutoSaveEnabled && FEDatabase->bProfileLoaded &&
            !FEDatabase->IsOnlineMode()) {
            result = true;
        }
    }
    return result;
}
