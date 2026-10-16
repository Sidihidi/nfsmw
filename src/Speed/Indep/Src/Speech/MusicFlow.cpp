#include "MusicFlow.h"
#include "SoundAI.h"
#include "Speed/Indep/Src/EAXSound/EAXSOund.hpp"
#include "Speed/Indep/Src/Frontend/Database/FEDatabase.hpp"
#include "Speed/Indep/Src/Generated/Messages/MControlPathfinder.h"
#include "Speed/Indep/Src/EAXSound/Stream/SpeechManager.hpp"
#include "Speed/Indep/Src/Generated/Messages/MNotifyMusicFlow.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/bWare/Inc/bMath.hpp"
#include "Speed/Indep/Tools/Inc/ConversionUtil.hpp"
#ifdef MILESTONE_BUILD
#include "Speed/Indep/Src/Frontend/DebugMenu.h"
#endif

int gXMP_DOWNSTATE = 0;
// .data:0x80435E98 del original, pegado detras de gXMP_DOWNSTATE y delante de
// _7SoundAI.mRefCount. Solo lo LEE SFXObj_Pathfinder.cpp (zEAXSound2), que lo
// declara extern; sin la definicion zSpeech no enlaza.
int MUSICFLOW_DISPLAY = 0;
namespace Speech {

MusicFlow::MusicFlow()
    :
#ifndef EA_BUILD_A124
      mStartDelay(false), //
      mStartEvent(0), //
#endif
      mTimer(0), //
      mBoostTimer(0), //
#ifndef EA_BUILD_A124
      mT_currPiece(0), //
#endif
      mElapsed(0.0f), //
      mIntensity(0.0f), //
      mAvgNumCopsInForm(0.0f), //
      mAvgNumCopsLOS(0.0f), //
      mAvgPlayerSpeed(0.0f), //
      mAvgPursuitDist(0.0f), //
      mCurrentPart(-1), //
      mRestrained(true), //
      mTopSpeed(0.0f), //
#ifndef EA_BUILD_A124
      mX360UserTunes(false), //
#endif
      mMsgNewPart(Hermes::Handler::Create<MNotifyMusicFlow, MusicFlow, MusicFlow>(
          this, &MusicFlow::MessageNewPart, "PartUpdate", 0)), //
      mMsgInitFlow(Hermes::Handler::Create<MNotifyMusicFlow, MusicFlow, MusicFlow>(
          this, &MusicFlow::MessageInitFlow, "Init", 0)), //
      mMsgTerminate(Hermes::Handler::Create<MNotifyMusicFlow, MusicFlow, MusicFlow>(
          this, &MusicFlow::MessageTerminate, "Terminate", 0)), //
      mMsgDone(Hermes::Handler::Create<MNotifyMusicFlow, MusicFlow, MusicFlow>(
          this, &MusicFlow::MessageDone, "InteractiveDone", 0)) //
#ifndef EA_BUILD_A124
      ,
      mMsgX360UserTunes(Hermes::Handler::Create<MNotifyMusicFlow, MusicFlow, MusicFlow>(
          this, &MusicFlow::MessageX360UserTunes, "X360UserTunes", 0))
#endif
{
    mState = kTransition;
    mBusy = 0;
}

MusicFlow::~MusicFlow() {
    if (mMsgNewPart) {
        Hermes::Handler::Destroy(mMsgNewPart);
    }
    if (mMsgInitFlow) {
        Hermes::Handler::Destroy(mMsgInitFlow);
    }
    if (mMsgTerminate) {
        Hermes::Handler::Destroy(mMsgTerminate);
    }
    if (mMsgDone) {
        Hermes::Handler::Destroy(mMsgDone);
    }
#ifndef EA_BUILD_A124
    if (mMsgX360UserTunes) {
        Hermes::Handler::Destroy(mMsgX360UserTunes);
    }
#endif
}

void MusicFlow::MessageNewPart(const MNotifyMusicFlow &message) {
    mCurrentPart = message.GetPart();

    switch (mCurrentPart) {
    case 4:
        SpeechFlow::ChangeStateTo(kNeutral);
        return;
    case 1:
        SpeechFlow::ChangeStateTo(kLose);
        return;
    case 3:
        SpeechFlow::ChangeStateTo(kWin);
        return;
    case 2:
        SpeechFlow::ChangeStateTo(kElude);
        return;
    case 5:
    case 6:
    case 7:
        SpeechFlow::ChangeStateTo(kTerminal);
        return;
    case -1:
    case 0:
    case 8:
        SpeechFlow::ChangeStateTo(kTerminal);
        return;
    default:
        return;
    }
}

void MusicFlow::MessageInitFlow(const MNotifyMusicFlow &message) {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    IPlayer *player;
    IVehicleAI *vai;

    ChangeStateTo(kWaiting);
    player = IPlayer::First(PLAYER_LOCAL);
    player->GetSimable()->QueryInterface(&vai);
    if (vai) {
        float top_speed_mps = vai->GetTopSpeed();
        mTopSpeed = MPS2MPH(top_speed_mps);
    } else {
        mTopSpeed = 160.0f;
    }

#ifndef EA_BUILD_A124
    mTimer = WorldTimer;
    mStartDelay = true;
    mStartEvent = message.GetPart();
#endif
}

void MusicFlow::MessageTerminate(const MNotifyMusicFlow &) {
    ChangeStateTo(kTerminal);
}

void MusicFlow::MessageDone(const MNotifyMusicFlow &message) {
    Reset();
}

#ifndef EA_BUILD_A124
void MusicFlow::MessageX360UserTunes(const MNotifyMusicFlow &message) {
    mX360UserTunes = message.GetPart() > 0;
}
#endif

// La A124 aun no mira la distancia de la persecucion: vuelve siempre a kLose (lineas 207-213 de su MusicFlow.cpp).
void MusicFlow::Reacquire() {
#ifndef EA_BUILD_A124
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    if (ai) {
#endif
        if (static_cast<unsigned int>(mState - kWin) <= static_cast<unsigned int>(kElude - kWin)) {
#ifndef EA_BUILD_A124
            if (ai->GetPursuitDistance() <= 50.0f) {
#endif
                ChangeStateTo(kLose);
                mIntensity = 0.5f;
#ifndef EA_BUILD_A124
            } else {
                ChangeStateTo(kNeutral);
                mIntensity = 0.8f;
            }
#endif
        } else if (mState == kNeutral) {
            mIntensity = 1.0f;
        }
#ifndef EA_BUILD_A124
    }
#endif
}

void MusicFlow::Update() {
    int y;
    int x;
    if ((g_pEAXSound->GetCurMusicVolume() == 0.0f) ||
        (g_pEAXSound->GetCurAudioSettings()->InteractiveMusicMode == 0)
#ifndef EA_BUILD_A124
#ifdef EA_PLATFORM_PLAYSTATION2
        // SLUS mira el miembro (lw 0x50), no gXMP_DOWNSTATE como GameCube y PC.
        || mX360UserTunes
#else
        || (gXMP_DOWNSTATE != 0)
#endif
#endif
    ) {
        if (mState != kTransition) {
            Reset();
        }
        return;
    }

#ifdef MILESTONE_BUILD
    // Pantalla de depuracion de la A124 (cadenas de su .rodata).
    x = -200;
    y = -125;
    if (MUSICFLOW_DISPLAY) {
        ScreenPrintf(x, y, 1.0f, "Music FLOW (t=%.1f) (i=%.1f) %d", mElapsed, mIntensity, mCurrentPart);
        y += 15;
    }
#endif

    if (mState != kTransition) {
        SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
#ifdef MILESTONE_BUILD
        if (MUSICFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "avel=%.0f", mAvgPlayerSpeed);
            y += 15;
            ScreenPrintf(x, y, 1.0f, "apd=%.1f", mAvgPursuitDist);
            y += 15;
            ScreenPrintf(x, y, 1.0f, "avgLOS=%.1f", mAvgNumCopsLOS);
            y += 15;
            ScreenPrintf(x, y, 1.0f, "avgForm=%.1f", mAvgNumCopsInForm);
            y += 15;
        }
#endif
        mElapsed = (WorldTimer - mTimer).GetSeconds();
#ifndef EA_BUILD_A124
        mTimeInPiece = (WorldTimer - mT_currPiece).GetSeconds();
#endif
        mAvgNumCopsInForm = mAvgNumCopsInForm * 0.97f + static_cast<float>(ai->GetCopsInFormation().size()) * 0.03f;
        mAvgNumCopsLOS = mAvgNumCopsLOS * 0.97f + static_cast<float>(ai->NumCopsWithLOS()) * 0.03f;
        mAvgPlayerSpeed = mAvgPlayerSpeed * 0.95f + ai->GetPlayerSpeed() * 0.05f;
        mAvgPursuitDist =
            mAvgPursuitDist * 0.97f + (ai->GetPursuitDistance() > 0.0f ? ai->GetPursuitDistance() : mAvgPursuitDist) * 0.03f;
    }

