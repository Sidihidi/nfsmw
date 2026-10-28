#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/FEng/FEngStandard.h"
#include "Speed/Indep/Src/Frontend/Careers/UnlockSystem.hpp"
#include "Speed/Indep/Src/Frontend/Database/RaceDB.hpp"
#include "Speed/Indep/Src/Frontend/FECarViewer.hpp"
#include "Speed/Indep/Src/Frontend/FEJoyInput.hpp"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/MemCard/uiMemcardInterface.hpp"
#include "Speed/Indep/Src/Gameplay/GRace.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Gameplay/GManager.h"
#include "Speed/Indep/Src/Gameplay/GMilestone.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/audiosystem.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/controller_hash.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/frontend.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/music.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/pvehicle.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Interfaces/Simables/IAI.h"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Misc/EasterEggs.hpp"
#include "Speed/Indep/Src/Misc/MD5.hpp"
#include "Speed/Indep/Tools/AttribSys/Runtime/AttribSys.h"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Src/EAXSound/CARSFX/SFXObj_Pathfinder.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/RaceStarter.hpp"
#include "Speed/Indep/Src/Frontend/FEManager.hpp"
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.hpp"
#include "Speed/Indep/Src/Generated/LanguageHashes.hpp"
#ifdef EA_BUILD_A124
#include "Speed/Indep/Src/Speech/MiscSpeech.h"
#endif

int g_MaxSongs = (int)0xFFFFFFFF;

extern Sound::SongInfoList Songs;

// .rodata:0x803EA794 (0x28 B) y .rodata:0x803EA7BC (0x28 B), las dos
// scope:local -> `static`, y en .rodata -> `const`.  Los diez hashes salen del
// ELF original y coinciden uno a uno con Attrib::Hash::controller (DriveConfigs
// primero, que es el orden de direcciones del objetivo).
static const Attrib::Key DriveConfigs[5][2] = {
    {Attrib::Hash::controller::key_drive, Attrib::Hash::controller::key_drive_analog},
    {Attrib::Hash::controller::key_drive2, Attrib::Hash::controller::key_drive2_analog},
    {Attrib::Hash::controller::key_drive3, Attrib::Hash::controller::key_drive3_analog},
    {Attrib::Hash::controller::key_drive4, Attrib::Hash::controller::key_drive4_analog},
    {Attrib::Hash::controller::key_drive5, Attrib::Hash::controller::key_drive5_analog},
};

static const Attrib::Key HudConfigs[5][2] = {
    {Attrib::Hash::controller::key_hud_analog, Attrib::Hash::controller::key_hud},
    {Attrib::Hash::controller::key_hud_analog, Attrib::Hash::controller::key_hud},
    {Attrib::Hash::controller::key_hud_analog, Attrib::Hash::controller::key_hud},
    {Attrib::Hash::controller::key_hud_analog, Attrib::Hash::controller::key_hud},
    {Attrib::Hash::controller::key_hud_analog, Attrib::Hash::controller::key_hud},
};

cFrontendDatabase *FEDatabase = nullptr;
cFrontendDatabase *gFEDatabase = FEDatabase;

char *SaveSomeData(void *save_to, void *save_from, int bytes, void *maxptr) {
    if (reinterpret_cast<uint32>(save_to) + bytes > reinterpret_cast<uint32>(maxptr)) {
        return static_cast<char *>(save_to);
    }
    bMemCpy(save_to, save_from, bytes);
    return static_cast<char *>(save_to) + bytes;
}

char *LoadSomeData(void *load_to, void *load_from, int bytes, void *maxptr) {
    if (reinterpret_cast<uint32>(load_from) + bytes <= reinterpret_cast<uint32>(maxptr)) {
        bMemCpy(load_to, load_from, bytes);
    }
    return static_cast<char *>(load_from) + bytes;
}

FEKeyboardSettings::FEKeyboardSettings() {
    AcceptCallbackHash = 0xAE83B9DB;
    DeclineCallbackHash = 0x6A97B51F;
    DefaultTextHash = 0;
    MaxTextLength = 64;
    Mode = 0;
    Buffer[0] = 0;
}

void PlayerSettings::Default() {
    GaugesOn = 1;
    PositionOn = 1;
    LapInfoOn = 1;
    ScoreOn = 1;
    LeaderboardOn = 1;
    TransmissionPromptOn = 1;




    // PS2 arranca sin vibracion (sw zero en las dos PS2; GameCube y PC la ponen a 1).
#ifdef EA_PLATFORM_PLAYSTATION2
    Rumble = 0;
#else
    Rumble = 1;
#endif






    DriveWithAnalog = 1;
    Config = CC_CONFIG_1;

    SplitTimeType = 0;
    CurCam = PSC_DEFAULT;
    Transmission = 0;
    Handling = 1;
}

bool PlayerSettings::operator==(const PlayerSettings &rhs) const {
    return bMemCmp(this, &rhs, sizeof(PlayerSettings)) == 0;
}

void PlayerSettings::DefaultFromOptionsScreen() {
    bool savedDriveWithAnalog = DriveWithAnalog;
    eControllerConfig savedConfig = Config;
    bool savedRumble = Rumble;
    Default();
    DriveWithAnalog = savedDriveWithAnalog;
    Config = savedConfig;
    Rumble = savedRumble;
}

Attrib::Key PlayerSettings::GetControllerAttribs(eControllerAttribs type
#ifndef EA_BUILD_A124
                                                 ,
                                                 bool wheel_connected
#endif
) const {
    int analog = 0;
    if (DriveWithAnalog) {
        analog = 1;
    }
#ifndef EA_BUILD_A124
    // the local config only exists for the wheel override: the prototype indexes by Config directly
    int config = Config;

    if (wheel_connected) {

        config = 0;
        analog = 1;
    }
#endif

    switch (type) {
        case CA_DRIVING:
            return DriveConfigs[
#ifndef EA_BUILD_A124
                config
#else
                Config
#endif
            ][analog];

        case CA_HUD:
            return HudConfigs[
#ifndef EA_BUILD_A124
                config
#else
                Config
#endif
            ][analog];
    }

    return 0;
}

void PlayerSettings::ScrollDriveCam(int dir) {
    int cur_cam = CurCam;

    if (dir == 1) {
        do {
            if (++cur_cam > 6) {
                cur_cam = 0;
            }
        } while (!IsPlayerCameraSelectable(GetPOVTypeFromPlayerCamera(static_cast<ePlayerSettingsCameras>(cur_cam))));
    } else if (dir == -1) {
        do {
            if (--cur_cam < 0) {
                cur_cam = 6;
            }
        } while (!IsPlayerCameraSelectable(GetPOVTypeFromPlayerCamera(static_cast<ePlayerSettingsCameras>(cur_cam))));
    }

    CurCam = static_cast<ePlayerSettingsCameras>(cur_cam);
}

void GameplaySettings::Default() {
    AutoSaveOn = 1;
    RearviewOn = 1;
    Damage = 1;

    RacingMiniMapMode = 1;
    ExploringMiniMapMode = 0;

    MapItems = static_cast<unsigned int>(-1);
    LastMapZoom = 1;
    LastPursuitMapZoom = 2;
    LastMapView = 0;

    JumpCam = 1;
    HighlightCam = 127.5f;
    eLanguages lang = GetCurrentLanguage();
    if (lang) {
        SpeedoUnits = 1;
    } else {
        SpeedoUnits = 0;
    }
}

bool GameplaySettings::IsMapItemEnabled(eWorldMapItemType type) {
    if ((MapItems & type) != 0) {
        return true;
    }
    return false;
}

void GameplaySettings::SetMapItem(eWorldMapItemType type, bool enabled) {
    if (enabled) {
        MapItems = MapItems | type;
        return;
    }
    MapItems = MapItems & ~type;
}

bool GameplaySettings::operator==(const GameplaySettings &rhs) const {
    return bMemCmp(this, &rhs, sizeof(GameplaySettings)) == 0;
}

void VideoSettings::Default() {
#ifdef EA_PLATFORM_GAMECUBE
    FEScale = 0.84f;
#else
    FEScale = 1.0f;
#endif
    ScreenOffsetX = 0.0f;
    ScreenOffsetY = 0.0f;
    WideScreen = false;
#ifdef EA_PLATFORM_XENON
    X360Video10 = 0.5f; X360Video14 = 0.51020408f; // NFS.exe 0x822E9BE0 (WideScreen lo toma de un global del XDK)
#endif
}
bool VideoSettings::operator==(const VideoSettings &rhs) const {
    return bMemCmp(this, &rhs, sizeof(VideoSettings)) == 0; // X360: 0x18 (NFS.exe 0x822E9B08)
}
void AudioSettings::Default() {
    MasterVol = 1.0f;
    SpeechVol = 1.0f;
    FEMusicVol = 0.8f;
    IGMusicVol = 0.8f;
    SoundEffectsVol = 1.0f;
    EngineVol = 1.0f;
    CarVol = 1.0f;
    AmbientVol = 1.0f;
    SpeedVol = 1.0f;
#ifdef EA_PLATFORM_WIN32
    AudioMode = 1;
#else
    AudioMode = 2;
#endif
#ifndef EA_BUILD_A124
    AudioMode = g_pEAXSound->GetDefaultPlatformAudioMode();
#endif
    InteractiveMusicMode = 1;
    EATraxMode = 1;
    PlayState = 0;
}
bool AudioSettings::operator==(const AudioSettings &rhs) const {
#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_XENON) // X360: 0x34 B (NFS.exe 0x822E9BA0)
    return bMemCmp(this, &rhs, sizeof(AudioSettings)) == 0;
#else
    if (MasterVol != rhs.MasterVol)
        return false;
    if (SpeechVol != rhs.SpeechVol)
        return false;
    if (FEMusicVol != rhs.FEMusicVol)
        return false;
    if (IGMusicVol != rhs.IGMusicVol)
        return false;
    if (SoundEffectsVol != rhs.SoundEffectsVol)
        return false;
    if (EngineVol != rhs.EngineVol)
        return false;
    if (CarVol != rhs.CarVol)
        return false;
    if (AmbientVol != rhs.AmbientVol)
        return false;
    if (SpeedVol != rhs.SpeedVol)
        return false;
    if (InteractiveMusicMode != rhs.InteractiveMusicMode)
        return false;
    if (EATraxMode != rhs.EATraxMode)
        return false;
    if (PlayState != rhs.PlayState)
        return false;
    if (AudioMode != rhs.AudioMode && AudioMode != AUDIO_MODE_MONO && AudioMode != AUDIO_MODE_STEREO)
        return false;
    return true;
#endif
}

