#include "Speed/Indep/Src/Frontend/NFSDebugMenu.h"

#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/Src/Camera/Camera.hpp"
#include "Speed/Indep/Src/Ecstasy/Ecstasy.hpp"
#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/Indep/Src/Frontend/Careers/UnlockSystem.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Frontend/Database/VehicleDB.hpp"
#include "Speed/Indep/Src/Frontend/DebugMenu.h"
#include "Speed/Indep/Src/Frontend/DebugMenuWidgets.h"
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/Localization/Localize.hpp"
#include "Speed/Indep/Src/Frontend/MemoryCard/MemoryCard.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Common/FEAnyMovieScreen.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/Safehouse/customize/CarCustomize.hpp"
#include "Speed/Indep/Src/Gameplay/GManager.h"
#include "Speed/Indep/Src/Gameplay/GMilestone.h"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Gameplay/GSpeedTrap.h"
#include "Speed/Indep/Src/Generated/AttribSys/GenericAccessor.h"
#include "Speed/Indep/Src/Generated/Events/EAddSMS.hpp"
#include "Speed/Indep/Src/Generated/Events/EEnterBin.hpp"
#include "Speed/Indep/Src/Generated/Events/EFadeScreenOff.hpp"
#include "Speed/Indep/Src/Generated/Events/EFadeScreenOn.hpp"
#include "Speed/Indep/Src/Input/ActionQueue.h"
#include "Speed/Indep/Src/Interfaces/SimActivities/ICopMgr.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Interfaces/Simables/IAI.h"
#include "Speed/Indep/Src/Interfaces/Simables/IRigidBody.h"
#include "Speed/Indep/Src/Interfaces/Simables/IVehicle.h"
#include "Speed/Indep/Src/Lua/LuaGameHooks.h"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Sim/Simulation.h"
#include "Speed/Indep/Src/World/Scenery.hpp"
#include "Speed/Indep/Src/World/TimeOfDay.hpp"
#include "Speed/Indep/Src/World/VisualTreatment.h"
#include "Speed/Indep/Src/World/WCollisionMgr.h"
#ifdef EA_PLATFORM_XENON
#include "Speed/Indep/Src/Misc/Timer.hpp"
#endif
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bFunk.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/bWare/Inc/bPrintf.hpp"

// Tweakables of other units that no header declares; the debug menu binds them. The type of each one is the one
// of its definition in the A124.
extern int32 TakeSingleScreenshot;                       // zEcstasy
extern int SaveHotPosition;                              // zWorld
extern int JumpToHotPosition;                            // zWorld
extern uint32 SeeulatorToolActive;                       // zWorld (Scenery)
extern int EnableJoystickMonkey;                         // zMisc
extern bool bSendRaceLengthToBigBang;                    // zGameplay
extern bool TWEAK_ShowGameplayMilestoneValues;           // zGameplay (GManager)
extern bool CameraDebugWatchCar;                         // zCamera (CameraMover)
extern int mToggleCar;                                   // zCamera (CDActionDebugWatchCar)
extern eVehicleList mToggleCarList;                      // zCamera (CDActionDebugWatchCar)
extern bool bToggleAiControl;                            // zAI (AIVehicle)
extern FormationType DebugCopFormation;                  // zAI (AICopManager)
extern float Tweak_TuningAero;                           // zPhysics (PVehicle)
extern bool Tweak_UseTweakerTunings;                     // zPhysics (PVehicle)
extern int DrawFEng;                                     // zFe (FEManager)
extern int FEngMemoryPoolNumber;                         // zFEng (FEngStandard)
#ifdef EA_PLATFORM_XENON
extern float ForcePursuitHeatLevel;                      // zAI (float en X360: EGM 0x82A80C60)
#else
extern int ForcePursuitHeatLevel;                        // zAI
#endif
extern bool RenderLookAtPoint;                           // zFeOverlay (FEPkg_GarageMain)
extern bool gToggleLocTracking;                          // zFe2 (Localize)
extern bool gbShowFreeIOPMem;                            // zMisc
extern bool IsMovieTimerPrintf;                          // zFe (MoviePlayer)
extern bool FMVScreenPrintf;                             // zFe (MoviePlayer)
extern bool MinimapShowNonPursuitCops;                   // zFe2 (FeMinimap)
extern bool MinimapShowPursuitCops;                      // zFe2 (FeMinimap)
extern bool gHideGPSArrow;                               // zAI
extern bool CopFormationOverriden;                       // zAI
extern bool bAiRandomTurns;                              // zWorld (WRoadNetwork)
extern bool RBVehicle_DisableVehicleVsVehicleCollisions; // zPhysics (RBVehicle)
extern bool Tweak_InfiniteNOS;                           // zPhysics (EngineRacer)
extern bool Tweak_InfiniteRaceBreaker;                   // zSim (LocalPlayer)
extern int DisplayCarLoaderMemoryOnScreen;               // zWorld (CarLoader)
extern float kMaxLatency;                                // zWorld
extern float kBlendMult;                                 // zWorld
extern float kMinBlendTime;                              // zWorld
extern float kMaxBlendTime;                              // zWorld
extern bool kUseDriverAI;                                // zWorld
extern bool kSpamPhysics;                                // zWorld
extern eTimeOfDay gTheTimeOfDay;                         // zWorld (TimeOfDay)
extern int Chance100;                                    // zWorld (rain)
extern int gApplyMWVisualLook;                           // zEcstasy
extern bool Tweak_barrierDraw;                           // zWorld
extern bool Tweak_triggerDraw;                           // zWorld
extern bool Tweak_objectDraw;                            // zWorld
extern int32 DrawWorld;                                  // zEcstasy
extern int32 DrawCars;                                   // zWorld (CarRender)
extern int ScreenShotSampleRez;                          // zEcstasy
extern int bEnableNisTextDisplay;                        // zAnim (AnimScene)

bool Tweak_ToggleSetReputation = false;
int Tweak_SetReputationRace = 0;
int Tweak_SetReputationMilestones = 0;
int Tweak_SetReputationPursuit = 0;
int BountyBoost = 2;
int CareerSMSID = 10;
bool LanguageCyclingEnabled = false;
bool dump_loc_tracking = false;
bool show_sms = false;
float gHeat = 4.0f;
bool gCopAIDisabled = !ICopMgr::AreCopsEnabled();
static bool gAutoPilot = false;
static int gPursuitTarget = 0;

void ExitDebugMenu() {
    gDebugMenuIOHandler->Exit();
}

bool gRigidBodyVol = false;

// The A124 expands two calls to an inline of Debugable.h (line 53) that compile to nothing.
void TriggerRigidBodyVol() {}

bool gOnlineDbg = false;

void TriggerOnlineDbg() {}

void SetPrecullerModeOff() {
    SetPrecullerMode(PRECULLER_MODE_OFF);
}

void SetPrecullerModeOn() {
    SetPrecullerMode(PRECULLER_MODE_ON);
}

void SetPrecullerModeBlinking() {
    SetPrecullerMode(PRECULLER_MODE_BLINKING);
}

void SetPrecullerModeHighSpeed() {
    SetPrecullerMode(PRECULLER_MODE_HIGH_SPEED);
}

StringListParam PrecullerModeParams[4] = {
    {"Off", SetPrecullerModeOff},
    {"On", SetPrecullerModeOn},
    {"Blinking", SetPrecullerModeBlinking},
    {"HighSpeed", SetPrecullerModeHighSpeed},
};

int NOSLevel = 0;

void set_nos_level_callback() {}

int RenderingNOSLevel = 0;

void set_nosforrendering_callback() {}

int gFinishPlace = 1;
float gFinishTime = 1.0f;
int playerCash = 0;

void SetCash() {
    CareerSettings *career = FEDatabase->GetCareerSettings();

    if (playerCash > 0) {
        career->AwardCash(playerCash);
    } else if (playerCash < 0) {
        career->SpendCash(playerCash);
    }
}

void ShowSMS() {
    for (int i = 0; i < 125; i++) {
        SMSMessage *msg = FEDatabase->GetCareerSettings()->GetSMSMessage(i);
        msg->SetHandle(i);
        msg->ClearFlags();
        if (DoesStringExist(msg->GetMsgHash())) {
            msg->SetFlag(1);
        } else {
            msg->SetHandle(0xFFFF);
        }
    }
}

void DumpLocTracking() {
    DumpMap();
}

int playerBin = 16;

void SetBin() {
    FEDatabase->GetCareerSettings()->SetCurrentBin(playerBin);
}

// The three of them read Tweak_SetReputationRace in the A124.
void SetRaceReputation() {
    if (Tweak_ToggleSetReputation) {
        FECarRecord *fe_car =
            FEDatabase->GetPlayerCarStable(0)->GetCarRecordByHandle(FEDatabase->GetCareerSettings()->GetCurrentCar());
        FECareerRecord *fe_career = FEDatabase->GetPlayerCarStable(0)->GetCareerRecordByHandle(fe_car->CareerHandle);

        if (Tweak_SetReputationRace != fe_career->GetBounty()) {
            fe_career->TweakBounty(Tweak_SetReputationRace);
        }
    }
}

void SetMilestonesReputation() {
    if (Tweak_ToggleSetReputation) {
        FECarRecord *fe_car =
            FEDatabase->GetPlayerCarStable(0)->GetCarRecordByHandle(FEDatabase->GetCareerSettings()->GetCurrentCar());
        FECareerRecord *fe_career = FEDatabase->GetPlayerCarStable(0)->GetCareerRecordByHandle(fe_car->CareerHandle);

        if (Tweak_SetReputationRace != fe_career->GetBounty()) {
            fe_career->TweakBounty(Tweak_SetReputationRace);
        }
    }
}

void SetPursuitReputation() {
    if (Tweak_ToggleSetReputation) {
        FECarRecord *fe_car =
            FEDatabase->GetPlayerCarStable(0)->GetCarRecordByHandle(FEDatabase->GetCareerSettings()->GetCurrentCar());
        FECareerRecord *fe_career = FEDatabase->GetPlayerCarStable(0)->GetCareerRecordByHandle(fe_car->CareerHandle);

        if (Tweak_SetReputationRace != fe_career->GetBounty()) {
            fe_career->TweakBounty(Tweak_SetReputationRace);
        }
    }
}

void FinishRaceInPlace() {
    if (MemoryCard::GetInstance()->IsAutoSaving()) {
        return;
    }

    gFinishPlace = bMin(gFinishPlace, GRaceStatus::Get().GetRacerCount());

    GRaceStatus::Get().SkipToEndOfRaceInPlace(gFinishPlace, gFinishTime);
}

void ChangeUserCarCallback() {}

void TakeScreenShot() {
    TakeSingleScreenshot = 1;
}

void JumpToNewPos(bVector3 *jumpPosition) {
    UMath::Vector3 newPosition;
    bConvertToBond(newPosition, *jumpPosition);

    IPlayer *iPlayer = IPlayer::First(PLAYER_LOCAL);
    if (iPlayer) {
        Sim::SetStream(newPosition, true);

        float height = jumpPosition->z;
        if (WCollisionMgr(0, 3).GetWorldHeightAtPointRigorous(newPosition, height, nullptr)) {
            newPosition.y = height + 1.0f;
        }

        iPlayer->SetPosition(newPosition);
    }
}

void JumpToRacePos(const char *race) {
    GRaceParameters *parms = GRaceDatabase::Get().GetRaceFromName(race);
    if (!parms) {
        return;
    }

    parms->BlockUntilLoaded();

    UMath::Vector3 startPos;
    parms->GetStartPosition(startPos);
    bConvertFromBond(startPos, startPos);

    bVector3 pos(startPos.x, startPos.y, startPos.z);
    JumpToNewPos(&pos);
}

void SaveHotPos() {
    SaveHotPosition = 1;
}

void JumpToHotPos() {
    JumpToHotPosition = 1;
}

float gCoord_x1;
float gCoord_x2;
float gCoord_x3;
float gCoord_x4;
float gCoord_y1;
float gCoord_y2;
float gCoord_y3;
float gCoord_y4;
float gCoord_z1;
float gCoord_z2;
float gCoord_z3;
float gCoord_z4;

void JumpToCoordPos() {
    bVector3 inPos;

    inPos.x = bAbs(gCoord_x1 * 1000.0f) + bAbs(gCoord_x2 * 100.0f) + bAbs(gCoord_x3 * 10.0f) + bAbs(gCoord_x4);
    if (gCoord_x1 < 0.0f || gCoord_x2 < 0.0f || gCoord_x3 < 0.0f || gCoord_x4 < 0.0f) {
        inPos.x = -inPos.x;
    }

    inPos.y = bAbs(gCoord_y1 * 1000.0f) + bAbs(gCoord_y2 * 100.0f) + bAbs(gCoord_y3 * 10.0f) + bAbs(gCoord_y4);
    if (gCoord_y1 < 0.0f || gCoord_y2 < 0.0f || gCoord_y3 < 0.0f || gCoord_y4 < 0.0f) {
        inPos.y = -inPos.y;
    }

    inPos.z = bAbs(gCoord_z1 * 1000.0f) + bAbs(gCoord_z2 * 100.0f) + bAbs(gCoord_z3 * 10.0f) + bAbs(gCoord_z4);
    if (gCoord_z1 < 0.0f || gCoord_z2 < 0.0f || gCoord_z3 < 0.0f || gCoord_z4 < 0.0f) {
        inPos.z = -inPos.z;
    }

    JumpToNewPos(&inPos);
}

void JumpToCaffeineCursor() {}

void JumpToSeeulatorCursor() {
    if (SeeulatorToolActive) {
        bVector3 v;
        if (bFunkCallSync("Seeulator", 8, nullptr, 0, &v, 16) == 16) {
            JumpToNewPos(&v);
        }
    }
}

void JumpToJR2Camera() {
    if (JR2ServerExists) {
        bMatrix4 m;
        if (bFunkCallSync("JR2Server", 3, nullptr, 0, &m, 64) == 64) {
#ifdef EA_PLATFORM_PLAYSTATION2
            // El original carga m.v3 aqui mismo (linea 670) y la escala con el operador por valor de bMath.hpp.
            bVector3 v = bScale(*reinterpret_cast<bVector3Reg *>(&m.v3), 0.01f);
#else
            bVector3 v = bScale(*reinterpret_cast<bVector3 *>(&m.v3), 0.01f);
#endif
            JumpToNewPos(&v);
        }
    }
}

