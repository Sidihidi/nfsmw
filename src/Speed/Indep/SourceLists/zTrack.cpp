// Prefijo comun de las unidades de juego del original (como zFEng.cpp): en la A124 los 402 primeros tipos de
// los stabs son los mismos en todas, y la .rodata empieza por las cadenas de bWare.hpp.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"

// datos-gc 2026-10-13: la CU original parsea WorldConn (guardas _0.._11), MGeneric (_12) y
// MAudioReflection (_13) antes del estatico de DoTunnelBloom (_14, en ScreenEffects.cpp).
// gcdatos2: esta cabeza no admite cambios, medido tres veces: (a) bajar MGeneric/
// MAudioReflection hasta antes de TrackStreamer (la Z2 de gcdatos1, sin sus recortes de
// cabeceras) y (b) adelantar effects/audioimpact/audioscrape/SimSurface delante de
// WorldConn hacen que los 14 estaticos hash/k salten a la cabeza del .bss (+0x70) en la
// primera medida y cuestan ademas -7 fn (a) y -4 fn (b) en el zTrack de PC. Las cadenas
// de MGeneric/MAudioReflection y las de la cadena de Skids (gameplay, versiones,
// pvehicle, GManager, EAGL4::SymbolEntry) se quedan por tanto en cabeza, desplazadas
// respecto al objetivo: moverlas exige recortar Skids.hpp/Camera.hpp/WorldConn.h, que es
// lo que costo -28 fn en X360 y -34 en PC en la ronda anterior. Residuo documentado en
// docs/analisis/2026-10-18_gcdatos2.md.
// gcdatos3: reintento de (b) en variante ligera: solo las clases Gen (sin SimSurface.h,
// que es lo que arrastraba la clase SimSurface completa en el intento de gcdatos2).
// En el objetivo van effects, audioimpact, audioscrape, simsurface (obj+0xF4..0x16B)
// antes de las cadenas de WorldConn. Medido x5 asentado: .rodata GC 83,0973->84,7853 %
// (falta -70, sobra -46) y PC intacto en 504 fn: se queda. El reintento de (a) en
// solitario y asentado volvio a costar PC (504->493 fn), revertido: ver gcdatos3.md.
#include "Speed/Indep/Src/Generated/AttribSys/Classes/effects.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/audioimpact.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/audioscrape.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/simsurface.h"
#include "Speed/Indep/Src/World/WorldConn.h"
#include "Speed/Indep/Src/Generated/Messages/MGeneric.h"
#include "Speed/Indep/Src/Generated/Messages/MAudioReflection.h"

#include "Speed/Indep/Src/World/Skids.cpp"

#include "Speed/Indep/Src/World/Clans.cpp"

#include "Speed/Indep/Src/World/Track.cpp"

#include "Speed/Indep/Src/World/TrackPositionMarker.cpp"

#include "Speed/Indep/Src/World/TrackInfo.cpp"

#include "Speed/Indep/Src/World/TrackPath.cpp"

// gcdatos2: el bloque Gen (~430 B) que en el original entra con la cadena de TrackStreamer
// (CameraMover->Camera->GManager), reproducido como parseos en este punto de la TU, en el
// orden exacto de la .rodata objetivo: gameplay, milestonetypes, 16.1.0, 16.2.1, 1.2.3,
// 1.8.1, 19.8.31, speech, aivehicle, pursuitlevels, pursuitescalation, pursuitsupport,
// ecar, camerainfo, pvehicle, engine, engineaudio, [MGeneric/MAudioReflection arriba],
// audiosystem, GRaceStatus, SMS_MESSAGE_*. Sin recortar ninguna cabecera compartida.
// 16.1.0 seguira faltando mientras GRaceDatabase::GetDDayStartRace no devuelva el literal
// (receta gcdatos1-B, otro carril). OJO al medir: la primera medida tras tocar esta TU usa
// los mapeos de sufijos locales del objeto anterior y el .bss parece colapsar (4 B); a la
// segunda medida empareja (47.296 B). No es una regresion real.
#include "Speed/Indep/Src/Generated/AttribSys/Classes/gameplay.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/milestonetypes.h"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/speech.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/aivehicle.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/pursuitlevels.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/pursuitescalation.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/pursuitsupport.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/ecar.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/camerainfo.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/pvehicle.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/engine.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/engineaudio.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/audiosystem.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"

#include "Speed/Indep/Src/World/TrackStreamer.cpp"

#include "Speed/Indep/Src/World/Scenery.cpp"

#include "Speed/Indep/Src/World/VisibleSection.cpp"

#include "Speed/Indep/Src/World/WeatherMan.cpp"

#include "Speed/Indep/Src/World/ScreenEffects.cpp"

#include "Speed/Indep/Src/World/EventManager.cpp"
