#include "debugtrap.h"

#include "Speed/Indep/bWare/Inc/bDebug.hpp"

// misc/testhooks/debugtrap.cpp de la A124 (zMisc, entre Timer.cpp e inputsimulation.cpp). Sus stabs dan tambien
// dbgCmdPrintf, isMonEnabled, getTickCount, checkCmdProcCounter y createMonitor, pero el ELF no los conserva: aqui
// solo van los cuatro que tiene. El contador es el Count de COP0 (bGetTicker, con su `ret' en los stabs).

cCommandProcessor::cCommandProcessor() {
    mTick = bGetTicker();
    initExceptionHandler();
}

void cCommandProcessor::initExceptionHandler() {}

void cCommandProcessor::setState(int startStop) {
    if (startStop > 0) {
        mInitMon = 1;
        mTick = bGetTicker();
    } else {
        mInitMon = 0;
    }
}

void cCommandProcessor::setTickCount() {
    mTick = bGetTicker();
}
