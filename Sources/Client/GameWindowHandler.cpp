// GameWindowHandler.cpp: Window event handler adapter for CGame
//
// Routes window events to CGame and Input::
//////////////////////////////////////////////////////////////////////

#include "GameWindowHandler.h"
#include "Game.h"
#include "IInput.h"
#include "RendererFactory.h"
#include "ISpriteFactory.h"
#include "DevConsole.h"
#include "Overlay_DevConsole.h"
#include "GameModeManager.h"

#ifdef _WIN32
#include <windowsx.h>
#endif

// Custom message IDs (these should match what's used in the game)
#define WM_USER_CALCSOCKETEVENT (WM_USER + 600)

GameWindowHandler::GameWindowHandler(CGame* pGame)
    : m_pGame(pGame)
{
}

void GameWindowHandler::OnClose()
{
    if (!m_pGame)
    {
        // No game, just close via Window abstraction
        Window::Close();
        return;
    }

    if ((GameModeManager::GetMode() == GameMode::MainGame) && (m_pGame->m_bForceDisconn == false))
    {
        // In main game, start logout countdown instead of closing immediately
#ifdef _DEBUG
        if (m_pGame->m_cLogOutCount == -1 || m_pGame->m_cLogOutCount > 2)
            m_pGame->m_cLogOutCount = 1;
#else
        if (m_pGame->m_cLogOutCount == -1 || m_pGame->m_cLogOutCount > 11)
            m_pGame->m_cLogOutCount = 11;
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
        Window::Close();
    }
    else
    {
        // Other modes (loading, etc.), proceed with closing
        Window::Close();
    }
}

void GameWindowHandler::OnDestroy()
{
    if (m_pGame)
    {
        m_pGame->Quit();
    }
#ifdef _WIN32
    WSACleanup();
#endif
}

void GameWindowHandler::OnActivate(bool active)
{
    if (!m_pGame)
        return;

    if (!active)
    {
        if (Input::Get())
            Input::Get()->SetWindowActive(false);
    }
    else
    {
        if (Input::Get())
            Input::Get()->SetWindowActive(true);

        if (m_pGame->m_Renderer != nullptr)
            m_pGame->m_Renderer->ChangeDisplayMode(Window::GetHandle());

        // Only check files after game is fully initialized (sprite factory exists)
        // This prevents false failures during early window activation before bInit completes
        if (SpriteLib::Sprites::GetFactory() != nullptr)
        {
            if (m_pGame->bCheckImportantFile() == false)
            {
                Window::ShowError("ERROR1", "File checksum error! Get Update again please!");
                Window::Close();
            }
        }
    }
}

void GameWindowHandler::OnResize(int width, int height)
{
    // Currently not handling resize events
}

void GameWindowHandler::OnKeyDown(int keyCode)
{
    if (!m_pGame)
        return;

    // Route all keys through Input system
    if (Input::Get())
        Input::Get()->OnKeyDown(keyCode);

    // Route arrow/page keys to DevConsole when visible
    if (DevConsole::Get().IsVisible())
    {
        DevConsole::Get().HandleKeyDown(static_cast<WPARAM>(keyCode));
        return;
    }

    // Also notify game for special key handling (hotkeys, etc.)
    // Skip modifier keys as they're handled purely through Input::
    if (keyCode != VK_SHIFT && keyCode != VK_CONTROL && keyCode != VK_MENU &&
        keyCode != VK_LSHIFT && keyCode != VK_RSHIFT &&
        keyCode != VK_LCONTROL && keyCode != VK_RCONTROL &&
        keyCode != VK_LMENU && keyCode != VK_RMENU &&
        keyCode != VK_RETURN && keyCode != VK_ESCAPE)
    {
        m_pGame->OnKeyDown(keyCode);
    }
}

void GameWindowHandler::OnKeyUp(int keyCode)
{
    if (!m_pGame)
        return;

    // Route all keys through Input system
    if (Input::Get())
        Input::Get()->OnKeyUp(keyCode);

    // Alt+Tilde: Toggle developer console (checked before anything else)
    if (keyCode == static_cast<int>(KeyCode::Grave) && Input::IsAltDown())
    {
        DevConsole& console = DevConsole::Get();
        if (console.IsVisible())
        {
            console.Hide();
            GameModeManager::clear_overlay();
        }
        else
        {
            console.Show();
            GameModeManager::set_overlay<Overlay_DevConsole>();
        }
        return;
    }

    // Block all other keys when DevConsole is visible
    if (DevConsole::Get().IsVisible())
        return;

    // Also notify game for special key handling
    // Skip modifier keys as they're handled purely through Input::
    if (keyCode != VK_SHIFT && keyCode != VK_CONTROL && keyCode != VK_MENU &&
        keyCode != VK_LSHIFT && keyCode != VK_RSHIFT &&
        keyCode != VK_LCONTROL && keyCode != VK_RCONTROL &&
        keyCode != VK_LMENU && keyCode != VK_RMENU &&
        keyCode != VK_RETURN && keyCode != VK_ESCAPE)
    {
        m_pGame->OnKeyUp(keyCode);
    }
}

void GameWindowHandler::OnChar(char character)
{
    // Character input is handled through OnTextInput
}

void GameWindowHandler::OnMouseMove(int x, int y)
{
    if (Input::Get())
        Input::Get()->OnMouseMove(x, y);
}

void GameWindowHandler::OnMouseButtonDown(int button, int x, int y)
{
    if (Input::Get())
    {
        Input::Get()->OnMouseMove(x, y);
        Input::Get()->OnMouseDown(button);
    }
}

void GameWindowHandler::OnMouseButtonUp(int button, int x, int y)
{
    if (Input::Get())
    {
        Input::Get()->OnMouseMove(x, y);
        Input::Get()->OnMouseUp(button);
    }
}

void GameWindowHandler::OnMouseWheel(int delta, int x, int y)
{
    if (Input::Get())
    {
        Input::Get()->OnMouseMove(x, y);
        Input::Get()->OnMouseWheel(delta);
    }
}

bool GameWindowHandler::OnCustomMessage(uint32_t message, uintptr_t wParam, intptr_t lParam)
{
    if (!m_pGame)
        return false;

    switch (message)
    {
    case WM_USER_CALCSOCKETEVENT:
        m_pGame->_CalcSocketClosed();
        return true;

    case WM_SETCURSOR:
#ifdef _WIN32
        SetCursor(nullptr);
#endif
        return true;

    case WM_SETFOCUS:
        if (Input::Get())
            Input::Get()->SetWindowActive(true);
        return true;

    case WM_KILLFOCUS:
        if (Input::Get())
            Input::Get()->SetWindowActive(false);
        return true;

    case WM_LBUTTONDBLCLK:
        // Handle double-click as button down for manual detection
        if (Input::Get())
        {
            Input::Get()->OnMouseMove(GET_X_LPARAM(static_cast<LPARAM>(lParam)), GET_Y_LPARAM(static_cast<LPARAM>(lParam)));
            Input::Get()->OnMouseDown(MOUSE_BUTTON_LEFT);
        }
        return true;
    }

    return false;
}

bool GameWindowHandler::OnTextInput(NativeWindowHandle hWnd, uint32_t message, uintptr_t wParam, intptr_t lParam)
{
    // Route WM_CHAR text input to DevConsole when visible
    if (message == WM_CHAR && DevConsole::Get().IsVisible())
    {
        return DevConsole::Get().HandleChar(wParam);
    }

    if (m_pGame)
    {
        return m_pGame->GetText(hWnd, message, wParam, lParam) != 0;
    }
    return false;
}
