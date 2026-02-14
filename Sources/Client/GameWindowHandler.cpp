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

#include "platform_headers.h"

namespace MouseButton = hb::shared::input::MouseButton;

#ifdef _WIN32
#include <windowsx.h>
#endif

GameWindowHandler::GameWindowHandler(CGame* game)
    : m_game(game)
{
}

void GameWindowHandler::on_close()
{
    if (!m_game)
    {
        // No game, just close via hb::shared::render::Window abstraction
        hb::shared::render::Window::close();
        return;
    }

    if ((GameModeManager::get_mode() == GameMode::MainGame) && (m_game->m_force_disconn == false))
    {
        // In main game, start logout countdown instead of closing immediately
#ifdef _DEBUG
        if (m_game->m_logout_count == -1 || m_game->m_logout_count > 2)
            m_game->m_logout_count = 1;
#else
        if (m_game->m_logout_count == -1 || m_game->m_logout_count > 11)
            m_game->m_logout_count = 11;
#endif
    }
    else if (GameModeManager::get_mode() == GameMode::MainMenu)
    {
        // On main menu, show quit screen
        m_game->change_game_mode(GameMode::Quit);
    }
    else if (GameModeManager::get_mode() == GameMode::Null)
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
    if (m_game)
    {
        hb::shared::render::Window::close();
    }
    // ASIO handles Winsock cleanup internally
}

void GameWindowHandler::on_activate(bool active)
{
    if (!m_game)
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

        // Legacy file integrity check removed — check_important_file() no longer exists.
    }
}

void GameWindowHandler::on_resize(int width, int height)
{
    // Currently not handling resize events
}

void GameWindowHandler::on_key_down(KeyCode keyCode)
{
    if (!m_game)
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
        m_game->handle_key_down(keyCode);
    }
}

void GameWindowHandler::on_key_up(KeyCode keyCode)
{
    if (!m_game)
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
        m_game->handle_key_up(keyCode);
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

bool GameWindowHandler::on_custom_message(uint32_t message, uintptr_t param, intptr_t lParam)
{
#ifdef _WIN32
    if (!m_game)
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

bool GameWindowHandler::on_text_input(hb::shared::types::NativeWindowHandle hWnd, uint32_t message, uintptr_t param, intptr_t lParam)
{
    if (m_game)
    {
        return text_input_manager::get().handle_char(hWnd, message, param, lParam) != 0;
    }
    return false;
}
