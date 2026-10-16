#include "TimeOfDay.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"
#include "Speed/Indep/Src/Misc/Config.h"
#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/Misc/bFile.hpp"
#endif
#ifdef EA_PLATFORM_WIN32
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/World/OnlineManager.hpp"
#endif

eTimeOfDay gTheTimeOfDay = eTOD_MIDDAY;
static eTimeOfDay desiredTOD = eTOD_MIDDAY;

const char *GetTimeOfDaySuffix(eTimeOfDay tod) {
    if (tod == eTOD_SUNSET) {
        return "_Sunset";
    }
    if (tod == eTOD_MIDDAY) {
        return "";
    }
    return "THIS_IS_NOT_A_TIME_OF_DAY_SUFFIX";
}

bool NeedsSeperateTODStreamingFile(const char *platform_name) {
    if (bStrEqual(platform_name, "PSX2") || bStrEqual(platform_name, "XBOX")) {
        return true;
    }
    return false;
}

eTimeOfDay GetCurrentTimeOfDay() {
    return gTheTimeOfDay;
}

// Las dos PS2 alternan el atardecer cada cinco minutos de juego (locales estaticas de los stabs de la A124,
// TimeOfDay.cpp:47-93); GameCube lo compila vacio. SLUS ya no fuerza la hora en el frontend (eso queda en
// ApplyTimeOfDayTickOver) y solo cambia si el disco trae la pista del atardecer.
void TickOverTimeOfday() {
#ifdef EA_PLATFORM_PLAYSTATION2
    static unsigned int ticktimeminutes;
    static unsigned int lastminutes;

    if (TimeOfDaySwapEnable) {
        unsigned int minutes = static_cast<unsigned int>(WorldTimeSeconds * (1.0f / 60.0f));
#ifdef EA_BUILD_A124
        if (TheGameFlowManager.GetState() == GAMEFLOW_STATE_IN_FRONTEND) {
            if (gTheTimeOfDay != desiredTOD) {
                gTheTimeOfDay = desiredTOD;
            }
        }
#endif
        if (minutes != lastminutes) {
            lastminutes = minutes;
            ticktimeminutes++;
            if (ticktimeminutes == 5) {
                ticktimeminutes = 0;
#ifndef EA_BUILD_A124
                if (!bFileExists("TRACKS/L2RA_Sunset.BUN")) {
                    return;
                }
#endif
                if (TheGameFlowManager.GetState() == GAMEFLOW_STATE_RACING) {
                    if (desiredTOD == gTheTimeOfDay) {
                        desiredTOD = static_cast<eTimeOfDay>((desiredTOD + 1) % eTOD_NUM_TIMES_OF_DAY);
                    }
                }
            }
        }
    }
#endif
}

void ApplyTimeOfDayTickOver() {
    if (TimeOfDaySwapEnable == 0) {
        return;
    }
    if (gTheTimeOfDay == desiredTOD) {
        return;
    }
    gTheTimeOfDay = desiredTOD;
}

#ifdef EA_PLATFORM_WIN32
TimeOfDay *TimeOfDayPointer; // (0x9B392C)

// Negativa, la luz sigue a las nubes de la vista; si no, una nubosidad fija (0x905CE8; nombre nuestro).
float TimeOfDayCloudOverride = -1.0f;
// La distancia del sol al origen (0x905D44; nombre nuestro).
float TimeOfDaySunDistance = 100000.0f;

// a + (b - a) * t de tres componentes (0x768E40 y 0x768E80) y de una (0x768EC0). Nombres nuestros.
void TimeOfDayLerp(bVector4 *out, float t, const UMath::Vector4 *a, const UMath::Vector4 *b) {
    out->x = (b->x - a->x) * t + a->x;
    out->y = (b->y - a->y) * t + a->y;
    out->z = (b->z - a->z) * t + a->z;
}

void TimeOfDayLerp(bVector4 *out, float t, const bVector4 *a, const bVector4 *b) {
    out->x = (b->x - a->x) * t + a->x;
    out->y = (b->y - a->y) * t + a->y;
    out->z = (b->z - a->z) * t + a->z;
}

void TimeOfDayLerp(float *out, float t, const float *a, const float *b) {
    *out = (*b - *a) * t + *a;
}

