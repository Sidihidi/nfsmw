#ifndef WORLD_TIME_OF_DAY_H
#define WORLD_TIME_OF_DAY_H

enum eTimeOfDay {
    eTOD_NUM_TIMES_OF_DAY = 2,
    eTOD_SUNSET = 1,
    eTOD_MIDDAY = 0,
    eTOD_ERROR = -1,
};

void TickOverTimeOfday();
void ApplyTimeOfDayTickOver();
void SetCurrentTimeOfDay(float tod);
const char *GetTimeOfDaySuffix(eTimeOfDay tod);
bool NeedsSeperateTODStreamingFile(const char *platform_name);
eTimeOfDay GetCurrentTimeOfDay();

#ifdef EA_PLATFORM_WIN32
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/timeofdaylighting.h"

// La hora del dia de PC (speed.exe 1.3). Solo PC la tiene: GameCube y las dos PS2 no traen nada de esto y X360 lo
// hace de otra forma (eTimeOfDayLighting). Los nombres de las clases, de los miembros y de los metodos son nuestros
// (el exe no los trae), salvo TimeOfDayPointer; entre parentesis, la direccion en speed.exe. En PC,
// SetCurrentTimeOfDay(float) pone la hora de esta clase (TimeOfDay.cpp).

// La luz de un momento del dia (0xA0 B): los campos de timeofdaylighting que se mezclan, en el orden de la mezcla
// (0x768EE0). Se lee desde el render de PC (CarSpecScale en eSetLightMaterial, la niebla y el cielo).
struct TimeOfDayLighting {
    bVector4 DiffuseColour;                           // offset 0x0, size 0x10
    bVector4 AmbientColour;                           // offset 0x10, size 0x10
    bVector4 SpecularColour;                          // offset 0x20, size 0x10
    bVector4 Unknown30;                               // offset 0x30, size 0x10 (nadie lo escribe)
    float CarSpecScale;                               // offset 0x40, size 0x4
    float EnvSkyBrightness;                           // offset 0x44, size 0x4
    bVector4 FogSkyColour;                            // offset 0x50, size 0x10
    bVector4 FogHazeColour;                           // offset 0x60, size 0x10
    float FogSkyColourScale;                          // offset 0x70, size 0x4
    float FogHazeColourScale;                         // offset 0x74, size 0x4
    float FogInLightScatter;                          // offset 0x78, size 0x4
    float FogSunFalloff;                              // offset 0x7C, size 0x4
    float FogDistanceScale;                           // offset 0x80, size 0x4
    bVector4 FixedFunctionSkyColor;                   // offset 0x90, size 0x10

    // La luz de `a` a `b` en la fraccion t (0x768EE0).
    void Blend(float t, const Attrib::Gen::timeofdaylighting &a, const Attrib::Gen::timeofdaylighting &b);
    // Lo mismo entre dos luces ya mezcladas (0x769190).
    void Blend(float t, const TimeOfDayLighting &a, const TimeOfDayLighting &b);
};

// total size: 0x190 (la crea eCreateTimeOfDay, 0x6BF290, y la anima el cuadro de PC, 0x6DE300)
class TimeOfDay {
  public:
    TimeOfDay(); // (0x770D70)

    // Avanza la hora y recoloca el sol (0x769360).
    void Update();
    // La luz de la hora actual (0x769580). El cuadro de PC (0x6DE451) le pasa la nubosidad de la vista del jugador 1
    // (eViews[1].Precipitation->GetCloudIntensity()): de cada pareja de luces, la despejada o la nublada; y, con la
    // altura del sol, la de sol alto o la de sol bajo. Las A son con la opcion "Visual Treatment"; las Plain, sin ella.
    void UpdateLighting(float cloud_intensity);

    float TimeScale;                                  // offset 0x0, size 0x4
    int Direction;                                    // offset 0x4, size 0x4 (1 o -1: la hora va y vuelve)
    float CurrentTime;                                // offset 0x8, size 0x4 (de 0 a 1; el sol en lo alto a 0,5)
    float SunHeading;                                 // offset 0xC, size 0x4 (radianes)
    float SunTilt;                                    // offset 0x10, size 0x4 (radianes)
    TimeOfDayLighting CurrentLighting;                // offset 0x20, size 0xA0
    Attrib::Gen::timeofdaylighting HighSunClear;      // offset 0xC0, size 0x14
    Attrib::Gen::timeofdaylighting LowSunClear;       // offset 0xD4, size 0x14
    Attrib::Gen::timeofdaylighting HighSunCloudy;     // offset 0xE8, size 0x14
    Attrib::Gen::timeofdaylighting LowSunCloudy;      // offset 0xFC, size 0x14
    Attrib::Gen::timeofdaylighting HighSunClearPlain; // offset 0x110, size 0x14
    Attrib::Gen::timeofdaylighting LowSunClearPlain;  // offset 0x124, size 0x14
    Attrib::Gen::timeofdaylighting HighSunCloudyPlain;// offset 0x138, size 0x14
    Attrib::Gen::timeofdaylighting LowSunCloudyPlain; // offset 0x14C, size 0x14
    float CloudIntensity;                             // offset 0x160, size 0x4
    float SunBlend;                                   // offset 0x164, size 0x4
    bVector4 SunPosition;                             // offset 0x170, size 0x10
    bVector4 SunDirection;                            // offset 0x180, size 0x10
};

extern TimeOfDay *TimeOfDayPointer;

// La opcion de video "Visual Treatment" de PC (0x6C0A20, en la plataforma; nombre nuestro): con ella la hora del dia
// mezcla las luces A, sin ella las B.
bool eIsVisualTreatmentEnabled();

#endif

#endif
