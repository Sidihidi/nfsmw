#ifndef ONLINE_INGAME_ONLINE_GAME_HPP
#define ONLINE_INGAME_ONLINE_GAME_HPP

#ifdef EA_PRAGMA_ONCE_SUPPORTED
#pragma once
#endif

#include "Speed/Indep/bWare/Inc/Strings.hpp"

struct OnlineGame {
  private:
    /* 0x00 */ bool m_isPassworded;
    /* 0x04 */ char m_password[20];
    /* 0x18 */ bool m_playermuted[8];

  public:
    OnlineGame();
    // Declared but not defined in zOnline: the A124 destroys TheOnlineGame by calling _$_10OnlineGame, which the
    // link resolves to the destructor of the other OnlineGame (the Sim activity).
    ~OnlineGame();
    void SetPlayerMuted(int index, bool muted) { m_playermuted[index] = muted; }
    void SetPassword(char *password) { bStrCpy(m_password, password); }
    void ShowDiagnostics();
};

extern OnlineGame TheOnlineGame;

#endif
