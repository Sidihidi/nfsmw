#ifndef EVENTS_ESHOWAUTOSAVEENGAGE_H
#define EVENTS_ESHOWAUTOSAVEENGAGE_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Main/Event.h"

// total size: 0x8
class EShowAutoSaveEngage : public Event {
  public:
    // total size: 0x4
    struct StaticData : public Event::StaticData {};

    // enum { kEventID = 0 };

    EShowAutoSaveEngage();

    ~EShowAutoSaveEngage() override;

    const char *GetEventName() const override;
};

void EShowAutoSaveEngage_MakeEvent_Callback(const void *staticData);
int EShowAutoSaveEngage_MakeEvent_LuaBinding(struct lua_State *L);
void EShowAutoSaveEngage_ResolveEvent_Callback(void *event, const struct UGroup *group);

#endif
