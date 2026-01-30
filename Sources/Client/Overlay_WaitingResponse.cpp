// Overlay_WaitingResponse.cpp: "Connected. Waiting for response..." progress overlay
//
//////////////////////////////////////////////////////////////////////

#include "Overlay_WaitingResponse.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "lan_eng.h"
#include "IInput.h"
#include "TextLibExt.h"
#include "GameFonts.h"

Overlay_WaitingResponse::Overlay_WaitingResponse(CGame* pGame)
    : IGameScreen(pGame)
{
}

void Overlay_WaitingResponse::on_initialize()
{
    m_dwStartTime = GameClock::GetTimeMS();
    m_dwAnimTime = m_dwStartTime;
}

void Overlay_WaitingResponse::on_uninitialize()
{
    // Nothing to clean up
}

void Overlay_WaitingResponse::on_update()
{
    uint32_t dwTime = GameClock::GetTimeMS();
    uint32_t dwElapsed = dwTime - m_dwStartTime;

    // ESC key cancels (only after 7 seconds to wait for response)
    if (Input::IsKeyPressed(VK_ESCAPE))
    {
        if (dwElapsed > 7000)
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

            // Clear overlay - base screen will be revealed
            clear_overlay();
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

void Overlay_WaitingResponse::on_render()
{
    uint32_t dwTime = GameClock::GetTimeMS();
    uint32_t dwElapsed = dwTime - m_dwStartTime;

    // Draw dialog box
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME4, 162 + SCREENX, 125 + SCREENY, 2);

    // Draw status text
    TextLib::DrawText(GameFont::Bitmap1, 172 + 44 - 17 + SCREENX, 190 + SCREENY, "Connected. Waiting for response...", TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed.r, GameColors::UIDarkRed.g, GameColors::UIDarkRed.b));

    // Show appropriate message based on elapsed time
    if (dwElapsed > 7000)
    {
        // Show ESC hint after 7 seconds
        PutAlignedString(180 + SCREENX, 463 + SCREENX, 195 + 30 + SCREENY, UPDATE_SCREEN_ON_WATING_RESPONSE1);
        PutAlignedString(180 + SCREENX, 463 + SCREENX, 195 + 45 + SCREENY, UPDATE_SCREEN_ON_WATING_RESPONSE2);
    }
    else
    {
        PutAlignedString(180 + SCREENX, 463 + SCREENX, 195 + 30 + SCREENY, UPDATE_SCREEN_ON_WATING_RESPONSE3);
    }

    DrawVersion();
}
