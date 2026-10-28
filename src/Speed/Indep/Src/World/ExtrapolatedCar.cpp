#include "OnlineManager.hpp"

#if ONLINE_SUPPORT && (defined(EA_BUILD_A124) || defined(EA_PLATFORM_WIN32))
// El coche extrapolado de la carrera online: solo existe en el prototipo (A124). Las funciones salen del asm de la
// A124 con las locales de sus stabs; GameCube y SLUS no tienen nada de este fichero. PC (speed.exe) lo tiene con
// otro layout (OnlineManager.hpp) y lo que cambia va bajo EA_PLATFORM_WIN32.
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/Libs/Support/Utility/UVectorMath.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Interfaces/Simables/IAI.h"
#include "Speed/Indep/Src/Interfaces/Simables/ICollisionBody.h"
#include "Speed/Indep/Src/Interfaces/Simables/IINput.h"
#include "Speed/Indep/Src/Interfaces/Simables/IRigidBody.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISuspension.h"
#include "Speed/Indep/Src/Interfaces/Simables/ITransmission.h"
#include "Speed/Indep/Src/Interfaces/Simables/IVehicle.h"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Online/InGame/Client.hpp"
#include "Speed/Indep/Src/Online/SmartBitstream.hpp"
#include "Speed/Indep/Src/Online/VoiceCore.hpp"
#include "Speed/Indep/Src/Physics/Dynamics/Inertia.h"
#include "Speed/Indep/Src/Physics/PVehicle.h"
#include "Speed/Indep/Src/Sim/Simulation.h"
#include "Speed/Indep/Src/World/WCollisionMgr.h"
#include "Speed/Indep/bWare/Inc/Strings.hpp"

#ifdef EA_PLATFORM_WIN32
// PC: State::Blend llama a una funcion de UMath que la cabecera comun solo declara en PS2 (speed.exe 0x5D2B90,
// en zFoundation, que interpola y normaliza el cuaternion; sus argumentos van en otro orden: a, b, r, t).
// Declarada aqui y no en UMath.h: alli voltea un registro de VehicleFragmentConn::UpdateModel (100 -> 99,947 %).
namespace UMath {
void Slerp(const Vector4 &a, const Vector4 &b, float t, Vector4 &r);
}
#endif

float kMaxLatency = 0.6f;
float kBlendMult = 3.0f;
float kMinBlendTime = 0.1f;
float kMaxBlendTime = 2.0f;
bool kUseDriverAI = false;
bool kSpamPhysics = false;

static HSIMABLE__ *ExtrapolatedCar_kHandles[16];
static const unsigned int ExtrapolatedCar_CopTypes[8] = {0x82d1a1a3, 0x565398cc, 0xb25e4606, 0xf61dece9,
                                                         0xe22ff3b2, 0xac9e142e, 0x8866eba4, 0x38299bbb};

static uint32 GetMasterTime() {
    return TheOnlineManager.GetMasterTime();
}

ExtrapolatedCar::State::State() {
    mGear = 1;
    mBlendRate = -1.0f;
    mPosition.x = 0.0f;
    mPosition.y = 0.0f;
    mPosition.z = 0.0f;
    mSteering = 0.0f;
    mLinearVelocity.x = 0.0f;
    mLinearVelocity.y = 0.0f;
    mLinearVelocity.z = 0.0f;
    mGas = 0.0f;
    mLinearAcceleration.x = 0.0f;
    mLinearAcceleration.y = 0.0f;
    mLinearAcceleration.z = 0.0f;
    mBrake = 0.0f;
    mRotation.x = 0.0f;
    mRotation.y = 0.0f;
    mRotation.z = 0.0f;
    mRotation.w = 1.0f;
    mAngularVelocity.x = 0.0f;
    mAngularVelocity.y = 0.0f;
    mAngularVelocity.z = 0.0f;
    mAngularVelocity.w = 1.0f;
    mAngularAcceleration.x = 0.0f;
    mAngularAcceleration.y = 0.0f;
    mAngularAcceleration.z = 0.0f;
    mAngularAcceleration.w = 1.0f;
    mBlend = 1.0f;
    mHandBrake = 0.0f;
    mTime = 0.0f;
    mInFlight = false;
    mNOS = false;
}

