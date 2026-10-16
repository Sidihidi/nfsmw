#ifndef MISC_MISSIONEDIT_H
#define MISC_MISSIONEDIT_H

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/Libs/Support/Utility/UStandard.h"

// Editor/debug hooks del TU MissionEdit. El DWARF de GOWE69 lista las siete
// emitidas y descartadas (low_pc 0xFFFFFFFF tras -strip-unused-data).
// DrawMissionComponents es global: la A124 y SLUS la exportan (T) y el
// eDisplayFrame de PS2 la llama; el _GLOBAL_$I del TU lleva su nombre.

class IRigidBody;
class IVehicle;

void DrawMissionComponents();

static void AIEntityCreated(unsigned int entity, const char *name, IRigidBody *body, IVehicle *vehicle);
static void AIEntityDestroyed(unsigned int entity);

static void SendLuaOutput(char *key, char *value);
static void SendLuaOutput(char *value);
static bool IsLuaOutputEnabled();

static void SendToolIMMessage(char *messageID, char *channel, char *key, char *value);
static void SendToolIMMessage(char *messageID, char *channel, UTL::Std::list<char *, _type_list> &keys,
                              UTL::Std::list<char *, _type_list> &values);

#endif