void OptionsSettings::Default() {
    CurrentCategory = static_cast<eOptionsCategory>(0);
    TheVideoSettings.Default();
    TheAudioSettings.Default();
    TheGameplaySettings.Default();
    ThePlayerSettings[0].Default();
    ThePlayerSettings[1].Default();
}

void CareerSettings::Default() {
    CurrentBin = 0x10;
    CurrentCar = 0;
    SpecialFlags = 0;
    AdaptiveDifficulty = 0;
    CurrentCash = 0;
    for (int i = 0; i <
#ifdef EA_BUILD_A124
                    125
#else
                    150
#endif
         ;
         i++) {
        SMSMessages[i].SetHandle(static_cast<FESMSHandle>(i));
        if (!DoesStringExist(SMSMessages[i].GetMsgHash())) {
            SMSMessages[i].SetHandle(
#ifdef EA_BUILD_A124
                0xFFFF
#else
                0xFF
#endif
            );
        }
        SMSMessages[i].ClearFlags();
    }
#ifdef EA_BUILD_A124 // el prototipo borra aqui las puntuaciones, el juego y los desbloqueos
    if (GRaceDatabase::Exists()) {
        GRaceDatabase::Get().ClearRaceScores();
    }
    if (GManager::Exists()) {
        GManager::Get().ResetAllGameplayData();
    }
    DefaultUnlockData();
#else
    SMSSortOrder = 0;
#endif
}

SMSMessage *CareerSettings::GetSMSMessage(uint32 index) {
    if (index >=
#ifdef EA_BUILD_A124
        0x7D
#else
        0x96
#endif
    ) {
        return nullptr;
    }
    return &SMSMessages[index];
}

#ifndef EA_BUILD_A124
unsigned short CareerSettings::GetSMSSortOrder() {
    SMSSortOrder = SMSSortOrder + 1;
    return SMSSortOrder;
}
#endif

bool SMSMessage::IsVoice() {
#ifdef EA_BUILD_A124 // el prototipo pregunta al habla si hay llamada para el mensaje
    if (IsSpeechEnabled && IsSoundEnabled) {
        return MiscSpeech::IsSMSValid(Handle);
    }
    return false;
#else
    switch (Handle) {
        case 0x6E: case 0x6F:
        case 0x78: case 0x79:
        case 0x7A: case 0x7B:
        case 0x7C: case 0x7D:
        case 0x7E: case 0x7F:
        case 0x80: case 0x81:
        case 0x82: case 0x84:
        case 0x85: case 0x86:
        case 0x87: case 0x88: case 0x89:
            return false;
        default:
            return true;
    }
#endif
}

void CareerSettings::SpendCash(int amount) {
    if (static_cast<unsigned int>(amount) > CurrentCash) {
        CurrentCash = 0;
        return;
    }
    CurrentCash = CurrentCash - amount;
}

void CareerSettings::StartNewCareer(bool bEnterGameplay) {
#ifdef EA_BUILD_A124 // el prototipo borra aqui los marcadores, las puntuaciones y el juego
    TheFEMarkerManager.Default();
    if (GRaceDatabase::Exists()) {
        GRaceDatabase::Get().ClearRaceScores();
    }
    if (GManager::Exists()) {
        GManager::Get().ResetAllGameplayData();
    }
#endif
    Default();
    CurrentCar = FEDatabase->GetDefaultCar();
    GenerateCaseFileName();
    SpecialFlags |= 1;

    if (SkipCareerIntro && SkipDDayRaces) {
        CurrentBin = 0xF;
#ifdef EA_BUILD_A124 // el prototipo no simula el dia D y crea los dos primeros coches de la lista
        FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
        FECarRecord *rec = stable->CreateNewCareerCar(1);
        CurrentCar = rec->Handle;
        rec = stable->CreateNewCareerCar(2);
        CurrentCar = rec->Handle;
#else
        GRaceDatabase::Get().SimulateDDayComplete();
        FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
        FECarRecord *rec = stable->CreateNewCareerCar(0x2CF385B2);
        CurrentCar = rec->Handle;
        rec = stable->CreateNewCareerCar(0x03A94520);
        CurrentCar = rec->Handle;
#endif
    }

#ifndef EA_BUILD_A124
    TryAwardDemoMarker();
#endif

    if (!bEnterGameplay) {
        return;
    }

    FEDatabase->ResetGameMode();
    FEDatabase->SetGameMode(eFE_GAME_MODE_CAREER);

    if (SkipDDayRaces) {
        FEManager::Get()->SetGarageType(GARAGETYPE_CAREER_SAFEHOUSE);
        FEDatabase->ClearGameMode(eFE_GAME_MODE_CAREER_MANAGER);
        if (SkipCareerIntro) {
            CurrentBin = 0xF;
        }
    } else {
#ifndef EA_BUILD_A124 // el prototipo no empieza la carrera con el M3 GTR
        unsigned int hash = FEHashUpper("M3GTRCAREERSTART");
        FEDatabase->GetCareerSettings()->SetCurrentCar(hash);
        FEDatabase->GetQuickRaceSettings(GRace::kRaceType_NumTypes)->SetSelectedCar(hash, 0);
#endif
        gMemcardSetup.mOp = 0;
        gMemcardSetup.mMemScreen = nullptr;
        gMemcardSetup.mToScreen = nullptr;
        gMemcardSetup.mFromScreen = nullptr;
        gMemcardSetup.mTermFunc = nullptr;
        gMemcardSetup.mTermFuncParam = nullptr;
        gMemcardSetup.mLastMessage = 0;
        gMemcardSetup.mSuccessMsg = 0;
        gMemcardSetup.mFailedMsg = 0;
        gMemcardSetup.mInBootFlow = false;
        gMemcardSetup.mPreviousCommand = 0;
        gMemcardSetup.mPreviousPrompt = 0;
        GRaceParameters *parms = GRaceDatabase::Get().GetRaceFromName("16.1.0");
        GRaceCustom *race = GRaceDatabase::Get().AllocCustomRace(parms);
        GRaceDatabase::Get().SetStartupRace(race, GRace::kRaceContext_Career);
        GRaceDatabase::Get().FreeCustomRace(race);
        RaceStarter::StartCareerFreeRoam();
    }
}

void CareerSettings::TryAwardDemoMarker() {
    if (!HasBeenAwardedDemoMarker() && gEasterEggs.IsEasterEggUnlocked(static_cast<EasterEggsSpecial>(5))) {
        TheFEMarkerManager.AddMarkerToInventory(FEMarkerManager::ePossibleMarker(2), 0);
        SetAwardedDemoMarker();
    }
}

void CareerSettings::ResumeCareer() {
    const char *lastDDayRace = GRaceDatabase::Get().GetDDayEndRace();
    bool dday_flow_completed = false;
    if (SkipDDayRaces || GRaceDatabase::Get().IsCareerRaceComplete(GRaceDatabase::Get().GetRaceFromName(lastDDayRace)->GetEventHash())) {
        dday_flow_completed = true;
    }

#ifdef EA_BUILD_A124
    // El prototipo no mira HasBeatenCareer: con el jefe final ganado vuelve siempre a la persecucion.
    bool finalBossRaceCompleted =
        GRaceDatabase::Get().IsCareerRaceComplete(GRaceDatabase::Get().GetRaceFromName(GRaceDatabase::Get().GetFinalBossRace())->GetEventHash());
#else
    const char *lastBossRace = GRaceDatabase::Get().GetFinalBossRace();
    bool finalBossRaceCompleted = false;
    if (!HasBeatenCareer()) {
        if (GRaceDatabase::Get().IsCareerRaceComplete(GRaceDatabase::Get().GetRaceFromName(lastBossRace)->GetEventHash())) {
            finalBossRaceCompleted = true;
        }
    }
#endif

    if (CurrentBin == 0x10) {
        if (!dday_flow_completed) {
#ifndef EA_BUILD_A124
            unsigned int hash = FEHashUpper("M3GTRCAREERSTART");
            FEDatabase->GetCareerSettings()->CurrentCar = hash;
            FEDatabase->GetQuickRaceSettings(GRace::kRaceType_NumTypes)->SelectedCar[0] = hash;
#endif
            const char *nextDDayRace = GRaceDatabase::Get().GetNextDDayRace();
            GRaceParameters *parms = GRaceDatabase::Get().GetRaceFromName(nextDDayRace);
            GRaceCustom *race = GRaceDatabase::Get().AllocCustomRace(parms);
            GRaceDatabase::Get().SetStartupRace(race, GRace::kRaceContext_Career);
            GRaceDatabase::Get().FreeCustomRace(race);
#ifndef EA_BUILD_A124
            if (bStrCmp(nextDDayRace, "16.1.0") != 0) {
                MemoryCard::GetInstance()->CancelNextAutoSave();
            }
#endif
        }
        RaceStarter::StartCareerFreeRoam();
    } else if (finalBossRaceCompleted) {
        const char *finalEpicChaseRace = "1.8.1";
        GRaceParameters *parms = GRaceDatabase::Get().GetRaceFromName(finalEpicChaseRace);
        GRaceCustom *race = GRaceDatabase::Get().AllocCustomRace(parms);
        GRaceDatabase::Get().SetStartupRace(race, GRace::kRaceContext_Career);
        GRaceDatabase::Get().FreeCustomRace(race);
        RaceStarter::StartCareerFreeRoam();
#ifndef EA_BUILD_A124
        MemoryCard::GetInstance()->CancelNextAutoSave();
#endif
    } else {
        FEManager::Get()->SetGarageType(static_cast<eGarageType>(2));
        FEDatabase->ClearGameMode(eFE_GAME_MODE_CAREER_MANAGER);
    }
    FEDatabase->SetGameMode(eFE_GAME_MODE_CAREER);
}

