// Overlay_Connecting.cpp: "Connecting to Server..." progress overlay
//
//////////////////////////////////////////////////////////////////////

#include "Overlay_Connecting.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "lan_eng.h"
#include "IInput.h"

Overlay_Connecting::Overlay_Connecting(CGame* pGame)
    : IGameScreen(pGame)
{
}

void Overlay_Connecting::on_initialize()
{
    m_dwStartTime = GameClock::GetTimeMS();
    m_dwAnimTime = m_dwStartTime;
}

void Overlay_Connecting::on_uninitialize()
{
    // Nothing to clean up
}

void Overlay_Connecting::on_update()
{
    uint32_t dwTime = GameClock::GetTimeMS();
    uint32_t dwElapsed = dwTime - m_dwStartTime;

    // ESC key cancels connection (only after 1 second to prevent accidental exit)
    if (Input::IsKeyPressed(VK_ESCAPE))
    {
        if (dwElapsed > 1000)
        {
            // Close sockets
            if (m_pGame->m_pLSock != nullptr)
            {
                m_pGame->m_pLSock.reset();
            }
            if (m_pGame->m_pGSock != nullptr)
            {
                m_pGame->m_pGSock.reset();
            }

            // Clear overlay and go to main menu (using legacy mode change since MainMenu isn't migrated yet)
            clear_overlay();
            m_pGame->ChangeGameMode(GameMode::MainMenu);
            return;
        }
    }

    // Animation frame updates (for cursor/menu animations)
    if ((dwTime - m_dwAnimTime) > 100)
    {
        m_pGame->m_cMenuFrame++;
        m_dwAnimTime = dwTime;
    }
    if (m_pGame->m_cMenuFrame >= 8)
    {
        m_pGame->m_cMenuDirCnt++;
        if (m_pGame->m_cMenuDirCnt > 8)
        {
            m_pGame->m_cMenuDir++;
            m_pGame->m_cMenuDirCnt = 1;
        }
        if (m_pGame->m_cMenuDir > 8) m_pGame->m_cMenuDir = 1;
        m_pGame->m_cMenuFrame = 0;
    }
}

void Overlay_Connecting::on_render()
{
    uint32_t dwTime = GameClock::GetTimeMS();
    uint32_t dwElapsed = dwTime - m_dwStartTime;

    // Draw dialog box
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME4, 162 + SCREENX, 125 + SCREENY, 2);

    // Draw countdown text
    char cTxt[64];
    wsprintf(cTxt, "Connecting to Server... %3dSec", dwElapsed / 1000);
    PutString_SprFont(172 + 35 + SCREENX, 190 + SCREENY, cTxt, 58, 0, 0);

    // Show appropriate message based on elapsed time
    if (dwElapsed > 7000)
    {
        // Show ESC hint after 7 seconds
        PutAlignedString(180 + SCREENX, 463 + SCREENX, 195 + 30 + SCREENY, UPDATE_SCREEN_ON_CONNECTING1);
        PutAlignedString(180 + SCREENX, 463 + SCREENX, 195 + 45 + SCREENY, UPDATE_SCREEN_ON_CONNECTING2);
    }
    else
    {
        PutAlignedString(180 + SCREENX, 463 + SCREENX, 195 + 30 + SCREENY, UPDATE_SCREEN_ON_CONNECTING3);
    }

    DrawVersion();
}
