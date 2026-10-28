#ifndef FRONTEND_NFSDEBUGMENU_H
#define FRONTEND_NFSDEBUGMENU_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Libs/Support/Utility/FastMem.h"
#include "Speed/Indep/Src/Frontend/DebugMenu.h"

class ActionQueue;

// total size: 0x8
struct ActionQueueNode {
    ActionQueue *actionQ;  // offset 0x0, size 0x4
    ActionQueueNode *next; // offset 0x4, size 0x4
};

// total size: 0x1C
class DebugMenuInputHandler {
  public:
    // void *operator new(size_t size, void *ptr) {}

    // void operator delete(void *mem, void *ptr) {}

    void *operator new(size_t size) {
        return gFastMem.Alloc(size, nullptr);
    }

    // void operator delete(void *mem, size_t size) {}

    // void *operator new(size_t size, const char *name) {}

    // void operator delete(void *mem, const char *name) {}

    // void operator delete(void *mem, size_t size, const char *name) {}

    DebugMenuInputHandler();
    ~DebugMenuInputHandler();
    void PollInput();
    void Exit();
#ifdef EA_BUILD_A124 // en linea en la cabecera de la A124 (la lee InputPlayer::FetchInput)
    bool IsDebugKeyPressed() {
        return mActiveShift;
    }
#else
    bool IsDebugKeyPressed();
#endif

  private:
    void Activate();
    void Deactivate();

  public:
    bool mActiveSpeed;       // offset 0x0, size 0x4
    bool mPrevPrintfState;   // offset 0x4, size 0x4
    bool mPrevFEngDrawState; // offset 0x8, size 0x4
    bool mPauseWorld;        // offset 0xC, size 0x4

  private:
    bool mActiveShift;              // offset 0x10, size 0x4
    ActionQueueNode *mDisabledList; // offset 0x14, size 0x4
    ActionQueue *mActionQ;          // offset 0x18, size 0x4
};

// total size: 0x4
class NFSMenuRender : public IMenuRender {
  public:
    void renderOverlay(int x, int y, int width, int height, float alpha, unsigned int color) override;
    void print(int x, int y, unsigned int color, const char *str) override;
    void getStringSize(const char *str, int *x, int *y) override;

  private:
    static int printfXOffset;
    static int printfYOffset;
};

extern DebugMenuInputHandler *gDebugMenuIOHandler;

void DebugMenuInit();
bool DebugMenuShouldPauseWorld();

#endif