    switch (mState) {
    case kTransition:
#ifdef MILESTONE_BUILD
        if (MUSICFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "Transition");
        }
#endif
        break;
    case kNeutral:
#ifdef MILESTONE_BUILD
        if (MUSICFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "Neutral");
        }
#endif
        Neutral();
        break;
    case kLose:
#ifdef MILESTONE_BUILD
        if (MUSICFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "Lose");
        }
#endif
        Lose();
        break;
    case kWin:
#ifdef MILESTONE_BUILD
        if (MUSICFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "Win");
        }
#endif
        Win();
        break;
    case kElude:
#ifdef MILESTONE_BUILD
        if (MUSICFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "Elude");
        }
#endif
        Elude();
        break;
    // El objetivo emite el cuerpo de Terminal ANTES que el de Waiting: el orden
    // de los `case` en fuente es el orden de emision de los cuerpos (96,95 -> 97,00%).
    case kTerminal:
#ifdef MILESTONE_BUILD
        if (MUSICFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "Terminal");
        }
#endif
        Terminal();
        break;
    case kWaiting:
#ifdef MILESTONE_BUILD
        if (MUSICFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "Waiting");
        }
#endif
#ifndef EA_BUILD_A124
        Waiting();
#endif
        break;
    default:
#ifdef MILESTONE_BUILD
        if (MUSICFLOW_DISPLAY) {
            ScreenPrintf(x, y, 1.0f, "--unknown--");
        }
