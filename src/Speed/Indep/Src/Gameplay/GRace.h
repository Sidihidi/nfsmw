#ifndef GAMEPLAY_GRACE_H
#define GAMEPLAY_GRACE_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "types.h"

namespace GRace {

enum Difficulty {
    kRaceDifficulty_Easy = 0,
    kRaceDifficulty_Medium = 1,
    kRaceDifficulty_Hard = 2,
    kRaceDifficulty_NumDifficulties = 3,
};

// Del DWARF, junto a Difficulty y con sus reflection_typedef.
enum TrafficLevel {
    kRaceTraffic_Off = 0,
    kRaceTraffic_Light = 1,
    kRaceTraffic_Medium = 2,
    kRaceTraffic_Heavy = 3,
    kRaceTraffic_NumLevels = 4,
};

enum CopDensity {
    kRaceCops_Off = 0,
    kRaceCops_Light = 1,
    kRaceCops_Medium = 2,
    kRaceCops_Heavy = 3,
    kRaceCops_NumDensities = 4,
};

// El prototipo A124 no tiene contrarreloj y si tiene online: su .mdebug da
// QuickRace = 0, Online = 1, Career = 2. La contrarreloj se anadio despues, y la
// version de venta de PS2 ya no tiene online, asi que alli Career tambien vale
// 2 pero por el otro lado. Los sitios que en la venta miran kRaceContext_TimeTrial
// miran kRaceContext_Online en el prototipo: mismo valor 1, mismo codigo maquina
// (PauseMenu::Setup esta al 100 % en SLES comparando con 1).
//
// En PC (WIN32) Career va ANTES que Online (QuickRace = 0, TimeTrial = 1,
// Career = 2, Online = 3): el SPEED_EXE_1_3 original compara el contexto con 2
// en los sitios que miran GRace::kRaceContext_Career
// (HudResourceManager::ChooseLoadableTextures, HudResourceManager::GetCarPart,
// PhotoFinishScreen::PhotoFinishScreen: `cmp [reg+0x1964], 0x2`), mientras que
// con Career = 3 compilabamos `cmp ..., 0x3` (ventana 8, corriente 6: +3 en zFe2).
enum Context {
    kRaceContext_QuickRace,
#ifdef EA_BUILD_A124
    kRaceContext_Online,
    kRaceContext_Career,
#else
    kRaceContext_TimeTrial,
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON) // 360 igual que PC: GManager::PreBeginGameplay guarda 2
    kRaceContext_Career,
#if ONLINE_SUPPORT
    kRaceContext_Online,
#endif
#else
#if ONLINE_SUPPORT
    kRaceContext_Online,
#endif
    kRaceContext_Career,
#endif
#endif
    kRaceContext_Count,
};

// Del DWARF: GRace::Region.
enum Region {
    kRaceRegion_None = -1,
    kRaceRegion_College = 0,
    kRaceRegion_Coastal = 1,
    kRaceRegion_City = 2,
    kRaceRegion_NumRegions = 3,
};

enum Type {
    kRaceType_None = -1,
    kRaceType_P2P = 0,
    kRaceType_Circuit = 1,
    kRaceType_Drag = 2,
    kRaceType_Knockout = 3,
    kRaceType_Tollbooth = 4,
    kRaceType_SpeedTrap = 5,
    kRaceType_Checkpoint = 6,
    kRaceType_CashGrab = 7,
    kRaceType_Challenge = 8,
    kRaceType_JumpToSpeedTrap = 9,
    kRaceType_JumpToMilestone = 10,
    kRaceType_NumTypes = 11,
};

}; // namespace GRace

enum eRaceStage {
    RACE_STAGE_NONE = 0,
    RACE_STAGE_FLYBY = 1,
    RACE_STAGE_PRERACE = 2,
    RACE_STAGE_COUNTDOWN = 3,
    RACE_STAGE_RACING = 4,
};

#endif
