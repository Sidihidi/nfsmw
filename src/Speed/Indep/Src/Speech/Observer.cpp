#include "Observer.h"
#include "EAXDispatch.h"
#include "MiscSpeech.h"
#include "RoadblockFlow.h"
#include "SoundAI.h"
#include "Speed/Indep/Src/World/Rain.hpp"
#include "Speed/Indep/Src/Generated/Messages/MGamePlayMoment.h"
#include "Speed/Indep/Src/Generated/Messages/MMiscSound.h"
#include "Speed/Indep/Src/Generated/Messages/MNotifySpeechStatus.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Interfaces/Simables/IINput.h"
#include "Speed/Indep/Src/Interfaces/Simables/IRigidBody.h"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#ifdef MILESTONE_BUILD
#include "Speed/Indep/Src/Frontend/DebugMenu.h"

extern const int SPEECHFLOW_DISPLAY;
#endif

namespace UTL {
namespace COM {
template <> inline bool IUnknown::QueryInterface<IInput>(IInput **out) {
    HINTERFACE handle = IInput::_IHandle();

    *out = (IInput *)_mCOMObject->_mInterfaces.Find(handle);
    return *out;
}
}
}


namespace Speech {

struct ComComparator {
    const UTL::COM::IUnknown *Me;

    ComComparator(const UTL::COM::IUnknown *me) : Me(me) {}

