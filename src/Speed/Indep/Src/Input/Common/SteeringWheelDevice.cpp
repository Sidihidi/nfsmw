#include "Speed/Indep/Src/Input/SteeringWheelDevice.h"
#ifdef EA_PLATFORM_PLAYSTATION2
#include "Speed/PSX2/Src/Logitech/LGWheels.h"
#else
#include "Speed/GameCube/Src/Logitech/LGWheels.h"
#endif
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISimable.h"
#include "Speed/Indep/Src/Interfaces/Simables/IVehicle.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISuspension.h"
#include "Speed/Indep/Src/Interfaces/Simables/IINput.h"
#include "Speed/Indep/Src/Interfaces/SimActivities/IGameState.h"
#include "Speed/Indep/Src/Sim/Simulation.h"
#include "Speed/Indep/Src/Sim/SimSurface.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#if defined(EA_PLATFORM_PLAYSTATION2) && !defined(EA_BUILD_A124)
#include "Speed/Indep/Src/Frontend/FEJoyInput.hpp"
#include "Speed/Indep/Src/Frontend/MenuScreens/ControllerUnplugged.hpp"
#endif

LGWheels *SteeringWheelDevice::lgwheels = 0;
#ifdef EA_PLATFORM_PLAYSTATION2
float SteeringWheelDevice::sPedalScale = 1.0f / 65535.0f;
#endif

