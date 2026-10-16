#ifndef PSX2_LOGITECH_LOGIFORCE_H
#define PSX2_LOGITECH_LOGIFORCE_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// Las cuatro derivadas no anaden miembros (los stabs les dan 0x80, lo mismo que la base), asi que viven aqui junto
// a LogiForce, como Force.h en GameCube.

// Ranuras de cada objeto de fuerza: el segundo indice de Playing y EffectID.
enum ConditionForceNumber {
    LG_CONDITION_0 = 0,
    LG_CONDITION_1 = 1,
    LG_CONDITION_2 = 2,
    LG_CONDITION_3 = 3,
    LG_CONDITION_4 = 4,
    LG_CONDITION_5 = 5,
    LG_CONDITION_6 = 6,
    LG_CONDITION_7 = 7,
};

enum PeriodicForceNumber {
    LG_PERIODIC_0 = 0,
    LG_PERIODIC_1 = 1,
    LG_PERIODIC_2 = 2,
    LG_PERIODIC_3 = 3,
    LG_PERIODIC_4 = 4,
    LG_PERIODIC_5 = 5,
    LG_PERIODIC_6 = 6,
    LG_PERIODIC_7 = 7,
};

enum ConstantForceNumber {
    LG_CONSTANT_0 = 0,
    LG_CONSTANT_1 = 1,
    LG_CONSTANT_2 = 2,
    LG_CONSTANT_3 = 3,
    LG_CONSTANT_4 = 4,
    LG_CONSTANT_5 = 5,
    LG_CONSTANT_6 = 6,
    LG_CONSTANT_7 = 7,
};

enum RampForceNumber {
    LG_RAMP_0 = 0,
    LG_RAMP_1 = 1,
    LG_RAMP_2 = 2,
    LG_RAMP_3 = 3,
    LG_RAMP_4 = 4,
    LG_RAMP_5 = 5,
    LG_RAMP_6 = 6,
    LG_RAMP_7 = 7,
};

// total size: 0x80
struct LogiForce {
    bool Playing[2][8];  // offset 0x0, size 0x40
    int EffectID[2][8];  // offset 0x40, size 0x40

    LogiForce();
    int Start(int channel, int forceNumber);
    int Stop(int channel, int forceNumber);
    int Destroy(int channel, int forceNumber);
    void SetOverallForceGain(int &handle, int value);
    int GetOverallForceGain(int &handle);

  private:
    void InitVars();
};

// total size: 0x80
struct LogiCondition : public LogiForce {
    LogiCondition();
    int DownloadForce(int channel, int forceNumber, int &handle, int type, int duration, int startDelay, int offset, int deadband,
                      int satNeg, int satPos, int coeffNeg, int coeffPos);
    int UpdateForce(int channel, int forceNumber, int type, int duration, int startDelay, int offset, int deadband, int satNeg,
                    int satPos, int coeffNeg, int coeffPos);
};

// total size: 0x80
struct LogiConstant : public LogiForce {
    LogiConstant();
    void Initialize();
    int DownloadForce(int channel, int forceNumber, int &handle, int duration, int startDelay, int magnitude, int direction,
                      int attackTime, int fadeTime, int attackLevel, int fadeLevel);
    int UpdateForce(int channel, int forceNumber, int duration, int startDelay, int magnitude, int direction, int attackTime,
                    int fadeTime, int attackLevel, int fadeLevel);
};

// total size: 0x80
struct LogiPeriodic : public LogiForce {
    LogiPeriodic();
    int DownloadForce(int channel, int forceNumber, int &handle, int type, int duration, int startDelay, int magnitude, int direction,
                      int period, int phase, int offset, int attackTime, int fadeTime, int attackLevel, int fadeLevel);
    int UpdateForce(int channel, int forceNumber, int type, int duration, int startDelay, int magnitude, int direction, int period,
                    int phase, int offset, int attackTime, int fadeTime, int attackLevel, int fadeLevel);
};

// total size: 0x80
struct LogiRamp : public LogiForce {
    LogiRamp();
    int DownloadForce(int channel, int forceNumber, int &handle, int duration, int startDelay, int magnitudeStart, int magnitudeEnd,
                      int direction);
    int UpdateForce(int channel, int forceNumber, int duration, int startDelay, int magnitudeStart, int magnitudeEnd, int direction);
};

#endif