    bool operator()(IVehicle *vehicle) const {
        return UTL::COM::ComparePtr(vehicle, Me);
    }
};

Observer::Observer()
    : mObservations(), //
      mObserveMask(CarRam | Outcome), //
      mLastEvent(None), //
      mNumCopsWithLOS(0), //
      mT_bullhorn(WorldTimer), //
      mT_unstable(WorldTimer), //
      mT_airborne(WorldTimer), //
      mT_flipped(WorldTimer), //
      mT_trackingOutcome(WorldTimer), //
      mDotTrack(0.0f), //
      mOffroadHistory(0), //
      mCurrOffroadID(-1), //
      mTracking(0), //
      mT_gasstationexpl(WorldTimer), //
      mPrevPursuitState(2), //
      mWeather(false), //
      mTunnel(false), //
      mRamCop(0), //
      mGasStationPos(UMath::Vector3::kZero), //
      mFwPlayer(UMath::Vector3::kZero), //
      mFwRoad(UMath::Vector3::kZero), //
      mMsgBlewPastCop(Hermes::Handler::Create<MGamePlayMoment, Observer, Observer>(
          this, &Observer::MessageBlewPastCop, "BlewByCop", 0)), //
      mMsgGamePlayMoment(Hermes::Handler::Create<MGamePlayMoment, Observer, Observer>(
          this, &Observer::MessageGamePlayMoment, "MomentStrm", 0)), //
      mMsgNotifyEventCompletion(Hermes::Handler::Create<MNotifySpeechStatus, Observer, Observer>(
          this, &Observer::MessageEventComplete, UCrc32(0x20d60dbf), 0)), //
      mMsgTunnelUpdate(Hermes::Handler::Create<MMiscSound, Observer, Observer>(
          this, &Observer::MessageTunnelUpdate, "TunnelUpdate", 0)) {
    mObservations.clear();
}

Observer::~Observer() {
    mObservations.clear();
    if (mMsgBlewPastCop) {
        Hermes::Handler::Destroy(mMsgBlewPastCop);
    }
    if (mMsgNotifyEventCompletion) {
        Hermes::Handler::Destroy(mMsgNotifyEventCompletion);
    }
    if (mMsgGamePlayMoment) {
        Hermes::Handler::Destroy(mMsgGamePlayMoment);
    }
    if (mMsgTunnelUpdate) {
        Hermes::Handler::Destroy(mMsgTunnelUpdate);
    }
}

void Observer::Update() {
    int y;
    int x;
#ifdef MILESTONE_BUILD
    // Pantalla de depuracion de la A124 (cadenas de su .rodata).
    x = -200;
    y = -50;
    if (SPEECHFLOW_DISPLAY) {
        ScreenPrintf(x, y, 1.0f, "OBSERVER");
        y += 15;
    }
#endif
    if (mState != kTransition) {
        if (mState == kCullCheck) {
#ifdef MILESTONE_BUILD
            if (SPEECHFLOW_DISPLAY) {
                ScreenPrintf(x, y, 1.0f, "CullCheck");
            }
#endif
            CullCheck();
            return;
        }
    }
    ChangeStateTo(kCullCheck);
}

void Observer::CullCheck() {
    SoundAI *ai = SoundAI::Get();
    if (ai->GetPursuitState() != SoundAI::kActive) {
        return;
    }
    Process();
}

bool Observer::IsTransitionable() {
    return true;
}

void Observer::Reset() {
    SpeechFlow::Reset();
    mNumCopsWithLOS = 0;
    mT_bullhorn = Timer(0);
    mT_unstable = Timer(0);
    mT_airborne = Timer(0);
    mT_flipped = Timer(0);
#ifndef EA_BUILD_A124 // la A124 acaba en mWeather (lineas 144-153 de su Observer.cpp)
    mT_trackingOutcome = Timer(0);
    mT_gasstationexpl = Timer(0);
#endif
    mDotTrack = 0.0f;
    mOffroadHistory = 0;
    mTracking = 0;
    mPrevPursuitState = 2;
    mWeather = false;
#ifndef EA_BUILD_A124
    mTunnel = false;
    mRamCop = 0;
    mAirborneHeight = 0.0f;
    mAirborneLength = 0.0f;
    mGasStationPos = UMath::Vector3::kZero;
    mFwPlayer = UMath::Vector3::kZero;
    mFwRoad = UMath::Vector3::kZero;
#endif
}

void Observer::Observe(int currobsrvation, int speaker, float f) {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
#ifndef EA_BUILD_A124
    if (ai->GetPursuit() && (ai->GetFocus() != SoundAI::kPursuitFlow) && (ai->GetFocus() != SoundAI::kTerminal)) {
#else
    if ((ai->GetFocus() != SoundAI::kPursuitFlow) && (ai->GetFocus() != SoundAI::kTerminal) && ai->GetPursuit()) {
#endif
        bool found = false;
        for (observations::iterator i = mObservations.begin(); i != mObservations.end(); i++) {
            SpeechObservation &curr = *i;
            if (curr.observation == currobsrvation) {
                curr.time = WorldTimer;
                curr.speakerID = speaker;
                curr.force = curr.force < f ? f : curr.force;
                found = true;
            }
        }
        if (!found) {
            SpeechObservation newObs;
            newObs.speakerID = speaker;
            newObs.time = WorldTimer;
            newObs.force = f;
            newObs.observation = static_cast<SpeechObservations>(currobsrvation);
            mObservations.push_back(newObs);
        }
        mLastEvent = static_cast<SpeechObservations>(currobsrvation);
    }
}

void Observer::Process() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
#ifndef EA_BUILD_A124
    if (ai->GetPursuit()) {
        if ((ai->GetFocus() == SoundAI::kPursuitFlow && ai->GetPursuit()->TimeUntilBusted() < 0.5f) ||
            ai->GetFocus() == SoundAI::kTerminal) {
            return;
        }
#else
    if (ai->GetFocus() != SoundAI::kPursuitFlow && ai->GetFocus() != SoundAI::kTerminal && ai->GetPursuit()) {
#endif
        observations::iterator i = mObservations.begin();
        while (i != mObservations.end()) {
            SpeechObservation &obs = *i;
            EAXCop *speaker = ai->GetCop(obs.speakerID);
            if (speaker) {
                speaker->Collision(obs.observation, obs.force, ai->GetRandomCop(0));
            }
            mObservations.erase(i);
            if (mObservations.empty()) {
                break;
            }
            i = mObservations.begin();
        }
        IPlayer *player = IPlayer::First(PLAYER_LOCAL);
        if (player) {
            player->GetSimable()->GetRigidBody()->GetForwardVector(mFwPlayer);
        }
        AssessArrest();
#ifndef EA_BUILD_A124
        if ((mTracking & Arrest) == 0)
#endif
        {
            AssessLOS();
#ifdef EA_BUILD_A124
            AssessAirborne();
#else
            AssessFlippage();
#endif
            Assess180();
            AssessOutcome();
            AssessBraking();
            AssessOutrun();
            AssessOffroad();
            GasStationAftermath();
        }
        mNumCopsWithLOS = ai->NumCopsWithLOS();
    }
}

float Observer::CalcFWVec_Road_Car() {
    float rval = -666.6f;
    IPlayer *player = IPlayer::First(PLAYER_LOCAL);
    if (player) {
        IVehicleAI *vai = 0;
        player->GetSimable()->QueryInterface(&vai);
        if (vai) {
            WRoadNav *nav = vai->GetDriveToNav();
            UMath::Vector3 fwRoad = nav->GetForwardVector();
            UMath::Vector3 fwCar;
            player->GetSimable()->GetRigidBody()->GetForwardVector(fwCar);
            rval = UMath::Dot(fwCar, fwRoad);
            mFwPlayer = fwCar;
            mFwRoad = fwRoad;
        }
    }
    return rval;
}

void Observer::GasStationAftermath() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();

    if ((WorldTimer - mT_gasstationexpl).GetSeconds() < 15.0f) {
        copMap::const_iterator iter = ai->GetActors().begin();
        while (iter != ai->GetActors().end()) {
            EAXCop *cop = iter->cop;
            UMath::Vector3 cop_pos = cop->GetPosition();
            float rel_dist_expl = UMath::Distance(mGasStationPos, cop_pos);
            if (rel_dist_expl < 50.0f) {
                cop->CallforEV(2);
            }
            ++iter;
        }
    }
}

// gcdatos4: AssessWeather estaba ausente del arbol. En el DWARF-1 de la CU
// zSpeech su DIE de definicion (.debug+0x3E0B4CD, low_pc=-1, sin AT_inline, sin
// copia en el ELF final y sin expansiones inline en ningun llamante) va entre
// GasStationAftermath y AssessArrest, con locales ai (SoundAI* en r31), view
// (eView*), intensity (float en f13) y randcop (EAXCop* en r11) en un bloque
// interior, e inlines Singleton::Get, eGetView, Rain::GetRainIntensity y
// SoundAI::GetHeli x3; la clase Rain se completa en la fase de cabeceras de
// este segmento (.debug+0x3E04591). Sus unicas constantes son el par huerfano
// 0.0f/0.25f de obj+0x1874/0x1878, en posicion de fichero entre los pools de
// GasStationAftermath y AssessArrest: la definicion original no era inline
// (con `inline`, SN emite la copia al final de la TU y el pool cae en
// src+0x2FD8, medido) sino una definicion normal sin llamantes, cuyo codigo
// elimino el enlazador MW dejando el pool. El cuerpo sigue el esqueleto del
// DIE: mWeather es el miembro que WeatherExists() expone y que ninguna otra
// funcion pone a true, y EAXCop::WeatherReport no tiene otro llamante.
void Observer::AssessWeather() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    eView *view = eGetView(EVIEW_PLAYER1, false);
    if (view->Precipitation) {
        float intensity = view->Precipitation->GetRainIntensity();
        mWeather = intensity > 0.0f;
        if (intensity > 0.25f && ai->GetHeli() && ai->GetHeli()->IsActive()) {
            ai->GetHeli()->WeatherReport();
        }
        {
            EAXCop *randcop = ai->GetRandomActiveCop(0, false);
            if (randcop) {
                randcop->WeatherReport();
            }
        }
    }
}