void SteeringWheelDevice::UpdateForces(IPlayer *player) {

    static float previousVelocity[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    static float timeAtCollision[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    float speedParam;
    float velocity;
    float forceLeftWheelAmplitude;
    float forceRightWheelAmplitude;
    float frontWheelVectorForceAmplitude;
    float constantForceAmplitude;
    int currentSurfaceType;
    int currentSurfaceMaxMagnitude;
    int currentSurfacePeriod;
    float speedDiffBetwFrames;
    float previousSpeedDiffBetwFrames = 0.0f;
    float normalizedCollisionParameter;
    float normalizedBottomingParameter;
    float timeBetwCollisions;

    if (!lgwheels->IsConnected(this->mDeviceIndex)) {
        return;
    }

    if (!this->IsConnected()) {
        return;
    }

#ifdef EA_PLATFORM_PLAYSTATION2
    if (lgwheels->IsMultiturnCapable(this->mDeviceIndex)) {
        if (!lgwheels->IsMultiturnEnabled(this->mDeviceIndex)) {
            lgwheels->SetMultiturn(this->mDeviceIndex, 1);
        }
    }
#endif

    {
        PlayerSettings *settings = player->GetSettings();
#ifndef EA_BUILD_A124
        Sim::IStateManager *state_manager = UTL::COM::QueryInterface<Sim::IStateManager>(IGameState::Get());
#endif

        {
            ISimable *simable;
            IVehicle *vehicle;
            ISuspension *suspension;
            IInput *input;
            IPlayer *localPlayer;

            if (!settings->Rumble ||
#ifdef EA_BUILD_A124
                // Con `!=` y el `player == NULL` de al lado, EE-GCC junta las dos comparaciones en un OR sin
                // cortocircuito (fold_truthop) y el original salta.
                !TheGameFlowManager.IsInGame() || !(Sim::GetState() == Sim::STATE_ACTIVE) ||
#else
                !state_manager || state_manager->ShouldPauseInput() ||
#endif
                player == NULL ||
                (simable = player->GetSimable()) == NULL ||
                (vehicle = UTL::COM::QueryInterface<IVehicle>(simable)) == NULL ||
                (suspension = UTL::COM::QueryInterface<ISuspension>(vehicle)) == NULL) {

                StopAllForces();
                return;
            }

#ifndef EA_PLATFORM_PLAYSTATION2
            this->mManualTransmission = false;

            if (vehicle->QueryInterface(&input)) {
                this->mManualTransmission = !input->IsAutomaticShift();
            }
#endif

            velocity = vehicle->GetSpeed();

            // Aqui no es bAbs: en GC y en PS2 el abs lleva la linea de esta sentencia, no la de bMath.hpp.
            speedParam = bMin(1.0f, fabsf(velocity * 0.05f));

            if (GRaceStatus::IsDragRace()) {

                lgwheels->PlaySpringForce(this->mDeviceIndex, 0, this->ScaledForceParam(220.0f),
                                          this->ScaledForceParam(220.0f));
            } else {

                lgwheels->PlaySpringForce(this->mDeviceIndex, 0, this->ScaledForceParam(100.0f) * speedParam,
                                          this->ScaledForceParam(100.0f));
            }

            forceLeftWheelAmplitude = suspension->GetWheelSkid(0);
            forceRightWheelAmplitude = suspension->GetWheelSkid(1);
            constantForceAmplitude = forceLeftWheelAmplitude + forceRightWheelAmplitude;

            if (bAbs(velocity) > 1.0f) {

                frontWheelVectorForceAmplitude = UMath::Clamp(constantForceAmplitude * 100.0f, -255.0f, 255.0f);
            } else {

                frontWheelVectorForceAmplitude = 0.0f;
            }

            localPlayer = IPlayer::First(PLAYER_ALL);

            if (localPlayer != NULL && localPlayer->InGameBreaker()) {

                lgwheels->PlayConstantForce(this->mDeviceIndex, this->ScaledForceParam(frontWheelVectorForceAmplitude * 0.5f), 0x10e);

            } else if (GRaceStatus::IsDragRace()) {

                lgwheels->StopConstantForce(this->mDeviceIndex);

            } else {

                lgwheels->PlayConstantForce(this->mDeviceIndex, this->ScaledForceParam(frontWheelVectorForceAmplitude), 0x10e);
            }

#ifdef EA_PLATFORM_PLAYSTATION2
            if (!lgwheels->IsMultiturnEnabled(this->mDeviceIndex))
#endif
            {
                lgwheels->PlayDamperForce(this->mDeviceIndex, this->ScaledForceParam(100.0f) * (1.0f - speedParam));
            }

            if (!suspension->IsWheelOnGround(0) && !suspension->IsWheelOnGround(1)) {

                if (!lgwheels->IsPlaying(this->mDeviceIndex, 9)) {

                    lgwheels->PlayCarAirborne(this->mDeviceIndex);
                }
            } else {

                if (lgwheels->IsPlaying(this->mDeviceIndex, 9)) {

                    lgwheels->StopCarAirborne(this->mDeviceIndex);
                }
            }

            speedDiffBetwFrames = previousVelocity[this->mDeviceIndex] - velocity;

            if (bAbs(speedDiffBetwFrames) > 4.0f) {

                if (previousSpeedDiffBetwFrames > 3.0f) {
                    speedDiffBetwFrames += previousSpeedDiffBetwFrames;
                    timeAtCollision[this->mDeviceIndex] = 0.0f;
                }

                normalizedCollisionParameter = bMin(1.0f, (speedDiffBetwFrames - 4.0f) * 0.06666667f);
                timeBetwCollisions = WorldTimer.GetSeconds() - timeAtCollision[this->mDeviceIndex];

                if (timeBetwCollisions < 0.0f) {

                    timeAtCollision[this->mDeviceIndex] = 0.0f;

                } else if (timeBetwCollisions >= 0.15f) {

                    timeAtCollision[this->mDeviceIndex] = WorldTimer.GetSeconds();
                    lgwheels->PlayFrontalCollisionForce(this->mDeviceIndex,
                                                        this->ScaledForceParam(255.0f) * normalizedCollisionParameter);
                }
            }

            const SimSurface &surfaceLeft = suspension->IsWheelOnGround(0) ? suspension->GetWheelRoadSurface(0) : SimSurface::kNull;
            const SimSurface &surfaceRight = suspension->IsWheelOnGround(1) ? suspension->GetWheelRoadSurface(1) : SimSurface::kNull;

            int surfaceForceType = surfaceLeft.WheelSurfaceEffect();
            int surfaceForceMaxMag = (surfaceLeft.WheelEffectIntensity() + surfaceRight.WheelEffectIntensity()) >> 1;
            int surfaceForcePeriod = (surfaceLeft.WheelEffectFrequency() + surfaceRight.WheelEffectFrequency()) >> 1;

            if (surfaceForceType == 2) {
                surfaceForceType = 4;
            } else if (surfaceForceType == 1) {
                surfaceForceType = 3;
            } else {
                surfaceForceType = 2;
            }

            lgwheels->PlaySurfaceEffect(this->mDeviceIndex, surfaceForceType,
                                        this->ScaledForceParam(surfaceForceMaxMag) * speedParam, surfaceForcePeriod);

            previousVelocity[this->mDeviceIndex] = velocity;
        }
    }
}

void SteeringWheelDevice::StopAllForces() {
    lgwheels->StopConstantForce(this->mDeviceIndex);
    lgwheels->StopSurfaceEffect(this->mDeviceIndex);
    lgwheels->StopDamperForce(this->mDeviceIndex);
    lgwheels->StopCarAirborne(this->mDeviceIndex);
    lgwheels->StopSlipperyRoadEffect(this->mDeviceIndex);
    lgwheels->PlaySpringForce(this->mDeviceIndex, 0, this->ScaledForceParam(220.0f), this->ScaledForceParam(220.0f));
}

#ifdef EA_PLATFORM_PLAYSTATION2
void SteeringWheelDevice::ReadInput(float *inputBuffer) {
#ifndef EA_BUILD_A124
    // SLUS activa el volante con el mando requerido o cargando, salvo con la pantalla de mando desenchufado.
    bool activate = false;
    if (!bControllerUnpluggedScreenUp &&
        ((this->mDeviceIndex == 0 && cFEngJoyInput::mInstance->IsRequiredJoy(JOYSTICK_PORT1)) ||
         TheGameFlowManager.GetState() == GAMEFLOW_STATE_LOADING_REGION ||
         TheGameFlowManager.GetState() == GAMEFLOW_STATE_LOADING_TRACK)) {
        activate = true;
    }
#endif
    if (lgwheels->IsConnected(this->mDeviceIndex)) {
#ifdef EA_BUILD_A124
        if (TheGameFlowManager.IsInGame() && !FEManager::IsPaused()) {
#else
        if (lgwheels->IsMultiturnCapable(this->mDeviceIndex) && !lgwheels->IsMultiturnEnabled(this->mDeviceIndex)) {
            lgwheels->SetMultiturn(this->mDeviceIndex, 1);
        }
        if (activate) {
#endif
            this->isActivated = true;
        } else {
            this->isActivated = false;
        }

        float twist = this->ConvertWheelRotation(this->mDeviceIndex);
        inputBuffer[0] = UMath::Clamp(twist, 0.0f, 1.0f);
        inputBuffer[1] = -UMath::Clamp(twist, -1.0f, 0.0f);

        float gas = this->ConvertAcceleratorPosition(this->mDeviceIndex);
        inputBuffer[2] = this->ConvertBrakePosition(this->mDeviceIndex);
        inputBuffer[3] = gas;

        inputBuffer[5] = this->GetShiftUpValue(this->mDeviceIndex);
        inputBuffer[4] = this->GetShiftDownValue(this->mDeviceIndex);
        inputBuffer[6] = this->GetNOSValue(this->mDeviceIndex);
        inputBuffer[7] = this->GetHandbrakeValue(this->mDeviceIndex);
        inputBuffer[8] = this->GetGameBrakerValue(this->mDeviceIndex);
        inputBuffer[9] = this->GetLookBackvalue(this->mDeviceIndex);

        inputBuffer[10] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x80000) ? 1.0f : 0.0f;
        inputBuffer[11] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x20000) ? 1.0f : 0.0f;
        inputBuffer[12] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x40000) ? 1.0f : 0.0f;
        inputBuffer[13] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x10000) ? 1.0f : 0.0f;
        inputBuffer[14] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x2000000) ? 1.0f : 0.0f;
        inputBuffer[15] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x1000000) ? 1.0f : 0.0f;
        inputBuffer[16] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x200000) ? 1.0f : 0.0f;
        inputBuffer[17] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x400000) ? 1.0f : 0.0f;
        inputBuffer[18] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x800000) ? 1.0f : 0.0f;
        inputBuffer[19] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x80000000) ? 1.0f : 0.0f;
        inputBuffer[20] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x40000000) ? 1.0f : 0.0f;
        inputBuffer[21] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x80000000) ? 1.0f : 0.0f;
        inputBuffer[22] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x40000000) ? 1.0f : 0.0f;
        inputBuffer[23] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x4000000) ? 1.0f : 0.0f;
        inputBuffer[24] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x8000000) ? 1.0f : 0.0f;
        inputBuffer[25] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x20000000) ? 1.0f : 0.0f;
        inputBuffer[26] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x10000000) ? 1.0f : 0.0f;
        inputBuffer[27] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x100000) ? 1.0f : 0.0f;
    } else {
#ifndef EA_BUILD_A124
        if (!activate) {
            this->isActivated = false;
        }
#endif
        for (int i = 0; i <= 27; i++) {
            inputBuffer[i] = 0.0f;
        }
    }
}
#else
void SteeringWheelDevice::ReadInput(float *inputBuffer) {
    if (lgwheels->IsConnected(this->mDeviceIndex)) {
        {
            this->isActivated = true;

            inputBuffer[0] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x4) ? 1.0f : 0.0f;
            inputBuffer[1] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x8) ? 1.0f : 0.0f;
            inputBuffer[2] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x1) ? 1.0f : 0.0f;
            inputBuffer[3] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x2) ? 1.0f : 0.0f;

            inputBuffer[8] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x100) ? 1.0f : 0.0f;
            inputBuffer[9] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x200) ? 1.0f : 0.0f;
            inputBuffer[6] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x400) ? 1.0f : 0.0f;
            inputBuffer[7] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x800) ? 1.0f : 0.0f;
            inputBuffer[10] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x10) ? 1.0f : 0.0f;
            inputBuffer[13] = lgwheels->ButtonIsPressed(this->mDeviceIndex, 0x1000) ? 1.0f : 0.0f;

            inputBuffer[4] = (unsigned int)lgwheels->Position[this->mDeviceIndex].triggerLeft * 0.003921569f;
            inputBuffer[5] = (unsigned int)lgwheels->Position[this->mDeviceIndex].triggerRight * 0.003921569f;

            if (lgwheels->PedalsConnected(this->mDeviceIndex)) {

                inputBuffer[15] = (unsigned int)lgwheels->Position[this->mDeviceIndex].brake * 0.003921569f;
                inputBuffer[14] = (unsigned int)lgwheels->Position[this->mDeviceIndex].accelerator * 0.003921569f;

            } else {

                inputBuffer[15] = (unsigned int)lgwheels->Position[this->mDeviceIndex].triggerLeft * 0.003921569f;
                inputBuffer[14] = (unsigned int)lgwheels->Position[this->mDeviceIndex].triggerRight * 0.003921569f;
            }

            float wheel = this->ConvertWheelRotation(this->mDeviceIndex);

            inputBuffer[12] = UMath::Clamp(wheel, 0.0f, 1.0f);
            inputBuffer[11] = -UMath::Clamp(wheel, -1.0f, 0.0f);
        }
    } else {
        {
            for (int i = 0; i <= 15; i++) {
                inputBuffer[i] = 0.0f;
            }
        }
    }
}
#endif