bVector3 kPosDebugCity(1467.0f, 355.0f, 101.0f);
bVector3 kPosDebugCoastal(4452.0f, 633.0f, 48.0f);
bVector3 kPosDebugCollege(1630.0f, 2180.0f, 336.0f);
bVector3 kPosDebugSafeHouseCity(2313.0f, -70.0f, 94.0f);
bVector3 kPosDebugSafeHouseCollege(1773.0f, 2499.0f, 150.0f);
bVector3 kPosDebugSafeHouseCoastal(4254.0f, 75.0f, 11.0f);
bVector3 kPosTemp(2276.0f, 2629.0f, 140.0f);
bVector3 cityEastParkPos(2085.0f, 141.0f, 98.0f);
bVector3 cityWestParkPos(709.0f, 155.0f, 115.0f);
bVector3 cityStadiumPos(934.0f, -649.0f, 116.0f);
bVector3 cityTimesSquarePos(1452.0f, 360.0f, 101.0f);
bVector3 cityLittleItalyPos(2113.0f, 322.0f, 100.0f);
bVector3 citySubwayEntrance1Pos(2147.0f, 40.0f, 94.0f);
bVector3 citySubwayEntrance2Pos(926.0f, -556.0f, 115.0f);
bVector3 citySubwayEntrance3Pos(1819.0f, 512.0f, 94.0f);
bVector3 cityHighwayNorthPos(1819.0f, 916.0f, 124.0f);
bVector3 cityHighwaySouthPos(1972.0f, -581.0f, 105.0f);
bVector3 cityHighwayEastPos(2515.0f, 291.0f, 93.0f);
bVector3 cityHighwayWestPos(499.0f, 176.0f, 108.0f);
bVector3 cityMuseumPos(885.0f, 455.0f, 113.0f);
bVector3 cityAmphitheatrePos(850.0f, 246.0f, 114.0f);
bVector3 coastalConeyIslandPos(4094.0f, 166.0f, 23.0f);
bVector3 coastalFishMarketPos(4629.0f, 149.0f, 7.0f);
bVector3 coastalOilRefineryPos(3993.0f, 2100.0f, 28.0f);
bVector3 coastalFishingVillagePos(3693.0f, 3516.0f, 26.0f);
bVector3 coastalTrainyardPos(2920.0f, 175.0f, 28.0f);
bVector3 coastalShipyardPos(3107.0f, 490.0f, 18.0f);
bVector3 coastalLighthousePos(4092.0f, -173.0f, 17.0f);
bVector3 coastalDriveInTheatrePos(3144.0f, 1661.0f, 110.0f);
bVector3 coastalGasStation1Pos(3713.0f, 3490.0f, 27.0f);
bVector3 coastalGasStation2Pos(2749.0f, 2159.0f, 108.0f);
bVector3 coastalGasStation3Pos(3071.0f, 1030.0f, 67.0f);
bVector3 coastalGasStation4Pos(3836.0f, 605.0f, 25.0f);
bVector3 coastalTrailerParkPos(2871.0f, 3004.0f, 74.0f);
bVector3 coastalCanneryPos(3401.0f, 2853.0f, 12.0f);
bVector3 coastalFireHallPos(3878.0f, 459.0f, 23.0f);
bVector3 coastalAmusementParkPos(3944.0f, 273.0f, 17.0f);
bVector3 coastalBoardwalkPos(4542.0f, 118.0f, 6.0f);
bVector3 coastalPrisonPos(4093.0f, 1302.0f, 45.0f);
bVector3 collegeRosewoodParkPos(663.0f, 4084.0f, 210.0f);
bVector3 collegeGolfCoursePos(2180.0f, 3591.0f, 165.0f);
bVector3 collegeCampusPos(1087.0f, 3187.0f, 202.0f);
bVector3 collegeMainStreetPos(1606.0f, 2210.0f, 145.0f);
bVector3 collegeBaseballStadiumPos(24.0f, 3239.0f, 195.0f);
bVector3 collegeHighwayNorthPos(1830.0f, 4288.0f, 233.0f);
bVector3 collegeHighwaySouthPos(1798.0f, 2003.0f, 152.0f);
bVector3 collegeHighwayEastPos(2259.0f, 2616.0f, 141.0f);
bVector3 collegeHighwayWestPos(-293.0f, 3633.0f, 207.0f);
bVector3 collegeClubHousePos(2168.0f, 3566.0f, 162.0f);
bVector3 collegeTollBooth1Pos(378.0f, 4502.0f, 236.0f);
bVector3 collegeTollBooth2Pos(1580.0f, 1800.0f, 168.0f);
bVector3 collegeGasStation1Pos(822.0f, 4500.0f, 205.0f);
bVector3 collegeGasStation2Pos(1216.0f, 3667.0f, 201.0f);
bVector3 collegeGasStation3Pos(549.0f, 2622.0f, 167.0f);
bVector3 collegeGasStation4Pos(1641.0f, 2486.0f, 152.0f);
bVector3 collegeGasStation5Pos(1990.0f, 1640.0f, 152.0f);
bVector3 collegeSmParkingLotPos(1057.0f, 3826.0f, 201.0f);
bVector3 collegeLgParkingLotPos(1227.0f, 3259.0f, 204.0f);
bVector3 collegeTennisCourtPos(729.0f, 3540.0f, 201.0f);
bVector3 collegeStadiumPos(43.0f, 3154.0f, 189.0f);
bVector3 collegeCemetaryPos(846.0f, 2439.0f, 151.0f);
bVector3 collegeHospitalPos(1419.0f, 2613.0f, 164.0f);
bVector3 collegeFireHallPos(1510.0f, 2144.0f, 146.0f);
bVector3 collegeDonutShopPos(1825.0f, 1860.0f, 151.0f);
bVector3 collegeClockTowerPos(1683.0f, 2114.0f, 146.0f);
bVector3 collegeStripMallPos(2633.0f, 2199.0f, 108.0f);
bVector3 collegeOverpassPos(2155.0f, 2642.0f, 148.0f);
bVector3 collegeBusStationPos(2094.0f, 1427.0f, 152.0f);
bVector3 collegeNorthChopShopPos(713.0f, 4507.0f, 214.0f);
bVector3 collegeChopShopPos(1513.0f, 2550.0f, 158.0f);
bVector3 collegeMWCarShopPos(990.0f, 2164.0f, 154.0f);
bVector3 cityNorthChopShopPos(1863.0f, 1193.0f, 146.0f);
bVector3 cityChopShopPos(1086.0f, 54.0f, 101.0f);
bVector3 cityMWCarShopPos(1762.0f, 529.0f, 93.0f);
bVector3 citySouthChopShopPos(3410.0f, -203.0f, 14.0f);
bVector3 coastalNorthChopShopPos(3611.0f, 3636.0f, 31.0f);
bVector3 coastalRefineChopShopPos(3467.0f, 2019.0f, 67.0f);
bVector3 coastalMWCarShopPos(4200.0f, 1276.0f, 48.0f);
bVector3 coastalChopShopPos(4234.0f, 714.0f, 56.0f);

void JumpToHighMemPos() {
    UMath::Vector3 newPosition = {-3221.0f, 394.0f, 1113.0f};
    IPlayer *iPlayer = IPlayer::First(PLAYER_LOCAL);
    if (iPlayer) {
        iPlayer->SetPosition(newPosition);
        Sim::SetStream(newPosition, true);
    }
}

void PlayMovie(char *package) {
    FEAnyMovieScreen::SetMovieName(package);
    cFEng::Get()->QueuePackagePush("FEAnyMovie.fng", 0, 0, false);
}

void PlayLS_EALogo() { PlayMovie("ealogo"); }
void Play2MW_LS_AttractFMV() { PlayMovie("attract_movie"); }
void Play2MW_LS_IntroFMV() { PlayMovie("intro_movie"); }
void PlayBlacklist_01() { PlayMovie("blacklist_01"); }
void PlayBlacklist_02() { PlayMovie("blacklist_02"); }
void PlayBlacklist_03() { PlayMovie("blacklist_03"); }
void PlayBlacklist_04() { PlayMovie("blacklist_04"); }
void PlayBlacklist_05() { PlayMovie("blacklist_05"); }
void PlayBlacklist_06() { PlayMovie("blacklist_06"); }
void PlayBlacklist_07() { PlayMovie("blacklist_07"); }
void PlayBlacklist_08() { PlayMovie("blacklist_08"); }
void PlayBlacklist_09() { PlayMovie("blacklist_09"); }
void PlayBlacklist_10() { PlayMovie("blacklist_10"); }
void PlayBlacklist_11() { PlayMovie("blacklist_11"); }
void PlayBlacklist_12() { PlayMovie("blacklist_12"); }
void PlayBlacklist_13() { PlayMovie("blacklist_13"); }
void PlayBlacklist_14() { PlayMovie("blacklist_14"); }
void PlayBlacklist_15() { PlayMovie("blacklist_15"); }
void PlayDrag_tutorial() { PlayMovie("drag_tutorial"); }
void PlaySprint_tutorial() { PlayMovie("sprint_tutorial"); }
void PlayStoryfmv_bla134() { PlayMovie("storyfmv_bla134"); }
void PlayStoryfmv_bus12() { PlayMovie("storyfmv_bus12"); }
void PlayStoryfmv_cro06_coh06a() { PlayMovie("storyfmv_cro06_coh06a"); }
void PlayStoryfmv_dda01() { PlayMovie("storyfmv_dda01"); }
void PlayStoryfmv_epi138() { PlayMovie("storyfmv_epi138"); }
void PlayStoryfmv_her136() { PlayMovie("storyfmv_her136"); }
void PlayStoryfmv_pin11() { PlayMovie("storyfmv_pin11"); }
void PlayStoryfmv_pol17_mot21() { PlayMovie("storyfmv_pol17_mot21"); }
void PlayStoryfmv_rap30() { PlayMovie("storyfmv_rap30"); }
void PlayStoryfmv_raz08() { PlayMovie("storyfmv_raz08"); }
void PlayStoryfmv_roc02() { PlayMovie("storyfmv_roc02"); }
void PlayStoryfmv_saf25() { PlayMovie("storyfmv_saf25"); }
void PlayTollbooth_tutorial() { PlayMovie("tollbooth_tutorial"); }
void PlaySSX_OT_n_480() { PlayMovie("SSX-OT-n-480"); }
void PlayPursuit_tutorial() { PlayMovie("pursuit_tutorial"); }
void PlayPsa() { PlayMovie("psa"); }
void PlayNBALive_2006() { PlayMovie("NBALive-2006-n"); }
void PlayMarvNM_480() { PlayMovie("MarvNM-480"); }
void PlayBounty_tutorial() { PlayMovie("bounty_tutorial"); }

void JumpToCityPos() { JumpToNewPos(&kPosDebugCity); }
void JumpToCitySHPos() { JumpToNewPos(&kPosDebugSafeHouseCity); }
void JumpToCityEastPark() { JumpToNewPos(&cityEastParkPos); }
void JumpToCityWestPark() { JumpToNewPos(&cityWestParkPos); }
void JumpToCityStadium() { JumpToNewPos(&cityStadiumPos); }
void JumpToCityTimeSquare() { JumpToNewPos(&cityTimesSquarePos); }
void JumpToCityLittleItaly() { JumpToNewPos(&cityLittleItalyPos); }
void JumpToCitySubwayEntrance1() { JumpToNewPos(&citySubwayEntrance1Pos); }
void JumpToCitySubwayEntrance2() { JumpToNewPos(&citySubwayEntrance2Pos); }
void JumpToCitySubwayEntrance3() { JumpToNewPos(&citySubwayEntrance3Pos); }
void JumpToCityHighwayNorth() { JumpToNewPos(&cityHighwayNorthPos); }
void JumpToCityHighwaySouth() { JumpToNewPos(&cityHighwaySouthPos); }
void JumpToCityHighwayEast() { JumpToNewPos(&cityHighwayEastPos); }
void JumpToCityHighwayWest() { JumpToNewPos(&cityHighwayWestPos); }
void JumpToCityMuseum() { JumpToNewPos(&cityMuseumPos); }
void JumpToCityAmphitheatre() { JumpToNewPos(&cityAmphitheatrePos); }

void JumpToCoastalPos() { JumpToNewPos(&kPosDebugCoastal); }
void JumpToCoastalSHPos() { JumpToNewPos(&kPosDebugSafeHouseCoastal); }
void JumpToCoastalFishMarket() { JumpToNewPos(&coastalFishMarketPos); }
void JumpToCoastalOilRefinery() { JumpToNewPos(&coastalOilRefineryPos); }
void JumpToCoastalFishingVillage() { JumpToNewPos(&coastalFishingVillagePos); }
void JumpToCoastalShipyard() { JumpToNewPos(&coastalShipyardPos); }
void JumpToCoastalTrainyard() { JumpToNewPos(&coastalTrainyardPos); }
void JumpToCoastalLighthouse() { JumpToNewPos(&coastalLighthousePos); }
void JumpToCoastalDriveInTheatre() { JumpToNewPos(&coastalDriveInTheatrePos); }
void JumpToCoastalGasStation1() { JumpToNewPos(&coastalGasStation1Pos); }
void JumpToCoastalGasStation2() { JumpToNewPos(&coastalGasStation2Pos); }
void JumpToCoastalGasStation3() { JumpToNewPos(&coastalGasStation3Pos); }
void JumpToCoastalGasStation4() { JumpToNewPos(&coastalGasStation4Pos); }
void JumpToCoastalTrailerPark() { JumpToNewPos(&coastalTrailerParkPos); }
void JumpToCoastalCannery() { JumpToNewPos(&coastalCanneryPos); }
void JumpToCoastalFireHall() { JumpToNewPos(&coastalFireHallPos); }
void JumpToCoastalAmusementPark() { JumpToNewPos(&coastalAmusementParkPos); }
void JumpToCoastalBoardwalk() { JumpToNewPos(&coastalBoardwalkPos); }
void JumpToCoastalPrison() { JumpToNewPos(&coastalPrisonPos); }

void JumpToCollegePos() { JumpToNewPos(&kPosDebugCollege); }
void JumpToCollegeSHPos() { JumpToNewPos(&kPosDebugSafeHouseCollege); }
void JumpToCollegeRosewoodPark() { JumpToNewPos(&collegeRosewoodParkPos); }
void JumpToCollegeGolfCourse() { JumpToNewPos(&collegeGolfCoursePos); }
void JumpToCollegeCampus() { JumpToNewPos(&collegeCampusPos); }
void JumpToCollegeMainStreet() { JumpToNewPos(&collegeMainStreetPos); }
void JumpToCollegeBaseballStadium() { JumpToNewPos(&collegeBaseballStadiumPos); }
void JumpToCollegeHighwayNorth() { JumpToNewPos(&collegeHighwayNorthPos); }
void JumpToCollegeHighwaySouth() { JumpToNewPos(&collegeHighwaySouthPos); }
void JumpToCollegeHighwayEast() { JumpToNewPos(&collegeHighwayEastPos); }
void JumpToCollegeHighwayWest() { JumpToNewPos(&collegeHighwayWestPos); }
void JumpToCollegeClubHouse() { JumpToNewPos(&collegeClubHousePos); }
void JumpToCollegeTollBooth1() { JumpToNewPos(&collegeTollBooth1Pos); }
void JumpToCollegeTollBooth2() { JumpToNewPos(&collegeTollBooth2Pos); }
void JumpToCollegeGasStation1() { JumpToNewPos(&collegeGasStation1Pos); }
void JumpToCollegeGasStation2() { JumpToNewPos(&collegeGasStation2Pos); }
void JumpToCollegeGasStation3() { JumpToNewPos(&collegeGasStation3Pos); }
void JumpToCollegeGasStation4() { JumpToNewPos(&collegeGasStation4Pos); }
void JumpToCollegeGasStation5() { JumpToNewPos(&collegeGasStation5Pos); }
void JumpToCollegeSmParkingLot() { JumpToNewPos(&collegeSmParkingLotPos); }
void JumpToCollegeLgParkingLot() { JumpToNewPos(&collegeLgParkingLotPos); }
void JumpToCollegeTennisCourt() { JumpToNewPos(&collegeTennisCourtPos); }
void JumpToCollegeCemetary() { JumpToNewPos(&collegeCemetaryPos); }
void JumpToCollegeHospital() { JumpToNewPos(&collegeHospitalPos); }
void JumpToCollegeFireHall() { JumpToNewPos(&collegeFireHallPos); }
void JumpToCollegeDonutShop() { JumpToNewPos(&collegeDonutShopPos); }
void JumpToCollegeClockTower() { JumpToNewPos(&collegeClockTowerPos); }
void JumpToCollegeStripMall() { JumpToNewPos(&collegeStripMallPos); }
void JumpToCollegeOverpass() { JumpToNewPos(&collegeOverpassPos); }
void JumpToCollegeBusStation() { JumpToNewPos(&collegeBusStationPos); }
void JumpToCollegeNorthChopShop() { JumpToNewPos(&collegeNorthChopShopPos); }
void JumpToCollegeChopShop() { JumpToNewPos(&collegeChopShopPos); }
void JumpToCollegeMWCarShop() { JumpToNewPos(&collegeMWCarShopPos); }
void JumpToCityNorthChopShop() { JumpToNewPos(&cityNorthChopShopPos); }
void JumpToCityChopShop() { JumpToNewPos(&cityChopShopPos); }
void JumpToCityMWCarShop() { JumpToNewPos(&cityMWCarShopPos); }
void JumpToCitySouthChopShop() { JumpToNewPos(&citySouthChopShopPos); }
void JumpToCoastalNorthChopShop() { JumpToNewPos(&coastalNorthChopShopPos); }
void JumpToCoastalRefineChopShop() { JumpToNewPos(&coastalRefineChopShopPos); }
void JumpToCoastalMWCarShop() { JumpToNewPos(&coastalMWCarShopPos); }
void JumpToCoastalChopShop() { JumpToNewPos(&coastalChopShopPos); }

void JumpToBin15BossRace() { JumpToRacePos("15.1.2"); }
void JumpToBin14BossRace() { JumpToRacePos("14.2.4"); }
void JumpToBin13BossRace() { JumpToRacePos("13.2.2"); }
void JumpToBin12BossRace() { JumpToRacePos("12.2.1"); }
void JumpToBin11BossRace() { JumpToRacePos("11.1.2"); }
void JumpToBin10BossRace() { JumpToRacePos("10.2.1"); }
void JumpToBin9BossRace() { JumpToRacePos("9.5.3"); }
void JumpToBin8BossRace() { JumpToRacePos("8.2.1"); }
void JumpToBin7BossRace() { JumpToRacePos("7.2.2"); }
void JumpToBin6BossRace() { JumpToRacePos("6.1.1"); }
void JumpToBin5BossRace() { JumpToRacePos("5.2.2"); }
void JumpToBin4BossRace() { JumpToRacePos("4.5.2"); }
void JumpToBin3BossRace() { JumpToRacePos("3.2.1.0"); }
void JumpToBin2BossRace() { JumpToRacePos("2.2.1"); }
void JumpToBin1BossRace() { JumpToRacePos("1.2.1"); }
void JumpToBin15CareerRace() { JumpToRacePos("15.3.1"); }
void JumpToBin14CareerRace() { JumpToRacePos("14.2.1"); }
void JumpToBin13CareerRace() { JumpToRacePos("13.2.1"); }
void JumpToBin12CareerRace() { JumpToRacePos("12.1.1"); }
void JumpToBin11CareerRace() { JumpToRacePos("11.2.1"); }
void JumpToBin10CareerRace() { JumpToRacePos("10.3.1"); }
void JumpToBin9CareerRace() { JumpToRacePos("9.1.1"); }
void JumpToBin8CareerRace() { JumpToRacePos("8.3.1"); }
void JumpToBin7CareerRace() { JumpToRacePos("7.2.1"); }
void JumpToBin6CareerRace() { JumpToRacePos("6.2.2"); }
void JumpToBin5CareerRace() { JumpToRacePos("5.3.1"); }
void JumpToBin4CareerRace() { JumpToRacePos("4.2.2"); }
void JumpToBin3CareerRace() { JumpToRacePos("3.2.1"); }
void JumpToBin2CareerRace() { JumpToRacePos("2.1.1"); }
void JumpToBin1CareerRace() { JumpToRacePos("1.2.2"); }

void AwardRapSheet() {
    FEDatabase->GetCareerSettings()->SetHasRapSheet();
}

void BoostBounty() {
    int amount = 10;
    for (int i = 0; i < BountyBoost; i++) {
        amount *= 10;
    }
    Game_AwardPlayerBounty(amount);
}

void PassChallenge() {
    GMilestone *binMilestone = GManager::Get().GetFirstMilestone(true, FEDatabase->GetCareerSettings()->GetCurrentBin());
    if (binMilestone) {
        binMilestone->DebugForceComplete();
        Game_ChallengeCompleted();
    }
}

void PassSpeedtrap() {
    GSpeedTrap *binSpeedTrap = GManager::Get().GetFirstSpeedTrap(true, FEDatabase->GetCareerSettings()->GetCurrentBin());
    if (binSpeedTrap) {
        binSpeedTrap->DebugForceComplete();
        Game_ChallengeCompleted();
    }
}