void Observer::AssessArrest() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    float t_busted = 0.0f;
    Speech::Module *cop_speech = nullptr;
    if (ai->GetPursuit()) {
        t_busted = ai->GetPursuit()->TimeUntilBusted();
        if ((t_busted > 0.8f) && !Manager::IsQueued(kSPCH1_EventID_HeliBullhornArrest, 4) &&
            !Manager::IsCopSpeechPlaying(kSPCH1_EventID_HeliBullhornArrest) && !Manager::IsQueued(kSPCH1_EventID_BullhornArrest, 4) &&
            !Manager::IsCopSpeechPlaying(kSPCH1_EventID_BullhornArrest)) {
#ifndef EA_BUILD_A124
            GRaceParameters *parms = GRaceStatus::Exists() ? GRaceStatus::Get().GetRaceParameters() : nullptr;
            bool isEpicPursuit = false;
            if (parms && parms->GetIsEpicPursuitRace()) {
                isEpicPursuit = true;
            }
            bool isEpicCellCallQueued = false;
            if (isEpicPursuit && (Manager::IsQueued(kSPCH1_EventID_CellCall, 4) || Manager::IsCopSpeechPlaying(kSPCH1_EventID_CellCall))) {
                isEpicCellCallQueued = true;
            }

            // La A124 no tiene la persecucion epica ni marca Arrest.
            if (!isEpicCellCallQueued) {
                mTracking |= Arrest;
#endif
                EAXCop *cop = ai->GetRandomActiveCop(0, true);
                if (cop) {
                    cop_speech = Manager::GetSpeechModule(COPSPEECH_MODULE);
                    if (cop_speech) {
                        cop_speech->ReleaseResource();
                    }
                    Manager::ClearPlayback();
                    if (ai->GetHeli() && (ai->GetHeli()->GetDistance() < 100.0f) && ai->GetHeli()->IsActive()) {
                        ai->GetHeli()->BullhornArrest();
                    } else {
                        cop->BullhornArrest();
                    }
                }
#ifndef EA_BUILD_A124
            }
        } else {
            mTracking &= ~Arrest;
#endif
        }
    }

    if (!cop_speech) {
        cop_speech = Manager::GetSpeechModule(COPSPEECH_MODULE);
        if (!cop_speech) {
            return;
        }
    }

    if (cop_speech->GetSFXOBJ_Speech()) {
        float ftmp = t_busted * 32767.0f;
        if (ftmp > 32767.0f) {
            ftmp = 32767.0f;
        }
        if (ftmp < 0.0f) {
            ftmp = 0.0f;
        }
        int q15scale = static_cast<int>(ftmp);
        cop_speech->GetSFXOBJ_Speech()->SetDMIX_Input(5, q15scale);
    }
}

