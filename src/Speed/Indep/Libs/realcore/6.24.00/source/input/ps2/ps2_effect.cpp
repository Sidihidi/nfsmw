// realcore 6.24.00: vibracion del mando de PS2 (source/input/ps2/ps2_effect.cpp en el .mdebug de la A124).
#include "ps2_effect.h"

#include "ps2_device.h"

#include "../cmn/imem.h"
#include "../../../include/common/realcore/std.h"

#include <libpad.h>

namespace RealInput {

Effect::Info::Info()
    : mMagnitudeLeft(MAX_MAGNITUDE_LEFT)
    , mMagnitudeRight(0) {
}

Ps2Effect::Ps2Effect()
    : EffectImp()
    , mPlaying(false)
    , mMagLeft(0)
    , mMagRight(0)
    , mPort(0)
    , mSlot(0) {
}

Ps2Effect::~Ps2Effect() {
    if (this->mPlaying) {
        this->Stop();
    }
}

void Ps2Effect::Init(Device *pDevice, Info *pInfo) {
    EffectImp::Init(pDevice, pInfo);
    IMem::Clear(this->mAct_Val, sizeof(this->mAct_Val));
    this->mPlaying = false;
    static_cast<Ps2Device *>(pDevice)->GetPortSlotNum(this->mPort, this->mSlot);
    this->SetInfo(pInfo);
}

void Ps2Effect::Release() {
    EffectImp::Release();
}

void Ps2Effect::Start() {
    scePadSetActDirect(this->mPort, this->mSlot, this->mAct_Val);
    this->mPlaying = true;
}

void Ps2Effect::Stop() {
    unsigned char Act_Val[6];

    IMem::Clear(Act_Val, sizeof(Act_Val));
    scePadSetActDirect(this->mPort, this->mSlot, Act_Val);
    this->mPlaying = false;
}

Effect::Status Ps2Effect::GetStatus() {
    if (this->mPlaying) {
        this->mStatus = STATUS_PLAYING;
    } else {
        this->mStatus = STATUS_STOPPED;
    }
    return this->mStatus;
}

void Ps2Effect::SetInfo(Info *pInfo) {
    MEM_copy(&this->mInfo, pInfo, sizeof(Info));
    this->mAct_Val[1] = pInfo->mMagnitudeLeft;
    this->mAct_Val[0] = pInfo->mMagnitudeRight;
    if (this->mPlaying) {
        this->Start();
    }
}

} // namespace RealInput
