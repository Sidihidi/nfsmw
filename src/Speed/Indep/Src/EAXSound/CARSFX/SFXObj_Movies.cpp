#include "Speed/Indep/Src/EAXSound/CARSFX/SFXObj_Movies.hpp"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"

// Solo lo tienen las dos PS2 (la SourceList de GameCube no lo incluye). El 0x30 es el de su s_TypeInfo en .data.
DEFINE_CREATABLE(0x30, SFXObj_Movies, SndBase);

SFXObj_Movies::SFXObj_Movies() : CARSFX() {}

SFXObj_Movies::~SFXObj_Movies() {}

void SFXObj_Movies::Destroy() {}
