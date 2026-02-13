// GameWindowHandler.cpp: hb::shared::render::Window event handler adapter for CGame
//
// Routes window events to CGame and hb::shared::input::
//////////////////////////////////////////////////////////////////////

#include "GameWindowHandler.h"
#include "Game.h"
#include "TextInputManager.h"
#include "IInput.h"
#include "RendererFactory.h"
#include "ISpriteFactory.h"
#include "GameModeManager.h"

#ifdef _WIN32
#include <windowsx.h>

namespace MouseButton = hb::shared::input::MouseButton;

#endif

GameWindowHandler::GameWindowHandler(CGame* pGame)
    : m_pGame(pGame)
{
}

void GameWindowHandler::on_close()
{
    if (!m_pGame)
    {
        // No game, just close via hb::shared::render::Window abstraction
        hb::shared::render::Window::close();
        return;
    }

    if ((GameModeManager::GetMode() == GameMode::MainGame) && (m_pGame->m_bForceDisconn == false))
    {
        // In main game, start logout countdown instead of closing immediately
#ifdef _DEBUG
        if (m_pGame->m_logout_count == -1 || m_pGame->m_logout_count > 2)
            m_pGame->m_logout_count = 1;
#else
        if (m_pGame->m_logout_count == -1 || m_pGame->m_logout_count > 11)
            m_pGame->m_logout_count = 11;
#endif
    }
    else if (GameModeManager::GetMode() == GameMode::MainMenu)
    {
        // On main menu, show quit screen
        m_pGame->ChangeGameMode(GameMode::Quit);
    }
    else if (GameModeManager::GetMode() == GameMode::Null)
    {
        // Game code requested close (e.g., from quit screen), proceed with destruction
        hb::shared::render::Window::close();
    }
    else
    {
        // Other modes (loading, etc.), proceed with closing
        hb::shared::render::Window::close();
    }
}

void GameWindowHandler::on_destroy()
{
    if (m_pGame)
    {
        m_pGame->Quit();
    }
    // ASIO handles Winsock cleanup internally
}

void GameWindowHandler::on_activate(bool active)
{
    if (!m_pGame)
        return;

    if (!active)
    {
        if (hb::shared::input::get())
            hb::shared::input::get()->set_window_active(false);
    }
    else
    {
        if (hb::shared::input::get())
            hb::shared::input::get()->set_window_active(true);

        // Only check files after game is fully initialized (sprite factory exists)
        // Rate-limited to at most once per 60 seconds to avoid I/O stall on every alt-tab
        if (hb::shared::sprite::Sprites::GetFactory() != nullptr)
        {
            static uint32_t s_dwLastFileCheck = 0;
            uint32_t dwNow = GameClock::GetTimeMS();
            if (s_dwLastFileCheck == 0 || (dwNow - s_dwLastFileCheck) > 60000)
            {
                s_dwLastFileCheck = dwNow;
                if (m_pGame->bCheckImportantFile() == false)
                {
                    hb::shared::render::Window::show_error("ERROR1", "File checksum error! Get Update again please!");
                    hb::shared::render::Window::close();
                }
            }
        }
    }
}

void GameWindowHandler::on_resize(int width, int height)
{
    // Currently not handling resize events
}

void GameWindowHandler::on_key_down(KeyCode keyCode)
{
    if (!m_pGame)
        return;

    // Route all keys through Input system
    if (hb::shared::input::get())
        hb::shared::input::get()->on_key_down(keyCode);

    // Also notify game for special key handling (hotkeys, etc.)
    // Skip modifier keys as they're handled purely through hb::shared::input::
    if (keyCode != KeyCode::Shift && keyCode != KeyCode::Control && keyCode != KeyCode::Alt &&
        keyCode != KeyCode::LShift && keyCode != KeyCode::RShift &&
        keyCode != KeyCode::LControl && keyCode != KeyCode::RControl &&
        keyCode != KeyCode::LAlt && keyCode != KeyCode::RAlt &&
        keyCode != KeyCode::Enter && keyCode != KeyCode::Escape)
    {
        m_pGame->OnKeyDown(keyCode);
    }
}

void GameWindowHandler::on_key_up(KeyCode keyCode)
{
    if (!m_pGame)
        return;

    // Route all keys through Input system
    if (hb::shared::input::get())
        hb::shared::input::get()->on_key_up(keyCode);

    // Also notify game for special key handling
    // Skip modifier keys as they're handled purely through hb::shared::input::
    if (keyCode != KeyCode::Shift && keyCode != KeyCode::Control && keyCode != KeyCode::Alt &&
        keyCode != KeyCode::LShift && keyCode != KeyCode::RShift &&
        keyCode != KeyCode::LControl && keyCode != KeyCode::RControl &&
        keyCode != KeyCode::LAlt && keyCode != KeyCode::RAlt &&
        keyCode != KeyCode::Enter)
    {
        m_pGame->OnKeyUp(keyCode);
    }
}

void GameWindowHandler::on_char(char character)
{
    // Character input is handled through OnTextInput
}

void GameWindowHandler::on_mouse_move(int x, int y)
{
    if (hb::shared::input::get())
        hb::shared::input::get()->on_mouse_move(x, y);
}

void GameWindowHandler::on_mouse_button_down(int button, int x, int y)
{
    if (hb::shared::input::get())
    {
        hb::shared::input::get()->on_mouse_move(x, y);
        hb::shared::input::get()->on_mouse_down(button);
    }
}

void GameWindowHandler::on_mouse_button_up(int button, int x, int y)
{
    if (hb::shared::input::get())
    {
        hb::shared::input::get()->on_mouse_move(x, y);
        hb::shared::input::get()->on_mouse_up(button);
    }
}

void GameWindowHandler::on_mouse_wheel(int delta, int x, int y)
{
    if (hb::shared::input::get())
    {
        hb::shared::input::get()->on_mouse_move(x, y);
        hb::shared::input::get()->on_mouse_wheel(delta);
    }
}

bool GameWindowHandler::on_custom_message(uint32_t message, uintptr_t wParam, intptr_t lParam)
{
#ifdef _WIN32
    if (!m_pGame)
        return false;

    switch (message)
    {
    case WM_SETCURSOR:
        if (hb::shared::render::Window::get())
            hb::shared::render::Window::get()->set_mouse_cursor_visible(false);
        return true;

    case WM_SETFOCUS:
        if (hb::shared::input::get())
            hb::shared::input::get()->set_window_active(true);
        return true;

    case WM_KILLFOCUS:
        if (hb::shared::input::get())
            hb::shared::input::get()->set_window_active(false);
        return true;

    case WM_LBUTTONDBLCLK:
        // Handle double-click as button down for manual detection
        if (hb::shared::input::get())
        {
            hb::shared::input::get()->on_mouse_move(GET_X_LPARAM(static_cast<LPARAM>(lParam)), GET_Y_LPARAM(static_cast<LPARAM>(lParam)));
            hb::shared::input::get()->on_mouse_down(MouseButton::Left);
        }
        return true;
    }
#endif
    return false;
}

bool GameWindowHandler::on_text_input(hb::shared::types::NativeWindowHandle hWnd, uint32_t message, uintptr_t wParam, intptr_t lParam)
{
    if (m_pGame)
    {
        return TextInputManager::Get().HandleChar(hWnd, message, wParam, lParam) != 0;
    }
    return false;
}