void CareerSettings::AwardOneTimeCashBonus(bool bOldSaveExists) {
    SpecialFlags = SpecialFlags | CS_ONE_TIME_CASH_BONUS;
    if (!bOldSaveExists) {
        return;
    }
    CurrentCash = CurrentCash + 10000;
}

void CareerSettings::SetPlayerHasBeatenTheGame() {
    SetHasBeatenCareer();
}

void CareerSettings::GenerateCaseFileName() {
    const int SCOTTS_RAND_CASE_FILE_NUMBER_RANGE = 0x19B3;
    const int SCOTTS_RAND_CASE_FILE_NUMBER_START = 0x42D;
    unsigned int num = bRandom(SCOTTS_RAND_CASE_FILE_NUMBER_RANGE) + SCOTTS_RAND_CASE_FILE_NUMBER_START;
    const char *profile_name = FEDatabase->GetUserProfile(0)->GetProfileName();
    bSNPrintf(CaseFileName, 13, "MW-%d-%.4sXXX", num, profile_name);
    bToUpper(CaseFileName);
}

char *CareerSettings::SaveToBuffer(void *buffer, void *maxbuf) {
    char *buf = SaveGameplayData(buffer, maxbuf);
    buf = SaveSomeData(buf, &CurrentCar, 4, maxbuf);
    buf = SaveSomeData(buf, &CurrentBin, 1, maxbuf);
    buf = SaveSomeData(buf, &CurrentCash, 4, maxbuf);
    buf = SaveSomeData(buf, &AdaptiveDifficulty, 2, maxbuf);
    buf = SaveSomeData(buf, &SpecialFlags, 4, maxbuf);
    buf = SaveSomeData(buf, SMSMessages,
#ifdef EA_BUILD_A124
                       500,
#else
                       600,
#endif
                       maxbuf);
#ifndef EA_BUILD_A124
    buf = SaveSomeData(buf, &SMSSortOrder, 2, maxbuf);
#endif
    buf = SaveSomeData(buf, CaseFileName, 16, maxbuf);
    buf = SaveRaceData(buf, maxbuf);
    buf = SaveUnlockData(buf, maxbuf);
    return TheFEMarkerManager.SaveToBuffer(buf);
}

char *CareerSettings::LoadFromBuffer(void *buffer, void *maxbuf) {
    char *buf = LoadGameplayData(buffer, maxbuf);
    buf = LoadSomeData(&CurrentCar, buf, 4, maxbuf);
    buf = LoadSomeData(&CurrentBin, buf, 1, maxbuf);
    buf = LoadSomeData(&CurrentCash, buf, 4, maxbuf);
    buf = LoadSomeData(&AdaptiveDifficulty, buf, 2, maxbuf);
    buf = LoadSomeData(&SpecialFlags, buf, 4, maxbuf);
    buf = LoadSomeData(SMSMessages, buf,
#ifdef EA_BUILD_A124
                       500,
#else
                       600,
#endif
                       maxbuf);
#ifndef EA_BUILD_A124
    buf = LoadSomeData(&SMSSortOrder, buf, 2, maxbuf);
#endif
    buf = LoadSomeData(CaseFileName, buf, 16, maxbuf);
    buf = LoadRaceData(buf, maxbuf);
    buf = LoadUnlockData(buf, maxbuf);
    return TheFEMarkerManager.LoadFromBuffer(buf);
}

int32 CareerSettings::GetSaveBufferSize(bool bExcludeGameplay) {
#ifdef EA_BUILD_A124 // los ajustes de carrera del prototipo ocupan menos
    int size = TheFEMarkerManager.GetSaveBufferSize() + 0x3e7;
#else
    int size = TheFEMarkerManager.GetSaveBufferSize() + 0x441;
#endif
    if (!bExcludeGameplay) {
        size += 0x52C4;
    }
    return size;
}

char *CareerSettings::SaveRaceData(void *save_to, void *maxptr) {
    if (!GRaceDatabase::Exists()) {
        return static_cast<char *>(save_to);
    }

    unsigned int nEntries = GRaceDatabase::Get().GetScoreInfoCount();
    nEntries = bMin(300, nEntries);

    char *buf = SaveSomeData(save_to, &nEntries, 4, maxptr);

    GRaceSaveInfo *current = GRaceDatabase::Get().GetScoreInfo();
    for (unsigned int index = 0; index < nEntries; index++) {

        if (gVerboseTesterOutput && current->mRaceHash != 0 && (current->mFlags & 2)) {

            GRaceDatabase::Get().GetRaceFromHash(current->mRaceHash);
        }




        buf = SaveSomeData(buf, current, 0x10, maxptr);
        current++;
    }

    return static_cast<char *>(save_to) + 0x12C4;
}

char *CareerSettings::SaveUnlockData(void *save_to, void *maxptr) {
    char *buf = static_cast<char *>(save_to);
    for (unsigned int i = 0; i < 0x39; i++) {
        buf = SaveSomeData(buf, &TheUnlockData[i], 8, maxptr);
    }
    return buf;
}

char *CareerSettings::SaveGameplayData(void *save_to, void *maxptr) {
    char *buf = static_cast<char *>(save_to);
    if (GManager::Exists()) {
        GManager::Get().SaveGameplayData(reinterpret_cast<uint8 *>(buf), 0x4000);
    } else {
        bMemSet(buf, 0, 0x4000);
    }
    return buf + 0x4000;
}

char *CareerSettings::LoadRaceData(void *load_from_here, void *maxptr) {
    if (!GRaceDatabase::Exists()) {
        return static_cast<char *>(load_from_here);
    }

    unsigned int nEntries = 0;
    char *buf = LoadSomeData(&nEntries, static_cast<char *>(load_from_here), 4, maxptr);
    nEntries = bMin(300, static_cast<int>(nEntries));
    GRaceSaveInfo saveInfoEntries[300];
    GRaceSaveInfo *current = saveInfoEntries;
    for (unsigned int index = 0; index < nEntries; index++) {
        buf = LoadSomeData(current, buf, 0x10, maxptr);
        if (gVerboseTesterOutput && current->mRaceHash != 0 && (current->mFlags & 2)) {
            GRaceDatabase::Get().GetRaceFromHash(current->mRaceHash);
        }
        current++;
    }
    GRaceDatabase::Get().LoadBestScores(saveInfoEntries, nEntries);
    return static_cast<char *>(load_from_here) + 0x12C4;
}

char *CareerSettings::LoadUnlockData(void *load_from, void *maxptr) {
    char *buf = static_cast<char *>(load_from);
    for (unsigned int i = 0; i < 0x39; i++) {
        buf = LoadSomeData(&TheUnlockData[i], buf, 8, maxptr);
    }
    return buf;
}

char *CareerSettings::LoadGameplayData(void *load_from_here, void *maxptr) {
    char *buf = static_cast<char *>(load_from_here);
    if (GManager::Exists()) {
        GManager::Get().LoadGameplayData(reinterpret_cast<unsigned char *>(buf), 0x4000);
    }
    return buf + 0x4000;
}

UserProfile::UserProfile()
    : TheOptionsSettings(), TheCareerSettings(),
#ifdef EA_BUILD_A124
      Playlist(nullptr),
#endif
      HighScores() {}

UserProfile::~UserProfile() {
#ifdef EA_BUILD_A124
    delete[] Playlist;
    Playlist = nullptr;
#endif
}

void UserProfile::SetProfileName(const char *pName, bool isP1) {
    bool named = false;
    if (pName && bStrLen(pName) > 0) {
        named = true;
    }
    bMemSet(m_aProfileName, 0, 0x20);
    if (named) {
        bStrNCpy(m_aProfileName, pName, 0x20);
        m_bNamed = true;
    } else {
        char defaultName[32];
        if (isP1 == true) {
            GetLocalizedString(defaultName, 0x20, LANGUAGE_COMMON_PLAYER_1);
        } else {
            GetLocalizedString(defaultName, 0x20, LANGUAGE_COMMON_PLAYER_2);
        }
        bStrNCpy(m_aProfileName, defaultName, 0x20);
        m_bNamed = false;
    }
}

const char *UserProfile::GetProfileName() {
    return m_aProfileName;
}

bool UserProfile::IsProfileNamed() {
    return m_bNamed;
}

