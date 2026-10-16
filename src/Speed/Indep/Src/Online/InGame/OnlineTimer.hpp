#ifndef ONLINE_INGAME_ONLINE_TIMER_HPP
#define ONLINE_INGAME_ONLINE_TIMER_HPP

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Misc/Timer.hpp"

// The A124 stabs also list its inline static methods (SetTimestamp, GetTimestamp, IsDirty, ClearDirty, GetTimer,
// AdvanceTimer, GetRealTime), but not their parameters; nothing written so far calls them.
struct OnlineTimer {
  public:
    // onlinetimer.hpp:35 de la A124: TrackLoaded y ReadyStartLine copian WorldTimer aqui con el operator= de Timer.
    static void SetTimestamp(const Timer &timer) {
        ServerTimer = timer;
    }

  private:
    static bool bIsDirty;
    static Timer ServerTimer;
};

#endif
