#include "Speed/Indep/Libs/Support/Utility/UDefs.h"
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/Libs/Support/Utility/UTypes.h"
#include "Speed/Indep/Src/AI/AIAction.h"
#include "Speed/Indep/Src/AI/AITarget.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/pursuitlevels.h"
#include "Speed/Indep/Src/Interfaces/IListener.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Interfaces/Simables/IRigidBody.h"
#include "Speed/Indep/Src/Interfaces/Simables/ISimable.h"
#include "Speed/Indep/Src/Sim/Collision.h"
#include "Speed/Indep/Src/Sim/Simulation.h"
#include "Speed/Indep/Src/Speech/SoundAI.h"
#include "Speed/Indep/Src/World/WRoadNetwork.h"

// total size: 0x48
class AIActionHeliPursuit : public AIAction, public Debugable, public Sim::Collision::IListener {
  public:
    enum kPursuitMode {
        kStraight_Line = 0,
#ifdef EA_BUILD_A124
        // El helicoptero del prototipo aun sigue carreteras (los stabs de la A124 traen kFollow_Roads).
        kFollow_Roads = 1,
        kSearch_Pattern = 2,
        kSkid_Hit_Approach = 3,
        kSkid_Hit_Strike = 4,
#else
        kSearch_Pattern = 1,
        kSkid_Hit_Approach = 2,
        kSkid_Hit_Strike = 3,
#endif
    };

    static AIAction *Construct(struct AIActionParams *params);

    AIActionHeliPursuit(AIActionParams *params, float score);
  private:
    void StartPathToPlayerCar();
    float UpdateNavPos(float lookAheadDistance);
    void StraightLinePursuit();
#ifdef EA_BUILD_A124
    void FollowRoadPursuit();
#endif
    void SkidHitPursuit();
    void SetNextPerpSearchDest();
    void SearchForPerp();
  public:
    // Virtual functions
    virtual void OnDebugDraw();

    // Virtual overrides
    // IUnknown
    ~AIActionHeliPursuit() override {
        Sim::Collision::RemoveListener(this);
    }

    // AIAction
    bool CanBeAttempted(float dT) override {
        return true;
    }
    void BeginAction(float dT) override;
    bool IsFinished() override;
    void FinishAction(float dT) override;
    void Update(float dT) override;

    void OnBehaviorChange(const UCrc32 &mechanic) override {}

    bool ShouldRestartWhenFinished() override {
        return true;
    }

    // IListener
    void OnCollision(const COLLISION_INFO &cinfo) override;

    WRoadNav *GetPursuitNav() {
        return mIVehicleAI->GetDriveToNav();
    }

  private:
    IVehicleAI *mIVehicleAI;                  // offset 0x50, size 0x4
    IVehicle *mIVehicle;                      // offset 0x54, size 0x4
    IRigidBody *mIRigidBody;                  // offset 0x58, size 0x4
    IAIHelicopter *mIAIHelicopter;            // offset 0x5C, size 0x4
    IPursuitAI *mIPursuitAI;                  // offset 0x60, size 0x4
    float mPursuitTime;                       // offset 0x64, size 0x4
    float mSkidKnockTimer;                    // offset 0x68, size 0x4
    float mPathTime;                          // offset 0x6C, size 0x4
    bool mBuildingPath;                       // offset 0x70, size 0x1
    float mSearchPatternAngle;                // offset 0x74, size 0x4
    ALIGNVEC UMath::Vector3 mSearchDestPoint; // offset 0x78, size 0xC
    IRigidBody *mPlayerRigidBody;             // offset 0x84, size 0x4
    ALIGNVEC UMath::Vector3 mPlayerPosition;  // offset 0x88, size 0xC
    ALIGNVEC UMath::Vector3 mSkidHitOffset;   // offset 0x94, size 0xC
    int mCollisionAbort;                      // offset 0xA0, size 0x4
    float mPlayerSpeed;                       // offset 0xA4, size 0x4
    kPursuitMode mPursuitMode;                // offset 0xA8, size 0x4
};

AIActionHeliPursuit::AIActionHeliPursuit(AIActionParams *params, float score)
    : AIAction(params, score),                 //
      mSearchDestPoint(UMath::Vector3::kZero), //
      mPursuitTime(0.0f),                      //
      mSkidKnockTimer(0.0f),                   //
      mPathTime(0.0f),                         //
      mSearchPatternAngle(0.0f),               //
      mBuildingPath(false),                    //
      mPlayerSpeed(0.0f),                      //
