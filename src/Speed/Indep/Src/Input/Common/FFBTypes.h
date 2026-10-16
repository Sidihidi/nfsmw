#ifndef INPUT_COMMON_FFBTYPES_H
#define INPUT_COMMON_FFBTYPES_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

namespace RealInput {
class Effect;
}

#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/Indep/Libs/Support/Utility/UMath.h"

// PS2 (InputDevicePS2.cpp): los dos motores del mando se mezclan a partir de rampas y ondas por canal. Tipos y
// miembros de los stabs de la A124; los cuerpos, de su asm (FFBTypes.h:38-76).

// total size: 0x10
struct EffectRamp {
    float Time;         // offset 0x0, size 0x4
    float MaxTime;      // offset 0x4, size 0x4
    float Amplitude;    // offset 0x8, size 0x4
    float MinAmplitude; // offset 0xC, size 0x4

    void Clear() {
        Time = MaxTime = Amplitude = MinAmplitude = 0.0f;
    }

    float Run(float milliseconds) {
        if (Time > 0.0f && MaxTime > 0.0f) {
            Time -= milliseconds * 0.001f;
            return VU0_floatmax(0.0f, MinAmplitude + (Amplitude - MinAmplitude) * Time / MaxTime);
        }
        return 0.0f;
    }
};

// total size: 0x10
struct EffectWave {
    float Time;      // offset 0x0, size 0x4
    float Amplitude; // offset 0x4, size 0x4
    float Frequency; // offset 0x8, size 0x4
    float Offset;    // offset 0xC, size 0x4

    void Clear() {
        Time = Amplitude = Frequency = Offset = 0.0f;
    }

    float Run(float milliseconds) {
        float result = Offset;
        if (Frequency * Amplitude > 0.0f) {
            Time += Frequency * milliseconds * 0.001f;
            if (Time > 1.0f) {
                Time -= 1.0f;
            }
            result += UMath::Sina(Time) * Amplitude;
        }
        return result;
    }
};

enum ChannelID {
    CHANNEL_LEFT_LOW = 0,
    CHANNEL_LEFT_HIGH = 1,
    CHANNEL_RIGHT_LOW = 2,
    CHANNEL_RIGHT_HIGH = 3,
    CHANNEL_MAX = 4,
    TWO_CHANNEL_LEFT = 0,
    TWO_CHANNEL_RIGHT = 1,
    TWO_CHANNEL_MAX = 2,
};

// total size: 0x1B0
struct InputEffectState {
    EffectWave RoadNoise[CHANNEL_MAX];                // offset 0x0, size 0x40
    EffectWave SkidNoise[CHANNEL_MAX];                // offset 0x40, size 0x40
    EffectWave SlipNoise[CHANNEL_MAX];                // offset 0x80, size 0x40
    EffectWave EngineNoise[CHANNEL_MAX];              // offset 0xC0, size 0x40
    EffectRamp CollisionNoise[TWO_CHANNEL_MAX];       // offset 0x100, size 0x20
    EffectRamp ShiftPotentialNoise[TWO_CHANNEL_MAX];  // offset 0x120, size 0x20
    EffectRamp NOSNoise[TWO_CHANNEL_MAX];             // offset 0x140, size 0x20
    EffectRamp EngineBlownNoise[TWO_CHANNEL_MAX];     // offset 0x160, size 0x20
    EffectRamp ShiftNoise[TWO_CHANNEL_MAX];           // offset 0x180, size 0x20
    float Volume_Left;                                // offset 0x1A0, size 0x4
    float Volume_Right;                               // offset 0x1A4, size 0x4
    float Mag_right;                                  // offset 0x1A8, size 0x4
    bool Enabled;                                     // offset 0x1AC, size 0x4

    void Run(float ms);
    void Push(RealInput::Effect *effect);
};
#else
struct EffectBinary {
    float Time; // offset 0x0, size 0x4
    float MaxTime; // offset 0x4, size 0x4

    void Clear() {
        this->MaxTime = 0.0f;
        this->Time = 0.0f;
    }

    float Run(float milliseconds) {
        if (this->Time > 0.0f && this->MaxTime > 0.0f) {
            this->Time -= milliseconds * 0.001f;
            return 1.0f;
        }
        return 0.0f;
    }
};

struct InputEffectState {
    EffectBinary CollisionNoise; // offset 0x0, size 0x8
    float On;                    // offset 0x8, size 0x4
    float Mag_right;             // offset 0xC, size 0x4
    bool Enabled;                // offset 0x10, size 0x1

    void Run(float ms);
    void Push(struct RealInput::Effect *effect);
};
#endif

#endif