void UserProfile::Default(int player_number, bool commit_default) {
#ifdef EA_BUILD_A124 // el prototipo reserva la lista de canciones segun la licencia y elige ya el modo de audio
    if (!player_number) {
        SetProfileName(nullptr, true);
    } else {
        SetProfileName(nullptr, false);
    }

    PlayersCarStable.Default();

    if (!player_number) {
        TheOptionsSettings.Default();
        TheCareerSettings.Default();
        HighScores.Default();
        CareerModeHasBeenCompletedAtLeastOnce = false;

        if (g_pEAXSound) {
            TheOptionsSettings.TheAudioSettings.AudioMode = g_pEAXSound->GetDefaultPlatformAudioMode();
        }

        if (!Playlist) {
            Attrib::Gen::audiosystem *playlist_atrs = new Attrib::Gen::audiosystem(0x7E4B0ED2, 0, nullptr);
            if (playlist_atrs->IsValid()) {
                Attrib::Gen::audiosystem licensed_music(playlist_atrs->LicensedMusic(), 0, nullptr);
                g_MaxSongs = licensed_music.Num_PFMapping();

                for (int i = 0; i < static_cast<int>(Songs.size()); i++) {
                    delete Songs[i];
                }
                Songs.clear();

                if (static_cast<int>(Songs.capacity()) < g_MaxSongs) {
                    Songs.reserve(g_MaxSongs);
                }

                for (int i = 0; i < g_MaxSongs; i++) {
                    Sound::stSongInfo *newsong = new ("Sound::stSongInfo", 0) Sound::stSongInfo;
                    Attrib::Gen::music currsong(static_cast<const Attrib::Collection *>(nullptr), 0, nullptr);
                    const char *tmpSongName;
                    currsong.ChangeWithDefault(licensed_music.PFMapping(i));
                    tmpSongName = currsong.SongName().GetString();
                    newsong->SongName = const_cast<char *>(currsong.SongName().GetString());
                    newsong->Album = const_cast<char *>(currsong.Album().GetString());
                    newsong->Artist = const_cast<char *>(currsong.Artist().GetString());
                    newsong->DefPlay = const_cast<char *>(currsong.DefPlay().GetString());
                    newsong->PathEvent = currsong.PathEvent();
                    Songs.push_back(newsong);
                }
            }
        }

        if (Playlist) {
            delete[] Playlist;
        }
        Playlist = new ("JukeboxEntry[]", 0) JukeboxEntry[Songs.size()];

        for (unsigned int i = 0; i < Songs.size(); i++) {
            Playlist[i].SongIndex = i;

            Sound::stSongInfo *song = Songs[i];
            eSongPlayability epbf;
            if (song == nullptr) {
                epbf = ePLAY_OFF;
            } else {
                if (bStrCmp(song->DefPlay, "FE") == 0) {
                    epbf = ePLAY_MENU;
                } else if (bStrCmp(song->DefPlay, "IG") == 0) {
                    epbf = ePLAY_RACE;
                } else if (bStrCmp(song->DefPlay, "AL") == 0) {
                    epbf = ePLAY_ALL;
                } else {
                    epbf = ePLAY_OFF;
                }
            }
            Playlist[i].PlayabilityField = epbf;
        }

        InitializeEATrax();
    }
#else
    static bool song_info_loaded = false;

    if (!player_number) {
        SetProfileName(nullptr, true);
    } else {
        SetProfileName(nullptr, false);
    }

    PlayersCarStable.Default();

    if (!player_number) {
        TheOptionsSettings.Default();
        TheCareerSettings.Default();
        HighScores.Default();
        CareerModeHasBeenCompletedAtLeastOnce = false;

        if (!song_info_loaded) {
            song_info_loaded = true;

            Attrib::Gen::audiosystem *playlist_atrs = new Attrib::Gen::audiosystem(0x7E4B0ED2, 0, nullptr);
            if (playlist_atrs->IsValid()) {
                Attrib::Gen::audiosystem licensed_music(playlist_atrs->LicensedMusic(), 0, nullptr);
                g_MaxSongs = licensed_music.Num_PFMapping();

                for (int i = 0; i < static_cast<int>(Songs.size()); i++) {
                    delete Songs[i];
                }
                Songs.clear();

                if (static_cast<int>(Songs.capacity()) < g_MaxSongs) {
                    Songs.reserve(g_MaxSongs);
                }

                for (int i = 0; i < g_MaxSongs; i++) {
                    Sound::stSongInfo *newsong = new ("Sound::stSongInfo", 0) Sound::stSongInfo;
                    Attrib::Gen::music currsong(static_cast<const Attrib::Collection *>(nullptr), 0, nullptr);
                    const char *tmpSongName;

                    currsong.ChangeWithDefault(licensed_music.PFMapping(i));

                    tmpSongName = currsong.SongName().GetString();
                    newsong->SongName = const_cast<char *>(currsong.SongName().GetString());
                    tmpSongName = currsong.Album().GetString();
                    newsong->Album = const_cast<char *>(tmpSongName);
                    tmpSongName = currsong.Artist().GetString();
                    newsong->Artist = const_cast<char *>(tmpSongName);
                    tmpSongName = currsong.DefPlay().GetString();
                    newsong->DefPlay = const_cast<char *>(tmpSongName);
                    newsong->PathEvent = currsong.PathEvent();
                    Songs.push_back(newsong);
                }
            }
        }

        for (int i = 0; i < g_MaxSongs; i++) {
            Playlist[i].SongIndex = i;

            Sound::stSongInfo *song = Songs[i];
            unsigned char playability;
            if (song == nullptr) {
                playability = 0;
            } else {
                if (bStrCmp(song->DefPlay, "FE") == 0) {
                    playability = 1;
                } else if (bStrCmp(song->DefPlay, "IG") == 0) {
                    playability = 2;
                } else if (bStrCmp(song->DefPlay, "AL") == 0) {
                    playability = 3;
                } else {
                    playability = 0;
                }
            }
            Playlist[i].PlayabilityField = playability;
        }

        InitializeEATrax(
#ifndef EA_BUILD_A124
            true
#endif
        );
    }

    PlayersCarStable.AwardBonusCars();
    TheCareerSettings.TryAwardDemoMarker();
#endif
}

void UserProfile::CommitHighScoresPauseQuit() {
    HighScores.CommitHighScoresPauseQuit();
}

void UserProfile::CommitPursuitInfo(IPursuit *iPursuit, uint32 car_handle, uint32 bounty, unsigned int num_infractions) {
    HighScores.CommitPursuitInfo(iPursuit, car_handle, bounty, num_infractions);
}

void UserProfile::IncInfration(GInfractionManager::InfractionType infrat, unsigned int car) {}
void UserProfile::CommitServeInfractions(unsigned int car) {}

void UserProfile::WriteProfileHash(void *bufferToHash, void *bufferToWrite, int bytes, void *maxptr) {
    MD5 md5;
    md5.Update(bufferToHash, bytes);
    md5.GetRaw();
    SaveSomeData(bufferToWrite, md5.GetRaw(), 0x10, maxptr);
}

bool UserProfile::VerifyProfileHash(void *bufferToHash, void *bufferHash, int bytes) {
    MD5 md5;
    md5.Update(bufferToHash, bytes);
    md5.GetRaw();
    return bMemCmp(md5.GetRaw(), bufferHash, 0x10) == 0;
}

#ifdef EA_PLATFORM_WIN32 // PC: los mapeos del mando de GameDevice (19 juegos) van detras de lo de online; nombres neutros
char *pc_fedb_56E180(char *buf, char *maxbuf); // probablemente los guarda
char *pc_fedb_56E200(char *buf, char *maxbuf); // probablemente los carga
int pc_fedb_56E290();                          // probablemente su tamano en la partida guardada
#endif
void UserProfile::SaveToBuffer(void *buffer, int size) {
    char *buf = static_cast<char *>(buffer);
    char *maxbuf = buf + size;
    bMemSet(buf, 0, size);
    char aVersion[16];
    bMemSet(aVersion, 0, 0x10);
    buf = SaveSomeData(buf, aVersion, 0x10, maxbuf);
    buf = TheCareerSettings.SaveToBuffer(buf, maxbuf);
    buf = SaveSomeData(buf, &FEDatabase->iDefaultStableHash, 4, maxbuf);
    buf = SaveSomeData(buf, m_aProfileName, 0x20, maxbuf);
#ifdef EA_BUILD_A124 // el prototipo guarda la lista de canciones entera, un solo coche de carrera rapida y lo de online
    buf = SaveSomeData(buf, Playlist, g_MaxSongs * sizeof(JukeboxEntry), maxbuf);
    buf = SaveSomeData(buf, &TheOptionsSettings, 0xC0, maxbuf);
    buf = PlayersCarStable.SaveToBuffer(buf, maxbuf - buf);
    buf = SaveSomeData(buf, &CareerModeHasBeenCompletedAtLeastOnce, 4, maxbuf);
    buf = SaveSomeData(buf, &HighScores, 0xBD8, maxbuf);
    buf = SaveSomeData(buf, &FEDatabase->GetQuickRaceSettings(GRace::kRaceType_NumTypes)->SelectedCar[0], 4, maxbuf);
    buf = FEDatabase->OnlineSettings.SaveToBuffer(buf);
#else
    buf = SaveSomeData(buf, Playlist, 0xF0, maxbuf);
    buf = SaveSomeData(buf, &TheOptionsSettings, sizeof(TheOptionsSettings), maxbuf);
    int stableSize = PlayersCarStable.GetSaveBufferSize();
    buf = PlayersCarStable.SaveToBuffer(buf, stableSize);
    buf = SaveSomeData(buf, &CareerModeHasBeenCompletedAtLeastOnce, sizeof(CareerModeHasBeenCompletedAtLeastOnce), maxbuf);
    buf = SaveSomeData(buf, &HighScores, 0xBD8, maxbuf);
    for (int i = 0; i < 11; i++) {
        RaceSettings *settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(i));
        unsigned int h = settings->GetSelectedCar(0);
        buf = SaveSomeData(buf, &h, 4, maxbuf);
    }
#if defined(EA_PLATFORM_WIN32) && ONLINE_SUPPORT
    buf = FEDatabase->OnlineSettings.SaveToBuffer(buf);
#endif
#ifdef EA_PLATFORM_WIN32
    buf = pc_fedb_56E180(buf, maxbuf);
