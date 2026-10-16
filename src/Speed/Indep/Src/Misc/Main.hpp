//
//
#ifndef MAIN_HPP
#define MAIN_HPP

void MiniMainLoop();

#ifdef EA_PLATFORM_PLAYSTATION2
// Main.cpp de PS2 (A124: lineas 416-428): llama a function(arg1, arg2) con la pila en la memoria principal si la de
// ahora esta en el scratchpad. La usan el frontend online y los puentes de MoviePlayer.
int DoWithMainMemoryStack(void *function, int arg1, int arg2);
#endif

#endif
