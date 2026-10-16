#include "PursuitFlow.h"

#include "Speed/Indep/Src/EAXSound/Stream/SpeechManager.hpp"
#include "Speed/Indep/Src/EAXSound/Stream/SpeechModule.hpp"
#include "Speed/Indep/Src/EAXSound/Stream/GameSpeech.hpp"
#include "Speed/Indep/Src/Speech/EAXCop.h"
#include "Speed/Indep/Src/Speech/EAXDispatch.h"
#include "Speed/Indep/Src/Speech/MiscSpeech.h"
#include "Speed/Indep/Src/Speech/SoundAI.h"
#include "Speed/Indep/Src/Gameplay/GRaceStatus.h"
#include "Speed/Indep/Libs/Support/Utility/UCollections.h"
#include "Speed/Indep/bWare/Inc/bMath.hpp"

#ifdef MILESTONE_BUILD
#include "Speed/Indep/Src/Frontend/DebugMenu.h"
#endif

extern const int SPEECHFLOW_DISPLAY;

namespace Speech {

PursuitFlow::PursuitFlow() {
    mCauseofPursuit = kUnknown;
    mFirstOnScene = nullptr;
    mReqRestart = false;
    mAVSUnitRammedSaid = false;
    mSpeaker = 0;
    mMsgNotifyEventCompletion =
        Hermes::Handler::Create<MNotifySpeechStatus, PursuitFlow, PursuitFlow>(this, &PursuitFlow::MessageEventComplete,
                                                                              0x20D60DBF, 0);
}

PursuitFlow::~PursuitFlow() {
    if (mMsgNotifyEventCompletion != nullptr) {
        Hermes::Handler::Destroy(mMsgNotifyEventCompletion);
    }
}

void PursuitFlow::OnCopRemoved(EAXCop *cop) {
    if (mFirstOnScene != nullptr) {
        if (mFirstOnScene->GetHandle() == cop->GetHandle()) {
            mFirstOnScene = nullptr;
        }
    }
}

void PursuitFlow::Update() {
    int y;
    int x;

#ifdef MILESTONE_BUILD
    // Pantalla de depuracion de la A124 (cadenas de su .rodata).
    x = -200;
    y = -95;
    if (SPEECHFLOW_DISPLAY) {
        ScreenPrintf(x, y, 1.0f, "PURSUIT FLOW %s", mBusy ? "busy" : "");
        y += 15;
        switch (mCauseofPursuit) {
        case k911Reported:
            ScreenPrintf(x, y, 1.0f, "911 was reported");
            y += 15;
            break;
        case kCopAssaulted:
            ScreenPrintf(x, y, 1.0f, "Assaulting an officer");
            y += 15;
            break;
        case kSpotted:
            ScreenPrintf(x, y, 1.0f, "Spotted doing something illegal");
            y += 15;
            break;
        case kScripted:
            ScreenPrintf(x, y, 1.0f, "Pursuit start was scripted");
            y += 15;
            break;
        case kReacquired:
            ScreenPrintf(x, y, 1.0f, "Suspect reacquired");
            y += 15;
            break;
        case kCopAssaultedScripted:
        case kUnknown:
        default:
            ScreenPrintf(x, y, 1.0f, "Unknown cause of pursuit");
            y += 15;
            break;
        }
        SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
        if (ai != nullptr && ai->GetPursuitState() == SoundAI::kActive) {
            switch (ai->GetPursuit()->GetFormationType()) {
            case PIT:
                ScreenPrintf(x, y, 1.0f, "Pit");
                y += 15;
                break;
            case BOX_IN:
                ScreenPrintf(x, y, 1.0f, "Box In");
                y += 15;
                break;
            case ROLLING_BLOCK:
                ScreenPrintf(x, y, 1.0f, "Rolling Block");
                y += 15;
                break;
            case HERD:
                ScreenPrintf(x, y, 1.0f, "Herd");
                y += 15;
                break;
            case FOLLOW:
            case STAGGER_FOLLOW:
                ScreenPrintf(x, y, 1.0f, "Follow");
                y += 15;
                break;
            case HELI_PURSUIT:
                ScreenPrintf(x, y, 1.0f, "Heli Pursuit");
                y += 15;
                break;
            default:
                ScreenPrintf(x, y, 1.0f, "Unknown");
                y += 15;
                break;
            }
        }
    }
#else
    if (SPEECHFLOW_DISPLAY != 0) {
        switch (mCauseofPursuit) {
        case k911Reported:
            y = 0;
            break;
        case kCopAssaulted:
            y = 1;
            break;
        case kSpotted:
            y = 2;
            break;
        case kReacquired:
            y = 3;
            break;
        case kScripted:
            y = 4;
            break;
        default:
            y = 5;
            break;
        }
        SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
        if (ai != nullptr && ai->GetPursuitState() == SoundAI::kActive) {
            switch (ai->GetPursuit()->GetFormationType()) {
            case PIT:
                y = 0;
                break;
            case BOX_IN:
                y = 1;
                break;
            case ROLLING_BLOCK:
                y = 2;
                break;
            case FOLLOW:
            case STAGGER_FOLLOW:
                y = 3;
                break;
            case HELI_PURSUIT:
                y = 4;
                break;
            case HERD:
                y = 5;
                break;
            default:
                y = 6;
                break;
            }
        }
    }
#endif
    switch (mState) {
    case kTransition:
#ifdef MILESTONE_BUILD
        if (SPEECHFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "Transition");
        }
#endif
        break;
    case kCullCheck:
#ifdef MILESTONE_BUILD
        if (SPEECHFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "CullCheck");
        }
#endif
        CullCheck();
        return;
    case kCloseInCheck:
#ifdef MILESTONE_BUILD
        if (SPEECHFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "CloseInCheck");
        }
