#include "Speed/Indep/Src/World/Rain.hpp"
#include "Speed/Indep/Src/Ecstasy/EcstasyData.hpp"
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/Src/World/ParameterMaps.hpp"
#include "Speed/Indep/Src/Ecstasy/Texture.hpp"
#include "Speed/Indep/Src/World/WeatherMan.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/Src/Camera/CameraMover.hpp"
#include "Speed/Indep/Src/Interfaces/SimActivities/INIS.h"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Misc/GameFlow.hpp"
#include "Speed/Indep/Src/Misc/Profiler.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/World/OnlineManager.hpp"
#include "Speed/Indep/Src/World/ScreenEffects.hpp"
#include "Speed/Indep/Src/World/TrackPath.hpp"

extern uint32 numCopsActive;
extern int TweakerPauseWorld;
extern float TUNHEIGHT;

uint32 precipDEBUG = 0;
float precipPERCENT = 1.0f;

float SNOWLINE = 350.0f;
float SNOWLINErange = 100.0f;

float RAINX = 0.02f;
float RAINY = 0.02f;
float RAINZ = 0.03f;
float RAINZconstant = 0.35f;

float SNOWX = 0.05f;
float SNOWY = 0.05f;
float SNOWZ = 0.03f;
float SNOWZconstant = 0.1f;

float SLEETX = 0.05;
float SLEETY = 0.05;
float SLEETZ = 0.03f;
float SLEETZconstant = 0.1f;

float HAILX = 0.05f;
float HAILY = 0.05f;
float HAILZ = 0.03f;
float HAILZconstant = 0.1f;

float precipBoundX = 20.0f;
float precipBoundY = 7.0f;
float precipBoundZ = 6.0f;

float precipAheadX = 8.0f;
float precipAheadY = 0.0f;
float precipAheadZ = 0.0f;

float RAINstick = 0.4f;
float SNOWstick = 2.0f;
float SLEETstick = 0.1f;
float HAILstick = 0.5f;

float RAINwindEffect = 0.3f;
float SNOWwindEffect = 1.0f;
float SLEETwindEffect = 0.4f;
float HAILwindEffect = 0.3f;

float RAINRadiusX = 0.01f;
float RAINRadiusY = 0.45f;
float RAINRadiusZ = 0.05f;

float SNOWRadiusX = 0.1f;
float SNOWRadiusY = 0.1f;
float SNOWRadiusZ = 0.1f;

float SLEETRadiusX = 0.05f;
float SLEETRadiusY = 0.05f;
float SLEETRadiusZ = 0.05f;

float HAILRadiusX = 0.05f;
float HAILRadiusY = 0.05f;
float HAILRadiusZ = 0.05f;

float DistBoundSq0 = 64.0f;

static const int32 PRECIPFogDEBUG = 0;

// TODO are the values right?
static const float PRECIPFogIntense = 0.9f;
static const float PRECIPFogRED = 111.0f;
static const float PRECIPFogGREEN = 128.0f;
static const float PRECIPFogBLUE = 137.0f;
static const float PRECIPFogStart = 74.0f;
static const float PRECIPFogFalloff = 0.75f;
static const float PRECIPFogFalloffX = 0.1f;
static const float PRECIPFogFalloffY = 1.0f;

static const int ShowPreciData = 0;

float twkCloudsMinAmount = 0.0f;
float twkRainRateOfChange = 1.0f;

float twkCloudsRateOfChange = 1.0f;

float driveFactor = -0.03f;

ParameterAccessor RainAccessor("Rain");
ParameterAccessor CloudAccessor("Clouds");

uint32 precipRENDER = 1;

GenericRegion TempRegions[1];

// TODO
extern bool EnableRainIn2P;

void TempInits() {}

void SetRainBase() {
#ifdef EA_BUILD_A124
    eGetView(EVIEW_PLAYER1, false)->Precipitation->SetRoadDampness(0.0f);
    eGetView(EVIEW_PLAYER2, false)->Precipitation->SetRoadDampness(0.0f);

    eGetView(EVIEW_PLAYER1, false)->Precipitation->Init(INACTIVE, 1.0f);
    eGetView(EVIEW_PLAYER2, false)->Precipitation->Init(INACTIVE, 1.0f);
#else
    eGetView(EVIEW_PLAYER1, false)->Precipitation->SetRoadDampness(0.0f);
    eGetView(EVIEW_PLAYER1, false)->Precipitation->Init(INACTIVE, 1.0f);

    // PS2 siempre tiene lluvia en las dos vistas (SLUS no mira EnableRainIn2P).
#ifndef EA_PLATFORM_PLAYSTATION2
    if (EnableRainIn2P)
#endif
    {
        eGetView(EVIEW_PLAYER2, false)->Precipitation->SetRoadDampness(0.0f);
        eGetView(EVIEW_PLAYER2, false)->Precipitation->Init(INACTIVE, 1.0f);
    }
#endif
}

// UNSOLVED
Rain::Rain(eView *view, RainType StartType) {
    this->RoadDampness = 0.0f;
    this->intensity = 0.0f;
    this->CloudIntensity = twkCloudsMinAmount;
    this->percentPrecip[RAIN] = 0.0f;
    this->percentPrecip[INACTIVE] = 0.0f;
    this->percentPrecip[StartType] = 1.0f;
    this->NumRainPoints = -1;
    this->MyView = view;
    this->NoRain = 0;
    this->NoRainAhead = 0;
    this->PRECIPpoly[0].UVs[0][0] = 0.0f;
    this->PRECIPpoly[0].UVs[0][1] = 1.0f;
    this->PRECIPpoly[0].UVs[1][0] = 0.1f;
    this->PRECIPpoly[0].UVs[1][1] = 1.0f;
    this->PRECIPpoly[0].UVs[2][0] = 0.1f;
    this->PRECIPpoly[0].UVs[2][1] = 0.0f;
    this->PRECIPpoly[0].UVs[3][0] = 0.0f;
    this->PRECIPpoly[0].UVs[3][1] = 0.0f;

    unsigned char r = 128;
    unsigned char g = 128;
    unsigned char b = 128;
    unsigned char a = 128;

    this->PRECIPpoly[0].Colours[0][0] = r;
    this->PRECIPpoly[0].Colours[0][1] = g;
    this->PRECIPpoly[0].Colours[0][2] = b;
    this->PRECIPpoly[0].Colours[0][3] = a;

    this->PRECIPpoly[0].Colours[1][0] = r;
    this->PRECIPpoly[0].Colours[1][1] = g;
    this->PRECIPpoly[0].Colours[1][2] = b;
    this->PRECIPpoly[0].Colours[1][3] = a;

    this->PRECIPpoly[0].Colours[2][0] = r;
    this->PRECIPpoly[0].Colours[2][1] = g;
    this->PRECIPpoly[0].Colours[2][2] = b;
    this->PRECIPpoly[0].Colours[2][3] = a;

    this->PRECIPpoly[0].Colours[3][0] = r;
    this->PRECIPpoly[0].Colours[3][1] = g;
    this->PRECIPpoly[0].Colours[3][2] = b;
    this->PRECIPpoly[0].Colours[3][3] = a;

    this->PRECIPpoly[1].UVs[0][0] = 0.0f;
    this->PRECIPpoly[1].UVs[0][1] = 1.0f;
    this->PRECIPpoly[1].UVs[1][0] = 0.1f;
    this->PRECIPpoly[1].UVs[1][1] = 1.0f;
    this->PRECIPpoly[1].UVs[2][0] = 0.1f;
    this->PRECIPpoly[1].UVs[2][1] = 0.0f;
    this->PRECIPpoly[1].UVs[3][0] = 0.0f;
    this->PRECIPpoly[1].UVs[3][1] = 0.0f;

    unsigned char r1 = 100;
    unsigned char g1 = 100;
    unsigned char b1 = 100;
    unsigned char a1 = 28;

    float v;

    this->PRECIPpoly[1].Colours[0][0] = r1;
    this->PRECIPpoly[1].Colours[0][1] = g1;
    this->PRECIPpoly[1].Colours[0][2] = b1;
    this->PRECIPpoly[1].Colours[0][3] = a1;

    this->PRECIPpoly[1].Colours[1][0] = r1;
    this->PRECIPpoly[1].Colours[1][1] = g1;
    this->PRECIPpoly[1].Colours[1][2] = b1;
    this->PRECIPpoly[1].Colours[1][3] = a1;

    this->PRECIPpoly[1].Colours[2][0] = r1;
    this->PRECIPpoly[1].Colours[2][1] = g1;
    this->PRECIPpoly[1].Colours[2][2] = b1;
    this->PRECIPpoly[1].Colours[2][3] = a1;

    this->PRECIPpoly[1].Colours[3][0] = r1;
    this->PRECIPpoly[1].Colours[3][1] = g1;
    this->PRECIPpoly[1].Colours[3][2] = b1;
    this->PRECIPpoly[1].Colours[3][3] = a1;
    this->SetPrecipFogColour(0, 0, 0);
    this->inTunnel = 0;
    this->inOverpass = 0;
    this->IsValidRainCurtainPos = CT_INACTIVE;
}