void ToggleMilestoneDebug() {
    TWEAK_ShowGameplayMilestoneValues = !TWEAK_ShowGameplayMilestoneValues;
}

void TriggerDirtyDiscError() {}

void DNASMessage() {}

void DoSMS() {
    new EAddSMS(CareerSMSID);
    gDebugMenuIOHandler->Exit();
}

char JumpToBinOptionText[32] = "Jump to bin XX";

void SetJumpToBinText() {
    if (!GRaceStatus::Exists() && FEDatabase->GetCareerSettings()->GetCurrentBin() > 1) {
        bSPrintf(JumpToBinOptionText, "Jump to bin %d", FEDatabase->GetCareerSettings()->GetCurrentBin() - 1);
    } else {
        bSPrintf(JumpToBinOptionText, "------------");
    }
}

void JumpToBin() {
    if (!GRaceStatus::Exists() && FEDatabase->GetCareerSettings()->GetCurrentBin() > 1) {
        new EEnterBin(FEDatabase->GetCareerSettings()->GetCurrentBin() - 1);
    }
}

void BuyCar() {
    ISimable *simable = IPlayer::First(PLAYER_LOCAL)->GetSimable();
    int plrIndex = simable->GetPlayer()->GetSettingsIndex();
    if (plrIndex >= 0 && plrIndex < 2) {
        UserProfile *prof = FEDatabase->GetUserProfile(plrIndex);
        prof->GetCareer()->AwardCash(200000);
    }
    Game_JumpToCarLot();
}

void SwitchOverlay(char *package) {
    cFEng::Get()->QueuePackagePop(1);
    cFEng::Get()->QueuePackageSwitch(package, 0, 0, false);
}

void OverlayMainMenu() { SwitchOverlay("MainMenu.fng"); }
void OverlayMainMenu_Sub() { SwitchOverlay("MainMenu_Sub.fng"); }
void OverlayOptions() { SwitchOverlay("Options.fng"); }
void OverlayQRBrief() { SwitchOverlay("Quick_Race_Brief.fng"); }
void OverlayTrackSelect() { SwitchOverlay("Track_Select.fng"); }
void OverlayTrackOptions() { SwitchOverlay("Track_Options.fng"); }
void OverlayCarSelect() { SwitchOverlay("Car_Select.fng"); }
void OverlayShowcase() { SwitchOverlay("Showcase.fng"); }
void OverlayPause_Main() { SwitchOverlay("Pause_Main.fng"); }
void OverlayPause_Performance_Tuning() { SwitchOverlay("Pause_Performance_Tuning.fng"); }
void OverlayWorldMapMain() { SwitchOverlay("WorldMapMain.fng"); }
void OverlayPause_Options() { SwitchOverlay("Pause_Options.fng"); }
void OverlayInGameAnyMovie() { SwitchOverlay("InGameAnyMovie.fng"); }
void OverlayEngageEventDialog() { SwitchOverlay("EngageEventDialog.fng"); }
void OverlaySafehouseRaceSheet() { SwitchOverlay("SafehouseRaceSheet.fng"); }
void OverlaySafehouseReputationOverview() { SwitchOverlay("SafehouseReputationOverview.fng"); }
void OverlayRapSheetOverview() { SwitchOverlay("RapSheetOverview.fng"); }
void OverlayRapSheetRankings() { SwitchOverlay("RapSheetRankings.fng"); }
void OverlayRapSheetInfractions() { SwitchOverlay("RapSheetInfractions.fng"); }
void OverlayRapSheetCostToState() { SwitchOverlay("RapSheetCostToState.fng"); }
void OverlaySafehouseRivalChallenge() { SwitchOverlay("SafehouseRivalChallenge.fng"); }
void OverlaySafehouseMilestones() { SwitchOverlay("SafehouseMilestones.fng"); }
void OverlayInGameRaceSheet() { SwitchOverlay("InGameRaceSheet.fng"); }
void OverlayInGameReputationOverview() { SwitchOverlay("InGameReputationOverview.fng"); }
void OverlayInGameMilestones() { SwitchOverlay("InGameMilestones.fng"); }
void OverlayInGameRivalChallenge() { SwitchOverlay("InGameRivalChallenge.fng"); }
void OverlaySMS_Mailboxes() { SwitchOverlay("SMS_Mailboxes.fng"); }
void OverlayBlackList() { SwitchOverlay("BlackList.fng"); }
void OverlayControllerUnplugged() { SwitchOverlay("ControllerUnplugged.fng"); }
void OverlayMyCarsManager() { SwitchOverlay("MyCarsManager.fng"); }
void OverlayUI_DebugCarCustomize() { SwitchOverlay("UI_DebugCarCustomize.fng"); }
void OverlayCustomizeMain() { SwitchOverlay("CustomizeMain.fng"); }
void OverlayCustomizeCategory() { SwitchOverlay("CustomizeCategory.fng"); }
void OverlayCustomizeCategory_BACKROOM() { SwitchOverlay("CustomizeCategory_BACKROOM.fng"); }
void OverlayCustomizeGenericTop() { SwitchOverlay("CustomizeGenericTop.fng"); }
void OverlayCustomizeGenericTop_BACKROOM() { SwitchOverlay("CustomizeGenericTop_BACKROOM.fng"); }
void OverlayShoppingCart() { SwitchOverlay("ShoppingCart.fng"); }
void OverlayShoppingCart_BACKROOM() { SwitchOverlay("ShoppingCart_BACKROOM.fng"); }
void OverlayCustomizeParts() { SwitchOverlay("CustomizeParts.fng"); }
void OverlayCustomizeParts_BACKROOM() { SwitchOverlay("CustomizeParts_BACKROOM.fng"); }
void OverlayCustomHUD() { SwitchOverlay("CustomHUD.fng"); }
void OverlayCustomHUD_BACKROOM() { SwitchOverlay("CustomHUD_BACKROOM.fng"); }
void OverlayCustomHUDColor() { SwitchOverlay("CustomHUDColo.fng"); }
void OverlayCustomHUDColor_BACKROOM() { SwitchOverlay("CustomHUDColor_BACKROOM.fng"); }
void OverlayDecals() { SwitchOverlay("Decals.fng"); }
void OverlayDecals_BACKROOM() { SwitchOverlay("Decals_BACKROOM.fng"); }
void OverlayNumbers() { SwitchOverlay("Numbers.fng"); }
void OverlayPaint() { SwitchOverlay("Paint.fng"); }
void OverlayPaint_BACKROOM() { SwitchOverlay("Paint_BACKROOM.fng"); }
void OverlayRims() { SwitchOverlay("Rims_BACKROOM.fng"); }
void OverlaySpoilers() { SwitchOverlay("Spoilers.fng"); }
void OverlaySpoilers_BACKROOM() { SwitchOverlay("Spoilers_BACKROOM.fng"); }
void OverlayCustomizePerformance() { SwitchOverlay("CustomizePerformance.fng"); }
void OverlayCustomizePerformance_BACKROOM() { SwitchOverlay("CustomizePerformance_BACKROOM.fng"); }
void OverlayGarageMain() { SwitchOverlay("GarageMain.fng"); }
void OverlayLS_EALogo() { SwitchOverlay("LS_EALogo.fng"); }
void OverlayKeyboard() { SwitchOverlay("Keyboard.fng"); }
void OverlayLS_THXMovie() { SwitchOverlay("LS_THXMovie.fng"); }
void OverlayMW_LS_IntroFMV() { SwitchOverlay("MW_LS_IntroFMV.fng"); }
void OverlayMW_LS_AttractFMV() { SwitchOverlay("MW_LS_AttractFMV.fng"); }
void OverlayMW_LS_Splash() { SwitchOverlay("MW_LS_Splash.fng"); }
void OverlayLoading_Tips() { SwitchOverlay("Loading_Tips.fng"); }
void OverlayLS_LangSelect() { SwitchOverlay("LS_LangSelect.fng"); }
void OverlayLoading() { SwitchOverlay("Loading.fng"); }
void OverlayWS_Loading() { SwitchOverlay("WS_Loading.fng"); }
void OverlayLoading_Controller() { SwitchOverlay("Loading_Controller.fng"); }
void OverlayUI_OptionsController() { SwitchOverlay("UI_OptionsController.fng"); }
void OverlayPostRace_Results() { SwitchOverlay("PostRace_Results.fng"); }
void OverlayPostRace_Pursuit() { SwitchOverlay("PostRace_Pursuit.fng"); }
void OverlayUI_OL_Disconnect() { SwitchOverlay("UI_OL_Disconnect.fng"); }
void OverlayUI_OL_Disconnect_BG() { SwitchOverlay("UI_OL_Disconnect_BG.fng"); }
void OverlayOL_News_and_Terms() { SwitchOverlay("OL_News_and_Terms.fng"); }
void OverlayUI_OL_News() { SwitchOverlay("UI_OL_News.fng"); }
void OverlayOL_LobbyRoom() { SwitchOverlay("OL_LobbyRoom.fng"); }
void OverlayUI_OLGameRoom() { SwitchOverlay("UI_OLGameRoom.fng"); }
void OverlayUI_OLGameRoom_host() { SwitchOverlay("OL_GameRoom_Dialog.fng"); }
void OverlayUI_OLGameRoom_client() { SwitchOverlay("UI_OLGameRoom_client.fng"); }
void OverlayOL_ModeSelectList() { SwitchOverlay("OL_ModeSelectList.fng"); }
void OverlayUI_OLPreRaceStart() { SwitchOverlay("UI_OLPreRaceStart.fng"); }
void OverlayUI_OL_ViewCar() { SwitchOverlay("UI_OL_ViewCar.fng"); }
void OverlayUI_OLViewTrack() { SwitchOverlay("UI_OLViewTrack.fng"); }
void OverlayUI_OLCarLot() { SwitchOverlay("UI_OLCarLot.fng"); }
void OverlayOL_MAIN() { SwitchOverlay("OL_MAIN.fng"); }
void OverlayUI_OLX_QuickRace() { SwitchOverlay("OL_Quickrace_Main.fng"); }
void OverlayUI_OLX_OptiMatch_Search() { SwitchOverlay("UI_OLISPConnect.fng"); }
void OverlayUI_OLFilters() { SwitchOverlay("UI_OLFilters.fng"); }
void OverlayUI_OLX_OptiMatch_Available() { SwitchOverlay("UI_OLX_OptiMatch_Available.fng"); }
void OverlayUI_OLX_OptiMatch_Filters() { SwitchOverlay("UI_OLX_OptiMatch_Filters.fng"); }
void OverlayUI_OLX_CreateGame_Start() { SwitchOverlay("OverlayUI_OLX_CreateGame_Start.fng"); }
void OverlayUI_OLX_News() { SwitchOverlay("UI_OLX_News.fng"); }
void OverlayUI_OLRankings_Personal() { SwitchOverlay("UI_OLRankings_Personal.fng"); }
void OverlayUI_OLRankings_Overall() { SwitchOverlay("UI_OLRankings_Overall.fng"); }
void OverlayUI_OLRankings_Monthly() { SwitchOverlay("UI_OLRankings_Monthly.fng"); }
void OverlayOL_EAMessenger() { SwitchOverlay("OL_EAMessenger.fng"); }
void OverlayOL_FriendDialogue() { SwitchOverlay("OL_FriendDialogue.fng"); }
void OverlayOLX_Message() { SwitchOverlay("OLX_Message.fng"); }
void OverlayUI_OLRankings() { SwitchOverlay("UI_OLRankings.fng"); }
void OverlayUI_OLX_FindResults() { SwitchOverlay("UI_OLX_FindResults.fng"); }
void OverlayUI_OL_Feedback() { SwitchOverlay("OL_Feedback.fng"); }
void OverlayUI_OL_Challenge() { SwitchOverlay("UI_OL_Challenge.fng"); }
void OverlayOL_VoiceChat() { SwitchOverlay("OL_VoiceChat.fng"); }
void OverlayUI_OLNetSelect() { SwitchOverlay("UI_OLNetSelect.fng"); }
void OverlayUI_OLAuthDNAS() { SwitchOverlay("UI_OLAuthDNAS.fng"); }
void OverlayUI_OLISPConnect() { SwitchOverlay("UI_OLISPConnect.fng"); }
void OverlayUI_OLSelectPersona() { SwitchOverlay("OL_SelectPersona.fng"); }
void OverlayUI_OLCreateUser() { SwitchOverlay("OL_Create_Account.fng"); }
void OverlayUI_OLAgeVerif() { SwitchOverlay("UI_OLAgeVerif.fng"); }
void OverlayUI_OLAgeTooYoung() { SwitchOverlay("UI_OLAgeTooYoung.fng"); }
void OverlayUI_OLUseExisting() { SwitchOverlay("OL_UseExisting.fng"); }
void OverlayUI_OLForgotAccountName() { SwitchOverlay("OL_ForgotAccountName.fng"); }
void OverlayUI_OLEALogin() { SwitchOverlay("UI_OLEALogin.fng"); }
void OverlayUI_DateEntry() { SwitchOverlay("UI_DateEntry.fng"); }
void OverlayMC_ProfileManager() { SwitchOverlay("MC_ProfileManager.fng"); }
void OverlayMC_Deleteprofile() { SwitchOverlay("MC_Deleteprofile.fng"); }
void OverlayMC_Bootup() { SwitchOverlay("MC_Bootup.fng"); }
void OverlayMC_Bootup_GC() { SwitchOverlay("MC_Bootup_GC.fng"); }
void OverlayMC_List() { SwitchOverlay("MC_List.fng"); }
void OverlayMC_Main() { SwitchOverlay("MC_Main.fng"); }
void OverlayMC_Main_GC() { SwitchOverlay("MC_Main_GC.fng"); }

void CheatToGivePlayerMarkers() {
    TheFEMarkerManager.CheatToGetMarkers();
}

void SwitchToAndFromFEngTestScreen() {
    static char screenNameToSwitchBackTo[128] = "";
    static bool inTestScreen = false;

    if (!inTestScreen) {
        inTestScreen = true;
        FEPackage *currentPackageWithControl = cFEng::Get()->FindPackageWithControl();
        if (currentPackageWithControl) {
            bStrCpy(screenNameToSwitchBackTo, currentPackageWithControl->GetFilename());
        } else {
            bStrCpy(screenNameToSwitchBackTo, "");
        }
        cFEng::Get()->QueuePackageSwitch("TestScreen.fng", 0, 0, false);
    } else {
        inTestScreen = false;
        if (bStrCmp(screenNameToSwitchBackTo, "") != 0) {
            cFEng::Get()->QueuePackageSwitch(screenNameToSwitchBackTo, 0, 0, false);
        } else {
            cFEng::Get()->QueuePackagePop(1);
        }
    }
}

void FadeScreenOn() {
#ifdef EA_BUILD_A124
    new EFadeScreenOn();
#else
    new EFadeScreenOn(false);
#endif
}

void FadeScreenOff() {
    new EFadeScreenOff(FEHASH_15_IN);
}

void TriggerJoystickMonkey() {
    if (!EnableJoystickMonkey) {
        EnableJoystickMonkey = 1;
    } else {
        EnableJoystickMonkey = 0;
    }
}

void TriggerSendRaceLengthToBigBang() {
    bSendRaceLengthToBigBang = !bSendRaceLengthToBigBang;
}

void TriggerWatchCar() {
    if (CameraDebugWatchCar) {
        CameraDebugWatchCar = false;
        mToggleCar = 0;
    } else {
        CameraDebugWatchCar = true;
        mToggleCar = 0;
    }
}

void TriggerWatchCopCar() {
    mToggleCarList = VEHICLE_AICOPS;
    TriggerWatchCar();
}

void TriggerWatchTrafficCar() {
    mToggleCarList = VEHICLE_AITRAFFIC;
    TriggerWatchCar();
}

void TriggerWatchAIRacerCar() {
    mToggleCarList = VEHICLE_AIRACERS;
    TriggerWatchCar();
}

void TriggerScatterCops() {
    static float gScatterForce = 50.0f;
    static float gScatterJump = 5.0f;
    static float gScatterRoll = 10.0f;

    IRigidBody *playerIRB = IPlayer::First(PLAYER_LOCAL)->GetSimable()->GetRigidBody();

    const IVehicle::List &cops = IVehicle::GetList(VEHICLE_AICOPS);
    for (IVehicle::List::const_iterator iter = cops.begin(); iter != cops.end(); ++iter) {
        IVehicle *vehicle = *iter;
        ISimable *simable = vehicle->GetSimable();
        IRigidBody *rigidBody = simable->GetRigidBody();
        UMath::Vector3 toCop;
        UMath::Vector3 linearVel;
        UMath::Vector3 angularVel;

        const UMath::Vector3 &playerPos = playerIRB->GetPosition();
        UMath::Sub(rigidBody->GetPosition(), playerPos, toCop);
        UMath::Normalize(toCop);
        UMath::Scale(toCop, gScatterForce, linearVel);
        linearVel.y += gScatterJump;

        angularVel.x = Sim::GetRandom().SimRandom_FloatRange(gScatterRoll);
        angularVel.y = Sim::GetRandom().SimRandom_FloatRange(gScatterRoll);
        angularVel.z = Sim::GetRandom().SimRandom_FloatRange(gScatterRoll);

        rigidBody->SetLinearVelocity(linearVel);
        rigidBody->SetAngularVelocity(angularVel);
    }

    IVisualTreatment *ivt = IVisualTreatment::Get();
    if (ivt) {
        ivt->TriggerPulse(0.2f);
    }
}

