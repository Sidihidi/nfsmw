#include "./Player.hpp"

#include <types.h>

// gcdatos15: los cuatro estaticos de Player, en su hueco exacto de la A124
// (0x643cd0-0x643ce4 = zWorld .data +0x200..0x214, tras _Physics_System_World y delante
// de los g_tweak* de RaceParameters; PS2_globals:5726-5729). Tipos y tamanos de sus
// declaraciones en Player.hpp (0x8/0x8/0x4/0x4, los del .data objetivo); valores a 0,
// los de su .data objetivo. Solo A124: en SLUS/GC/PC/X360 no estan (otrasELF).
#ifdef EA_BUILD_A124
Player *Player::pPlayersByNumber[2] = {0, 0};
Player *Player::pPlayersByIndex[2] = {0, 0};
int Player::NumPlayers = 0;
int Player::nJoysticksChanged = 0;
#endif

// STRIPPED
Player *Player::GetPlayerByNumber(int number) {
    return nullptr;
}

Player *Player::GetPlayerByIndex(int number) {
    return nullptr;
}

// STRIPPED
int Player::GetNumPlayers() {
    return 0;
}
