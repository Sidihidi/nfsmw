#include "SoundAI.h"
#include "MusicFlow.h"
#include "Observer.h"
#include "PursuitFlow.h"
#include "RoadblockFlow.h"
#include "StrategyFlow.h"
#include "Speed/Indep/Src/Speech/EAXDispatch.h"
#include "Speed/Indep/Src/Speech/CsisSpeechEvents.h"
// m1b 2026-10-13 (regresion zSpeech): PVehicle.h se queda (VehicleClass se usa aqui, y el original
// de zSpeech si lo parsea: "VehicleClass" x4 en sus stabs de la A124) pero su static `value` de
// VehicleParams::TypeName ya no se emite en esta TU: el cuerpo vive ahora en zAI.cpp, el unico
// slice del DOL que define TypeName__13VehicleParams. Antes cada TU que parseaba la cabecera
// emitia value + guarda en .bss (zSpeech: 7.424 B contra los 7.216 del original).
#include "Speed/Indep/Src/Physics/PVehicle.h"
#include "Speed/Indep/Src/EAXSound/States/STATE_DriveBy.hpp"
#include "Speed/Indep/Src/EAXSound/Stream/SpeechManager.hpp"
#include "Speed/Indep/Src/Interfaces/Simables/IRenderable.h"
#include "Speed/Indep/Src/Misc/Config.h"
#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/Src/Misc/Profiler.hpp"
#include "Speed/Indep/Src/Sim/Simulation.h"
#include "Speed/Indep/Src/EAXSound/AudioMemoryManager.hpp"
#include "Speed/Indep/bWare/Inc/bSlotPool.hpp"
#include "Speed/Indep/Src/Main/AttribSupport.h"
#include "Speed/Indep/Src/Speech/EAXCharacter.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Gameplay/GRaceDatabase.h"
#include "Speed/Indep/Src/Interfaces/SimActivities/ICopMgr.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Interfaces/Simables/IINput.h"
#include "Speed/Indep/Src/Interfaces/Simables/IAI.h"
#include "Speed/Indep/Src/Interfaces/Simables/IRigidBody.h"
#include "Speed/Indep/Src/Physics/Common/VehicleSystem.h"
#include "Speed/Indep/Src/World/ParameterMaps.hpp"
#include "Speed/Indep/Tools/Inc/ConversionUtil.hpp"
#include "Speed/Indep/Src/Generated/Messages/MControlPathfinder.h"
#include "Speed/Indep/Src/Generated/Messages/MGamePlayMoment.h"
#include "Speed/Indep/Src/Generated/Messages/MMiscSound.h"
#include "Speed/Indep/Src/Generated/Messages/MPerpBusted.h"
#include "Speed/Indep/Src/Generated/Messages/MRestartRace.h"
#include "Speed/Indep/Src/Generated/Messages/MUnspawnCop.h"
#include "Speed/Indep/Src/Generated/AttribSys/Classes/smackable.h"
#include "Speed/Indep/Src/Speech/SpeechCache.h"
#include "Speed/Indep/Src/World/CarInfo.hpp"

// En el arbol MiscSpeech es una CLASE con estaticas (asi mangla el original:
// SMSCellCall__10MiscSpeechi), no un namespace de funciones libres.
#include "Speed/Indep/Src/Speech/MiscSpeech.h"
#include "Speed/Indep/Src/Frontend/FEngHashes/ScriptHashes.hpp"

// gcdatos4 (receta gcdatos1-F): en el objetivo, SPEECHFLOW_DISPLAY
// (obj+0x1C6C) y heat_cutoffs (obj+0x1C70) van justo tras las cadenas de la
// fase de includes de este fichero (la ultima es Attrib::Gen::smackable) y
// delante del "Speech" del constructor. Estaban tras IsHeadingValid y
// caian desplazados en src+0x1DE0. Todas las lecturas de SPEECHFLOW_DISPLAY
// estan en ficheros anteriores de la TU (PursuitFlow, StrategyFlow,
// RoadblockFlow, Observer), asi que la definicion sigue detras de todos sus
// usos y GCC no puede plegar la lectura a un inmediato (la regla de la r31
// que motivaba la posicion vieja).
extern const int SPEECHFLOW_DISPLAY = 0;

const SoundAI::HeatCutoffs SoundAI::heat_cutoffs[4] = {
    {2.0f, Csis::Type_heat_level_2},
    {3.0f, Csis::Type_heat_level_3},
    {4.0f, Csis::Type_heat_level_4},
    {5.0f, Csis::Type_heat_level_5},
};

// .data:0x80435E9C y 0x80435EA0 del original, en este orden y justo detras de
// MUSICFLOW_DISPLAY. Los 276 UND de zSpeech son estos dos.
int SoundAI::mRefCount = 0;
template <> SoundAI *UTL::Collections::Singleton<SoundAI>::mInstance = NULL;

#ifdef EA_BUILD_A124
// gcdatos13: solo en la A124, entre SPEECHFLOW_DISPLAY y DESTROY_COPS_ON_INACTIVITY
// (0x62f758, PS2_globals.hpp:5511), a 0 en su .data objetivo. Sin simbolo en las demas.
int SPEECHFLOW_DISPLAY_DITCHED = 0;
#endif
static int DESTROY_COPS_ON_INACTIVITY = 0;

int FORCE_VOICE_RANDOMIZATION = 0;
extern ParameterAccessor SPAMAccessorSpeech;


struct ColourHashToSoundColour {
    unsigned int Hash;
    unsigned int SoundColour;
};

extern CarTypeInfo *CarTypeInfoArray;










UTL::COM::Factory<Sim::Param, Sim::IActivity, UCrc32>::Prototype _SoundAI(UCrc32("SoundAI"), SoundAI::Construct);
static float CopMinClosingVelSq = MPH2MPS(25.0f);

SoundAI::SoundAI()
    : Sim::Activity(1),
      mMainUpdate(AddTask("Speech", 0.1f, 0.0f, Sim::TASK_FRAME_FIXED)),
      mProcessObservations(AddTask("Comment", 0.25f, 0.0f, Sim::TASK_FRAME_FIXED)),
      mFlags(0),
      mActors(10),
      mUsage(),
      mDispatch(0),
      mLeader(0),
      mHeli(0),
      mCopsInFormation(),
      mDeadAir(0.0f),
      mLastCopInFormation(0),
      mLatestCop(0),
      mPlayerHeat(0),
      mPlayerSpeed(0.0f),
      mPlayerPos(UMath::Vector3::kZero),
      mSmoothedFWRoad(UMath::Vector3::kZero),
      mPursuit(0),
      mAIPursuit(0),
      mFocus(kPursuitFlow),
      mPursuitDist(0.0f),
      mT_PerpLastSeen(0.0f),
      mTrafficHits911(0),
      mCTS911(0),
      mHavoc(0),
      mPursuitCount(0),
      mNumRoadBlocks(0),
      mRacerCount(0),
#ifndef EA_BUILD_A124
      mTimeSinceLastChase(0.0f),
#endif
      mPVehicle(static_cast<const Attrib::Collection *>(0), 0, 0),
      mTune(static_cast<const Attrib::Collection *>(0), 0, 0),
      mPursuitLevel(static_cast<const Attrib::Collection *>(0), 0, 0),
      mPursuitState(kInactive),
      mQuadrantState(kReset),
      mRecentBlowby(),
      mInfraction(-1),
      mNumCopsInWave(0),
      mNumActiveCopCars(0),
      mPlayerOffroadID(-1),
      mCopsInView(0),
      mPursuitFlow(0),
      mStrategyFlow(0),
      mObserver(0),
      mRoadblockFlow(0),
      mMusicFlow(0),
      mT_outofFormation(0),
      mT_reallylowspeed(0),
      mT_noLOS(0),
      mT_LOS(0),
      mT_lastCrashed(0),
      mT_lastCopNailed(0),
      mT_pursuitStart(WorldTimer),
      mT_sinceLastPursuit(0),
      mPlayerCarCustom(0),
      mAICarCustom(0),
      mActorPool(0),
      mMsgPerpBusted(Hermes::Handler::Create<MPerpBusted, SoundAI, SoundAI>(
          this, &SoundAI::MessagePerpBusted, UCrc32(0x20D60DBF), 0)),
      mMsgAIPerpBusted(Hermes::Handler::Create<MPerpBusted, SoundAI, SoundAI>(
          this, &SoundAI::MessageAIPerpBusted, "AIRacerBusted", 0)),
      mMsgRestartRace(Hermes::Handler::Create<MRestartRace, SoundAI, SoundAI>(
          this, &SoundAI::MessageRestart, UCrc32(0x20D60DBF), 0)),
      mMsgInfraction(Hermes::Handler::Create<MMiscSound, SoundAI, SoundAI>(
          this, &SoundAI::MessageInfraction, "Infraction", 0)),
      mMsgUnspawnCop(Hermes::Handler::Create<MUnspawnCop, SoundAI, SoundAI>(
          this, &SoundAI::MessageUnspawnCop, "SoundAI", 0)),
      mMsgTireBlown(Hermes::Handler::Create<MGamePlayMoment, SoundAI, SoundAI>(
          this, &SoundAI::MessageTireBlown, "TireBlo", 0)) {

    SoundAI::mRefCount = 1;
    mCopsInFormation.clear();
    mRecentBlowby.Reset();

    Sim::ProfileTask(mMainUpdate, "Speech");
    Sim::ProfileTask(mProcessObservations, "Speech");

    mTune.ChangeWithDefault(0);
    mPursuitLevel.ChangeWithDefault(0);

    mActorPool = bNewSlotPool(0xA0, 10, "VoiceActors slotpool", AudioMemoryPool);
    mActorPool->ClearFlag(SLOTPOOL_FLAG_OVERFLOW_IF_FULL);
    mActorPool->ClearFlag(SLOTPOOL_FLAG_WARN_IF_NONEMPTY_DELETE);

    mDispatch = new EAXDispatch(1);
    mPursuitFlow = new ("SpeechFlow: Pursuit", 0) Speech::PursuitFlow;
    mStrategyFlow = new ("SpeechFlow: Strategy", 0) Speech::StrategyFlow;
    mObserver = new ("SpeechFlow: Observer", 0) Speech::Observer;
    mRoadblockFlow = new ("SpeechFlow: Roadblock", 0) Speech::RoadblockFlow;
    mMusicFlow = new ("MusicFlow: Interactive", 0) Speech::MusicFlow;

    for (int i = 0; i < 2; i++) {
        mPlayerCurrent[i].direction = CalcPlayerDirection(false);
        mAICurrent[i].direction = 0;
        mPlayerCurrent[i].roadID = mAICurrent[i].roadID = untagged;
    }

    mAILastKnown.direction = 0;
    mLastKnown.direction = 0;
    mAILastKnown.roadID = untagged;
    mLastKnown.roadID = untagged;

    for (int i = 3; i < 10; i++) {
        mUsage.voices.push_back(i);
    }
    for (unsigned int i = 0; i < mUsage.voices.size(); i++) {
        int rand = bRandom(static_cast<int>(mUsage.voices.size()));
        int rand_voice = mUsage.voices[rand];
        int curr_voice = mUsage.voices[i];
        if (rand_voice != curr_voice) {
            mUsage.voices[i] = rand_voice;
            mUsage.voices[rand] = curr_voice;
        }
    }

    Enable();
}

SoundAI::~SoundAI() {
    SoundAI::mRefCount = 0;
    RemoveTask(mMainUpdate);
    RemoveTask(mProcessObservations);

    Speech::copMap::iterator iter = mActors.begin();
    while (iter != mActors.end()) {
        EAXCop *cop = iter->cop;
        if (cop) {
            delete cop;
        }
        ++iter;
    }
    mActors.clear();
    mCopsInFormation.clear();

    if (mActorPool) {
        bDeleteSlotPool(mActorPool);
    }

    Sim::Collision::RemoveListener(this);
    mLeader = 0;
    mHeli = 0;

    if (mPursuitFlow) {
        delete mPursuitFlow;
    }
    mPursuitFlow = 0;
    if (mStrategyFlow) {
        delete mStrategyFlow;
    }
    mStrategyFlow = 0;
    if (mObserver) {
        delete mObserver;
    }
    mObserver = 0;
    if (mRoadblockFlow) {
        delete mRoadblockFlow;
    }
    mRoadblockFlow = 0;
    if (mMusicFlow) {
        delete mMusicFlow;
    }
    mMusicFlow = 0;

    if (mPlayerCarCustom) {
        delete mPlayerCarCustom;
    }
    mPlayerCarCustom = 0;
    if (mAICarCustom) {
        delete mAICarCustom;
    }
    mAICarCustom = 0;

    if (mMsgPerpBusted) {
        Hermes::Handler::Destroy(mMsgPerpBusted);
    }
    if (mMsgAIPerpBusted) {
        Hermes::Handler::Destroy(mMsgAIPerpBusted);
    }
    if (mMsgForcePursuitStart) {
        Hermes::Handler::Destroy(mMsgForcePursuitStart);
    }
    if (mMsgRestartRace) {
        Hermes::Handler::Destroy(mMsgRestartRace);
    }
    if (mMsgUnspawnCop) {
        Hermes::Handler::Destroy(mMsgUnspawnCop);
    }
    if (mMsgInfraction) {
        Hermes::Handler::Destroy(mMsgInfraction);
    }
    if (mMsgTireBlown) {
        Hermes::Handler::Destroy(mMsgTireBlown);
    }
}

void SoundAI::MessagePerpBusted(const MPerpBusted &) {
    mFocus = kTerminal;
    if (mMusicFlow) {
        MControlPathfinder(false, 0xF, 0, 0).Send(UCrc32("Event"));
    }
    mFlags |= BUSTED;
}

void SoundAI::MessageAIPerpBusted(const MPerpBusted &) {
    if (mPursuitState != kInactive) {
        EAXCop *cop = FindFurthestCop(true);
        if (cop) {
            cop->Arrest();
        }
    }
}

void SoundAI::MessageInfraction(const MMiscSound &message) {
    mInfraction = message.GetSoundID();
}

