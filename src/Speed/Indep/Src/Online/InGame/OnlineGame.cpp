#include "OnlineGame.hpp"

OnlineGame TheOnlineGame;

OnlineGame::OnlineGame()
    : m_isPassworded(false) {
    m_password[0] = '\0';
}

void OnlineGame::ShowDiagnostics() {}