#endif
        break;
    }

#ifndef EA_BUILD_A124
    if (mRequestedSwap) {
        if ((mState != kWaiting) && (mState != kTerminal) && (mState != kTransition)) {
            mRequestedSwap = false;
        }
    }
#endif

    if (mState != kTransition) {
        int intensity = static_cast<int>(mIntensity * 127.0f);
        MControlPathfinder(false, 0, intensity, 0).Send(UCrc32("Control"));
    }
}

float MusicFlow::UpdateIntensity(float adj) {
    mIntensity = mIntensity * 0.99f + adj * 0.01f;
    mIntensity = bClamp(mIntensity, 0.0f, 1.0f);
    return mIntensity;
}

#ifndef EA_BUILD_A124
void MusicFlow::Waiting() {
    if (mStartDelay && (mElapsed > 0.5f)) {
        mStartDelay = false;
        MControlPathfinder(false, 0x11, 0, 0).Send(UCrc32("Event"));
        mT_currPiece = WorldTimer;
    }

    if (mRequestedSwap) {
        mT_currPiece = WorldTimer;
    }
}
#endif

#ifdef EA_BUILD_A124
// La A124 es otra version: sin in_cooldown al principio ni la rama de mRestrained que lo mira.
void MusicFlow::Neutral() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();

    mBusy = (mElapsed < 45.0f || mRestrained) ? mBusy++ : 0;

    UpdateIntensity(mAvgPursuitDist < 32.0f ? 1.0f : 0.0f);
    UpdateIntensity(ai->GetTimeLastCrashed() >= 2.5f ? mIntensity : 1.0f);
    UpdateIntensity(ai->GetTimeLastNailedCop() >= 2.5f ? mIntensity : 1.0f);

    if (mRestrained) {
        mIntensity = bClamp(mIntensity, 0.0f, 0.5f);
    }

    if (ai->GetPursuitState() == SoundAI::kInactive) {
        ChangeStateTo(kTerminal);
        return;
    }

    if (mRestrained) {
        if ((mElapsed > 45.0f) || (ai->GetFocus() == 2)) {
            mRestrained = false;
            mIntensity = 1.0f;
            mTimer = WorldTimer;
        } else {
            return;
        }
    }

    if (ai->GetPlayerSpeed() < mTopSpeed * 0.125f && ai->GetPursuit()->IsCollapseActive()) {
        mIntensity = 0.86f;
        ChangeStateTo(kLose);
        return;
    }

    if (ai->GetPlayerSpeed() < mTopSpeed * 0.125f && ai->GetPursuitState() == SoundAI::kSearching && ai->GetPerpLostTime() > 4.0f) {
        ChangeStateTo(kElude);
        return;
    }

    if (mBusy != 0) {
        return;
    }

    if (ai->GetPursuitState() != SoundAI::kInactive) {
        bool in_cooldown;
        if (ai->GetPursuit()) {
            in_cooldown = ai->GetPursuit()->GetPursuitStatus() == 2;
        } else {
            in_cooldown = false;
        }

        if (mAvgPlayerSpeed < mTopSpeed * 0.125f && (ai->GetPursuitState() == SoundAI::kSearching || in_cooldown)) {
            ChangeStateTo(kElude);
            return;
        }

        if (mAvgPlayerSpeed > mTopSpeed * 0.5f || in_cooldown) {
            ChangeStateTo(kWin);
            return;
        }

        if (mAvgPursuitDist < 32.0f) {
            ChangeStateTo(kLose);
            mIntensity = 0.35f;
        }
    }
}
#else
void MusicFlow::Neutral() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    bool in_cooldown;

    if (ai->GetPursuit()) {
        in_cooldown = ai->GetPursuit()->GetPursuitStatus() == 2;
    } else {
        in_cooldown = false;
    }

    int *busy = &mBusy;
    if (in_cooldown) {
        *busy = 0;
    } else {
        int busy_value;
        if (mElapsed < 30.0f || mRestrained) {
            busy_value = mBusy;
            mBusy = busy_value + 1;
        } else {
            busy_value = 0;
        }
        *busy = busy_value;
    }

    UpdateIntensity(mAvgPursuitDist < 32.0f ? 1.0f : 0.0f);
    UpdateIntensity(ai->GetTimeLastCrashed() >= 2.5f ? mIntensity : 1.0f);
    UpdateIntensity(ai->GetTimeLastNailedCop() >= 2.5f ? mIntensity : 1.0f);

    if (mRestrained) {
        mIntensity = bClamp(mIntensity, 0.0f, 0.5f);
    }

    if (ai->GetPursuitState() == SoundAI::kInactive) {
        ChangeStateTo(kTerminal);
        return;
    }

    if (mRestrained && !in_cooldown) {
        if ((mElapsed > 30.0f) || (ai->GetFocus() == 2)) {
            mRestrained = in_cooldown;
            mIntensity = 1.0f;
            mTimer = WorldTimer;
        } else {
            return;
        }
    }

    if (ai->GetPlayerSpeed() < mTopSpeed * 0.125f) {
        if (ai->GetPursuit()->IsCollapseActive()) {
            mIntensity = 0.86f;
            ChangeStateTo(kLose);
            return;
        }

        if (ai->GetPlayerSpeed() < mTopSpeed * 0.125f && ai->GetPursuitState() == SoundAI::kSearching &&
            ai->GetPerpLostTime() > 4.0f && ai->GetTimeLastCrashed() > 4.0f) {
            ChangeStateTo(kElude);
            return;
        }
    }

    if (mBusy != 0) {
        return;
    }

    if (ai->GetPursuitState() == SoundAI::kInactive) {
        return;
    }

    if (in_cooldown) {
        if (ai->GetPlayerSpeed() >= mTopSpeed * 0.35f) {
            ChangeStateTo(kWin);
        } else {
            ChangeStateTo(kElude);
        }
    } else {
        if (mAvgPlayerSpeed < mTopSpeed * 0.125f && ai->GetPursuitState() == SoundAI::kSearching) {
            ChangeStateTo(kElude);
            return;
        }

        if (mAvgPlayerSpeed > mTopSpeed * 0.5f) {
            ChangeStateTo(kWin);
            return;
        }

        if (mAvgPursuitDist < 32.0f) {
            ChangeStateTo(kLose);
            mIntensity = 0.35f;
        }
    }
}
#endif