void Observer::AssessLOS() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
#ifndef EA_BUILD_A124
    if (((ai->GetFocus() == SoundAI::kPursuitFlow) || (ai->GetFocus() == SoundAI::kLost)) && (mTracking & Lost)) {
        mTracking &= ~Lost;
    }
#endif

    if ((ai->GetTimeInView() > 0.0f) && (mTracking & Lost)) {
        copList losList;
        losList.reserve(ai->GetActors().size());
        {
            copMap::const_iterator iter = ai->GetActors().begin();
            while (iter != ai->GetActors().end()) {
                EAXCop *cop = iter->cop;
                if (cop->IsActive() && cop->HasLOS()) {
                    losList.push_back(cop);
                }
                ++iter;
            }
        }

        if (losList.size() > 1) {
            std::sort(losList.begin(), losList.end());
        }

        if (!losList.empty()) {
            EAXCop *closest = losList.front();
            closest->RegainVisual();
            mTracking &= ~Lost;
        }
    }
}

#ifdef EA_BUILD_A124
void Observer::AssessAirborne() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (ai->NumCopsWithLOS() > 0) {
        float pct_tractive_tires;
        bool flipped = false;
        IPlayer *player = IPlayer::First(PLAYER_LOCAL);
        if (player) {
            ISimable *simable = player->GetSimable();
            IRigidBody *irb = simable->GetRigidBody();
            ISuspension *suspension;
            if (simable && simable->QueryInterface(&suspension) && suspension->GetNumWheels()) {
                pct_tractive_tires = static_cast<float>(suspension->GetNumWheelsOnGround()) / suspension->GetNumWheels();
                mT_airborne = pct_tractive_tires < 0.1f ? mT_airborne : WorldTimer;
            } else {
                mT_airborne = WorldTimer;
            }
            UMath::Vector3 upVec;
            irb->GetUpVector(upVec);
            if (upVec.y < 0.0f) {
                flipped = true;
            }
            if (!flipped) {
                mT_flipped = WorldTimer;
            }
        }

        // Los stabs de la A124 no dan otro float que pct_tractive_tires (f1) y t_flipped (f3): el tiempo en el aire
        // reutiliza la primera variable, que vive en f1 tambien en las comparaciones.
        pct_tractive_tires = (WorldTimer - mT_airborne).GetSeconds();
        float t_flipped = (WorldTimer - mT_flipped).GetSeconds();
        if (pct_tractive_tires > 0.0f && !flipped) {
            if (pct_tractive_tires >= ai->GetTune().HangTimeForCommentary()) {
                EAXCop *cop = ai->FindClosestCop(true, true);
                if (cop) {
                    cop->SuspectAirborne(ai->GetPlayerSpeed() > 65.0f ? Csis::Type_intensity_High : Csis::Type_intensity_Normal);
                }
            }
        } else if (flipped) {
            if (t_flipped >= 1.0f) {
                EAXCop *cop = ai->FindClosestCop(true, true);
                if (cop) {
                    cop->SuspectRollover(ai->GetPlayerSpeed() > 65.0f ? Csis::Type_intensity_High : Csis::Type_intensity_Normal);
                }
            }
        }
    }
}
#else
void Observer::NotifyAirborne(float alt, float t) {
    SoundAI *ai = SoundAI::Get();

    if (ai->NumCopsWithLOS() > 0 && alt >= ai->GetTune().MinHeightAirborne()) {
        if (t >= ai->GetTune().HangTimeForCommentary()) {
            EAXCop *cop = ai->FindClosestCop(true, true);
            if (cop) {
                cop->SuspectAirborne(ai->GetPlayerSpeed() >= ai->GetTune().SpeedThreshFlyFlipIntensity()
                                         ? Csis::Type_intensity_High
                                         : Csis::Type_intensity_Normal);
            }
        }
    }
}

