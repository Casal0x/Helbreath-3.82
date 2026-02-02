// Screen_Quit.cpp: Quit Screen Implementation
//
//////////////////////////////////////////////////////////////////////

#include "Screen_Quit.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "IInput.h"
#include "XSocket.h"
#include "SpriteID.h"

extern class XSocket* G_pCalcSocket;

Screen_Quit::Screen_Quit(CGame* pGame)
    : IGameScreen(pGame)
{
}

void Screen_Quit::on_initialize()
{
    // Set current mode for code that checks GameModeManager::GetMode()
    GameModeManager::SetCurrentMode(GameMode::Quit);

    m_dwStartTime = GameClock::GetTimeMS();
    m_iFrameCount = 0;

    // Close sockets
    if (G_pCalcSocket != nullptr)
    {
        delete G_pCalcSocket;
        G_pCalcSocket = nullptr;
    }
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
    // Keep frame counter for draw phase animation (capped at 120 for compatibility)
    m_iFrameCount++;
    if (m_iFrameCount > 120) m_iFrameCount = 120;

    uint32_t dwElapsed = GameClock::GetTimeMS() - m_dwStartTime;

    // After 3 seconds, allow input to skip
    if (dwElapsed >= INPUT_ACTIVE_MS)
    {
        // Handle escape/enter to quit
        if (Input::IsKeyPressed(VK_ESCAPE) || Input::IsKeyPressed(VK_RETURN))
        {
            m_pGame->ChangeGameMode(GameMode::Null);
            Window::Close();
            return;
        }

        // Check for mouse click
        if (Input::IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            m_pGame->ChangeGameMode(GameMode::Null);
            Window::Close();
            return;
        }
    }

    // Auto-quit after 10 seconds
    if (dwElapsed >= AUTO_QUIT_MS)
    {
        m_pGame->ChangeGameMode(GameMode::Null);
        Window::Close();
        return;
    }
}

void Screen_Quit::on_render()
{
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_QUIT, 0 + MENUX(), 0 + MENUY(), 0, true);

    if (m_iFrameCount > 20)
    {
        DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_QUIT, 255 + MENUX(), 123 + MENUY(), 1, true);
    }
    else if ((m_iFrameCount >= 15) && (m_iFrameCount <= 20))
    {
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_QUIT]->Draw(255 + MENUX(), 123 + MENUY(), 1, SpriteLib::DrawParams::Alpha(0.25f));
    }

    DrawVersion();
}
