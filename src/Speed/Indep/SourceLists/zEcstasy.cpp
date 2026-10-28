// Prefijo comun de las unidades de juego del original (como zFEng.cpp): en la A124 los 402 primeros tipos de
// los stabs son los mismos en todas, y la .rodata empieza por las cadenas de bWare.hpp.
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Libs/Support/Utility/UStandard.h"
#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"

#include "Speed/Indep/Src/Ecstasy/Ecstasy.cpp"

#include "Speed/Indep/Src/Ecstasy/eStreamingPack.cpp"

#include "Speed/Indep/Src/Ecstasy/eModel.cpp"

#include "Speed/Indep/Src/Ecstasy/eSolid.cpp"

#include "Speed/Indep/Src/Ecstasy/eLight.cpp"

#include "Speed/Indep/Src/Ecstasy/eView.cpp"

#include "Speed/Indep/Src/Ecstasy/Texture.cpp"

#include "Speed/Indep/Src/World/Sun.cpp"

#include "Speed/Indep/Src/Ecstasy/DefragFixer.cpp"

#ifdef EA_PLATFORM_PLAYSTATION2
// El orden es el que da el .mdebug de la A124 (scripts/ficheros_mdebug.py): EcstasyE, eLightE,
// eEnvMapE, eEnvEffectsE, eMathE, eStripDataE, eSolidPlat, eViewPlat, DmaDump y eVisualTreatment.
#include "Speed/PSX2/Src/Ecstasy/EcstasyE.cpp"

#include "Speed/PSX2/Src/Ecstasy/eLightE.cpp"

#include "Speed/PSX2/Src/Ecstasy/eEnvMapE.cpp"

#include "Speed/PSX2/Src/Ecstasy/eEnvEffectsE.cpp"

#include "Speed/PSX2/Src/Ecstasy/eMathE.cpp"

#include "Speed/PSX2/Src/Ecstasy/eStripDataE.cpp"

#include "Speed/PSX2/Src/Ecstasy/eSolidPlat.cpp"

#include "Speed/PSX2/Src/Ecstasy/eViewPlat.cpp"

#include "Speed/PSX2/Src/DmaDump.cpp"

#include "Speed/PSX2/Src/Ecstasy/eVisualTreatment.cpp"
#endif

#ifdef EA_PLATFORM_GAMECUBE
#include "Speed/GameCube/Src/Ecstasy/EcstasyE.cpp"

#include "Speed/GameCube/Src/Ecstasy/EcstasyEx.cpp"

#include "Speed/GameCube/Src/Ecstasy/eLightE.cpp"

#include "Speed/GameCube/Src/Ecstasy/eEnvMapE.cpp"

#include "Speed/GameCube/Src/Ecstasy/eMathE.cpp"

#include "Speed/GameCube/Src/Ecstasy/eMatrixE.cpp"

#include "Speed/GameCube/Src/Ecstasy/ePShader.cpp"

#include "Speed/GameCube/Src/Ecstasy/eVShader.cpp"

#include "Speed/GameCube/Src/Ecstasy/eStrip.cpp"

#include "Speed/GameCube/Src/Ecstasy/eSolidPlat.cpp"

#include "Speed/GameCube/Src/Ecstasy/eViewPlat.cpp"

#include "Speed/GameCube/Src/Ecstasy/eVisualTreatment.cpp"

#endif
#include "Speed/Indep/Src/Generated/TexturePacks/TexEnumGen_ParticleTextures.cpp"

#include "Speed/Indep/Src/Ecstasy/EmitterSystem.cpp"