#endif
        CloseInCheck();
        return;
    case kPrimaryBranch:
#ifdef MILESTONE_BUILD
        if (SPEECHFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "PrimaryBranch");
        }
#endif
        PrimaryBranch();
        return;
    case kSpotterBranch:
#ifdef MILESTONE_BUILD
        if (SPEECHFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "SpotterBranch");
        }
#endif
        SpotterBranch();
        return;
    case kScriptedBranch:
#ifdef MILESTONE_BUILD
        if (SPEECHFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "ScriptedBranch");
        }
#endif
        ScriptedBranch();
        return;
    case kWaitForSpotter:
#ifdef MILESTONE_BUILD
        if (SPEECHFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "kWaitForSpotter");
        }
#endif
        SpotterWait();
        return;
    case kLostWhileSpotWait:
#ifdef MILESTONE_BUILD
        if (SPEECHFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "kLostWhileSpotWait");
        }
#endif
        LostWhileSpotterWait();
        return;
    case kPlayerStopped:
#ifdef MILESTONE_BUILD
        if (SPEECHFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "Player Stopped");
        }
#endif
        PlayerStopped();
        return;
    case kBailout:
#ifdef MILESTONE_BUILD
        if (SPEECHFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "Bailout");
        }
#endif
        Bailout();
        return;
    case kChangeTarget:
#ifdef MILESTONE_BUILD
        if (SPEECHFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "ChangeTarget");
        }
#endif
        ChangeTarget();
        return;
    case kTerminal:
#ifdef MILESTONE_BUILD
        if (SPEECHFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "Terminal");
        }
#endif
        Terminal();
        return;
    default:
        ChangeStateTo(kCullCheck);
        break;
    }
}

void PursuitFlow::CullCheck() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (ai->GetPursuitState() == SoundAI::kActive) {
        ChangeStateTo(kCloseInCheck);
    }
}

void PursuitFlow::Reset() {
    mFirstOnScene = nullptr;
    ChangeStateTo(kTransition);
    mState = kCullCheck;
    mBusy = 0;
    mCauseofPursuit = kUnknown;
}

void PursuitFlow::Reacquire() {
#ifndef EA_BUILD_A124 // la A124 no mira el 911 (lineas 217-218: dos sentencias)
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (ai != nullptr && !ai->Is911Active()) {
#endif
        mCauseofPursuit = kReacquired;
        ChangeStateTo(kPrimaryBranch);
#ifndef EA_BUILD_A124
    }
#endif
}