void ExtrapolatedCar::State::Import(const ISimable *simable, float simtime) {
    const IRigidBody *irb;
    const IInput *input;
    const ITransmission *transmission;
    const ISuspension *suspension;
    const ICollisionBody *collision_body;
    GearID a;
    GearID amax;

    if (simable->QueryInterface(&input)) {
        const InputControls &controls = input->GetControls();
        mSteering = controls.fSteering;
        mNOS = controls.fNOS;
        mGas = controls.fGas;
        mBrake = controls.fBrake;
        mHandBrake = controls.fHandBrake;
    }

    if (!simable->QueryInterface(&transmission)) {
        mInFlight = false;
    } else {
        a = transmission->GetGear();
        amax = G_SIXTH;
        mGear = static_cast<uint8>(UMath::Clamp(a, G_REVERSE, amax));
        mInFlight = false;
    }

    if (simable->QueryInterface(&suspension)) {
        if (suspension->GetNumWheels() == 0 ||
            suspension->GetNumWheelsOnGround() < static_cast<int>(suspension->GetNumWheels())) {
            mInFlight = true;
        }
    }

    irb = simable->GetRigidBody();
    mPosition = irb->GetPosition();
    mLinearVelocity = irb->GetLinearVelocity();
    UMath::Copy(irb->GetOrientation(), mRotation);
    UMath::Vector4To3(mAngularVelocity) = irb->GetAngularVelocity();

    if (simable->QueryInterface(&collision_body)) {
        if (UMath::LengthSquare(mLinearVelocity) > 1.0f) {
            UMath::Scale(collision_body->GetForce(), irb->GetOOMass(), mLinearAcceleration);
        } else {
            UMath::Clear(mLinearAcceleration);
        }

        Dynamics::Inertia::Tensor inertiaTensor = collision_body->GetInertiaTensor();
        UMath::Matrix4 inverseWorldTensor;

        inertiaTensor.GetInverseWorldTensor(collision_body->GetMatrix4(), inverseWorldTensor);
        UMath::Vector3 angularAcceleration;
        VU0_MATRIX3x4_vect3mult(collision_body->GetTorque(), inverseWorldTensor, angularAcceleration);
        UMath::Vector4To3(mAngularAcceleration) = angularAcceleration;
    }

    mTime = simtime;
    mBlend = 1.0f;
    mBlendRate = 1.0f / kMinBlendTime;
}

void ExtrapolatedCar::State::Export(SmartBitStream &data, ePosDataPriorityMask priority_mask, uint8 repositioncount) {
    UMath::Matrix4 matrix;

    mPriority = static_cast<uint8>(priority_mask);
    data.AddQuantizedFloat(mTime, TheOnlineManager.QuantFloatSimTime);
    data.AddQuantizedInt(repositioncount, TheOnlineManager.QuantInt2Bit);
    data.AddBool(mInFlight);
    if ((priority_mask & PDP_MASK_CRITICAL) != PDP_MASK_NONE) {
        if (!mInFlight || (priority_mask & PDP_MASK_INFLIGHT) != PDP_MASK_NONE) {
            data.AddQuantizedFloat(mAngularAcceleration.y, TheOnlineManager.AccelerationQuantizer);
        }
        float minvalue = TheOnlineManager.AccelerationQuantizer.GetMinValue();
        float maxvalue = TheOnlineManager.AccelerationQuantizer.GetMaxValue();
        data.AddQuantizedFloat(UMath::Clamp(mLinearAcceleration.x, minvalue, maxvalue), TheOnlineManager.AccelerationQuantizer);
        data.AddQuantizedFloat(UMath::Clamp(mLinearAcceleration.y, minvalue, maxvalue), TheOnlineManager.AccelerationQuantizer);
        data.AddQuantizedFloat(UMath::Clamp(mLinearAcceleration.z, minvalue, maxvalue), TheOnlineManager.AccelerationQuantizer);
    }
    if ((priority_mask & PDP_MASK_NORMAL) != PDP_MASK_NONE) {
        if (mInFlight) {
            data.AddQuantizedFloat(mAngularVelocity.x, TheOnlineManager.AVelocityQuantizer);
            data.AddQuantizedFloat(mAngularVelocity.z, TheOnlineManager.AVelocityQuantizer);
        }
        data.AddQuantizedFloat(mAngularVelocity.y, TheOnlineManager.AVelocityQuantizer);
        data.AddQuantizedFloat(mLinearVelocity.x, TheOnlineManager.VelocityQuantizer);
        data.AddQuantizedFloat(mLinearVelocity.y, TheOnlineManager.VelocityQuantizer);
        data.AddQuantizedFloat(mLinearVelocity.z, TheOnlineManager.VelocityQuantizer);
    }
    if ((priority_mask & PDP_MASK_LOW) != PDP_MASK_NONE) {
        if (mInFlight || (priority_mask & PDP_MASK_INFLIGHT) != PDP_MASK_NONE) {
            data.AddQuantizedFloat(mRotation.x, TheOnlineManager.MatrixQuantizer);
            data.AddQuantizedFloat(mRotation.y, TheOnlineManager.MatrixQuantizer);
            data.AddQuantizedFloat(mRotation.z, TheOnlineManager.MatrixQuantizer);
            data.AddQuantizedFloat(mRotation.w, TheOnlineManager.MatrixQuantizer);
        } else {
            UMath::QuaternionToMatrix4(mRotation, matrix);
            float heading = UMath::Atan2a(matrix.v2.z, -matrix.v2.x);
            data.AddQuantizedFloat(heading, TheOnlineManager.AngleQuantizer);
        }
        data.AddQuantizedFloat(mPosition.x, TheOnlineManager.PositionQuantizerX);
        data.AddQuantizedFloat(mPosition.y, TheOnlineManager.PositionQuantizerY);
        data.AddQuantizedFloat(mPosition.z, TheOnlineManager.PositionQuantizerZ);
    }
    if ((priority_mask & (PDP_MASK_NORMAL | PDP_MASK_CRITICAL)) != PDP_MASK_NONE) {
        data.AddQuantizedFloat(mSteering, TheOnlineManager.MatrixQuantizer);
        data.AddQuantizedFloat(mHandBrake, TheOnlineManager.ControlQuantizer);
        data.AddQuantizedFloat(mGas, TheOnlineManager.ControlQuantizer);
        data.AddQuantizedFloat(mBrake, TheOnlineManager.ControlQuantizer);
        data.AddQuantizedInt(mGear, TheOnlineManager.QuantInt3Bit);
        data.AddBool(mNOS);
    }
}

