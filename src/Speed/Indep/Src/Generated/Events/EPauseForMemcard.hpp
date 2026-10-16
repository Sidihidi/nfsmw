#ifndef EVENTS_EPAUSEFORMEMCARD_H
#define EVENTS_EPAUSEFORMEMCARD_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Main/Event.h"

// total size: 0x8
class EPauseForMemcard : public Event {
  public:
    // total size: 0x4
    struct StaticData : public Event::StaticData {};

    // enum { kEventID = 0 };

    EPauseForMemcard();

    ~EPauseForMemcard() override;

    const char *GetEventName() const override;
};

void EPauseForMemcard_MakeEvent_Callback(const void *staticData);

#endif