void Rain::Init(RainType type, float percent) {
    TempInits();

    this->texture_info[RAIN] = GetTextureInfo(bStringHash("RAINDROP"), 0, 0);
    this->NumRainPoints = MAXRAINPOINTS;

    for (int j = 0; j < 2; j++) {
        this->NumOfType[j] = 0;
        this->DesiredNumOfType[j] = 0;
    }

    this->NewSwapBuffer = 0;
    this->OldSwapBuffer = 1;
    this->NumOfType[type] = this->NumRainPoints;
#ifdef EA_PLATFORM_PLAYSTATION2
    // En PS2 [RAIN][0] va junto a [RAIN][1] (comparten la carga del global);
    // en el resto va tarde, con precipZconstant.
    this->precipWindEffect[INACTIVE][0] = 1.0f;
    this->precipWindEffect[INACTIVE][1] = 1.0f;
    this->precipWindEffect[RAIN][1] = RAINwindEffect * 0.5f;
    this->precipWindEffect[RAIN][0] = RAINwindEffect;
#else
    this->precipWindEffect[RAIN][1] = RAINwindEffect * 0.5f;
    this->precipWindEffect[INACTIVE][0] = 1.0f;
    this->precipWindEffect[INACTIVE][1] = 1.0f;
#endif
    this->precipRadius[RAIN] = bVector3(RAINRadiusX, RAINRadiusY, RAINRadiusZ);
    this->precipSpeedRange[RAIN] = bVector3(RAINX, RAINY, RAINZ);
#ifndef EA_PLATFORM_PLAYSTATION2
    this->precipWindEffect[RAIN][0] = RAINwindEffect;
#endif
    this->precipZconstant[RAIN] = RAINZconstant;
    this->windType[RAIN] = VECTOR_WIND;
    this->windTime = 0.0f;
    this->windSpeed = bVector3(0.0f, 0.0f, 0.0f);

#ifndef DISABLE_RAIN
    for (int32 i = 0; i < this->NumRainPoints; i++) {
        this->RainPointsInf[i].status = 2;
        this->RainPointsInf[i].type = static_cast<uint8>(type);
        this->RainPointsInf[i].subType = static_cast<uint8>(bRandom(2));

        this->RainPoints[i].NormalizedPoint[this->NewSwapBuffer].x = precipAheadX + (bRandom(precipBoundX) - precipBoundX * 0.5f);
        this->RainPoints[i].NormalizedPoint[this->NewSwapBuffer].y = precipAheadY + (bRandom(precipBoundY) - precipBoundY * 0.5f);
        this->RainPoints[i].NormalizedPoint[this->NewSwapBuffer].z = precipAheadZ + bRandom(precipBoundZ);

        this->RainPoints[i].NormalizedPoint[this->OldSwapBuffer] = this->RainPoints[i].NormalizedPoint[this->NewSwapBuffer];
    }

    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 10; i++) {
            this->Velocities[j][i].x = bRandom(this->precipSpeedRange[j].x) - this->precipSpeedRange[j].x * 0.5f;
            this->Velocities[j][i].y = bRandom(this->precipSpeedRange[j].y) - this->precipSpeedRange[j].y * 0.5f;
            this->Velocities[j][i].z = -bRandom(this->precipSpeedRange[j].z) - this->precipZconstant[j];
            this->Velocities[j][i].pad = -99999.9f;
        }
    }
#endif
}

// STRIPPED
Rain::~Rain() {}

float snowPercent = 1.0f;
float rainPercent = 1.0f;
float fogPercent = 0.0f;

int Chance100 = 0;
float ChancePercent = 1.0f;

float rainOverrideIntensity = 0.0f;

void SetOverRideRainIntensity(float rov) {
    rainOverrideIntensity = rov;
}


static const int TimeTestRain = 3;
static const float TimeTestFactor = 1.4f;
static const float DistanceTestFactor = 50.0f;

float IsRainingAt(float x, float y);
float IsCloudyAt(float x, float y);