#ifdef EA_BUILD_A124
      mPursuitMode(kFollow_Roads), //
#else
      mPursuitMode(kStraight_Line), //
#endif
      mCollisionAbort(0) {
    MakeDebugable(DBG_AI);

    params->mOwner->QueryInterface(&mIVehicleAI);
    params->mOwner->QueryInterface(&mIPursuitAI);
    params->mOwner->QueryInterface(&mIVehicle);
    params->mOwner->QueryInterface(&mIAIHelicopter);
    mIRigidBody = params->mOwner->GetRigidBody();
    Sim::Collision::AddListener(this, mIRigidBody
#ifndef EA_BUILD_A124
        ,
        "AIActionHeliPursuit"
#endif
    );
}

void AIActionHeliPursuit::OnCollision(const COLLISION_INFO &cinfo) {
    if (mSkidKnockTimer > 0.0f) {
        if (cinfo.type == COLLISION_INFO::GROUND) {
            mCollisionAbort += 2;
        } else if (cinfo.type == COLLISION_INFO::OBJECT) {
            ISimable *obja = ISimable::FindInstance(cinfo.objA);
            if (!obja || !obja->IsPlayer()) {
                ISimable *objb = ISimable::FindInstance(cinfo.objB);
                if (objb && objb->IsPlayer()) {
                    mCollisionAbort += 5;
                }
            } else {
                mCollisionAbort += 5;
            }
        }

        if (mCollisionAbort > 20) {
            mSkidKnockTimer = 0.0f;
        }
    }
}

#ifdef EA_PLATFORM_PLAYSTATION2
// La velocidad de SteerToNav cuando el helicoptero sigue carreteras (FollowRoadPursuit, solo en la A124; SLUS conserva
// el simbolo en .data).
float RoadNetSpeed = 85.0f;
#endif

UTL::COM::Factory<AIActionParams *, AIAction, UCrc32>::Prototype _AIActionHeliPursuit("AIActionHeliPursuit", AIActionHeliPursuit::Construct);

AIAction *AIActionHeliPursuit::Construct(AIActionParams *params) {
    return new AIActionHeliPursuit(params, 0.0f);
}

bool AIActionHeliPursuit::IsFinished() {
    if (mPursuitTime > 10.0f) {
        return true;
    }
    return false;
}

void AIActionHeliPursuit::FinishAction(float dT) {
    mPursuitTime = 0.0f;
    mSkidKnockTimer = 0.0f;
}

void AIActionHeliPursuit::StartPathToPlayerCar() {
    IRigidBody *mPlayerRigidBody = IPlayer::First(PLAYER_LOCAL)->GetSimable()->GetRigidBody();
    UMath::Vector3 seekPosition = mPlayerRigidBody->GetPosition();
    mIAIHelicopter->StartPathToPoint(seekPosition);
    mBuildingPath = true;
    mPathTime = mPursuitTime;
}

void AIActionHeliPursuit::BeginAction(float dT) {
    StartPathToPlayerCar();
}

#ifdef EA_BUILD_A124
float AIActionHeliPursuit::UpdateNavPos(float lookAheadDistance) {
    WRoadNav *road_nav = GetPursuitNav();
    if (road_nav->HitDeadEnd()) {
        return 0.0f;
    }

    UMath::Vector3 navPos = road_nav->GetPosition();
    UMath::Vector3 heliPosition = mIRigidBody->GetPosition();
    UMath::Vector3 heliToNav;
    UMath::Vector3 navForwardVector = road_nav->GetForwardVector();
    UMath::Sub(navPos, heliPosition, heliToNav);
    float length = UMath::Length(heliToNav);
    float dot = UMath::Dot(navForwardVector, heliToNav);

    if (dot < lookAheadDistance) {
        road_nav->IncNavPosition(fabsf(lookAheadDistance - dot), UMath::Vector3::kZero, 0.0f);
    }

    if (mBuildingPath == true) {
        float newdot;
        do {
            navPos = road_nav->GetPosition();
            navForwardVector = road_nav->GetForwardVector();
            UMath::Sub(navPos, heliPosition, heliToNav);
            length = UMath::Length(heliToNav);
            newdot = UMath::Dot(navForwardVector, heliToNav);
            if (newdot < dot) {
                road_nav->IncNavPosition(lookAheadDistance, UMath::Vector3::kZero, 0.0f);
            }
        } while (newdot < dot && road_nav->IsValid() && !road_nav->HitDeadEnd());
        mBuildingPath = false;
    }
    return length;
}
#endif