void MusicFlow::Lose() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();
    bool in_cooldown;

#ifdef EA_BUILD_A124
    mBusy = (mElapsed < 60.0f) ? mBusy++ : 0;
#else
    mBusy = (mElapsed < 45.0f) ? mBusy++ : 0;
#endif

    if (ai->GetPursuitState() == SoundAI::kInactive) {
        ChangeStateTo(kTerminal);
        return;
    }

    if (ai->GetPlayerSpeed() < mTopSpeed * 0.125f && ai->GetPursuit()->TimeUntilBusted() > 0.5f && ai->IsHighIntensity()) {
        mIntensity = 1.0f;
    } else {
#ifndef EA_BUILD_A124
        if (mIntensity >= 0.93f) {
            mIntensity = 0.8f;
        }
#endif
        UpdateIntensity(ai->GetPlayerSpeed() < mTopSpeed * 0.375f ? 0.86f : 0.0f);
        UpdateIntensity(ai->GetTimeLastCrashed() >= 2.5f ? mIntensity : 0.86f);
        UpdateIntensity(ai->GetTimeLastNailedCop() >= 2.5f ? mIntensity : 0.86f);
    }

    if (ai->GetPursuit()) {
        in_cooldown = ai->GetPursuit()->GetPursuitStatus() == 2;
    } else {
        in_cooldown = false;
    }