uint8 ExtrapolatedCar::State::Import(float time, SmartBitStream &data,
                                     ePosDataPriorityMask priority_mask) {
    UMath::Matrix4 matrix;
    float AccError;
    float VelError;
    int repositioncount;

    mPriority = static_cast<uint8>(priority_mask);
    mTime = time;
    repositioncount = data.GetQuantizedInt(TheOnlineManager.QuantInt2Bit);
    data.GetBool(mInFlight);
    if ((priority_mask & PDP_MASK_CRITICAL) != PDP_MASK_NONE) {
        UMath::Clear(mAngularAcceleration);
        AccError = TheOnlineManager.AccelerationQuantizer.GetMaxError() * 2.0f;
        if (!mInFlight || (priority_mask & PDP_MASK_INFLIGHT) != PDP_MASK_NONE) {
            mAngularAcceleration.y = data.GetQuantizedFloat(TheOnlineManager.AccelerationQuantizer);
            if (bAbs(mAngularAcceleration.y) < AccError) {
                mAngularAcceleration.y = 0.0f;
            }
        }
        UMath::Clear(mLinearAcceleration);
        mLinearAcceleration.x =
            data.GetQuantizedFloat(TheOnlineManager.AccelerationQuantizer);
        if (bAbs(mLinearAcceleration.x) < AccError) {
            mLinearAcceleration.x = 0.0f;
        }
        mLinearAcceleration.y =
            data.GetQuantizedFloat(TheOnlineManager.AccelerationQuantizer);
        if (bAbs(mLinearAcceleration.y) < AccError) {
            mLinearAcceleration.y = 0.0f;
        }
        mLinearAcceleration.z =
            data.GetQuantizedFloat(TheOnlineManager.AccelerationQuantizer);
        if (bAbs(mLinearAcceleration.z) < AccError) {
            mLinearAcceleration.z = 0.0f;
        }
    }

    if ((priority_mask & PDP_MASK_NORMAL) != PDP_MASK_NONE) {
        UMath::Clear(mAngularVelocity);

        if (mInFlight || (priority_mask & PDP_MASK_INFLIGHT) != PDP_MASK_NONE) {
            mAngularVelocity.x =
                data.GetQuantizedFloat(TheOnlineManager.AVelocityQuantizer);
            mAngularVelocity.z =
                data.GetQuantizedFloat(TheOnlineManager.AVelocityQuantizer);
        }

        mAngularVelocity.y = data.GetQuantizedFloat(TheOnlineManager.AVelocityQuantizer);
        if (bAbs(mAngularVelocity.y) < TheOnlineManager.AVelocityQuantizer.GetMaxError() * 2.0f) {
            mAngularVelocity.y = 0.0f;
        }
        VelError = TheOnlineManager.VelocityQuantizer.GetMaxError() * 2.0f;
        mLinearVelocity.x = data.GetQuantizedFloat(TheOnlineManager.VelocityQuantizer);
        if (bAbs(mLinearVelocity.x) < VelError) {
            mLinearVelocity.x = 0.0f;
        }
        mLinearVelocity.y = data.GetQuantizedFloat(TheOnlineManager.VelocityQuantizer);
        if (bAbs(mLinearVelocity.y) < VelError) {
            mLinearVelocity.y = 0.0f;
        }
        mLinearVelocity.z = data.GetQuantizedFloat(TheOnlineManager.VelocityQuantizer);
        if (bAbs(mLinearVelocity.z) < VelError) {
            mLinearVelocity.z = 0.0f;
        }
    }

    if ((priority_mask & PDP_MASK_LOW) != PDP_MASK_NONE) {
        if (mInFlight || (priority_mask & PDP_MASK_INFLIGHT) != PDP_MASK_NONE) {
            mRotation.x = data.GetQuantizedFloat(TheOnlineManager.MatrixQuantizer);
            mRotation.y = data.GetQuantizedFloat(TheOnlineManager.MatrixQuantizer);
            mRotation.z = data.GetQuantizedFloat(TheOnlineManager.MatrixQuantizer);
            mRotation.w = data.GetQuantizedFloat(TheOnlineManager.MatrixQuantizer);
        } else {
            float heading = data.GetQuantizedFloat(TheOnlineManager.AngleQuantizer) - 0.25f;

            UMath::SetYRot(matrix, heading - floorf(heading + 0.5f));
            UMath::Matrix4ToQuaternion(matrix, mRotation);
        }

        mPosition.x = data.GetQuantizedFloat(TheOnlineManager.PositionQuantizerX);
        mPosition.y = data.GetQuantizedFloat(TheOnlineManager.PositionQuantizerY);
        mPosition.z = data.GetQuantizedFloat(TheOnlineManager.PositionQuantizerZ);
    }

    if ((priority_mask & (PDP_MASK_NORMAL | PDP_MASK_CRITICAL)) != PDP_MASK_NONE) {
        mSteering = data.GetQuantizedFloat(TheOnlineManager.MatrixQuantizer);
        mHandBrake = data.GetQuantizedFloat(TheOnlineManager.ControlQuantizer);
        mGas = data.GetQuantizedFloat(TheOnlineManager.ControlQuantizer);
        mBrake = data.GetQuantizedFloat(TheOnlineManager.ControlQuantizer);
        mGear = static_cast<uint8>(data.GetQuantizedInt(TheOnlineManager.QuantInt3Bit));
        data.GetBool(mNOS);
    }

    mBlend = 0.0f;
    mBlendRate = -1.0f;
    return static_cast<uint8>(repositioncount);
}