void WatchCarCallback(int dir) {
    IVehicle *iVehicle = nullptr;
    int count = IVehicle::Count(mToggleCarList);
    if (count == 0) {
        return;
    }

    mToggleCar = (mToggleCar + count) % count;
    if (dir == 1) {
        for (int i = mToggleCar + 1; i < count; i++) {
            iVehicle = IVehicle::GetList(mToggleCarList)[i];
            if (!iVehicle) {
                return;
            }
            if (iVehicle->IsActive()) {
                mToggleCar = i;
                break;
            }
        }
    } else {
        for (int i = mToggleCar - 1; i >= 0; i--) {
            iVehicle = IVehicle::GetList(mToggleCarList)[i];
            if (!iVehicle) {
                return;
            }
            if (iVehicle->IsActive()) {
                mToggleCar = i;
                break;
            }
        }
    }

    if (iVehicle) {
        ScreenPrintf(0, 0, 3.0f, "%s\n", iVehicle->GetSimable()->GetAttributes()->MODEL(0).GetString());
    }
}

void ToggleCopAI() {
    if (!ICopMgr::AreCopsEnabled()) {
        ICopMgr::EnableCops();
        DebugMenu::SetPursuitState(STATE_IDLE);
        DebugMenu::SetPursuitFormation(static_cast<FormationType>(0));
    } else {
        ICopMgr::DisableCops();
        DebugMenu::SetPursuitState(STATE_IDLE);
        DebugMenu::SetPursuitFormation(static_cast<FormationType>(0));
    }
    gCopAIDisabled = !ICopMgr::AreCopsEnabled();
}

void ToggleAiControl() {
    if (bToggleAiControl) {
        bToggleAiControl = false;
    } else {
        bToggleAiControl = true;
    }
    gDebugMenuIOHandler->Exit();
}

void FlipCar() {
    IRigidBody *irb;
    if (IVehicle::First(VEHICLE_PLAYERS)->QueryInterface(&irb)) {
        UMath::Matrix4 bodyMatrix;
        irb->GetMatrix4(bodyMatrix);

        UMath::Matrix4 rotationMatrix;
        UMath::SetZRot(rotationMatrix, 0.5f);

        UMath::Mult(bodyMatrix, rotationMatrix, bodyMatrix);

        irb->SetOrientation(bodyMatrix);
    }
}

float gSpeed = 0.0f;

void TriggerSetSpeed() {
    IPlayer *player = IPlayer::First(PLAYER_LOCAL);

    if (player) {
        IVehicle *ivehicle;
        player->GetSimable()->QueryInterface(&ivehicle);

        if (ivehicle) {
            ivehicle->SetSpeed(gSpeed / 2.237f);

            Tweak_TuningAero = 1.0f;

            Tweak_UseTweakerTunings = true;
        }
    }
}

void TriggerSetHeat() {
#ifdef EA_PLATFORM_XENON
    ForcePursuitHeatLevel = gHeat;
#else
    ForcePursuitHeatLevel = static_cast<int>(gHeat);
#endif

    IPlayer *player = IPlayer::First(PLAYER_LOCAL);
    IPerpetrator *iperp;

    if (player->GetSimable()->QueryInterface(&iperp)) {
        iperp->SetHeat(gHeat);
    }

    AdjustStableHeat_DebugIncrease(0, gHeat);

    gDebugMenuIOHandler->Exit();
}

void UpdateHeat() {
    IPlayer *player = IPlayer::First(PLAYER_LOCAL);
    IPerpetrator *iperp;
    player->GetSimable()->QueryInterface(&iperp);
    gHeat = iperp->GetHeat();
}

bool DebugMenuShouldPauseWorld() {
    if (gDebugMenuIOHandler) {
        return gDebugMenuIOHandler->mPauseWorld || DebugMenu::isActive();
    }
    return false;
}

void SetFormationPIT() { DebugCopFormation = PIT; }
void SetFormationBOX() { DebugCopFormation = BOX_IN; }
void SetFormationROLLING() { DebugCopFormation = ROLLING_BLOCK; }
void SetFormationFOLLOW() { DebugCopFormation = FOLLOW; }
void SetFormationHELI() { DebugCopFormation = HELI_PURSUIT; }
void SetFormationHERD() { DebugCopFormation = HERD; }

// The A124 defines it with the tweakables at the top of the file (its .data sits between CareerSMSID and
// LanguageCyclingEnabled); here it follows the callbacks it points to.
StringListParam CopFormationParam[6] = {
    {"PIT", SetFormationPIT},
    {"BOX_IN", SetFormationBOX},
    {"ROLLING_BLOCK", SetFormationROLLING},
    {"FOLLOW", SetFormationFOLLOW},
    {"HELI_PURSUIT", SetFormationHELI},
    {"HERD", SetFormationHERD},
};

bool gCareerRace[20];
bool gCareerChallenge[8];
int gCareerBounty;
GRaceBin *gCareerCurBin;

void CareerulatorReset() {
    for (int raceCounter = 0; raceCounter < 20; raceCounter++) {
        gCareerRace[raceCounter] = false;
    }
    for (int chalCounter = 0; chalCounter < 8; chalCounter++) {
        gCareerChallenge[chalCounter] = false;
    }
    gCareerBounty = 0;
}

#ifdef EA_PLATFORM_XENON
// X360: el int que edita "Video Mode (PAL:0 PAL60:1 NTSC:2)" (EGM 0x829509FC, .data, vale 3) y la funcion que lo
// rellena con el modo actual (EGM 0x822A48D8, entre CareerulatorReset y Careerulator; DebugMenuInit la expande).
// Nombres neutros: el binario no los da.
int DebugMenuVideoMode = 3;

void DebugMenuReadVideoMode() {
    DebugMenuVideoMode = GetVideoMode();
}
#endif

void Careerulator() {
    if (gCareerCurBin) {
        int binNum = gCareerCurBin->GetBinNumber();
        GMilestone *binMilestone;
        GSpeedTrap *binSpeedTrap;

        for (int raceCounter = 0; raceCounter < 20; raceCounter++) {
            if (gCareerRace[raceCounter]) {
                GRaceParameters *raceParams = GRaceDatabase::Get().GetRaceFromHash(gCareerCurBin->GetWorldRaceHash(raceCounter));
                GRaceSaveInfo *saveInfo = GRaceDatabase::Get().GetScoreInfo(raceParams->GetEventHash());
                saveInfo->mFlags |= GRaceDatabase::kCompleted_ContextCareer;
                CareerSettings *career = FEDatabase->GetCareerSettings();
                career->AwardCash(bFloatToInt(raceParams->GetCashValue()));
            }
        }

        for (binMilestone = GManager::Get().GetFirstMilestone(true, binNum); binMilestone;
             binMilestone = GManager::Get().GetNextMilestone(binMilestone, true, binNum)) {
            binMilestone->DebugForceComplete();
            Game_ChallengeCompleted();
        }

        for (binSpeedTrap = GManager::Get().GetFirstSpeedTrap(true, binNum); binSpeedTrap;
             binSpeedTrap = GManager::Get().GetNextSpeedTrap(binSpeedTrap, true, binNum)) {
            binSpeedTrap->DebugForceComplete();
            Game_ChallengeCompleted();
        }

        Game_AwardPlayerBounty(gCareerBounty);
        gCareerCurBin->RefreshProgress();
    }
    gDebugMenuIOHandler->Exit();
}

void DebugMenuAllocate() {
    DebugMenu::setRenderer(new ("DebugMenuRender", 0, BMEMORY_POOL(FEngMemoryPoolNumber)) NFSMenuRender);
}

void DebugMenuInit() {
    gDebugMenuIOHandler = new DebugMenuInputHandler();
    DebugMenuAllocate();
#ifdef EA_PLATFORM_XENON
    DebugMenuReadVideoMode();
#endif
}

void DebugMenuUnload() {
    DebugMenu::destroy();
}

bool OverClock = false;

