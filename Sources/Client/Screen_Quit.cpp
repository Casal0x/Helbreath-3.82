// Screen_Quit.cpp: Quit Screen Implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_Quit.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "IInput.h"
#include "SpriteID.h"
using namespace hb::client::sprite_id;


namespace MouseButton = hb::shared::input::MouseButton;

Screen_Quit::Screen_Quit(CGame* pGame)
    : IGameScreen(pGame)
{
}

void Screen_Quit::on_initialize()
{
    // Set current mode for code that checks GameModeManager::GetMode()
    GameModeManager::SetCurrentMode(GameMode::Quit);

    m_dwStartTime = GameClock::GetTimeMS();

    // Close game socket
    if (m_pGame->m_pGSock != nullptr)
    {
        m_pGame->m_pGSock.reset();
    }
}

void Screen_Quit::on_uninitialize()
{
    // Nothing to clean up
}

void Screen_Quit::on_update()
{
    uint32_t dwElapsed = GameClock::GetTimeMS() - m_dwStartTime;

    // After 3 seconds, allow input to skip
    if (dwElapsed >= INPUT_ACTIVE_MS)
    {
        // Handle escape/enter to quit
        if (hb::shared::input::is_key_pressed(KeyCode::Escape) || hb::shared::input::is_key_pressed(KeyCode::Enter))
        {
            m_pGame->ChangeGameMode(GameMode::Null);
            hb::shared::render::Window::close();
            return;
        }

        // Check for mouse click
        if (hb::shared::input::is_mouse_button_pressed(MouseButton::Left))
        {
            m_pGame->ChangeGameMode(GameMode::Null);
            hb::shared::render::Window::close();
            return;
        }
    }

    // Auto-quit after 10 seconds
    if (dwElapsed >= AUTO_QUIT_MS)
    {
        m_pGame->ChangeGameMode(GameMode::Null);
        hb::shared::render::Window::close();
        return;
    }
}

void Screen_Quit::on_render()
{
    DrawNewDialogBox(InterfaceNdQuit, 0, 0, 0, true);

    // Fade in the quit dialog over 500ms
    uint32_t dwElapsed = GameClock::GetTimeMS() - m_dwStartTime;
    if (dwElapsed >= FADE_IN_MS)
    {
        DrawNewDialogBox(InterfaceNdQuit, 335, 183, 1, true);
    }
    else
    {
        float fAlpha = static_cast<float>(dwElapsed) / static_cast<float>(FADE_IN_MS);
        m_pGame->m_pSprite[InterfaceNdQuit]->Draw(335, 183, 1, hb::shared::sprite::DrawParams::Alpha(fAlpha));
    }

    DrawVersion();
}
