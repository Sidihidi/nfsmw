#include "GTrigger.h"

#include "GIcon.h"
#include "GManager.h"
#include "GRaceDatabase.h"
#include "GReflected.h"
#include "Speed/Indep/Libs/Support/Utility/UMath.h"
#include "Speed/Indep/Src/Ecstasy/EmitterSystem.h"
#include "Speed/Indep/Src/Ecstasy/eMath.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Generated/Events/EGTriggerInternal.hpp"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/gameplay.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/gameplay_hash.h"
#include "Speed/Indep/Src/Generated/Messages/MTriggerEnter.h"
#include "Speed/Indep/Src/Generated/Messages/MTriggerExit.h"
#include "Speed/Indep/Src/Generated/Messages/MTriggerInside.h"
#include "Speed/Indep/Src/World/WCollisionAssets.h"

// Las locales llevan los nombres del DWARF. Las banderas del disparador se ponen una a una, como en la A124
// (lineas 123-130); GameCube las junta en 0x4810d. Los datos del evento son los de EGTriggerInternal.
GTrigger::GTrigger(const unsigned int &triggerKey)
    : GRuntimeInstance(triggerKey, kGameplayObjType_Trigger), mWorldTrigger(), mTriggerEnabled(0) {
    mIcon = NULL;
    mEnabled = false;
    mActivationReferences = 0;
    bMemSet(mParticleEffect, 0, sizeof(mParticleEffect));

    {
        UMath::Matrix4 rotMat = UMath::Matrix4::kIdentity;
        UMath::Vector3 initialVec = {0.0f, 0.0f, 1.0f};
        UMath::Init(rotMat, 1.0f, 1.0f, 1.0f);
        UMath::MultYRot(rotMat, -Rotation() / 360.0f, rotMat);
        UMath::Rotate(initialVec, rotMat, initialVec);
        mDirection = initialVec;
    }

    mSimObjInside.reserve(8);

    const UMath::Vector3 &pos = Position();
    UMath::Vector3 dim;
    bool hasDimensions = Dimensions(dim);
    UMath::Vector3 posSwizzled = UMath::Vector3Make(-pos.y, pos.z, pos.x);
    UMath::Vector3 dimSwizzled = UMath::Vector3Make(dim.y, dim.z, dim.x);
    UMath::Matrix4 mat = UMath::Matrix4::kIdentity;

    float radius = 0.0f;
    if (Radius(radius)) {
        mWorldTrigger.fShape = 3;
        dimSwizzled.x = dimSwizzled.y = dimSwizzled.z = radius + radius;
    } else if (hasDimensions) {
        mWorldTrigger.fShape = 1;
        radius = UMath::Sqrt(0.25f * dimSwizzled.x * dimSwizzled.x + 0.25f * dimSwizzled.z * dimSwizzled.z);
    } else {
        dimSwizzled.x = Width();
        dimSwizzled.y = 50.0f;
        dimSwizzled.z = 1.0f;
        mWorldTrigger.fShape = 1;
        radius = UMath::Sqrt(0.25f * dimSwizzled.x * dimSwizzled.x + 0.25f * dimSwizzled.z * dimSwizzled.z);
    }

    UMath::Init(mat, 1.0f, 1.0f, 1.0f);
    UMath::MultYRot(mat, -Rotation() / 360.0f, mat);

    mWorldTrigger.fHeight = dimSwizzled.y + dimSwizzled.y;
    mWorldTrigger.fType = 1;
    mWorldTrigger.fFingerprint = 0;
    mWorldTrigger.fMatRow0Width = mat.v0;
    mWorldTrigger.fMatRow0Width.w = dimSwizzled.x;
    mWorldTrigger.fMatRow2Length = mat.v2;
    mWorldTrigger.fMatRow2Length.w = dimSwizzled.z;
    mWorldTrigger.fPosRadius = UMath::Vector4Make(posSwizzled, radius);
    mWorldTrigger.fFlags = 0;
    mWorldTrigger.fFlags |= 0x4;
    mWorldTrigger.fFlags |= 0x8;
    mWorldTrigger.fFlags |= 0x1;
    mWorldTrigger.fFlags |= 0x100;
    mWorldTrigger.fFlags |= 0x8000;
    mWorldTrigger.fFlags |= 0x40000;
    if (OneShot()) {
        mWorldTrigger.fFlags |= 0x2;
    }

    mEventList.fNumEvents = 1;
    mEventStaticData.fEventID = 0xC34649C0;
    mEventStaticData.fEventSize = 8;
    mEventStaticData.fDataOffset = reinterpret_cast<const char *>(mTriggerEventData) - reinterpret_cast<const char *>(&mEventStaticData);
    EGTriggerInternal::StaticData *pTriggerData = reinterpret_cast<EGTriggerInternal::StaticData *>(mTriggerEventData);
    pTriggerData->fTriggerKey = GetCollection();
    mWorldTrigger.fEvents = &mEventList;

    bool showIconBasedOnBin = true;
    if (IsDerivedFromTemplate(Attrib::Hash::gameplay::key_engagetrigger)) {
        SetFlag(0x200);
        const GCollectionKey &targetActivityKey = TargetActivity();
        GRaceParameters *parms = GRaceDatabase::Get().GetRaceFromKey(targetActivityKey.GetCollectionKey());
        if (parms != NULL) {
            GIcon::Type iconType = (GIcon::Type)0;
            if (parms->GetIsBossRace()) {
                iconType = GIcon::kType_RaceRival;
            } else {
                switch (parms->GetRaceType()) {
                case GRace::kRaceType_P2P:
                    iconType = GIcon::kType_RaceSprint;
                    break;
                case GRace::kRaceType_Circuit:
                    iconType = GIcon::kType_RaceCircuit;
                    break;
                case GRace::kRaceType_Drag:
                    iconType = GIcon::kType_RaceDrag;
                    break;
                case GRace::kRaceType_Knockout:
                    iconType = GIcon::kType_RaceKnockout;
                    break;
                case GRace::kRaceType_Tollbooth:
                    iconType = GIcon::kType_RaceTollbooth;
                    break;
                case GRace::kRaceType_SpeedTrap:
                    iconType = GIcon::kType_RaceSpeedtrap;
                    break;
                default:
                    break;
                }
            }
            if (iconType != 0) {
                mIcon = GManager::Get().AllocIcon(iconType, pos, 0.0f, false);
            }
            showIconBasedOnBin = false;
        }
    } else if (IsDerivedFromTemplate(Attrib::Hash::gameplay::key_zone_carlot)) {
        SetFlag(0x2000);
        mIcon = GManager::Get().AllocIcon(GIcon::kType_GateCarLot, pos, 0.0f, false);
    } else if (IsDerivedFromTemplate(Attrib::Hash::gameplay::key_zone_customshop)) {
        SetFlag(0x4000);
        mIcon = GManager::Get().AllocIcon(GIcon::kType_GateCustomShop, pos, 0.0f, false);
    } else if (IsDerivedFromTemplate(Attrib::Hash::gameplay::key_zone_safehouse)) {
        SetFlag(0x8000);
        mIcon = GManager::Get().AllocIcon(GIcon::kType_GateSafehouse, pos, 0.0f, false);
    } else if (IsDerivedFromTemplate(Attrib::Hash::gameplay::key_speedtrap)) {
        SetFlag(0x100);
#ifdef EA_BUILD_A124 // sin icono de radar de carrera: todos los radares llevan el del mundo abierto
        mIcon = GManager::Get().AllocIcon(GIcon::kType_SpeedTrap, pos, 0.0f, false);
#else
        if (OpenWorldSpeedTrap()) {
            mIcon = GManager::Get().AllocIcon(GIcon::kType_SpeedTrap, pos, 0.0f, false);
        } else {
            showIconBasedOnBin = false;
            mIcon = GManager::Get().AllocIcon(GIcon::kType_SpeedTrapInRace, pos, 0.0f, false);
        }
#endif
    } else if (IsDerivedFromTemplate(Attrib::Hash::gameplay::key_finishline)) {
        SetFlag(0x400);
    } else if (IsDerivedFromTemplate(Attrib::Hash::gameplay::key_time_bonus_checkpoint)) {
        SetFlag(0x800);
    } else if (IsDerivedFromTemplate(Attrib::Hash::gameplay::key_moneybag)) {
        SetFlag(0x1000);
#ifndef EA_BUILD_A124 // la A124 no tiene disparadores de desbloqueo de zona con icono
    } else if (IsDerivedFromTemplate(Attrib::Hash::gameplay::key_areaunlocktrigger)) {
        mIcon = GManager::Get().AllocIcon(GIcon::kType_AreaUnlock, pos, 0.0f, false);
#endif
    }

    if (showIconBasedOnBin && mIcon != NULL) {
        if (BinIndex() > 0) {
            if (FEDatabase->GetCareerSettings()->GetCurrentBin() <= BinIndex()) {
                mIcon->Show();
                mIcon->ShowOnMap();
            }
        }
    }
}

