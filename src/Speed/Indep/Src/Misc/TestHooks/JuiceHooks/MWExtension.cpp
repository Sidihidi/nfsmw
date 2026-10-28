#include "Speed/Indep/Src/Misc/TestHooks/JuiceHooks/JuiceHooks.h"
#include "Speed/Indep/Src/Misc/BuildRegion.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/FEng/FEObject.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/AI/aireflectedtypes.h"
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Interfaces/Simables/IRigidBody.h"
#include "Speed/Indep/Src/World/TrackStreamer.hpp"
#ifdef EA_PLATFORM_XENON
#include "Speed/Indep/Src/Interfaces/Simables/IAI.h"
#include "Speed/Indep/Src/Interfaces/Simables/IVehicle.h"
#endif
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/bWare/Inc/bMemory.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/file/driver.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system/threads.h"
#include "Speed/Indep/Libs/realcore/6.24.00/include/common/realcore/system/systask.h"

extern int BuildVersionChangelistNumber;
extern int ForceJuiceConnect;
extern char *ForceJuiceConnectIP;
extern int GetJoylogChannelRepeatCount(int channel);
extern int ASYNCFILE_getstatus(int handle);
extern int bStrCmp(const char *a, const char *b);
extern void LoadNetworkIRXModulesPart2();
extern bool bIsDebuggerConnected();
extern void WriteJoylogFileHeader();
extern float PreviousGpuFrameRate;
extern float fpsTolerateValue;
// El TU de la A124 los DEFINE (MWExtension.cpp:195 del original, .data 0x617d34/0x617d38) y el
// nuestro solo los declaraba: el $I$ de zMisc inicializa countDown con el valor de logCountDownMax
// (`lw`/`sw` sobre los dos) y sin las definiciones no habia codigo que emitir. El valor es el de
// la imagen .data del objeto original (838860800); countDown se inicializa con el, en dinamico.
int logCountDownMax = 838860800;
int countDown = logCountDownMax;

bool JuicePutStringFunction(int terminal_channel, const char *s) {
    Juice::GameHook *gameHook =
        Juice::GameHook::Instance();
    gameHook->LogText(s);
    return true;
}

void InitJuice() {
    LoadNetworkIRXModulesPart2();
    bool debuggerConnected = bIsDebuggerConnected();
    if (debuggerConnected && ForceJuiceConnect == 0) {
        return;
    }
    gJuiceEnabled = 1;
    Juice::MWExtension::Instance();
    Juice::JuiceDirtyNet::Instance();
    Juice::GameHook::Instance()->Initialize();
    Juice::GameHook::Instance()->InitTrapHandler();
    WriteJoylogFileHeader();
}

int DoPolyCount() {
    int total = GetPolyCount(POLY_COUNT_MAIN_CAR);
    total += GetPolyCount(POLY_COUNT_REFLECTION_CAR);
    total += GetPolyCount(POLY_COUNT_RVM_CAR);
    total += GetPolyCount(POLY_COUNT_ENVMAP_SCENERY);
    total += GetPolyCount(POLY_COUNT_MAIN_SCENERY);
    total += GetPolyCount(POLY_COUNT_REFLECTION_SCENERY);
    total += GetPolyCount(POLY_COUNT_RVM_SCENERY);
    total += GetPolyCount(POLY_COUNT_MAIN_WORLDMODEL);
    total += GetPolyCount(POLY_COUNT_REFLECTION_WORLDMODEL);
    total += GetPolyCount(POLY_COUNT_RVM_WORLDMODEL);
    return total;
}

