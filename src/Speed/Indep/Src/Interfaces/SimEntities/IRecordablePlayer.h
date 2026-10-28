#ifndef IRECORDABLEPLAYER_H
#define IRECORDABLEPLAYER_H

#include "Speed/Indep/Libs/Support/Utility/UCOM.h"
#include "Speed/Indep/Libs/Support/Utility/UListable.h"
#include "Speed/Indep/Libs/Support/Utility/UTypes.h"
#include "Speed/Indep/Src/Sim/SimTypes.h"

class IRecordablePlayer : public UTL::COM::IUnknown, public UTL::Collections::Listable<IRecordablePlayer, 8> {
  public:
    DECL_INTERFACE(IRecordablePlayer);

    virtual void StartRecording() = 0;
    virtual void StopRecording() = 0;
    virtual void StartPlayback() = 0;
    virtual void Pause() = 0;
    virtual void UnPause() = 0;
    virtual float GetTimeTotalLength() = 0;
    virtual float GetTimeElapsed() = 0;
    virtual void SetTime(float time) = 0;
    virtual void StopPlayback() = 0;
    virtual unsigned int GetPacketSize() = 0;
};

#endif