#endif
#endif
    WriteProfileHash(static_cast<char *>(buffer) + 0x10, buf, size - 0x20, maxbuf);
}
bool UserProfile::LoadFromBuffer(void *buffer, int size, bool commit_changes
#ifndef EA_BUILD_A124
                                 ,
                                 int player_id
#endif
) {
#ifdef EA_BUILD_A124 // el prototipo carga la lista de canciones entera y lo de online, y aplica el audio al momento
    char *buf = static_cast<char *>(buffer);
    char *maxbuf = buf + size;
    char aVersion[16];
    buf = LoadSomeData(aVersion, buf, 0x10, maxbuf);
    buf = TheCareerSettings.LoadFromBuffer(buf, maxbuf);
    unsigned int version;
    buf = LoadSomeData(&version, buf, 4, maxbuf);
    if (version != FEDatabase->iDefaultStableHash) {
        return false;
    }
    buf = LoadSomeData(m_aProfileName, buf, 0x20, maxbuf);
    buf = LoadSomeData(Playlist, buf, g_MaxSongs * sizeof(JukeboxEntry), maxbuf);
    buf = LoadSomeData(&TheOptionsSettings, buf, 0xC0, maxbuf);
    buf = PlayersCarStable.LoadFromBuffer(buf, maxbuf - buf);
    PlayersCarStable.AwardBonusCars();
    buf = LoadSomeData(&CareerModeHasBeenCompletedAtLeastOnce, buf, 4, maxbuf);
    buf = LoadSomeData(&HighScores, buf, 0xBD8, maxbuf);
    buf = LoadSomeData(&FEDatabase->GetQuickRaceSettings(GRace::kRaceType_NumTypes)->SelectedCar[0], buf, 4, maxbuf);
    buf = FEDatabase->OnlineSettings.LoadFromBuffer(buf);
    if (!VerifyProfileHash(static_cast<char *>(buffer) + 0x10, buf, size - 0x20)) {
        return false;
    }
    m_bNamed = true;
    if (commit_changes) {
        TheOptionsSettings.TheAudioSettings.AudioMode =
            g_pEAXSound->SetAudioModeFromMemoryCard(static_cast<eSndAudioMode>(TheOptionsSettings.TheAudioSettings.AudioMode));
        g_pEAXSound->UpdateVolumes(&TheOptionsSettings.TheAudioSettings, -1.0f);
        MControlPathfinder(true, 0, 0, 0).Send("EATraxInit");
    }
    return true;
#else
    char *buf = static_cast<char *>(buffer);
    char *maxbuf = buf + size;
    char aVersion[16];
    buf = LoadSomeData(aVersion, buf, 0x10, maxbuf);
    if (!player_id) {
        buf = TheCareerSettings.LoadFromBuffer(buf, maxbuf);
        TheCareerSettings.TryAwardDemoMarker();
    } else {
        int careerSize = TheCareerSettings.GetSaveBufferSize(false);
        buf = buf + careerSize;
    }
    unsigned int version;
    buf = LoadSomeData(&version, buf, 4, maxbuf);
    if (version != FEDatabase->iDefaultStableHash) {
        return false;
    }
    buf = LoadSomeData(m_aProfileName, buf, 0x20, maxbuf);
    if (!player_id) {
        buf = LoadSomeData(Playlist, buf, 0xF0, maxbuf);
    } else {
        buf = buf + 0xF0;
    }
    buf = LoadSomeData(&TheOptionsSettings, buf, sizeof(TheOptionsSettings), maxbuf);
    int stableSize = PlayersCarStable.GetSaveBufferSize();
    buf = PlayersCarStable.LoadFromBuffer(buf, stableSize);
    PlayersCarStable.AwardBonusCars();
    buf = LoadSomeData(&CareerModeHasBeenCompletedAtLeastOnce, buf, sizeof(CareerModeHasBeenCompletedAtLeastOnce), maxbuf);
    buf = LoadSomeData(&HighScores, buf, 0xBD8, maxbuf);
    for (int i = 0; i < 11; i++) {
        unsigned int h;
        buf = LoadSomeData(&h, buf, 4, maxbuf);
        RaceSettings *settings = FEDatabase->GetQuickRaceSettings(static_cast<GRace::Type>(i));
        settings->SelectedCar[player_id] = h;
    }
#if defined(EA_PLATFORM_WIN32) && ONLINE_SUPPORT
    buf = FEDatabase->OnlineSettings.LoadFromBuffer(buf);
#endif
#ifdef EA_PLATFORM_WIN32
    buf = pc_fedb_56E200(buf, maxbuf);
#endif
    if (!VerifyProfileHash(static_cast<char *>(buffer) + 0x10, buf, size - 0x20)) {
        return false;
    }
    m_bNamed = true;
#ifdef EA_PLATFORM_WIN32 // PC aplica el audio al momento, como la A124
    if (commit_changes) {
        TheOptionsSettings.TheAudioSettings.AudioMode =
            g_pEAXSound->SetAudioModeFromMemoryCard(static_cast<eSndAudioMode>(TheOptionsSettings.TheAudioSettings.AudioMode));
        g_pEAXSound->UpdateVolumes(&TheOptionsSettings.TheAudioSettings, -1.0f);
        MControlPathfinder(true, 0, 0, 0).Send("EATraxInit");
    }
#endif
    return true;
#endif
}
int32 UserProfile::GetSaveBufferSize(bool bExcludeGameplay) {
#ifdef EA_BUILD_A124
    int size = g_MaxSongs * sizeof(JukeboxEntry) + 0xf4;
    size += TheCareerSettings.GetSaveBufferSize(bExcludeGameplay);
    size += PlayersCarStable.GetSaveBufferSize();
    size += 0xbe0;
    size += FEDatabase->OnlineSettings.GetSaveBufferSize();
    return size + 0x10;
#else
    int size = TheCareerSettings.GetSaveBufferSize(bExcludeGameplay) + 0x124 + sizeof(TheOptionsSettings);
#if defined(EA_PLATFORM_WIN32) && ONLINE_SUPPORT
    size += FEDatabase->OnlineSettings.GetSaveBufferSize();
#endif
#ifdef EA_PLATFORM_WIN32
    size += pc_fedb_56E290();
#endif
    return size + PlayersCarStable.GetSaveBufferSize() + 0xc14 + sizeof(CareerModeHasBeenCompletedAtLeastOnce);
#endif
}
cFrontendDatabase::cFrontendDatabase()
    : iDefaultStableHash(0), m_pCarStableBackup(nullptr), m_pDBBackup(nullptr), SplitScreenCustomization(nullptr) {
    for (int i = 0; i < 2; i++) {
        CurrentUserProfiles[i] = nullptr;
    }
    CurrentUserProfiles[0] = new ("Player 1 UserProfile", 0) UserProfile();
}

// clang-format off
// Relleno: desde aqui hasta AllocBackupDB las lineas siguen al original de la A124, cuyo bMalloc
// (MILESTONE_BUILD) pasa __LINE__: 1145 en Default y 1407 en AllocBackupDB. No anadas ni quites lineas.
void cFrontendDatabase::Default() {
    iNumPlayers = 1;
    bComingFromBoot = true;
    bProfileLoaded = false;
    bIsOptionsDirty = false;
#if !defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_PLATFORM_WIN32) // speed.exe no lo pone a false aqui
    bAutoSaveOverwriteConfirmed = false;
#endif
    GetUserProfile(0)->Default(0, true);
    iCurPauseSubOptionType = 0;
    iCurPauseOptionType = 0;
    FEGameMode = 0;
#ifndef EA_BUILD_A124
    if (SkipFE && SkipFESplitScreen) {
        FEGameMode = 4;
        iNumPlayers = 2;
    }
#endif
    PlayerJoyports[0] = 0;
    PlayerJoyports[1] = -1;
    RaceMode = GRace::kRaceType_Circuit;
    unsigned int default_car = GetDefaultCar();
    DefaultRaceSettings();
    GetUserProfile(0)->GetCareer()->SetCurrentCar(default_car);
    if (!iDefaultStableHash) {
        FEPlayerCarDB *stable = &GetUserProfile(0)->PlayersCarStable;
        int buf_size = stable->GetSaveBufferSize();
        char *buf = static_cast<char *>(bMalloc(buf_size, "Memcard Car Stable Hashing", __LINE__, 0x40));
        int save_size = stable->GetSaveBufferSize();
        stable->SaveToBuffer(buf, save_size);




































        int crc_size = stable->GetSaveBufferSize();
        iDefaultStableHash = bCalculateCrc32(buf, crc_size, 0xFFFFFFFF);
        bFree(buf);
    }
}


void cFrontendDatabase::DefaultProfile() {
#ifdef EA_BUILD_A124 // el prototipo no tiene desbloqueos, marcadores ni puntuaciones que borrar
    CurrentUserProfiles[0]->Default(0, true);
    DefaultRaceSettings();
    unsigned int default_car = GetDefaultCar();
    GetCareerSettings()->SetCurrentCar(default_car);
    bProfileLoaded = false;
    bIsOptionsDirty = false;
    GetPlayerCarStable(0)->Default();
    GetQuickRaceSettings(GRace::kRaceType_NumTypes)->SetSelectedCar(GetDefaultCar(), 0);
    MemoryCard::GetInstance()->SetCardRemovedWithAutoSaveEnabled(false);
#else
    CurrentUserProfiles[0]->Default(0, true);
#ifndef EA_PLATFORM_PLAYSTATION2
    bAutoSaveOverwriteConfirmed = false;
#endif
    DefaultRaceSettings();
    unsigned int default_car = GetDefaultCar();
    GetCareerSettings()->SetCurrentCar(default_car);
    bIsOptionsDirty = false;
    GetPlayerCarStable(0)->Default();
    MemoryCard::GetInstance()->SetCardRemovedWithAutoSaveEnabled(false);
    DefaultUnlockData();
    TheFEMarkerManager.Default();
    if (GRaceDatabase::Exists()) {
        GRaceDatabase::Get().ClearRaceScores();
    }
    if (GManager::Exists()) {
        GManager::Get().ResetAllGameplayData();
    }
#endif
}