void ExtrapolatedCar::State::Extrapolate(float simtime) {
    float amin;
    float amax;
    UMath::Vector4 temp;
    UMath::Matrix4 matrix;

    if (mBlendRate < 0.0f) {
        amin = kMinBlendTime;
        amax = kMaxBlendTime;
        mBlendRate = 1.0f / UMath::Clamp(kBlendMult * (simtime - mTime), amin, amax);
    }

    while (mTime != simtime) {
        float dt = UMath::Max(-0.016666668f, UMath::Min(simtime - mTime, 0.016666668f));

        UMath::ScaleAdd(mLinearAcceleration, dt, mLinearVelocity, mLinearVelocity);
        UMath::ScaleAdd(mLinearVelocity, dt, mPosition, mPosition);
        if (!mInFlight) {
            UMath::ScaleAdd(mAngularAcceleration, dt, mAngularVelocity, mAngularVelocity);
        }
        UMath::Mult(mAngularVelocity, mRotation, temp);
        UMath::ScaleAdd(temp, dt * 0.5f, mRotation, mRotation);
        UMath::Normalize(mRotation);

        UMath::SetYRot(matrix, mAngularVelocity.y * dt * UMath::OOTWOPI);
        UMath::Rotate(mLinearAcceleration, matrix, mLinearAcceleration);

        mTime += dt;
    }
}

bool ExtrapolatedCar::State::Blend(State &blended, float t) {
    if (mBlendRate < 0.0f) {
        mBlendRate = 1.0f / kMinBlendTime;
    }
    mBlend += t * mBlendRate;
    if (mBlend < 1.0f) {
        float s = 1.0f - mBlend * mBlend * (3.0f - (mBlend + mBlend));

        UMath::Sub(blended.mPosition, mPosition, blended.mPosition);
        UMath::ScaleAdd(blended.mPosition, s, mPosition, blended.mPosition);
        UMath::Sub(blended.mLinearVelocity, mLinearVelocity, blended.mLinearVelocity);
        UMath::ScaleAdd(blended.mLinearVelocity, s, mLinearVelocity, blended.mLinearVelocity);
        UMath::Slerp(mRotation, blended.mRotation, s, blended.mRotation);
        UMath::Sub(UMath::Vector4To3(blended.mAngularVelocity), UMath::Vector4To3(mAngularVelocity),
                   UMath::Vector4To3(blended.mAngularVelocity));
        UMath::ScaleAdd(UMath::Vector4To3(blended.mAngularVelocity), s, UMath::Vector4To3(mAngularVelocity),
                        UMath::Vector4To3(blended.mAngularVelocity));
        blended.mSteering = (blended.mSteering - mSteering) * s + mSteering;
        blended.mGas = (blended.mGas - mGas) * s + mGas;
        blended.mBrake = (blended.mBrake - mBrake) * s + mBrake;
        blended.mHandBrake = (blended.mHandBrake - mHandBrake) * s + mHandBrake;
        blended.mGear = mGear;
        blended.mNOS = mNOS;
        return false;
    }
    blended = *this;
    return true;
}

bool ExtrapolatedCar::State::IsBlending() const { return mBlend > 0.0f; }