void Observer::AssessFlippage() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (ai->NumCopsWithLOS() > 0) {
        bool flipped = false;
        IPlayer *player = IPlayer::First(PLAYER_LOCAL);
        if (player) {
            ISimable *simable = player->GetSimable();
            IRigidBody *irb = simable->GetRigidBody();
            UMath::Vector3 upVec;
            irb->GetUpVector(upVec);
            flipped = upVec.y < 0.0f;
            if (!flipped) {
                mT_flipped = WorldTimer;
            }
        }

        float t_flipped = (WorldTimer - mT_flipped).GetSeconds();
        if (flipped && (t_flipped >= ai->GetTune().FlipTimeForCommentary())) {
            EAXCop *cop = ai->FindClosestCop(true, true);
            if (cop) {
                cop->SuspectRollover(ai->GetPlayerSpeed() > ai->GetTune().SpeedThreshFlyFlipIntensity() ? Csis::Type_intensity_High : Csis::Type_intensity_Normal);
            }
        }
    }
}
#endif

void Observer::Assess180() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (ai->NumCopsWithLOS() > 0) {
        float fwDot = CalcFWVec_Road_Car();
        if ((fwDot >= 0.7f) && ((mTracking & UTurnResult) == 0)) {
            mT_unstable = WorldTimer;
        }

        static UMath::Vector3 fw0 = UMath::Vector3::kZero;
#ifndef EA_BUILD_A124
        static UMath::Vector3 startpos = UMath::Vector3::kZero;
#endif
        static char datapts = 0;

        if (fwDot < 0.7f) {
            if ((mTracking ^ UTurn) & UTurn) {
                if ((mTracking & UTurnResult) == 0) {
                    mDotTrack = fwDot;
                    fw0 = mFwRoad;
                    mTracking |= UTurn;
                    datapts = 0;
#ifndef EA_BUILD_A124
                    startpos = ai->GetPlayerPos();
#endif
                }
            }
        }

            if ((mTracking & UTurn) == 0) {
                return;
            }
            if (fwDot > -0.7f) {
                if ((mTracking & UTurnResult) == 0) {
                    if (fwDot > mDotTrack) {
                        mTracking = mTracking & ~UTurn;
                        mT_unstable = WorldTimer;
                        datapts = 0;
#ifndef EA_BUILD_A124
                        startpos = UMath::Vector3::kZero;
#endif
                    } else {
                        mDotTrack = fwDot;
                        datapts++;
                    }
                }
            }

            if (fwDot <= -0.7f) {
                if (datapts >= ai->GetTune().MinContigFramesFor180()) {
                    if ((mTracking & UTurnResult) == 0) {
#ifndef EA_BUILD_A124
                        UMath::Vector3 endpos = ai->GetPlayerPos();
                        float dist_from_start = UMath::Distance(startpos, endpos);
                        if (dist_from_start > ai->GetTune().MaxRangeFor180()) {
                            mTracking &= ~UTurn;
                            mT_unstable = WorldTimer;
                            datapts = 0;
                            startpos = UMath::Vector3::kZero;
                        } else
#endif
                        {
                            mTracking |= UTurnResult;
                            mT_unstable = WorldTimer;
                        }
                    }
                }
            }

            if ((mTracking & UTurnResult) != 0) {
                // El DWARF del original pone un Timer::operator= DENTRO del
                // bloque de `cop` y otro (rango vacio) antes de GetTune: la
                // cola estaba escrita DOS veces y el cross-jumping borro la
                // primera. Y llama a VU0_v3dotprod sin el envoltorio Dot.
                float delta_dot = VU0_v3dotprod(mFwPlayer, fw0);
                float t_unstable = (WorldTimer - mT_unstable).GetSeconds();
                if (delta_dot > -0.7f) {
                    mTracking &= ~UTurn;
                    mTracking &= ~UTurnResult;
                    mT_unstable = WorldTimer;
                    datapts = 0;
                } else if (t_unstable > ai->GetTune().MaxTimeFor180()) {
                    EAXCop *cop = ai->FindClosestCop(true, true);
                    if (cop) {
                        cop->SuspectUTurn();
                        mTracking &= ~UTurn;
                        mTracking &= ~UTurnResult;
                        mT_unstable = WorldTimer;
                        datapts = 0;
                    }
                }
            }
    } else {
        mTracking &= ~UTurn;
        mTracking &= ~UTurnResult;
        mT_unstable = WorldTimer;
    }
}