void cFrontendDatabase::DefaultRaceSettings() {
    unsigned int default_car = GetDefaultCar();
    for (unsigned int i = 0; i < 11; i++) {
        RaceSettings &settings = TheQuickRaceSettings[i];
        settings.Default();
        settings.SetSelectedCar(default_car, 0);
        settings.SetSelectedCar(default_car, 1);
#ifdef EA_BUILD_A124 // el filtro de coches de cada jugador vive en la carrera rapida
        settings.CarSelectFilterBits[0] = 0xf0001;
        settings.CarSelectFilterBits[1] = 0xf0001;
#endif
    }
    TheQuickRaceSettings[0].NumLaps = 1;
    TheQuickRaceSettings[2].NumLaps = 1;
    TheQuickRaceSettings[5].NumLaps = 1;
    TheQuickRaceSettings[3].NumLaps = TheQuickRaceSettings[3].NumOpponents;
    TheQuickRaceSettings[4].NumLaps = 1;
    TheQuickRaceSettings[4].NumOpponents = 0;
}
























void cFrontendDatabase::NotifyDeleteCar(uint32 handle) {
    uint32 default_car = GetDefaultCar();
    for (unsigned int i = 0; i < 11; i++) {
        RaceSettings &settings = TheQuickRaceSettings[i];
        if (settings.SelectedCar[0] == handle) {
            settings.SelectedCar[0] = default_car;
        }
        if (settings.SelectedCar[1] == handle) {
            settings.SelectedCar[0] = default_car;
        }
    }
}


void cFrontendDatabase::SetPlayersJoystickPort(int player, int8 joy_port) {
    if (joy_port == -1 && PlayerJoyports[player] != -1) {
        cFEngJoyInput::mInstance->SetRequiredJoy(static_cast<JoystickPort>(PlayerJoyports[player]), false);
    }
    PlayerJoyports[player] = joy_port;
}























unsigned int cFrontendDatabase::GetDefaultCar() {
    Attrib::Gen::frontend frontend(0xeec2271a, 0, nullptr);
    Attrib::RefSpec spec;
    FEPlayerCarDB *stable = FEDatabase->GetPlayerCarStable(0);
    unsigned int handle = 0;

    ATTRIB_GETLAYOUTFIELD(frontend, default_car, spec);

    Attrib::Gen::pvehicle vehicle(spec, 0, nullptr);
    Attrib::Key collection = vehicle.GetCollection();

    for (int i = 0; i < 200; i++) {
        FECarRecord *car = stable->GetCarByIndex(i);

        if (car->IsValid() && car->VehicleKey == collection) {
            handle = car->Handle;
            break;
        }
    }
    return handle;
}


















void cFrontendDatabase::CreateMultiplayerProfile(int player) {
#ifdef EA_BUILD_A124 // el prototipo rehace siempre el perfil
    DeleteMultiplayerProfile(player);
    CurrentUserProfiles[player] = new ("FEDatabase UserProfile", 0) UserProfile;
    CurrentUserProfiles[player]->Default(player, true);
#else
    if (!CurrentUserProfiles[player]) {
        CurrentUserProfiles[player] = new ("FEDatabase UserProfile", 0) UserProfile;
        CurrentUserProfiles[player]->Default(player, true);
    }
#endif
}

void cFrontendDatabase::DeleteMultiplayerProfile(int player) {
#ifdef EA_BUILD_A124 // el prototipo borra el perfil de cualquier jugador
    if (CurrentUserProfiles[player]) {
        if (player == 1) {
            RaceSettings *settings = GetQuickRaceSettings(GRace::kRaceType_NumTypes);
            FEPlayerCarDB *stable = GetPlayerCarStable(1);
            FECarRecord *record = stable->GetCarRecordByHandle(settings->GetSelectedCar(1));
            FECustomizationRecord *customization = stable->GetCustomizationRecordByHandle(record->Customization);
            bStrCpy(SplitScreenCarType, record->GetDebugName());
            if (customization) {
                SplitScreenCustomization = static_cast<FECustomizationRecord *>(bMalloc(sizeof(FECustomizationRecord), "Splitscreen FECustomizationRecord", 0, 0x47));
                bMemCpy(SplitScreenCustomization, customization, sizeof(FECustomizationRecord));
            } else {
                SplitScreenCustomization = nullptr;
            }
        }
        delete CurrentUserProfiles[player];
        CurrentUserProfiles[player] = nullptr;
    }
#else
    if (player == 1 && CurrentUserProfiles[1]) {
        RaceSettings *settings = GetQuickRaceSettings(GRace::kRaceType_NumTypes);
        FEPlayerCarDB *stable = GetPlayerCarStable(1);
        FECarRecord *record = stable->GetCarRecordByHandle(settings->GetSelectedCar(1));
        FECustomizationRecord *customization = stable->GetCustomizationRecordByHandle(record->Customization);
        bStrCpy(SplitScreenCarType, record->GetDebugName());
        if (customization) {
            SplitScreenCustomization = static_cast<FECustomizationRecord *>(bMalloc(sizeof(FECustomizationRecord), "Splitscreen FECustomizationRecord", 0, 0x47));
            bMemCpy(SplitScreenCustomization, customization, sizeof(FECustomizationRecord));
        } else {
            SplitScreenCustomization = nullptr;
        }
        delete CurrentUserProfiles[player];
        CurrentUserProfiles[player] = nullptr;
    }
#endif
}
void cFrontendDatabase::AllocBackupDB(bool bForce) {

    if (m_pDBBackup) {
        return;
    }

// clang-format on
    if (bForce) {
        m_pDBBackup = static_cast<char *>(bMalloc(GetUserProfileSaveSize(false), "BackupDB Buffer", __LINE__, BMEMORY_ALLOCATE_FROM_TOP));
        SaveUserProfileToBuffer(m_pDBBackup, GetUserProfileSaveSize(false));
    }
}

#ifdef EA_PLATFORM_WIN32
#include "Speed/Indep/Src/Input/IOModule.h"
#include "Speed/Indep/Src/Input/PC/InputDevicePC.h"

// PC (0x56E180): el hash del mando (GetWin32Unknown634DA0(3)) y los mapeos de los controles configurables.
char *pc_fedb_56E180(char *buf, char *maxbuf) {
    GameDevice *device = static_cast<GameDevice *>(IOModule::GetIOModule().GetDevice(0));
    unsigned int joystick = device->GetWin32Unknown634DA0(3);
    buf = SaveSomeData(buf, &joystick, sizeof(joystick), maxbuf);
    for (int i = 0; i < NUM_CONFIGURABLE_GAME_ACTIONS; i++) {
        buf = SaveSomeData(buf, device->GetMappings(i), device->GetMappingsSize(), maxbuf);
    }
    return buf;
}

// PC (0x56E200): los carga y, si la partida es de otro mando que el de ahora, vuelve a los del mando por defecto.
char *pc_fedb_56E200(char *buf, char *maxbuf) {
    GameDevice *device = static_cast<GameDevice *>(IOModule::GetIOModule().GetDevice(0));
    unsigned int joystick;
    buf = LoadSomeData(&joystick, buf, sizeof(joystick), maxbuf);
    for (int i = 0; i < NUM_CONFIGURABLE_GAME_ACTIONS; i++) {
        buf = LoadSomeData(device->GetMappings(i), buf, device->GetMappingsSize(), maxbuf);
    }
    unsigned int current = device->GetWin32Unknown634DA0(3);
    if (joystick != current && current != 0) {
        device->RestoreJoystickDefaults();
    }
    return buf;
}

// PC (0x56E290): el tamano de lo que guarda pc_fedb_56E180 (el hash del mando y los mapeos). Con la suma en una local:
// asi AllocBackupDB y RestoreFromBackupDB suman `19*M + 4` antes que el resto, como speed.exe.
int pc_fedb_56E290() {
    GameDevice *device = static_cast<GameDevice *>(IOModule::GetIOModule().GetDevice(0));
    int size = sizeof(unsigned int);
    size += device->GetMappingsSize() * NUM_CONFIGURABLE_GAME_ACTIONS;
    return size;
}
#endif

void cFrontendDatabase::DeallocBackupDB() {
    if (m_pDBBackup) {
        bFree(m_pDBBackup);
        m_pDBBackup = nullptr;
    }
}

void cFrontendDatabase::RestoreFromBackupDB() {
    if (m_pDBBackup) {
        LoadUserProfileFromBuffer(m_pDBBackup, GetUserProfileSaveSize(false), 0);
        DeallocBackupDB();
    }
}
void cFrontendDatabase::BackupCarStable() {
    if (!m_pCarStableBackup) {
        m_pCarStableBackup = static_cast<char *>(bMalloc(GetPlayerCarStable(0)->GetSaveBufferSize(), "CarStable backup buffer", 0, 0));
        bMemCpy(m_pCarStableBackup, GetPlayerCarStable(0), GetPlayerCarStable(0)->GetSaveBufferSize());
    }
}

bool cFrontendDatabase::IsCarStableDirty() {
    if (!m_pCarStableBackup) {
        return false;
    }
    bool result = bMemCmp(m_pCarStableBackup, GetPlayerCarStable(0), GetPlayerCarStable(0)->GetSaveBufferSize()) != 0;
    bFree(m_pCarStableBackup);
    m_pCarStableBackup = nullptr;
    return result;
}

void cFrontendDatabase::RefreshCurrentRide() {
    RideInfo ride;
    FEPlayerCarDB *stable = GetPlayerCarStable(0);
    if (IsCareerMode() || IsSafehouseMode() || IsCareerManagerMode()) {
        BuildCurrentRideForPlayer(0, &ride);
    } else {
        RaceSettings *settings = GetQuickRaceSettings(GRace::kRaceType_NumTypes);
        unsigned int handle = settings->GetSelectedCar(0);
        stable->BuildRideForPlayer(handle, 0, &ride);
    }
    CarViewer::SetRideInfo(&ride, static_cast<eSetRideInfoReasons>(2), static_cast<eCarViewerWhichCar>(0));
}

RaceSettings *cFrontendDatabase::GetQuickRaceSettings(GRace::Type type) {
    if (type >= GRace::kRaceType_NumTypes) {
        return &TheQuickRaceSettings[RaceMode];
    }
    return &TheQuickRaceSettings[type];
}