void DebugMenuLoad() {
    CareerulatorReset();
    DebugMenu::StartNewList();
    DebugMenu *root = DebugMenu::create("rootMenu", nullptr);
    {
        DebugMenu *frontend_menu = DebugMenu::create("Front End ->", root);
        {
            {
                // No stab for this local in the A124 (GCC folds it into the next call): its name is not known.
                DebugMenu *safehouse_menu = DebugMenu::create("SafeHouse ->", frontend_menu);
                DebugMenu::create("Reputation ->", safehouse_menu);
                {
                    DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Set Reputation", &Tweak_ToggleSetReputation, nullptr));
                    DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("Race Rep", &Tweak_SetReputationRace, 0, 999, 10, SetRaceReputation));
                    DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("Milestones Rep", &Tweak_SetReputationMilestones, 0, 999, 10, SetMilestonesReputation));
                    DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("Pursuit Rep", &Tweak_SetReputationPursuit, 0, 999, 10, SetPursuitReputation));
                    DebugMenu::endNewestMenu();
                }
                DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Unlock All Things", &UnlockAllThings, nullptr));
                MenuWidget *this_is_a_unique_identifier_1 = new ("ActionWidget", 0) ActionWidget("Get 1 of each Marker", CheatToGivePlayerMarkers);
                DebugMenu::addWidgetToNewestMenu(this_is_a_unique_identifier_1);
                this_is_a_unique_identifier_1->SetExitMenuAfterSelected(true);
                DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("Adjust Player's Cash", &playerCash, -1000000, 1000000, 1000, SetCash));
                DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("Current Bin", &playerBin, 1, 20, 1, SetBin));
                DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Car Guys Camera", &CarGuysCamera, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Render Lookat Point", &RenderLookAtPoint, nullptr));
                DebugMenu::endNewestMenu();
            }
            DebugMenu::create("Localization ->", frontend_menu);
            {
                MenuWidget *sms_dump = new ("ActionWidget", 0) ActionWidget("Show All SMS", ShowSMS);
                DebugMenu::addWidgetToNewestMenu(sms_dump);
                sms_dump->SetExitMenuAfterSelected(true);
                MenuWidget *dump_the_loc = new ("ActionWidget", 0) ActionWidget("Dump Loc Tracking", DumpLocTracking);
                DebugMenu::addWidgetToNewestMenu(dump_the_loc);
                dump_the_loc->SetExitMenuAfterSelected(true);
                DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Enable Language Cycling", &LanguageCyclingEnabled, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Enable Loc Tracking", &gToggleLocTracking, nullptr));
                DebugMenu::endNewestMenu();
            }
            DebugMenu::create("Printfs ->", frontend_menu);
            {
                DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Screen Printf", &gDebugMenuIOHandler->mPrevPrintfState, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("IOP Printf", &gbShowFreeIOPMem, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Online Session ID Printf", &GameSessionIDPrintf, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Soak Timer", &DebugMenu::mDoSoakTimer, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Pursuit Printf", &DebugMenu::mDoPursuitPrintf, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("FMV Timer Printf", &IsMovieTimerPrintf, nullptr));
                DebugMenu::endNewestMenu();
            }
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Front End Drawing", &gDebugMenuIOHandler->mPrevFEngDrawState, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Display FMV filename", &FMVScreenPrintf, nullptr));
            MenuWidget *trigger_dirty_disc = new ("ActionWidget", 0) ActionWidget("Trigger Dirty Disc", TriggerDirtyDiscError);
            DebugMenu::addWidgetToNewestMenu(trigger_dirty_disc);
            trigger_dirty_disc->SetExitMenuAfterSelected(true);
            MenuWidget *switch_to_from_feng_test_screen = new ("ActionWidget", 0) ActionWidget("Toggle FEng Test Screen", SwitchToAndFromFEngTestScreen);
            DebugMenu::addWidgetToNewestMenu(switch_to_from_feng_test_screen);
            switch_to_from_feng_test_screen->SetExitMenuAfterSelected(true);
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Test Career Customization", &g_bTestCareerCustomization, nullptr));
            DebugMenu::endNewestMenu();
        }
        DebugMenu *career_settings = DebugMenu::create("Career ->", root);
        {
            if (TheGameFlowManager.IsInGame()) {
                DebugMenu *careerulator = DebugMenu::create("Careerulator ->", career_settings);
                {
                    gCareerCurBin = GRaceStatus::Get().GetRaceBin();
                    if (gCareerCurBin) {
                        DebugMenu::create("Races ->", careerulator);
                        {
                            for (unsigned int raceCount = 0; raceCount < gCareerCurBin->GetWorldRaceCount(); raceCount++) {
                                unsigned int hash = gCareerCurBin->GetWorldRaceHash(raceCount);
                                GRaceParameters *raceParams = GRaceDatabase::Get().GetRaceFromHash(hash);

                                if (hash) {
                                    DebugMenu::addWidgetToNewestMenu(
                                        new ("BoolWidget", 0) BoolWidget(raceParams->GetEventID(), &gCareerRace[raceCount], nullptr));
                                }
                            }
                            DebugMenu::endNewestMenu();
                        }
                        gCareerBounty = gCareerCurBin->GetRequiredBounty();
                        DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("Bounty", &gCareerBounty, 0, 100000, 50, nullptr));
                        MenuWidget *careerulator = new ("ActionWidget", 0) ActionWidget("Careerulate", Careerulator);
                        DebugMenu::addWidgetToNewestMenu(careerulator);
                        careerulator->SetExitMenuAfterSelected(true);
                    }
                    DebugMenu::endNewestMenu();
                }
            }
            DebugMenu *career_jumping = DebugMenu::create("Career Teleports ->", career_settings);
            {
                DebugMenu::create("Boss Races ->", career_jumping);
                {
                    MenuWidget *jump_boss_15 = new ("ActionWidget", 0) ActionWidget("Boss 15", JumpToBin15BossRace);
                    DebugMenu::addWidgetToNewestMenu(jump_boss_15);
                    jump_boss_15->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_boss_14 = new ("ActionWidget", 0) ActionWidget("Boss 14", JumpToBin14BossRace);
                    DebugMenu::addWidgetToNewestMenu(jump_boss_14);
                    jump_boss_14->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_boss_13 = new ("ActionWidget", 0) ActionWidget("Boss 13", JumpToBin13BossRace);
                    DebugMenu::addWidgetToNewestMenu(jump_boss_13);
                    jump_boss_13->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_boss_12 = new ("ActionWidget", 0) ActionWidget("Boss 12", JumpToBin12BossRace);
                    DebugMenu::addWidgetToNewestMenu(jump_boss_12);
                    jump_boss_12->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_boss_11 = new ("ActionWidget", 0) ActionWidget("Boss 11", JumpToBin11BossRace);
                    DebugMenu::addWidgetToNewestMenu(jump_boss_11);
                    jump_boss_11->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_boss_10 = new ("ActionWidget", 0) ActionWidget("Boss 10", JumpToBin10BossRace);
                    DebugMenu::addWidgetToNewestMenu(jump_boss_10);
                    jump_boss_10->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_boss_9 = new ("ActionWidget", 0) ActionWidget("Boss 9", JumpToBin9BossRace);
                    DebugMenu::addWidgetToNewestMenu(jump_boss_9);
                    jump_boss_9->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_boss_8 = new ("ActionWidget", 0) ActionWidget("Boss 8", JumpToBin8BossRace);
                    DebugMenu::addWidgetToNewestMenu(jump_boss_8);
                    jump_boss_8->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_boss_7 = new ("ActionWidget", 0) ActionWidget("Boss 7", JumpToBin7BossRace);
                    DebugMenu::addWidgetToNewestMenu(jump_boss_7);
                    jump_boss_7->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_boss_6 = new ("ActionWidget", 0) ActionWidget("Boss 6", JumpToBin6BossRace);
                    DebugMenu::addWidgetToNewestMenu(jump_boss_6);
                    jump_boss_6->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_boss_5 = new ("ActionWidget", 0) ActionWidget("Boss 5", JumpToBin5BossRace);
                    DebugMenu::addWidgetToNewestMenu(jump_boss_5);
                    jump_boss_5->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_boss_4 = new ("ActionWidget", 0) ActionWidget("Boss 4", JumpToBin4BossRace);
                    DebugMenu::addWidgetToNewestMenu(jump_boss_4);
                    jump_boss_4->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_boss_3 = new ("ActionWidget", 0) ActionWidget("Boss 3", JumpToBin3BossRace);
                    DebugMenu::addWidgetToNewestMenu(jump_boss_3);
                    jump_boss_3->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_boss_2 = new ("ActionWidget", 0) ActionWidget("Boss 2", JumpToBin2BossRace);
                    DebugMenu::addWidgetToNewestMenu(jump_boss_2);
                    jump_boss_2->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_boss_1 = new ("ActionWidget", 0) ActionWidget("Boss 1", JumpToBin1BossRace);
                    DebugMenu::addWidgetToNewestMenu(jump_boss_1);
                    jump_boss_1->SetExitMenuAfterSelected(true);
                    DebugMenu::endNewestMenu();
                }
                DebugMenu::create("Career Races ->", career_jumping);
                {
                    MenuWidget *jump_career_15 = new ("ActionWidget", 0) ActionWidget("Career 15", JumpToBin15CareerRace);
                    DebugMenu::addWidgetToNewestMenu(jump_career_15);
                    jump_career_15->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_career_14 = new ("ActionWidget", 0) ActionWidget("Career 14", JumpToBin14CareerRace);
                    DebugMenu::addWidgetToNewestMenu(jump_career_14);
                    jump_career_14->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_career_13 = new ("ActionWidget", 0) ActionWidget("Career 13", JumpToBin13CareerRace);
                    DebugMenu::addWidgetToNewestMenu(jump_career_13);
                    jump_career_13->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_career_12 = new ("ActionWidget", 0) ActionWidget("Career 12", JumpToBin12CareerRace);
                    DebugMenu::addWidgetToNewestMenu(jump_career_12);
                    jump_career_12->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_career_11 = new ("ActionWidget", 0) ActionWidget("Career 11", JumpToBin11CareerRace);
                    DebugMenu::addWidgetToNewestMenu(jump_career_11);
                    jump_career_11->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_career_10 = new ("ActionWidget", 0) ActionWidget("Career 10", JumpToBin10CareerRace);
                    DebugMenu::addWidgetToNewestMenu(jump_career_10);
                    jump_career_10->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_career_9 = new ("ActionWidget", 0) ActionWidget("Career 9", JumpToBin9CareerRace);
                    DebugMenu::addWidgetToNewestMenu(jump_career_9);
                    jump_career_9->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_career_8 = new ("ActionWidget", 0) ActionWidget("Career 8", JumpToBin8CareerRace);
                    DebugMenu::addWidgetToNewestMenu(jump_career_8);
                    jump_career_8->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_career_7 = new ("ActionWidget", 0) ActionWidget("Career 7", JumpToBin7CareerRace);
                    DebugMenu::addWidgetToNewestMenu(jump_career_7);
                    jump_career_7->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_career_6 = new ("ActionWidget", 0) ActionWidget("Career 6", JumpToBin6CareerRace);
                    DebugMenu::addWidgetToNewestMenu(jump_career_6);
                    jump_career_6->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_career_5 = new ("ActionWidget", 0) ActionWidget("Career 5", JumpToBin5CareerRace);
                    DebugMenu::addWidgetToNewestMenu(jump_career_5);
                    jump_career_5->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_career_4 = new ("ActionWidget", 0) ActionWidget("Career 4", JumpToBin4CareerRace);
                    DebugMenu::addWidgetToNewestMenu(jump_career_4);
                    jump_career_4->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_career_3 = new ("ActionWidget", 0) ActionWidget("Career 3", JumpToBin3CareerRace);
                    DebugMenu::addWidgetToNewestMenu(jump_career_3);
                    jump_career_3->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_career_2 = new ("ActionWidget", 0) ActionWidget("Career 2", JumpToBin2CareerRace);
                    DebugMenu::addWidgetToNewestMenu(jump_career_2);
                    jump_career_2->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_career_1 = new ("ActionWidget", 0) ActionWidget("Career 1", JumpToBin1CareerRace);
                    DebugMenu::addWidgetToNewestMenu(jump_career_1);
                    jump_career_1->SetExitMenuAfterSelected(true);
                    DebugMenu::endNewestMenu();
                }
                DebugMenu::endNewestMenu();
            }
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Skip bin 15 intro", &SkipCareerIntro, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Skip dday races", &SkipDDayRaces, nullptr));
            MenuWidget *award_rap_sheet = new ("ActionWidget", 0) ActionWidget("Award Rap Sheet", AwardRapSheet);
            DebugMenu::addWidgetToNewestMenu(award_rap_sheet);
            award_rap_sheet->SetExitMenuAfterSelected(true);
            DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("Bounty 10^X", &BountyBoost, 2, 6, 1, nullptr));
            MenuWidget *award_bounty = new ("ActionWidget", 0) ActionWidget("Award bounty", BoostBounty);
            DebugMenu::addWidgetToNewestMenu(award_bounty);
            award_bounty->SetExitMenuAfterSelected(true);
            MenuWidget *pass_challenge = new ("ActionWidget", 0) ActionWidget("Pass Challenge", PassChallenge);
            DebugMenu::addWidgetToNewestMenu(pass_challenge);
            pass_challenge->SetExitMenuAfterSelected(true);
            MenuWidget *pass_speedtrap = new ("ActionWidget", 0) ActionWidget("Pass Speedtrap", PassSpeedtrap);
            DebugMenu::addWidgetToNewestMenu(pass_speedtrap);
            pass_speedtrap->SetExitMenuAfterSelected(true);
            MenuWidget *toggle_milestone = new ("ActionWidget", 0) ActionWidget("Toggle milestone debug", ToggleMilestoneDebug);
            DebugMenu::addWidgetToNewestMenu(toggle_milestone);
            toggle_milestone->SetExitMenuAfterSelected(true);
            MenuWidget *buy_car = new ("ActionWidget", 0) ActionWidget("Buy Car", BuyCar);
            DebugMenu::addWidgetToNewestMenu(buy_car);
            buy_car->SetExitMenuAfterSelected(true);
            MenuWidget *enter_bin = new ("ActionWidget", 0) ActionWidget(JumpToBinOptionText, JumpToBin);
            DebugMenu::addWidgetToNewestMenu(enter_bin);
            enter_bin->SetExitMenuAfterSelected(true);
            DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("SMS ID", &CareerSMSID, 10, 200, 1, nullptr));
            MenuWidget *career_do_sms = new ("ActionWidget", 0) ActionWidget("Do SMS", DoSMS);
            DebugMenu::addWidgetToNewestMenu(career_do_sms);
            career_do_sms->SetExitMenuAfterSelected(true);
            DebugMenu::endNewestMenu();
        }
        DebugMenu *pos_jumping = DebugMenu::create("Teleport ->", root);
        {
            DebugMenu::create("by Coordinate ->", pos_jumping);
            {
                DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("X1", &gCoord_x1, -9.0f, 9.0f, 1.0f, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("X2", &gCoord_x2, -9.0f, 9.0f, 1.0f, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("X3", &gCoord_x3, -9.0f, 9.0f, 1.0f, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("X4", &gCoord_x4, -9.0f, 9.0f, 1.0f, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("Y1", &gCoord_y1, -9.0f, 9.0f, 1.0f, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("Y2", &gCoord_y2, -9.0f, 9.0f, 1.0f, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("Y3", &gCoord_y3, -9.0f, 9.0f, 1.0f, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("Y4", &gCoord_y4, -9.0f, 9.0f, 1.0f, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("Z1", &gCoord_z1, -9.0f, 9.0f, 1.0f, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("Z2", &gCoord_z2, -9.0f, 9.0f, 1.0f, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("Z3", &gCoord_z3, -9.0f, 9.0f, 1.0f, nullptr));
                DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("Z4", &gCoord_z4, -9.0f, 9.0f, 1.0f, nullptr));
                MenuWidget *jump_coords = new ("ActionWidget", 0) ActionWidget("           Engage!", JumpToCoordPos);
                DebugMenu::addWidgetToNewestMenu(jump_coords);
                jump_coords->SetExitMenuAfterSelected(true);
                DebugMenu::endNewestMenu();
            }
            DebugMenu *landmark_jumping = DebugMenu::create("by Landmark ->", pos_jumping);
            {
                DebugMenu::create("City Landmarks ->", landmark_jumping);
                {
                    MenuWidget *jump_city_eastpark = new ("ActionWidget", 0) ActionWidget("East Park", JumpToCityEastPark);
                    DebugMenu::addWidgetToNewestMenu(jump_city_eastpark);
                    jump_city_eastpark->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_westpark = new ("ActionWidget", 0) ActionWidget("West Park", JumpToCityWestPark);
                    DebugMenu::addWidgetToNewestMenu(jump_city_westpark);
                    jump_city_westpark->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_stadium = new ("ActionWidget", 0) ActionWidget("Stadium", JumpToCityStadium);
                    DebugMenu::addWidgetToNewestMenu(jump_city_stadium);
                    jump_city_stadium->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_time_square = new ("ActionWidget", 0) ActionWidget("Time Square", JumpToCityTimeSquare);
                    DebugMenu::addWidgetToNewestMenu(jump_city_time_square);
                    jump_city_time_square->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_little_italy = new ("ActionWidget", 0) ActionWidget("Little Italy", JumpToCityLittleItaly);
                    DebugMenu::addWidgetToNewestMenu(jump_city_little_italy);
                    jump_city_little_italy->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_subway1 = new ("ActionWidget", 0) ActionWidget("Subway Entrance 1", JumpToCitySubwayEntrance1);
                    DebugMenu::addWidgetToNewestMenu(jump_city_subway1);
                    jump_city_subway1->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_subway2 = new ("ActionWidget", 0) ActionWidget("Subway Entrance 2", JumpToCitySubwayEntrance2);
                    DebugMenu::addWidgetToNewestMenu(jump_city_subway2);
                    jump_city_subway2->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_subway3 = new ("ActionWidget", 0) ActionWidget("Subway Entrance 3", JumpToCitySubwayEntrance3);
                    DebugMenu::addWidgetToNewestMenu(jump_city_subway3);
                    jump_city_subway3->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_highway_north = new ("ActionWidget", 0) ActionWidget("Highway North", JumpToCityHighwayNorth);
                    DebugMenu::addWidgetToNewestMenu(jump_city_highway_north);
                    jump_city_highway_north->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_highway_South = new ("ActionWidget", 0) ActionWidget("Highway South", JumpToCityHighwaySouth);
                    DebugMenu::addWidgetToNewestMenu(jump_city_highway_South);
                    jump_city_highway_South->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_highway_East = new ("ActionWidget", 0) ActionWidget("Highway East", JumpToCityHighwayEast);
                    DebugMenu::addWidgetToNewestMenu(jump_city_highway_East);
                    jump_city_highway_East->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_highway_West = new ("ActionWidget", 0) ActionWidget("Highway West", JumpToCityHighwayWest);
                    DebugMenu::addWidgetToNewestMenu(jump_city_highway_West);
                    jump_city_highway_West->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_sh_city = new ("ActionWidget", 0) ActionWidget("Safehouse", JumpToCitySHPos);
                    DebugMenu::addWidgetToNewestMenu(jump_sh_city);
                    jump_sh_city->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_museum = new ("ActionWidget", 0) ActionWidget("Museum", JumpToCityMuseum);
                    DebugMenu::addWidgetToNewestMenu(jump_city_museum);
                    jump_city_museum->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_amphitheatre = new ("ActionWidget", 0) ActionWidget("Amphitheatre", JumpToCityAmphitheatre);
                    DebugMenu::addWidgetToNewestMenu(jump_city_amphitheatre);
                    jump_city_amphitheatre->SetExitMenuAfterSelected(true);
                    DebugMenu::endNewestMenu();
                }
                DebugMenu::create("Coastal Landmarks ->", landmark_jumping);
                {
                    MenuWidget *jump_sh_coastal = new ("ActionWidget", 0) ActionWidget("Coastal Safe House", JumpToCoastalSHPos);
                    DebugMenu::addWidgetToNewestMenu(jump_sh_coastal);
                    jump_sh_coastal->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_fish = new ("ActionWidget", 0) ActionWidget("Fish Market", JumpToCoastalFishMarket);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_fish);
                    jump_coastal_fish->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_oil = new ("ActionWidget", 0) ActionWidget("Oil Refinery", JumpToCoastalOilRefinery);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_oil);
                    jump_coastal_oil->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_fishing_village = new ("ActionWidget", 0) ActionWidget("Fishing Village", JumpToCoastalFishingVillage);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_fishing_village);
                    jump_coastal_fishing_village->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_shipyard = new ("ActionWidget", 0) ActionWidget("Shipyard", JumpToCoastalShipyard);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_shipyard);
                    jump_coastal_shipyard->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_trainyard = new ("ActionWidget", 0) ActionWidget("Trainyard", JumpToCoastalTrainyard);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_trainyard);
                    jump_coastal_trainyard->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_lighthouse = new ("ActionWidget", 0) ActionWidget("Lighthouse", JumpToCoastalLighthouse);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_lighthouse);
                    jump_coastal_lighthouse->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_drive_theatre = new ("ActionWidget", 0) ActionWidget("Drive-in Theatre", JumpToCoastalDriveInTheatre);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_drive_theatre);
                    jump_coastal_drive_theatre->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_gas1 = new ("ActionWidget", 0) ActionWidget("Gas Station 1", JumpToCoastalGasStation1);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_gas1);
                    jump_coastal_gas1->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_gas2 = new ("ActionWidget", 0) ActionWidget("Gas Station 2", JumpToCoastalGasStation2);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_gas2);
                    jump_coastal_gas2->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_gas3 = new ("ActionWidget", 0) ActionWidget("Gas Station 3", JumpToCoastalGasStation3);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_gas3);
                    jump_coastal_gas3->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_gas4 = new ("ActionWidget", 0) ActionWidget("Gas Station 4", JumpToCoastalGasStation4);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_gas4);
                    jump_coastal_gas4->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_trailer = new ("ActionWidget", 0) ActionWidget("Trailer Park", JumpToCoastalTrailerPark);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_trailer);
                    jump_coastal_trailer->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_fire = new ("ActionWidget", 0) ActionWidget("Fire Hall", JumpToCoastalFireHall);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_fire);
                    jump_coastal_fire->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_amusement = new ("ActionWidget", 0) ActionWidget("Amusement Park", JumpToCoastalAmusementPark);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_amusement);
                    jump_coastal_amusement->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_boardwalk = new ("ActionWidget", 0) ActionWidget("Boardwalk", JumpToCoastalBoardwalk);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_boardwalk);
                    jump_coastal_boardwalk->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_prison = new ("ActionWidget", 0) ActionWidget("Prison", JumpToCoastalPrison);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_prison);
                    jump_coastal_prison->SetExitMenuAfterSelected(true);
                    DebugMenu::endNewestMenu();
                }
                DebugMenu::create("College Landmarks ->", landmark_jumping);
                {
                    MenuWidget *jump_sh_college = new ("ActionWidget", 0) ActionWidget("College Safe House", JumpToCollegeSHPos);
                    DebugMenu::addWidgetToNewestMenu(jump_sh_college);
                    jump_sh_college->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_rose = new ("ActionWidget", 0) ActionWidget("Rosewood Park", JumpToCollegeRosewoodPark);
                    DebugMenu::addWidgetToNewestMenu(jump_college_rose);
                    jump_college_rose->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_golf = new ("ActionWidget", 0) ActionWidget("Golf Course", JumpToCollegeGolfCourse);
                    DebugMenu::addWidgetToNewestMenu(jump_college_golf);
                    jump_college_golf->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_campus = new ("ActionWidget", 0) ActionWidget("Campus", JumpToCollegeCampus);
                    DebugMenu::addWidgetToNewestMenu(jump_college_campus);
                    jump_college_campus->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_main_st = new ("ActionWidget", 0) ActionWidget("Main Street", JumpToCollegeMainStreet);
                    DebugMenu::addWidgetToNewestMenu(jump_college_main_st);
                    jump_college_main_st->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_baseball_stadium = new ("ActionWidget", 0) ActionWidget("Baseball Stadium", JumpToCollegeBaseballStadium);
                    DebugMenu::addWidgetToNewestMenu(jump_college_baseball_stadium);
                    jump_college_baseball_stadium->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_highway_north = new ("ActionWidget", 0) ActionWidget("Highway North", JumpToCollegeHighwayNorth);
                    DebugMenu::addWidgetToNewestMenu(jump_college_highway_north);
                    jump_college_highway_north->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_highway_South = new ("ActionWidget", 0) ActionWidget("Highway South", JumpToCollegeHighwaySouth);
                    DebugMenu::addWidgetToNewestMenu(jump_college_highway_South);
                    jump_college_highway_South->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_highway_East = new ("ActionWidget", 0) ActionWidget("Highway East", JumpToCollegeHighwayEast);
                    DebugMenu::addWidgetToNewestMenu(jump_college_highway_East);
                    jump_college_highway_East->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_highway_West = new ("ActionWidget", 0) ActionWidget("Highway West", JumpToCollegeHighwayWest);
                    DebugMenu::addWidgetToNewestMenu(jump_college_highway_West);
                    jump_college_highway_West->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_club = new ("ActionWidget", 0) ActionWidget("Club House", JumpToCollegeClubHouse);
                    DebugMenu::addWidgetToNewestMenu(jump_college_club);
                    jump_college_club->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_toll1 = new ("ActionWidget", 0) ActionWidget("Toll Booth 1", JumpToCollegeTollBooth1);
                    DebugMenu::addWidgetToNewestMenu(jump_college_toll1);
                    jump_college_toll1->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_toll2 = new ("ActionWidget", 0) ActionWidget("Toll Booth 2", JumpToCollegeTollBooth2);
                    DebugMenu::addWidgetToNewestMenu(jump_college_toll2);
                    jump_college_toll2->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_gas1 = new ("ActionWidget", 0) ActionWidget("Gas Station 1", JumpToCollegeGasStation1);
                    DebugMenu::addWidgetToNewestMenu(jump_college_gas1);
                    jump_college_gas1->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_gas2 = new ("ActionWidget", 0) ActionWidget("Gas Station 2", JumpToCollegeGasStation2);
                    DebugMenu::addWidgetToNewestMenu(jump_college_gas2);
                    jump_college_gas2->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_gas3 = new ("ActionWidget", 0) ActionWidget("Gas Station 3", JumpToCollegeGasStation3);
                    DebugMenu::addWidgetToNewestMenu(jump_college_gas3);
                    jump_college_gas3->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_gas4 = new ("ActionWidget", 0) ActionWidget("Gas Station 4", JumpToCollegeGasStation4);
                    DebugMenu::addWidgetToNewestMenu(jump_college_gas4);
                    jump_college_gas4->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_gas5 = new ("ActionWidget", 0) ActionWidget("Gas Station 5", JumpToCollegeGasStation5);
                    DebugMenu::addWidgetToNewestMenu(jump_college_gas5);
                    jump_college_gas5->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_slot = new ("ActionWidget", 0) ActionWidget("Small Parking Lot", JumpToCollegeSmParkingLot);
                    DebugMenu::addWidgetToNewestMenu(jump_college_slot);
                    jump_college_slot->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_llot = new ("ActionWidget", 0) ActionWidget("Large Parking Lot", JumpToCollegeLgParkingLot);
                    DebugMenu::addWidgetToNewestMenu(jump_college_llot);
                    jump_college_llot->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_cemetary = new ("ActionWidget", 0) ActionWidget("Cemetary", JumpToCollegeCemetary);
                    DebugMenu::addWidgetToNewestMenu(jump_college_cemetary);
                    jump_college_cemetary->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_hospital = new ("ActionWidget", 0) ActionWidget("Hospital", JumpToCollegeHospital);
                    DebugMenu::addWidgetToNewestMenu(jump_college_hospital);
                    jump_college_hospital->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_fire = new ("ActionWidget", 0) ActionWidget("Fire Hall", JumpToCollegeFireHall);
                    DebugMenu::addWidgetToNewestMenu(jump_college_fire);
                    jump_college_fire->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_donut = new ("ActionWidget", 0) ActionWidget("Donut Shop", JumpToCollegeDonutShop);
                    DebugMenu::addWidgetToNewestMenu(jump_college_donut);
                    jump_college_donut->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_clock = new ("ActionWidget", 0) ActionWidget("Clock Tower", JumpToCollegeClockTower);
                    DebugMenu::addWidgetToNewestMenu(jump_college_clock);
                    jump_college_clock->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_mall = new ("ActionWidget", 0) ActionWidget("Strip Mall", JumpToCollegeStripMall);
                    DebugMenu::addWidgetToNewestMenu(jump_college_mall);
                    jump_college_mall->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_overpass = new ("ActionWidget", 0) ActionWidget("Overpass", JumpToCollegeOverpass);
                    DebugMenu::addWidgetToNewestMenu(jump_college_overpass);
                    jump_college_overpass->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_bus = new ("ActionWidget", 0) ActionWidget("Bus Station", JumpToCollegeBusStation);
                    DebugMenu::addWidgetToNewestMenu(jump_college_bus);
                    jump_college_bus->SetExitMenuAfterSelected(true);
                    DebugMenu::endNewestMenu();
                }
                DebugMenu::create("Shops ->", landmark_jumping);
                {
                    MenuWidget *jump_college_north_chop = new ("ActionWidget", 0) ActionWidget("North College Chop", JumpToCollegeNorthChopShop);
                    DebugMenu::addWidgetToNewestMenu(jump_college_north_chop);
                    jump_college_north_chop->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_chop = new ("ActionWidget", 0) ActionWidget("College Chop", JumpToCollegeChopShop);
                    DebugMenu::addWidgetToNewestMenu(jump_college_chop);
                    jump_college_chop->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_college_car = new ("ActionWidget", 0) ActionWidget("College Car", JumpToCollegeMWCarShop);
                    DebugMenu::addWidgetToNewestMenu(jump_college_car);
                    jump_college_car->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_north_chop = new ("ActionWidget", 0) ActionWidget("North City Chop", JumpToCityNorthChopShop);
                    DebugMenu::addWidgetToNewestMenu(jump_city_north_chop);
                    jump_city_north_chop->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_chop = new ("ActionWidget", 0) ActionWidget("City Chop", JumpToCityChopShop);
                    DebugMenu::addWidgetToNewestMenu(jump_city_chop);
                    jump_city_chop->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_car = new ("ActionWidget", 0) ActionWidget("City Car", JumpToCityMWCarShop);
                    DebugMenu::addWidgetToNewestMenu(jump_city_car);
                    jump_city_car->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_city_south_chop = new ("ActionWidget", 0) ActionWidget("South City Chop", JumpToCitySouthChopShop);
                    DebugMenu::addWidgetToNewestMenu(jump_city_south_chop);
                    jump_city_south_chop->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_north_chop = new ("ActionWidget", 0) ActionWidget("North Coastal Chop", JumpToCoastalNorthChopShop);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_north_chop);
                    jump_coastal_north_chop->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_refine_chop = new ("ActionWidget", 0) ActionWidget("Refinery Coastal Chop", JumpToCoastalRefineChopShop);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_refine_chop);
                    jump_coastal_refine_chop->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_car = new ("ActionWidget", 0) ActionWidget("Coastal Car", JumpToCoastalMWCarShop);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_car);
                    jump_coastal_car->SetExitMenuAfterSelected(true);
                    MenuWidget *jump_coastal_chop = new ("ActionWidget", 0) ActionWidget("Coastal Chop", JumpToCoastalChopShop);
                    DebugMenu::addWidgetToNewestMenu(jump_coastal_chop);
                    jump_coastal_chop->SetExitMenuAfterSelected(true);
                    DebugMenu::endNewestMenu();
                }
                DebugMenu::endNewestMenu();
            }
            MenuWidget *save_hot_position = new ("ActionWidget", 0) ActionWidget("Save Hot Position", SaveHotPos);
            DebugMenu::addWidgetToNewestMenu(save_hot_position);
            save_hot_position->SetExitMenuAfterSelected(true);
            MenuWidget *jump_hot_position = new ("ActionWidget", 0) ActionWidget("Jump to Hot Position", JumpToHotPos);
            DebugMenu::addWidgetToNewestMenu(jump_hot_position);
            jump_hot_position->SetExitMenuAfterSelected(true);
            MenuWidget *jump_caffeine_cursor = new ("ActionWidget", 0) ActionWidget("Jump to Caffeine Cursor", JumpToCaffeineCursor);
            DebugMenu::addWidgetToNewestMenu(jump_caffeine_cursor);
            jump_caffeine_cursor->SetExitMenuAfterSelected(true);
            MenuWidget *jump_seeulator_cursor = new ("ActionWidget", 0) ActionWidget("Jump to Seeulator Cursor", JumpToSeeulatorCursor);
            DebugMenu::addWidgetToNewestMenu(jump_seeulator_cursor);
            jump_seeulator_cursor->SetExitMenuAfterSelected(true);
            MenuWidget *jump_jr2_camera = new ("ActionWidget", 0) ActionWidget("Jump to JR2 Camera", JumpToJR2Camera);
            DebugMenu::addWidgetToNewestMenu(jump_jr2_camera);
            jump_jr2_camera->SetExitMenuAfterSelected(true);
            MenuWidget *jump_high_mem = new ("ActionWidget", 0) ActionWidget("Jump to Memory High Watermark", JumpToHighMemPos);
            DebugMenu::addWidgetToNewestMenu(jump_high_mem);
            jump_high_mem->SetExitMenuAfterSelected(true);
            DebugMenu::endNewestMenu();
        }
        DebugMenu::create("Race ->", root);
        {
            DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("Finish Time", &gFinishTime, 1.0f, 600.0f, 0.5f, nullptr));
            MenuWidget *finish_in_place = new ("IntWidget", 0) IntWidget("Force Finish In Place", &gFinishPlace, 1, 8, 1, FinishRaceInPlace);
            DebugMenu::addWidgetToNewestMenu(finish_in_place);
            finish_in_place->SetExitMenuAfterSelected(true);
            MenuWidget *send_length_to_big_bang = new ("ActionWidget", 0) ActionWidget("Toggle Send Race Length Update", TriggerSendRaceLengthToBigBang);
            DebugMenu::addWidgetToNewestMenu(send_length_to_big_bang);
            send_length_to_big_bang->SetExitMenuAfterSelected(true);
            DebugMenu::endNewestMenu();
        }
        {
            // No stab for this local in the A124 (GCC folds it into the next call): its name is not known.
            DebugMenu *ai_menu = DebugMenu::create("AI ->", root);
            DebugMenu::create("Car Watches ->", ai_menu);
            {
                MenuWidget *enable_watch_cop_car = new ("ActionWidget", 0) ActionWidget("Toggle Watch Cop Car", TriggerWatchCopCar);
                DebugMenu::addWidgetToNewestMenu(enable_watch_cop_car);
                enable_watch_cop_car->SetExitMenuAfterSelected(true);
                MenuWidget *enable_watch_traffic_car = new ("ActionWidget", 0) ActionWidget("Toggle Watch Traffic Car", TriggerWatchTrafficCar);
                DebugMenu::addWidgetToNewestMenu(enable_watch_traffic_car);
                enable_watch_traffic_car->SetExitMenuAfterSelected(true);
                MenuWidget *enable_watch_racer_car = new ("ActionWidget", 0) ActionWidget("Toggle Watch Racer Car", TriggerWatchAIRacerCar);
                DebugMenu::addWidgetToNewestMenu(enable_watch_racer_car);
                enable_watch_racer_car->SetExitMenuAfterSelected(true);
                DebugMenu::endNewestMenu();
            }
            MenuWidget *toggle_ai_control = new ("ActionWidget", 0) ActionWidget("Toggle AI Control", ToggleAiControl);
            DebugMenu::addWidgetToNewestMenu(toggle_ai_control);
            toggle_ai_control->SetExitMenuAfterSelected(true);
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Show Non-Pursuit Cops (Minimap)", &MinimapShowNonPursuitCops, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Show Pursuit Cops (Minimap)", &MinimapShowPursuitCops, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("Set Heat", &gHeat, 1.0f, 10.0f, 1.0f, TriggerSetHeat));
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("No Cops", &gCopAIDisabled, ToggleCopAI));
            MenuWidget *scatter_cops = new ("ActionWidget", 0) ActionWidget("Scatter Cops", TriggerScatterCops);
            DebugMenu::addWidgetToNewestMenu(scatter_cops);
            scatter_cops->SetExitMenuAfterSelected(true);
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Hide GPS arrow", &gHideGPSArrow, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Override Cop Formation", &CopFormationOverriden, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("StringListWidget", 0) StringListWidget("Cop Formation", CopFormationParam, 6, 0));
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("AI Random Turns", &bAiRandomTurns, nullptr));
            DebugMenu::endNewestMenu();
        }
        DebugMenu::create("Car ->", root);
        {
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Speed Boost Button", &gDebugMenuIOHandler->mActiveSpeed, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("Speed Boost Speed (MPH)", &gSpeed, 10.0f, 350.0f, 10.0f, nullptr));
            MenuWidget *change_car = new ("ActionWidget", 0) ActionWidget("Change Car", ChangeUserCarCallback);
            DebugMenu::addWidgetToNewestMenu(change_car);
            change_car->SetExitMenuAfterSelected(true);
            MenuWidget *flip_car = new ("ActionWidget", 0) ActionWidget("Flip Car", FlipCar);
            DebugMenu::addWidgetToNewestMenu(flip_car);
            flip_car->SetExitMenuAfterSelected(true);
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("No Car vs Car Collision", &RBVehicle_DisableVehicleVsVehicleCollisions, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Infinite NOS", &Tweak_InfiniteNOS, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Infinite RaceBreaker", &Tweak_InfiniteRaceBreaker, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("Display Car Memory", &DisplayCarLoaderMemoryOnScreen, 0, 1, 1, nullptr));
            DebugMenu::endNewestMenu();
        }
        DebugMenu::create("Online ->", root);
        {
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Draw Positions", &gOnlineDbg, TriggerOnlineDbg));
            DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("Max Latency", &kMaxLatency, 0.0f, 0.475f, 0.025f, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("Blend Multiplier", &kBlendMult, 0.25f, 5.0f, 0.25f, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("Min Blend", &kMinBlendTime, 0.1f, kMaxBlendTime, 0.1f, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("FloatWidget", 0) FloatWidget("Max Blend", &kMaxBlendTime, kMinBlendTime, 2.5f, 0.1f, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Use driver AI", &kUseDriverAI, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Spam Physics", &kSpamPhysics, nullptr));
            DebugMenu::endNewestMenu();
        }
        DebugMenu::create("Rendering ->", root);
        {
            DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("Weather Effects", &PrecipitationEnable, 0, 1, 1, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("TimeOfDay", reinterpret_cast<int *>(&gTheTimeOfDay), 0, 1, 1, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("TimeOfDay Swapping", &TimeOfDaySwapEnable, 0, 1, 1, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("Always Raining", &Chance100, 0, 1, 1, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Pause World", &gDebugMenuIOHandler->mPauseWorld, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("Visual Look Filter", &gApplyMWVisualLook, 0, 1, 1, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("OverClock CPU", &OverClock, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("StringListWidget", 0) StringListWidget("Preculler Mode", PrecullerModeParams, 4, PrecullerMode));
            MenuWidget *set_preculler_booboo = new ("ActionWidget", 0) ActionWidget("Set Preculler BooBoo", SetPrecullerBooBoo);
            DebugMenu::addWidgetToNewestMenu(set_preculler_booboo);
            set_preculler_booboo->SetExitMenuAfterSelected(true);
            MenuWidget *clr_preculler_booboo = new ("ActionWidget", 0) ActionWidget("Clr Preculler BooBoo", ClrPrecullerBooBoo);
            DebugMenu::addWidgetToNewestMenu(clr_preculler_booboo);
            clr_preculler_booboo->SetExitMenuAfterSelected(true);
            DebugMenu::endNewestMenu();
        }
        DebugMenu *FMV = DebugMenu::create("FMV ->", root);
        {
            DebugMenu::create("BlackList FMV ->", FMV);
            {
                MenuWidget *Blacklist_01 = new ("ActionWidget", 0) ActionWidget("Blacklist_01", PlayBlacklist_01);
                DebugMenu::addWidgetToNewestMenu(Blacklist_01);
                Blacklist_01->SetExitMenuAfterSelected(true);
                MenuWidget *Blacklist_02 = new ("ActionWidget", 0) ActionWidget("Blacklist_02", PlayBlacklist_02);
                DebugMenu::addWidgetToNewestMenu(Blacklist_02);
                Blacklist_02->SetExitMenuAfterSelected(true);
                MenuWidget *blacklist_03 = new ("ActionWidget", 0) ActionWidget("blacklist_03", PlayBlacklist_03);
                DebugMenu::addWidgetToNewestMenu(blacklist_03);
                blacklist_03->SetExitMenuAfterSelected(true);
                MenuWidget *blacklist_04 = new ("ActionWidget", 0) ActionWidget("blacklist_04", PlayBlacklist_04);
                DebugMenu::addWidgetToNewestMenu(blacklist_04);
                blacklist_04->SetExitMenuAfterSelected(true);
                MenuWidget *blacklist_05 = new ("ActionWidget", 0) ActionWidget("blacklist_05", PlayBlacklist_05);
                DebugMenu::addWidgetToNewestMenu(blacklist_05);
                blacklist_05->SetExitMenuAfterSelected(true);
                MenuWidget *blacklist_06 = new ("ActionWidget", 0) ActionWidget("blacklist_06", PlayBlacklist_06);
                DebugMenu::addWidgetToNewestMenu(blacklist_06);
                blacklist_06->SetExitMenuAfterSelected(true);
                MenuWidget *Blacklist_07 = new ("ActionWidget", 0) ActionWidget("Blacklist_07", PlayBlacklist_07);
                DebugMenu::addWidgetToNewestMenu(Blacklist_07);
                Blacklist_07->SetExitMenuAfterSelected(true);
                MenuWidget *Blacklist_08 = new ("ActionWidget", 0) ActionWidget("Blacklist_08", PlayBlacklist_08);
                DebugMenu::addWidgetToNewestMenu(Blacklist_08);
                Blacklist_08->SetExitMenuAfterSelected(true);
                MenuWidget *Blacklist_09 = new ("ActionWidget", 0) ActionWidget("Blacklist_09", PlayBlacklist_09);
                DebugMenu::addWidgetToNewestMenu(Blacklist_09);
                Blacklist_09->SetExitMenuAfterSelected(true);
                MenuWidget *Blacklist_10 = new ("ActionWidget", 0) ActionWidget("Blacklist_10", PlayBlacklist_10);
                DebugMenu::addWidgetToNewestMenu(Blacklist_10);
                Blacklist_10->SetExitMenuAfterSelected(true);
                MenuWidget *Blacklist_11 = new ("ActionWidget", 0) ActionWidget("Blacklist_11", PlayBlacklist_11);
                DebugMenu::addWidgetToNewestMenu(Blacklist_11);
                Blacklist_11->SetExitMenuAfterSelected(true);
                MenuWidget *Blacklist_12 = new ("ActionWidget", 0) ActionWidget("Blacklist_12", PlayBlacklist_12);
                DebugMenu::addWidgetToNewestMenu(Blacklist_12);
                Blacklist_12->SetExitMenuAfterSelected(true);
                MenuWidget *Blacklist_13 = new ("ActionWidget", 0) ActionWidget("Blacklist_13", PlayBlacklist_13);
                DebugMenu::addWidgetToNewestMenu(Blacklist_13);
                Blacklist_13->SetExitMenuAfterSelected(true);
                MenuWidget *Blacklist_14 = new ("ActionWidget", 0) ActionWidget("Blacklist_14", PlayBlacklist_14);
                DebugMenu::addWidgetToNewestMenu(Blacklist_14);
                Blacklist_14->SetExitMenuAfterSelected(true);
                MenuWidget *Blacklist_15 = new ("ActionWidget", 0) ActionWidget("Blacklist_15", PlayBlacklist_15);
                DebugMenu::addWidgetToNewestMenu(Blacklist_15);
                Blacklist_15->SetExitMenuAfterSelected(true);
                DebugMenu::endNewestMenu();
            }
            DebugMenu::create("Story FMV ->", FMV);
            {
                MenuWidget *storyfmv_bla134 = new ("ActionWidget", 0) ActionWidget("storyfmv_bla134", PlayStoryfmv_bla134);
                DebugMenu::addWidgetToNewestMenu(storyfmv_bla134);
                storyfmv_bla134->SetExitMenuAfterSelected(true);
                MenuWidget *storyfmv_bus12 = new ("ActionWidget", 0) ActionWidget("storyfmv_bus12", PlayStoryfmv_bus12);
                DebugMenu::addWidgetToNewestMenu(storyfmv_bus12);
                storyfmv_bus12->SetExitMenuAfterSelected(true);
                MenuWidget *storyfmv_cro06_coh06a = new ("ActionWidget", 0) ActionWidget("storyfmv_cro06_coh06a", PlayStoryfmv_cro06_coh06a);
                DebugMenu::addWidgetToNewestMenu(storyfmv_cro06_coh06a);
                storyfmv_cro06_coh06a->SetExitMenuAfterSelected(true);
                MenuWidget *storyfmv_dda01 = new ("ActionWidget", 0) ActionWidget("storyfmv_dda01", PlayStoryfmv_dda01);
                DebugMenu::addWidgetToNewestMenu(storyfmv_dda01);
                storyfmv_dda01->SetExitMenuAfterSelected(true);
                MenuWidget *storyfmv_epil138 = new ("ActionWidget", 0) ActionWidget("storyfmv_epil138", PlayStoryfmv_epi138);
                DebugMenu::addWidgetToNewestMenu(storyfmv_epil138);
                storyfmv_epil138->SetExitMenuAfterSelected(true);
                MenuWidget *storyfmv_her136 = new ("ActionWidget", 0) ActionWidget("storyfmv_her136", PlayStoryfmv_her136);
                DebugMenu::addWidgetToNewestMenu(storyfmv_her136);
                storyfmv_her136->SetExitMenuAfterSelected(true);
                MenuWidget *storyfmv_pin11 = new ("ActionWidget", 0) ActionWidget("storyfmv_pin11", PlayStoryfmv_pin11);
                DebugMenu::addWidgetToNewestMenu(storyfmv_pin11);
                storyfmv_pin11->SetExitMenuAfterSelected(true);
                MenuWidget *storyfmv_pol17_mot21 = new ("ActionWidget", 0) ActionWidget("storyfmv_pol17_mot21", PlayStoryfmv_pol17_mot21);
                DebugMenu::addWidgetToNewestMenu(storyfmv_pol17_mot21);
                storyfmv_pol17_mot21->SetExitMenuAfterSelected(true);
                MenuWidget *storyfmv_rap30 = new ("ActionWidget", 0) ActionWidget("storyfmv_rap30", PlayStoryfmv_rap30);
                DebugMenu::addWidgetToNewestMenu(storyfmv_rap30);
                storyfmv_rap30->SetExitMenuAfterSelected(true);
                MenuWidget *storyfmv_raz08 = new ("ActionWidget", 0) ActionWidget("storyfmv_raz08", PlayStoryfmv_raz08);
                DebugMenu::addWidgetToNewestMenu(storyfmv_raz08);
                storyfmv_raz08->SetExitMenuAfterSelected(true);
                MenuWidget *storyfmv_roc02 = new ("ActionWidget", 0) ActionWidget("storyfmv_roc02", PlayStoryfmv_roc02);
                DebugMenu::addWidgetToNewestMenu(storyfmv_roc02);
                storyfmv_roc02->SetExitMenuAfterSelected(true);
                MenuWidget *storyfmv_saf25 = new ("ActionWidget", 0) ActionWidget("storyfmv_saf25", PlayStoryfmv_saf25);
                DebugMenu::addWidgetToNewestMenu(storyfmv_saf25);
                storyfmv_saf25->SetExitMenuAfterSelected(true);
                DebugMenu::endNewestMenu();
            }
            DebugMenu::create("Demo FMV ->", FMV);
            {
                MenuWidget *SSX_OT_n_480 = new ("ActionWidget", 0) ActionWidget("SSX_OT_n_480", PlaySSX_OT_n_480);
                DebugMenu::addWidgetToNewestMenu(SSX_OT_n_480);
                SSX_OT_n_480->SetExitMenuAfterSelected(true);
                MenuWidget *NBALive_2006 = new ("ActionWidget", 0) ActionWidget("NBALive_2006", PlayNBALive_2006);
                DebugMenu::addWidgetToNewestMenu(NBALive_2006);
                NBALive_2006->SetExitMenuAfterSelected(true);
                MenuWidget *MarvNM_480 = new ("ActionWidget", 0) ActionWidget("MarvNM_480", PlayMarvNM_480);
                DebugMenu::addWidgetToNewestMenu(MarvNM_480);
                MarvNM_480->SetExitMenuAfterSelected(true);
                DebugMenu::endNewestMenu();
            }
            MenuWidget *MW_LS_IntroFMV2 = new ("ActionWidget", 0) ActionWidget("MW_LS_IntroFMV 2", Play2MW_LS_IntroFMV);
            DebugMenu::addWidgetToNewestMenu(MW_LS_IntroFMV2);
            MW_LS_IntroFMV2->SetExitMenuAfterSelected(true);
            MenuWidget *MW_LS_AttractFMV2 = new ("ActionWidget", 0) ActionWidget("MW_LS_AttractFMV 2", Play2MW_LS_AttractFMV);
            DebugMenu::addWidgetToNewestMenu(MW_LS_AttractFMV2);
            MW_LS_AttractFMV2->SetExitMenuAfterSelected(true);
            MenuWidget *LS_EALogo = new ("ActionWidget", 0) ActionWidget("MW_LS_EALogo", PlayLS_EALogo);
            DebugMenu::addWidgetToNewestMenu(LS_EALogo);
            LS_EALogo->SetExitMenuAfterSelected(true);
            MenuWidget *drag_tutorial = new ("ActionWidget", 0) ActionWidget("drag_tutorial", PlayDrag_tutorial);
            DebugMenu::addWidgetToNewestMenu(drag_tutorial);
            drag_tutorial->SetExitMenuAfterSelected(true);
            MenuWidget *sprint_tutorial = new ("ActionWidget", 0) ActionWidget("sprint_tutorial", PlaySprint_tutorial);
            DebugMenu::addWidgetToNewestMenu(sprint_tutorial);
            sprint_tutorial->SetExitMenuAfterSelected(true);
            MenuWidget *tollbooth_tutorial = new ("ActionWidget", 0) ActionWidget("tollbooth_tutorial", PlayTollbooth_tutorial);
            DebugMenu::addWidgetToNewestMenu(tollbooth_tutorial);
            tollbooth_tutorial->SetExitMenuAfterSelected(true);
            MenuWidget *pursuit_tutorial = new ("ActionWidget", 0) ActionWidget("pursuit_tutorial", PlayPursuit_tutorial);
            DebugMenu::addWidgetToNewestMenu(pursuit_tutorial);
            pursuit_tutorial->SetExitMenuAfterSelected(true);
            MenuWidget *bounty_tutorial = new ("ActionWidget", 0) ActionWidget("bounty_tutorial", PlayBounty_tutorial);
            DebugMenu::addWidgetToNewestMenu(bounty_tutorial);
            bounty_tutorial->SetExitMenuAfterSelected(true);
            MenuWidget *psa = new ("ActionWidget", 0) ActionWidget("psa", PlayPsa);
            DebugMenu::addWidgetToNewestMenu(psa);
            psa->SetExitMenuAfterSelected(true);
            DebugMenu::endNewestMenu();
        }
        DebugMenu::create("Rub Test ->", root);
        {
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Draw Collision Surfaces", &Tweak_barrierDraw, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Draw Triggers", &Tweak_triggerDraw, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Draw Objects", &Tweak_objectDraw, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("Draw Rigid Bodies", &gRigidBodyVol, TriggerRigidBodyVol));
            DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("Draw World", &DrawWorld, 0, 1, 1, nullptr));
            DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("Draw Cars", &DrawCars, 0, 1, 1, nullptr));
            DebugMenu::endNewestMenu();
        }
        DebugMenu::create("Screenshot ->", root);
        {
            MenuWidget *screenshot = new ("ActionWidget", 0) ActionWidget("Take Screen Shot", TakeScreenShot);
            DebugMenu::addWidgetToNewestMenu(screenshot);
            screenshot->SetExitMenuAfterSelected(true);
            DebugMenu::addWidgetToNewestMenu(new ("IntWidget", 0) IntWidget("Resolution", &ScreenShotSampleRez, 0, 5, 1, nullptr));
            DebugMenu::endNewestMenu();
        }
        DebugMenu *overlay = DebugMenu::create("Overlays ->", frontend_menu);
        {
            DebugMenu::create("FadeScreens ->", overlay);
            {
                MenuWidget *fadescreen_on = new ("ActionWidget", 0) ActionWidget("Fade Screen On", FadeScreenOn);
                DebugMenu::addWidgetToNewestMenu(fadescreen_on);
                fadescreen_on->SetExitMenuAfterSelected(true);
                MenuWidget *fadescreen_off = new ("ActionWidget", 0) ActionWidget("Fade Screen Off", FadeScreenOff);
                DebugMenu::addWidgetToNewestMenu(fadescreen_off);
                fadescreen_off->SetExitMenuAfterSelected(true);
                DebugMenu::endNewestMenu();
            }
            DebugMenu::create("Loading Screen ->", overlay);
            {
                MenuWidget *Loading = new ("ActionWidget", 0) ActionWidget("Loading", OverlayLoading);
                DebugMenu::addWidgetToNewestMenu(Loading);
                Loading->SetExitMenuAfterSelected(true);
                MenuWidget *Loading_Controller = new ("ActionWidget", 0) ActionWidget("Loading_Controller", OverlayLoading_Controller);
                DebugMenu::addWidgetToNewestMenu(Loading_Controller);
                Loading_Controller->SetExitMenuAfterSelected(true);
                DebugMenu::endNewestMenu();
            }
            DebugMenu::create("Mainmenu ->", overlay);
            {
                MenuWidget *overlay_main_menu = new ("ActionWidget", 0) ActionWidget("Main Menu", OverlayMainMenu);
                DebugMenu::addWidgetToNewestMenu(overlay_main_menu);
                overlay_main_menu->SetExitMenuAfterSelected(true);
                MenuWidget *main_menu_sub = new ("ActionWidget", 0) ActionWidget("Main Menu Sub", OverlayMainMenu_Sub);
                DebugMenu::addWidgetToNewestMenu(main_menu_sub);
                main_menu_sub->SetExitMenuAfterSelected(true);
                MenuWidget *options = new ("ActionWidget", 0) ActionWidget("Options", OverlayOptions);
                DebugMenu::addWidgetToNewestMenu(options);
                options->SetExitMenuAfterSelected(true);
                MenuWidget *quick_race_brief = new ("ActionWidget", 0) ActionWidget("Quick Race Brief", OverlayQRBrief);
                DebugMenu::addWidgetToNewestMenu(quick_race_brief);
                quick_race_brief->SetExitMenuAfterSelected(true);
                MenuWidget *track_select = new ("ActionWidget", 0) ActionWidget("Track Select", OverlayTrackSelect);
                DebugMenu::addWidgetToNewestMenu(track_select);
                track_select->SetExitMenuAfterSelected(true);
                DebugMenu::endNewestMenu();
            }
            DebugMenu::create("Gameplay -> (Must be in race mode)", overlay);
            {
                MenuWidget *Pause_Main = new ("ActionWidget", 0) ActionWidget("Pause_Main", OverlayPause_Main);
                DebugMenu::addWidgetToNewestMenu(Pause_Main);
                Pause_Main->SetExitMenuAfterSelected(true);
                MenuWidget *Pause_Performance_Tuning = new ("ActionWidget", 0) ActionWidget("Pause_Performance_Tuning", OverlayPause_Performance_Tuning);
                DebugMenu::addWidgetToNewestMenu(Pause_Performance_Tuning);
                Pause_Performance_Tuning->SetExitMenuAfterSelected(true);
                MenuWidget *WorldMapMain = new ("ActionWidget", 0) ActionWidget("WorldMapMain", OverlayWorldMapMain);
                DebugMenu::addWidgetToNewestMenu(WorldMapMain);
                WorldMapMain->SetExitMenuAfterSelected(true);
                MenuWidget *Pause_Options = new ("ActionWidget", 0) ActionWidget("Pause_Options", OverlayPause_Options);
                DebugMenu::addWidgetToNewestMenu(Pause_Options);
                Pause_Options->SetExitMenuAfterSelected(true);
                MenuWidget *InGameReputationOverview = new ("ActionWidget", 0) ActionWidget("InGameReputationOverview", OverlayInGameReputationOverview);
                DebugMenu::addWidgetToNewestMenu(InGameReputationOverview);
                InGameReputationOverview->SetExitMenuAfterSelected(true);
                MenuWidget *InGameMilestones = new ("ActionWidget", 0) ActionWidget("InGameMilestones", OverlayInGameMilestones);
                DebugMenu::addWidgetToNewestMenu(InGameMilestones);
                InGameMilestones->SetExitMenuAfterSelected(true);
                MenuWidget *InGameRivalChallenge = new ("ActionWidget", 0) ActionWidget("InGameRivalChallenge", OverlayInGameRivalChallenge);
                DebugMenu::addWidgetToNewestMenu(InGameRivalChallenge);
                InGameRivalChallenge->SetExitMenuAfterSelected(true);
                DebugMenu::endNewestMenu();
            }
            DebugMenu::create("Career ->", overlay);
            {
                MenuWidget *SafehouseReputationOverview = new ("ActionWidget", 0) ActionWidget("SafehouseReputationOverview", OverlaySafehouseReputationOverview);
                DebugMenu::addWidgetToNewestMenu(SafehouseReputationOverview);
                SafehouseReputationOverview->SetExitMenuAfterSelected(true);
                MenuWidget *RapSheetOverview = new ("ActionWidget", 0) ActionWidget("RapSheetOverview", OverlayRapSheetOverview);
                DebugMenu::addWidgetToNewestMenu(RapSheetOverview);
                RapSheetOverview->SetExitMenuAfterSelected(true);
                MenuWidget *RapSheetRankings = new ("ActionWidget", 0) ActionWidget("RapSheetRankings", OverlayRapSheetRankings);
                DebugMenu::addWidgetToNewestMenu(RapSheetRankings);
                RapSheetRankings->SetExitMenuAfterSelected(true);
                MenuWidget *RapSheetInfractions = new ("ActionWidget", 0) ActionWidget("RapSheetInfractions", OverlayRapSheetInfractions);
                DebugMenu::addWidgetToNewestMenu(RapSheetInfractions);
                RapSheetInfractions->SetExitMenuAfterSelected(true);
                MenuWidget *RapSheetCostToState = new ("ActionWidget", 0) ActionWidget("RapSheetCostToState", OverlayRapSheetCostToState);
                DebugMenu::addWidgetToNewestMenu(RapSheetCostToState);
                RapSheetCostToState->SetExitMenuAfterSelected(true);
                MenuWidget *SafehouseRivalChallenge = new ("ActionWidget", 0) ActionWidget("SafehouseRivalChallenge", OverlaySafehouseRivalChallenge);
                DebugMenu::addWidgetToNewestMenu(SafehouseRivalChallenge);
                SafehouseRivalChallenge->SetExitMenuAfterSelected(true);
                MenuWidget *SafehouseMilestones = new ("ActionWidget", 0) ActionWidget("SafehouseMilestones", OverlaySafehouseMilestones);
                DebugMenu::addWidgetToNewestMenu(SafehouseMilestones);
                SafehouseMilestones->SetExitMenuAfterSelected(true);
                MenuWidget *BlackList = new ("ActionWidget", 0) ActionWidget("BlackList", OverlayBlackList);
                DebugMenu::addWidgetToNewestMenu(BlackList);
                BlackList->SetExitMenuAfterSelected(true);
                MenuWidget *ControllerUnplugged = new ("ActionWidget", 0) ActionWidget("ControllerUnplugged", OverlayControllerUnplugged);
                DebugMenu::addWidgetToNewestMenu(ControllerUnplugged);
                ControllerUnplugged->SetExitMenuAfterSelected(true);
                DebugMenu::endNewestMenu();
            }
            DebugMenu::create("Customization -> (MUST be in Customized Car Screen)", overlay);
            {
                MenuWidget *MyCarsManager = new ("ActionWidget", 0) ActionWidget("MyCarsManager", OverlayMyCarsManager);
                DebugMenu::addWidgetToNewestMenu(MyCarsManager);
                MyCarsManager->SetExitMenuAfterSelected(true);
                MenuWidget *UI_DebugCarCustomize = new ("ActionWidget", 0) ActionWidget("UI_DebugCarCustomize", OverlayUI_DebugCarCustomize);
                DebugMenu::addWidgetToNewestMenu(UI_DebugCarCustomize);
                UI_DebugCarCustomize->SetExitMenuAfterSelected(true);
                MenuWidget *CustomizeParts = new ("ActionWidget", 0) ActionWidget("CustomizeParts", OverlayCustomizeParts);
                DebugMenu::addWidgetToNewestMenu(CustomizeParts);
                CustomizeParts->SetExitMenuAfterSelected(true);
                MenuWidget *CustomizeParts_BACKROOM = new ("ActionWidget", 0) ActionWidget("CustomizeParts_BACKROOM", OverlayCustomizeParts_BACKROOM);
                DebugMenu::addWidgetToNewestMenu(CustomizeParts_BACKROOM);
                CustomizeParts_BACKROOM->SetExitMenuAfterSelected(true);
                MenuWidget *CustomHUD = new ("ActionWidget", 0) ActionWidget("CustomizeCategory", OverlayCustomHUD);
                DebugMenu::addWidgetToNewestMenu(CustomHUD);
                CustomHUD->SetExitMenuAfterSelected(true);
                MenuWidget *CustomHUD_BACKROOM = new ("ActionWidget", 0) ActionWidget("CustomHUD_BACKROOM", OverlayCustomHUD_BACKROOM);
                DebugMenu::addWidgetToNewestMenu(CustomHUD_BACKROOM);
                CustomHUD_BACKROOM->SetExitMenuAfterSelected(true);
                MenuWidget *Decals = new ("ActionWidget", 0) ActionWidget("Decals", OverlayDecals);
                DebugMenu::addWidgetToNewestMenu(Decals);
                Decals->SetExitMenuAfterSelected(true);
                MenuWidget *Decals_BACKROOM = new ("ActionWidget", 0) ActionWidget("Decals_BACKROOM", OverlayDecals_BACKROOM);
                DebugMenu::addWidgetToNewestMenu(Decals_BACKROOM);
                Decals_BACKROOM->SetExitMenuAfterSelected(true);
                MenuWidget *Numbers = new ("ActionWidget", 0) ActionWidget("Numbers", OverlayNumbers);
                DebugMenu::addWidgetToNewestMenu(Numbers);
                Numbers->SetExitMenuAfterSelected(true);
                MenuWidget *Rims_BACKROOM = new ("ActionWidget", 0) ActionWidget("Rims_BACKROOM", OverlayRims);
                DebugMenu::addWidgetToNewestMenu(Rims_BACKROOM);
                Rims_BACKROOM->SetExitMenuAfterSelected(true);
                DebugMenu::endNewestMenu();
            }
            DebugMenu::create("Misc ->", overlay);
            {
                MenuWidget *Keyboard = new ("ActionWidget", 0) ActionWidget("Keyboard", OverlayKeyboard);
                DebugMenu::addWidgetToNewestMenu(Keyboard);
                Keyboard->SetExitMenuAfterSelected(true);
                MenuWidget *LS_LangSelect = new ("ActionWidget", 0) ActionWidget("LS_LangSelect", OverlayLS_LangSelect);
                DebugMenu::addWidgetToNewestMenu(LS_LangSelect);
                LS_LangSelect->SetExitMenuAfterSelected(true);
                MenuWidget *Loading_Controller = new ("ActionWidget", 0) ActionWidget("Loading_Controller", OverlayLoading_Controller);
                DebugMenu::addWidgetToNewestMenu(Loading_Controller);
                Loading_Controller->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OptionsController = new ("ActionWidget", 0) ActionWidget("UI_OptionsController", OverlayUI_OptionsController);
                DebugMenu::addWidgetToNewestMenu(UI_OptionsController);
                UI_OptionsController->SetExitMenuAfterSelected(true);
                DebugMenu::endNewestMenu();
            }
            DebugMenu::create("Online -> (MUST be in ONLINE connected)", overlay);
            {
                MenuWidget *OL_News_and_Terms = new ("ActionWidget", 0) ActionWidget("OL_News_and_Terms", OverlayOL_News_and_Terms);
                DebugMenu::addWidgetToNewestMenu(OL_News_and_Terms);
                OL_News_and_Terms->SetExitMenuAfterSelected(true);
                MenuWidget *OL_LobbyRoom = new ("ActionWidget", 0) ActionWidget("OL_LobbyRoom", OverlayOL_LobbyRoom);
                DebugMenu::addWidgetToNewestMenu(OL_LobbyRoom);
                OL_LobbyRoom->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OLGameRoom = new ("ActionWidget", 0) ActionWidget("UI_OLGameRoom", OverlayUI_OLGameRoom);
                DebugMenu::addWidgetToNewestMenu(UI_OLGameRoom);
                UI_OLGameRoom->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OLGameRoom_host = new ("ActionWidget", 0) ActionWidget("UI_OLGameRoom_host", OverlayUI_OLGameRoom_host);
                DebugMenu::addWidgetToNewestMenu(UI_OLGameRoom_host);
                UI_OLGameRoom_host->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OLGameRoom_client = new ("ActionWidget", 0) ActionWidget("UI_OLGameRoom_client", OverlayUI_OLGameRoom_client);
                DebugMenu::addWidgetToNewestMenu(UI_OLGameRoom_client);
                UI_OLGameRoom_client->SetExitMenuAfterSelected(true);
                MenuWidget *OL_ModeSelectList = new ("ActionWidget", 0) ActionWidget("OL_ModeSelectList", OverlayOL_ModeSelectList);
                DebugMenu::addWidgetToNewestMenu(OL_ModeSelectList);
                OL_ModeSelectList->SetExitMenuAfterSelected(true);
                MenuWidget *OL_MAIN = new ("ActionWidget", 0) ActionWidget("OL_MAIN", OverlayOL_MAIN);
                DebugMenu::addWidgetToNewestMenu(OL_MAIN);
                OL_MAIN->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OLX_QuickRace = new ("ActionWidget", 0) ActionWidget("UI_OLX_QuickRace", OverlayUI_OLX_QuickRace);
                DebugMenu::addWidgetToNewestMenu(UI_OLX_QuickRace);
                UI_OLX_QuickRace->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OLFilters = new ("ActionWidget", 0) ActionWidget("UI_OLFilters", OverlayUI_OLFilters);
                DebugMenu::addWidgetToNewestMenu(UI_OLFilters);
                UI_OLFilters->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OLX_OptiMatch_Available = new ("ActionWidget", 0) ActionWidget("UI_OLX_OptiMatch_Available", OverlayUI_OLX_OptiMatch_Available);
                DebugMenu::addWidgetToNewestMenu(UI_OLX_OptiMatch_Available);
                UI_OLX_OptiMatch_Available->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OLX_OptiMatch_Filters = new ("ActionWidget", 0) ActionWidget("UI_OLX_OptiMatch_Filters", OverlayUI_OLX_OptiMatch_Filters);
                DebugMenu::addWidgetToNewestMenu(UI_OLX_OptiMatch_Filters);
                UI_OLX_OptiMatch_Filters->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OLRankings_Personal = new ("ActionWidget", 0) ActionWidget("UI_OLRankings_Personal", OverlayUI_OLRankings_Personal);
                DebugMenu::addWidgetToNewestMenu(UI_OLRankings_Personal);
                UI_OLRankings_Personal->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OLRankings_Overall = new ("ActionWidget", 0) ActionWidget("UI_OLRankings_Overall", OverlayUI_OLRankings_Overall);
                DebugMenu::addWidgetToNewestMenu(UI_OLRankings_Overall);
                UI_OLRankings_Overall->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OLRankings_Monthly = new ("ActionWidget", 0) ActionWidget("UI_OLRankings_Monthly", OverlayUI_OLRankings_Monthly);
                DebugMenu::addWidgetToNewestMenu(UI_OLRankings_Monthly);
                UI_OLRankings_Monthly->SetExitMenuAfterSelected(true);
                MenuWidget *OL_FriendDialogue = new ("ActionWidget", 0) ActionWidget("OL_FriendDialogue", OverlayOL_FriendDialogue);
                DebugMenu::addWidgetToNewestMenu(OL_FriendDialogue);
                OL_FriendDialogue->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OL_Feedback = new ("ActionWidget", 0) ActionWidget("UI_OL_Feedback", OverlayUI_OL_Feedback);
                DebugMenu::addWidgetToNewestMenu(UI_OL_Feedback);
                UI_OL_Feedback->SetExitMenuAfterSelected(true);
                MenuWidget *OL_VoiceChat = new ("ActionWidget", 0) ActionWidget("OL_VoiceChat", OverlayOL_VoiceChat);
                DebugMenu::addWidgetToNewestMenu(OL_VoiceChat);
                OL_VoiceChat->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OLAuthDNAS = new ("ActionWidget", 0) ActionWidget("UI_OLAuthDNAS", OverlayUI_OLAuthDNAS);
                DebugMenu::addWidgetToNewestMenu(UI_OLAuthDNAS);
                UI_OLAuthDNAS->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OLISPConnect = new ("ActionWidget", 0) ActionWidget("UI_OLISPConnect", OverlayUI_OLISPConnect);
                DebugMenu::addWidgetToNewestMenu(UI_OLISPConnect);
                UI_OLISPConnect->SetExitMenuAfterSelected(true);
                MenuWidget *OL_SelectPersona = new ("ActionWidget", 0) ActionWidget("OL_SelectPersona", OverlayUI_OLSelectPersona);
                DebugMenu::addWidgetToNewestMenu(OL_SelectPersona);
                OL_SelectPersona->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OLCreateUser = new ("ActionWidget", 0) ActionWidget("UI_OLCreateAccount", OverlayUI_OLCreateUser);
                DebugMenu::addWidgetToNewestMenu(UI_OLCreateUser);
                UI_OLCreateUser->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OLAgeVerif = new ("ActionWidget", 0) ActionWidget("UI_OLAgeVerif", OverlayUI_OLAgeVerif);
                DebugMenu::addWidgetToNewestMenu(UI_OLAgeVerif);
                UI_OLAgeVerif->SetExitMenuAfterSelected(true);
                MenuWidget *UI_OLAgeTooYoung = new ("ActionWidget", 0) ActionWidget("UI_OLAgeTooYoung", OverlayUI_OLAgeTooYoung);
                DebugMenu::addWidgetToNewestMenu(UI_OLAgeTooYoung);
                UI_OLAgeTooYoung->SetExitMenuAfterSelected(true);
                MenuWidget *OL_UseExisting = new ("ActionWidget", 0) ActionWidget("OL_UseExisting", OverlayUI_OLUseExisting);
                DebugMenu::addWidgetToNewestMenu(OL_UseExisting);
                OL_UseExisting->SetExitMenuAfterSelected(true);
                MenuWidget *UI_DateEntry = new ("ActionWidget", 0) ActionWidget("UI_DateEntry", OverlayUI_DateEntry);
                DebugMenu::addWidgetToNewestMenu(UI_DateEntry);
                UI_DateEntry->SetExitMenuAfterSelected(true);
                DebugMenu::endNewestMenu();
            }
            DebugMenu::create("Memory Card ->", overlay);
            {
                MenuWidget *MC_ProfileManager = new ("ActionWidget", 0) ActionWidget("MC_ProfileManager", OverlayMC_ProfileManager);
                DebugMenu::addWidgetToNewestMenu(MC_ProfileManager);
                MC_ProfileManager->SetExitMenuAfterSelected(true);
                DebugMenu::endNewestMenu();
            }
            DebugMenu::endNewestMenu();
        }
        MenuWidget *joystick_monkey = new ("ActionWidget", 0) ActionWidget("Toggle Joystick Monkey", TriggerJoystickMonkey);
        DebugMenu::addWidgetToNewestMenu(joystick_monkey);
        joystick_monkey->SetExitMenuAfterSelected(true);
        DebugMenu::addWidgetToNewestMenu(new ("BoolWidget", 0) BoolWidget("NIS Text Display", reinterpret_cast<bool *>(&bEnableNisTextDisplay), nullptr));
    }

    if (gLastCurMenuName) {
        DebugMenu::SetCurrentMenu();
    }
}

void GetStringSize(const char *str, int *x, int *y) {
    float rx = 0.0f;
    float ry = 0.0f;

    ScreenPrintItem *item = GetUnusedScreenPrintItem();
    if (item) {
        FEString *text_chunk = item->pFEString;
        if (text_chunk) {
            FEPrintf(text_chunk, str);
            FEngGetSize(text_chunk, rx, ry);
        }
    }

    *x = static_cast<int>(rx);
    *y = static_cast<int>(ry);
}

DebugMenuInputHandler::DebugMenuInputHandler()
    : mActiveSpeed(false), //
      mPrevPrintfState(false), //
      mPauseWorld(false), //
      mActiveShift(false), //
      mDisabledList(nullptr) {
    mActionQ = new ActionQueue(0, 0x28694659, "tedebugmenu", false);
}

void DebugMenuInputHandler::PollInput() {
    static int last_watch_index = -1;
    static HSIMABLE last_watch_car = nullptr;

    int count = IVehicle::Count(mToggleCarList);
    if (CameraDebugWatchCar && count > 0) {
        if (last_watch_index == mToggleCar) {
            for (int i = 0; i < count; i++) {
                if (IVehicle::GetList(mToggleCarList)[i]->GetSimable()->GetOwnerHandle() == last_watch_car) {
                    mToggleCar = i;
                    break;
                }
            }
        }
        last_watch_index = mToggleCar;
        last_watch_car = IVehicle::GetList(mToggleCarList)[mToggleCar % count]->GetSimable()->GetOwnerHandle();
    } else {
        last_watch_index = -1;
        last_watch_car = nullptr;
    }

    while (mActionQ && !mActionQ->IsEmpty()) {
        ActionRef aRef = mActionQ->GetAction();

        switch (aRef.ID()) {
            case FRONTENDACTION_BUTTON0:
                if (CameraDebugWatchCar) {
                    WatchCarCallback(-1);
                }
                if (LanguageCyclingEnabled) {
                    LanguageChangerCallback(-1);
                }
                break;

            case FRONTENDACTION_BUTTON1:
                if (CameraDebugWatchCar) {
                    WatchCarCallback(1);
                }
                if (LanguageCyclingEnabled) {
                    LanguageChangerCallback(1);
                }
                break;

            case FRONTENDACTION_RTRIGGER:
                if (aRef.Data() == 1.0f) {
                    mActiveShift = true;
                } else if (aRef.Data() == 0.0f) {
                    mActiveShift = false;
                }
                break;

            case FRONTENDACTION_LTRIGGER:
                if (aRef.Data() == 1.0f && mActiveShift) {
                    if (DebugMenu::isActive()) {
                        DebugMenu::deactivate();
                        Deactivate();
                    } else {
                        DebugMenu::activate();
                        Activate();
                    }
                }
                break;

            case FRONTENDACTION_UP:
                if (DebugMenu::isActive()) {
                    DebugMenu::eventUp();
                }
                break;

            case FRONTENDACTION_DOWN:
                if (DebugMenu::isActive()) {
                    DebugMenu::eventDown();
                }
                break;

            case FRONTENDACTION_LEFT:
                if (DebugMenu::isActive()) {
                    DebugMenu::eventLeft(false);
                }
                break;

            case FRONTENDACTION_RIGHT:
                if (DebugMenu::isActive()) {
                    DebugMenu::eventRight(false);
                }
                break;

            case FRONTENDACTION_ACCEPT:
                if (DebugMenu::isActive()) {
                    DebugMenu::eventSelect();
                }
                break;

            case FRONTENDACTION_CANCEL:
                if (DebugMenu::isActive()) {
                    DebugMenu::eventCancel();
                }
                break;

            case FRONTENDACTION_CANCEL_ALT:
                if (mActiveSpeed) {
                    TriggerSetSpeed();
                }
                break;
        }

        mActionQ->PopAction();
    }
}

void DebugMenuInputHandler::Exit() {
    if (DebugMenu::isActive()) {
        DebugMenu::deactivate();
        Deactivate();
    }
}

void DebugMenuInputHandler::Activate() {
    mPrevFEngDrawState = DrawFEng;
    DrawFEng = true;

    mPrevPrintfState = DoScreenPrintf;
    DoScreenPrintf = true;

    if (TheGameFlowManager.IsInGame()) {
        UpdateHeat();
    }

    SetJumpToBinText();

    ActionQueueNode *currQNode = new ("NFSDebugMenu ActionQ Disabler", 0) ActionQueueNode;
    currQNode->actionQ = nullptr;
    currQNode->next = nullptr;
    mDisabledList = currQNode;

    for (ActionQueue::List::const_iterator iter = ActionQueue::GetList().begin(); iter != ActionQueue::GetList().end();
         ++iter) {
        ActionQueue *thisQ = *iter;
        if (thisQ->IsEnabled() && thisQ != mActionQ) {
            thisQ->Enable(false);
            currQNode->actionQ = thisQ;
            currQNode->next = new ("NFSDebugMenu ActionQ Disabler", 0) ActionQueueNode;
            currQNode = currQNode->next;
            currQNode->actionQ = nullptr;
            currQNode->next = nullptr;
        }
    }

    if (!DebugMenu::getRenderer()) {
        DebugMenuAllocate();
    }

    DebugMenuLoad();
}

void DebugMenuInputHandler::Deactivate() {
    ActionQueueNode *currQNode;
    ActionQueueNode *prevQNode;

    DrawFEng = mPrevFEngDrawState;
    DoScreenPrintf = mPrevPrintfState;

    currQNode = mDisabledList;
    while (currQNode) {
        if (currQNode->actionQ) {
            currQNode->actionQ->Enable(true);
        }
        prevQNode = currQNode;
        currQNode = currQNode->next;
        delete prevQNode;
    }
    mDisabledList = nullptr;

    DebugMenuUnload();
}

void NFSMenuRender::print(int x, int y, unsigned int color, const char *str) {
    DoScreenPrintf = 1;
    ScreenPrintf(x - 320, y - 240, color, str);
    DoScreenPrintf = 0;
}

void NFSMenuRender::getStringSize(const char *str, int *x, int *y) {
    GetStringSize(str, x, y);
}

void NFSMenuRender::renderOverlay(int x, int y, int width, int height, float alpha, unsigned int color) {
    if (FEDatabase && FEDatabase->GetVideoSettings()->WideScreen) {
        x -= 120;
        width += 240;
    }

    ePoly poly;
    TextureInfo *tex;

    poly.Vertices[0].x = x;
    poly.Vertices[0].y = y;
    poly.Vertices[1].x = x + width;
    poly.Vertices[1].y = y;
    poly.Vertices[2].x = x + width;
    poly.Vertices[2].y = y + height;
    poly.Vertices[3].x = x;
    poly.Vertices[3].y = y + height;
#ifdef EA_PLATFORM_XENON
    // Xenon: la z de los cuatro vertices a 0 y sin Flailer (EGM 0x822AE490).
    poly.Vertices[0].z = 0.0f;
    poly.Vertices[1].z = 0.0f;
    poly.Vertices[2].z = 0.0f;
    poly.Vertices[3].z = 0.0f;
#else
    poly.SetFlailer(1);
#endif

    reinterpret_cast<unsigned int *>(poly.Colours)[0] = (static_cast<int>(alpha * 255.0f) << 24) + color;
    reinterpret_cast<unsigned int *>(poly.Colours)[1] = (static_cast<int>(alpha * 255.0f) << 24) + color;
    reinterpret_cast<unsigned int *>(poly.Colours)[2] = (static_cast<int>(alpha * 255.0f) << 24) + color;
    reinterpret_cast<unsigned int *>(poly.Colours)[3] = (static_cast<int>(alpha * 255.0f) << 24) + color;

    if (alpha >= 1.0f) {
        tex = GetTextureInfo(bStringHash("WHITE32X32"), 1, 0);
    } else {
        tex = GetTextureInfo(bStringHash("WHITE16X16"), 1, 0);
    }

    eGetView(1, false)->FEBeginBatchRender(1);
    eGetView(1, false)->FERender(&poly, tex, 0);
    eGetView(1, false)->FEEndBatchRender();
}

DebugMenuInputHandler *gDebugMenuIOHandler;