GTrigger::~GTrigger() {
    Enable(false);
    for (unsigned int i = 0; i < 2; i++) {
        if (mParticleEffect[i] != NULL) {
            mParticleEffect[i]->UnSubscribe();
            if (mParticleEffect[i] != NULL) {
                delete mParticleEffect[i];
            }
        }
    }
    if (mIcon != NULL) {
        GManager::Get().FreeIcon(mIcon);
        mIcon = NULL;
    }
}

GActivity *GTrigger::GetTargetActivity() {
    unsigned int key = 0x277566F3;
    return static_cast<GActivity *>(GetConnectedInstance(key, 0));
}

void GTrigger::AddActivationReference() {
    mActivationReferences = mActivationReferences + 1;
    if (mEnabled == 0) {
        Enable(true);
    }
}

void GTrigger::RemoveActivationReference() {
#ifdef EA_BUILD_A124 // la A124 descuenta sin mirar si ya estaba a cero
    mActivationReferences = mActivationReferences - 1;
#else
    if (mActivationReferences > 0) {
        mActivationReferences = mActivationReferences - 1;
    }
#endif
    if (mActivationReferences < 1) {
        Disable();
    }
}

EmitterGroup *GTrigger::CreateParticleEffect(const char *effectName, UMath::Vector3 &pos) {
    EmitterGroup *group = gEmitterSystem.CreateEmitterGroup(Attrib::StringKey(effectName), 0x8040000);
    if (group != NULL) {
        bMatrix4 matrix;
        bVector3 translation;
        bFill(&translation, pos.z, -pos.x, pos.y);
        eCreateTranslationMatrix(&matrix, translation);
        group->SetLocalWorld(&matrix);
        group->SetAutoUpdate(true);
        group->SubscribeToDeletion(this, NotifyEmitterGroupDelete);
        group->Disable();
        gEmitterSystem.AddEmitterGroup(group);
    }
    return group;
}