void ExtrapolatedCar::State::Export(ISimable *simable) const {
    IRigidBody *irb;
    IVehicle *vehicle;
    ISuspension *suspension;
    IInput *input;
    ITransmission *transmission;
    bool stillinflight;
    UMath::Vector3 pos;
    UMath::Vector3 velocity;
    UMath::Matrix4 matrix;
    UMath::Vector3 angularVelocity;

    stillinflight = false;
    irb = simable->GetRigidBody();
    if (!simable->QueryInterface(&vehicle) ||
        vehicle->GetPhysicsMode() == PHYSICS_MODE_EMULATED ||
        !simable->QueryInterface(&suspension) ||
        suspension->GetNumWheelsOnGround() == 0) {
        stillinflight = true;
    }

    if (mInFlight && stillinflight) {
        irb->SetPosition(mPosition);
        irb->SetOrientation(mRotation);
        irb->SetLinearVelocity(mLinearVelocity);
        irb->SetAngularVelocity(UMath::Vector4To3(mAngularVelocity));
    } else {
        pos = mPosition;
        velocity = mLinearVelocity;

        if (vehicle) {
            float d;
            d = pos.y - irb->GetPosition().y;
            if ((d > -10.0f) && (d < 10.0f)) {
                pos.y = irb->GetPosition().y;
                velocity.y = irb->GetLinearVelocity().y;
            }
        }

        if (UMath::Abs(pos.x - irb->GetPosition().x) >
            TheOnlineManager.PositionQuantizerX.GetMaxError()) {
            if (UMath::Abs(pos.z - irb->GetPosition().z) >
                TheOnlineManager.PositionQuantizerZ.GetMaxError()) {
                irb->SetPosition(pos);
            }
        }
        irb->SetLinearVelocity(velocity);

        UMath::QuaternionToMatrix4(mRotation, matrix);
        float result = VU0_Atan2(matrix.v2.z, -matrix.v2.x);
        irb->GetMatrix4(matrix);
        UMath::MultYRot(matrix, result - VU0_Atan2(matrix.v2.z, -matrix.v2.x), matrix);
        irb->SetOrientation(matrix);

        angularVelocity.x = irb->GetAngularVelocity().x;
        angularVelocity.y = mAngularVelocity.y;
        angularVelocity.z = irb->GetAngularVelocity().z;
        irb->SetAngularVelocity(angularVelocity);
    }

    if (simable->QueryInterface(&input)) {
        input->SetControlSteering(mSteering);
        input->SetControlGas(mGas);
        input->SetControlBrake(mBrake);
        input->SetControlHandBrake(mHandBrake);
        input->SetControlNOS(mNOS);
    }

    if (simable->QueryInterface(&transmission)) {
        transmission->Shift(static_cast<GearID>(mGear));
    }

#ifdef EA_PLATFORM_PLAYSTATION2
    if (kSpamPhysics) {
        UMath::QuaternionToEuler(irb->GetOrientation(), pos);
    }
#endif
}

void ExtrapolatedCar::State::SetOnGround(IVehicle *vehicle) {
    UMath::Vector3 forwardvector;

    UMath::ExtractZAxis(mRotation, forwardvector);
    vehicle->SetVehicleOnGround(mPosition, forwardvector);
}

bool ExtrapolatedCar::State::IsValidPosition() {
    WCollisionMgr collision_mgr(0, 3);
    float elevation;

    return collision_mgr.GetWorldHeightAtPointRigorous(mPosition, elevation, nullptr);
}

float ExtrapolatedCar::State::SquaredDistanceTo(State &target) const {
    return VU0_v3distancesquare(mPosition, target.mPosition);
}

void ExtrapolatedCar::State::ExtractDirection(UMath::Vector3 &direction) const {
    UMath::ExtractZAxis(mRotation, direction);
}

ISimable *ExtrapolatedCar::State::SpawnVehicle(Attrib::Key cartype) {
    UMath::Vector3 forwardvector;

    if (cartype == 0) {
        cartype = Attrib::StringToKey(SkipFEPlayer2Car);
    }
    UMath::ExtractZAxis(mRotation, forwardvector);
    return ISimable::CreateInstance(
        "PVehicle", VehicleParams(nullptr, DRIVER_REMOTE, cartype, forwardvector, mPosition, VPF_SNAP_TO_GROUND, nullptr, nullptr));
}

ExtrapolatedCar::ExtrapolatedCar(Attrib::Key cartype) {
    mCarType = cartype;
    mCops = nullptr;
    mMutex = new NetworkMutex;
    mMutex->Create();
    mLast = mStateArray;
#ifdef EA_PLATFORM_WIN32
    mLastPDPAesthetic = 0.0f;
    mCurrentMode = PHYSICS_MODE_SIMULATED;
#endif
    mActive = true;
    mUpdateTime = 0;
    mTail = 0;
    mHead = 0;
    mCollisionTime = 0.0f;
    mRepositionCount = 0;
    mHasHeadset = false;
    mPaused = false;
    mUseDriverAI = false;
}

ExtrapolatedCar::~ExtrapolatedCar() {
    mMutex->Destroy();
    delete mMutex;
    if (mCops) {
        CopMap::iterator iter = mCops->begin();
        while (iter != mCops->end()) {
            if (iter->second) {
                delete iter->second;
            }
            ++iter;
        }
        mCops->clear();
        delete mCops;
    }
}

