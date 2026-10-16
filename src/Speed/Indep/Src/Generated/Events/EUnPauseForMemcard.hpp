#ifndef EVENTS_EUNPAUSEFORMEMCARD_H
#define EVENTS_EUNPAUSEFORMEMCARD_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Main/Event.h"

// total size: 0x8
class EUnPauseForMemcard : public Event {
  public:
    // total size: 0x4
    struct StaticData : public Event::StaticData {};

    // enum { kEventID = 0 };

    EUnPauseForMemcard();

    ~EUnPauseForMemcard() override;

    const char *GetEventName() const override;
};

void EUnPauseForMemcard_MakeEvent_Callback(const void *staticData);

#endif
