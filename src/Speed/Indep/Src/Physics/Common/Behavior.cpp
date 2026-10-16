#include "Speed/Indep/Src/Physics/Behavior.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/pvehicle.h"
#include "Speed/Indep/Src/Generated/AttribSys/GenericAccessor.h"

IMPLEMENT_FACTORY(Behavior);

Behavior::Behavior(const BehaviorParams &params, unsigned int num_interfaces)
    : Sim::Object(num_interfaces + 1),
      mPaused(false),
      mOwner(params.fowner),
      mIOwner(params.fowner),
      mMechanic(params.fMechanic),
      mSignature(params.fSig),
      mPriority(0),
      mProfile(0) {
    const Attrib::Instance &attribs = params.fowner->GetAttributes();
    unsigned int count = attribs->Num_BEHAVIOR_ORDER();
    while (mPriority < count) {
        if (UCrc32(attribs->BEHAVIOR_ORDER(mPriority)) == mMechanic) {
            break;
        }
        mPriority++;
    }
}

void Behavior::Pause(bool pause) {
    if (mPaused != pause) {
        mPaused = pause;
        if (pause) {
            OnPause();
        } else {
            OnUnPause();
        }
    }
}

Attrib::StringKey BEHAVIOR_MECHANIC_AI("BEHAVIOR_MECHANIC_AI");
Attrib::StringKey BEHAVIOR_MECHANIC_RIGIDBODY("BEHAVIOR_MECHANIC_RIGIDBODY");
Attrib::StringKey BEHAVIOR_MECHANIC_INPUT("BEHAVIOR_MECHANIC_INPUT");
Attrib::StringKey BEHAVIOR_MECHANIC_SUSPENSION("BEHAVIOR_MECHANIC_SUSPENSION");
Attrib::StringKey BEHAVIOR_MECHANIC_ENGINE("BEHAVIOR_MECHANIC_ENGINE");
Attrib::StringKey BEHAVIOR_MECHANIC_DAMAGE("BEHAVIOR_MECHANIC_DAMAGE");
Attrib::StringKey BEHAVIOR_MECHANIC_DRAW("BEHAVIOR_MECHANIC_DRAW");
Attrib::StringKey BEHAVIOR_MECHANIC_AUDIO("BEHAVIOR_MECHANIC_AUDIO");
Attrib::StringKey BEHAVIOR_MECHANIC_EFFECTS("BEHAVIOR_MECHANIC_EFFECTS");
Attrib::StringKey BEHAVIOR_MECHANIC_RESET("BEHAVIOR_MECHANIC_RESET");