void SoundAI::MessageRestart(const MRestartRace &) {
    ResetPursuit(true);
    Speech::Module *cop_speech = Speech::Manager::GetSpeechModule(1);
    if (cop_speech) {
        cop_speech->ReleaseResource();
    }
    Speech::Manager::ClearPlayback();
}

void SoundAI::MessageUnspawnCop(const MUnspawnCop &message) {
    EAXCop *j = mActors.Find(message.GetCopHandle());
    if (j) {
        switch (message.GetParam()) {
        case 1:
        case 3:
        case 4:
        case 5:
        case 6:
            j->SetActive(false);
            break;
        case 0:
        case 2:
        default:
            RemoveCop(message.GetCopHandle());
            break;
        }
    }
}

void SoundAI::MessageTireBlown(const MGamePlayMoment &message) {
    if (mRoadblockFlow) {
        mRoadblockFlow->NailedSomethingInRB(0x10);
    }
#ifdef EA_BUILD_A124
    EAXCop *spkr = FindClosestCop(false, true);
#else
    EAXCop *spkr = FindClosestCop(true, true);
#endif
    if (mObserver && spkr) {
        mObserver->Observe(0xE, spkr->GetSpeakerID(), 1.0f);
    }
}

void SoundAI::OnVehicleAdded(IVehicle *ivehicle) {
    Sim::Collision::AddListener(this, ivehicle
#ifndef EA_BUILD_A124
        ,
        "SoundAI"
#endif
    );
}