void ExtrapolatedCar::ExportSimable(ISimable *simable) {
    State *state;
    float simtime;

    if (!simable) {
        return;
    }

    state = mStateArray + mHead;
    simtime = Sim::GetTime();
    state->Import(simable, simtime);
    mHasHeadset = VoiceCore::mInstance->IsHeadsetConnected();

    if (!simable->IsPlayer()) {
        return;
    }

    if (mCops) {
        CopMap::iterator copiter;
        CopMap::iterator nextiter = mCops->begin();

        while (nextiter != mCops->end()) {
            copiter = nextiter++;
            HSIMABLE handle = copiter->first;

            if (!ISimable::FindInstance(handle)) {
                for (int i = 0; i < 15; i++) {
                    if (copiter->first == ExtrapolatedCar_kHandles[i]) {
                        if (kSpamPhysics) {
                            Attrib::Gen::pvehicle vehicle(copiter->second->GetCarType(), 0, nullptr);
                        }
                        ExtrapolatedCar_kHandles[i] = nullptr;
                    }
                }
                delete copiter->second;
                mCops->erase(copiter);
            }
        }
    }

    for (IVehicle::List::const_iterator vehicleiter = IVehicle::GetList(VEHICLE_AICOPS).begin();
         vehicleiter != IVehicle::GetList(VEHICLE_AICOPS).end(); ++vehicleiter) {
        IVehicle *vehicle = *vehicleiter;
        if (vehicle->IsActive() && !vehicle->IsAnimating() && !vehicle->IsDestroyed()) {
            if (!mCops) {
                mCops = new CopMap;
            }

            ISimable *copsimable = vehicle->GetSimable();
            HSIMABLE handle = copsimable->GetInstanceHandle();
            std::pair<CopMap::iterator, bool> result;
            result = mCops->insert(CopMap::value_type(
                handle, static_cast<ExtrapolatedCar *>(nullptr)));
            if (result.second) {
                for (int i = 0; i < 15; i++) {
                    if (!ExtrapolatedCar_kHandles[i]) {
                        ExtrapolatedCar_kHandles[i] = handle;
                        break;
                    }
                }
                result.first->second = new ExtrapolatedCar(vehicle->GetVehicleKey());
            }
            result.first->second->ExportSimable(copsimable);
        }
    }
}

void ExtrapolatedCar::ExportStream(SmartBitStream &data,
                                   ePosDataPriorityMask priority_mask) {
    mLast = mStateArray + mHead;
    mLast->Export(data, priority_mask, mRepositionCount);
    data.AddBool(mHasHeadset);
    mSaved = *mLast;
    mUpdateTime = GetMasterTime();

    if (mCops) {
        for (CopMap::iterator iter = mCops->begin(); iter != mCops->end(); ++iter) {
            int i;
            Attrib::Key type;

            for (i = 0; i < 15; i++) {
                if (iter->first == ExtrapolatedCar_kHandles[i]) {
                    break;
                }
            }
            if (i != 16) {
                data.AddQuantizedInt(i, TheOnlineManager.QuantInt4Bit);
                type = iter->second->GetCarType();
                for (i = 0; i < 7; i++) {
                    if (type == ExtrapolatedCar_CopTypes[i]) {
                        break;
                    }
                }
                data.AddQuantizedInt(i, TheOnlineManager.QuantInt3Bit);
                iter->second->ExportStream(data, priority_mask);
            }
        }
    }
}

void ExtrapolatedCar::ImportStream(SmartBitStream &data,
                                   ePosDataPriorityMask priority_mask) {
    uint8 repositioncount;

    if (Sim::GetState() != Sim::STATE_ACTIVE) {
        return;
    }

    if (mLast->IsBlending()) {
        int c = mHead;
        int n = Next(c);
        if (n == mTail) {
            return;
        }
        mHead = n;
        mLast = mStateArray + c;
    }

    *mLast = mSaved;

    {
        float t = data.GetQuantizedFloat(TheOnlineManager.QuantFloatSimTime);
        mLast->Extrapolate(t);
        repositioncount = mLast->Import(t, data, priority_mask);
    }

    data.GetBool(mHasHeadset);
    if (repositioncount != mRepositionCount ||
        Sim::GetTime() - mLast->GetTime() > kMaxLatency) {
        mLast->Blend(mSaved, kMaxBlendTime);
        mRepositionCount = repositioncount;
    }

    mSaved = *mLast;
    mUpdateTime = GetMasterTime();

    if (data.GetByteLengthRemaining() > 1) {
        if (!mCops) {
            mCops = new CopMap;
        }

        HSIMABLE handle = reinterpret_cast<HSIMABLE>(data.GetQuantizedInt(TheOnlineManager.QuantInt4Bit));
        int type = data.GetQuantizedInt(TheOnlineManager.QuantInt3Bit);
        Attrib::Key cartype = ExtrapolatedCar_CopTypes[type];
        std::pair<CopMap::iterator, bool> result;
        result = mCops->insert(CopMap::value_type(
            handle, static_cast<ExtrapolatedCar *>(nullptr)));
        if (result.second) {
            result.first->second = new ExtrapolatedCar(cartype);
        }
        result.first->second->ImportStream(data, priority_mask);
    }
}

