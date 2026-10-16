// The local player of an online race. Only the PS2 prototype (SLES-53558-A124) has it: its .mdebug puts
// sim/entities/onlinelocalplayer.cpp in zSim, after LocalPlayer.cpp. Every task it exports the car of the player to
// its OnlineRacer, the replica that the other machines see.
#include "Speed/Indep/Src/Sim/Entities/LocalPlayer.hpp"
#include "Speed/Indep/Src/Interfaces/SimEntities/IOnlinePlayer.h"
#include "Speed/Indep/Src/Online/InGame/OnlineRacer.h"

// total size: 0x130
class OnlineLocalPlayer : public LocalPlayer, public IOnlinePlayer {
  public:
    void *operator new(std::size_t size) {
        return gFastMem.Alloc(size, nullptr);
    }

    void operator delete(void *mem, std::size_t size) {
        if (mem) {
            gFastMem.Free(mem, size, nullptr);
        }
    }

    static Sim::IEntity *Construct(Sim::Param params) {
        return new OnlineLocalPlayer(params);
    }

    // ITaskable
    bool OnTask(HSIMTASK htask, float dT) override;

    // IOnlinePlayer
    void SetOnlineRacer(OnlineRacer *racer) override {
        mOnlineRacer = racer;
        if (racer) {
            mOnlineRacer->ExportSimable(GetSimable());
        }
    }

    OnlineRacer *GetOnlineRacer() override {
        return mOnlineRacer;
    }

    void Reposition() override {
        if (mOnlineRacer) {
            mOnlineRacer->IncreaseRepositionCount();
        }
    }

    OnlineLocalPlayer(Sim::Param params);
    ~OnlineLocalPlayer() override;

  private:
    OnlineRacer *mOnlineRacer; // offset 0x120, size 0x4
    HSIMTASK mOnlineTask;      // offset 0x124, size 0x4
};

Sim::IEntity::Prototype _OnlineLocalPlayer(UCrc32("OnlineLocalPlayer"), OnlineLocalPlayer::Construct);

OnlineLocalPlayer::OnlineLocalPlayer(Sim::Param params)
    : LocalPlayer(params),  //
      IOnlinePlayer(this),  //
      mOnlineRacer(nullptr) {
    mOnlineTask = AddTask("WorldUpdate", 1.0f, 0.0f, Sim::TASK_FRAME_FIXED);
}

OnlineLocalPlayer::~OnlineLocalPlayer() {
    RemoveTask(mOnlineTask);
}

bool OnlineLocalPlayer::OnTask(HSIMTASK htask, float dT) {
    if (htask == mOnlineTask) {
        if (mOnlineRacer) {
            mOnlineRacer->ExportSimable(GetSimable());
        }
        return true;
    }
    return LocalPlayer::OnTask(htask, dT);
}