void SteeringWheelDevice::InitWheelSupport() {
#ifdef EA_PLATFORM_PLAYSTATION2
    if (lgwheels == 0) {
        lgwheels = new ("LGWheel", 0) LGWheels;
        lgwheels->ReadAll();
        for (int i = 0; i < 2; i++) {
            if (lgwheels->IsConnected(i)) {
                lgwheels->PlaySpringForce(i, 0, 180, 180);
                if (lgwheels->IsMultiturnCapable(i)) {
                    if (!lgwheels->IsMultiturnEnabled(i)) {
                        lgwheels->SetMultiturn(i, 1);
                    }
                }
            }
        }
    }
#else
    if (lgwheels == 0) {
        int i = 0;
        LGWheels *wheels = new ("LGWheel", 0) LGWheels;
        lgwheels = wheels;
        wheels->ReadAll();
        for (; i <= 3; i++) {
            if (lgwheels->IsConnected(i)) {
                lgwheels->PlaySpringForce(i, 0, 180, 180);
            }
        }
    }
#endif
}

void SteeringWheelDevice::PollWheels() {
    if (lgwheels != 0) {
        lgwheels->ReadAll();
    }
}

float SteeringWheelDevice::ConvertWheelRotation(int channel) {
#ifdef EA_PLATFORM_PLAYSTATION2
    float val;
    if (lgwheels->IsMultiturnEnabled(channel)) {
        float originalVal = lgwheels->Position[channel].X1;
        if (GRaceStatus::IsDragRace()) {
            originalVal *= 5.0f;
        }
        val = UMath::Clamp(originalVal / 32000.0f, -1.0f, 1.0f);
        // La misma cuenta en las dos ramas: el compilador las funde y el return queda en la union (mov.s).
        val = val * 0.8f + val * val * val * 0.2f;
    } else {
        float originalVal = lgwheels->Position[channel].X1;
        val = UMath::Clamp(originalVal / 32000.0f, -1.0f, 1.0f);
        val = val * 0.8f + val * val * val * 0.2f;
    }
    return val;
#else
    float val = lgwheels->Position[channel].wheel;
    float originalVal = UMath::Clamp(val * 0.007874016f, -1.0f, 1.0f);
    val = originalVal * 0.8f + originalVal * originalVal * originalVal * 0.2f;
    return val;
#endif
}