void AIActionHeliPursuit::StraightLinePursuit() {
    UMath::Vector3 myPosition = mIRigidBody->GetPosition();
    UMath::Vector3 seekPosition;
    UMath::Vector3 direction;
    UMath::Vector3 perp2Me;

    UMath::Vector3 perpUnitVel = mPlayerRigidBody->GetLinearVelocity();
    UMath::Unit(perpUnitVel, perpUnitVel);
    UMath::Sub(myPosition, mPlayerPosition, perp2Me);

    bool SkidHitEnabled = false;
    if (mIVehicleAI->GetPursuit()->SkidHitEnabled()) {
        if (mSkidKnockTimer < -5.0f) {
            SkidHitEnabled = true;
        }
    }
    float heliAheadOfPerpDot = UMath::Length(perp2Me);
    float distToPerp;
    if (heliAheadOfPerpDot < 35.0f && heliAheadOfPerpDot > 5.0f) {
        float ydif = perp2Me.y;
        perp2Me.y = 0.0f;
        perpUnitVel.y = 0.0f;
        if (UMath::Dot(perp2Me, perpUnitVel) / heliAheadOfPerpDot > 0.707f && mIVehicleAI && SkidHitEnabled && ydif < 13.0f) {
            mPursuitMode = kSkid_Hit_Approach;
            mSkidKnockTimer = 8.0f;
            mSkidHitOffset = UMath::Vector3::kZero;
            SoundAI *copspeech = SoundAI::Get();
            if (copspeech && copspeech->GetHeli()) {
                int parm = bRandom(2) != 0 ? 2 : 4;
                copspeech->GetHeli()->SelfStrategy(parm);
            }
        }
    }

    mPlayerRigidBody->GetForwardVector(direction);
    UMath::Unit(direction, direction);

    float leadDist = mPlayerSpeed * 0.4f + 30.0f;
    if (leadDist > 45.0f) {
        leadDist = 45.0f;
    }
    if (SkidHitEnabled) {
        leadDist *= 0.75f;
    }

    UMath::ScaleAdd(direction, leadDist, mPlayerPosition, seekPosition);

    if (mSkidKnockTimer > 0.0f) {
        seekPosition.y += 2.0f;
    } else if (heliAheadOfPerpDot > 55.0f || mSkidKnockTimer > -5.0f) {
        seekPosition.y += 12.0f;
    } else {
        seekPosition.y += 6.0f;
    }

    float chaseHeight = seekPosition.y;
    seekPosition.y = 0.0f;

    UMath::Vector3 meToSeek;
    UMath::Sub(seekPosition, myPosition, meToSeek);
    meToSeek.y = 0.0f;
    float distToSeekPos = UMath::Normalize(meToSeek);

    float adjustDriving = UMath::Dot(meToSeek, mPlayerRigidBody->GetLinearVelocity());
    if (adjustDriving < 0.0f) {
        adjustDriving *= 0.5f;
    }
    float driveSpeed;
    float desiredClosingSpeed;
    UMath::Vector3 *lookAtPosition = &seekPosition;
    if (distToSeekPos > 15.0f) {
        desiredClosingSpeed = 30.0f;
        if (distToSeekPos > 300.0f) {
#ifdef EA_BUILD_A124
            // La A124 vuelve a buscar el camino al coche del jugador (StartPathToPlayerCar en su asm).
            chaseHeight += 30.0f;
            StartPathToPlayerCar();
            mPursuitMode = kFollow_Roads;
#else
            desiredClosingSpeed = 50.0f;
#endif
        }
    } else {
        if (distToSeekPos > 3.0f) {
            desiredClosingSpeed = 20.0f;
        } else {
            desiredClosingSpeed = distToSeekPos;
        }
        if (distToSeekPos < 5.0f && mPlayerSpeed < 15.0f) {
            lookAtPosition = &mPlayerPosition;
        }
    }
    driveSpeed = adjustDriving + desiredClosingSpeed;
    mIVehicleAI->SetDriveSpeed(driveSpeed);

    mIAIHelicopter->SetDestinationVelocity(mPlayerRigidBody->GetLinearVelocity());

    seekPosition.y = chaseHeight;
    mIVehicleAI->SetDriveTarget(seekPosition);

    mIAIHelicopter->SetLookAtPosition(*lookAtPosition);

    mIVehicleAI->DoDriving(7);
}

