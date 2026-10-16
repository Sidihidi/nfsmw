// A player of an online race driven from another machine. Only the PS2 prototype (SLES-53558-A124) has it: its
// .mdebug puts sim/entities/onlineremoteplayer.cpp in zSim, after OnlineLocalPlayer.cpp. Every task it imports the
// replica in its OnlineRacer into the car; it has no hud, controller or game breaker of its own.
#include "Speed/Indep/Src/Interfaces/ITaskable.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IEntity.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IOnlinePlayer.h"
#include "Speed/Indep/Src/Interfaces/SimEntities/IPlayer.h"
#include "Speed/Indep/Src/Online/InGame/OnlineRacer.h"
#include "Speed/Indep/Src/Sim/SimEntity.h"
#include "Speed/Indep/Src/Sim/Simulation.h"

// total size: 0xF0
class OnlineRemotePlayer : public Sim::Entity, public IOnlinePlayer, public IPlayer {
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
        return new OnlineRemotePlayer(params);
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

    void Reposition() override {}

    // IEntity
    ISimable *GetSimable() const override {
        return Sim::Entity::GetSimable();
    }

    const UMath::Vector3 &GetPosition() const override {
        return Sim::Entity::GetPosition();
    }

    bool SetPosition(const UMath::Vector3 &position) override {
        return Sim::Entity::SetPosition(position);
    }

    // IPlayer
    void SetRenderPort(int renderport) override {}

    int GetRenderPort() const override {
        return -1;
    }

    PlayerSettings *GetSettings() const override {
        return nullptr;
    }

    void SetSettings(int fe_index) override {
        mSettingIndex = fe_index;
    }

    int GetSettingsIndex() const override {
        return mSettingIndex;
    }

    IHud *GetHud() const override {
        return nullptr;
    }

    void SetHud(ePlayerHudType ht) override {}

    void SetControllerPort(int port) override {}

    int GetControllerPort() const override {
        return -1;
    }

    IFeedback *GetFFB() override {
        return nullptr;
    }

    ISteeringWheel *GetSteeringDevice() override {
        return nullptr;
    }

    bool InGameBreaker() const override {
        return false;
    }

    bool CanRechargeNOS() const override {
        return true;
    }

    void ResetGameBreaker(bool full) override {}

    void ChargeGameBreaker(float amount) override {}

    bool ToggleGameBreaker() override {
        return false;
    }

    OnlineRemotePlayer(Sim::Param params);
    ~OnlineRemotePlayer() override;

  private:
    int mSettingIndex;         // offset 0xE0, size 0x4
    OnlineRacer *mOnlineRacer; // offset 0xE4, size 0x4
    HSIMTASK mOnlineTask;      // offset 0xE8, size 0x4
};

Sim::IEntity::Prototype _OnlineRemotePlayer(UCrc32("OnlineRemotePlayer"), OnlineRemotePlayer::Construct);

OnlineRemotePlayer::OnlineRemotePlayer(Sim::Param params)
    : IOnlinePlayer(this), //
      IPlayer(this),       //
      mOnlineRacer(nullptr) {
    mOnlineTask = AddTask("WorldUpdate", 1.0f, 0.0f, Sim::TASK_FRAME_FIXED);
    IEntity::AddToList(ENTITY_PLAYERS);
    IPlayer::AddToList(PLAYER_REMOTE);
    IPlayer::AddToList(PLAYER_ALL);
}

OnlineRemotePlayer::~OnlineRemotePlayer() {
    RemoveTask(mOnlineTask);
}

bool OnlineRemotePlayer::OnTask(HSIMTASK htask, float dT) {
    if (htask == mOnlineTask) {
        if (mOnlineRacer) {
            float simtime = Sim::GetTime();
            mOnlineRacer->ImportSimable(GetSimable(), dT, simtime);
        }
        return true;
    }
    return false;
}