bool PursuitFlow::RequiresRestart() {
    if (Speech::Manager::IsCopSpeechPlaying((SPCHType_1_EventID)0x9A)) {
        Speech::Module *cop_speech = Speech::Manager::GetSpeechModule(1);
        if (cop_speech != nullptr) {
            Speech::GameSpeech *gamespeech = static_cast<Speech::GameSpeech *>(cop_speech);
            Speech::ScheduledSpeechEvent *avs = gamespeech->GetCurrentEvent();
            if (avs != nullptr) {
                Csis::Setup_AttmptVehStpStruct *data = static_cast<Csis::Setup_AttmptVehStpStruct *>(avs->GetData());
                if (data != nullptr && data->pursuit_type == 0x10) {
                    mCauseofPursuit = kCopAssaulted;
                    mAVSUnitRammedSaid = true;
                }
            }
        }
    }
    if (mAVSUnitRammedSaid) {
        return false;
    }
    mReqRestart = true;
    mBusy = 0;
    return true;
}

void PursuitFlow::CloseInCheck() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    const Speech::copMap &cops = ai->GetActors();
#ifdef EA_BUILD_A124
    if (ai->GetPursuitState() != SoundAI::kActive || cops.size() == 0) {
#else
    if (ai->GetPursuitState() == SoundAI::kInactive || cops.size() == 0) {
#endif
        Reset();
        return;
    }
    IPursuit *pursuit = ai->GetPursuit();
    if (pursuit == nullptr) {
        return;
    }
    if (ai->GetPursuitDuration() < ai->GetTune().PursuitInitDelay()) {
        return;
    }
    Speech::BlowByRecord &recent = ai->GetRecentBlowby();
    float lastblowby = (WorldTimer - recent.timestamp).GetSeconds();
    if (recent.distance < ai->GetTune().RangeForSpotterBranch() && recent.speed >= ai->GetTune().SpeedDiffForBlowby()
        && lastblowby < ai->GetTune().BlowbyInterval() && ai->GetPlayerSpeed() >= 35.0f && !ai->AreCopsAhead()) {
        mCauseofPursuit = kSpotted;
    }
    if (ai->Is911Active()) {
        mCauseofPursuit = k911Reported;
    }
    bool pursuitRace = false;
    if (GRaceStatus::Get().GetRaceParameters() != nullptr) {
        pursuitRace = GRaceStatus::Get().GetRaceParameters()->GetIsPursuitRace();
    }
    if (pursuitRace) {
        mCauseofPursuit = kScripted;
    }
    bool officer_assaulted = ai->GetTimeLastNailedCop() < 10.0f;
    if (officer_assaulted && mCauseofPursuit != kScripted) {
        mCauseofPursuit = kCopAssaulted;
    }
    switch (mCauseofPursuit) {
    case kSpotted:
        ChangeStateTo(kSpotterBranch);
        break;
    case kCopAssaulted:
    case kCopAssaultedScripted:
    case kUnknown:
        ChangeStateTo(kPrimaryBranch);
        break;
    case k911Reported:
        ChangeStateTo(kWaitForSpotter);
        break;
    case kScripted:
    default:
        ChangeStateTo(kScriptedBranch);
        break;
    }
}