#ifdef EA_BUILD_A124
    if (mAvgPlayerSpeed < mTopSpeed * 0.125f && (ai->GetPursuitState() == SoundAI::kSearching || in_cooldown) && ai->GetPerpLostTime() > 4.0f) {
        ChangeStateTo(kElude);
        mIntensity = 1.0f;
    } else if (in_cooldown) {
        ChangeStateTo(kWin);
        mIntensity = 0.4f;
    } else {
        if (mBusy != 0) {
            return;
        }

        if ((mAvgPlayerSpeed > mTopSpeed * 0.5f) || (ai->GetPerpLostTime() > 5.0f) || (mElapsed > 90.0f)) {
            ChangeStateTo(kNeutral);
            mIntensity = 1.0f;
        }
    }
#else
    if ((ai->GetPursuitState() == SoundAI::kSearching || in_cooldown) && ai->GetPerpLostTime() > 4.0f) {
        if (ai->GetPlayerSpeed() < mTopSpeed * 0.35f) {
            ChangeStateTo(kElude);
            mIntensity = 1.0f;
        } else {
            ChangeStateTo(kWin);
            mIntensity = 1.0f;
        }
    } else {
        if (mBusy != 0) {
            return;
        }

        if ((mAvgPlayerSpeed > mTopSpeed * 0.5f) || (ai->GetPerpLostTime() > 5.0f) || (mElapsed > 90.0f)) {
            ChangeStateTo(kNeutral);
            mIntensity = 0.65f;
        }
    }
#endif
}

#ifndef EA_BUILD_A124
void MusicFlow::RequestSwap() {
    mRequestedSwap = true;
}
#endif

void MusicFlow::Win() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();

    mBusy = (mElapsed < 45.0f) ? mBusy++ : 0;

#ifdef EA_BUILD_A124
    UpdateIntensity(mAvgPlayerSpeed > (mTopSpeed * 0.625f) ? 1.0f : 0.0f);
#else
    UpdateIntensity(mAvgPlayerSpeed > (mTopSpeed * 0.5f) ? 1.0f : 0.0f);
#endif

#ifdef EA_BUILD_A124
    if ((ai->GetTimeLastCrashed() < 5.0f) || (ai->GetTimeLastNailedCop() < 5.0f) || (ai->GetPlayerSpeed() < (mTopSpeed * 0.375f))) {
#else
    if ((ai->GetTimeLastCrashed() < 2.0f) || (ai->GetTimeLastNailedCop() < 2.0f)) {
#endif
        mIntensity = 0.25f;
    }

    if ((ai->GetTimeLastCrashed() > 10.0f) && (ai->GetTimeLastNailedCop() > 10.0f) && (ai->GetPlayerSpeed() > (mTopSpeed * 0.8f))) {
        float t_lastboosted = (WorldTimer - mBoostTimer).GetSeconds();
        if (t_lastboosted > 10.0f) {
            mIntensity *= 1.15f;
            mBoostTimer = WorldTimer;
        }
    }

    if (ai->GetPursuitState() == SoundAI::kInactive) {
        ChangeStateTo(kTerminal);
        return;
    }

    if ((ai->GetPlayerSpeed() < (mTopSpeed * 0.125f)) && ai->GetPursuit()->IsCollapseActive()) {
        ChangeStateTo(kLose);
        return;
    }

    bool in_cooldown;
    if (ai->GetPursuit()) {
        in_cooldown = ai->GetPursuit()->GetPursuitStatus() == PS_COOL_DOWN;
    } else {
        in_cooldown = false;
    }

    if (mAvgPlayerSpeed < (mTopSpeed * 0.125f)) {
        if ((ai->GetPursuitState() == SoundAI::kSearching) || in_cooldown) {
            ChangeStateTo(kElude);
            return;
        }
    }

    if (mBusy != 0) {
        return;
    }

    if (mElapsed > 90.0f) {
        ChangeStateTo(kNeutral);
    }
}

