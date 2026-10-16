#ifndef EVENTS_EPLAYDDFLYINNIS_H
#define EVENTS_EPLAYDDFLYINNIS_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Main/Event.h"

// total size: 0x8
class EPlayDDFlyInNIS : public Event {
  public:
    // total size: 0x4
    struct StaticData : public Event::StaticData {};

    // enum { kEventID = 0 };

    EPlayDDFlyInNIS();

    ~EPlayDDFlyInNIS() override;

    const char *GetEventName() const override;
};

void EPlayDDFlyInNIS_MakeEvent_Callback(const void *staticData);
int EPlayDDFlyInNIS_MakeEvent_LuaBinding(struct lua_State *L);
void EPlayDDFlyInNIS_ResolveEvent_Callback(void *event, const struct UGroup *group);

#endif
