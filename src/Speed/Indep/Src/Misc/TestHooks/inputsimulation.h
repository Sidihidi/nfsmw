#ifndef MISC_TEST_HOOKS_INPUT_SIMULATION_H
#define MISC_TEST_HOOKS_INPUT_SIMULATION_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Src/Input/ActionRef.h"

// misc/testhooks/inputsimulation.cpp: el "joystick monkey" (solo lo conserva la A124).
extern int EnableJoystickMonkey;

void InitJoystickMonkey();
ActionRef GetFEJoystickMonkeyAction(int port);
ActionRef GetHUDJoystickMonkeyAction(int port);

#endif