void SoundAI::OnVehicleRemoved(IVehicle *ivehicle) {
    Sim::Collision::RemoveListener(this, ivehicle);
    EAXCop *cop = mObserver->GetRamCop();
    if (cop) {
        EAXCop *actor = mActors.Find(ivehicle->GetSimable()->GetOwnerHandle());
#ifdef EA_BUILD_A124
        if (actor && actor->GetSpeakerID() == cop->GetSpeakerID()) {
#else
        if (actor == cop) {
#endif
            mObserver->DetachRamCop();
        }
    }
}

void SoundAI::OnCollision(const COLLISION_INFO &cinfo) {
    if (UMath::LengthSquare(cinfo.closingVel) < CopMinClosingVelSq) {
        return;
    }

    short actors_involved = 0;
    short objects_visible = 0;
    IRenderable *renderA;
    IRenderable *renderB;
    EAXCop *actorA = nullptr;
    EAXCop *actorB = nullptr;
    ISimable *simableA;
    if (cinfo.objA != nullptr) {
        simableA = ISimable::FindInstance(cinfo.objA);
    } else {
        simableA = nullptr;
    }
    ISimable *simableB;
    if (cinfo.objB != nullptr) {
        simableB = ISimable::FindInstance(cinfo.objB);
    } else {
        simableB = nullptr;
    }

    if (simableA != nullptr) {
        actorA = this->mActors.Find(cinfo.objA);
        if (actorA != nullptr) {
            actors_involved++;
        }
        renderA = nullptr;
        UTL::COM::ValidatePtr(simableA);
        if (simableA->QueryInterface(&renderA) && renderA->InView()) {
            objects_visible++;
        }
    }
    if (simableB != nullptr) {
        actorB = this->mActors.Find(cinfo.objB);
        if (actorB != nullptr) {
            actors_involved++;
        }
        renderB = nullptr;
        UTL::COM::ValidatePtr(simableB);
        if (simableB->QueryInterface(&renderB) && renderB->InView()) {
            objects_visible++;
        }
    }
    if (simableA == nullptr && simableB == nullptr) {
        return;
    }

    float collisionspeed = cinfo.impulseA + cinfo.impulseB;
    float minspeed = MPH2MPS(this->mTune.PlayerSmashSpeedRange(0));
    float maxspeed = MPH2MPS(this->mTune.PlayerSmashSpeedRange(1));
    float intensity = UMath::Clamp((collisionspeed - minspeed) / (maxspeed - minspeed), 0.0f, 1.0f);

    if (cinfo.type != Sim::Collision::Info::OBJECT) {
        if (cinfo.type < Sim::Collision::Info::WORLD) {
            return;
        }
        switch (cinfo.type) {
            default:
                return;
            case Sim::Collision::Info::WORLD:
                break;
        }
    } else {
        if (actors_involved == 2) {
            return;
        }

        if (actors_involved == 1) {
            IVehicle *theOtherCar;
#ifdef EA_BUILD_A124
            // La A124 elige el otro por el policia: si A es el policia, el otro es B.
            ISimable *theOtherObj = actorA == nullptr ? simableA : simableB;
#else
            ISimable *theOtherObj = simableA != nullptr ? simableA : simableB;
#endif

            EAXCop *actor = actorA != nullptr ? actorA : actorB;

            if (!theOtherObj->QueryInterface(&theOtherCar)) {
                return;
            }

            switch (theOtherCar->GetDriverClass()) {
                case DRIVER_HUMAN: {
                    bool cop_is_in_rb = false;
                    bool cop_rammed = false;
                    bool cop_is_suv = false;
                    bool cop_is_braking = false;
                    IRigidBody *rbCop;
                    IRigidBody *rbRacer;
                    IVehicle *vehicleCop;
                    IInput *inputcop;
                    ISimable *simableCop;
                    if (theOtherObj == simableA) {
                        simableCop = simableB;
                    } else {
                        simableCop = simableA;
                    }
                    if (simableCop == nullptr) {
                        return;
                    }

                    inputcop = nullptr;
                    if (simableCop->QueryInterface(&inputcop)) {
                        float brake = UMath::Clamp(inputcop->GetControls().fBrake, 0.0f, 1.0f);
                        float ebrake = UMath::Clamp(inputcop->GetControls().fHandBrake, 0.0f, 1.0f);
                        if ((brake > 0.0f) || (ebrake > 0.0f)) {
                            cop_is_braking = true;
                        }
                    }

                    if ((this->mFocus == kPursuitFlow) && ((this->mFlags & SETUP_RESTARTED) == 0)) {
                        if (this->mPursuitFlow->GetPursuitCause() != Speech::PursuitFlow::kScripted) {
                            if ((this->mPursuitFlow->GetPursuitCause() != Speech::PursuitFlow::kCopAssaulted) &&
                                (this->mPursuitFlow->GetPursuitCause() != Speech::PursuitFlow::kCopAssaultedScripted) &&
                                this->mPursuitFlow->RequiresRestart()) {
                                Speech::Module *cop_speech = Speech::Manager::GetSpeechModule(COPSPEECH_MODULE);
                                if (cop_speech != nullptr) {
                                    Speech::Manager::ClearPlayback();
                                    cop_speech->ReleaseResource();
                                    this->mT_lastCopNailed = WorldTimer;
                                    actor->WasRammed();
                                    if (this->mPursuitFlow->GetPursuitCause() == Speech::PursuitFlow::kScripted) {
                                        this->mPursuitFlow->SetPursuitCause(Speech::PursuitFlow::kCopAssaultedScripted);
                                    } else {
                                        this->mPursuitFlow->SetPursuitCause(Speech::PursuitFlow::kCopAssaulted);
                                    }
                                    this->mPursuitFlow->ChangeStateTo(Speech::PursuitFlow::kPrimaryBranch);
                                    this->mPursuitFlow->Update();
                                    this->mFlags |= SETUP_RESTARTED;
                                }
                            }
                        }
                    }

                    IRoadBlock *block = this->GetRoadblock();
                    if (block != nullptr) {
                        IVehicle *car = block->IsComprisedOf(simableCop->GetOwnerHandle());
                        if (car != nullptr) {
                            cop_is_in_rb = true;
                            if (UTL::COM::ComparePtr(car, simableCop)) {
                                this->mRoadblockFlow->NailedSomethingInRB(0x40);
                            }
                        }
                    }
                    if (!cop_is_in_rb) {
                        this->mObserver->Observe(Speech::Collision_Cop_Suspect, -1, intensity);
                    }

                    vehicleCop = nullptr;
                    simableCop->QueryInterface(&vehicleCop);
                    if (vehicleCop != nullptr) {
                        unsigned int vtype = vehicleCop->GetVehicleAttributes().GetCollection();
                        if ((vtype == 0x38b38226) || (vtype == 0x54b10e38) || (vtype == 0x2e149eac)) {
                            cop_is_suv = true;
                        }
                    }

                    rbRacer = theOtherObj->GetRigidBody();
                    rbCop = simableCop->GetRigidBody();

                    UMath::Vector3 armCop = actorA == nullptr ? cinfo.armB : cinfo.armA;
                    UMath::Vector3 armRacer = actorA == nullptr ? cinfo.armA : cinfo.armB;
                    UMath::Vector3 fwCop;
                    UMath::Vector3 fwRacer;
                    UMath::Vector3 dimCop;
                    UMath::Vector3 dimRacer;
                    UMath::Vector3 velCop;
                    UMath::Vector3 velRacer;
                    UMath::Vector3 cnormal = cinfo.normal;
                    rbCop->GetForwardVector(fwCop);
                    rbCop->GetDimension(dimCop);
                    velCop = (simableCop->GetOwnerHandle() == cinfo.objA) ? cinfo.objAVel : cinfo.objBVel;
                    rbRacer->GetForwardVector(fwRacer);
                    rbRacer->GetDimension(dimRacer);
                    velRacer = (theOtherObj->GetOwnerHandle() == cinfo.objA) ? cinfo.objAVel : cinfo.objBVel;

                    float fwDot = VU0_v3dotprod(fwRacer, fwCop);
                    float vel_norm_dotCop = bAbs(VU0_v3dotprod(velCop, cnormal));
                    float vel_norm_dotRacer = bAbs(VU0_v3dotprod(velRacer, cnormal));
                    VehicleImpactType coll_type = kUnknown;

                    if (fwDot >= 0.7f) {
                        if ((bAbs(armCop.x) > dimCop.x * 0.75f) && (bAbs(armRacer.x) > dimRacer.x * 0.75f)) {
                            if (vel_norm_dotCop > vel_norm_dotRacer) {
                                coll_type = kCopSSPerp;
                            } else {
                                coll_type = kPerpSSCop;
                                cop_rammed = true;
                            }

                        } else {
                            if ((armCop.z >= dimCop.z * 0.75f) && (armRacer.z <= armRacer.z * -0.75f)) {
                                coll_type = kCopREperp;
                            }
                            if ((armRacer.z >= dimRacer.z * 0.75f) && (armCop.z <= dimCop.z * -0.75f)) {
                                if (VU0_v3length(velRacer) > VU0_v3length(velCop)) {
                                    cop_rammed = true;
                                }

                                coll_type = kPerpRECop;
                            }
                        }
                    } else if ((fwDot > -0.7f) && (fwDot < 0.7f)) {
                        if (armRacer.z >= dimRacer.z * 0.7f) {
                            cop_rammed = true;
                            coll_type = kPerpTBCop;
                        }
                        if (armCop.z >= dimCop.z * 0.7f) {
                            coll_type = kCopTBPerp;
                        }
                    } else if (fwDot <= -0.7f) {
                        if (vel_norm_dotRacer > vel_norm_dotCop) {
                            cop_rammed = true;
                            coll_type = kPerpHOCop;
                        } else {
                            coll_type = kCopHOPerp;
                        }
                    }

                    if (!cop_rammed) {
                        break;
                    }
                    this->mT_lastCopNailed = WorldTimer;
                    if (actor == nullptr) {
                        break;
                    }
                    if (cop_is_in_rb || cop_is_suv || cop_is_braking || ((this->mFlags & COPS_IMMUNE) != 0)) {
                        break;
                    }
                    actor->WasRammed();
                    if ((this->mFocus == kPursuitFlow) || (this->mFocus == kTerminal) || actor->IsDead() || !actor->IsActive()) {
                        break;
                    }

                    if (intensity > 0.15f) {
                        if (intensity <= 0.5f) {
                            if (coll_type == kPerpRECop) {
                                actor->RearEnded(Csis::Type_intensity_Normal);
                            } else if (coll_type == kPerpTBCop) {
                                actor->TBoned(Csis::Type_intensity_Normal);
                            } else if (coll_type == kPerpHOCop) {
                                actor->HeadOn(Csis::Type_intensity_Normal);
                            } else if (coll_type == kPerpSSCop) {
                                actor->SideSwiped(Csis::Type_intensity_Normal);
                            }
                            break;
                        }
                    }
                    if ((intensity > 0.5f) && (intensity <= 0.75f)) {
                        float rand_select = bRandom(1.0f);
                        if (rand_select < 0.67f) {
                            if (coll_type == kPerpRECop) {
                                actor->RearEnded(Csis::Type_intensity_High);
                            } else if (coll_type == kPerpTBCop) {
                                actor->TBoned(Csis::Type_intensity_High);
                            } else if ((coll_type == kPerpHOCop) && !cop_is_suv) {
                                actor->HeadOn(Csis::Type_intensity_High);
                            } else if (coll_type == kPerpSSCop) {
                                actor->SideSwiped(Csis::Type_intensity_High);
                            } else {
                                actor->InterruptExpletive();
                            }
                        } else {
                            actor->InterruptExpletive();
                        }
                    } else if (intensity > 0.75f) {
                        actor->InterruptViolent();
                    }
                    break;
                }

                case DRIVER_TRAFFIC:
                    this->mObserver->Observe(Speech::Collision_Cop_Traffic, -1, intensity);
                    actor->JustHitTraffic();
                    if ((this->mPursuitState == kActive) && (this->mFocus == kStrategyFlow) && actor->IsActive()) {
                        actor->BailoutTraffic();
                        this->RandomBailoutDeny(actor);
                    }
                    break;

                default:
                    break;
            }
        } else {
            IPlayer *player = IPlayer::First(PLAYER_LOCAL);
            IVehicle *pvehicle = nullptr;
            if (player != nullptr) {
                if (UTL::COM::ComparePtr(player->GetSimable(), simableA)) {
                    simableA->QueryInterface(&pvehicle);
                } else if (UTL::COM::ComparePtr(player->GetSimable(), simableB)) {
                    simableB->QueryInterface(&pvehicle);
                }
            }
            if (pvehicle != nullptr) {
                ISimable *otherObj = simableA;
                if (pvehicle->GetSimable()->GetOwnerHandle() == cinfo.objA) {
                    otherObj = simableB;
                }
                IModel *model;
                if (otherObj != nullptr) {
                    model = otherObj->GetModel();
                } else {
                    model = nullptr;
                }
                IVehicle *otherVehicle = nullptr;
                if (otherObj != nullptr) {
                    otherObj->QueryInterface(&otherVehicle);
                }

                float collisionspeed = cinfo.impulseA + cinfo.impulseB;
                float minspeed = MPH2MPS(this->mTune.PlayerSmashSpeedRange(0));
                float maxspeed = MPH2MPS(this->mTune.PlayerSmashSpeedRange(1));
                float intensity = UMath::Clamp((collisionspeed - minspeed) / (maxspeed - minspeed), 0.0f, 1.0f);

                const UMath::Vector3 &player_vel = (pvehicle->GetSimable()->GetOwnerHandle() == cinfo.objA) ? cinfo.objAVel : cinfo.objBVel;
                float speed_b4_impact = UMath::Length(player_vel);
                float curr_speed = pvehicle->GetSimable()->GetRigidBody()->GetSpeed();
                float pct_decrease;
                if (speed_b4_impact > 0.0f) {
                    pct_decrease = curr_speed / speed_b4_impact;
                } else {
                    pct_decrease = 1.0f;
                }
                if (pct_decrease < (1.0f - this->mTune.CrashSlowdownPct())) {
                    this->mT_lastCrashed = WorldTimer;
                }

                if (model != nullptr) {
                    unsigned int model_atr = model->GetAttributes().GetCollection();
                    if (model->IsRootModel() && (this->mPursuitState == kInactive)) {
                        Attrib::Gen::smackable obj_atr(model->GetAttributes());
                        int cost_to_state = obj_atr.COST_TO_STATE();
                        this->mCTS911 += cost_to_state;
                    }
                }

                if (this->GetRoadblock() != nullptr) {
                    const IRoadBlock::Smackables &objects = this->GetRoadblock()->GetSmackables();
                    if (objects.size() > 0) {
                        for (IRoadBlock::Smackables::const_iterator i = objects.begin(); i != objects.end(); ++i) {
                            IPlaceableScenery *object = *i;
                            if (UTL::COM::ComparePtr(object, model) && model != nullptr) {
                                if (model->GetAttributes().GetCollection() == 0xca89ef8f) {
                                    this->mRoadblockFlow->NailedSomethingInRB(0x10);
                                    EAXCop *spkr = this->FindClosestCop(false, true);
                                    this->mObserver->Observe(Speech::Collision_Suspect_Spikebelt, spkr != nullptr ? spkr->GetSpeakerID() : -1,
                                                             intensity);
                                } else {
                                    this->mRoadblockFlow->NailedSomethingInRB(0x20);
                                }
                            }
                        }
                    }
                }

                if (otherVehicle != nullptr) {
                    switch (otherVehicle->GetDriverClass()) {
                        case DRIVER_HUMAN:
                            this->mObserver->Observe(Speech::Collision_Suspect_Suspect, -1, intensity);
                            break;
                        case DRIVER_TRAFFIC:
                        case DRIVER_NONE: {
                            this->mObserver->Observe(Speech::Collision_Suspect_Traffic, -1, intensity);
                            {
#ifdef EA_BUILD_A124
                                EAXCop *spkr = this->FindClosestCop(false, true);
#else
                                EAXCop *spkr = this->FindClosestCop(true, true);
#endif
                                int spkrID = -1;

                                if (spkr != nullptr) {
                                    spkrID = spkr->GetSpeakerID();
                                }
                                if ((otherVehicle->GetVehicleClass() == VehicleClass::TRACTOR) ||
                                    (otherVehicle->GetVehicleClass() == VehicleClass::TRAILER)) {
#ifdef EA_BUILD_A124
                                    this->mObserver->Observe(Speech::Collision_Suspect_Semi, spkrID, intensity);
#else
                                    if (intensity >= this->mTune.MinIntensityTrafficSmash()) {
                                        this->mObserver->Observe(Speech::Collision_Suspect_Semi, spkrID, intensity);
                                    }
#endif
                                } else {

                                    this->mObserver->Observe(Speech::Collision_Suspect_Traffic, spkrID, intensity);
                                }
                            }
                            if (intensity >= this->mTune.MinIntensityTrafficSmash()) {
                                this->mTrafficHits911++;
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
        }
        return;
    }
    {
        ISimable *isimable = simableA;
        IVehicle *ivehicle;
        float collisionspeed = cinfo.impulseA + cinfo.impulseB;
        float minspeed = MPH2MPS(this->mTune.PlayerSmashSpeedRange(0));
        float maxspeed = MPH2MPS(this->mTune.PlayerSmashSpeedRange(1));
        float intensity = UMath::Clamp((collisionspeed - minspeed) / (maxspeed - minspeed), 0.0f, 1.0f);
        if (isimable->QueryInterface(&ivehicle)) {
            switch (ivehicle->GetDriverClass()) {
                case DRIVER_HUMAN: {
                    {
                        const UMath::Vector3 &player_vel = cinfo.objAVel;
                        float speed_b4_impact = UMath::Length(player_vel);
                        float curr_speed = isimable->GetRigidBody()->GetSpeed();
                        float pct_decrease;
                        if (speed_b4_impact > 0.0f) {
                            pct_decrease = curr_speed / speed_b4_impact;
                        } else {
                            pct_decrease = 1.0f;
                        }
                        if (pct_decrease < (1.0f - this->mTune.CrashSlowdownPct())) {
                            this->mT_lastCrashed = WorldTimer;
                            if ((this->mPursuitState == kActive) && (this->mFocus == kStrategyFlow)) {
#ifdef EA_BUILD_A124
                                EAXCop *spkr = this->GetRandomActiveCop(0, false);
#else
                                EAXCop *spkr = this->GetRandomActiveCop(0, true);
#endif

                                if (spkr != nullptr) {
                                    this->mObserver->Observe(Speech::Collision_Suspect_World, spkr->GetSpeakerID(), intensity);
                                }
                            }
                        }
                    }
                    return;

                }
                case DRIVER_COP:
                    break;
                default:
                    return;
            }

            {
                EAXCop *actor = actorA != nullptr ? actorA : actorB;
                if (actor == nullptr || (objects_visible <= 0) || !actor->IsActive()) {
                    return;
                }
                if (intensity >= this->mTune.MinIntensityCopSmash()) {
                    if (this->mObserver != nullptr && this->mObserver->WeatherExists()) {
                        actor->BailoutBadRoad();
                    } else {
                        actor->Bailout();
                    }
                }
                if (actor->GetInFormation() && (this->mCopsInFormation.size() > 1)) {
                    this->RandomBailoutDeny(actor);
                }
                this->mObserver->Observe(Speech::Collision_Cop_World, actor != nullptr ? actor->GetSpeakerID() : -1, intensity);
            }
        }
    }
}

EAXCop *SoundAI::GetCopInRB() {
    IRoadBlock *block = GetRoadblock();

    if (block) {
        Speech::copMap::const_iterator iter = mActors.begin();
        while (iter != mActors.end()) {
            EAXCop *cop = iter->cop;
            if (cop->IsActive() && !cop->IsPrimary()) {
                IVehicle *car = block->IsComprisedOf(cop->GetHandle());
                if (car) {
                    return cop;
                }
            }
            ++iter;
        }
    }
    return 0;
}

EAXCop *SoundAI::GetRandomActiveCop(int type, bool reqLOS) {
    Speech::copList active;
    active.reserve(mActors.size());

    Speech::copMap::iterator iter = mActors.begin();
    while (iter != mActors.end()) {
        EAXCop *cop = iter->cop;
        if (cop->IsActive() && (!reqLOS || cop->HasLOS())) {
            active.push_back(cop);
        }
        ++iter;
    }

    if (active.empty()) {
        return 0;
    }

    EAXCop *spkr = 0;
    if ((type == 0) && (active.size() > 1)) {
        spkr = active[bRandom(static_cast<int>(active.size()))];
    } else if (active.size() == 1) {
        switch (type) {
        case 0:
            spkr = active[0];
            break;
        case 1:
            if (active[0]->IsPrimary()) {
                spkr = active[0];
            }
            break;
        case 2:
            if (!active[0]->IsPrimary()) {
                spkr = active[0];
            }
            break;
        default:
            break;
        }
    } else {
        switch (type) {
        case 1: {
        UTL::Std::vector<EAXCop *, _type_vector> primaries;
        primaries.reserve(active.size());

        Speech::copList::iterator i = active.begin();
        while (i != active.end()) {
            if ((*i)->IsPrimary()) {
                primaries.push_back(*i);
            }
            ++i;
        }

        if (!primaries.empty()) {
            spkr = primaries[bRandom(static_cast<int>(primaries.size()))];
        }
        primaries.clear();
        break;
        }
        case 2: {
        UTL::Std::vector<EAXCop *, _type_vector> secondaries;
        secondaries.reserve(active.size());

        Speech::copList::iterator i = active.begin();
        while (i != active.end()) {
            if (!(*i)->IsPrimary()) {
                secondaries.push_back(*i);
            }
            ++i;
        }

        if (!secondaries.empty()) {
            spkr = secondaries[bRandom(static_cast<int>(secondaries.size()))];
        }
        secondaries.clear();
        break;
        }
        default:
            break;
        }
    }

    active.clear();
    return spkr;
}

EAXCop *SoundAI::GetRandomCop(int type) {
    EAXCop *spkr = 0;

    if ((type == 0) && (mActors.size() > 1)) {
        spkr = mActors[bRandom(static_cast<int>(mActors.size()))].cop;
    } else {
        if (mActors.size() == 1) {
            switch (type) {
            case 1:
                if (!mActors[0].cop->IsPrimary()) {
                    break;
                }
                spkr = mActors[0].cop;
                break;
            case 2:
                if (mActors[0].cop->IsPrimary()) {
                    break;
                }
                spkr = mActors[0].cop;
                break;
            case 0:
                spkr = mActors[0].cop;
                break;
            default:
                break;
            }
        } else {
            switch (type) {
            case 1: {
            UTL::Std::vector<EAXCop *, _type_vector> primaries;
            primaries.reserve(mActors.size());

            Speech::copMap::iterator i = mActors.begin();
            while (i != mActors.end()) {
                if (i->cop->IsPrimary()) {
                    primaries.push_back(i->cop);
                }
                ++i;
            }

            if (!primaries.empty()) {
                spkr = primaries[bRandom(static_cast<int>(primaries.size()))];
            }
            primaries.clear();
            break;
            }
            case 2: {
            UTL::Std::vector<EAXCop *, _type_vector> secondaries;
            secondaries.reserve(mActors.size());

            Speech::copMap::iterator i = mActors.begin();
            while (i != mActors.end()) {
                if (!i->cop->IsPrimary()) {
                    secondaries.push_back(i->cop);
                }
                ++i;
            }

            if (!secondaries.empty()) {
                spkr = secondaries[bRandom(static_cast<int>(secondaries.size()))];
            }
            secondaries.clear();
            break;
            }
            default:
                break;
            }
        }
    }

    return spkr;
}

void SoundAI::OnAttached(IAttachable *pOther) {
    bool attached;
    IVehicle *ivehicle = 0;

    attached = pOther->QueryInterface(&ivehicle);
    if (attached) {
        OnVehicleAdded(ivehicle);
    }
}

void SoundAI::OnDetached(IAttachable *pOther) {
    IVehicle *ivehicle = 0;

    if (pOther->QueryInterface(&ivehicle)) {
        OnVehicleRemoved(ivehicle);
    }

    if (UTL::COM::ComparePtr(mPursuit, pOther)) {
        mPursuit = 0;
        mT_pursuitStart = WorldTimer;
        mPursuitState = mAIPursuit ? kOtherTarget : kInactive;
    }

    if (UTL::COM::ComparePtr(mAIPursuit, pOther)) {
        mAIPursuit = 0;
        if ((mPursuitState == kOtherTarget) && !mPursuit) {
            mPursuitState = kInactive;
        }
    }

    Sim::Activity::OnDetached(pOther);
}

Sim::IActivity *SoundAI::Construct(Sim::Param params) {
    if ((IsSoundEnabled == 0) || Sim::IsSplitScreen()) {
        return nullptr;
    }

    SoundAI *result;
    result = UTL::Collections::Singleton<SoundAI>::Get();
    if (result) {
        SoundAI::mRefCount = SoundAI::mRefCount + 1;
        return static_cast<Sim::IActivity *>(result);
    }

    result = new SoundAI;
    return static_cast<Sim::IActivity *>(result);
}

IRoadBlock *SoundAI::GetRoadblock() {
    if (mPursuit) {
        return mPursuit->GetRoadBlock();
    }

    {
        const UTL::Collections::Listable<IRoadBlock, 8>::List &blocks = UTL::Collections::Listable<IRoadBlock, 8>::GetList();
        {
            UTL::Collections::Listable<IRoadBlock, 8>::List::const_iterator i = blocks.begin();
            while (i != blocks.end()) {
                IRoadBlock *rb = *i;
                if (rb) {
                    IPursuit *pursuit = rb->GetPursuit();
                    if (pursuit && pursuit->IsPlayerPursuit()) {
                        return rb;
                    }
                }
                ++i;
            }
        }
    }
    return 0;
}

bool SoundAI::IsMusicActive() {
    if (mMusicFlow == nullptr) {
        return false;
    }
    return mMusicFlow->GetState() != -1 ? true : false;
}

bool SoundAI::OnTask(HSIMTASK htask, float dT) {
    ProfileNode profile_node;
    float tout = (WorldTimer - WorldTimer).GetSeconds();

    mDeadAir = Speech::Manager::IsCopSpeechBusy()
                   ? 0.0f
                   : (WorldTimer - Speech::Manager::GetTimeSinceLastEvent(COPSPEECH_MODULE)).GetSeconds();

    if (htask == mMainUpdate) {
        if (FORCE_VOICE_RANDOMIZATION != 0) {
            ForceGlobalVoiceChange();
            FORCE_VOICE_RANDOMIZATION = 0;
        }
        if ((mFlags & BUSTED) == 0) {
            SyncPursuit();
            SyncCarsToActors();
            SyncPlayers();
            SyncFormations();
            ShuffleActors();
            UpdateStateMachines();
            if ((IsSpeechEnabled != 0) && (mDeadAir > 0.0f)) {
                DealWithDeadAir();
            }
        }
    }

    if (htask == mProcessObservations) {
        if ((mFlags & BUSTED) == 0) {
            mObserver->Update();
            mRoadblockFlow->Update();
        }
        Speech::Manager::Deduce();
    }
    return true;
}

void SoundAI::DealWithDeadAir() {
    if (mPursuitState != kActive) {
        return;
    }
    if (!mLeader) {
        return;
    }
    if (mFocus == kPursuitFlow) {
        return;
    }

    if (!Speech::Manager::IsQueued(static_cast<SPCHType_1_EventID>(0x63), 4) && (mPursuitDuration > 60.0f)) {
        mDispatch->PursuitUpdate(mLeader);
    }

    if (GetPerpLostTime() >= mTune.NoLOSCommentaryTime()) {
        if (!mHeli) {
            mLeader->LostVisual();
        } else {
            if (mHeli->HasLOS()) {
                if (!IsHeadingValid() || (mPlayerOffroadID < 0) || (bRandom(1.0f) > 0.5f)) {
                    mHeli->PursuitUpdateReply();
                } else {
                    mHeli->LocationReport();
                }
            } else {
                mHeli->LostVisual();
            }
        }
    } else {
        if (!Speech::Manager::IsQueued(static_cast<SPCHType_1_EventID>(0x64), 4) &&
            !Speech::Manager::IsQueued(static_cast<SPCHType_1_EventID>(0x9e), 4) && (mPursuitDuration > 60.0f)) {
            if (!IsHeadingValid() || (mPlayerOffroadID < 0) || (bRandom(1.0f) > 0.5f)) {
                mLeader->PursuitUpdateReply();
            } else {
                mLeader->LocationReport();
            }
        }
    }

    if (Speech::Manager::IsQueued(static_cast<SPCHType_1_EventID>(0x9e), 4) &&
        Speech::Manager::HasBeenSaid(static_cast<SPCHType_1_EventID>(0x63))) {
        mLeader->PursuitUpdateReply();
    }
}

void SoundAI::UpdateStateMachines() {
    if ((IsSpeechEnabled != 0) && ((mFlags & BUSTED) == 0)) {
        switch (mFocus) {
        case kPursuitFlow:
            mPursuitFlow->Update();
            if (mPursuitFlow->IsTransitionable()) {
                mFocus = kStrategyFlow;
                mStrategyFlow->ChangeStateTo(0);
            }
            break;

        case kRoadblockFlow:
        case kStrategyFlow:
            if (mStrategyFlow->IsTransitionable()) {
                mStrategyFlow->ChangeStateTo(kWaiting);
            }
            mStrategyFlow->Update();
            break;

        case kLost:
            if ((mPursuit != 0) && (mPursuitState == kSearching)) {
                if (mStrategyFlow->GetState() != kOtherTarget) {
                    mStrategyFlow->ChangeStateTo(kOtherTarget);
                }
                mStrategyFlow->Update();
            }
            break;

#ifdef EA_BUILD_A124
        case kTerminal:
        default:
            if (mPursuit == 0) {
                ResetPursuit(false);
            }
            break;
#else
        case kTerminal:
            break;

        default:
            break;
#endif
        }
    }

    mMusicFlow->Update();
}

void SoundAI::AttemptReattachPursuit() {
    short playerfound = mPursuit && mPursuit->IsPlayerPursuit();
    short aifound = mAIPursuit && !mAIPursuit->IsPlayerPursuit();

    const UTL::Collections::Listable<IPursuit, 8>::List &pursuits = UTL::Collections::Listable<IPursuit, 8>::GetList();
    if (pursuits.size() > 0) {
        UTL::Collections::Listable<IPursuit, 8>::List::const_iterator start = pursuits.begin();
        if (!playerfound) {
            UTL::Collections::Listable<IPursuit, 8>::List::const_iterator i = start;
            while (i != pursuits.end()) {
                IPursuit *pursuit = *i;
                if (pursuit->IsPlayerPursuit() && !playerfound) {
                    playerfound = true;
                    if (mPursuit != pursuit) {
                        mPursuitCount++;
                        if (!mPursuit) {
                            mT_pursuitStart = WorldTimer;
                        } else {
                            if (IsAttached(mPursuit)) {
                                Detach(mPursuit);
                            }
                            mPursuit = 0;
                        }
                        if (!IsAttached(pursuit)) {
                            Attach(pursuit);
                        }
                        mPursuit = pursuit;
                    }
                } else if (!aifound) {
                    aifound = true;
                    if (mAIPursuit != pursuit) {
                        if (mAIPursuit) {
                            if (IsAttached(mAIPursuit)) {
                                Detach(mAIPursuit);
                            }
                            mAIPursuit = 0;
                        }
                        if (!IsAttached(pursuit)) {
                            if (Attach(pursuit)) {
                                mAIPursuit = pursuit;
                            }
                        }
                        mAIPursuit = pursuit;
                    }
                }
                ++i;
            }
        }
    }

    if (!playerfound) {
        mPursuit = 0;
        mT_pursuitStart = WorldTimer;
        mPursuitState = kInactive;
    }

    if (!aifound) {
        mAIPursuit = 0;
        if ((mPursuitState == kOtherTarget) && !mPursuit) {
            mPursuitState = kInactive;
        }
    }
}

#ifdef EA_BUILD_A124
// La A124 es otra version: sin helicoptero en la linea de vision, sin PURSUIT_EXPIRED (no existe en su enum), con
// t_lost calculado tras actualizar los temporizadores y quitando los policias al expirar la persecucion.
void SoundAI::SyncPursuit() {
    if (!mPursuit || !mPursuit->IsPlayerPursuit() || !mAIPursuit || mAIPursuit->IsPlayerPursuit()) {
        AttemptReattachPursuit();
    }

    if (!mPursuit) {
        if ((mPursuitState != kOtherTarget) && (mPursuitState != kInactive)) {
            if (mAIPursuit && (mRacerCount > 0)) {
                mPursuitState = kOtherTarget;
            } else {
                mT_sinceLastPursuit = WorldTimer;
                mPursuitState = kInactive;
            }
        }
    } else {
        bool ai_searching = false;
        if (mPursuit->AttemptingToReAquire() || !mPursuit->IsPerpInSight() || (mPursuit->GetPursuitStatus() == PS_COOL_DOWN)) {
            ai_searching = true;
        }

        if (ai_searching || (NumCopsWithLOS() == 0)) {
            mPursuitState = kSearching;
        } else if ((NumCopsWithLOS() > 0) || GetRoadblock()) {
            mPursuitState = kActive;
        }
    }

    if (mPursuitState == kActive) {
        mT_noLOS = mT_sinceLastPursuit = WorldTimer;
    } else {
        mT_LOS = WorldTimer;
    }

    float inactivity_cutoff[2];
    inactivity_cutoff[0] = mTune.PursuitInactivityTimer(0) + mTune.TimeConsideredLostNoLOS();
    inactivity_cutoff[1] = mTune.PursuitInactivityTimer(1) + mTune.TimeConsideredLostNoLOS();
    float t_lost = GetPerpLostTime();

    if ((mFocus == kLost) && (mPursuitState == kActive) && (t_lost < inactivity_cutoff[1])) {
        mFocus = kPursuitFlow;
        mQuadrantState = kReset;
        mFlags &= ~SETUP_RESTARTED;
        Speech::Manager::ClearPlayback();
        mPursuitFlow->Reacquire();
        mMusicFlow->Reacquire();
    }

    if ((mFocus == kStrategyFlow) && ((mPursuitState == kSearching) || (mPursuitState == kInactive)) &&
        (t_lost > inactivity_cutoff[0]) &&
        (!mPursuit || ((mPursuit->GetPursuitStatus() == PS_COOL_DOWN) && (mPursuit->GetCoolDownTimeRemaining() > 0.0f)))) {
        TerminatePursuit(kOutrunBail);
    } else if ((mFocus == kLost) && ((mPursuitState == kSearching) || (mPursuitState == kInactive)) &&
               (t_lost > inactivity_cutoff[0]) && !Speech::Manager::IsCopSpeechBusy() &&
               (!mPursuit || ((mPursuit->GetPursuitStatus() == PS_COOL_DOWN) && (mPursuit->GetCoolDownTimeRemaining() > 0.0f)))) {
        switch (mQuadrantState) {
        case kInitial:
            MiscSpeech::QuadrantForming();
            mQuadrantState = kForming;
            break;
        case kForming:
            if (bRandom(1.0f) > 0.5f) {
                mQuadrantState = kFiction2;
                MiscSpeech::PossibleSuspect();
            } else {
                mQuadrantState = kFiction1;
                int rand = bRandom(3);
                if (rand != 1) {
                    if (rand < 2) {
                        if (rand == 0) {
                            MiscSpeech::SuspectPossiblyGone();
                            break;
                        }
                    }
                } else {
                    MiscSpeech::QuadrantMoving();
                    break;
                }
                MiscSpeech::OtherLead();
            }
            break;
        case kFiction2:
            MiscSpeech::WrongSuspect();
            mQuadrantState = kExpired;
            break;
        case kFiction1:
        default:
            mQuadrantState = kExpired;
            break;
        }
    } else if ((mFocus == kLost) && ((mPursuitState == kSearching) || (mPursuitState == kInactive)) &&
               (!mPursuit || ((mPursuit->GetPursuitStatus() == PS_COOL_DOWN) && (mPursuit->GetCoolDownTimeRemaining() == 0.0f)) ||
                (mPursuit->GetPursuitStatus() == PS_EVADED)) &&
               (mQuadrantState == kExpired) && ((mFlags & DISP911_ACTIVE) == 0)) {
        Speech::Manager::ClearPlayback();
        mDispatch->TimeExpired();
        mQuadrantState = kReset;
    } else if ((mFocus == kLost) && ((mPursuitState == kSearching) || (mPursuitState == kInactive)) &&
               (t_lost > inactivity_cutoff[1]) && (mQuadrantState == kReset)) {
        ResetPursuit(false);
        if (mActors.size() > 0) {
            Speech::copMap::iterator iter = mActors.begin();
            while (mActors.size() > 0) {
                RemoveCop(iter->hsimable);
            }
        }
    }

    if (mFocus == kTerminal) {
        mPursuitState = kInactive;
    }

    mPursuitDuration = ((static_cast<unsigned int>(mPursuitState) < static_cast<unsigned int>(kInactive)) && mPursuit)
                           ? (WorldTimer - mT_pursuitStart).GetSeconds()
                           : -1.0f;

    if (((mFlags & HELI_INTRO_REQ) != 0) && mHeli && (mFocus == kStrategyFlow) && (mPursuitState != kInactive)) {
        mHeli->BackupArrives();
        mFlags &= ~HELI_INTRO_REQ;
    }
}
#else
void SoundAI::SyncPursuit() {
    if (!mPursuit || !mPursuit->IsPlayerPursuit() || !mAIPursuit || mAIPursuit->IsPlayerPursuit()) {
        AttemptReattachPursuit();
    }

    if (!mPursuit) {
        if ((mPursuitState != kOtherTarget) && (mPursuitState != kInactive)) {
            if (mAIPursuit && (mRacerCount > 0)) {
                mPursuitState = kOtherTarget;
            } else {
                mT_sinceLastPursuit = WorldTimer;
                mPursuitState = kInactive;
            }
        }
    } else {
        bool ai_searching = false;
        if (mPursuit->AttemptingToReAquire() || !mPursuit->IsPerpInSight() || (mPursuit->GetPursuitStatus() == PS_COOL_DOWN)) {
            ai_searching = true;
        }

        bool heli_LOS;
        if (mHeli && mHeli->IsActive()) {
            heli_LOS = mHeli->HasLOS();
        } else {
            heli_LOS = false;
        }

        if (ai_searching || ((NumCopsWithLOS() == 0) && !heli_LOS)) {
            mPursuitState = kSearching;
        } else if (((!ai_searching) && ((NumCopsWithLOS() > 0) || (heli_LOS == true))) || GetRoadblock()) {
#ifndef EA_BUILD_A124
            if (mPursuitState != kActive) {
                mTimeSinceLastChase = (WorldTimer - mT_sinceLastPursuit).GetSeconds();
            }
#endif
            mPursuitState = kActive;
        }

        if (static_cast<unsigned int>(mPursuitState) <= static_cast<unsigned int>(kSearching)) {
            mTrafficHits911 = 0;
            mCTS911 = 0;
        }
    }

    float t_lost = GetPerpLostTime();
    if (mPursuitState == kActive) {
        mT_noLOS = mT_sinceLastPursuit = WorldTimer;
    } else {
        mT_LOS = WorldTimer;
    }

    float inactivity_cutoff[2];
    inactivity_cutoff[0] = mTune.PursuitInactivityTimer(0) + mTune.TimeConsideredLostNoLOS();
    inactivity_cutoff[1] = mTune.PursuitInactivityTimer(1) + mTune.TimeConsideredLostNoLOS();

    if ((mFocus == kLost) && (mPursuitState == kActive) && (GetTimeSinceLastChase() < inactivity_cutoff[1])) {
        mFocus = kPursuitFlow;
        mQuadrantState = kReset;
        mFlags &= ~SETUP_RESTARTED;
        mFlags &= ~PURSUIT_EXPIRED;
        Speech::Manager::ClearPlayback();
        mPursuitFlow->Reacquire();
        mMusicFlow->Reacquire();
    }

    if ((mFocus == kStrategyFlow) && ((mPursuitState == kSearching) || (mPursuitState == kInactive)) &&
        (t_lost > inactivity_cutoff[0]) &&
        (!mPursuit || ((mPursuit->GetPursuitStatus() == PS_COOL_DOWN) && (mPursuit->GetCoolDownTimeRemaining() > 0.0f)))) {
        TerminatePursuit(kOutrunBail);
    }

    else if (mFocus == kLost) {
        if (((mPursuitState == kSearching) || (mPursuitState == kInactive)) && (t_lost > inactivity_cutoff[0]) &&
            (t_lost < inactivity_cutoff[1]) &&
            !Speech::Manager::IsCopSpeechBusy() &&
            (!mPursuit || ((mPursuit->GetPursuitStatus() == PS_COOL_DOWN) && (mPursuit->GetCoolDownTimeRemaining() > 0.0f)))) {
            switch (mQuadrantState) {
            case kInitial:
                MiscSpeech::QuadrantForming();
                mQuadrantState = kForming;
                break;
            case kForming:
                if (bRandom(1.0f) > 0.5f) {
                    mQuadrantState = kFiction2;
                    MiscSpeech::PossibleSuspect();
                } else {
                    mQuadrantState = kFiction1;
                    int rand = bRandom(3);
                    if (rand != 1) {
                        if (rand < 2) {
                            if (rand == 0) {
                                MiscSpeech::SuspectPossiblyGone();
                                break;
                            }
                        }
                    } else {
                        MiscSpeech::QuadrantMoving();
                        break;
                    }
                    MiscSpeech::OtherLead();
                }
                break;
            case kFiction2:
                MiscSpeech::WrongSuspect();
                mQuadrantState = kExpired;
                break;
            case kFiction1:
            default:
                mQuadrantState = kExpired;
                break;
            }
        } else if ((mFocus == kLost) && ((mPursuitState == kSearching) || (mPursuitState == kInactive)) &&
                   (!mPursuit || ((mPursuit->GetPursuitStatus() == PS_COOL_DOWN) &&
                                  (mPursuit->GetCoolDownTimeRemaining() == 0.0f)))) {
            if ((mQuadrantState == kExpired) &&
                (!mPursuit || (mPursuit->GetPursuitStatus() == PS_EVADED)) && ((mFlags & PURSUIT_EXPIRED) == 0)) {
                Speech::Manager::ClearPlayback();
                mDispatch->TimeExpired();
                mQuadrantState = kReset;
                mFlags |= PURSUIT_EXPIRED;
            } else if (t_lost > inactivity_cutoff[1]) {
                ResetPursuit(false);
            }
        }
    }

    if (mFocus == kTerminal) {
        mPursuitState = kInactive;
    }

    mPursuitDuration = ((static_cast<unsigned int>(mPursuitState) < static_cast<unsigned int>(kInactive)) && mPursuit)
                           ? (WorldTimer - mT_pursuitStart).GetSeconds()
                           : -1.0f;

    if (((mFlags & HELI_INTRO_REQ) != 0) && mHeli && (mFocus == kStrategyFlow) && (mPursuitState != kInactive)) {
        mHeli->BackupArrives();
        mFlags &= ~HELI_INTRO_REQ;
    }
}
#endif

void SoundAI::TerminatePursuit(BailoutType type) {
    int t = type;
    if ((t != kOutrunBail) && (type == kForcedBail)) {
        mMusicFlow->ChangeStateTo(kTerminal);

        bool is_DDay = GRaceStatus::Get().GetRaceParameters() ? GRaceStatus::Get().GetRaceParameters()->GetIsDDayRace() : false;

        if (!is_DDay) {
            Speech::Module *cop_speech = Speech::Manager::GetSpeechModule(1);
            if (cop_speech) {
                cop_speech->ReleaseResource();
            }
            Speech::Manager::ClearPlayback();
        }

        EAXCop *bailer = GetRandomActiveCop(0, false);
#ifdef EA_BUILD_A124
        if (bailer) {
            bailer->Bailout();
        }
#else
        if (bailer) {
            MiscSpeech::Bailout(bailer->GetSpeakerID());
        } else {
            MiscSpeech::Bailout(0);
        }
#endif
        mFocus = kTerminal;
    } else {
        bool cops_visible = false;
        IVehicle::List::const_iterator i = IVehicle::GetList(VEHICLE_AICOPS).begin();
        while (i != IVehicle::GetList(VEHICLE_AICOPS).end()) {
            IVehicle *vehicle = *i;
            IRenderable *renderable = 0;
            if (vehicle->QueryInterface(&renderable) && renderable->InView()) {
                cops_visible = true;
            }
            ++i;
        }
        Speech::Manager::ClearPlayback();
        if (!cops_visible) {
            MiscSpeech::LostSuspect(static_cast<int>(Speech::Manager::mLastSpeakerID));
        }
        mDispatch->BreakAway();
        mFocus = kLost;
        mQuadrantState = kInitial;
        mFlags &= ~SETUP_RESTARTED;
    }
}

void SoundAI::ResetPursuit(bool including_music) {
    mPursuitFlow->Reset();
    mStrategyFlow->Reset();
    mRoadblockFlow->Reset();
    mObserver->Reset();
    if (including_music) {
        mMusicFlow->Reset();
    }
    Speech::Manager::ResetGlobalHistory();

    mFocus = kPursuitFlow;
    mQuadrantState = kReset;
    mRacerCount = 0;

#ifdef EA_BUILD_A124
    // La A124 desactiva los coches de policia en vez de quitarlos.
    for (Speech::copMap::iterator iter = mActors.begin(); iter != mActors.end(); ++iter) {
        if (iter->cop) {
            iter->cop->SetActive(false);
        }
    }
#else
    if (mActors.size() > 0) {
        Speech::copMap::iterator iter = mActors.begin();
        while (mActors.size() > 0) {
            RemoveCop(iter->hsimable);
        }
    }
#endif

    mLeader = 0;
    mCopsInFormation.clear();
#ifndef EA_BUILD_A124
    mPursuit = 0;
    mAIPursuit = 0;
    mLastCopInFormation = 0;
    mLatestCop = 0;
#endif
    mT_pursuitStart = WorldTimer;
    mT_lastCopNailed = Timer(0);
    mT_lastCrashed = Timer(0);
    mT_noLOS = Timer(0);
    mT_reallylowspeed = Timer(0);
    mT_outofFormation = Timer(0);
#ifndef EA_BUILD_A124
    mT_LOS = Timer(0);
#endif
    mCopsInView = 0;
    mInfraction = 0;
    mPursuitState = kInactive;
    mFlags = 0;
    mPursuitDist = 0.0f;
    mTrafficHits911 = 0;
    mCTS911 = 0;
    mHavoc = 0;
}

void SoundAI::ShuffleActors() {
    if (mActors.size() == 0) {
        return;
    }

    Speech::copList active;
    Speech::copList inactive;
    active.reserve(mActors.size());
    inactive.reserve(mActors.size());

    {
        Speech::copMap::const_iterator iter = mActors.begin();
        while (iter != mActors.end()) {
            EAXCop *cop = iter->cop;
            if (cop->IsActive()) {
                active.push_back(cop);
            } else {
                inactive.push_back(cop);
            }
            iter++;
        }
    }

    {
        Speech::copList::iterator i = inactive.begin();
        while (i != inactive.end()) {
            Speech::copList::iterator j = active.begin();
            while (j != active.end()) {
                if ((*i)->GetSpeakerID() == (*j)->GetSpeakerID()) {
                    RemoveCop((*i)->GetHandle());
                    break;
                }
                ++j;
            }
            ++i;
        }
    }

    if (active.size() > 1) {
        bool item_removed = false;
        Speech::copList::iterator i = active.begin();
        while (i != active.end()) {
            if (active.size() <= 1) {
                break;
            }
            Speech::copList::iterator j = i + 1;
            while (j != active.end()) {
                if (((*i)->GetSpeakerID() == (*j)->GetSpeakerID()) && ((*i)->GetHandle() != (*j)->GetHandle())) {
                    EAXCop *a = *i;
                    EAXCop *b = *j;
                    // `item_removed = true` DUPLICADO en las dos ramas: el
                    // cross-jumping funde las dos copias y deja el `li 1` donde
                    // el objetivo lo tiene (99,45 -> 100%, 2.012 B).
                    if (a->GetDistance() < b->GetDistance()) {
                        RemoveCop(b->GetHandle());
                        active.erase(j);
                        item_removed = true;
                    } else {
                        RemoveCop(a->GetHandle());
                        active.erase(i);
                        item_removed = true;
                    }
                    break;
                }
                ++j;
            }
            if (!item_removed) {
                ++i;
            }
            item_removed = false;
        }
    }

    if (active.size() != 0) {
        if (!mLeader || !mLeader->IsActive() || !mLeader || mLeader->IsHeli()) {
            Speech::copList::iterator i = active.begin();
            while (i != active.end()) {
                EAXCop *activecop = *i;
                if (activecop->IsPrimary() && activecop->HasLOS()) {
                    mLeader = activecop;
                    if ((mFocus == kStrategyFlow) && !mLeader->IsHeli()) {
                        mLeader->PrimaryEngage();
                    }
                    break;
                }
                ++i;
            }
        }
        if (mLeader && !mLeader->IsActive() && !MakeLeader(mLeader)) {
            return;
        }
    }
}

bool SoundAI::IsHeadingValid() {
#if defined(EA_PLATFORM_WIN32) || defined(EA_PLATFORM_XENON)
    if (mPlayerCurrent[0].roadID > untagged && mPlayerCurrent[0].roadID < Seaside_Interchange && (mPlayerCurrent[0].direction > 0)) {
#else
    if ((static_cast<unsigned int>(mPlayerCurrent[0].roadID - on_Highway99) < 0x50) && (mPlayerCurrent[0].direction != 0)) {
#endif
        return true;
    }
    return false;
}

#ifdef EA_BUILD_A124
// La A124 es otra version: el salto de calor recorre heat_cutoffs hacia arriba y el aviso al 911 sale de sus
// propios contadores (CTSFor911, NumCiviHitsFor911, TimeInactiveFor911) con la descripcion del vehiculo.
void SoundAI::SyncPlayers() {
    IPlayer *player = IPlayer::First(PLAYER_LOCAL);

    IPerpetrator *perp = 0;
    IVehicle *vehicle = 0;
    IVehicleAI *vai = 0;
    player->GetSimable()->QueryInterface(&vehicle);
    player->GetSimable()->QueryInterface(&perp);
    vehicle->QueryInterface(&vai);

    WRoadNav *nav = vai->GetDriveToNav();

    unsigned int roadID = nav->GetRoadSpeechId();
    mPlayerCurrent[0].roadID = static_cast<RoadNames>(roadID);
    unsigned int dir = CalcPlayerDirection(false);

    mPlayerCurrent[0].direction = dir;

    if (IsHeadingValid()) {
        bool road_changed = (mPlayerCurrent[1].roadID != mPlayerCurrent[0].roadID);
        mPlayerCurrent[1].roadID = mPlayerCurrent[0].roadID;
        mPlayerCurrent[1].direction = mPlayerCurrent[0].direction;
        if ((mLastKnown.direction != dir) && mPursuit && mPursuit->IsPerpInSight() && road_changed) {
            if ((mPursuitState == kActive) && (mFocus != kPursuitFlow)) {
                EAXCop *cop = FindClosestCop(true, true);
                if (cop) {
                    cop->DirectionChange();
                }
            }
            mLastKnown.direction = dir;
            mLastKnown.roadID = mPlayerCurrent[0].roadID;
        }
    }

    if (!perp) {
        return;
    }

    {

        static float prev_heat = 1.0f;
        Attrib::Gen::pursuitlevels *pursuitatr;

    if (mPVehicle.GetCollection() != player->GetSimable()->GetAttributes().GetCollection()) {
        mPVehicle.ChangeWithDefault(player->GetSimable()->GetAttributes().GetCollection());
    }

    if (!mPlayerCarCustom || (mPlayerCarCustom->color == 0)) {
        if (mPlayerCarCustom) {
            delete mPlayerCarCustom;
        }
        mPlayerCarCustom = new ("SoundAI CarCustomization", 0) CarCustomizations;
        if (!GetCustomized(vehicle, *mPlayerCarCustom)) {
            delete mPlayerCarCustom;
            mPlayerCarCustom = 0;
        }
    }

    mPlayerSpeed = MPS2MPH(vehicle->GetSpeed());
    if ((mPlayerSpeed < mTune.MinSpeedConsideredStopped()) && ((mFlags & LOWSPEEDTIMER) == 0)) {
        mT_reallylowspeed = WorldTimer;
        mFlags |= LOWSPEEDTIMER;
    } else if (mPlayerSpeed >= mTune.MinSpeedConsideredStopped()) {
        mFlags &= ~LOWSPEEDTIMER;
        mT_reallylowspeed = WorldTimer;
    }

    mPlayerPos = player->GetPosition();
    if (SPAMAccessorSpeech.IsValid()) {
        const bVector2 ppos(mPlayerPos.z, -mPlayerPos.x);
        SPAMAccessorSpeech.CaptureData(ppos.x, ppos.y);
        mPlayerOffroadID = SPAMAccessorSpeech.GetDataInt(1);
    }

    mPlayerHeat = static_cast<int>(perp->GetHeat());

    if ((mFocus != kPursuitFlow) && (mPursuitDuration > 5.0f)) {
        if (perp->GetHeat() > prev_heat) {
            bool jump = false;
            int i;
            for (i = 0; i < 4; i++) {
                if ((prev_heat < heat_cutoffs[i].value) && (perp->GetHeat() >= heat_cutoffs[i].value)) {
                    prev_heat = perp->GetHeat();
                    jump = true;
                    break;
                }
            }

            if (jump && ((mPursuitState == kActive) || (mPursuitState == kSearching))) {
                if (bRandom(1.0f) > 0.5f) {
                    mDispatch->HeatJump(heat_cutoffs[i].heat_level);
                } else {
                    EAXCop *cop = GetRandomActiveCop(1, false);
                    if (mHeli) {
                        if (bRandom(1.0f) > 0.5f) {
                            mHeli->HeatJump(heat_cutoffs[i].heat_level);
                        } else if (cop) {
                            cop->HeatJump(heat_cutoffs[i].heat_level);
                        }
                    } else if (cop) {
                        cop->HeatJump(heat_cutoffs[i].heat_level);
                    }
                }

                if (bRandom(1.0f) > 0.5f) {
                    if ((3.0f <= perp->GetHeat()) && (perp->GetHeat() < 4.0f)) {
                        mDispatch->JurisShift(Csis::Type_jurisdiction_state);
                    } else if (5.0f <= perp->GetHeat()) {
                        mDispatch->JurisShift(Csis::Type_jurisdiction_federal);
                    }
                }
            }
        }
    }

    pursuitatr = perp->GetPursuitLevelAttrib();
    if (pursuitatr && pursuitatr->IsValid()) {
        if (mPursuitLevel.GetCollection() != pursuitatr->GetCollection()) {
            mPursuitLevel.ChangeWithDefault(pursuitatr->GetCollection());
        }
    }

    if (mPursuitLevel.IsValid()) {
        if (mPursuitLevel.roadblockprobability() > 0.0f) {
            mFlags |= RB_ENABLED;
        } else {
            mFlags &= ~RB_ENABLED;
        }
        if (mPursuitLevel.roadblockhelichance() > 0.0f) {
            mFlags |= HELIRB_ENABLED;
        } else {
            mFlags &= ~HELIRB_ENABLED;
        }
        if (mPursuitLevel.roadblockspikechance() > 0.0f) {
            mFlags |= SPIKES_ENABLED;
        } else {
            mFlags &= ~SPIKES_ENABLED;
        }

        mNumCopsInWave = mPursuitLevel.NumCopsToTriggerBackup();

        if ((mPursuitState == kInactive) &&
            ((mCTS911 >= mPursuitLevel.CTSFor911()) || (mTrafficHits911 >= mPursuitLevel.NumCiviHitsFor911())) &&
            (GetTimeSinceLastChase() >= mPursuitLevel.TimeInactiveFor911()) && ((mFlags & DISP911_ACTIVE) == 0)) {
            bool is_DDay = false;
            bool is_Race = false;
            if (GRaceStatus::Get().GetRaceParameters()) {
                if (GRaceStatus::Get().GetRaceParameters()->GetIsDDayRace()) {
                    is_DDay = true;
                }
                if (GRaceStatus::Get().GetRaceParameters()->GetIsPursuitRace()) {
                    is_Race = true;
                }
            }
            if (!is_DDay && !is_Race) {
                mDispatch->Report911(IsHighIntensity() ? Csis::Type_pursuit_type_Possible_Wanted : Csis::Type_pursuit_type_Generic_Speeder);
                int speaker = MiscSpeech::MoreDetails(0);
                if (GetPlayerCarColor() && MiscSpeech::IsVehicleTypeOK() && (bRandom(1.0f) > 0.5f)) {
                    mDispatch->VehicleDescription();
                } else {
                    mDispatch->NoVehicleDescription();
                }
                MiscSpeech::Unit911Reply(speaker);
                mFlags |= DISP911_ACTIVE;
                perp->Set911CallTime(mPursuitLevel.Lifetime911());
            }
        }

        if ((mFlags & DISP911_ACTIVE) != 0) {
            if (perp->Get911CallTime() <= 0.0f) {
                mFlags &= ~DISP911_ACTIVE;
                mTrafficHits911 = 0;
                mCTS911 = 0;
            }
        }
    }

    UMath::Vector3 vel;
    UMath::Unit(player->GetSimable()->GetRigidBody()->GetLinearVelocity(), vel);
    mSmoothedFWRoad.x = (mSmoothedFWRoad.x * 0.9f) + (vel.x * 0.1f);
    mSmoothedFWRoad.y = (mSmoothedFWRoad.y * 0.9f) + (vel.y * 0.1f);
    mSmoothedFWRoad.z = (mSmoothedFWRoad.z * 0.9f) + (vel.z * 0.1f);
    }
}
#else
void SoundAI::SyncPlayers() {
    IPlayer *player = IPlayer::First(PLAYER_LOCAL);

    IPerpetrator *perp = 0;
    IVehicle *vehicle = 0;
    IVehicleAI *vai = 0;
    player->GetSimable()->QueryInterface(&vehicle);
    player->GetSimable()->QueryInterface(&perp);
    vehicle->QueryInterface(&vai);

    WRoadNav *nav = vai->GetDriveToNav();

    unsigned int roadID = nav->GetRoadSpeechId();
    mPlayerCurrent[0].roadID = static_cast<RoadNames>(roadID);
    unsigned int dir = CalcPlayerDirection(false);

    mPlayerCurrent[0].direction = dir;

    if (IsHeadingValid()) {
        bool road_changed = (mPlayerCurrent[1].roadID != mPlayerCurrent[0].roadID);
        mPlayerCurrent[1].roadID = mPlayerCurrent[0].roadID;
        mPlayerCurrent[1].direction = mPlayerCurrent[0].direction;
        if ((mLastKnown.direction != dir) && mPursuit && mPursuit->IsPerpInSight() && road_changed) {
            if ((mPursuitState == kActive) && (mFocus != kPursuitFlow)) {
                EAXCop *cop = FindClosestCop(true, true);
                if (cop) {
                    cop->DirectionChange();
                }
            }
            mLastKnown.direction = dir;
            mLastKnown.roadID = mPlayerCurrent[0].roadID;
        }
    }

    if (!perp) {
        return;
    }

    {
        int heat;
        static float prev_heat = 1.0f;
        Attrib::Gen::pursuitlevels *pursuitatr;

    if (mPVehicle.GetCollection() != player->GetSimable()->GetAttributes().GetCollection()) {
        mPVehicle.ChangeWithDefault(player->GetSimable()->GetAttributes().GetCollection());
    }

    if (!mPlayerCarCustom || (mPlayerCarCustom->color == 0)) {
        if (mPlayerCarCustom) {
            delete mPlayerCarCustom;
        }
        mPlayerCarCustom = new ("SoundAI CarCustomization", 0) CarCustomizations;
        if (!GetCustomized(vehicle, *mPlayerCarCustom)) {
            delete mPlayerCarCustom;
            mPlayerCarCustom = 0;
        }
    }

    mPlayerSpeed = MPS2MPH(vehicle->GetSpeed());
    if ((mPlayerSpeed < mTune.MinSpeedConsideredStopped()) && ((mFlags & LOWSPEEDTIMER) == 0)) {
        mT_reallylowspeed = WorldTimer;
        mFlags |= LOWSPEEDTIMER;
    } else if (mPlayerSpeed >= mTune.MinSpeedConsideredStopped()) {
        mFlags &= ~LOWSPEEDTIMER;
        mT_reallylowspeed = WorldTimer;
    }

    mPlayerPos = player->GetPosition();
    if (SPAMAccessorSpeech.IsValid()) {
        const bVector2 ppos(mPlayerPos.z, mPlayerPos.x * -1.0f);
        SPAMAccessorSpeech.CaptureData(ppos.x, ppos.y);
        mPlayerOffroadID = SPAMAccessorSpeech.GetDataInt(1);
    }

    heat = static_cast<int>(perp->GetHeat());
    mPlayerHeat = heat;

    if ((mFocus != kPursuitFlow) && (mPursuitDuration > 5.0f) && (perp->GetHeat() > prev_heat)) {
        bool jump = false;
        int i = 3;
        while (i >= 0) {
            if ((prev_heat < heat_cutoffs[i].value) && (perp->GetHeat() >= heat_cutoffs[i].value)) {
                jump = true;
                prev_heat = perp->GetHeat();
                break;
            }
            i--;
        }

        if (jump && ((mPursuitState == kActive) || (mPursuitState == kSearching))) {
            if (bRandom(1.0f) > 0.5f) {
                mDispatch->HeatJump(heat_cutoffs[i].heat_level);
            } else {
                EAXCop *cop = GetRandomActiveCop(1, false);
                if (mHeli) {
                    if (bRandom(1.0f) > 0.5f) {
                        mHeli->HeatJump(heat_cutoffs[i].heat_level);
                    } else if (cop) {
                        cop->HeatJump(heat_cutoffs[i].heat_level);
                    }
                } else if (cop) {
                    cop->HeatJump(heat_cutoffs[i].heat_level);
                }
            }

            if (bRandom(1.0f) > 0.5f) {
                if ((3.0f <= perp->GetHeat()) && (perp->GetHeat() < 4.0f)) {
                    mDispatch->JurisShift(Csis::Type_jurisdiction_state);
                } else if (5.0f <= perp->GetHeat()) {
                    mDispatch->JurisShift(Csis::Type_jurisdiction_federal);
                }
            }
        }
    }

    pursuitatr = perp->GetPursuitLevelAttrib();
    if (pursuitatr && pursuitatr->IsValid()) {
        if (mPursuitLevel.GetCollection() != pursuitatr->GetCollection()) {
            mPursuitLevel.ChangeWithDefault(pursuitatr->GetCollection());
        }
    }

    if (mPursuitLevel.IsValid()) {
        if (mPursuitLevel.roadblockprobability() > 0.0f) {
            mFlags |= RB_ENABLED;
        } else {
            mFlags &= ~RB_ENABLED;
        }
        if (mPursuitLevel.roadblockhelichance() > 0.0f) {
            mFlags |= HELIRB_ENABLED;
        } else {
            mFlags &= ~HELIRB_ENABLED;
        }
        if (mPursuitLevel.roadblockspikechance() > 0.0f) {
            mFlags |= SPIKES_ENABLED;
        } else {
            mFlags &= ~SPIKES_ENABLED;
        }

        mNumCopsInWave = mPursuitLevel.NumCopsToTriggerBackup();

        if ((mPursuitState == kInactive) && ((mFlags & DISP911_ACTIVE) == 0)) {
            bool is_DDay = false;
            bool is_Race = false;
            bool is_Roaming = false;
            float t_lockout;
            ICopMgr *copmgr;
            if ((GRaceStatus::Exists() && (GRaceStatus::Get().GetPlayMode() == GRaceStatus::kPlayMode_Roaming)) ||
                (GRaceDatabase::Exists() && !GRaceDatabase::Get().GetStartupRace())) {
                is_Roaming = true;
            }

            copmgr = UTL::Collections::Singleton<ICopMgr>::Get();
            if (copmgr && ICopMgr::AreCopsEnabled()) {
                t_lockout = copmgr->GetLockoutTimeRemaining();
                if (GRaceStatus::Get().GetRaceParameters()) {
                    if (GRaceStatus::Get().GetRaceParameters()->GetIsDDayRace()) {
                        is_DDay = true;
                    }
                    if (GRaceStatus::Get().GetRaceParameters()->GetIsPursuitRace()) {
                        is_Race = true;
                    }
                }
                bool scripted_911 = ((t_lockout > 0.0f) && (t_lockout < 20.0f) && !is_Race && !is_Roaming);
                bool req911_met = false;
                if (((mCTS911 >= mPursuitLevel.CTSFor911()) || (mTrafficHits911 >= mPursuitLevel.NumCiviHitsFor911())) && is_Roaming) {
                    req911_met = true;
                }

                if ((req911_met || scripted_911) && !is_DDay) {
                    mDispatch->Report911(IsHighIntensity() ? Csis::Type_pursuit_type_Possible_Wanted : Csis::Type_pursuit_type_Generic_Speeder);
                }
            }
        }
    }

    UMath::Vector3 vel;
    UMath::Unit(player->GetSimable()->GetRigidBody()->GetLinearVelocity(), vel);
    mSmoothedFWRoad.x = (mSmoothedFWRoad.x * 0.9f) + (vel.x * 0.1f);
    mSmoothedFWRoad.y = (mSmoothedFWRoad.y * 0.9f) + (vel.y * 0.1f);
    mSmoothedFWRoad.z = (mSmoothedFWRoad.z * 0.9f) + (vel.z * 0.1f);
    }
}
#endif

void SoundAI::Force911State() {
    IPlayer *player = IPlayer::First(PLAYER_LOCAL);
    IPerpetrator *perp = 0;

    player->GetSimable()->QueryInterface(&perp);
    ICopMgr *copmgr = UTL::Collections::Singleton<ICopMgr>::Get();
    if (copmgr && ICopMgr::AreCopsEnabled() && perp && mPursuitLevel.IsValid()) {
        mFlags |= DISP911_ACTIVE;
        perp->Set911CallTime(mPursuitLevel.Lifetime911());
        copmgr->LockoutCops(false);
    }
}

void SoundAI::SyncCarsToActors() {
    IVehicles new_cop_cars;
    new_cop_cars.reserve(30);
    new_cop_cars.clear();

    float closest = 65535.0f;
    unsigned char cops_in_view;
    unsigned char cops_with_los;
    unsigned char cops_ahead;
    unsigned char num_active;
    float pursuit_distance;
    mRacerCount = static_cast<char>(IVehicle::GetList(VEHICLE_AIRACERS).size());

    {
        IVehicle::List::const_iterator i = IVehicle::GetList(VEHICLE_AIRACERS).begin();
        while (i != IVehicle::GetList(VEHICLE_AIRACERS).end()) {
            IVehicle *vehicle = *i;
            IRenderable *renderable = 0;
            if (vehicle->QueryInterface(&renderable)) {
                float dist2cam = renderable->DistanceToView();
                if (dist2cam < closest) {
                    closest = dist2cam;
                }
            }
            ++i;
        }
    }

    if (closest <= mTune.AIRacerProximity()) {
        mFlags |= RACERS_PROXIMAL;
    } else {
        if ((mFlags & RACERS_PROXIMAL) != 0) {
            EAXCop *cop = GetRandomActiveCop(0, false);
            if (cop) {
                cop->FocusChange();
            }
        }
        mFlags &= ~RACERS_PROXIMAL;
    }

    cops_in_view = 0;
    cops_with_los = 0;
    cops_ahead = 0;
    num_active = 0;
    pursuit_distance = 65535.0f;

    {
    IVehicle::List::const_iterator i = IVehicle::GetList(VEHICLE_AICOPS).begin();
    while (i != IVehicle::GetList(VEHICLE_AICOPS).end()) {
        IVehicle *vehicle = *i;
        IRenderable *renderable = 0;
        vehicle->QueryInterface(&renderable);
        HSIMABLE thisObj;
        if (vehicle) {
            bool has_visual = false;
            bool is_ahead = false;
            bool in_view = false;
            bool is_in_rb = false;

            thisObj = vehicle->GetSimable()->GetOwnerHandle();

            IRoadBlock *irb = GetRoadblock();
            if (irb) {
                IVehicle *car_in_rb = irb->IsComprisedOf(thisObj);
                if (car_in_rb == vehicle) {
                    is_in_rb = true;
                }
            }

            if (vehicle->GetDriverClass() == DRIVER_COP) {
                IPursuitAI *ai = 0;
                vehicle->QueryInterface(&ai);

                if (vehicle->GetVehicleClass() == VehicleClass::CAR) {
                    if (renderable && renderable->InView()) {
                        cops_in_view++;
                        in_view = true;
                    }

                    if (vehicle->IsActive() && ai) {
                        float t_tgt_last_seen = ai->GetTimeSinceTargetSeen();
                        if ((t_tgt_last_seen < 0.05f) || (is_in_rb && in_view)) {
                            has_visual = true;
                            cops_with_los++;
                        }
                    }

                    UMath::Vector3 player_pos = mPlayerPos;
                    UMath::Vector3 copcar_pos = vehicle->GetPosition();
                    UMath::Vector3 player_fw = mPlayerFW;
                    UMath::Vector3 playerToCop;
                    EAXCop *actor;
                    if (vehicle->IsActive() || (renderable && renderable->InView())) {
                        UMath::Sub(copcar_pos, player_pos, playerToCop);
                        UMath::Unit(playerToCop, playerToCop);
                        float dot = UMath::Dot(playerToCop, player_fw);
                        if (dot > 0.0f) {
                            is_ahead = true;
                        }
                    }

                    if (vehicle->IsActive()) {
                        num_active++;
                        float cop_dist_to_car = UMath::Distance(copcar_pos, player_pos);
                        if (cop_dist_to_car < pursuit_distance) {
                            pursuit_distance = cop_dist_to_car;
                        }
                        if (is_ahead) {
                            cops_ahead++;
                        }
                    }

                    actor = mActors.Find(thisObj);
                    if (actor) {
                        if (vehicle->IsActive()) {
                            if (!actor->IsActive()) {
                                actor->SetActive(true);
                            }
                        } else if (DESTROY_COPS_ON_INACTIVITY) {
                            RemoveCop(actor->GetHandle());
                        } else {
                            actor->SetActive(false);
                        }

                        actor->SetLOS(has_visual);
                        actor->SetAhead(is_ahead);
                        actor->Update();

                        float t_lastblowby = (WorldTimer - mRecentBlowby.timestamp).GetSeconds();
                        if ((t_lastblowby > mTune.BlowbyInterval()) && (mPlayerSpeed > actor->GetSpeed()) && !is_ahead) {
                            mRecentBlowby.Set(actor->GetDistance(), mPlayerSpeed - actor->GetSpeed());
                        }

                        if ((actor->GetDistance() < mRecentBlowby.distance) && (mPlayerSpeed > actor->GetSpeed()) && !is_ahead &&
                            ((mPlayerSpeed - actor->GetSpeed()) > (mPlayerSpeed * 0.75f))) {
                            mRecentBlowby.Set(actor->GetDistance(), mPlayerSpeed - actor->GetSpeed());
                        }
                    } else {
                        new_cop_cars.push_back(vehicle);
                    }
                } else if (vehicle->GetVehicleClass() == VehicleClass::CHOPPER) {
                    if (mHeli) {
                        if (mHeli->GetHandle() != vehicle->GetSimable()->GetOwnerHandle()) {
                            mHeli->SetHandle(vehicle->GetSimable()->GetOwnerHandle());
                        }

                        if (vehicle->IsActive() && !mHeli->IsActive()) {
                            mHeli->SetActive(true);
                        } else if (!vehicle->IsActive() && mHeli->IsActive()) {
                            mHeli->SetActive(false);
                        }

                        if (vehicle->IsActive() && ai) {
                            float t_tgt_last_seen = ai->GetTimeSinceTargetSeen();
                            mHeli->SetLOS(t_tgt_last_seen < 0.05f);
                        }
                        mHeli->Update();
                    } else {
                        AddNewHeli(vehicle);
                    }
                }
            }
        }
        ++i;
    }
    }

    mCopsInView = cops_in_view;
    mLOSCount = cops_with_los;
    mPursuitDist = pursuit_distance;
    mNumActiveCopCars = num_active;
    if (num_active == cops_ahead) {
        mFlags |= COPS_ARE_AHEAD;
    } else {
        mFlags &= ~COPS_ARE_AHEAD;
    }

    if (new_cop_cars.size() > 0) {
        IVehicles::iterator i = new_cop_cars.begin();
        while (i != new_cop_cars.end()) {
            AddNewCop(*i);
            ++i;
        }
        new_cop_cars.clear();
    }

    {
    Speech::copMap::iterator iter = mActors.begin();
    while (iter != mActors.end()) {
        EAXCop *actor = iter->cop;
        if (actor && actor->IsActive() && !ISimable::FindInstance(actor->GetHandle())) {
            RemoveCop(actor->GetHandle());
            break;
        }
        ++iter;
    }
    }

    if (mDispatch) {
        mDispatch->Update();
    }
}

void SoundAI::SyncFormations() {
    Speech::copMap::iterator iter = mActors.begin();
    while (iter != mActors.end()) {
        EAXCop *cop = iter->cop;
        if (cop->GetInFormation() && !cop->IsHeli()) {
            bool found = false;
            Speech::copList::iterator i = mCopsInFormation.begin();
            while (i != mCopsInFormation.end()) {
                EAXCop *copInFormation = *i;
                if (copInFormation && copInFormation->GetHandle() == cop->GetHandle()) {
                    found = true;
                }
                ++i;
            }
            if (!found && !cop->IsHeli()) {
                mCopsInFormation.push_back(cop);
            }
        } else if (!mCopsInFormation.empty()) {
            Speech::copList::iterator i = mCopsInFormation.begin();
            while (i != mCopsInFormation.end()) {
                EAXCop *copInFormation = *i;
                if (copInFormation && copInFormation->GetHandle() == cop->GetHandle()) {
                    mCopsInFormation.erase(i);
                    break;
                }
                ++i;
            }
        }
        ++iter;
    }

    if (mCopsInFormation.size() == 1) {
        mLastCopInFormation = mCopsInFormation.front();
    }
    if (mCopsInFormation.size() > 0) {
        mT_outofFormation = WorldTimer;
    }
}

EAXCop *SoundAI::FindFurthestCop(bool includeHeli) {
    if (!mActors.size()) {
        return 0;
    }

    EAXCop *furthest = 0;
    {
        Speech::copMap::const_iterator iter = mActors.begin();
        while (iter != mActors.end()) {
            EAXCop *cop = iter->cop;
            if (cop) {
                if (!furthest || (cop->GetDistance() > furthest->GetDistance())) {
                    if ((cop->IsHeli() && includeHeli) || !cop->IsHeli()) {
                        furthest = cop;
                    }
                }
            }
            ++iter;
        }
    }
    return furthest;
}

EAXCop *SoundAI::FindClosestCop(bool enforceLOS, bool includeHeli) {
    if (!mActors.size()) {
        return 0;
    }

    EAXCop *closest = 0;
    {
        Speech::copMap::const_iterator iter = mActors.begin();
        while (iter != mActors.end()) {
            EAXCop *cop = iter->cop;
            if (cop && (!enforceLOS || cop->HasLOS()) && (includeHeli || !cop->IsHeli())) {
                if (!closest) {
                    closest = cop;
                }
                if (cop->GetDistance() < closest->GetDistance()) {
                    closest = cop;
                }
            }
            ++iter;
        }
    }
    return closest;
}

void SoundAI::RemoveCop(HSIMABLE seeya) {
    if (mActors.size() == 0) {
        return;
    }

    EAXCop *cop = mActors.Remove(seeya);
    if (!cop) {
        return;
    }

    gSpeechCache.RemoveSpeaker(cop->GetSpeakerID());

    if (!mCopsInFormation.empty()) {
        Speech::copList::iterator i = mCopsInFormation.begin();
        while (i != mCopsInFormation.end()) {
            EAXCop *copInFormation = *i;
            if (copInFormation && copInFormation->GetHandle() == cop->GetHandle()) {
                mCopsInFormation.erase(i);
                break;
            }
            ++i;
        }
    }

    if (mLastCopInFormation && (mLastCopInFormation->GetHandle() == cop->GetHandle())) {
        mLastCopInFormation = 0;
    }
    if (mLeader == cop) {
        mLeader = 0;
    }
    if (mHeli == cop) {
        mHeli = 0;
    }
    if (mLatestCop == cop) {
        mLatestCop = 0;
    }

    mPursuitFlow->OnCopRemoved(cop);
    mUsage.voices.push_back(cop->GetSpeakerID());
    delete cop;
}

void SoundAI::AddNewHeli(IVehicle *heli) {
    HSIMABLE handle = heli->GetSimable()->GetOwnerHandle();
    EAXAirSupport *chopper = new EAXAirSupport(2, handle);

    mActors.Add(handle, chopper);
    mHeli = chopper;

    if (mFocus != 1) {
        if ((mFocus != 999) && (mFocus != 0)) {
            chopper->BackupArrives();
            return;
        }
        if (mFocus != 1) {
            return;
        }
    }
    mFlags |= HELI_INTRO_REQ;
}

int SoundAI::GetBattalionFromRoadID(int roadID) {
    Csis::Type_location_region region;
    Csis::Type_location location;
    bool result = MiscSpeech::GetLocation(static_cast<RoadNames>(roadID), region, location);

    if (!result) {
        return -1;
    }
    switch (region) {
    case 8:
        return 4;
    case 2:
    case 4:
        return 2;
    case 1:
        return 1;
    default:
        return -1;
    }
}

int SoundAI::GetBattalionFromKey(unsigned int theKey) {
    if ((theKey == 0x38B38226) || (theKey == 0x54B10E38) || (theKey == 0x2E149EAC)) {
        return 0x20;
    }
    if ((theKey == 0x7A49CCCB) || (theKey == 0xA4EB6688) || (theKey == 0xB2F32FE2) || (theKey == 0xD37C806D)) {
        return 0x10;
    }
    return -1;
}

void SoundAI::AddNewCop(IVehicle *newcop) {
    IVehicleAI *vai = 0;
    newcop->QueryInterface(&vai);
    HSIMABLE newbie = newcop->GetSimable()->GetOwnerHandle();
    int bID;
    vai->GetAttributes();
    WRoadNav *nav = vai->GetDriveToNav();
    int roadID = nav->GetRoadSpeechId();
    bool is_rb_cop = false;
    bool is_cross = (newcop->GetVehicleKey() == static_cast<unsigned int>(-0x2c837f93));
    IRoadBlock *block = GetRoadblock();
    if (block) {
        if (block->IsComprisedOf(newbie) == newcop) {
            is_rb_cop = true;
        }
    }

    EAXCop *latest_cop = 0;
    int keyedID = GetBattalionFromKey(newcop->GetVehicleKey());
    if (keyedID > 0) {
        bID = keyedID;
    } else {
        if (roadID == 0x6b) {
            return;
        }
        int bid = GetBattalionFromRoadID(roadID);
        if (bid > 0) {
            bID = bid;
        } else {
#ifndef EA_BUILD_A124
            bID = 1 << bRandom(4);
#else
            return;
#endif
        }
    }

    if (mUsage.voices.empty() || !is_cross) {
        UMath::Vector3 cop_pos = newcop->GetPosition();
        UMath::Vector3 pPos = mPlayerPos;
        float distance = UMath::Distance(pPos, cop_pos);

        Speech::copMap::const_iterator i = mActors.begin();
        while (i != mActors.end()) {
            EAXCop *cop = i->cop;
            if (!cop->IsHeli() && !is_rb_cop && (distance < cop->GetDistance()) && !cop->IsHeli()) {
                int cID;
                if (cop->GetCallsign() != bID) {
                    keyedID = GetBattalionFromKey(newcop->GetVehicleKey());
                    if (keyedID > 0) {
                        bID = keyedID;
                    } else if (roadID != 0x6b) {
                        keyedID = GetBattalionFromRoadID(roadID);
                        if (keyedID > 0) {
                            bID = keyedID;
                        } else {
#ifndef EA_BUILD_A124
                            bID = 1 << bRandom(4);
#else
                            return;
#endif
                        }
                    } else {
                        return;
                    }
                }

                mActors.ModifyHandle(cop->GetHandle(), newbie);
                cop->SetHandle(newbie);
                cID = GetCallsign(static_cast<Csis::Type_speaker_battalion>(bID));
                cop->SetCallsign(bID);
                cop->SetUnitNumber(cID);
                cop->Update();
                latest_cop = cop;
                break;
            }
            ++i;
        }
    }

    if (!latest_cop) {
        if (!mUsage.voices.empty() || is_cross) {
            int voice = GetVoice(is_cross ? 3 : (is_rb_cop ? 2 : 0));
            if (voice > 0) {
                int cID = GetCallsign(static_cast<Csis::Type_speaker_battalion>(bID));
                EAXCop *primary = new EAXCop(voice, newbie, bID, cID);
                primary->SetRank(mActors.size());
                mActors.Add(newbie, primary);
                gSpeechCache.AddSpeaker(voice);
                latest_cop = primary;
                latest_cop->Update();
            }
        }
        if (!latest_cop) {
            return;
        }
    }

    mLatestCop = latest_cop;
    if (((mPursuitState == kActive) || (mPursuitState == kSearching)) && !is_rb_cop && (mFocus == kStrategyFlow)) {
        if (bRandom(1.0f) > 0.5f) {
            mLatestCop->BackupArrives();
        } else {
            mLatestCop->UnitBackupReply();
        }
    }
}

bool SoundAI::MakeLeader(EAXCop *newprim) {
    EAXCop *wannab;
    Speech::copMap::iterator primary;

    if (mLeader && newprim && (mLeader->GetSpeakerID() == newprim->GetSpeakerID())) {
        return true;
    }

    if (newprim->IsActive()) {
        if ((newprim->GetSpeakerID() == 5) || (newprim->GetSpeakerID() == 4) || (newprim->GetSpeakerID() == 3) ||
            (newprim->GetSpeakerID() == 2) || (newprim->GetSpeakerID() == 9)) {
            if (mLeader != newprim) {
                mLeader = newprim;
                if (mFocus == kStrategyFlow) {
                    newprim->PrimaryEngage();
                }
            }
            return true;
        }
    }

    wannab = mActors.Find(newprim->GetHandle());
    if (!wannab) {
        return false;
    }

    primary = mActors.begin();
    while (primary != mActors.end()) {
        EAXCop *cop = primary->cop;
        if ((cop->GetSpeakerID() == 5) || (cop->GetSpeakerID() == 4) || (cop->GetSpeakerID() == 3) ||
            (cop->GetSpeakerID() == 9)) {
            break;
        }
        ++primary;
    }

    if (primary == mActors.end()) {
        int rand = bRandom(3);
        switch (rand) {
        case 0:
            newprim->SetSpeakerID(3);
            break;
        case 1:
            newprim->SetSpeakerID(4);
            break;
        case 2:
            newprim->SetSpeakerID(5);
            break;
        default:
            newprim->SetSpeakerID(3);
            break;
        }

        Speech::voiceIDs::iterator i = mUsage.voices.begin();
        while (i != mUsage.voices.end()) {
            if (*i == newprim->GetSpeakerID()) {
                mUsage.voices.erase(i);
                break;
            }
            ++i;
        }

        if (mLeader != newprim) {
            mLeader = newprim;
            if (mFocus == kStrategyFlow) {
                newprim->PrimaryEngage();
            }
        }
        gSpeechCache.AddSpeaker(mLeader->GetSpeakerID());
        return true;
    } else {
        int spkrA = wannab->GetSpeakerID();
        int spkrB = primary->cop->GetSpeakerID();
        wannab->SetSpeakerID(spkrB);
        primary->cop->SetSpeakerID(spkrA);
    }

    if (mLeader != newprim) {
        mLeader = newprim;
        if (mFocus == kStrategyFlow) {
            newprim->PrimaryEngage();
        }
    }
    return true;
}

int SoundAI::GetCallsign(Csis::Type_speaker_battalion battalion) {
    Speech::voiceIDs *cs_pool = 0;

    switch (battalion) {
    case Csis::Type_speaker_battalion_Rosewood:
        if (mUsage.cs_Rosewood.empty()) {
            RandomizeCallsign(mUsage.cs_Rosewood, Csis::Type_speaker_call_sign_id_CallSign01, Csis::Type_speaker_call_sign_id_CallSign10);
        }
        cs_pool = &mUsage.cs_Rosewood;
        break;
    case Csis::Type_speaker_battalion_Coastal:
        if (mUsage.cs_Coastal.empty()) {
            RandomizeCallsign(mUsage.cs_Coastal, Csis::Type_speaker_call_sign_id_CallSign01, Csis::Type_speaker_call_sign_id_CallSign10);
        }
        cs_pool = &mUsage.cs_Coastal;
        break;
    case Csis::Type_speaker_battalion_City:
        if (mUsage.cs_City.empty()) {
            RandomizeCallsign(mUsage.cs_City, Csis::Type_speaker_call_sign_id_CallSign01, Csis::Type_speaker_call_sign_id_CallSign20);
        }
        cs_pool = &mUsage.cs_City;
        break;
#ifndef EA_BUILD_A124
    case Csis::Type_speaker_battalion_Alpine:
        if (mUsage.cs_Alpine.empty()) {
            RandomizeCallsign(mUsage.cs_Alpine, Csis::Type_speaker_call_sign_id_CallSign01, Csis::Type_speaker_call_sign_id_CallSign10);
        }
        cs_pool = &mUsage.cs_Alpine;
        break;
#endif
    case Csis::Type_speaker_battalion_Super_Pursuit:
        if (mUsage.cs_SuperPursuit.empty()) {
            RandomizeCallsign(mUsage.cs_SuperPursuit, Csis::Type_speaker_call_sign_id_CallSign01, Csis::Type_speaker_call_sign_id_CallSign05);
        }
        cs_pool = &mUsage.cs_SuperPursuit;
        break;
    case Csis::Type_speaker_battalion_Rhino_Units:
        if (mUsage.cs_Rhino.empty()) {
            RandomizeCallsign(mUsage.cs_Rhino, Csis::Type_speaker_call_sign_id_CallSign01, Csis::Type_speaker_call_sign_id_CallSign06);
        }
        cs_pool = &mUsage.cs_Rhino;
        break;
    default:
        break;
    }

    if (cs_pool != nullptr) {
        Speech::voiceIDs::iterator iter = cs_pool->begin();
        int id = *iter;
        cs_pool->erase(iter);
        return id;
    } else {
        return -1;
    }
}

void SoundAI::RandomizeCallsign(Speech::voiceIDs &cs, Csis::Type_speaker_call_sign_id start, Csis::Type_speaker_call_sign_id finish) {
    if (cs.empty()) {
        int i = start;
        while (i <= finish) {
            cs.push_back(i);
            i += i;
        }
        for (unsigned int ndx = 0; ndx < cs.size(); ndx++) {
            int rand = bRandom(static_cast<int>(cs.size()));
            int rand_cs = cs[rand];
            int curr_cs = cs[ndx];
            if (rand_cs != curr_cs) {
                cs[ndx] = rand_cs;
                cs[rand] = curr_cs;
            }
        }
    }
}

int SoundAI::GetVoice(int type) {
    int return_voice = -1;

    if (mUsage.voices.empty()) {
        return -1;
    }

    switch (type) {
    case 1:
        for (Speech::voiceIDs::iterator i = mUsage.voices.begin(); i < mUsage.voices.end(); ++i) {
            if (static_cast<unsigned int>(*i - 3) < 3) {
                return_voice = *i;
                mUsage.voices.erase(i);
                break;
            }
        }
        break;
    case 2:
        for (Speech::voiceIDs::iterator i = mUsage.voices.begin(); i < mUsage.voices.end(); ++i) {
            if (static_cast<unsigned int>(*i - 6) < 3) {
                return_voice = *i;
                mUsage.voices.erase(i);
                break;
            }
        }
        break;
    case 3:
        for (Speech::voiceIDs::iterator i = mUsage.voices.begin(); i < mUsage.voices.end(); ++i) {
            if (*i == 9) {
                return_voice = 9;
                mUsage.voices.erase(i);
                break;
            }
        }
        break;
    default:
        for (Speech::voiceIDs::iterator i = mUsage.voices.begin(); i < mUsage.voices.end(); ++i) {
            if (static_cast<unsigned int>(*i - 3) < 6) {
                return_voice = *i;
                mUsage.voices.erase(i);
                break;
            }
        }
        break;
    }
    return return_voice;
}

EAXCop *SoundAI::GetCop(int speaker) {
    Speech::copMap::iterator iter = mActors.begin();
    while (iter != mActors.end()) {
        EAXCop *cop = iter->cop;
        if (cop && cop->GetSpeakerID() == speaker) {
            return iter->cop;
        }
        ++iter;
    }
    return 0;
}

void SoundAI::RandomBailoutDeny(EAXCop *wimp) {
    if (wimp->GetInFormation() == 1) {
        if ((mActors.size() > 1) && (mPursuitState != kInactive)) {
            Speech::copMap::iterator iter = mActors.begin();
            while (iter != mActors.end()) {
                if (iter->cop->IsPrimary()) {
                    if (iter->cop->GetSpeakerID() != wimp->GetSpeakerID()) {
                        iter->cop->DenyBailout();
                    }
                }
                ++iter;
            }
        }
    }
}

unsigned int SoundAI::CalcPlayerDirection(bool force_set) {
    static unsigned int dir_tracking = 0;
    static Timer t_currdir(0);

    unsigned int dir;
    float zmag = UMath::Abs(mSmoothedFWRoad.z);
    float xmag = UMath::Abs(mSmoothedFWRoad.x);
#ifdef EA_PLATFORM_WIN32
    if (xmag > zmag) {
        if (mSmoothedFWRoad.x > 0.0f) {
            dir = 4;
        } else {
            dir = 8;
        }
    } else {
        if (mSmoothedFWRoad.z > 0.0f) {
            dir = 2;
        } else {
            dir = 1;
        }
    }

    if ((dir != 0) && (dir != dir_tracking)) {
        dir_tracking = dir;
        t_currdir = WorldTimer;
    }
#else
    if (xmag > zmag) {
        dir = 8;
        if (0.0f < mSmoothedFWRoad.x) {
            dir = 4;
        }
    } else {
        dir = 1;
        if (0.0f < mSmoothedFWRoad.z) {
            dir = 2;
        }
    }

    if ((dir != 0) && (dir != dir_tracking)) {
        t_currdir = WorldTimer;
        dir_tracking = dir;
    }
#endif

    float t_samedir = (WorldTimer - t_currdir).GetSeconds();
    return ((t_samedir > 3.0f) || force_set) ? dir : 0;
}

void SoundAI::ForceGlobalVoiceChange() {
    Speech::copMap::iterator iter = mActors.begin();
    while (iter != mActors.end()) {
        EAXCop *cop = iter->cop;
        if (cop) {
            delete cop;
        }
        ++iter;
    }

    mActors.clear();
    mCopsInFormation.clear();
}

void SoundAI::Release() {
    if ((mRefCount != 0) && ((mRefCount = mRefCount - 1) == 0)) {
        Sim::Activity::Release();
    }
}

ColourHashToSoundColour ColourHashToSoundColourMap[] = {
    {FEHASH_RED, 0x1},
    {0x027FF2DC, 0x2},
    {FEHASH_WHITE, 0x4},
    {FEHASH_BLUE, 0x8},
    {FEHASH_GREEN, 0x10},
    {0x713E201B, 0x20},
    {0x79F61014, 0x40},
    {0x00163085, 0x80},
    {0x73D2EDD7, 0x100},
    {0x02837B47, 0x200},
    {FEHASH_YELLOW, 0x400},
    {0x001B06B1, 0x800},
    {0x027C3EBB, 0x1000},
};
int NumberOfColourHashToSoundColourMaps = 13;

unsigned char SoundAI::GetCustomized(IVehicle *vehicle, CarCustomizations &custrec) {
    const FECustomizationRecord *record;
    bool has_vinyls;
    bool has_custom_paint;
    bool has_racing_numbers;
    bool has_decals;

    record = vehicle->GetCustomizations();
    custrec.flags = 0;

    has_vinyls = false;
    has_racing_numbers = false;
    has_decals = false;
    has_custom_paint = has_decals;
    if (record) {
        CarPart *paint_part =
            record->GetInstalledPart(vehicle->GetModelType(), 0x4c);
        if (paint_part) {
            unsigned int colour_hash = paint_part->GetAppliedAttributeUParam(0xd68a7bab, 0);
            for (int i = 0; i < NumberOfColourHashToSoundColourMaps; i++) {
                if (ColourHashToSoundColourMap[i].Hash == colour_hash) {
                    custrec.color = ColourHashToSoundColourMap[i].SoundColour;
                    break;
                }
            }

            CarPart *vinyls =
                record->GetInstalledPart(vehicle->GetModelType(), 0x4d);
            if (vinyls) {
                has_vinyls = true;
            }

            CarPart *left_number1 =
                record->GetInstalledPart(vehicle->GetModelType(), 0x69);
            CarPart *left_number2 =
                record->GetInstalledPart(vehicle->GetModelType(), 0x6a);
            CarPart *right_number1 =
                record->GetInstalledPart(vehicle->GetModelType(), 0x71);
            CarPart *right_number2 =
                record->GetInstalledPart(vehicle->GetModelType(), 0x72);
            if (left_number1 || left_number2 || right_number1 || right_number2) {
                has_racing_numbers = true;
            }

            CarPart *left_door_decal =
                record->GetInstalledPart(vehicle->GetModelType(), 0x48);
            CarPart *right_door_decal =
                record->GetInstalledPart(vehicle->GetModelType(), 0x49);
            CarPart *left_quarter =
                record->GetInstalledPart(vehicle->GetModelType(), 0x4a);
            CarPart *right_quarter =
                record->GetInstalledPart(vehicle->GetModelType(), 0x4b);
            if (left_door_decal || right_door_decal || left_quarter || right_quarter) {
                has_decals = true;
            }
        }
    } else {
        CarTypeInfo *type_info = GetCarTypeInfo(vehicle->GetModelType());
        CarPart *paint_part = CarPartDB.NewGetCarPart(vehicle->GetModelType(),
            0x4c,
            type_info->GetDefaultBasePaint(),
            0,
            -1);
        if (paint_part) {
            unsigned int colour_hash = paint_part->GetAppliedAttributeUParam(0xd68a7bab, 0);
            for (int i = 0; i < NumberOfColourHashToSoundColourMaps; i++) {
                if (ColourHashToSoundColourMap[i].Hash == colour_hash) {
                    custrec.color = ColourHashToSoundColourMap[i].SoundColour;
                    break;
                }
            }
        }
    }

    if (has_vinyls) {
        custrec.flags |= VINYLS;
    }
    if (has_custom_paint) {
        custrec.flags |= PAINT;
    }
    if (has_racing_numbers) {
        custrec.flags |= RACING_NUMS;
    }
    if (has_decals) {
        custrec.flags |= DECALS;
    }
    return 1;
}

bool SoundAI::IsHighIntensity() {
    if ((mPlayerHeat >= static_cast<int>(mTune.HighIntensityMark())) || (mPursuitDuration >= mTune.PursuitDurationHighIntensity())) {
        return true;
    }
    return false;
}

#ifndef EA_BUILD_A124
const float SoundAI::GetTimeLastNailedCop() {
    Speech::copList nailed;

    nailed.reserve(mActors.size());
    Speech::copMap::iterator iter = mActors.begin();
    while (iter != mActors.end()) {
        EAXCop *cop = iter->cop;
        if (cop->GetTimesRammed() > 0) {
            nailed.push_back(cop);
        }
        ++iter;
    }

    if (nailed.empty()) {
        return 65535.0f;
    }

    float t_mostrecent = nailed.front()->GetTimeLastRammed();
    Speech::copList::iterator i = nailed.begin();
    while (i != nailed.end()) {
        EAXCop *unfortunate = *i;
        if (unfortunate->GetTimeLastRammed() < t_mostrecent) {
            t_mostrecent = unfortunate->GetTimeLastRammed();
        }
        ++i;
    }
    return t_mostrecent;
}
#endif