void PursuitFlow::PrimaryBranch() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (ai->GetPursuitState() == SoundAI::kInactive) {
        ChangeStateTo(kBailout);
        return;
    }
    if (ai->GetPursuitState() == SoundAI::kOtherTarget) {
        ChangeStateTo(kChangeTarget);
        return;
    }
    if (mBusy == 0) {
        EAXCop *leader = ai->FindClosestCop(false, false);
        if (leader == nullptr) {
            ChangeStateTo(kCullCheck);
            return;
        }
        if (leader->IsPrimary() == 0) {
            bool converted = ai->MakeLeader(leader);
            if (!converted) {
                ChangeStateTo(kCullCheck);
                return;
            }
        }
        if (leader->IsHeli()) {
            ChangeStateTo(kCullCheck);
            return;
        }
        if (mCauseofPursuit == kReacquired) {
            leader->ReinitiatePursuit();
#ifdef EA_BUILD_A124
            if (ai->NumPursuits() > 2) {
                if (bRandom(1.0f) > 0.5f && ai->IsHeadingValid()) {
                    ai->GetDispatch()->PursuitEscalation();
                } else {
                    ai->GetDispatch()->PursuitEscalationGeneric();
                }
            }
#endif
        } else {
            if (mReqRestart != 0) {
                if (mSpeaker > 0) {
                    int orig_spkr_id = leader->GetSpeakerID();
                    if (orig_spkr_id != mSpeaker) {
                        EAXCop *orig_spkr = ai->GetCop(mSpeaker);
                        if (orig_spkr != nullptr) {
                            leader->SwapVoices(orig_spkr);
                        }
                    }
                }
                mReqRestart = false;
            }
            leader->AttemptVehicleStop();
            mSpeaker = leader->GetSpeakerID();
            if (ai->GetLeader() == nullptr) {
                ai->MakeLeader(leader);
            }
            if (ai->GetDispatch() != nullptr) {
                ai->GetDispatch()->GoAhead();
            }
            if (mCauseofPursuit != kCopAssaulted && mCauseofPursuit != kCopAssaultedScripted) {
                leader->VehicleReport();
            }
            if (mCauseofPursuit == kCopAssaultedScripted) {
                ChangeStateTo(kTerminal);
                return;
            }
            if (ai->NumPursuits() > 1) {
                if (ai->NumPursuits() > 2) {
                    EAXCop *rp = ai->GetRandomActiveCop(1, false);
                    if (rp != nullptr) {
                        if (rp->GetHandle() != leader->GetHandle()) {
                            rp->DriverHistory();
                        } else {
                            ai->GetDispatch()->DriverHistory();
                        }
                    }
                } else {
                    leader->SuspectConfirmed();
                }
                mCauseofPursuit = kReacquired;
            }
        }
        mBusy = mBusy + 1;
    } else {
        if (ai->GetPlayerStopTime() >= ai->GetTune().MinTimeConsideredStopped() &&
            (mCauseofPursuit != kCopAssaulted && mCauseofPursuit != kCopAssaultedScripted)) {
            mBusy = 0;
            ChangeStateTo(kPlayerStopped);
            return;
        }
        if (Speech::Manager::IsCopSpeechBusy() == 0) {
            if (mBusy != 0) {
                if (mReqRestart == 0) {
                    ChangeStateTo(kTerminal);
                    mBusy = 0;
                }
            }
        }
    }
}

void PursuitFlow::PlayerStopped() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (ai->GetPursuitState() == SoundAI::kInactive) {
        ChangeStateTo(kBailout);
        return;
    }
    if (ai->GetPursuitState() == SoundAI::kOtherTarget) {
        ChangeStateTo(kChangeTarget);
        return;
    }
    if (mBusy == 0) {
        if (Speech::Manager::HasBeenSaid((SPCHType_1_EventID)0x9A) || Speech::Manager::IsCopSpeechPlaying((SPCHType_1_EventID)0x9A)
            || Speech::Manager::HasBeenSaid((SPCHType_1_EventID)0x3A) || Speech::Manager::IsCopSpeechPlaying((SPCHType_1_EventID)0x3A)
            || Speech::Manager::HasBeenSaid((SPCHType_1_EventID)0x3B) || Speech::Manager::IsCopSpeechPlaying((SPCHType_1_EventID)0x3B)
            || Speech::Manager::HasBeenSaid((SPCHType_1_EventID)0xAE) || Speech::Manager::IsCopSpeechPlaying((SPCHType_1_EventID)0xAE)) {
            if (ai->GetPlayerStopTime() > 1.0f || ai->GetPlayerSpeed() < 10.0f) {
                if (ai->GetLeader() != nullptr) {
                    ai->GetLeader()->InterruptComposedLow();
                    mBusy = mBusy + 1;
                }
            }
        }
    } else {
        if (ai->GetPlayerStopTime() >= ai->GetTune().MinTimeConsideredStopped()) {
            if (ai->GetLeader() != nullptr) {
                ai->GetLeader()->InterruptComposedLow();
                ChangeStateTo(kTerminal);
                mBusy = 0;
                return;
            }
        }
    }
    if (mBusy == 0) {
        ChangeStateTo(kTransition);
        mBusy = 0;
    }
}

