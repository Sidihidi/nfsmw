#ifndef EVENTS_EHIDEAUTOSAVEENGAGE_H
#define EVENTS_EHIDEAUTOSAVEENGAGE_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Main/Event.h"

// total size: 0x8
class EHideAutoSaveEngage : public Event {
  public:
    // total size: 0x4
    struct StaticData : public Event::StaticData {};

    // enum { kEventID = 0 };

    EHideAutoSaveEngage();

    ~EHideAutoSaveEngage() override;

    const char *GetEventName() const override;
};

void EHideAutoSaveEngage_MakeEvent_Callback(const void *staticData);
int EHideAutoSaveEngage_MakeEvent_LuaBinding(struct lua_State *L);
void EHideAutoSaveEngage_ResolveEvent_Callback(void *event, const struct UGroup *group);

#endif