float GetDesiredRainIntensity(float x, float y) {
    // Xbox 360 la tiene vacia (4 B).
#ifndef EA_PLATFORM_XENON
    float rolly;

#ifdef EA_BUILD_A124
    if (numCopsActive < 3) {
#else
    if (numCopsActive < 3 || INIS::Exists()) {
#endif
        if (rainOverrideIntensity > 0.0f) {
            return rainOverrideIntensity;
        }
#ifdef EA_BUILD_A124
        // Solo la A124 mira el azar fijo aqui.
        if (Chance100 != 0) {
            return ChancePercent;
        }
#endif
        if (rainOverrideIntensity < 0.0f) {
            return 0.0f;
        }

        rolly = IsRainingAt(x, y);
        if (RainAccessor.IsValid()) {
            RainAccessor.CaptureData(x, y);
            float chance = RainAccessor.GetDataFloat(1);

            if (rolly > chance) {
                return 0.0f;
            }
            return RainAccessor.GetDataFloat(0);
        }
    }
    return 0.0f;
#endif
}

static const uint32 CloudsPlease = 0;
static const uint32 NoCloudsPlease = 0;

float GetDesiredCloudyness(float x, float y) {
#ifndef EA_PLATFORM_XENON
    float rolly = IsCloudyAt(x, y);

    if (CloudAccessor.IsValid()) {
        CloudAccessor.CaptureData(x, y);
        float chance = CloudAccessor.GetDataFloat(1);

        if (rolly > chance) {
            return twkCloudsMinAmount;
        }
        return CloudAccessor.GetDataFloat(0);
    }
    return twkCloudsMinAmount;
#endif
}

float WorldWeatherTime() {
#ifndef EA_PLATFORM_XENON
    return WorldTimeSeconds;
#endif
}

float IsRainingAt(float x, float y) {
#ifndef EA_PLATFORM_XENON
#ifdef EA_BUILD_A124
    float chance = bCos(bDegToAng(x / DistanceTestFactor + WorldTimeSeconds));
#else
    float chance = bCos(bDegToAng(x / DistanceTestFactor + WorldWeatherTime() / TimeTestFactor));

    if (chance < 0.0f) {
        chance = 0.0f;
    }
#endif
    return 1.0f - bAbs(chance);
#endif
}

float IsCloudyAt(float x, float y) {
#ifndef EA_PLATFORM_XENON
#ifdef EA_BUILD_A124
    float chance = bSin(bDegToAng(x / DistanceTestFactor + WorldTimeSeconds));
#else
    float chance = bSin(bDegToAng(x / DistanceTestFactor + WorldWeatherTime() / TimeTestFactor));

    if (chance < 0.0f) {
        chance = 0.0f;
    }
#endif
    return 1.0f - bAbs(chance);
#endif
}

void Rain::AttachRainCurtain(float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3) {
#ifndef DISABLE_RAIN
    this->RainCurtainPos[0].x = x0;
    this->RainCurtainPos[0].y = y0;
    this->RainCurtainPos[0].z = z0;

    this->RainCurtainPos[1].x = x1;
    this->RainCurtainPos[1].y = y1;
    this->RainCurtainPos[1].z = z1;

    this->RainCurtainPos[2].x = x2;
    this->RainCurtainPos[2].y = y2;
    this->RainCurtainPos[2].z = z2;

    this->RainCurtainPos[3].x = x3;
    this->RainCurtainPos[3].y = y3;
    this->RainCurtainPos[3].z = z3;

    this->CurtainLength = bDistBetween(&this->RainCurtainPos[1], &this->RainCurtainPos[0]);
#endif
}

void Rain::FindCurtains() {
#ifndef DISABLE_RAIN
    CameraMover *cameraMover = this->MyView->GetCameraMover();

    if (cameraMover == nullptr) {
        return;
    }
    if (cameraMover->GetAnchor() == nullptr) {
        return;
    }

    bVector3 EntryPosition(*this->MyView->GetCamera()->GetPosition());
    EntryPosition -= bScale(*this->MyView->GetCamera()->GetDirection(), 4.0f);

    bVector2 twoDentry(EntryPosition.x, EntryPosition.y);
    static_cast<TrackPathZone *>(this->the_zone)->GetSegmentNextTo(&twoDentry, &this->ent0, &this->ent1);

    bVector2 diff = this->ent0 - this->ent1;
    bVector2 direction(diff.y, -diff.x);
    bNormalize(&direction, &direction);

    bVector2 entrancePt = (static_cast<TrackPathZone *>(this->the_zone)->BBoxMin + static_cast<TrackPathZone *>(this->the_zone)->BBoxMax) * 0.5f;
    bVector2 exitPt;
    static_cast<TrackPathZone *>(this->the_zone)->GetOpposite(&this->ent0, &this->ent1, &this->ext0, &this->ext1);
#endif
}

void Rain::FindCurtain() {
#ifndef DISABLE_RAIN
    CameraMover *cameraMover = this->MyView->GetCameraMover();

    if (cameraMover == nullptr) {
        return;
    }
    CameraAnchor *cameraAnchor = cameraMover->GetAnchor();
    if (cameraAnchor == nullptr) {
        return;
    }

    bVector2 CameraDirection(this->MyView->GetCamera()->GetDirection()->x, this->MyView->GetCamera()->GetDirection()->y);
    bNormalize(&CameraDirection, &CameraDirection);

    bVector3 Position(*cameraAnchor->GetGeometryPosition());
    bVector2 Pos2D(Position.x, Position.y);
    bVector2 ent = (this->ent0 + this->ent1) * 0.5f;
    Pos2D = ent - Pos2D;
    bNormalize(&Pos2D, &Pos2D);

    if (bDot(&Pos2D, &CameraDirection) > 0.0f) {
        this->MyView->Precipitation->AttachRainCurtain(this->ent0.x, this->ent0.y, Position.z + TUNHEIGHT,
                                                       this->ent1.x, this->ent1.y, Position.z + TUNHEIGHT,
                                                       this->ent0.x, this->ent0.y, Position.z,
                                                       this->ent1.x, this->ent1.y, Position.z);
    } else {
        this->MyView->Precipitation->AttachRainCurtain(this->ext0.x, this->ext0.y, Position.z + TUNHEIGHT,
                                                       this->ext1.x, this->ext1.y, Position.z + TUNHEIGHT,
                                                       this->ext0.x, this->ext0.y, Position.z,
                                                       this->ext1.x, this->ext1.y, Position.z);
    }
#endif
}

void Rain::SeedCurtainXZ(RainPointsDef *rainpoints) {
#ifndef DISABLE_RAIN
    float distX = bRandom(this->CurtainLength);
    float distZ = bRandom(TUNHEIGHT);
    float distY = bRandom(10.0f);
    bVector3 Along = this->RainCurtainPos[1] - this->RainCurtainPos[0];

    bNormalize(&Along, &Along);
    rainpoints->NormalizedPoint[this->NewSwapBuffer] = this->RainCurtainPos[0] + Along * distX + this->outvex * distY;
    rainpoints->NormalizedPoint[this->NewSwapBuffer].z -= distZ;
    rainpoints->NormalizedPoint[this->OldSwapBuffer] = rainpoints->NormalizedPoint[this->NewSwapBuffer];
#endif
}

void Rain::SeedCurtainX(RainPointsDef *rainpoints) {
#ifndef DISABLE_RAIN
    float distX = bRandom(this->CurtainLength);
    float distY = bRandom(10.0f);
    bVector3 Along = this->RainCurtainPos[1] - this->RainCurtainPos[0];

    bNormalize(&Along, &Along);
    rainpoints->NormalizedPoint[this->NewSwapBuffer] = this->RainCurtainPos[0] + Along * distX + this->outvex * distY;
    rainpoints->NormalizedPoint[this->OldSwapBuffer] = rainpoints->NormalizedPoint[this->NewSwapBuffer];
#endif
}

static const int showCurtain = 0;
static const float curtainWidthMod = 2.0f;

void Rain::UpdateAndRenderCurtain() {
#ifndef DISABLE_RAIN
    int numCurtainPoints;
    bVector3 downV;
    bVector3 acrossV;
    bVector3 *CameraDirection;
    ePoly *PRECIPpolyPoint;
    bVector3 CamVelloc;
    bMatrix4 l2w(this->local2world);
    bVector2 outvex2;
    bVector2 cp;

    CameraDirection = this->MyView->GetCamera()->GetDirection();
    PRECIPpolyPoint = &this->PRECIPpoly[0];
    numCurtainPoints = static_cast<int>(this->intensity * 200.0f);

    l2w.v3.x = 0.0f;
    l2w.v3.y = 0.0f;
    l2w.v3.z = 0.0f;
    l2w.v3.w = 1.0f;
    eMulVector(&CamVelloc, &l2w, &this->CamVelLOCAL);

    cp = bVector2((this->RainCurtainPos[0].x + this->RainCurtainPos[1].x) * 0.5f, (this->RainCurtainPos[0].y + this->RainCurtainPos[1].y) * 0.5f) -
         (static_cast<TrackPathZone *>(this->the_zone)->BBoxMin + static_cast<TrackPathZone *>(this->the_zone)->BBoxMax) * 0.5f;
    outvex2 = cp;
    bNormalize(&outvex2, &outvex2);
    this->outvex = bVector3(outvex2.x, outvex2.y, 0.0f);

    for (int i = 0; i < numCurtainPoints; i++) {
        bVector3 *RpointN = &this->CurtainRainPoints[i].NormalizedPoint[this->NewSwapBuffer];
        bVector3 *RpointNold = &this->CurtainRainPoints[i].NormalizedPoint[this->OldSwapBuffer];

        *RpointN = CamVelloc + *RpointNold + (this->Velocities[RAIN][i % 10] + this->windSpeed * this->precipWindEffect[RAIN][1]);

        if (RpointN->z < this->RainCurtainPos[3].z || RpointN->z > this->RainCurtainPos[0].z) {
            this->SeedCurtainX(&this->CurtainRainPoints[i]);
        } else {
            downV = *RpointNold - *RpointN;
            bNormalize(&downV, &downV);
            bCross(&acrossV, CameraDirection, &downV);
            acrossV *= this->precipRadius[RAIN].x * curtainWidthMod;
            downV *= this->precipRadius[RAIN].y + this->LenModifier;

            PRECIPpolyPoint->Vertices[0] = *RpointN - (acrossV + downV);
            PRECIPpolyPoint->Vertices[1] = *RpointN + (acrossV - downV);
            PRECIPpolyPoint->Vertices[3] = *RpointN + (downV - acrossV);
            PRECIPpolyPoint->Vertices[2] = *RpointN + (acrossV + downV);
            this->MyView->Render(PRECIPpolyPoint, this->texture_info[RAIN], &eMathIdentityMatrix, 0, 0.5f);
        }
    }
#endif
}

void Rain::Debug() {
#ifndef DISABLE_RAIN
    static float zspeed;

    snowPercent = 1.0f - rainPercent;

    this->precipWindEffect[RAIN][0] = RAINwindEffect;
    this->precipWindEffect[RAIN][1] = RAINwindEffect * 0.5f;
    this->precipRadius[RAIN] = bVector3(RAINRadiusX, RAINRadiusY, RAINRadiusZ);
    this->precipSpeedRange[RAIN] = bVector3(RAINX, RAINY, RAINZ);
    this->precipZconstant[RAIN] = RAINZconstant;

    if (zspeed != RAINZconstant) {
        for (int j = 0; j < 2; j++) {
            for (int i = 0; i < 10; i++) {
                this->Velocities[j][i].x = bRandom(this->precipSpeedRange[j].x) - this->precipSpeedRange[j].x * 0.5f;
                this->Velocities[j][i].y = bRandom(this->precipSpeedRange[j].y) - this->precipSpeedRange[j].y * 0.5f;
                this->Velocities[j][i].z = -bRandom(this->precipSpeedRange[j].z) - this->precipZconstant[j];
                this->Velocities[j][i].pad = -99999.9f;
            }
        }
        zspeed = RAINZconstant;
    }

    this->Change(RAIN, rainPercent * (1.0f - fogPercent));
#endif
}

static const int windON = 1;
static const int freezing = 0;
static const float WetRamp = 0.005f;
#ifdef EA_BUILD_A124
static const float DryRamp = 0.01f;
#else
static const float DryRamp = 0.005f;
#endif

static const float AngMuly = 20.0f;
static const float RainAngMult = 0.00005;
static const float FloatClamp = 400.0f;
static const int ForceUpdate = 0;

static const float RainAheadCut = 9.5f;
int32 ONEpNoRainConnexion = 0;

float cameraMod = 1.0f;
static const float lookbackoffset = 10.0f;

void fInvertMatrix(bMatrix4 *d, bMatrix4 *s);

void Rain::Update() {
#ifndef DISABLE_RAIN
    float timeMod;
    float tempMod;
    bVector3 MyCarPos;
    bVector3 MyCarDir;
    Camera *view_camera;
    bVector3 *CameraPosition;
#ifndef EA_BUILD_A124
    bVector3 *CameraDirection;
#endif
    bVector3 *CameraVelocity;
    CameraMover *cameraMover;
    CameraAnchor *cameraAnchor;
    float curElev;
    TrackPathZone *zone;

    if (!IsGameFlowInGame() || TweakerPauseWorld) {
        return;
    }

    timeMod = WorldTimeElapsed * 30.0f;
    if (timeMod > 1.0f) {
        timeMod = 1.0f;
    }
    tempMod = 1.0f - timeMod;
    timeMod = 1.0f - tempMod * tempMod * tempMod;

    view_camera = this->MyView->GetCamera();
    cameraMover = this->MyView->GetCameraMover();
    CameraPosition = view_camera->GetPosition();
#ifndef EA_BUILD_A124
    CameraDirection = view_camera->GetDirection();
#endif
    CameraVelocity = view_camera->GetVelocityPosition();
    cameraAnchor = cameraMover->GetAnchor();

    if (cameraMover == nullptr) {
        return;
    }

#ifdef EA_BUILD_A124
    if (cameraAnchor == nullptr) {
#else
    if (cameraMover->GetType() != CM_DRIVE_CUBIC || cameraAnchor == nullptr) {
#endif
        this->local2world = *view_camera->GetCameraMatrix();
        this->local2world.v3.x = 0.0f;
        this->local2world.v3.y = 0.0f;
        this->local2world.v3.z = 0.0f;
        this->local2world.v3.w = 1.0f;
#ifdef EA_BUILD_A124
        MyCarDir = *view_camera->GetDirection();
#else
        MyCarDir = *CameraDirection;
#endif
        MyCarPos = *CameraPosition + MyCarDir * cameraMod;
    } else {
        this->local2world = *cameraAnchor->GetGeometryOrientation();
        MyCarPos = *cameraAnchor->GetGeometryPosition();
        MyCarDir = *cameraAnchor->GetForwardVector();

        if (cameraMover->GetLookbackAngle()) {
            MyCarPos -= MyCarDir * lookbackoffset;
        }
    }

    this->twoDpos = bVector2(CameraPosition->x, CameraPosition->y);
    this->inTunnel = 0;
    this->inOverpass = 0;
    curElev = 9999.0f;

    zone = TheTrackPathManager.FindZone(&this->twoDpos, TRACK_PATH_ZONE_TUNNEL, nullptr);
    if (zone != nullptr && MyCarPos.z < zone->GetElevation()) {
        this->inTunnel = 1;
        this->the_zone = zone;
        curElev = zone->GetElevation();
    }

    zone = TheTrackPathManager.FindZone(&this->twoDpos, TRACK_PATH_ZONE_OVERPASS, nullptr);
    if (zone != nullptr && MyCarPos.z < zone->GetElevation()) {
        if (zone->GetElevation() < curElev) {
            this->the_zone = zone;
            this->inOverpass = 1;
            curElev = zone->GetElevation();
        }
    }

    zone = TheTrackPathManager.FindZone(&this->twoDpos, TRACK_PATH_ZONE_OVERPASS_SMALL, nullptr);
    if (zone != nullptr && MyCarPos.z < zone->GetElevation()) {
        if (zone->GetElevation() < curElev) {
            this->the_zone = zone;
            this->inOverpass = 1;
            curElev = zone->GetElevation();
        }
    }

    zone = TheTrackPathManager.FindZone(&this->twoDpos, TRACK_PATH_ZONE_GARAGE, nullptr);
    if (zone != nullptr && MyCarPos.z < zone->GetElevation()) {
        if (zone->GetElevation() < curElev) {
            this->the_zone = zone;
            this->inOverpass = 1;
            curElev = zone->GetElevation();
        }
    }

    fInvertMatrix(&this->world2localrot, &this->local2world);

    this->Wind(WorldTimeElapsed);

    this->Change(RAIN, rainPercent);

    if (cameraMover->IsDriveCamera()) {
        eMulVector(&this->CamVelLOCAL, &this->world2localrot, CameraVelocity);
        this->CamVelLOCAL *= driveFactor;
        this->CamVelLOCAL.y = 0.0f;
        this->LenModifier = bLength(&this->CamVelLOCAL);
        if (this->LenModifier > 1.0f) {
            this->LenModifier = 1.0f;
            bNormalize(&this->CamVelLOCAL, &this->CamVelLOCAL);
        }
    } else {
        this->CamVelLOCAL = bVector3(0.0f, 0.0f, 0.0f);
    }

    if (precipDEBUG) {
        this->Debug();
    }

    bNormalize(&MyCarDir, &MyCarDir);
    bVector3 Ahead(1.0f, 0.0f, 0.0f);
    bAngle matAng = bACos(bDot(&MyCarDir, &Ahead));

    if (MyCarDir.y < 0.0f) {
        matAng = -1100 - matAng;
    }

    bMatrix4 matmat;
    eCreateRotationZ(&matmat, matAng);
    matmat.v3.x = MyCarPos.x;
    matmat.v3.y = MyCarPos.y;
    matmat.v3.z = MyCarPos.z;
    matmat.v3.w = 1.0f;
    this->local2world = matmat;

    bVector3 CarCross = bCross(MyCarDir, this->OldCarDirection);

    static bAngle oldangle[2];
    int index = this->MyView->GetID() != EVIEW_PLAYER1;
    float FloatAngle = static_cast<bAngle>(oldangle[index] - matAng);
    oldangle[index] = matAng;

    if (FloatAngle >= 32768.0f) {
        FloatAngle -= 65536.0f;
    }
    if (FloatAngle > FloatClamp) {
        FloatAngle = FloatClamp;
    }
    if (FloatAngle < -FloatClamp) {
        FloatAngle = -FloatClamp;
    }

    this->aabbMax.x = precipAheadX + precipBoundX * 0.5f;
    this->aabbMin.x = precipAheadX - precipBoundX * 0.5f;
    this->aabbMax.y = precipAheadY + precipBoundY * 0.5f;
    this->aabbMin.y = precipAheadY - precipBoundY * 0.5f;

    uint32 tb = this->OldSwapBuffer;
    this->OldSwapBuffer = this->NewSwapBuffer;
    this->NewSwapBuffer = tb;

    int total = 0;
    for (int j = 0; j < 2; j++) {
        total += this->DesiredNumOfType[j];
    }
    uint32 diff = MAXRAINPOINTS - total;
    if (diff != 0) {
        for (int j = 0; j < 2; j++) {
            if (this->DesiredNumOfType[j] >= diff) {
                this->DesiredNumOfType[j] -= diff;
                break;
            }
        }
    } else {
        for (int j = 0; j < 2; j++) {
            if (this->DesiredNumOfType[j] <= total) {
                this->DesiredNumOfType[j] += diff;
                break;
            }
        }
    }

    uint32 steadyPrecip = 1;
    for (int j = 0; j < 2; j++) {
        if (this->NumOfType[j] != this->DesiredNumOfType[j]) {
            steadyPrecip = 0;
            break;
        }
    }

    this->twoDpos = bVector2(MyCarPos.x, MyCarPos.y);
    this->NoRain = 0;
    if (this->inOverpass || this->inTunnel) {
        this->NoRain = 1;
    }
    if (this->MyView->GetID() == EVIEW_PLAYER1) {
        ONEpNoRainConnexion = this->NoRain;
    }
    this->NoRainAhead = this->NoRain;

    for (int i = 0; i < this->NumRainPoints; i++) {
        RainType rType = static_cast<RainType>(this->RainPointsInf[i].type);
        RainSubType rSubType = static_cast<RainSubType>(this->RainPointsInf[i].subType);

        switch (this->RainPointsInf[i].status) {
        case IN_AIR: {
            bVector3 *RpointN = &this->RainPoints[i].NormalizedPoint[this->NewSwapBuffer];
            bVector3 *RpointNold = &this->RainPoints[i].NormalizedPoint[this->OldSwapBuffer];

            RpointN->y += RpointN->x * RainAngMult * FloatAngle;
            RpointNold->y += RpointNold->x * RainAngMult * FloatAngle;

            *RpointN = this->CamVelLOCAL + *RpointNold +
                       (this->Velocities[rType][i % 10] + this->windSpeed * this->precipWindEffect[rType][rSubType]) * timeMod;

            if (RpointN->z < 0.0f) {
                this->RainPointsInf[i].status = ON_GROUND;
            } else if (RpointN->x > this->aabbMax.x) {
                RpointN->x -= precipBoundX;
                RpointNold->x -= precipBoundX;
            } else if (RpointN->x < this->aabbMin.x) {
                RpointN->x += precipBoundX;
                RpointNold->x += precipBoundX;
            } else if (RpointN->y > this->aabbMax.y) {
                RpointN->y -= precipBoundY;
                RpointNold->y -= precipBoundY;
            } else if (RpointN->y < this->aabbMin.y) {
                RpointN->y += precipBoundY;
                RpointNold->y += precipBoundY;
            }
            break;
        }
        case ON_GROUND:
        case IN_LIMBO:
            if (!steadyPrecip) {
                for (int j = 0; j < 2; j++) {
                    if (rType != j && this->NumOfType[j] < this->DesiredNumOfType[j]) {
                        this->NumOfType[rType]--;
                        this->RainPointsInf[i].type = j;
                        this->NumOfType[j]++;
                        j = NUMTYPES;
                    }
                }
            }

            if (this->RainPointsInf[i].type != INACTIVE) {
                if (this->RainPointsInf[i].status == ON_GROUND) {
                    this->RainPoints[i].NormalizedPoint[this->NewSwapBuffer].z += precipBoundZ;
                    this->RainPoints[i].NormalizedPoint[this->OldSwapBuffer].z += precipBoundZ;
                }
                this->RainPointsInf[i].status = IN_AIR;
            } else if (this->RainPointsInf[i].status == ON_GROUND) {
                this->RainPoints[i].NormalizedPoint[this->NewSwapBuffer].z = precipAheadZ + bRandom(precipBoundZ);
                this->RainPointsInf[i].status = IN_LIMBO;
            }
            break;
        }

        if (this->NoRain && this->RainPoints[i].NormalizedPoint[this->NewSwapBuffer].x <= RainAheadCut) {
            this->NumOfType[this->RainPointsInf[i].type]--;
            this->RainPointsInf[i].type = INACTIVE;
            this->NumOfType[INACTIVE]++;
            this->RainPointsInf[i].status = IN_LIMBO;
        }
    }
#endif
}

int windAngType = 4;
float windAng = 0.0f;
float swayMax = 1.5f;
float wspeed = 45.0f;

bVector3 windAxis;

// UNSOLVED
void CreateWindRotMatrix(eView *view, bMatrix4 *windrot, int offset, bMatrix4 *l2w) {
    static int index = 0;
    bMatrix4 local2world(*l2w);
    float sway = bSin(bDegToAng(windAng + offset)) * swayMax;

    bIdentity(windrot);
    windAxis = bVector3(1.0f, 0.0f, 0.0f);

#ifndef EA_BUILD_A124
    if (view->Precipitation != nullptr)
#endif
    {
        bNormalize(&windAxis, view->Precipitation->GetWind());
    }

    local2world.v3.x = 0.0f;
    local2world.v3.y = 0.0f;
    local2world.v3.z = 0.0f;
    local2world.v3.w = 1.0f;
    local2world.v1.x *= -1.0f;
    local2world.v0.y *= -1.0f;
    eMulVector(&windAxis, &local2world, &windAxis);
    eCreateAxisRotationMatrix(windrot, windAxis, bDegToAng(sway));
    eRotateZ(windrot, windrot, bDegToAng(sway));
}

float maxWindEffect = 25.0f;

ParameterAccessorBlendByDistance WindAccessor[2] = {"Wind", "Wind"};

float PrevailingMult = 0.01f;

void Rain::Wind(float time) {
    static ChangingStatus windState = CHANGE;
    static float changetime = 0.0f;
    // Xbox 360 la tiene vacia (4 B).
#ifndef EA_PLATFORM_XENON
    int idIndex = this->MyView->GetID() - 1;

    if (WindAccessor[idIndex].IsValid()) {
        float x = 0.0f;
        float y = 0.0f;
        float Power;
        float Angle;

        GetXYviewCar(this->MyView, &x, &y);
        WindAccessor[idIndex].CaptureData(x, y, 50.0f);
        Power = WindAccessor[idIndex].GetDataFloat(0);
        Angle = WindAccessor[idIndex].GetDataFloat(1);
        this->PrevailingWindSpeed.x = bSin(bDegToAng(Angle));
        this->PrevailingWindSpeed.y = bCos(bDegToAng(Angle));
        this->PrevailingWindSpeed.z = 0.0f;
    }

    this->PrevailingWindSpeed *= PrevailingMult;
    eMulVector(&this->PrevailingWindSpeed, &this->world2localrot, &this->PrevailingWindSpeed);

    switch (windState) {
    case STEADY:
        this->windTime += time;
        if (this->windTime >= this->DesiredWindTime) {
            windState = CHANGE;
        }
        break;
    case CHANGE:
        this->DesiredWindTime = bRandom(5.0f) + 1.0f;
        this->windTime = 0.0f;
        windState = CHANGING;
        this->DesiredwindSpeed.x = (bRandom(0.05f) - 0.025f) * maxWindEffect;
        this->DesiredwindSpeed.y = (bRandom(0.05f) - 0.025f) * maxWindEffect;
        this->DesiredwindSpeed.z = 0.0f;
        this->DesiredwindSpeed += this->PrevailingWindSpeed;
        break;
    case CHANGING: {
        bVector3 delta = this->DesiredwindSpeed - this->windSpeed;

        this->windSpeed.x += delta.x * time;
        this->windSpeed.y += delta.y * time;
        changetime += time;
        if (changetime >= 1.0f) {
            changetime = 0.0f;
            windState = STEADY;
        }
        break;
    }
    }
#endif
}

// STRIPPED
void Rain::Reset() {}

float FrustrumFactor = 0.0001f;

float precipBoundD = 10.0f;

// STRIPPED
void Rain::Seed(RainPointsDef *rainpoints, bVector3 *CameraPosition, bVector3 *CameraDirection) {}

void Rain::SetRainIntensity(float percent) {
#ifndef DISABLE_RAIN
    if (precipDEBUG) {
        this->intensity = precipPERCENT;
    } else {
        this->intensity = percent;
    }

    this->DesiredNumOfType[INACTIVE] = static_cast<uint32>((1.0f - this->intensity) * MAXRAINPOINTS);

    this->DesiredActive = MAXRAINPOINTS - this->DesiredNumOfType[INACTIVE];

    float sumPercent = 0.0f;
    for (int j = 0; j < NUMTYPES - 1; j++) {
        sumPercent += this->Percentages[j];
    }
    this->Percentages[INACTIVE] = 1.0f - sumPercent;
#endif
}

void Rain::Change(RainType type, float percent) {
#ifndef DISABLE_RAIN
    this->DesiredNumOfType[type] = static_cast<uint32>(percent * this->DesiredActive);
    this->Percentages[type] = percent;
#endif
}

float rw = 0.2f;
float rh = 0.2f;
float rl = 0.2f;
unsigned int RainCube = 0;
float px0 = -1.0f;
float py0 = -1.0f;
float px1 = 1.0f;
float py1 = -1.0f;
float px2 = 1.0f;
float py2 = 1.0f;
float px3 = -1.0f;
float py3 = 1.0f;

void Rain::Render() {
#ifndef DISABLE_RAIN
    bMatrix4 Rmat;
    bVector3 CamDirWORLD;
    bVector3 CamPosWORLD;
    bVector3 *CameraDirection;
    bMatrix4 *local_world;

    if (!PrecipitationEnable) {
        return;
    }
    if (!precipRENDER) {
        return;
    }
    if (!IsGameFlowInGame()) {
        return;
    }
    if (!this->renderCount) {
        return;
    }

    bIdentity(&Rmat);
    CamDirWORLD = *this->MyView->GetCamera()->GetDirection();
    CamPosWORLD = *this->MyView->GetCamera()->GetPosition();
    CameraDirection = &CamDirWORLD;
    eMulVector(CameraDirection, &this->world2localrot, CameraDirection);

    local_world = static_cast<bMatrix4 *>(eFrameMalloc(sizeof(bMatrix4)));
    if (local_world != nullptr) {
        *local_world = this->local2world;

        uint32 NumRainPointsLocal = this->NumRainPoints;
        for (int j = 0; j < NUMTYPES - 1; j++) {
            uint32 NumOfTypeLocal = this->NumOfType[j];

            if (NumOfTypeLocal != 0) {
                for (uint32 i = 0; i < NumRainPointsLocal; i++) {
                    RainType rType = static_cast<RainType>(this->RainPointsInf[i].type);

                    if (rType == j) {
                        bVector3 downV;
                        bVector3 acrossV;

                        if (this->RainPointsInf[i].status == IN_AIR) {
                            downV = this->RainPoints[i].NormalizedPoint[this->OldSwapBuffer] - this->RainPoints[i].NormalizedPoint[this->NewSwapBuffer];
                            bNormalize(&downV, &downV);
                            bCross(&acrossV, CameraDirection, &downV);
                            acrossV *= this->precipRadius[j].x;
                            downV *= this->precipRadius[j].y + this->LenModifier;
                        }

                        if (this->RainPointsInf[i].status == IN_AIR) {
                            this->PRECIPpoly[0].Vertices[0] = this->RainPoints[i].NormalizedPoint[this->NewSwapBuffer] - (acrossV + downV);
                            this->PRECIPpoly[0].Vertices[1] = this->RainPoints[i].NormalizedPoint[this->NewSwapBuffer] + (acrossV - downV);
                            this->PRECIPpoly[0].Vertices[3] = this->RainPoints[i].NormalizedPoint[this->NewSwapBuffer] + (downV - acrossV);
                            this->PRECIPpoly[0].Vertices[2] = this->RainPoints[i].NormalizedPoint[this->NewSwapBuffer] + (acrossV + downV);
                            this->MyView->Render(&this->PRECIPpoly[0], this->texture_info[j], local_world, 0, 2.0f);
                        }
                    }
                }
            }
        }
    }
#endif
}

OnScreenRain::OnScreenRain() {
#ifndef DISABLE_RAIN
    this->NumOnScreen = 0;
    for (int i = 0; i < MAX_OS_RAINPOINTS; i++) {
        this->Points[i].x = 0.0f;
        this->Points[i].y = 0.0f;
        this->Points[i].timer = 0.0f;
        this->Points[i].Maxtimer = bRandom(5.0f) + 0.3f;
        this->Points[i].DripSpeed = bRandom(0.1f) + 0.02;
        this->Points[i].Size = bRandom(1.0f);
        this->Points[i].DripShape = i % NUM_DRIP_SHAPES;
    }
#endif
}

float DripSpeed = 0.2f;
float SpeedMod = 0.0001f;
float DropShapeSpeedChange = 0.0025f;
int OverRide = 0;

void OnScreenRain::Update(eView *view) {
#ifndef DISABLE_RAIN
    float time = WorldTimeElapsed;
    bVector3 camera_world_velocity(*view->GetCamera()->GetVelocityPosition());
    float camera_speed = bLength(&camera_world_velocity) * SpeedMod;

    if (view->Precipitation->NoRain) {
        this->NumOnScreen = 0;
    } else if (eGetCurrentViewMode() >= EVIEWMODE_TWOH) {
        this->NumOnScreen = view->Precipitation->GetRainIntensity() == 0.0f ? 0 : 10;
    } else {
        this->NumOnScreen = view->Precipitation->GetRainIntensity() == 0.0f ? 0 : 20;
    }

    for (int i = 0; i < this->NumOnScreen; i++) {
        OnScreenRainPointsDef *Pts = &this->Points[i];

        if (!OverRide) {
            Pts->timer -= time;
        }
        if (Pts->timer <= 0.0f) {
            Pts->timer = Pts->Maxtimer;
            Pts->x = bRandom(1.0f);
            Pts->y = bRandom(1.0f);
        } else {
            float dy = Pts->DripSpeed * time * DripSpeed;
            float dx = 0.0f;

            Pts->y += dy;
            if (Pts->y > 1.0f || Pts->x > 1.0f || Pts->x < 0.0f) {
                Pts->timer = 0.0f;
            } else {
                bVector3 sv(Pts->x - 0.5f, Pts->y - 0.1f, 0.0f);

                bNormalize(&sv, &sv);
                sv *= camera_speed;
                dx = sv.x;
                Pts->x += dx;
            }

            if (camera_speed > DropShapeSpeedChange) {
                Pts->DripShape = (Pts->DripShape + 1) % NUM_DRIP_SHAPES;
            }
        }
    }
#endif
}

void OnScreenRain::GetData(int index, float *x, float *y, float *decay, float *size, int *dripShape) {
#ifndef DISABLE_RAIN
    *x = this->Points[index].x;
    *y = this->Points[index].y;
    *decay = this->Points[index].timer / this->Points[index].Maxtimer;
    *size = this->Points[index].Size;
    if (dripShape) {
        *dripShape = this->Points[index].DripShape;
    }
#endif
}

float RAINbias = 0.0f;
float SNOWbias = 0.0f;
float SLEETbias = 0.0f;
float HAILbias = 0.0f;
float FOGbias = 0.0f;
float BaseDampness = 1.0f;

static const int WatchRain = 0;

void Rain::UpdateAndRender() {
    ProfileNode profile_node;

    if (!IsGameFlowInGame()) {
        return;
    }

    float time = WorldTimeElapsed;

    if (IsGameFlowPaused()) {
        ScreenEffectDef SE_def;
        uint32 r;
        uint32 g;
        uint32 b;

        this->GetPrecipFogColour(&r, &g, &b);
        SE_def.r = r;
        SE_def.g = g;
        SE_def.b = b;
        SE_def.a = 128.0f;
        SE_def.UpdateFnc = nullptr;
#ifdef EA_BUILD_A124
        SE_def.intensity = this->GetCloudIntensity();
#else
        if (eGetCurrentViewMode() >= EVIEWMODE_TWOH && this->GetRainIntensity() > 0.0f) {
            SE_def.intensity = this->GetCloudIntensity() * 8.0f;
        } else {
            SE_def.intensity = this->GetCloudIntensity();
        }
#endif
        this->MyView->ScreenEffects->AddScreenEffect(SE_TINT, &SE_def, 1, SEC_FRAME);
        this->Render();
        return;
    }

    {
        static float ChangeWeatherTime = 0.0f;

        this->OSrain.Update(this->MyView);

        windAng += time * wspeed;
        if (windAng > 360.0f) {
            windAng = 0.0f;
        }

        if (!PrecipitationEnable) {
            this->inOverpass = 0;
            this->inTunnel = 0;
            this->inTunnel = AmIinATunnelSlow(this->MyView, 1);
            if (this->inTunnel & 2) {
                this->inOverpass = 1;
                this->inTunnel = 0;
            }
            if (this->inTunnel) {
                this->inTunnel = 1;
            }
            this->intensity = 0.0f;
            this->RoadDampness = 0.0f;
            return;
        }

        if (precipDEBUG) {
            this->RoadDampness = BaseDampness;
        } else if (IsGameFlowInGame()) {
            this->DesiredRoadDampness = this->GetRainIntensity() == 0.0f ? 0.0f : BaseDampness;
        } else {
            this->DesiredRoadDampness = 1.0f;
            return;
        }

        if (this->DesiredRoadDampness > this->RoadDampness) {
            this->RoadDampness += (this->DesiredRoadDampness - this->RoadDampness) * WetRamp;
        } else {
            this->RoadDampness += (this->DesiredRoadDampness - this->RoadDampness) * DryRamp;
            if (this->RoadDampness < 0.05f) {
                this->RoadDampness = 0.0f;
            }
        }

        if (RainAccessor.IsValid() && CloudAccessor.IsValid()) {
            float x = 0.0f;
            float y = 0.0f;

            GetXYviewCar(this->MyView, &x, &y);
            this->DesiredIntensity = GetDesiredRainIntensity(x, y);
            if (this->DesiredIntensity > 0.0f) {
                this->DesiredCloudyness = this->DesiredIntensity;
            } else {
                this->DesiredCloudyness = GetDesiredCloudyness(x, y);
            }
            if (eGetCurrentViewMode() >= EVIEWMODE_TWOH) {
                this->DesiredIntensity *= 0.125f;
            }
        }
    }

    if (time == 0.0f) {
        time = 0.001f;
    }

    this->intensity += (this->DesiredIntensity - this->intensity) / ((1.0f / time) * twkRainRateOfChange);
    if (this->DesiredIntensity == 0.0f && this->intensity < 0.01f) {
        this->intensity = 0.0f;
    }
    if (this->intensity > 1.0f) {
        this->intensity = 1.0f;
    }

    this->CloudIntensity += (this->DesiredCloudyness - this->CloudIntensity) / ((1.0f / time) * twkCloudsRateOfChange);
    if (this->DesiredCloudyness == 0.0f && this->CloudIntensity < twkCloudsMinAmount + 0.01f) {
        this->CloudIntensity = twkCloudsMinAmount;
    }
    if (this->CloudIntensity > 1.0f) {
        this->CloudIntensity = 1.0f;
    }

    this->SetRainIntensity(this->intensity);
    this->percentPrecip[RAIN] = 1.0f;
    this->Update();
    this->Render();

#ifndef DISABLE_RAIN
    if (AmIinATunnel(this->MyView, 1)) {
        switch (this->IsValidRainCurtainPos) {
        case CT_INACTIVE:
            this->IsValidRainCurtainPos = CT_TURNON;
            this->FindCurtains();
            this->FindCurtain();
            for (int i = 0; i < MAXCURTAINRAINPOINTS; i++) {
                this->SeedCurtainXZ(&this->CurtainRainPoints[i]);
            }
            break;
        case CT_TURNON:
            this->IsValidRainCurtainPos = CT_ACTIVE;
        case CT_ACTIVE:
            this->FindCurtain();
            break;
        case CT_OVERIDE:
            this->IsValidRainCurtainPos = CT_ACTIVE;
            break;
        }
        this->UpdateAndRenderCurtain();
    } else {
        this->IsValidRainCurtainPos = CT_INACTIVE;
    }
#endif
}

ParameterAccessorBlendByDistance FogAccessor[2] = {"Normal Fog", "Normal Fog"};
ParameterAccessorBlendByDistance RainFogAccessor[2] = {"Rain Fog", "Rain Fog"};
ParameterAccessor FogBlendDistAccessor[2] = {"Fog Blend Distance", "Fog Blend Distance"};

int FogControlOverRide = 0;

static const bool grabFogInfo = false;

#if defined(EA_PLATFORM_XENON) || defined(EA_PLATFORM_WIN32)
// En Xbox 360 mide 4 B (sin cuerpo).
int FogQuery::CalculateFogInfo(struct View *view, int InFE) {}
#else
// En GameCube esta estripada; el DWARF da los mismos locales y los mismos inline que las dos PS2.
int FogQuery::CalculateFogInfo(eView *view, int InFE) {
    int idIndex = view->GetID() - 1;

    if (FogAccessor[idIndex].IsValid()) {
        float x = 0.0f;
        float y = 0.0f;
        float intense;
        float start;
        float red;
        float green;
        float blue;
        float Falloff;
        float FalloffX;
        float FalloffY;
        uint32 tcolr_r;
        uint32 tcolr_g;
        uint32 tcolr_b;
        uint32 retcol;
        float RainIntensity;

        GetXYviewCar(view, &x, &y);

        FogAccessor[idIndex].CaptureData(x, y, 50.0f);
        intense = FogAccessor[idIndex].GetDataFloat(0);
        start = FogAccessor[idIndex].GetDataFloat(1);
        red = FogAccessor[idIndex].GetDataFloat(2);
        green = FogAccessor[idIndex].GetDataFloat(3);
        blue = FogAccessor[idIndex].GetDataFloat(4);
        Falloff = FogAccessor[idIndex].GetDataFloat(5);
        FalloffX = FogAccessor[idIndex].GetDataFloat(6);
        FalloffY = FogAccessor[idIndex].GetDataFloat(7);

        FogFalloff = Falloff;
        FogFalloffX = FalloffX;
        FogFalloffY = FalloffY;
        DistFogStart = start;
        DistFogPower = intense;

        tcolr_r = static_cast<int>(red);
        tcolr_g = static_cast<int>(green);
        tcolr_b = static_cast<int>(blue);
        retcol = tcolr_r | (tcolr_g << 8) | (tcolr_b << 16) | 0x80000000;
        DistFogColour = retcol;

        if (FogControlOverRide) {
            FogFalloff = BaseFogFalloff;
            FogFalloffX = BaseFogFalloffX;
            FogFalloffY = BaseFogFalloffY;
            DistFogStart = BaseWeatherFogStart;
            DistFogPower = BaseWeatherFog;

            uint32 fog_colour = (BaseWeatherFogColourB << 16) | (BaseWeatherFogColourG << 8) | BaseWeatherFogColourR;
            uint32 colr_r = fog_colour & 0xFF;
            uint32 colr_g = (fog_colour >> 8) & 0xFF;
            uint32 colr_b = (fog_colour >> 16) & 0xFF;

            red = colr_r;
            green = colr_g;
            blue = colr_b;

            DistFogStart = BaseWeatherFogStart;

            uint32 retcol = colr_r | (colr_g << 8) | (colr_b << 16) | 0x80000000;
            DistFogColour = retcol;
        }

        // La A124 aun no comprueba que la vista tenga lluvia.
#ifndef EA_BUILD_A124
        RainIntensity = view->Precipitation ? view->Precipitation->GetRainIntensity() : 0.0f;
#else
        RainIntensity = view->Precipitation->GetRainIntensity();
#endif
        if (RainIntensity == 0.0f) {
#ifndef EA_BUILD_A124
            RainIntensity = view->Precipitation ? view->Precipitation->GetCloudIntensity() : 0.0f;
#else
            RainIntensity = view->Precipitation->GetCloudIntensity();
#endif
            RainIntensity *= 0.33333334f;
        }

        if (RainIntensity > 0.0f) {
            float bd = 50.0f;

            if (FogBlendDistAccessor[idIndex].IsValid()) {
                FogBlendDistAccessor[idIndex].CaptureData(x, y);
                bd = FogBlendDistAccessor[idIndex].GetDataFloat(0);
            }

            if (RainFogAccessor[idIndex].IsValid()) {
                float invRainIntensity = 1.0f - RainIntensity;
                float red2;
                float green2;
                float blue2;

                RainFogAccessor[idIndex].CaptureData(x, y, bd);
                intense = RainFogAccessor[idIndex].GetDataFloat(0);
                start = RainFogAccessor[idIndex].GetDataFloat(1);
                red2 = RainFogAccessor[idIndex].GetDataFloat(2);
                green2 = RainFogAccessor[idIndex].GetDataFloat(3);
                blue2 = RainFogAccessor[idIndex].GetDataFloat(4);
                Falloff = RainFogAccessor[idIndex].GetDataFloat(5);
                FalloffX = RainFogAccessor[idIndex].GetDataFloat(6);
                FalloffY = RainFogAccessor[idIndex].GetDataFloat(7);

                FogFalloff = invRainIntensity * FogFalloff + Falloff * RainIntensity;
                FogFalloffX = invRainIntensity * FogFalloffX + FalloffX * RainIntensity;
                FogFalloffY = invRainIntensity * FogFalloffY + FalloffY * RainIntensity;
                DistFogStart = invRainIntensity * DistFogStart + start * RainIntensity;
                DistFogPower = invRainIntensity * DistFogPower + intense * RainIntensity;

                red = invRainIntensity * red + red2 * RainIntensity;
                green = invRainIntensity * green + green2 * RainIntensity;
                blue = invRainIntensity * blue + blue2 * RainIntensity;

                tcolr_r = static_cast<int>(red);
                tcolr_g = static_cast<int>(green);
                tcolr_b = static_cast<int>(blue);
#ifndef EA_BUILD_A124
                if (view->Precipitation) {
                    view->Precipitation->SetPrecipFogColour(static_cast<int>(red2), static_cast<int>(green2),
                                                            static_cast<int>(blue2));
                }
#else
                view->Precipitation->SetPrecipFogColour(static_cast<int>(red2), static_cast<int>(green2),
                                                        static_cast<int>(blue2));
#endif
                retcol = tcolr_r | (tcolr_g << 8) | (tcolr_b << 16) | 0x80000000;
                DistFogColour = retcol;
            }
        }

        return 1;
    }

    return 0;
}
#endif

int AmIinATunnel(eView *view, int CheckOverPass) {
#ifndef EA_BUILD_A124 // la A124 no comprueba que haya precipitacion
    if (view->Precipitation == nullptr) {
        return 0;
    }
#endif

    if (CheckOverPass != 0) {
        return static_cast<int>(view->Precipitation->inTunnel != 0 || view->Precipitation->inOverpass != 0);
    }

    return view->Precipitation->inTunnel;
}

int AmIinATunnelSlow(eView *view, int CheckOverPass) {
    // Xbox 360 la tiene vacia (4 B).
#ifndef EA_PLATFORM_XENON
    CameraMover *cameraMover = view->GetCameraMover();

    if (cameraMover != nullptr) {
        CameraAnchor *cameraAnchor = cameraMover->GetAnchor();

        if (cameraAnchor != nullptr) {
            bVector3 MyCarPos(*cameraAnchor->GetGeometryPosition());
            bVector2 twoDpos(MyCarPos.x, MyCarPos.y);
            int inTunnel = 0;
            TrackPathZone *zone = TheTrackPathManager.FindZone(&twoDpos, TRACK_PATH_ZONE_TUNNEL, nullptr);

            if (zone != nullptr && zone->GetElevation() > MyCarPos.z) {
                inTunnel = 1;
            }

            if (CheckOverPass != 0) {
                zone = TheTrackPathManager.FindZone(&twoDpos, TRACK_PATH_ZONE_OVERPASS, nullptr);
                if (zone != nullptr && zone->GetElevation() > MyCarPos.z) {
                    inTunnel += 2;
                    return inTunnel;
                }

                zone = TheTrackPathManager.FindZone(&twoDpos, TRACK_PATH_ZONE_OVERPASS_SMALL, nullptr);
                if (zone != nullptr && zone->GetElevation() > MyCarPos.z) {
                    inTunnel += 2;
                    return inTunnel;
                }

                zone = TheTrackPathManager.FindZone(&twoDpos, TRACK_PATH_ZONE_GARAGE, nullptr);
                if (zone != nullptr && zone->GetElevation() > MyCarPos.z) {
                    inTunnel += 2;
                    return inTunnel;
                }
            }
            return inTunnel;
        }
    }
    return 0;
#endif
}
