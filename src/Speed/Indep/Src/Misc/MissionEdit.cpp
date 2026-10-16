#include "Speed/Indep/Src/Misc/MissionEdit.hpp"
#include "Speed/Indep/Libs/Support/Utility/UVectorMath.h"


// Mission component debug drawing --------------------------------------

void DrawMissionComponents() {}

// AI entity lifetime hooks ---------------------------------------------
// Firma de la A124: AIEntityCreated__FUiPCcP10IRigidBodyP8IVehicle.
static void AIEntityCreated(unsigned int entity, const char *name, IRigidBody *body, IVehicle *vehicle) {}

static void AIEntityDestroyed(unsigned int entity) {}

// Lua output hooks ------------------------------------------------------
// VEDA r27: el cuerpo original es inline muerto en las dos plataformas
// (A124: low_pc 0xffffffff sin statics de TU asociados; GOWE69: igual, y el
// ELF final no lo retiene). Sin estado observable en ningun binario, la
// forma sin statics es la unica reconstruccion no-inventada.

static bool IsLuaOutputEnabled() {
    return false;
}

static void SendLuaOutput(char *key, char *value) {}

static void SendLuaOutput(char *value) {}

// Tool instant messaging hooks -----------------------------------------

static void SendToolIMMessage(char *messageID, char *channel, char *key, char *value) {}

static void SendToolIMMessage(char *messageID, char *channel, UTL::Std::list<char *, _type_list> &keys,
                              UTL::Std::list<char *, _type_list> &values) {}