namespace Juice {

char MWExtension::mFileName[256];
char MWExtension::mJuiceBuildName[32];


MWExtension *MWExtension::Instance() {
    static MWExtension mwExt;
    return &mwExt;
}

MWExtension::MWExtension()
    : mHasExecutedRPC(0)
    , mScreenShotHandle(0) {
    bMemSet(mFileName, '\0', 0x100);
}

void MWExtension::ThreadYield(int dur) {
    THREAD_yield(dur);
}

char *MWExtension::GetTitleName() {
    return "NFS Most Wanted";
}

char *MWExtension::GetBuildDate() {
    return "Sep 20 2005";
}

char *MWExtension::GetChangeList() {
    static char changeList[32];
    bSPrintf(changeList, "%d", BuildVersionChangelistNumber);
    return changeList;
}

char *MWExtension::GetConsoleName() {
    static char name[255];
    bMemSet(name, '\0', 0xff);
    return "Default PS2";
}

int MWExtension::GetServerPort() {
    return 0x7919;
}

int MWExtension::GetReadyToReset() {
    g_pEAXSound->RestoreDriver();
    return 0;
}

int MWExtension::IsOkToConnect() {
    return bStrCmp(JuiceDirtyNet::Instance()->GetLocalIpAddress(), "0.0.0.0") != 0;
}

void MWExtension::FileSyncUpdate() {
    SYNCTASK_run();
}

int MWExtension::HasFileLoaded(const void *fileHandle) {
    return static_cast<int>(ASYNCFILE_getstatus(reinterpret_cast<int>(fileHandle)) == 1);
}

char *MWExtension::GetServerIP() {
    char *result;
    if ((ForceJuiceConnect == 0) ||
        (result = ForceJuiceConnectIP, *ForceJuiceConnectIP == '\0')) {
        result = "10.10.229.117";
    }
    return result;
}

int MWExtension::GetBackupServerPort() {
    return 0x7919;
}

char *MWExtension::GetBackupServerIP() {
    return "10.10.229.117";
}

char *MWExtension::GetXboxPath() {
    return "xe:\\CDMW\\";
}

char *MWExtension::GetXboxExeName() {
    return "nfsMWMilestone.exe";
}

char *MWExtension::GetConsoleTarget() {
    return "nfsMWMilestone.elf";
}

unsigned int MWExtension::GetIntializationTimeOut() {
    return 30000;
}

unsigned int MWExtension::GetHeartBeatTimeOut() {
    return 20000;
}

float MWExtension::GetGameTime() {
    return 0.0f;
}

void MWExtension::InstantiateScriptExt() {}

int MWExtension::NetworkInitRetries() {
    return 100;
}

int MWExtension::InitialConnectionRetries() {
    return 100;
}

int MWExtension::WaitBetweenInitialConnectionRretries() {
    return 200;
}

int MWExtension::WaitBetweenNetworkInitRetries() {
    return 0x96;
}

char *MWExtension::GetPopupName() {
    return "null";
}

char *MWExtension::GetPopupCheck() {
    return "false";
}

int MWExtension::GetJuiceUpdateProcessor() {
    return 0;
}

void MWExtension::ResetGamePad() {
    MWJuicePad::Instance()->ResetGamePad();
}

void MWExtension::GamePadToJuicePad(void *gameInput, tJuicePad *juiceInput) {
    MWJuicePad::Instance()->PollInput();
}

void MWExtension::JuicePadToGamePad(tJuicePad *juiceInput, void *gameInput) {
    if (mHasExecutedRPC == 1) {
        MWJuicePad::Instance()->ReleaseAllButtons(JUICE_NORMAL);
        mHasExecutedRPC = 0;
    }
    if (JuicePad::Instance()->DidInputHappen() == 1) {
        mHasExecutedRPC = 1;
        if (JuicePad::Instance()->IsButtonPressed(JUICEPAD_DPADUP) != 0) {
            MWJuicePad::Instance()->PressButton(0, JUICE_UP);
        }
        if (JuicePad::Instance()->IsButtonPressed(JUICEPAD_DPADDOWN) != 0) {
            MWJuicePad::Instance()->PressButton(0, JUICE_DOWN);
        }
        if (JuicePad::Instance()->IsButtonPressed(JUICEPAD_DPADLEFT) != 0) {
            MWJuicePad::Instance()->PressButton(0, JUICE_LEFT);
        }
        if (JuicePad::Instance()->IsButtonPressed(JUICEPAD_DPADRIGHT) != 0) {
            MWJuicePad::Instance()->PressButton(0, JUICE_RIGHT);
        }
        if (JuicePad::Instance()->IsButtonPressed(JUICEPAD_BUTTON1) != 0) {
            MWJuicePad::Instance()->PressButton(0, JUICE_ACCEPT);
        }
        if (JuicePad::Instance()->IsButtonPressed(JUICEPAD_BUTTON2) != 0) {
            MWJuicePad::Instance()->PressButton(0, JUICE_CANCEL);
        }
        if (JuicePad::Instance()->IsButtonPressed(JUICEPAD_BUTTON4) != 0) {
            MWJuicePad::Instance()->PressButton(0, JUICE_Y);
        }
        if (JuicePad::Instance()->IsButtonPressed(JUICEPAD_BUTTON3) != 0) {
            MWJuicePad::Instance()->PressButton(0, JUICE_X);
        }
        if (JuicePad::Instance()->IsButtonPressed(JUICEPAD_BUTTONL1) != 0) {
            MWJuicePad::Instance()->PressButton(0, JUICE_L1);
        }
        if (JuicePad::Instance()->IsButtonPressed(JUICEPAD_BUTTONL2) != 0) {
            MWJuicePad::Instance()->PressButton(0, JUICE_WHITE);
        }
        if (JuicePad::Instance()->IsButtonPressed(JUICEPAD_BUTTONL3) != 0) {
            MWJuicePad::Instance()->PressButton(0, JUICE_LSTICKBUTTON);
        }
        if (JuicePad::Instance()->IsButtonPressed(JUICEPAD_BUTTONR1) != 0) {
            MWJuicePad::Instance()->PressButton(0, JUICE_R1);
        }
        if (JuicePad::Instance()->IsButtonPressed(JUICEPAD_BUTTONR2) != 0) {
            MWJuicePad::Instance()->PressButton(0, JUICE_BLACK);
        }
        if (JuicePad::Instance()->IsButtonPressed(JUICEPAD_BUTTONR3) != 0) {
            MWJuicePad::Instance()->PressButton(0, JUICE_RSTICKBUTTON);
        }
        if (JuicePad::Instance()->IsButtonPressed(JUICEPAD_BUTTONSTART) != 0) {
            MWJuicePad::Instance()->PressButton(0, JUICE_START);
        }
        if (JuicePad::Instance()->IsButtonPressed(JUICEPAD_BUTTONSELECT) != 0) {
            MWJuicePad::Instance()->PressButton(0, JUICE_BACK);
            return;
        }
    }
}

void MWExtension::UpdatePad(void *input) {
    MWJuicePad::Instance()->ReleaseSegmentPresses();
    tJuicePad *juicePad = JuicePad::Instance()->GetMasterPad();
    JuicePad::Instance()->UpdateJuicePad();

    IExtension::sCurrentExtension->JuicePadToGamePad(juicePad, input);
    IExtension::sCurrentExtension->GamePadToJuicePad(input, juicePad);

    static int oldFrame;
    if (PadConfigManager::Instance()->IsFENavCapturing() &&
        JuicePad::Instance()->DidInputHappen() == 1) {
        int currentFrame = GameHook::Instance()->GetFrame();
        int frameDelta = currentFrame - oldFrame;
        if (juicePad->mButtons != 0 && frameDelta > 0xe) {
            oldFrame = currentFrame;
            PadConfigManager::Instance()->CaptureKeyAndState(juicePad->mButtons);
        }
    }
    JuicePad::Instance()->UpdateJuicePad();
    JuicePad::Instance()->ResetButtons();
}

int MWExtension::NumberOfRepeatedReplayEntries(int channel) {
    return GetJoylogChannelRepeatCount(channel);
}

char *MWExtension::GetBuildName() {
    return GetJuiceBuildName();
}

char *MWExtension::GetImageName() {
    return "NFS_MW_Map";
}

int MWExtension::HangTimeoutLength() {
    return 65000;
}

char *MWExtension::GetJuiceBuildName() {
    if (ForceJuiceConnect != 0) {
        return "TestBuild";
    }
    if (BuildRegion::IsPal()) {
        bSPrintf(mJuiceBuildName, "%d-PAL", BuildVersionChangelistNumber);
    } else {
        bSPrintf(mJuiceBuildName, "%d", BuildVersionChangelistNumber);
    }
    return mJuiceBuildName;
}

char *MWExtension::GetScreenName() {
    static char screenName[128];
    bMemSet(screenName, '\0', 0x80);
    FEPackage *package = cFEng::Get()->FindPackageWithControl();
    if (package != nullptr) {
        package = cFEng::Get()->FindPackageWithControl();
        bStrCpy(screenName, package->GetName());
    }
    return screenName;
}

char *MWExtension::GetCursorPos() {
    static char cursor_pos[32];
    bMemSet(cursor_pos, '\0', 0x20);
    FEPackage *package = cFEng::Get()->FindPackageWithControl();
    if (package != nullptr) {
        FEObject *button = package->GetCurrentButton();
        if (button != nullptr) {
            bSPrintf(cursor_pos, "%d", button->NameHash);
            return cursor_pos;
        }
    }
    return "null";
}

char *MWExtension::GetPlayerPosition() {
    return GetPlayerPosition(IPlayer::First(PLAYER_LOCAL));
}

char *MWExtension::GetPlayerPosition(IPlayer *player) {
    if (player != nullptr) {
        static char coords[128];
        UMath::Vector3 vecCoords = player->GetPosition();
        bSPrintf(coords, "%f;%f;%f", vecCoords.z, -vecCoords.x, vecCoords.y);
        return coords;
    }
    if (IPlayer::First(PLAYER_LOCAL) != nullptr) {
        static char coords[128];
        UMath::Vector3 vecCoords = IPlayer::First(PLAYER_LOCAL)->GetPosition();
        bSPrintf(coords, "%f;%f;%f", vecCoords.z, -vecCoords.x, vecCoords.y);
        return coords;
    }
    return "0.0;0.0;0.0";
}

char *MWExtension::GetFormationStr(const FormationType &formation) {
    switch (formation) {
    case PIT:
        return "PIT";
    case BOX_IN:
        return "BOX_IN";
    case ROLLING_BLOCK:
        return "ROLLING_BLOCK";
    case FOLLOW:
        return "FOLLOW";
    case HELI_PURSUIT:
        return "HELI_PURSUIT";
    case HERD:
        return "HERD";
    default:
        return "NONE";
    }
}

char *MWExtension::GetCurrentRaceType() {
    GRace::Type raceType = GRaceStatus::Get().GetRaceType();
    switch (raceType + GRace::kRaceType_Circuit) {
    case 0:
        return "None";
    case 1:
        return "P2P";
    case 2:
        return "Circuit";
    case 3:
        return "Drag";
    case 4:
        return "Knockout";
    case 5:
        return "Tollbooth";
    case 6:
        return "SpeedTrap";
    case 7:
        return "Checkpoint";
    case 8:
        return "CashGrab";
    case 9:
        return "Challenge";
    default:
        return "Unknown";
    }
}

char *MWExtension::GetCurrentZoneName() {
    return TheTrackStreamer.GetCurrentZoneName();
}

#ifdef EA_PLATFORM_XENON
// X360 Milestone (EGM 0x823E0230): otra version del registro. Cuenta tambien las persecuciones y los coches de policia
// ("PUR_%d - CAR_%d", con su evento PURSUIT-FPS), el evento detallado se llama DETAILED-FPS, el printf no lleva el
// numero de frames y el bloque SUST escribe fpsTolerateValue. Los contadores se leen dentro de cada bloque (el EGM los
// vuelve a leer en el segundo). La rama sin poligonos pasa cinco argumentos a un formato de tres: es lo que hace el EGM.
void MWExtension::JuiceLowFrameRateLog(const float &fps) {
    if (fps < fpsTolerateValue) {
        char *zoneName = TheTrackStreamer.GetCurrentZoneName();
        char *tempImgName = GetImageName();
        char *tempPlayerPos = GetPlayerPosition();
        char outcomeString[32];

        if (countDown == logCountDownMax) {
            int polyCount = DoPolyCount();
            int rigidBodyCount = IRigidBody::GetList().size();
            int pursuitCount = IPursuit::GetList().size();
            int copCount = IVehicle::Count(VEHICLE_AICOPS);

            bSPrintf(outcomeString, "INST_%.0f", fps);
            bReleasePrintf("FRAME RATE: (%s) %s\n", zoneName, outcomeString);
            GameHook::Instance()->GameEvent(
                "WORLD", "FPS", outcomeString, static_cast<int>(fps), fps, zoneName, tempImgName, tempPlayerPos);

            if (polyCount != 0) {
                bSPrintf(outcomeString, "INST_%.0f {GPU_%.0f - SIM_%d - POLY_%d}", fps, PreviousGpuFrameRate,
                         rigidBodyCount, polyCount);
                GameHook::Instance()->GameEvent(
                    "WORLD", "DETAILED-FPS", outcomeString, rigidBodyCount, fps, zoneName, tempImgName, tempPlayerPos);
            } else {
                bSPrintf(outcomeString, "INST_%.0f {GPU_%.0f - SIM_%d}", fps, PreviousGpuFrameRate, rigidBodyCount,
                         pursuitCount, copCount);
                GameHook::Instance()->GameEvent(
                    "WORLD", "DETAILED-FPS", outcomeString, rigidBodyCount, fps, zoneName, tempImgName, tempPlayerPos);
            }

            if (pursuitCount != 0) {
                bSPrintf(outcomeString, "INST_%.0f {PUR_%d - CAR_%d}", fps, pursuitCount, copCount);
                GameHook::Instance()->GameEvent(
                    "WORLD", "PURSUIT-FPS", outcomeString, rigidBodyCount, fps, zoneName, tempImgName, tempPlayerPos);
            }
        }

        if (countDown <= 0) {
            countDown = logCountDownMax - 1;
            int polyCount = DoPolyCount();
            int rigidBodyCount = IRigidBody::GetList().size();
            int pursuitCount = IPursuit::GetList().size();
            int copCount = IVehicle::Count(VEHICLE_AICOPS);

            bSPrintf(outcomeString, "SUST_%.0f", fpsTolerateValue);
            bReleasePrintf("FRAME RATE: (%s) %s\n", zoneName, outcomeString);
            GameHook::Instance()->GameEvent("WORLD", "FPS", outcomeString, static_cast<int>(fpsTolerateValue),
                                            fpsTolerateValue, zoneName, tempImgName, tempPlayerPos);

            if (polyCount != 0) {
                bSPrintf(outcomeString, "SUST_%.0f {GPU_%.0f - SIM_%d - POLY_%d}", fpsTolerateValue,
                         PreviousGpuFrameRate, rigidBodyCount, polyCount);
                GameHook::Instance()->GameEvent("WORLD", "DETAILED-FPS", outcomeString, rigidBodyCount,
                                                fpsTolerateValue, zoneName, tempImgName, tempPlayerPos);
            } else {
                bSPrintf(outcomeString, "SUST_%.0f {GPU_%.0f - SIM_%d}", fpsTolerateValue, PreviousGpuFrameRate,
                         rigidBodyCount, pursuitCount, copCount);
                GameHook::Instance()->GameEvent("WORLD", "DETAILED-FPS", outcomeString, rigidBodyCount,
                                                fpsTolerateValue, zoneName, tempImgName, tempPlayerPos);
            }

            if (pursuitCount != 0) {
                bSPrintf(outcomeString, "SUST_%.0f {PUR_%d - CAR_%d}", fpsTolerateValue, pursuitCount, copCount);
                GameHook::Instance()->GameEvent("WORLD", "PURSUIT-FPS", outcomeString, rigidBodyCount,
                                                fpsTolerateValue, zoneName, tempImgName, tempPlayerPos);
            }
        } else {
            countDown--;
        }
    } else {
        countDown = logCountDownMax;
    }
}
#else
void MWExtension::JuiceLowFrameRateLog(const float &fps) {
    if (fps < fpsTolerateValue) {
        char *zoneName = TheTrackStreamer.GetCurrentZoneName();
        char *tempImgName = GetImageName();
        char *tempPlayerPos = GetPlayerPosition();
        int rigidBodyCount = IRigidBody::GetList().size();
        char outcomeString[32];

        if (countDown == logCountDownMax) {
            bSPrintf(outcomeString, "INST_%.0f", fps);
            bReleasePrintf("FRAME RATE: (%s) %s", zoneName, outcomeString);
            GameHook::Instance()->GameEvent(
                "WORLD", "FPS", outcomeString, static_cast<int>(fps), fps, zoneName, tempImgName, tempPlayerPos);

            int polyCount = DoPolyCount();
            if (polyCount != 0) {
                bSPrintf(outcomeString, "INS_%.0f {G_%.0f - S_%d - P_%d}", fps, PreviousGpuFrameRate,
                        rigidBodyCount, polyCount);
            } else {
                bSPrintf(outcomeString, "INS_%.0f {G_%.0f - S_%d", fps, PreviousGpuFrameRate,
                        rigidBodyCount);
            }
            GameHook::Instance()->GameEvent(
                "WORLD", "FPS_D", outcomeString, rigidBodyCount, fps, zoneName, tempImgName, tempPlayerPos);
        }

        if (countDown <= 0) {
            countDown = logCountDownMax - 1;
            bSPrintf(outcomeString, "SUST_%.0f", fpsTolerateValue);
            bReleasePrintf("FRAME RATE: (%s) %s for %d frames", zoneName, outcomeString, logCountDownMax);
            GameHook::Instance()->GameEvent(
                "WORLD", "FPS", outcomeString, static_cast<int>(fpsTolerateValue), fpsTolerateValue,
                zoneName, tempImgName, tempPlayerPos);

            int polyCount = DoPolyCount();
            if (polyCount != 0) {
                bSPrintf(outcomeString, "SUS_%.0f {G_%.0f - S_%d - P_%d}", fps, PreviousGpuFrameRate,
                        rigidBodyCount, polyCount);
            } else {
                bSPrintf(outcomeString, "SUS_%.0f {G_%.0f - S_%d", fps, PreviousGpuFrameRate,
                        rigidBodyCount);
            }
            GameHook::Instance()->GameEvent(
                "WORLD", "FPS_D", outcomeString, rigidBodyCount, fps, zoneName, tempImgName, tempPlayerPos);
        } else {
            countDown--;
        }
    } else {
        countDown = logCountDownMax;
    }
}
#endif

void MWExtension::JuiceScreenshot(char *fileName) {
    GameHook::Instance()
        ->LogText("[MW SS] - Calling Juice screenshot");
}

}