#ifdef EA_PLATFORM_PLAYSTATION2
float SteeringWheelDevice::ConvertAcceleratorPosition(int channel) {
    float level = lgwheels->Position[channel].Accelerator;
    level = UMath::Clamp(level * sPedalScale, 0.0f, 1.0f);
    return level;
}

float SteeringWheelDevice::ConvertBrakePosition(int channel) {
    float level = lgwheels->Position[channel].Brake;
    level = UMath::Clamp(level * sPedalScale, 0.0f, 1.0f);
    return level;
}

float SteeringWheelDevice::GetShiftUpValue(int channel) {
    if (lgwheels->ButtonIsPressed(channel, 0x2)) {
        return 1.0f;
    }
    return 0.0f;
}

float SteeringWheelDevice::GetShiftDownValue(int channel) {
    if (lgwheels->ButtonIsPressed(channel, 0x1)) {
        return 1.0f;
    }
    return 0.0f;
}

float SteeringWheelDevice::GetNOSValue(int channel) {
    if (lgwheels->IsWheelClassA(channel)) {
        if (lgwheels->ButtonIsPressed(channel, 0x20)) {
            return 1.0f;
        }
    }
    return 0.0f;
}

float SteeringWheelDevice::GetHandbrakeValue(int channel) {
    if (lgwheels->IsWheelClassA(channel)) {
        if (lgwheels->ButtonIsPressed(channel, 0x10)) {
            return 1.0f;
        }
    }
    return 0.0f;
}