void MusicFlow::Elude() {
    SoundAI *ai = UTL::Collections::Singleton<SoundAI>::Get();

    mBusy = (mElapsed < 5.0f) ? mBusy++ : 0;

    if (ai->GetPursuitState() == SoundAI::kInactive) {
        ChangeStateTo(kTerminal);
        return;
    }

#ifdef EA_BUILD_A124
    // La A124 aun mide la huida por el nivel de evasion y la velocidad del jugador (lineas 556-576 de su MusicFlow.cpp).
    UpdateIntensity(ai->GetPursuit()->GetEvadeLevel() < 0.5f ? 1.0f : 0.0f);
    UpdateIntensity(ai->GetPlayerSpeed() > 50.0f ? 1.0f : 0.0f);

    if (ai->GetPursuitState() == SoundAI::kSearching && ai->GetPlayerSpeed() < mTopSpeed * 0.125f) {
        mIntensity = 0.25f;
    }

    if (ai->GetPursuitState() == SoundAI::kActive) {
        ChangeStateTo(kLose);
    } else if (mAvgPlayerSpeed > mTopSpeed * 0.5f) {
        ChangeStateTo(kWin);
    }
#else
    UpdateIntensity(mAvgPlayerSpeed > 10.0f ? 1.0f : 0.0f);

    if (ai->GetPursuitState() == SoundAI::kActive) {
        if (ai->GetPursuitDistance() <= 50.0f) {
            ChangeStateTo(kLose);
        } else {
            ChangeStateTo(kNeutral);
            mIntensity = 0.8f;
        }
    } else if (mAvgPlayerSpeed > mTopSpeed * 0.45f) {
        ChangeStateTo(kWin);
    }
#endif
}

void MusicFlow::Terminal() {
    switch (mCurrentPart) {
    case 5:
    case 6:
    case 7:
        break;
    default:
        Reset();
        break;
    }
}

bool MusicFlow::IsTransitionable() {
    return false;
}

void MusicFlow::ChangeStateTo(int new_state) {
    if (new_state != mState) {
        SoundAI *ai = SoundAI::Get();
        SpeechFlow::ChangeStateTo(kWaiting);
        int nevt = -1;
        Timer timer = WorldTimer;
        mBusy = 1;
        mTimer = timer;
        switch (new_state) {
            case kNeutral:
                if (mCurrentPart != 4) {
                    nevt = 0xa;
                }
                break;
            case kLose:
                if (mCurrentPart != 1) {
                    nevt = 0xb;
                }
                break;
            case kWin:
                if (mCurrentPart != 3) {
                    nevt = 0xc;
                }
                break;
            case kElude:
                mAvgPlayerSpeed = 0.0f;
                if (mCurrentPart != 2) {
                    nevt = 0xd;
                }
                break;
            case kTerminal:
                if ((static_cast<unsigned int>(mCurrentPart - 5) > 2) && (ai->GetFocus() != kTerminal) && (ai->GetFocus() != kTransition)) {
                    nevt = 0xe;
                }
                break;
        }
        if (nevt != -1) {
            MControlPathfinder(false, static_cast<unsigned int>(nevt), 0, 0).Send(UCrc32("Event"));
        }
    }
}

void MusicFlow::Reset() {
    mTimer = Timer(0);
    mElapsed = 0.0f;
    mIntensity = 0.0f;
    mAvgNumCopsInForm = 0.0f;
    mAvgNumCopsLOS = 0.0f;
    mAvgPlayerSpeed = 0.0f;
    mAvgPursuitDist = 0.0f;
#ifndef EA_BUILD_A124
    mRequestedSwap = false;
#endif
    mRestrained = true;
    mState = kTransition;
}

} // namespace Speech