void Observer::AssessOutcome() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (ai->GetFocus() != SoundAI::kStrategyFlow) {
        return;
    }
    if (ai->GetPursuitState() != SoundAI::kActive) {
        return;
    }

    {
        float t_busted = ai->GetPursuit()->TimeUntilBusted();
        if ((t_busted > 0.0f) && (t_busted < 1.0f) || ai->GetPursuit()->IsCollapseActive()) {
            EAXCop *cop = ai->GetRandomActiveCop(1, false);
            if (cop && !cop->IsHeli() && !Manager::IsQueued(static_cast<SPCHType_1_EventID>(0x5A), 4)) {
                cop->AnticipateSuccess();
            }
        }

        if (((mTracking & Outcome) != 0) && (t_busted < 1.0f)) {
            float t_tracking = (WorldTimer - mT_trackingOutcome).GetSeconds();
            if (t_tracking >= ai->GetTune().OutcomeTrackTime()) {
                copList primvisual;
                primvisual.reserve(ai->GetActors().size());
                {
                    copMap::const_iterator iter = ai->GetActors().begin();
                    while (iter != ai->GetActors().end()) {
                        EAXCop *cop = iter->cop;
                        if (cop->IsActive() && cop->HasLOS() && cop->IsPrimary()) {
                            primvisual.push_back(cop);
                        }
                        ++iter;
                    }
                }

                if (primvisual.empty()) {
                    return;
                }

                {
                    bool say_outcome = true;
                    EAXCop *cop;
                    if (primvisual.size() > 1) {
                        std::sort(primvisual.begin(), primvisual.end());
                    }

                    cop = *primvisual.begin();
                    if (mTracking & CarRam) {
                        if (mRamCop) {
                            say_outcome = false;
                            if (mRamCop->GetTimeLastRammed() <= t_tracking) {
                                mRamCop->AnticipateSuccess();
                            } else {
                                mRamCop->AnticipateFail();
                            }
                        }
                        mTracking &= ~CarRam;
                        ai->ClearImmunity();
                    }

                    if (say_outcome) {
                        if (ai->GetPlayerSpeed() >= ai->GetTune().OutcomeFailSpeed()) {
                            cop->OutcomeFail(-1);
                        } else if (!cop->IsHeli()) {
                            cop->AnticipateSuccess();
                        }
                    }

                    mTracking &= ~Outcome;
                }
            }
        }
    }
}

void Observer::AssessBraking() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (ai->NumCopsWithLOS() <= 0) {
        return;
    }

    IPlayer *player = IPlayer::First(PLAYER_LOCAL);
    if (!player) {
        return;
    }

    IInput *input;
    player->GetSimable()->QueryInterface(&input);
    if (!input) {
        return;
    }

    float steer = CalcFWVec_Road_Car();
    float brake = UMath::Clamp(input->GetControls().fBrake, 0.0f, 1.0f);
    float ebrake = UMath::Clamp(input->GetControls().fHandBrake, 0.0f, 1.0f);

    static float speed0 = ai->GetPlayerSpeed();
    static Timer t_brake_start = WorldTimer;

    float t_last_crashed = bMin(ai->GetTimeLastCrashed(), ai->GetTimeLastNailedCop());
    if ((mTracking & Braking) == 0) {
        if ((steer >= 0.7f) && ((brake >= 1.0f) || (ebrake >= 1.0f)) && (ai->GetPlayerSpeed() > 60.0f) && (t_last_crashed > 5.0f)) {
            mTracking |= Braking;
            speed0 = ai->GetPlayerSpeed();
            t_brake_start = WorldTimer;
        }

        if ((mTracking & Braking) == 0) {
            return;
        }
    }

    if ((steer < 0.7f) || ((brake < 0.5f) && (ebrake < 0.5f)) || (t_last_crashed < 5.0f)) {
        mTracking &= ~Braking;
        return;
    }

    {
        float curr_speed = ai->GetPlayerSpeed();
        float pct_decrease = curr_speed / speed0;
        if (pct_decrease < (1.0f - ai->GetTune().CrashSlowdownPct())) {
            EAXCop *cop = ai->FindClosestCop(true, true);
            if (cop) {
                cop->SuspectBrake();
                mTracking &= ~Braking;
            }
        }
    }
}

void Observer::AssessOutrun() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (ai->NumCopsWithLOS() <= 0) {
        return;
    }

    if (ai->GetTimeInView() < 2.0f) {
        return;
    }

    copList visual;
    visual.reserve(ai->GetActors().size());

    copMap::const_iterator iter = ai->GetActors().begin();
    while (iter != ai->GetActors().end()) {
        EAXCop *cop = iter->cop;
        if (cop->IsActive() && cop->HasLOS()) {
            visual.push_back(cop);
        }
        ++iter;
    }

    if (visual.empty()) {
        return;
    }

    unsigned char num_being_outrun = 0;
    copList::iterator i = visual.begin();
    while (i != visual.end()) {
        EAXCop *cop = *i;
        if (cop->GetTimeLastClosing() > 8.0f) {
            num_being_outrun++;
        }
        ++i;
    }

    if ((num_being_outrun == visual.size()) && !ai->AreCopsAhead()) {
        EAXCop *cop = ai->GetRandomActiveCop(0, false);
        if (cop) {
            cop->SuspectOutrun();
        }
    }
}