void GTrigger::CreateAllParticleEffects() {
    const char *effectName = ParticleEffect();
    if (effectName != NULL) {
        if (*effectName != 0) {
            UMath::Vector3 position;
            GetPosition(position);
            float flareSpacing = FlareSpacing();
            if (flareSpacing > 0.0f) {
                UMath::Vector3 up = {0.0f, 1.0f, 0.0f};
                bVector3 cross;
                bCross(&cross, (const bVector3 *)&up, (const bVector3 *)&mDirection);
                bScale(&cross, &cross, flareSpacing);
                bVector3 flareA;
                bVector3 flareB;
                bScaleAdd(&flareA, (const bVector3 *)&position, &cross, -1.0f);
                bScaleAdd(&flareB, (const bVector3 *)&position, &cross, 1.0f);
                mParticleEffect[0] = CreateParticleEffect(effectName, *(UMath::Vector3 *)&flareA);
                mParticleEffect[1] = CreateParticleEffect(effectName, *(UMath::Vector3 *)&flareB);
            } else {
                mParticleEffect[0] = CreateParticleEffect(effectName, position);
            }
        }
    }
}

void GTrigger::ClearParticleEffects() {
    for (unsigned int onEffect = 0; onEffect < 2; onEffect++) {
        if (mParticleEffect[onEffect] != NULL) {
            mParticleEffect[onEffect]->UnSubscribe();
            if (mParticleEffect[onEffect] != NULL) {
                delete mParticleEffect[onEffect];
            }
        }
    }
    bMemSet(mParticleEffect, 0, 8);
}

