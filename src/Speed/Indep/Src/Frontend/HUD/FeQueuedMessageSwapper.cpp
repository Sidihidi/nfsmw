// frontend/hud/FeQueuedMessageSwapper.cpp. Nadie la usa: GameCube y la A124 la tiran al enlazar (Range 0xFFFFFFFF en
// el DWARF, solo stabs en la A124) y el EGM de Xbox 360 la conserva (/OPT:NOREF) en zFe2, entre FeOnlineHudSupport.cpp
// y FeMinimapStreamer.cpp: 0x822C0D28 (ctor), 0x822C0D70 (dtor), 0x822C0E10 (Update), 0x822C0F18 (QueueMessage).
#include "Speed/Indep/Src/Frontend/HUD/FeQueuedMessageSwapper.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterface.hpp"
#include "Speed/Indep/Src/Frontend/FEngInterfaces/FEngInterfaceFEObjects.hpp"
#include "Speed/Indep/bWare/Inc/Strings.hpp"
#include "Speed/Indep/bWare/Inc/bWare.hpp"

QueuedMessageSwapper::QueuedMessageSwapper(FEString *display_string, float display_seconds, int max_queued) {
    // El orden de los stores del EGM (0x822C0D28).
    fDisplaySeconds = display_seconds;
    iCurrentlyQueued = 0;
    iMaxQueued = max_queued;
    pDisplayString = display_string;
    CurrentCountdown.ResetLow();
}

QueuedMessageSwapper::~QueuedMessageSwapper() {
    FEngSetInvisible(pDisplayString);
    CurrentCountdown.ResetLow();
    iCurrentlyQueued = 0;
    while (!QueuedStringList.IsEmpty()) {
        char *message = static_cast<char *>(QueuedStringList.GetHead()->GetObject());
        delete[] message;
        QueuedStringList.RemoveHead();
    }
}

void QueuedMessageSwapper::Update(float t) {
    bool display_next = (!CurrentCountdown.IsSet() || !CurrentCountdown.CountDown(t)) && iCurrentlyQueued > 0;

    if (display_next) {
        char *message = static_cast<char *>(QueuedStringList.GetHead()->GetObject());
        FEPrintf(pDisplayString, message);
        CurrentCountdown.SetTime(fDisplaySeconds);
        delete[] message;
        QueuedStringList.RemoveHead();
        iCurrentlyQueued--;
    }

    if (iCurrentlyQueued == 0 && !CurrentCountdown.IsSet()) {
        FEngSetInvisible(pDisplayString);
    } else {
        FEngSetVisible(pDisplayString);
    }
}

void QueuedMessageSwapper::QueueMessage(const char *message) {
    if (iCurrentlyQueued == iMaxQueued) {
        CurrentCountdown.ResetLow();
        Update(0.0f);
    }

    char *new_str = new ("QueuedMessageStr", 0) char[bStrLen(message) + 1];
    bStrCpy(new_str, message);
    QueuedStringList.AddTail(new_str);
    iCurrentlyQueued++;
}