void Observer::AssessOffroad() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if ((ai->NumCopsWithLOS() < 1) || (ai->GetTimeInView() < 2.0f)) {
        return;
    }
    unsigned int id = ai->GetPlayerOffroadID();
    if (mCurrOffroadID != static_cast<int>(id)) {
        EAXCop *cop = ai->FindClosestCop(true, true);
        if (cop) {
            mCurrOffroadID = static_cast<int>(id);
            Csis::Type_offroad_moment_id speech_id;
            if (MiscSpeech::GetSPAMLocation(id, speech_id)) {
                cop->Offroad(speech_id, (mOffroadHistory & speech_id) != 0);
            }
        }
    }
}

void Observer::MessageEventComplete(const MNotifySpeechStatus &message) {
    ScheduledSpeechEvent *speech = message.GetEvent();
    if (speech == 0) {
        return;
    }

#ifndef EA_BUILD_A124
    static int spkrID911 = 0;
#endif
    mOutcomeIntensity = Csis::Type_intensity_High;
    bool outcome_req = false;

    switch (speech->ID) {
    case kSPCH1_EventID_OffroadMoment: {
        Csis::AnytimeEvents_OffroadMomentStruct *data =
            static_cast<Csis::AnytimeEvents_OffroadMomentStruct *>(speech->GetData());
        if (data != 0) {
            unsigned int offroad_id = data->offroad_moment_id;
            if ((mOffroadHistory & offroad_id) == 0) {
                mOffroadHistory |= offroad_id;
            }
        }
        break;
    }
    case kSPCH1_EventID_CollisionWorld: {
        Csis::AnytimeEvents_CollisionWorldStruct *data =
            static_cast<Csis::AnytimeEvents_CollisionWorldStruct *>(speech->GetData());
        outcome_req = true;
        if (data != 0 && data->world_object_type == 0x40) {
            mOutcomeIntensity = Csis::Type_intensity_Normal;
        }
        break;
    }
    case kSPCH1_EventID_RBEngage:
    case kSPCH1_EventID_ExtraRBEngage:
    case kSPCH1_EventID_HeliBullhornArrest: {
        outcome_req = true;
        SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
        if (ai != 0 && ai->GetRBFlow() != 0) {
            if (ai->GetRBFlow()->HasNailedSpikes()) {
                outcome_req = false;
            }
        }
        break;
    }
    case kSPCH1_EventID_StrategyExecute: {
        Csis::RollingStrategy_StrategyExecuteStruct *data =
            static_cast<Csis::RollingStrategy_StrategyExecuteStruct *>(speech->GetData());
        outcome_req = true;
        if (data != 0) {
            mOutcomeIntensity = static_cast<Csis::Type_intensity>(data->intensity);
        }
        break;
    }
    case kSPCH1_EventID_Arrest: {
        Csis::Arrest_ArrestStruct *data = static_cast<Csis::Arrest_ArrestStruct *>(speech->GetData());
        outcome_req = true;
        if (data != 0) {
            mOutcomeIntensity = static_cast<Csis::Type_intensity>(data->intensity);
        }
        break;
    }
    case kSPCH1_EventID_BullhornArrest: {
        Csis::Arrest_BullhornArrestStruct *data = static_cast<Csis::Arrest_BullhornArrestStruct *>(speech->GetData());
        outcome_req = true;
        if (data != 0) {
            mOutcomeIntensity = static_cast<Csis::Type_intensity>(data->intensity);
        }
        break;
    }
    case kSPCH1_EventID_CollWorld_Flip: {
        Csis::AnytimeEvents_CollWorld_FlipStruct *data =
            static_cast<Csis::AnytimeEvents_CollWorld_FlipStruct *>(speech->GetData());
        outcome_req = true;
        if (data != 0) {
            mOutcomeIntensity = static_cast<Csis::Type_intensity>(data->intensity);
        }
        break;
    }
    case kSPCH1_EventID_IntentToRam: {
        Csis::AnytimeEvents_IntentToRamStruct *data = static_cast<Csis::AnytimeEvents_IntentToRamStruct *>(speech->GetData());
        if (data != 0) {
            if (data->speaker_id == 2) {
                mTracking &= ~CarRam;
            } else {
                SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
                mTracking |= CarRam;
                if (ai != 0) {
                    mRamCop = ai->GetCop(data->speaker_id);
                    if (mRamCop != 0) {
                        ai->MakeCopsImmune();
                    }
                }
            }
            outcome_req = true;
            mOutcomeIntensity = static_cast<Csis::Type_intensity>(data->intensity);
        }
        break;
    }
    case kSPCH1_EventID_LostVisual:
    case kSPCH1_EventID_LostSuspect:
    case kSPCH1_EventID_HeliLostVisual:
        if ((mTracking & Lost) == 0) {
            mTracking |= Lost;
        }
        break;
#ifndef EA_BUILD_A124
    case kSPCH1_EventID_ReInitPursuit:
        if ((mTracking & Lost) != 0) {
            mTracking &= ~Lost;
        }
        if (Manager::IsQueued(kSPCH1_EventID_RegainVisual, 4)) {
            Manager::RecallSpeechEvent(kSPCH1_EventID_RegainVisual);
        }
        break;
    case kSPCH1_EventID_Disp911Report:
        spkrID911 = MiscSpeech::MoreDetails(0);
        if (UTL::Collections::Singleton<SoundAI>::Get() != 0) {
            UTL::Collections::Singleton<SoundAI>::Get()->Force911State();
        }
        break;
    case kSPCH1_EventID_MoreDetails: {
        SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
        if (ai != 0 && ai->GetPursuitState() != SoundAI::kActive) {
            if (ai->GetPlayerCarColor() != 0 && MiscSpeech::IsVehicleTypeOK() && bRandom(1.0f) > 0.5f) {
                ai->GetDispatch()->VehicleDescription();
            } else {
                ai->GetDispatch()->NoVehicleDescription();
            }
            if (spkrID911 > 0) {
                MiscSpeech::Unit911Reply(spkrID911);
            }
        }
        break;
    }
#endif
    default:
        break;
    }

    if (outcome_req) {
        mTracking |= Outcome;
        mT_trackingOutcome = WorldTimer;
    }
}

