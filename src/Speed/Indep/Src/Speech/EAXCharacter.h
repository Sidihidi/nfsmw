#ifndef SPEECH_EAXCHARACTER_H
#define SPEECH_EAXCHARACTER_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/EAXSound/AudioMemBase.hpp"
#include "Speed/Indep/Src/EAXSound/SND_GEN/COPSPEECH.hpp"
#include "Speed/Indep/Src/Interfaces/Simables/ISimable.h"

// gcdatos13: en el DWARF de GC (mw_dwarfdump.nothpp:125895) y los stabs de la A124
// (PS2_types.nothpp:9675) va delante de Battalion, con estos valores.
enum SpeakerID {
    Dispatch = 1,
    Heli = 2,
    Primary1 = 3,
    Primary2 = 4,
    Primary3 = 5,
    Secondary1 = 6,
    Secondary2 = 7,
    Secondary3 = 8,
    Cross = 9,
    NUM_SPEAKER_ID = 10,
};

// TODO where to put this?
// total size: 0x8
struct Battalion {
    Battalion(int nam, int num) : name(nam), number(num) {}

    int name;   // offset 0x0, size 0x4
    int number; // offset 0x4, size 0x4
};

// total size: 0x40
class EAXCharacter : public AudioMemBase {
  public:
    EAXCharacter(int sID, HSIMABLE wID, int bID, int cID);

    static void *operator new(unsigned int obj_size);
    static void operator delete(void *ptr);

    virtual ~EAXCharacter();

    // Virtual functions
    virtual void Ack();
    virtual void Deny();
    virtual void InterruptStatic();
    virtual void InterruptExpletive();
    virtual void InterruptViolent();
    virtual void InterruptComposedLow();
    virtual void InterruptComposedHigh();
    virtual void DriverHistory();
    virtual void HeatJump(Csis::Type_heat_level heat);

    virtual HSIMABLE GetHandle() { return mHandle; }

    virtual void SetHandle(HSIMABLE handle) { mHandle = handle; }

    virtual int GetSpeakerID() { return mSpeakerID; }

    virtual int GetCallsign() { return mCallsign.name; }

    virtual int GetUnitNumber() { return mCallsign.number; }

    virtual void SetCallsign(int callsign) { mCallsign.name = callsign; }

    virtual void SetUnitNumber(int unitnum) { mCallsign.number = unitnum; }

    virtual void SetSpeakerID(int spkrID) { mSpeakerID = spkrID; }

    virtual void SetPosition(const UMath::Vector3 &v) { mPos = v; }

    virtual const UMath::Vector3 GetPosition() { return mPos; }

    virtual void SetSpeed(const float speed) { mSpeed = speed; }

    virtual void Update();

    virtual float GetDistance() { return mDistance; }

    virtual float GetHealth() { return mHealth; }

    virtual bool IsActive() { return mActive; }

    virtual void SetActive(bool active) { mActive = active; }

    virtual float GetSpeed() { return mSpeed; }

    virtual bool IsDead() { return mDestroyed; }

    virtual bool HasLOS() { return mSuspectLOS; }

    virtual void SetLOS(bool yes) { mSuspectLOS = yes; }

    virtual Csis::Type_code GetRandomizedCode() { return Csis::Type_code_dont_use_10_code; }

    virtual void Reset();

  protected:
    int mTimeLastSpoken; // offset 0x4, size 0x4
    int mLastEvent;      // offset 0x8, size 0x4
    int mSpeakerID;      // offset 0xC, size 0x4
    HSIMABLE mHandle;    // offset 0x10, size 0x4
    Battalion mCallsign; // offset 0x14, size 0x8
    ALIGNVEC UMath::Vector3 mPos; // offset 0x1C, size 0xC
    float mSpeed;        // offset 0x28, size 0x4
    float mDistance;     // offset 0x2C, size 0x4
    float mHealth;       // offset 0x30, size 0x4
    bool mDestroyed;     // offset 0x34, size 0x1
    bool mActive;        // offset 0x38, size 0x1
    bool mSuspectLOS;    // offset 0x3C, size 0x1
};

#endif
