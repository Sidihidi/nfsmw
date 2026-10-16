#ifndef REALCORE_SOURCE_INPUT_PS2_PAD_H
#define REALCORE_SOURCE_INPUT_PS2_PAD_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#pragma interface

#include "ps2_device.h"
#include "ps2_effect.h"

// El estado de un mando de libpad (stabs de la A124: CONTROLLERtag mide 0x20).
typedef enum CONTROLLER_TYPEtag {
    CONTROLLER_TYPENOTCONNECTED = 0,
    CONTROLLER_TYPEUNSUPPORTED = 1,
    CONTROLLER_TYPESTANDARD = 2,
    CONTROLLER_TYPEDUALSHOCK = 3,
    CONTROLLER_TYPEDUALSHOCKANALOG = 4,
    CONTROLLER_TYPEDUALSHOCK2 = 5,
} CONTROLLER_TYPE;

typedef struct CONTROLLERtag {
    CONTROLLER_TYPE type;                   // offset 0x0
    unsigned char id;                       // offset 0x4
    unsigned char extid;                    // offset 0x5
    unsigned char term_id;                  // offset 0x6
    unsigned char phase;                    // offset 0x7
    char asyncVBlankDelay;                  // offset 0x8
    unsigned char Motor[2];                 // offset 0x9
    unsigned short DigitalButtonFlags;      // offset 0xC
    unsigned short eventDigitalButtonFlags; // offset 0xE
    unsigned char StickInfo[16];            // offset 0x10
} CONTROLLER;

namespace RealInput {

// Un mando de PS2 en su puerto y ranura (stabs de la A124: Ps2Pad mide 0x2C0).
struct Ps2Pad : Ps2Device {
    static inline void *operator new(unsigned int size) {
        return AllocateMemSize(nullptr, static_cast<int>(size), 64, 0, 0);
    }

    static inline void operator delete(void *ptr, unsigned int size) {
        FreeMemSize(ptr, static_cast<int>(size));
    }

    Ps2Pad(Ps2Interface *pInterface, int port, int slot);
    virtual ~Ps2Pad();
    virtual RiResult Update();
    virtual Effect *CreateEffect(Effect::Info *pInfo);

    virtual Effect *GetEffect() {
        return this->mpEffect;
    }

  private:
    void UpdateControllerBuf(int port, int slot, CONTROLLER *pCtrl);
    void UpdateCapabilities();
    void InitializePad();

    CONTROLLER mCntrlbuff;                                            // offset 0x14C
    bool mInitialized;                                                // offset 0x16C
    unsigned long long mPad_dma_buf[16] __attribute__((aligned(64))); // offset 0x180
    Ps2Effect *mpEffect;                                              // offset 0x280
};

} // namespace RealInput

#endif