#ifndef EA_BUILD_A124
bool cFrontendDatabase::IsFinalEpicChase() {
    if (GRaceStatus::Exists()) {
        bool ret = false;
        GRaceParameters *params = GRaceStatus::Get().GetRaceParameters();

        if (params != nullptr) {
            ret = params->GetEventHash() == Attrib::StringHash32("1.8.1");
        }
        return ret;
    }
    return false;
}
#endif

void cFrontendDatabase::GetRandomRaceOptions(RaceSettings *race, GRace::Type type) {
#ifdef EA_BUILD_A124 // el prototipo sortea tambien la ayuda y la habilidad
    race->CatchUp = bRandom(1) != 0;
    race->CopDensity = static_cast<uint8>(bRandom(4));
    race->AISkill = static_cast<uint8>(bRandom(3));
#else
    race->CatchUp = true;
    race->CopDensity = static_cast<uint8>(bRandom(4));
    race->AISkill = 1;
#endif
    race->NumOpponents = static_cast<uint8>(bRandom(3) + 1);
    switch (type) {
    case GRace::kRaceType_Knockout:
        race->NumLaps = race->NumOpponents;
        break;
    case GRace::kRaceType_Circuit:
        race->NumLaps = static_cast<uint8>(bRandom(5) + 1);
        break;
    default:
        race->NumLaps = 1;
        break;
    }
    race->TrafficDensity = static_cast<uint8>(bRandom(4));
    race->TrackDirection = static_cast<uint8>(bRandom(1));
}

void cFrontendDatabase::FillCustomRace(GRaceCustom *parms, RaceSettings *race) {
    if (!race) {
        return;
    }
    if (!parms) {
        return;
    }
    parms->SetCatchUp(race->CatchUp);
    parms->SetCopsEnabled(race->CopsOn);
#ifdef EA_BUILD_A124
    parms->SetCopDensity(static_cast<GRace::CopDensity>(race->CopDensity));
#else
    if (race->CopsOn) {
        parms->SetHeatLevel(race->CopDensity);
    }
#endif
    parms->SetDifficulty(static_cast<GRace::Difficulty>(race->AISkill));
    parms->SetNumLaps(race->NumLaps);
    parms->SetNumOpponents(race->NumOpponents);
    switch (race->TrafficDensity) {
        case 0:
        default:
            parms->SetTrafficDensity(0);
            break;
#ifdef EA_BUILD_A124
        case 1:
            parms->SetTrafficDensity(33);
            break;
        case 2:
            parms->SetTrafficDensity(66);
            break;
        case 3:
            parms->SetTrafficDensity(100);
            break;
    }
    parms->SetReversed(race->TrackDirection != 0);
#else
        case 1:
            parms->SetTrafficDensity(10);
            break;
        case 2:
            parms->SetTrafficDensity(30);
            break;
        case 3:
            parms->SetTrafficDensity(50);
            break;
    }
    parms->SetReversed(race->TrackDirection == 1);
#endif
}

void cFrontendDatabase::BuildCurrentRideForPlayer(int player, RideInfo *ride) {
    FEPlayerCarDB *stable = (player >= 0 && player <= 1) ? GetPlayerCarStable(player) : nullptr;
    unsigned int car;

    // El original mezcla accesos por this y por el global FEDatabase.
    if (FEDatabase->IsQuickRaceMode() || FEDatabase->IsLanMode() || FEDatabase->IsOnlineMode()) {
        car = GetQuickRaceSettings(GRace::kRaceType_NumTypes)->GetSelectedCar(player);
    } else {
        car = FEDatabase->GetCareerSettings()->GetCurrentCar();
    }

    stable->BuildRideForPlayer(car, player, ride);
}

void cFrontendDatabase::NotifyExitRaceToFrontend(eExitRacePlaces from_where) {
    {
        int is_split = IsSplitScreenMode();
    }
    PostRaceOptionChosen = static_cast<ePostRaceOptions>(1);
    if (from_where == EXIT_RACE_FROM_PAUSE) {
        CurrentUserProfiles[0]->CommitHighScoresPauseQuit();
    }
}

int32 cFrontendDatabase::GetUserProfileSaveSize(bool bExcludeGameplay) {
    return CurrentUserProfiles[0]->GetSaveBufferSize(bExcludeGameplay);
}

void cFrontendDatabase::SaveUserProfileToBuffer(void *buffer, int32 bufsize) {
    CurrentUserProfiles[0]->SaveToBuffer(buffer, bufsize);
}

bool cFrontendDatabase::LoadUserProfileFromBuffer(void *buffer, int bufsize, int player) {
#ifdef EA_BUILD_A124 // el prototipo carga cada perfil sin copiar los ajustes del segundo jugador
    return CurrentUserProfiles[player]->LoadFromBuffer(buffer, bufsize, true);
#else
    if (player == 0) {
        return CurrentUserProfiles[0]->LoadFromBuffer(buffer, bufsize, true
#ifndef EA_BUILD_A124
                                                      ,
                                                      0
#endif
        );
    } else {
        bool result = CurrentUserProfiles[player]->LoadFromBuffer(buffer, bufsize, false
#ifndef EA_BUILD_A124
                                                                  ,
                                                                  player
#endif
        );
        bMemCpy(&CurrentUserProfiles[0]->GetOptions()->ThePlayerSettings[1], &CurrentUserProfiles[1]->GetOptions()->ThePlayerSettings[0],
                sizeof(PlayerSettings));
        return result;
    }
#endif
}

unsigned int cFrontendDatabase::GetChallengeHeaderHash(unsigned int series) {
    return FEngHashString("CHALLENGE_SERIES_DESC_%02d_HEADER", series);
}

unsigned int cFrontendDatabase::GetChallengeDescHash(unsigned int series) {
    return FEngHashString("CHALLENGE_SERIES_DESC_%02d", series);
}

uint32 cFrontendDatabase::GetBountyIconHash(uint32 hal_id) {
#ifdef EA_BUILD_A124 // el prototipo usa el icono generico de recompensa
    return 0x021a4b0c;
#else
    if (hal_id < 5) {
        return 0x8A21B882;
    }
    if (hal_id < 9) {
        return 0x895EC0AE;
    }
    return 0x9129E7FB;
#endif
}

unsigned int cFrontendDatabase::GetBountyHeaderHash(unsigned int spawn_point) {
    return FEngHashString("BLACKLIST_BOUNTY_SPAWN_POINT_%02d", spawn_point);
}

unsigned int cFrontendDatabase::GetBountyDescHash(unsigned int spawn_point) {
    return FEngHashString("BLACKLIST_BOUNTY_SPAWN_POINT_%02d_DESC", spawn_point);
}

uint32 cFrontendDatabase::GetMilestoneHeaderHash(uint32 hal_id) {
    return FEngHashString("BLACKLIST_PURSUIT_MILESTONES_%02d_SHORT", hal_id);
}

uint32 cFrontendDatabase::GetMilestoneDescHash(uint32 hal_id) {
    return FEngHashString("BLACKLIST_PURSUIT_MILESTONES_%02d", hal_id);
}

unsigned int cFrontendDatabase::GetMilestoneIconHash(unsigned int type, bool bMilestone) {
    unsigned int hash = 0;

    switch (type) {
    case 0x850A64BC:
        hash = 0x88E8DE9E; // COPS_DAMAGED_ICON
        break;
    case 0x3FD1884D:
    case 0x4FC942CA:
        hash = 0x0FE608E6; // COPS_TAKENOUT_ICON
        break;
    case 0xFD989A3A:
        hash = 0x87807869; // MILESTONE_COST_TO_STATE
        break;
    case 0x1334DAE6:
    case 0x15E88693:
    case 0x1BF724E1:
    case 0x20F1AEF3:
    case 0x23B1BF0E:
    case 0x2CB7CAF4:
    case 0x411B084E:
    case 0x4D9777F1:
    case 0x7457EED4:
    case 0x755F7845:
    case 0x8ED622AD:
    case 0xABDF316E:
    case 0xC8993341:
    case 0xE9A4423C:
        hash = 0;
        break;
    case 0x5392E4FD:
        hash = 0x831B7EBE; // MILESTONE_PURSUIT_MAXTIME
        break;
    case 0x033FA23A:
        hash = 0x8C76CD0F; // MILESTONE_PURSUIT_DURATION
        if (bMilestone) {
            hash = 0x950FCEBC; // MILESTONE_PURSUIT_MINTIME
        }
        break;
    case 0xEB45F99D:
        hash = 0xC43959D2; // MILESTONE_ROADBLOCKS
        break;
    case 0x9E3EBB78:
        hash = 0x3FFE9EC9;
        break;
    case 0xCDF36FC2:
        hash = 0xE621B2EF; // MILESTONE_SPIKESTRIPS
        break;
    case 0xA61CAC24:
        hash = 0x6784A80E; // MILESTONE_INFRACTIONS
        break;
    case 0x2377E50D:
        hash = 0xB4E6456B; // MILESTONE_PURSUITBOUNTY
        break;
    // Estos cuatro tienen cuerpo vacio en el original: saltan a la etiqueta
    // final y devuelven el hash = 0 del inicializador. De ahi el beqlr extra.
    case 0x254230F5:
    case 0x9201E1F4:
    case 0x9F8E56CE:
    case 0xCA9AFDF0:
        break;
    default:
        hash = 0;
        break;
    }
    return hash;
}