void GTrigger::EnableParticleEffects(bool enabled) {
    for (unsigned int i = 0; i < 2; i++) {
        if (mParticleEffect[i] != NULL) {
            if (enabled) {
                mParticleEffect[i]->Enable();
            } else {
                mParticleEffect[i]->Disable();
            }
        }
    }
}

void GTrigger::RefreshParticleEffects() {
    ClearParticleEffects();
    CreateAllParticleEffects();
    EnableParticleEffects(mEnabled);
}

void GTrigger::NotifyEmitterGroupDelete(void *obj, EmitterGroup *group) {
    GTrigger *trigger = static_cast<GTrigger *>(obj);
    for (unsigned int i = 0; i < 2; i++) {
        if (trigger->mParticleEffect[i] == group) {
            trigger->mParticleEffect[i] = NULL;
        }
    }
}

void GTrigger::Enable(bool enable) {
    if (mTriggerEnabled == 0) {
        if (enable) {
            WCollisionAssets::sWCollisionAssets->AddTrigger(&mWorldTrigger);
            mTriggerEnabled = 1;
        }
    } else if (!enable) {
        WCollisionAssets::sWCollisionAssets->RemoveTrigger(&mWorldTrigger);
        mTriggerEnabled = enable;
    }
    if (enable) {
        mWorldTrigger.Enable();
    } else {
        mWorldTrigger.Disable();
    }
    if (enable) {
        CreateAllParticleEffects();
    } else {
        ClearParticleEffects();
    }
    mEnabled = enable;
}

void GTrigger::GetPosition(UMath::Vector3 &position) {
    position = Vector4To3(mWorldTrigger.fPosRadius);
}

void GTrigger::NotifySimableTrigger(ISimable *isim, int triggerStimulus) {
    if (isim == NULL) {
        return;
    }
    bool wasEnter = (triggerStimulus == 1);
    bool wasExit = (triggerStimulus == 2);
    bool inside = IsInside(isim);
    if (wasExit) {
        MarkAsOutside(isim);
        if (FireOnExit()) {
            (MTriggerExit(GCollectionKey(this), isim->GetInstanceHandle())).Post(UCrc32(0x20d60dbf));
        }
    }
    if (wasEnter) {
        if (!inside) {
            MarkAsInside(isim);
            (MTriggerEnter(GCollectionKey(this), isim->GetInstanceHandle())).Post(UCrc32(0x20d60dbf));
        }
        (MTriggerInside(GCollectionKey(this), isim->GetInstanceHandle())).Post(UCrc32(0x20d60dbf));
    }
}

void GTrigger::Reset() {
    mSimObjInside.erase(mSimObjInside.begin(), mSimObjInside.end());
}

void GTrigger::ShowIcon() {
    if (mIcon != NULL) {
        mIcon->Show();
        mIcon->ShowOnMap();
    }
}

void GTrigger::HideIcon() {
    if (mIcon != NULL) {
        mIcon->Hide();
        mIcon->HideOnMap();
    }
}

void GTrigger::MarkAsInside(ISimable *simable) {
    if (std::find(mSimObjInside.begin(), mSimObjInside.end(), simable) == mSimObjInside.end()) {
        mSimObjInside.push_back(simable);
    }
}

void GTrigger::MarkAsOutside(ISimable *simable) {
    UTL::Std::vector<ISimable *, _type_ID_SimObjList>::iterator iter = std::find(mSimObjInside.begin(), mSimObjInside.end(), simable);
    if (iter != mSimObjInside.end()) {
        mSimObjInside.erase(iter);
    }
}

bool GTrigger::IsInside(ISimable *simable) {
    return std::find(mSimObjInside.begin(), mSimObjInside.end(), simable) != mSimObjInside.end();
}