#ifdef EA_BUILD_A124
void AIActionHeliPursuit::FollowRoadPursuit() {
    UMath::Vector3 myPosition = mIRigidBody->GetPosition();
    if (UMath::Distancexz(mPlayerPosition, myPosition) < 50.0f) {
        mPursuitMode = kStraight_Line;
        mIAIHelicopter->SetDestinationVelocity(mPlayerRigidBody->GetLinearVelocity());
        return;
    }

    WRoadNav *road_nav = mIVehicleAI->GetDriveToNav();
    if (road_nav->GetNavType() == WRoadNav::kTypePath) {
        UpdateNavPos(bClamp(mIRigidBody->GetSpeed() * 0.7f, 15.0f, 35.0f));
    }

    if (mPursuitTime - mPathTime > 0.6f && !road_nav->FindingPath()) {
        StartPathToPlayerCar();
    }

    mIAIHelicopter->SteerToNav(road_nav, 20.0f, RoadNetSpeed, false);

    mIVehicleAI->DoDriving(7);
}
#endif

void AIActionHeliPursuit::SkidHitPursuit() {
    UMath::Vector3 seekPosition;
    UMath::Vector3 myPosition = mIRigidBody->GetPosition();
    UMath::Vector3 perp2Me;
    UMath::Sub(myPosition, mPlayerPosition, perp2Me);

    UMath::Vector3 perpForward;
    mPlayerRigidBody->GetForwardVector(perpForward);

    UMath::Vector3 perpLinVel;
    perpLinVel = mPlayerRigidBody->GetLinearVelocity();

    UMath::Vector3 perpRightVec;
    mPlayerRigidBody->GetRightVector(perpRightVec);

    float dot = UMath::Dot(perpForward, perp2Me);
    float dist2PerpSQ = UMath::LengthSquare(perp2Me);
    float dotWithRight = UMath::Dot(perp2Me, perpRightVec);

    if (mPursuitMode == kSkid_Hit_Approach) {
        float rscale = 6.0f;
        if (dotWithRight < 0.0f) {
            rscale = -6.0f;
        }
        UMath::Scale(perpRightVec, rscale, mSkidHitOffset);
        UMath::Vector3 localOffset;
        UMath::ScaleAdd(perpLinVel, 0.23f, mSkidHitOffset, localOffset);
        localOffset.y += 1.8f;
        if (perp2Me.y < 2.0f) {
            localOffset.y += 3.0f;
        }
        UMath::Add(localOffset, mPlayerPosition, seekPosition);

        float dToStrikeStart = UMath::Distancexz(seekPosition, myPosition);
        if (dToStrikeStart < 4.0f && UMath::Abs(dotWithRight) > 1.9f) {
            mPursuitMode = kSkid_Hit_Strike;
            if (mSkidKnockTimer < 2.0f) {
                mSkidKnockTimer = 2.0f;
            }
            SoundAI *copspeech = SoundAI::Get();
            if (copspeech && copspeech->GetHeli()) {
                copspeech->GetHeli()->IntentToRam();
            }
        }
    } else {
        UMath::Vector3 localOffset;
        UMath::Scale(perpLinVel, 0.092f, localOffset);
        UMath::ScaleAdd(mSkidHitOffset, -0.5f, localOffset, localOffset);
        localOffset.y += 0.4f;
        UMath::Add(localOffset, mPlayerPosition, seekPosition);
    }

    UMath::Vector3 lookPosition;
    UMath::Add(mPlayerPosition, perpLinVel, lookPosition);
    mIAIHelicopter->SetLookAtPosition(lookPosition);
    mIVehicleAI->SetDriveSpeed(100.0f);

    UMath::Vector3 destVel = mPlayerRigidBody->GetLinearVelocity();
    mIAIHelicopter->SetDestinationVelocity(destVel);
    mIVehicleAI->SetDriveTarget(seekPosition);
    mIVehicleAI->DoDriving(7);

    if (dot > 0.0f) {
        if (dist2PerpSQ > 1600.0f) {
            mSkidKnockTimer = 0.0f;
        }
        if (myPosition.y < mPlayerPosition.y) {
            mSkidKnockTimer = 0.0f;
        }
    } else if (dist2PerpSQ > 144.0f) {
        mSkidKnockTimer = 0.0f;
    }

    if (mSkidKnockTimer <= 0.0f) {
        mPursuitMode = kStraight_Line;
    }
}

