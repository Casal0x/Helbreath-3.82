// Screen_OnGame.h: Main gameplay screen
//
// Handles in-game rendering and input processing when player is in the world
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IGameScreen.h"
#include <cstdint>

class Screen_OnGame : public IGameScreen
{
public:
    SCREEN_TYPE(Screen_OnGame)

    explicit Screen_OnGame(CGame* pGame);
    ~Screen_OnGame() override = default;

    void on_initialize() override;
    void on_uninitialize() override;
    void on_update() override;
    void on_render() override;

private:
    // Screen-specific state (previously file-scope static variables)
    short m_sMsX = 0;
    short m_sMsY = 0;
    short m_sMsZ = 0;
    char m_cLB = 0;
    char m_cRB = 0;
    uint32_t m_dwTime = 0;
    short m_sDivX = 0;
    short m_sModX = 0;
    short m_sDivY = 0;
    short m_sModY = 0;
    short m_sPivotX = 0;
    short m_sPivotY = 0;
    uint32_t m_dwPrevChatTime = 0;
};
