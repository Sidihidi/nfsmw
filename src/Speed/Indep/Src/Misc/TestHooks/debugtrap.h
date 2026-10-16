#ifndef MISC_TEST_HOOKS_DEBUG_TRAP_H
#define MISC_TEST_HOOKS_DEBUG_TRAP_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

// El procesador de comandos del monitor de depuracion (misc/testhooks/debugtrap.cpp): solo lo tiene la A124.
// InitializeEverything lo crea, main lo arranca (setState) y MainLoop le anota el contador de ciclos (mTick) en cada
// vuelta. Campos y metodos de los stabs de la A124.
// total size: 0xC
class cCommandProcessor {
  public:
    cCommandProcessor();
    void setTickCount();
    unsigned int getTickCount();
    void setState(int startStop);
    int isMonEnabled();

  private:
    void initCommandProc();
    void initExceptionHandler();
    void createMonitor();

    unsigned int mTick; // offset 0x0, size 0x4
    int mInit;          // offset 0x4, size 0x4
    int mInitMon;       // offset 0x8, size 0x4
};

extern cCommandProcessor *pCommandProc;

#endif
