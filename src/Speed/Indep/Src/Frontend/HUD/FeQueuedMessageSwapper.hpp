#ifndef FEQUEUEDMESSAGESWAPPER_H
#define FEQUEUEDMESSAGESWAPPER_H

#include "Speed/Indep/Src/Misc/Timer.hpp"
#include "Speed/Indep/bWare/Inc/bList.hpp"

class FEString;

// frontend/hud/FeQueuedMessageSwapper.cpp (los stabs de la A124 y el DWARF de GameCube dan la clase entera; las dos
// tiran el codigo al enlazar, Range 0xFFFFFFFF). Una cola de mensajes que se turnan en un FEString cada
// fDisplaySeconds. El EGM de Xbox 360 conserva el codigo en zFe2 (0x822C0D28-0x822C0FF8), tras FEOnlineDisconnectMsg.
// total size: 0x1C
class QueuedMessageSwapper {
  public:
    QueuedMessageSwapper(FEString *display_string, float display_seconds, int max_queued);
    ~QueuedMessageSwapper();
    void Update(float t);
    void QueueMessage(const char *message);

  private:
    FEString *pDisplayString;      // offset 0x0, size 0x4
    float fDisplaySeconds;         // offset 0x4, size 0x4
    int iMaxQueued;                // offset 0x8, size 0x4
    int iCurrentlyQueued;          // offset 0xC, size 0x4
    Timer CurrentCountdown;        // offset 0x10, size 0x4
    bPList<char> QueuedStringList; // offset 0x14, size 0x8
};

#endif
