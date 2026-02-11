// Overlay_Connecting.cpp: "Connecting to Server..." progress overlay
//
//////////////////////////////////////////////////////////////////////

#include "Overlay_Connecting.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "lan_eng.h"
#include "IInput.h"
#include "TextLibExt.h"
#include "GameFonts.h"

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

    // ESC key cancels connection (only after 7 seconds when hint is shown)
    if (hb::shared::input::IsKeyPressed(KeyCode::Escape))
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

            m_pGame->m_bIsServerChanging = false;

            // Clear overlay - base screen (Login, SelectCharacter, etc.) will be revealed
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

void Overlay_Connecting::on_render()
{
    uint32_t dwTime = GameClock::GetTimeMS();
    uint32_t dwElapsed = dwTime - m_dwStartTime;

    int dlgX, dlgY;
    GetCenteredDialogPos(DEF_SPRID_INTERFACE_ND_GAME4, 2, dlgX, dlgY);

    // Draw dialog box
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME4, dlgX, dlgY, 2);

    // Draw countdown text
    char cTxt[64];
    std::snprintf(cTxt, sizeof(cTxt), "Connecting to Server... %3dSec", dwElapsed / 1000);
    hb::shared::text::DrawText(GameFont::Bitmap1, dlgX + 45, dlgY + 65, cTxt, hb::shared::text::TextStyle::WithHighlight(GameColors::UIDarkRed));

    // Show appropriate message based on elapsed time
    if (dwElapsed > 7000)
    {
        // Show ESC hint after 7 seconds
        PutAlignedString(dlgX + 18, dlgX + 301, dlgY + 100, UPDATE_SCREEN_ON_CONNECTING1);
        PutAlignedString(dlgX + 18, dlgX + 301, dlgY + 115, UPDATE_SCREEN_ON_CONNECTING2);
    }
    else
    {
        PutAlignedString(dlgX + 18, dlgX + 301, dlgY + 100, UPDATE_SCREEN_ON_CONNECTING3);
    }

    DrawVersion();
}