void PursuitFlow::SpotterBranch() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (ai->GetPursuitState() != SoundAI::kActive && ai->GetPursuitState() != SoundAI::kSearching) {
        if (ai->GetPursuitState() == SoundAI::kInactive) {
            ChangeStateTo(kBailout);
            return;
        }
        if (ai->GetPursuitState() == SoundAI::kOtherTarget) {
            ChangeStateTo(kChangeTarget);
            return;
        }
        return;
    }
    if (mBusy == 0) {
        if (ai->IsHighIntensity() || ai->GetLastInfraction() == 0x40) {
            EAXCop *cop = ai->FindClosestCop(false, true);
            if (cop == nullptr) {
                return;
            }
            mFirstOnScene = cop;
            cop->SpotterWanted();
        } else {
            EAXCop *cop = ai->FindClosestCop(false, false);
            if (cop == nullptr) {
                return;
            }
            mFirstOnScene = cop;
            cop->Spotter();
        }
        if (bRandom(1.0f) > 0.5f) {
            mFirstOnScene->VehicleReport();
        }
        if (bRandom(1.0f) > 0.5f) {
            ai->GetDispatch()->GoAhead();
        } else {
            ai->GetDispatch()->Ack();
        }
        EAXCop *cop = ai->FindClosestCop(false, true);
        if (mFirstOnScene != nullptr) {
            if (cop != nullptr) {
                if (cop->GetSpeakerID() != mFirstOnScene->GetSpeakerID()) {
                    cop->Reply911();
                }
            }
        } else {
            if (cop != nullptr) {
                cop->Reply911();
            }
        }
        mBusy = mBusy + 1;
    } else {
        if (ai->GetPlayerStopTime() >= ai->GetTune().MinTimeConsideredStopped() &&
            (mCauseofPursuit != kCopAssaulted && mCauseofPursuit != kCopAssaultedScripted)) {
            mBusy = 0;
            ChangeStateTo(kPlayerStopped);
            return;
        }
        if (Speech::Manager::IsCopSpeechBusy() == 0) {
            if (mBusy != 0) {
                if (ai->NumCopsWithLOS() > 0) {
                    ChangeStateTo(kWaitForSpotter);
                } else {
                    if (mFirstOnScene != nullptr) {
                        mFirstOnScene->LostVisual();
                    }
                    ChangeStateTo(kLostWhileSpotWait);
                }
                mBusy = 0;
            }
        }
    }
}

void PursuitFlow::ScriptedBranch() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (mBusy == 0) {
        if (ai->IsHeadingValid()) {
            ai->GetDispatch()->PursuitEscalation();
        } else {
            ai->GetDispatch()->PursuitEscalationGeneric();
        }
        if (ai->GetPlayerCarColor() != 0) {
            if (MiscSpeech::IsVehicleTypeOK()) {
                ai->GetDispatch()->VehicleDescription();
            }
        }
        mBusy = mBusy + 1;
    } else {
        if (Speech::Manager::IsCopSpeechBusy() == 0) {
            if (mBusy != 0) {
                ChangeStateTo(kTerminal);
                mBusy = 0;
            }
        }
    }
}

void PursuitFlow::LostWhileSpotterWait() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (mBusy == 0) {
        if (ai->GetPursuitState() == SoundAI::kInactive) {
            ChangeStateTo(kBailout);
            return;
        }
        if (ai->GetPursuitState() == SoundAI::kOtherTarget) {
            ChangeStateTo(kChangeTarget);
            return;
        }
        if (ai->NumCopsWithLOS() > 0) {
            ChangeStateTo(kWaitForSpotter);
            mBusy = 0;
            return;
        }
    }
    if (Speech::Manager::IsCopSpeechBusy() == 0 && mBusy != 0) {
        Reset();
        ChangeStateTo(kCullCheck);
        mBusy = 0;
    }
}