void cFrontendDatabase::SetMilestoneDescriptionString(char *const outputStr, const int milestoneType, float currVal, const float goalVal
#ifndef EA_BUILD_A124
                                                      ,
                                                      const bool showCurrVal
#endif
) const {
    if (
#ifndef EA_BUILD_A124
        showCurrVal &&
#endif
        currVal > goalVal) {
        currVal = goalVal;
    }
    switch (milestoneType) {
        case 0x33fa23a: {
            char currValTimeToPrint[16];
            char goalValTimeToPrint[16];
            Timer currValTimer(currVal);
            currValTimer.PrintToString(currValTimeToPrint, 4);
            Timer goalValTimer(goalVal);
            goalValTimer.PrintToString(goalValTimeToPrint, 4);
#ifndef EA_BUILD_A124
            if (showCurrVal) {
#endif
                bSPrintf(outputStr, "%s/%s", currValTimeToPrint, goalValTimeToPrint);
#ifndef EA_BUILD_A124
            } else {
                bSPrintf(outputStr, "%s", goalValTimeToPrint);
            }
#endif
            break;
        }
        case 0x5392e4fd: {
            float printTime = currVal;
            char currValTimeToPrint[16];
            char goalValTimeToPrint[16];
            if (currVal == 0.0f) {
                IPlayer *player = IPlayer::First(PLAYER_LOCAL);
                if (player) {
                    ISimable *simable = player->GetSimable();
                    if (simable) {
                        IVehicle *vehicle;
                        if (simable->QueryInterface(&vehicle)) {
                            IVehicleAI *vehicleai = vehicle->GetAIVehiclePtr();
                            if (vehicleai) {
                                IPursuit *ipursuit = vehicleai->GetPursuit();
                                if (ipursuit) {
                                    float pursuitElapsedTime = ipursuit->GetPursuitDuration();
                                    float timeRemaining = goalVal - pursuitElapsedTime;
                                    printTime = UMath::Max(0.0f, timeRemaining);
                                }
                            }
                        }
                    }
                }
            }
            Timer currValTimer(printTime);
            currValTimer.PrintToString(currValTimeToPrint, 4);
            Timer goalValTimer(goalVal);
            goalValTimer.PrintToString(goalValTimeToPrint, 4);
#ifndef EA_BUILD_A124
            if (showCurrVal) {
#endif
                bSPrintf(outputStr, "%s/%s", currValTimeToPrint, goalValTimeToPrint);
#ifndef EA_BUILD_A124
            } else {
                bSPrintf(outputStr, "%s", goalValTimeToPrint);
            }
#endif
            break;
        }
        default:
#ifndef EA_BUILD_A124
            if (showCurrVal) {
#endif
                bSPrintf(outputStr, "%$0.0f/%$0.0f", currVal, goalVal);
#ifndef EA_BUILD_A124
            } else {
                bSPrintf(outputStr, "%$0.0f", goalVal);
            }
#endif
            break;
    }
}

bool cFrontendDatabase::IsMilestoneTimeFormat(const int milestoneType) const {
    if (milestoneType == 0x33fa23a || milestoneType == 0x5392e4fd) {
        return true;
    }
    return false;
}

uint32 cFrontendDatabase::GetRaceNameHash(GRace::Type raceType) {
    switch (raceType) {
        case GRace::kRaceType_P2P:
            return 0xb94fd70e;
        case GRace::kRaceType_Circuit:
            return 0x034fa2c1;
        case GRace::kRaceType_Drag:
            return 0x6f547e4c;
        case GRace::kRaceType_Knockout:
            return 0x4930f5fc;
        case GRace::kRaceType_Tollbooth:
            return 0xa15e4505;
        case GRace::kRaceType_SpeedTrap:
            return 0xee1edc76;
        case GRace::kRaceType_Challenge:
            return 0x213cc8d1;
        default:
            return 0x7818f85e;
    }
}

unsigned int cFrontendDatabase::GetRaceIconHash(GRace::Type type) {
    unsigned int hash = 0;

    switch (type) {
    case 0:
        hash = 0x2521E5EB;
        break;
    case 1:
        hash = 0xE9638D3E;
        break;
    case 2:
        hash = 0xAAAB31E9;
        break;
    case 3:
        hash = 0x3A015595;
        break;
    case 4:
        hash = 0x1A091045;
        break;
    case 5:
        hash = 0x66C9A7B6;
        break;
    case 9:
        hash = 0x66C9A7B6;
        break;
    case 10:
        hash = 0x1A091045;
        break;
    }
    return hash;
}

uint32 cFrontendDatabase::GetSafehouseIconHash(const char *safehouseType) {
    uint32 hash = 0;
    if (bStrICmp(safehouseType, "carlot") == 0) {
        hash = 0x4eaee18b;
    } else if (bStrICmp(safehouseType, "safehouse") == 0) {
        hash = 0x0ed39f69;
    } else if (bStrICmp(safehouseType, "customshop") == 0) {
        hash = 0x0cf07089;
    }
    return hash;
}

GameCompletionStats::GameCompletionStats() {
    m_nOverall = 0;
    m_nCareer = 0;
    m_nRapSheetRankings = 0;
    m_nChallenge = 0;
    m_nTotalChallengeRaces = 0;
    m_nCompletedChallengeRaces = 0;
}

GameCompletionStats cFrontendDatabase::GetGameCompletionStats() {
    GameCompletionStats stats;
    float nTotalCareerRaces = 0.0f;
    float nCompletedCareerRaces = 0.0f;
    float nTotalMilestones = static_cast<float>(GManager::Get().GetNumMilestones());
    float nMilestonesAwarded = 0.0f;
    float nTotalRapSheetRankings = 140.0f;
    float nRapSheetRankings = 0.0f;

    for (unsigned int i = 1; i < 17; i++) {
        GRaceBin *pBin = GRaceDatabase::Get().GetBinNumber(i);
        unsigned int nBossRaces = pBin->GetBossRaceCount();
        for (unsigned int j = 0; j < nBossRaces; j++) {
            if (GRaceDatabase::Get().IsCareerRaceComplete(pBin->GetBossRaceHash(j))) {
                nCompletedCareerRaces += 1.0f;
            }
        }
        unsigned int nWorldRaces = pBin->GetWorldRaceCount();
        for (unsigned int j = 0; j < nWorldRaces; j++) {
            if (GRaceDatabase::Get().IsCareerRaceComplete(pBin->GetWorldRaceHash(j))) {
                nCompletedCareerRaces += 1.0f;
            }
        }
        nTotalCareerRaces += static_cast<float>(nBossRaces + nWorldRaces);
    }

    for (unsigned int i = 0; i < GRaceDatabase::Get().GetRaceCount(); i++) {
        GRaceParameters *pParams = GRaceDatabase::Get().GetRaceParameters(i);
#ifndef EA_BUILD_A124 // el prototipo no tiene la prueba de Burger King ni la edicion de coleccionista
        if (bStrCmp(pParams->GetEventID(), GRaceDatabase::Get().GetBurgerKingRace()) != 0) {
            if (GetIsCollectorsEdition() || !pParams->GetIsCollectorsEditionRace()) {
#endif
                if (pParams->GetIsChallengeSeriesRace()) {
                    stats.m_nTotalChallengeRaces++;
                    if (GRaceDatabase::Get().IsQuickRaceComplete(pParams->GetEventHash())) {
                        stats.m_nCompletedChallengeRaces++;
                    }
                }
#ifndef EA_BUILD_A124
            }
        }
#endif
    }

    for (unsigned int i = 0; static_cast<float>(i) < nTotalMilestones; i++) {
        if (GManager::Get().GetMilestone(i)->GetIsAwarded()) {
            nMilestonesAwarded += 1.0f;
        }
    }

    for (unsigned int i = 0; i < 10; i++) {
#ifdef EA_BUILD_A124
        int rankMovement = 15 - FEDatabase->GetMultiplayerProfile(0)->GetHighScores()->CalcPursuitRank(static_cast<ePursuitDetailTypes>(i), true);
        if (rankMovement >= 0) {
            nRapSheetRankings += static_cast<float>(rankMovement);
        }
#else
        int rankMovement = FEDatabase->GetMultiplayerProfile(0)->GetHighScores()->CalcPursuitRank(static_cast<ePursuitDetailTypes>(i), true);
        if (15 - rankMovement >= 0) {
            nRapSheetRankings += static_cast<float>(15 - rankMovement);
        }
#endif
    }

    stats.m_nRapSheetRankings = static_cast<unsigned char>(nRapSheetRankings / nTotalRapSheetRankings * 100.0f);
    stats.m_nCareer = static_cast<unsigned char>((nCompletedCareerRaces + nMilestonesAwarded) / (nTotalCareerRaces + nTotalMilestones) * 100.0f);
#ifdef EA_BUILD_A124 // el prototipo calcula el porcentaje de desafios en enteros
    stats.m_nChallenge = stats.m_nCompletedChallengeRaces / stats.m_nTotalChallengeRaces * 100;
#else
    stats.m_nChallenge =
        static_cast<unsigned char>(static_cast<float>(stats.m_nCompletedChallengeRaces) / static_cast<float>(stats.m_nTotalChallengeRaces) * 100.0f);
#endif
#ifdef EA_PLATFORM_PLAYSTATION2 // las dos PS2 ponderan en doble precision (litodp/dpmul/dpadd/dptoul en su asm)
    stats.m_nOverall = static_cast<unsigned char>(stats.m_nCareer * 0.7 + stats.m_nChallenge * 0.2 + stats.m_nRapSheetRankings * 0.1);
#else
    stats.m_nOverall = static_cast<unsigned char>(static_cast<float>(static_cast<int>(stats.m_nCareer)) * 0.7f +
                                                  static_cast<float>(static_cast<int>(stats.m_nChallenge)) * 0.2f +
                                                  static_cast<float>(static_cast<int>(stats.m_nRapSheetRankings)) * 0.1f);
#endif

    return stats;
}

void InitFrontendDatabase() {
#ifdef EA_PLATFORM_PLAYSTATION2
    FEDatabase = new ("cFrontendDatabase", 0) cFrontendDatabase();
#else
    FEDatabase = new (GetVirtualMemoryAllocParams()) cFrontendDatabase();
#endif
    FEDatabase->Default();
}

int GetMikeMannBuild() {
    return MikeMannBuild;
}

#ifdef EA_PLATFORM_WIN32
unsigned char GetIsCollectorsEdition() {
#else
int GetIsCollectorsEdition() {
#endif
    return IsCollectorsEdition;
}