void TimeOfDayLighting::Blend(float t, const Attrib::Gen::timeofdaylighting &a, const Attrib::Gen::timeofdaylighting &b) {
    TimeOfDayLerp(&DiffuseColour, t, &a.DiffuseColour(), &b.DiffuseColour());
    TimeOfDayLerp(&AmbientColour, t, &a.AmbientColour(), &b.AmbientColour());
    TimeOfDayLerp(&SpecularColour, t, &a.SpecularColour(), &b.SpecularColour());
    TimeOfDayLerp(&CarSpecScale, t, &a.CarSpecScale(), &b.CarSpecScale());
    TimeOfDayLerp(&EnvSkyBrightness, t, &a.EnvSkyBrightness(), &b.EnvSkyBrightness());
    TimeOfDayLerp(&FogSkyColour, t, &a.FogSkyColour(), &b.FogSkyColour());
    TimeOfDayLerp(&FogHazeColour, t, &a.FogHazeColour(), &b.FogHazeColour());
    TimeOfDayLerp(&FogSkyColourScale, t, &a.FogSkyColourScale(), &b.FogSkyColourScale());
    TimeOfDayLerp(&FogHazeColourScale, t, &a.FogHazeColourScale(), &b.FogHazeColourScale());
    TimeOfDayLerp(&FogInLightScatter, t, &a.FogInLightScatter(), &b.FogInLightScatter());
    TimeOfDayLerp(&FogSunFalloff, t, &a.FogSunFalloff(), &b.FogSunFalloff());
    TimeOfDayLerp(&FogDistanceScale, t, &a.FogDistanceScale(), &b.FogDistanceScale());
    TimeOfDayLerp(&FixedFunctionSkyColor, t, &a.FixedFunctionSkyColor(), &b.FixedFunctionSkyColor());
}

void TimeOfDayLighting::Blend(float t, const TimeOfDayLighting &a, const TimeOfDayLighting &b) {
    TimeOfDayLerp(&DiffuseColour, t, &a.DiffuseColour, &b.DiffuseColour);
    TimeOfDayLerp(&AmbientColour, t, &a.AmbientColour, &b.AmbientColour);
    TimeOfDayLerp(&SpecularColour, t, &a.SpecularColour, &b.SpecularColour);
    TimeOfDayLerp(&CarSpecScale, t, &a.CarSpecScale, &b.CarSpecScale);
    TimeOfDayLerp(&EnvSkyBrightness, t, &a.EnvSkyBrightness, &b.EnvSkyBrightness);
    TimeOfDayLerp(&FogSkyColour, t, &a.FogSkyColour, &b.FogSkyColour);
    TimeOfDayLerp(&FogHazeColour, t, &a.FogHazeColour, &b.FogHazeColour);
    TimeOfDayLerp(&FogSkyColourScale, t, &a.FogSkyColourScale, &b.FogSkyColourScale);
    TimeOfDayLerp(&FogHazeColourScale, t, &a.FogHazeColourScale, &b.FogHazeColourScale);
    TimeOfDayLerp(&FogInLightScatter, t, &a.FogInLightScatter, &b.FogInLightScatter);
    TimeOfDayLerp(&FogSunFalloff, t, &a.FogSunFalloff, &b.FogSunFalloff);
    TimeOfDayLerp(&FogDistanceScale, t, &a.FogDistanceScale, &b.FogDistanceScale);
    TimeOfDayLerp(&FixedFunctionSkyColor, t, &a.FixedFunctionSkyColor, &b.FixedFunctionSkyColor);
}

TimeOfDay::TimeOfDay()
    : HighSunClear(0x3dcfa673, 0, nullptr), //
      LowSunClear(0xb902cfad, 0, nullptr), //
      HighSunCloudy(0x59c373c3, 0, nullptr), //
      LowSunCloudy(0xd835d0c8, 0, nullptr), //
      HighSunClearPlain(0xa01a3942, 0, nullptr), //
      LowSunClearPlain(0x0e1c797c, 0, nullptr), //
      HighSunCloudyPlain(0x94abdff0, 0, nullptr), //
      LowSunCloudyPlain(0x919b6689, 0, nullptr) {
    bFill(&CurrentLighting.DiffuseColour, 0.61f, 0.49f, 0.23f, 1.0f);
    bFill(&CurrentLighting.AmbientColour, 0.14f, 0.16f, 0.22f, 1.0f);
    bFill(&CurrentLighting.SpecularColour, 0.58f, 0.53f, 0.42f, 1.0f);
    CurrentLighting.CarSpecScale = 1.0f;
    CurrentLighting.EnvSkyBrightness = 2.5f;
    bFill(&CurrentLighting.FogSkyColour, 0.27f, 0.46f, 0.96f, 1.0f);
    bFill(&CurrentLighting.FogHazeColour, 0.19f, 0.25f, 0.38f, 1.0f);
    CurrentLighting.FogSkyColourScale = 2.0f;
    CurrentLighting.FogHazeColourScale = 0.0f;
    CurrentLighting.FogInLightScatter = 8.0f;
    CurrentLighting.FogSunFalloff = -0.16f;
    CurrentLighting.DiffuseColour.w = 1.0f;
    CurrentLighting.AmbientColour.w = 4.0f;
    TimeScale = 1.0f;
    CurrentTime = 0.45f;
    SunHeading = 3.28f;
    SunTilt = 0.5f;
    Direction = -1;
    Update();
    UpdateLighting(0.1f);
    bFill(&CurrentLighting.FixedFunctionSkyColor, 0.61f, 0.49f, 0.23f, 1.0f);
}