#ifdef EA_PLATFORM_WIN32
// PC (speed.exe 0x75E210): el choque se guarda 0,2 s despues, todo va bajo mMutex, la latencia se mide con simtime
// (contra mLastPDPAesthetic y contra la ultima muestra) y el modo de fisica que queda se guarda en mCurrentMode; sin
// kUseDriverAI ni kSpamPhysics.
void ExtrapolatedCar::ImportSimable(ISimable *simable, float t, float simtime) {
    if (!simable) {
        return;
    }

    if (mCollisionTime == 0.0f) {
        ICollisionBody *collision_body;

        if (simable->QueryInterface(&collision_body) && collision_body->HasHadCollision()) {
            mCollisionTime = simtime + 0.2f;
        }
    }

    mMutex->Lock();
    int n = mTail;
    State &state = mStateArray[n];
    bool colliding = false;

    if (!mPaused && mCollisionTime > 0.0f) {
        if (!state.HasPriority(PDP_MASK_LOW) || state.GetTime() <= mCollisionTime) {
            colliding = true;
        }
    }

    if (colliding) {
        state.Import(simable, mCollisionTime);
    } else {
        state.Extrapolate(simtime);
        mCollisionTime = 0.0f;
    }

    mBlended = state;
    n = Next(n);
    while (n != mHead) {
        State &next_state = mStateArray[n];
        if (next_state.GetTime() <= mCollisionTime) {
            mTail = n;
        } else {
            next_state.Extrapolate(simtime);
            if (next_state.Blend(mBlended, t)) {
                mTail = n;
            }
        }
        n = Next(n);
    }

    IVehicle *vehicle;

    if (simable->QueryInterface(&vehicle)) {
        bool recent = simtime - mLastPDPAesthetic < kMaxLatency;

        if (UMath::Abs(simtime - mSaved.GetTime()) * 2.0f < kMaxLatency) {
            if (vehicle->GetPhysicsMode() == PHYSICS_MODE_SIMULATED) {
                if (vehicle->IsOffWorld() || !recent) {
                    mActive = false;
                }
            } else {
                mActive = true;
            }
        } else {
            if (vehicle->GetPhysicsMode() != PHYSICS_MODE_INACTIVE) {
                vehicle->SetPhysicsMode(PHYSICS_MODE_INACTIVE);
            }
            mHasHeadset = false;
            mActive = false;
        }

        bool needReset = false;

        if (mUseDriverAI) {
            IHumanAI *human_ai;

            if (simable->QueryInterface(&human_ai)) {
                human_ai->SetAiControl(false);
            }
            mUseDriverAI = false;
            needReset = true;
        }

        if (mActive) {
            bool can_simulate;

            if (vehicle->GetPhysicsMode() == PHYSICS_MODE_INACTIVE) {
                can_simulate = mBlended.IsValidPosition();
            } else {
                can_simulate = !vehicle->IsOffWorld();
            }
            if (vehicle->GetPhysicsMode() != PHYSICS_MODE_SIMULATED && can_simulate && recent) {
                vehicle->SetPhysicsMode(PHYSICS_MODE_SIMULATED);
                needReset = true;
            } else if (mPaused) {
                needReset = true;
            }
        } else if (vehicle->GetPhysicsMode() == PHYSICS_MODE_SIMULATED) {
            vehicle->SetPhysicsMode(PHYSICS_MODE_EMULATED);
            needReset = false;
        }

        mCurrentMode = vehicle->GetPhysicsMode();
        if (needReset) {
            mBlended.SetOnGround(vehicle);
        }
        if (vehicle->IsActive() && !mUseDriverAI) {
            mBlended.Export(simable);
        }
    }

    if (mCops) {
        CopMap::iterator copiter;
        CopMap::iterator nextiter = mCops->begin();

        while (nextiter != mCops->end()) {
            copiter = nextiter++;

            int i = static_cast<int>(reinterpret_cast<uintptr_t>(copiter->first));

            if (!ExtrapolatedCar_kHandles[i]) {
                simable = copiter->second->SpawnVehicle();
                if (!simable) {
                    continue;
                }
                ExtrapolatedCar_kHandles[i] = simable->GetInstanceHandle();
            } else {
                simable = ISimable::FindInstance(ExtrapolatedCar_kHandles[i]);
            }
            if (!simable) {
                simable = copiter->second->SpawnVehicle();
                ExtrapolatedCar_kHandles[i] = simable->GetInstanceHandle();
            }

            copiter->second->ImportSimable(simable, t, simtime);
            if (GetLastTime() > copiter->second->GetLastTime()) {
                simable->Kill();
                ExtrapolatedCar_kHandles[i] = nullptr;
                delete copiter->second;
                mCops->erase(copiter);
            }
        }
    }
    mMutex->Unlock();
}
#else
void ExtrapolatedCar::ImportSimable(ISimable *simable, float t, float simtime) {
    if (!simable) {
        return;
    }

    if (mCollisionTime == 0.0f) {
        ICollisionBody *collision_body;

        if (simable->QueryInterface(&collision_body) && collision_body->HasHadCollision()) {
            mCollisionTime = simtime;
        }
    }

    int n = mTail;
    State &state = mStateArray[n];
    bool colliding = false;

    if (!mPaused && mCollisionTime > 0.0f) {
        if (!state.HasPriority(PDP_MASK_LOW) || state.GetTime() <= mCollisionTime) {
            colliding = true;
        }
    }

    if (colliding) {
        state.Import(simable, mCollisionTime);
    } else {
        state.Extrapolate(simtime);
        mCollisionTime = 0.0f;
    }

    mBlended = state;
    n = Next(n);
    while (n != mHead) {
        state = mStateArray[n];
        if (state.GetTime() <= mCollisionTime) {
            mTail = n;
        } else {
            state.Extrapolate(simtime);
            if (state.Blend(mBlended, t)) {
                mTail = n;
            }
        }
        n = Next(n);
    }

    IVehicle *vehicle;

    if (simable->QueryInterface(&vehicle)) {
        if (Sim::GetTime() - mSaved.GetTime() < kMaxLatency) {
            if (vehicle->GetPhysicsMode() == PHYSICS_MODE_SIMULATED) {
                if (vehicle->IsOffWorld()) {
                    mActive = false;
                }
            } else {
                mActive = true;
            }
        } else {
            mHasHeadset = false;
            mActive = false;
        }

        bool needReset = false;

        if (kUseDriverAI != mUseDriverAI) {
            IHumanAI *human_ai;

            if (simable->QueryInterface(&human_ai)) {
                human_ai->SetAiControl(kUseDriverAI);
            }
            mUseDriverAI = kUseDriverAI;
            needReset = kUseDriverAI == false;
        }

        if (mActive) {
            if (vehicle->GetPhysicsMode() == PHYSICS_MODE_EMULATED && mBlended.IsValidPosition()) {
                vehicle->SetPhysicsMode(PHYSICS_MODE_SIMULATED);
                needReset = true;
            } else if (mPaused) {
                needReset = true;
            }
        } else if (vehicle->GetPhysicsMode() == PHYSICS_MODE_SIMULATED) {
            if (kSpamPhysics) {
                IVehicle *spam_vehicle;

                simable->QueryInterface(&spam_vehicle);
            }
            vehicle->SetPhysicsMode(PHYSICS_MODE_EMULATED);
            needReset = false;
        }

        if (needReset) {
            mBlended.SetOnGround(vehicle);
        }
        if (!mUseDriverAI) {
            mBlended.Export(simable);
        }
    }

    if (mCops) {
        CopMap::iterator copiter;
        CopMap::iterator nextiter = mCops->begin();

        while (nextiter != mCops->end()) {
            copiter = nextiter++;

            int i = static_cast<int>(reinterpret_cast<uintptr_t>(copiter->first));

            if (!ExtrapolatedCar_kHandles[i]) {
                simable = copiter->second->SpawnVehicle();
                if (!simable) {
                    continue;
                }
                if (kSpamPhysics) {
                    IVehicle *spam_vehicle;

                    simable->QueryInterface(&spam_vehicle);
                }
                ExtrapolatedCar_kHandles[i] = simable->GetInstanceHandle();
            } else {
                simable = ISimable::FindInstance(ExtrapolatedCar_kHandles[i]);
            }
            if (!simable) {
                simable = copiter->second->SpawnVehicle();
                ExtrapolatedCar_kHandles[i] = simable->GetInstanceHandle();
            }

            copiter->second->ImportSimable(simable, t, simtime);
            if (GetLastTime() > copiter->second->GetLastTime()) {
                if (kSpamPhysics) {
                    IVehicle *spam_vehicle;

                    simable->QueryInterface(&spam_vehicle);
                }
                simable->Kill();
                ExtrapolatedCar_kHandles[i] = nullptr;
                delete copiter->second;
                mCops->erase(copiter);
            }
        }
    }
}

#endif

void ExtrapolatedCar::ExtractExtrapolatedPosition(UMath::Vector3 &position) const {
    position = mBlended.mPosition;
}

void ExtrapolatedCar::ExtractExtrapolatedDirection(UMath::Vector3 &direction) const {
    mMutex->Lock();
    mBlended.ExtractDirection(direction);
    mMutex->Unlock();
}

bool ExtrapolatedCar::IsAbleToSee(ExtrapolatedCar &target) {
    return mBlended.SquaredDistanceTo(target.mBlended) < 40000.0f;
}
#endif
