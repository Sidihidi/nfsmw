#ifndef PSX2_LOGITECH_LGWHEELS_H
#define PSX2_LOGITECH_LGWHEELS_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/PSX2/Src/Logitech/LogiForce.h"
#include "Speed/PSX2/Src/Logitech/LogiWheels.h"

// El indice con el que trabajan StopForce e IsPlaying. SOFTSTOP va detras de LG_NUMBER_FORCE_EFFECTS: los arrays
// de wasPlayingBeforeAirborne son de diez.
enum ForceType {
    LG_SPRING = 0,
    LG_CONSTANT = 1,
    LG_DAMPER = 2,
    LG_SIDE_COLLISION = 3,
    LG_FRONTAL_COLLISION = 4,
    LG_DIRT_ROAD = 5,
    LG_BUMPY_ROAD = 6,
    LG_SLIPPERY_ROAD = 7,
    LG_SURFACE_EFFECT = 8,
    LG_CAR_AIRBORNE = 9,
    LG_NUMBER_FORCE_EFFECTS = 10,
    LG_SOFTSTOP = 11,
};

// total size: 0x14C0 (alineado a 64 por el PositionLast de LogiWheels)
struct LGWheels {
    int NonLinearWheel[2][256];  // offset 0x0, size 0x800
    lgDevPosition2 Position;   // offset 0x800, size 0x30

  private:
    LogiWheels wheels;           // offset 0x840, size 0x900
    LogiForce force;             // offset 0x1140, size 0x80
    LogiCondition condition;     // offset 0x11C0, size 0x80
    LogiConstant constant;       // offset 0x1240, size 0x80
    LogiPeriodic periodic;       // offset 0x12C0, size 0x80
    LogiRamp ramp;               // offset 0x1340, size 0x80
    bool damperWasPlaying[2];    // offset 0x13C0, size 0x8
    bool springWasPlaying[2];    // offset 0x13C8, size 0x8
    bool wasPlayingBeforeAirborne[2][10]; // offset 0x13D0, size 0x50
    bool IsAirborne[2];          // offset 0x1420, size 0x8

    struct {
        int offset;      // offset 0x0, size 0x4
        int saturation;  // offset 0x4, size 0x4
        int coefficient; // offset 0x8, size 0x4
    } SpringForceParams[2]; // offset 0x1428, size 0x18

    struct {
        int deadband;    // offset 0x0, size 0x4
        int saturation;  // offset 0x4, size 0x4
        int coefficient; // offset 0x8, size 0x4
    } SoftstopForceParams[2]; // offset 0x1440, size 0x18

    struct {
        int magnitude; // offset 0x0, size 0x4
        int direction; // offset 0x4, size 0x4
    } ConstantForceParams[2]; // offset 0x1458, size 0x10

    struct {
        int coefficient; // offset 0x0, size 0x4
    } DamperForceParams[2]; // offset 0x1468, size 0x8

    struct {
        int magnitude; // offset 0x0, size 0x4
        int direction; // offset 0x4, size 0x4
    } SideCollisionParams[2]; // offset 0x1470, size 0x10

    struct {
        int magnitude; // offset 0x0, size 0x4
    } FrontalCollisionParams[2]; // offset 0x1480, size 0x8

    struct {
        int magnitude; // offset 0x0, size 0x4
    } DirtRoadParams[2]; // offset 0x1488, size 0x8

    struct {
        int magnitude; // offset 0x0, size 0x4
    } BumpyRoadParams[2]; // offset 0x1490, size 0x8

    struct {
        int magnitude; // offset 0x0, size 0x4
    } SlipperyRoadParams[2]; // offset 0x1498, size 0x8

    struct {
        int type;      // offset 0x0, size 0x4
        int magnitude; // offset 0x4, size 0x4
        int period;    // offset 0x8, size 0x4
    } SurfaceEffectParams[2]; // offset 0x14A0, size 0x18

  public:
    LGWheels();
    void ReadAll();
    bool ButtonTriggered(int channel, int buttonMask);
    bool ButtonReleased(int channel, int buttonMask);
    bool ButtonIsPressed(int channel, int buttonMask);
    bool WheelAMenuIsPressed(int channel, int direction);
    bool WheelAMenuTriggered(int channel, int direction);
    bool IsConnected(int channel);
    bool IsWheelClassA(int channel);
    bool IsWheelClassB(int channel);
    bool IsMultiturnCapable(int channel);
    bool IsMultiturnEnabled(int channel);
    void SetMultiturn(int channel, int value);
    int GetMultiturn(int channel);
    void SetOverallForceGain(int channel, int value);
    int GetOverallForceGain(int channel);
    void GenerateNonLinValues(int channel, int nonLinCoeff);
    void PlaySpringForce(int channel, int offset, int saturation, int coefficient);
    void StopSpringForce(int channel);
    void PlaySoftstopForce(int channel, int deadband, int saturation, int coefficient);
    void StopSoftstopForce(int channel);
    void PlayConstantForce(int channel, int magnitude, int direction);
    void StopConstantForce(int channel);
    void PlayDamperForce(int channel, int coefficient);
    void StopDamperForce(int channel);
    void PlaySideCollisionForce(int channel, int magnitude, int direction);
    void PlayFrontalCollisionForce(int channel, int magnitude);
    void PlayDirtRoadEffect(int channel, int magnitude);
    void StopDirtRoadEffect(int channel);
    void PlayBumpyRoadEffect(int channel, int magnitude);
    void StopBumpyRoadEffect(int channel);
    void PlaySlipperyRoadEffect(int channel, int magnitude);
    void StopSlipperyRoadEffect(int channel);
    void PlaySurfaceEffect(int channel, int type, int magnitude, int period);
    void StopSurfaceEffect(int channel);
    void PlayCarAirborne(int channel);
    void StopCarAirborne(int channel);
    bool IsPlaying(int channel, int forceType);
    void PrepareForIOPReboot();
    void InitAfterIOPReboot();

  private:
    void InitVars();
    void GetState();
    void ReadPort(int channel);
    void StopForce(int channel, int forceType);
    bool SameSpringForceParams(int channel, int offset, int saturation, int coefficient);
    bool SameSoftstopForceParams(int channel, int deadband, int saturation, int coefficient);
    bool SameConstantForceParams(int channel, int magnitude, int direction);
    bool SameDamperForceParams(int channel, int coefficient);
    bool SameSideCollisionForceParams(int channel, int magnitude, int direction);
    bool SameFrontalCollisionForceParams(int channel, int magnitude);
    bool SameDirtRoadEffectParams(int channel, int magnitude);
    bool SameBumpyRoadEffectParams(int channel, int magnitude);
    bool SameSlipperyRoadEffectParams(int channel, int magnitude);
    bool SameSurfaceEffectParams(int channel, int type, int magnitude, int period);
};

#endif