float SteeringWheelDevice::GetGameBrakerValue(int channel) {
    if (lgwheels->IsWheelClassA(channel)) {
        if (lgwheels->ButtonIsPressed(channel, 0x8)) {
            return 1.0f;
        }
    }
    return 0.0f;
}

float SteeringWheelDevice::GetLookBackvalue(int channel) {
    if (lgwheels->IsWheelClassA(channel)) {
        if (lgwheels->ButtonIsPressed(channel, 0x4)) {
            return 1.0f;
        }
    }
    return 0.0f;
}
#endif

float SteeringWheelDevice::ScaledForceParam(float forceParam) {
#ifdef EA_PLATFORM_PLAYSTATION2
    if (lgwheels->IsMultiturnCapable(this->mDeviceIndex)) {
        return forceParam * 0.75f;
    }
#endif
    return forceParam * 0.85f;
}

bool SteeringWheelDevice::IsConnected() {
    if (lgwheels == 0) {
        return false;
    }
    return lgwheels->IsConnected(this->mDeviceIndex);
}

ISteeringWheel::SteeringType SteeringWheelDevice::GetSteeringType() {
    if (this->IsConnected()) {
#ifdef EA_PLATFORM_PLAYSTATION2
        if (lgwheels->IsMultiturnEnabled(this->mDeviceIndex)) {
            return ISteeringWheel::kWheelSpeedInsensitive;
        }
#endif
        return ISteeringWheel::kWheelSpeedSensitive;
    }
    return ISteeringWheel::kGamePad;
}

bool SteeringWheelDevice::WheelConnected(int port) {
    return lgwheels->IsConnected(port);
}