// En la carrera la hora va y viene entre 0,05 y 0,95; en el frontend, entre 0,35 y 0,65. En una carrera en linea la
// pone el servidor (OnlineManager, 0x735A30), que nuestro OnlineManager de PC no tiene.
void TimeOfDay::Update() {
    float limit = 0.35f;
    if (TheGameFlowManager.GetState() != GAMEFLOW_STATE_IN_FRONTEND) {
        limit = 0.05f;
    }
    if (!TheOnlineManager.IsOnlineRace()) {
        CurrentTime += RealTimeElapsed * (1.0f / 60.0f) * Direction * TimeScale * 0.05f;
        if (CurrentTime < limit) {
            CurrentTime = limit;
            Direction = -Direction;
        } else if (CurrentTime > 1.0f - limit) {
            CurrentTime = 1.0f - limit;
            Direction = -Direction;
        }
    }
    float elevation = (bClamp(CurrentTime, limit, 1.0f - limit) - 0.5) * 3.1415927f;
    float heading = SunHeading;
    float tilt = SunTilt;
    SunPosition.x = (bSin(elevation) * bCos(heading) + bSin(heading) * bSin(tilt) * bCos(elevation)) * TimeOfDaySunDistance;
    SunPosition.y = (bSin(elevation) * bSin(heading) + bSin(tilt) * bCos(heading) * bCos(elevation)) * TimeOfDaySunDistance;
    SunPosition.z = bCos(elevation) * TimeOfDaySunDistance * bCos(tilt);
    SunPosition.w = 1.0f;
    SunDirection.w = 0.0f;
    bNormalize(reinterpret_cast<bVector3 *>(&SunDirection), reinterpret_cast<bVector3 *>(&SunPosition));
}

void TimeOfDay::UpdateLighting(float cloud_intensity) {
    float elevation = bClamp(SunDirection.z, 0.25f, 0.5f);
    CloudIntensity = cloud_intensity;
    SunBlend = 1.0f - (elevation - 0.25f) * 4.0f;
    if (TimeOfDayCloudOverride >= 0.0f) {
        CloudIntensity = TimeOfDayCloudOverride;
    }
    TimeOfDayLighting a;
    TimeOfDayLighting b;
    if (eIsVisualTreatmentEnabled()) {
        a.Blend(CloudIntensity, LowSunClear, LowSunCloudy);
        b.Blend(CloudIntensity, HighSunClear, HighSunCloudy);
    } else {
        a.Blend(CloudIntensity, LowSunClearPlain, LowSunCloudyPlain);
        b.Blend(CloudIntensity, HighSunClearPlain, HighSunCloudyPlain);
    }
    CurrentLighting.Blend(SunBlend, b, a);
    CurrentLighting.AmbientColour.w = 4.0f;
}

// PC: la hora es la de la TimeOfDay (0x7696B0).
void SetCurrentTimeOfDay(float tod) {
    TimeOfDayPointer->CurrentTime = tod;
}
#else
void SetCurrentTimeOfDay(float tod) {
#if defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_BUILD_A124)
    if (tod < 0.0f) {
        return;
    }
    eTimeOfDay new_tod = tod >= 0.8f ? eTOD_SUNSET : eTOD_MIDDAY;
    if (new_tod == eTOD_SUNSET && !bFileExists("TRACKS/L2RA_Sunset.BUN")) {
        new_tod = eTOD_MIDDAY;
    }
    gTheTimeOfDay = new_tod;
#endif
}
#endif
