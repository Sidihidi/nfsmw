// El vehiculo de otro jugador en una partida online: solo existe en el prototipo A124 (stabs y ELF), donde va
// entre RBCop.cpp y Effects.cpp. La version de venta no tiene online.
#include "Speed/Indep/Src/Generated/Hash.hpp"
#include "Speed/Indep/Src/Physics/Behaviors/RBVehicle.h"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Interfaces/Simables/IAI.h"
#include "Speed/Indep/Src/World/RaceParameters.hpp"

// total size: 0x260
class RBRemote : public RBVehicle {
  public:
    typedef RBVehicle Base;

    static Behavior *Construct(const BehaviorParams &params);

    RBRemote(const BehaviorParams &bp, const RBComplexParams &params);

  protected:
    // IUnknown
    ~RBRemote() override;

    // RigidBody
    bool CanCollideWith(const RigidBody &other) const override;

    virtual void SetInvulerability(float time, bool on) {}
};

BIND_BEHAVIOR_FACTORY(RBRemote);

Behavior *RBRemote::Construct(const BehaviorParams &params) {
    const RBComplexParams rp(params.fparams.Fetch<RBComplexParams>(UCrc32(UCRC32_BASE)));
    return new RBRemote(params, rp);
}

RBRemote::RBRemote(const BehaviorParams &bp, const RBComplexParams &params) : RBVehicle(bp, params) {}

RBRemote::~RBRemote() {}

bool RBRemote::CanCollideWith(const RigidBody &other) const {
    ISimable *othersimable = other.GetOwner();
    if (othersimable->GetSimableType() == SIMABLE_VEHICLE) {
        if (TheRaceParameters.nMaxCops > 0) {
            return true;
        }
        if (!FEDatabase->GetOnlineSettings()->CollisionDetection) {
            return false;
        }

        GRacerInfo *thisinfo = GRaceStatus::Get().GetRacerInfo(GetOwner());
        if (thisinfo) {
            GRacerInfo *otherinfo = GRaceStatus::Get().GetRacerInfo(othersimable);
            if (otherinfo) {
                if (thisinfo->GetLapsCompleted() != otherinfo->GetLapsCompleted()) {
                    return false;
                }
            }
        }

        IHumanAI *humanai;
        if (GetOwner()->QueryInterface(&humanai)) {
            if (humanai->IsFacingWrongWay()) {
                return false;
            }
        }
        if (othersimable->QueryInterface(&humanai)) {
            if (humanai->IsFacingWrongWay()) {
                return false;
            }
        }
    }
    return RBVehicle::CanCollideWith(other);
}
