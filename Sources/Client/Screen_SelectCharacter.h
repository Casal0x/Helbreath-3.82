// Screen_SelectCharacter.h: Select Character Screen Class
//
// Handles character selection, navigation to creation/password screens.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IGameScreen.h"
#include <cstdint>

class Screen_SelectCharacter : public IGameScreen
{
public:
    SCREEN_TYPE(Screen_SelectCharacter)

    explicit Screen_SelectCharacter(CGame* pGame);
    ~Screen_SelectCharacter() override = default;

    void on_initialize() override;
    void on_uninitialize() override;
    void on_update() override;
    void on_render() override;

    // Static helper for drawing the character selection background
    // Used by this screen and other screens (ChangePassword, QueryForceLogin, etc.)
    // to maintain the background visual context.
    static void DrawBackground(CGame* pGame, short sX, short sY, short msX, short msY, bool bIgnoreFocus);

    bool EnterGame();

private:
    // Screen-specific state (migrated from file-scope statics and member vars)
    uint32_t m_dwSelCharCTime;
    short m_sSelCharMsX;
    short m_sSelCharMsY;
    
    // Focus state (local to screen)
    int m_cCurFocus;
    int m_cMaxFocus;

    // Offset for centering 640x480 content in 800x600 base resolution
    static constexpr short OX = 80;
    static constexpr short OY = 60;
};