void Observer::MessageBlewPastCop(const MGamePlayMoment &message) {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if ((ai->GetFocus() == SoundAI::kStrategyFlow) &&
        ((ai->GetPursuitState() == SoundAI::kActive) || (ai->GetPursuitState() == SoundAI::kSearching))
#ifndef EA_BUILD_A124
        && (ai->NumCopsWithLOS() < 2)
#endif
    ) {
        unsigned int copID = message.GethSimable();
        for (copMap::const_iterator i = ai->GetActors().begin(); i != ai->GetActors().end(); ++i) {
            EAXCop *cop = i->cop;
            ISimable *simable = ISimable::FindInstance(cop->GetHandle());
            if (simable && (simable->GetWorldID() == copID)) {
                IVehicle *vehicle = nullptr;
                if (simable->QueryInterface(&vehicle)) {
#ifdef EA_BUILD_A124
                    // La A124 no mira si el coche es de un control de carretera.
                    if (ai->GetPlayerSpeed() > (MPS2MPH(vehicle->GetSpeed()) + 50.0f)) {
                        cop->Spotted();
                    }
#else
                    float cop_speed = MPS2MPH(vehicle->GetSpeed());
                    if (ai->GetPlayerSpeed() > (cop_speed + 50.0f)) {
                        bool cop_in_rb = false;
                        IRoadBlock *block = ai->GetRoadblock();
                        if (block) {
                            IVehicle *car = block->IsComprisedOf(simable->GetOwnerHandle());
                            if (car) {
                                cop_in_rb = true;
                            }
                        }

                        if (!cop_in_rb) {
                            cop->Spotted();
                        }
                    }
#endif
                }
            }
        }
    }
}

void Observer::MessageTunnelUpdate(const MMiscSound &message) {
    mTunnel = message.GetSoundID() > 0;
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (ai && mTunnel && ai->GetHeli() && ai->GetHeli()->IsActive() && ai->GetHeli()->HasLOS()) {
        ai->GetHeli()->HazardAlert(Csis::Type_heli_hazard_alert_type_approaching_tunnel);
    }
}

void Observer::MessageGamePlayMoment(const MGamePlayMoment &message) {
    if (message.GetAttribKey() == 0xa1794c23) {
        SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
        if (ai->GetFocus() != SoundAI::kStrategyFlow) {
            return;
        }

        bool player_started = false;
        ISimable *simable = ISimable::FindInstance(reinterpret_cast<HSIMABLE>(message.GethSimable()));

        if (simable) {
            ICause *causer = ICause::FindInstance(simable->GetCausality());
            if (IVehicle::FindIf(VEHICLE_PLAYERS, ComComparator(causer)) != nullptr) {
                player_started = true;
            }
        }

        if (player_started) {
            EAXCop *closest = ai->FindClosestCop(false, true);
            if (closest && closest->HasLOS()) {
                closest->Collision(0xd, 1.0f, closest);
            }
        }

        mT_gasstationexpl = WorldTimer;
        mGasStationPos = UMath::Vector4To3(message.GetPosition());
    }
}

void Observer::SetMask(unsigned int mask) {
    mObserveMask = mask;
}

} // namespace Speech
