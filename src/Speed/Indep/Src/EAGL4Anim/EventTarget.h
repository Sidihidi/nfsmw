#ifndef EAGL4ANIM_EVENTTARGET_H
#define EAGL4ANIM_EVENTTARGET_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "AnimationTarget.h"
#include "EventHandler.h"

namespace EAGL4Anim {

// Declaracion del DWARF de GameCube. Las funciones de EventTarget.cpp estan estripadas en las tres versiones y las
// en linea solo las usan ellas: se quedan sin cuerpo.
// total size: 0x18
class EventTarget : public AnimationTarget {
  public:
    // total size: 0x8
    struct EventMapping {
        char *mEventName; // offset 0x0, size 0x4
        int mId;          // offset 0x4, size 0x4
    };

    inline EventTarget();

    inline virtual ~EventTarget();

    inline int GetNumEvents() const;

    inline void InitHandlers(EventHandler **eventHandlers, EventHandler *defaultHandler);

    inline const char *GetEventName(int eventId) const;

    inline bool MissingHandlers(EventHandler **eventHandlers) const;

    inline void SetIdSizeIncrement(int inc);

    inline int GetIdSizeIncrement() const;

    void DeleteMe();

    bool ResolveEventId(const char *symbolName, int &eventId);

    int GetEventId(const char *eventName) const;

    int GetEventId(const char *eventName, int &s) const;

    bool AddHandler(EventHandler **eventHandlers, const char *eventName, EventHandler *handler);

    bool RemoveHandler(EventHandler **eventHandlers, const char *eventName, EventHandler *handler);

    bool RemoveHandler(EventHandler **eventHandlers, const char *eventName);

  private:
    EventMapping *mEventMapping; // offset 0x4, size 0x4
    const char **mEventNames;    // offset 0x8, size 0x4
    int mEventMappingSize;       // offset 0xC, size 0x4
    int mSizeIncrement;          // offset 0x10, size 0x4
};

}; // namespace EAGL4Anim

#endif
