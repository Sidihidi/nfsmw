// realcore 6.24.00: el mando de PS2 sobre libpad (source/input/ps2/ps2_pad.cpp en el .mdebug de la A124). Locales,
// globales y el orden de las sentencias, de sus stabs y su mapa de lineas.
#pragma implementation "ps2_pad.h"
#include "ps2_pad.h"

#include "ps2_interface.h"
#include "../cmn/eventqueue.h"
#include "../cmn/imem.h"
#include "../../../include/common/realcore/std.h"

#include <libpad.h>

namespace RealInput {

extern Interface *gInterface;

static bool gAlwaysUseAnalog = true;
static unsigned char gAct_Align[6] = {0, 1, 0xFF, 0xFF, 0xFF, 0xFF};

Ps2Pad::Ps2Pad(Ps2Interface *pInterface, int port, int slot)
    : Ps2Device(TYPE_PAD, pInterface) {
    this->mInitialized = false;
    this->mInfo.mControllerID = 0;
    this->mCapabilities.mNumDigitalButtons = 0;
    this->mCapabilities.mNumAnalogButtons = 0;
    this->mCapabilities.mAttached = 0;
    this->mCapabilities.mForceFeedback = 0;
    this->mpEffect = nullptr;
    IMem::Clear(&this->mCntrlbuff, sizeof(this->mCntrlbuff));
    this->SetPortSlotNum(port, slot);
    this->mInfo.mPortNum = port;
    this->mInfo.mSlotNum = slot;
    this->mpInterface->RegisterDevice(this);
}

Ps2Pad::~Ps2Pad() {
    this->mpInterface->UnRegisterDevice(this);
    if (this->mInitialized) {
        scePadPortClose(this->mInfo.mPortNum, this->mInfo.mSlotNum);
    }
    if (this->mpEffect != nullptr) {
        delete this->mpEffect;
    }
}

void Ps2Pad::InitializePad() {
    int port = this->mInfo.mPortNum;
    int slot = this->mInfo.mSlotNum;

    if (scePadPortOpen(port, slot, reinterpret_cast<u_long128 *>(this->mPad_dma_buf)) != 0) {
        this->UpdateControllerBuf(port, slot, &this->mCntrlbuff);
        this->UpdateCapabilities();
        IMem::Clear(&this->mData, sizeof(this->mData));
    }
    this->mInitialized = true;
}

void Ps2Pad::UpdateCapabilities() {
    this->mCapabilities.mNumDigitalButtons = 0;
    this->mCapabilities.mNumAnalogButtons = 0;
    this->mCapabilities.mAttached = 0;
    this->mCapabilities.mForceFeedback = 0;
    this->mInfo.mControllerID = 0;
    switch (this->mCntrlbuff.type) {
    case CONTROLLER_TYPENOTCONNECTED:
        break;
    case CONTROLLER_TYPEUNSUPPORTED:
        this->mInfo.mControllerID = 1;
        break;
    case CONTROLLER_TYPESTANDARD:
        this->mInfo.mControllerID = 2;
        this->mCapabilities.mNumDigitalButtons = 14;
        this->mCapabilities.mAttached = 1;
        break;
    case CONTROLLER_TYPEDUALSHOCK:
        this->mInfo.mControllerID = 3;
        this->mCapabilities.mNumDigitalButtons = 16;
        this->mCapabilities.mForceFeedback = 1;
        this->mCapabilities.mAttached = 1;
        break;
    case CONTROLLER_TYPEDUALSHOCKANALOG:
        this->mInfo.mControllerID = 4;
        this->mCapabilities.mNumDigitalButtons = 16;
        this->mCapabilities.mNumAnalogButtons = 0;
        this->mCapabilities.mForceFeedback = 1;
        this->mCapabilities.mAttached = 1;
        break;
    case CONTROLLER_TYPEDUALSHOCK2:
        this->mInfo.mControllerID = 5;
        this->mCapabilities.mNumDigitalButtons = 16;
        this->mCapabilities.mNumAnalogButtons = 12;
        this->mCapabilities.mForceFeedback = 1;
        this->mCapabilities.mAttached = 1;
        break;
    }
}

void Ps2Pad::UpdateControllerBuf(int port, int slot, CONTROLLER *pCtrl) {
    int state;
    int nbytes;
    unsigned char rdata[32];

    state = scePadGetState(port, slot);
    if (state == scePadStateDiscon) {
        pCtrl->type = CONTROLLER_TYPENOTCONNECTED;
        pCtrl->phase = 0;
    }
    switch (pCtrl->phase) {
    case 0:
        if (state != scePadStateStable && state != scePadStateFindCTP1) {
            break;
        }
        pCtrl->id = scePadInfoMode(port, slot, InfoModeCurID, 0);
        if (pCtrl->id == 0) {
            break;
        }
        pCtrl->extid = scePadInfoMode(port, slot, InfoModeCurExID, 0);
        switch (pCtrl->extid) {
        case 0:
            if (pCtrl->id == 4) {
                pCtrl->type = CONTROLLER_TYPESTANDARD;
            } else {
                pCtrl->type = CONTROLLER_TYPEUNSUPPORTED;
            }
            pCtrl->phase = 99;
            break;
        case 4:
            pCtrl->phase = 40;
            break;
        case 7:
            pCtrl->phase = 70;
            break;
        default:
            pCtrl->type = CONTROLLER_TYPEUNSUPPORTED;
            pCtrl->phase = 99;
            break;
        }
        break;
    case 10:
        if (scePadGetState(port, slot) != scePadStateExecCmd) {
            pCtrl->phase = 0;
        }
        break;
    case 40:
        if (state != scePadStateStable) {
            break;
        }
    case 74:
        if (scePadInfoAct(port, slot, -1, 0) == 0) {
            pCtrl->type = CONTROLLER_TYPESTANDARD;
            pCtrl->phase = 99;
        }
        if (scePadSetActAlign(port, slot, gAct_Align) == 0) {
            break;
        }
        pCtrl->phase++;
        break;
    case 41:
        switch (scePadGetReqState(port, slot)) {
        case scePadReqStateComplete:
            if (gAlwaysUseAnalog) {
                if (scePadSetMainMode(port, slot, 1, 3) == 1) {
                    pCtrl->phase = 10;
                }
            } else {
                pCtrl->type = CONTROLLER_TYPEDUALSHOCK;
                pCtrl->phase = 99;
            }
            break;
        case scePadReqStateFaild:
            pCtrl->phase--;
            break;
        case scePadReqStateBusy:
            break;
        }
        break;
    case 70:
        if (scePadInfoPressMode(port, slot) != 1) {
            pCtrl->type = CONTROLLER_TYPEDUALSHOCKANALOG;
            pCtrl->phase = 99;
            break;
        }
        pCtrl->phase++;
    case 71:
        if (scePadEnterPressMode(port, slot) != 1) {
            break;
        }
        pCtrl->phase++;
        break;
    case 72:
        switch (scePadGetReqState(port, slot)) {
        case scePadReqStateComplete:
            pCtrl->type = CONTROLLER_TYPEDUALSHOCK2;
            pCtrl->phase = 99;
            break;
        case scePadReqStateFaild:
            pCtrl->phase--;
            break;
        }
        break;
    case 73:
        if (state != scePadStateStable) {
            break;
        }
        pCtrl->phase++;
        break;
    case 75:
        switch (scePadGetReqState(port, slot)) {
        case scePadReqStateComplete:
            pCtrl->phase++;
            break;
        case scePadReqStateFaild:
            pCtrl->phase--;
            break;
        }
        break;
    default:
        if (state != scePadStateStable && state != scePadStateFindCTP1) {
            break;
        }
        if (scePadRead(port, slot, rdata) != 0) {
            if (rdata[0] == 0) {
                pCtrl->phase = 99;
                pCtrl->DigitalButtonFlags = ~((rdata[2] << 8) | rdata[3]);
                nbytes = (rdata[1] & 0xF) * 2;
                if (nbytes > 16) {
                    nbytes = 16;
                }
                MEM_copy(pCtrl->StickInfo, &rdata[4], nbytes);
            }
        }
        if (rdata[1] != 0) {
            if (pCtrl->term_id != 0 && rdata[1] != pCtrl->term_id) {
                pCtrl->phase = 0;
            }
            pCtrl->term_id = rdata[1];
        }
        break;
    }
}

RiResult Ps2Pad::Update() {
    EventQueue *pEventQ;

    if (!this->mInitialized) {
        this->InitializePad();
    }
    pEventQ = this->mpInterface->GetEventQueue();
    if (pEventQ != nullptr) {
        Event event;
        unsigned int NewCID = 0;
        unsigned char DButtons[64];
        unsigned char Analog[64];
        unsigned short DButtonF;
        int i;

        IMem::Clear(&event, sizeof(event));
        event.mpDevice = this;
        event.mTimestamp = ITimer::GetTime();
        this->UpdateControllerBuf(this->mInfo.mPortNum, this->mInfo.mSlotNum, &this->mCntrlbuff);
        switch (this->mCntrlbuff.type) {
        case CONTROLLER_TYPENOTCONNECTED:
            NewCID = 0;
            break;
        case CONTROLLER_TYPEUNSUPPORTED:
            NewCID = 1;
            break;
        case CONTROLLER_TYPESTANDARD:
            NewCID = 2;
            break;
        case CONTROLLER_TYPEDUALSHOCK:
            NewCID = 3;
            break;
        case CONTROLLER_TYPEDUALSHOCKANALOG:
            NewCID = 4;
            break;
        case CONTROLLER_TYPEDUALSHOCK2:
            NewCID = 5;
            break;
        }
        if (NewCID != this->mInfo.mControllerID) {
            this->mInfo.mControllerID = NewCID;
            this->UpdateCapabilities();
            if (NewCID != 0) {
                event.mType = TYPE_CONNECTED;
            } else {
                event.mType = TYPE_DISCONNECTED;
                IMem::Clear(&this->mData, sizeof(this->mData));
            }
            pEventQ->AddEvent(&event);
        }
        if (NewCID > 1) {
            DButtonF = this->mCntrlbuff.DigitalButtonFlags;
            for (i = 0; i < 16; i++) {
                if (DButtonF & 1) {
                    DButtons[i] = 255;
                } else {
                    DButtons[i] = 0;
                }
                DButtonF >>= 1;
                if (DButtons[i] != this->mData.mPad.mButtons[i + 40]) {
                    this->mData.mPad.mButtons[i + 40] = DButtons[i];
                    if (i >= 8 && i < 12) {
                        this->mData.mPad.mButtons[i - 8] = DButtons[i];
                        event.mObject = OBJECT_BUTTON;
                        event.mType = DButtons[i] ? TYPE_PRESSED : TYPE_RELEASED;
                        event.mObjectIndex = i - 8;
                        pEventQ->AddEvent(&event);
                    }
                }
            }
            if (this->mInfo.mControllerID != 2) {
                for (i = 0; i < 4; i++) {
                    Analog[i] = this->mCntrlbuff.StickInfo[i];
                    if (Analog[i] != this->mData.mPad.mButtons[i + 32]) {
                        event.mObject = OBJECT_DIRECTIONLEFT;
                        event.mType = TYPE_CHANGE;
                        switch (i) {
                        case 2:
                            event.mObjectIndex = 0;
                            this->mData.mPad.mDirection[0].mX = Analog[2];
                            break;
                        case 3:
                            this->mData.mPad.mDirection[0].mY = Analog[3];
                            event.mObjectIndex = 0;
                            break;
                        case 0:
                            event.mObject = OBJECT_DIRECTIONRIGHT;
                            this->mData.mPad.mDirection[1].mX = Analog[0];
                            event.mObjectIndex = 1;
                            break;
                        case 1:
                            event.mObject = OBJECT_DIRECTIONRIGHT;
                            this->mData.mPad.mDirection[1].mY = Analog[1];
                            event.mObjectIndex = 1;
                            break;
                        }
                        pEventQ->AddEvent(&event);
                        this->mData.mPad.mButtons[i + 32] = Analog[i];
                    }
                }
            }
            if (this->mInfo.mControllerID == 5) {
                for (i = 4; i < 16; i++) {
                    Analog[i] = this->mCntrlbuff.StickInfo[i];
                }
            } else {
                Analog[4] = DButtons[13];
                Analog[5] = DButtons[15];
                Analog[6] = DButtons[12];
                Analog[7] = DButtons[14];
                Analog[8] = DButtons[4];
                Analog[9] = DButtons[5];
                Analog[10] = DButtons[6];
                Analog[11] = DButtons[7];
                Analog[12] = DButtons[2];
                Analog[13] = DButtons[3];
                Analog[14] = DButtons[0];
                Analog[15] = DButtons[1];
            }
            for (i = 4; i < 16; i++) {
                if (Analog[i] != this->mData.mPad.mButtons[i]) {
                    event.mObject = OBJECT_BUTTON;
                    event.mType = TYPE_CHANGE;
                    event.mObjectIndex = i;
                    if (this->mData.mPad.mButtons[i] != 0 && Analog[i] == 0) {
                        event.mType = TYPE_RELEASED;
                    } else if (this->mData.mPad.mButtons[i] == 0 && Analog[i] != 0) {
                        event.mType = TYPE_PRESSED;
                    }
                    pEventQ->AddEvent(&event);
                    this->mData.mPad.mButtons[i] = Analog[i];
                }
            }
        }
    }
    return RI_OK;
}

Effect *Ps2Pad::CreateEffect(Effect::Info *pInfo) {
    if (!this->mInitialized) {
        return nullptr;
    }
    if (!this->mCapabilities.mAttached) {
        return nullptr;
    }
    if (!this->GetCapabilities()->mForceFeedback) {
        return nullptr;
    }
    if (this->mpEffect != nullptr) {
        this->mpEffect->Release();
    }
    this->mpEffect = static_cast<Ps2Interface *>(gInterface)->GetUnusedEffectSlot();
    this->mpEffect->Init(this, pInfo);
    return this->mpEffect;
}

} // namespace RealInput
