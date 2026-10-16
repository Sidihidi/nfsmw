// Prefijo comun de las unidades de juego del original (como zFEng.cpp): en la A124 los 402 primeros tipos de
// los stabs son los mismos en todas, y la .rodata empieza por las cadenas de bWare.hpp.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"

// datos-gc 2026-10-13: orden de parseo de la CU original (DWARF): MGeneric (guarda _0), los 12
// hash de WorldConn (_1.._12, via la cadena de SpeechManager), MAudioReflection (_13) y
// VehicleParams (_14) van antes de los tres MNotify* y del `tune` de CanPlayback (_18).
// M1T43 2026-10-14: el resto de la cadena de SpeechManager en el orden en que el original interna
// sus nombres en .rodata (MGeneric, simsurface, speech, ecar, camerainfo, WorldConn, pvehicle,
// engine, engineaudio, MAudioReflection, audiosystem, gameplay, milestonetypes, VehicleParams,
// speechtune, pursuitlevels, SpeechCache, MNotify*, versiones de GRaceDatabase, IAI, GRaceStatus):
// las 43 primeras cadenas de la .rodata quedan como las del original; el .bss no cambia.
#include "Speed/Indep/Src/Generated/Messages/MGeneric.h"
#include "Speed/Indep/Src/Sim/SimSurface.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/speech.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/ecar.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/camerainfo.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/effects.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/audioimpact.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/audioscrape.h"
#include "Speed/Indep/Src/World/WorldConn.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/pvehicle.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/engine.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/engineaudio.h"
#include "Speed/Indep/Src/Generated/Messages/MAudioReflection.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/audiosystem.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/gameplay.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/milestonetypes.h"
#include "Speed/Indep/Src/Physics/PVehicle.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/speechtune.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/pursuitlevels.h"
#include "Speed/Indep/Src/Speech/SpeechCache.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifySpeechStatus.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifyCellCallComplete.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifyCellCallStarted.h"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Interfaces/Simables/IAI.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"

#include "Speed/Indep/Src/EAXSound/Stream/SpeechManager.cpp"

#include "Speed/Indep/Src/EAXSound/Stream/SpeechModule.cpp"

#include "Speed/Indep/Src/EAXSound/Stream/GameSpeech.cpp"

#include "Speed/Indep/Src/EAXSound/Stream/NISSFXModule.cpp"

#include "Speed/Indep/Src/Speech/SpeechCache.cpp"

#include "Speed/Indep/Src/Speech/EAXCharacter.cpp"

#include "Speed/Indep/Src/Speech/EAXDispatch.cpp"

#include "Speed/Indep/Src/Speech/EAXAirSupport.cpp"

#include "Speed/Indep/Src/Speech/EAXCop.cpp"

#include "Speed/Indep/Src/Speech/SpeechFlow.cpp"

#include "Speed/Indep/Src/Speech/PursuitFlow.cpp"

#include "Speed/Indep/Src/Speech/StrategyFlow.cpp"

#include "Speed/Indep/Src/Speech/Observer.cpp"

#include "Speed/Indep/Src/Speech/RoadblockFlow.cpp"

#include "Speed/Indep/Src/Speech/MusicFlow.cpp"

// datos-gc 2026-10-13: en la CU original MForcePursuitStart se parsea entre MNotifyMusicFlow y
// MPerpBusted (guardas _28.._34 tras el t_brake_start _27), antes del t_currdir de SoundAI (_35).
#include "Speed/Indep/Src/Generated/Messages/MForcePursuitStart.h"

#include "Speed/Indep/Src/Speech/SoundAI.cpp"

#include "Speed/Indep/Src/Speech/MiscSpeech.cpp"
