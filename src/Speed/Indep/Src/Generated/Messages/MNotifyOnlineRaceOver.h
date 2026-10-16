#ifndef GENERATED_MESSAGES_MNOTIFYONLINERACEOVER_H
#define GENERATED_MESSAGES_MNOTIFYONLINERACEOVER_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Misc/Hermes.h"

// total size: 0x14
class MNotifyOnlineRaceOver : public Hermes::Message {
  public:
    static std::size_t _GetSize() {
        return sizeof(MNotifyOnlineRaceOver);
    }

    static UCrc32 _GetKind() {
        static UCrc32 k("MNotifyOnlineRaceOver");

        return k;
    }

    static void BuildMessageTable(lua_State *luaState, const Hermes::Message *messageBase);

    static void HandleMessage_LuaBinding(const MNotifyOnlineRaceOver &message);

    MNotifyOnlineRaceOver(
#ifndef EA_BUILD_A124
        bool _ShowPostRace
#endif
        )
        : Hermes::Message(_GetKind(), _GetSize(), 0)
#ifndef EA_BUILD_A124
          ,
          fShowPostRace(_ShowPostRace)
#endif
    {}

#ifndef EA_BUILD_A124
    bool GetShowPostRace() const {
        return fShowPostRace;
    }

    void SetShowPostRace(bool _ShowPostRace) {
        fShowPostRace = _ShowPostRace;
    }
#endif

#ifndef EA_BUILD_A124
  private:
    bool fShowPostRace; // offset 0x10, size 0x1
#endif
};


#include "Speed/Indep/Src/Lua/LuaBindery.h"
#include "Speed/Indep/Src/Lua/LuaPostOffice.h"

inline void MNotifyOnlineRaceOver::HandleMessage_LuaBinding(const MNotifyOnlineRaceOver &message) {
    LuaMessageDeliveryInfo deliveryInfo(_GetKind(), &message, BuildMessageTable);

    LuaPostOffice::Get().RouteMessage(&deliveryInfo);
}

inline void MNotifyOnlineRaceOver::BuildMessageTable(lua_State *luaState, const Hermes::Message *messageBase) {
    const MNotifyOnlineRaceOver *message = static_cast<const MNotifyOnlineRaceOver *>(messageBase);

    lua_newtable(luaState);

#ifndef EA_BUILD_A124
    lua_pushstring(luaState, "ShowPostRace");
    lua_pushboolean(luaState, message->fShowPostRace);
    lua_settable(luaState, -3);
#endif
}

#endif
