// GameWindowHandler.cpp: Window event handler adapter for CGame
//
// Routes window events to CGame and InputManager
//////////////////////////////////////////////////////////////////////

// MODERNIZED: Prevent old winsock.h from loading (must be before windows.h)
#define _WINSOCKAPI_

#include "GameWindowHandler.h"
#include "Game.h"
#include "InputManager.h"
#include "RendererFactory.h"

// Custom message IDs (these should match what's used in the game)
#define WM_USER_TIMERSIGNAL     (WM_USER + 500)
#define WM_USER_CALCSOCKETEVENT (WM_USER + 600)

GameWindowHandler::GameWindowHandler(CGame* pGame)
    : m_pGame(pGame)
{
}

void GameWindowHandler::OnClose()
{
    if (!m_pGame)
        return;

    if ((m_pGame->m_cGameMode == DEF_GAMEMODE_ONMAINGAME) && (m_pGame->m_bForceDisconn == false))
    {
#ifdef _DEBUG
        if (m_pGame->m_cLogOutCount == -1 || m_pGame->m_cLogOutCount > 2)
            m_pGame->m_cLogOutCount = 1;
#else
        if (m_pGame->m_cLogOutCount == -1 || m_pGame->m_cLogOutCount > 11)
            m_pGame->m_cLogOutCount = 11;
#endif
    }
    else if (m_pGame->m_cGameMode == DEF_GAMEMODE_ONLOADING)
    {
        // Let default handler process during loading
        return;
    }
    else if (m_pGame->m_cGameMode == DEF_GAMEMODE_ONMAINMENU)
    {
        m_pGame->ChangeGameMode(DEF_GAMEMODE_ONQUIT);
    }
}

void GameWindowHandler::OnDestroy()
{
    if (m_pGame)
    {
        m_pGame->m_bIsProgramActive = false;
        m_pGame->Quit();
    }
    WSACleanup();
}

void GameWindowHandler::OnActivate(bool active)
{
    if (!m_pGame)
        return;

    if (!active)
    {
        m_pGame->m_bIsProgramActive = false;
        InputManager::Get().SetActive(false);
    }
    else
    {
        m_pGame->m_bIsProgramActive = true;
        InputManager::Get().SetActive(true);
        InputManager::Get().ClearAllKeys();

        m_pGame->m_bIsRedrawPDBGS = true;
        if (m_pGame->m_DDraw != nullptr)
            m_pGame->m_DDraw->ChangeDisplayMode(Window::GetHandle());

        if (m_pGame->bCheckImportantFile() == false)
        {
            MessageBox(m_pGame->m_hWnd, "File checksum error! Get Update again please!", "ERROR1", MB_ICONEXCLAMATION | MB_OK);
            PostQuitMessage(0);
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

    switch (keyCode)
    {
    case VK_SHIFT:
        InputManager::Get().OnKeyDown(VK_SHIFT);
        break;
    case VK_CONTROL:
        InputManager::Get().OnKeyDown(VK_CONTROL);
        break;
    case VK_MENU:
        InputManager::Get().OnKeyDown(VK_MENU);
        break;
    case VK_RETURN:
        InputManager::Get().OnKeyDown(VK_RETURN);
        break;
    default:
        // Let the game handle other keys
        m_pGame->OnKeyDown(keyCode);
        break;
    }
}

void GameWindowHandler::OnKeyUp(int keyCode)
{
    if (!m_pGame)
        return;

    switch (keyCode)
    {
    case VK_SHIFT:
        InputManager::Get().OnKeyUp(VK_SHIFT);
        break;
    case VK_CONTROL:
        InputManager::Get().OnKeyUp(VK_CONTROL);
        break;
    case VK_MENU:
        InputManager::Get().OnKeyUp(VK_MENU);
        break;
    case VK_RETURN:
        InputManager::Get().OnKeyUp(VK_RETURN);
        InputManager::Get().SetEnterPressed();
        break;
    case VK_ESCAPE:
        InputManager::Get().OnKeyUp(VK_ESCAPE);
        InputManager::Get().ClearEscPressed();
        break;
    default:
        m_pGame->OnKeyUp(keyCode);
        break;
    }
}

void GameWindowHandler::OnChar(char character)
{
    // Character input is handled through OnTextInput
}

void GameWindowHandler::OnMouseMove(int x, int y)
{
    InputManager::Get().OnMouseMove(x, y);
}

void GameWindowHandler::OnMouseButtonDown(int button, int x, int y)
{
    InputManager::Get().OnMouseMove(x, y);
    InputManager::Get().OnMouseDown(button);
}

void GameWindowHandler::OnMouseButtonUp(int button, int x, int y)
{
    InputManager::Get().OnMouseMove(x, y);
    InputManager::Get().OnMouseUp(button);
}

void GameWindowHandler::OnMouseWheel(int delta, int x, int y)
{
    InputManager::Get().OnMouseWheel(delta, x, y);
}

bool GameWindowHandler::OnCustomMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!m_pGame)
        return false;

    switch (message)
    {
    case WM_USER_TIMERSIGNAL:
        m_pGame->OnTimer();
        return true;

    case WM_USER_CALCSOCKETEVENT:
        m_pGame->_CalcSocketClosed();
        return true;

    case WM_SETCURSOR:
        SetCursor(nullptr);
        return true;

    case WM_SETFOCUS:
        InputManager::Get().SetActive(true);
        return true;

    case WM_KILLFOCUS:
        InputManager::Get().SetActive(false);
        return true;

    case WM_LBUTTONDBLCLK:
        // Handle double-click as button down for manual detection
        InputManager::Get().OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InputManager::Get().OnMouseDown(0);
        return true;
    }

    return false;
}

bool GameWindowHandler::OnTextInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (m_pGame)
    {
        return m_pGame->GetText(hWnd, message, wParam, lParam) != 0;
    }
    return false;
}