void AIActionHeliPursuit::SetNextPerpSearchDest() {
    IPursuit *ip = mIVehicleAI->GetPursuit();
    if (!ip) {
        return;
    }
    const UMath::Vector3 &centre = ip->GetLastKnownLocation();
    AITarget *target = mIVehicleAI->GetTarget();
    IPerpetrator *iperp;
    target->QueryInterface(&iperp);

    Attrib::Gen::pursuitlevels *pl = iperp->GetPursuitLevelAttrib();
    if (!pl) {
        return;
    }
    float radius = pl->SearchModeRoadblockRadius();
    float workingRadius = radius * 0.2f + Sim::GetRandom()._SimRandom_FloatRange(radius * 0.7f);
    UMath::Vector3 baseVector = UMath::Vector3Make(workingRadius, 0.0f, 0.0f);

    mSearchPatternAngle += 0.125f;
    if (mSearchPatternAngle >= 1.0f) {
        mSearchPatternAngle -= 1.0f;
    }
    UMath::RotateInXZ(mSearchPatternAngle, baseVector, mSearchDestPoint);
    UMath::Add(mSearchDestPoint, centre, mSearchDestPoint);
    mIAIHelicopter->FilterHeliAltitude(mSearchDestPoint);
    mSearchDestPoint.y += 5.0f;
}

// how can this have a diff??
void AIActionHeliPursuit::SearchForPerp() {
    UMath::Vector3 myPosition = mIRigidBody->GetPosition();
    float dToDest = UMath::Distancexz(mSearchDestPoint, myPosition);
    if (dToDest < 30.0f) {
        SetNextPerpSearchDest();
    }
    mIVehicleAI->SetDriveSpeed(70.0f);
    UMath::Vector3 destVel = mIRigidBody->GetLinearVelocity();
    mIAIHelicopter->SetDestinationVelocity(destVel);
    mIVehicleAI->SetDriveTarget(mSearchDestPoint);
    mIAIHelicopter->SetLookAtPosition(mSearchDestPoint);
    mIVehicleAI->DoDriving(7); // TODO magic
}

bool bIgnoreHeliSheet = 0;
bool NeverIgnoreHeliSheet = true;

void AIActionHeliPursuit::Update(float dT) {
    mPursuitTime += dT;
    mSkidKnockTimer -= dT;
    if (mCollisionAbort != 0) {
        mCollisionAbort--;
    }
    mPlayerRigidBody = IPlayer::First(PLAYER_LOCAL)->GetSimable()->GetRigidBody();
    mPlayerPosition = mPlayerRigidBody->GetPosition();
    mPlayerSpeed = mPlayerRigidBody->GetSpeed();
    IPursuit *ip = mIVehicleAI->GetPursuit();
    if (ip && !ip->IsPerpInSight()) {
        if (mPursuitMode != kSearch_Pattern) {
            SetNextPerpSearchDest();
            mPursuitMode = kSearch_Pattern;
        }
    } else {
        if (mPursuitMode == kSearch_Pattern) {
            mPursuitMode = kStraight_Line;
        }
    }
    bIgnoreHeliSheet = false;
    switch (mPursuitMode) {
        case kStraight_Line:
            StraightLinePursuit();
            break;
#ifdef EA_BUILD_A124
        case kFollow_Roads:
            FollowRoadPursuit();
            break;
#endif
        case kSkid_Hit_Approach:
        case kSkid_Hit_Strike:
            if (!NeverIgnoreHeliSheet) {
                bIgnoreHeliSheet = true;
            }
            SkidHitPursuit();
            break;
        case kSearch_Pattern:
            SearchForPerp();
            break;
    }
}

void AIActionHeliPursuit::OnDebugDraw() {}