void PursuitFlow::SpotterWait() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (mBusy == 0) {
        if (ai->GetPursuitState() != SoundAI::kActive) {
            ChangeStateTo(kLostWhileSpotWait);
            return;
        }
        EAXCop *cansee = ai->FindClosestCop(true, true);
        if (cansee == nullptr) {
            mBusy = mBusy + 1;
            return;
        }
        if (!ai->MakeLeader(cansee)) {
            mBusy = mBusy + 1;
            return;
        }
        if (mCauseofPursuit != kScripted) {
#ifdef EA_BUILD_A124
            if ((mFirstOnScene != nullptr) && (mFirstOnScene->GetSpeakerID() != cansee->GetSpeakerID())) {
                if (ai->AreCopsAhead()) {
#else
            if ((mCauseofPursuit == k911Reported) || (mCauseofPursuit != k911Reported &&
                (mFirstOnScene != nullptr) && (mFirstOnScene->GetSpeakerID() != cansee->GetSpeakerID()))) {
                if (ai->AreCopsAhead() || cansee->IsPrimary() == 0) {
#endif
                    cansee->Spotted();
                } else {
                    cansee->SpotterReply();
                }
            }
#ifndef EA_BUILD_A124
            if (mCauseofPursuit != k911Reported)
#endif
            {
                if (bRandom(1.0f) > 0.5f && ai->IsHeadingValid()) {
                    ai->GetDispatch()->PursuitEscalation();
                } else {
                    ai->GetDispatch()->PursuitEscalationGeneric();
                }
            }
        }
        mBusy = mBusy + 1;
    } else {
        if (ai->GetPlayerStopTime() >= ai->GetTune().MinTimeConsideredStopped() &&
            (mCauseofPursuit != kCopAssaulted && mCauseofPursuit != kCopAssaultedScripted)) {
            mBusy = 0;
            ChangeStateTo(kPlayerStopped);
            return;
        }
        if (Speech::Manager::IsCopSpeechBusy() == 0) {
            if (mBusy != 0) {
                ChangeStateTo(kTerminal);
                mBusy = 0;
            }
        }
    }
}

void PursuitFlow::Bailout() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (ai != nullptr) {
        EAXCop *cop = ai->GetRandomActiveCop(0, false);
        if (cop != nullptr) {
            cop->LoBailout();
        }
        ChangeStateTo(kTransition);
        Reset();
    }
}

void PursuitFlow::ChangeTarget() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (ai != nullptr) {
        EAXCop *closest = ai->FindClosestCop(false, true);
        if (closest != nullptr) {
            closest->FocusChange();
        }
        ai->GetDispatch()->PursuitEscalation();
        ChangeStateTo(kTransition);
        Reset();
    }
}

void PursuitFlow::Terminal() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (mBusy == 0) {
        EAXCop *leader = ai->GetLeader();
        if (ai->GetPlayerSpeed() >= 60.0f && leader != nullptr && (mCauseofPursuit != kSpotted && mCauseofPursuit != k911Reported
            && mCauseofPursuit != kScripted && mCauseofPursuit != kReacquired && mCauseofPursuit != kCopAssaultedScripted
            && mCauseofPursuit != kCopAssaulted)) {
            leader->InitiatePursuit();
        }
        if (!ai->IsHighIntensity() && leader != nullptr && (mCauseofPursuit != kCopAssaulted
            && mCauseofPursuit != kCopAssaultedScripted)) {
            leader->LocationReport();
        }
        if (ai->GetHavoc() >= ai->GetTune().MinHavocForSuspectBehavior() && (mCauseofPursuit != kCopAssaulted
            && mCauseofPursuit != kCopAssaultedScripted)) {
            EAXCop *cansee = ai->FindClosestCop(true, true);
            if (cansee != nullptr) {
                cansee->SuspectBehavior();
            }
        }
        mBusy = mBusy + 1;
    } else {
        if (Speech::Manager::IsCopSpeechBusy() == 0) {
            if (mBusy != 0) {
                ChangeStateTo(kTransition);
                mBusy = 0;
                mFirstOnScene = nullptr;
            }
        }
    }
}

bool PursuitFlow::IsTransitionable() {
    return mState == kTransition ? true : false;
}

void PursuitFlow::MessageEventComplete(const MNotifySpeechStatus &message) {
    Speech::ScheduledSpeechEvent *speech = reinterpret_cast<Speech::ScheduledSpeechEvent *>(message.GetEvent());
    if (speech != nullptr) {
        switch (speech->ID) {
        case 0x9A: {
            Csis::Setup_AttmptVehStpStruct *data = static_cast<Csis::Setup_AttmptVehStpStruct *>(speech->GetData());
            if (data == nullptr) {
                return;
            }
            if (data->pursuit_type == 0x10) {
                mAVSUnitRammedSaid = true;
            }
            break;
        }
#ifndef EA_BUILD_A124
        case 0xA0: {
            SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
            if (ai != nullptr && ai->NumPursuits() > 2) {
                if (bRandom(1.0f) > 0.5f && ai->IsHeadingValid()) {
                    ai->GetDispatch()->PursuitEscalation();
                } else {
                    ai->GetDispatch()->PursuitEscalationGeneric();
                }
            }
            break;
        }
#endif
        default:
            break;
        }
    }
}

}; // namespace Speech
