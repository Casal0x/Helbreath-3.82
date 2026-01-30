// Overlay_WaitInitData.cpp: Waiting for server init data overlay
//
//////////////////////////////////////////////////////////////////////

#include "Overlay_WaitInitData.h"
#include "Game.h"
#include "GameModeManager.h"
#include "CommonTypes.h"
#include "lan_eng.h"
#include "IInput.h"
#include "TextLibExt.h"
#include "GameFonts.h"

Overlay_WaitInitData::Overlay_WaitInitData(CGame* pGame)
    : IGameScreen(pGame)
{
}

void Overlay_WaitInitData::on_initialize()
{
    m_dwStartTime = GameClock::GetTimeMS();
    m_iFrameCount = 0;
}

void Overlay_WaitInitData::on_uninitialize()
{
    // Nothing to clean up
}

void Overlay_WaitInitData::on_update()
{
    uint32_t dwTime = GameClock::GetTimeMS();
    uint32_t dwElapsed = dwTime - m_dwStartTime;

    m_iFrameCount++;
    if (m_iFrameCount > 100) m_iFrameCount = 100;

    // ESC key returns to MainMenu (only after 7 seconds)
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

            m_pGame->ChangeGameMode(GameMode::MainMenu);
            return;
        }
    }
}

void Overlay_WaitInitData::on_render()
{
    uint32_t dwTime = GameClock::GetTimeMS();
    uint32_t dwElapsed = dwTime - m_dwStartTime;

    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME4, 162 + SCREENX, 125 + SCREENY, 2);

    wsprintf(m_pGame->G_cTxt, "Waiting for response... %dsec", dwElapsed / 1000);
    TextLib::DrawText(GameFont::Bitmap1, 172 + 44 + SCREENX, 190 + SCREENY, m_pGame->G_cTxt, TextLib::TextStyle::WithHighlight(GameColors::UIDarkRed.r, GameColors::UIDarkRed.g, GameColors::UIDarkRed.b));

    if (dwElapsed > 7000)
    {
        PutAlignedString(174 + SCREENX, 467 + SCREENX, 190 + 30 + SCREENY, UPDATE_SCREEN_ON_WAIT_INIT_DATA1);
        PutAlignedString(174 + SCREENX, 467 + SCREENX, 190 + 45 + SCREENY, UPDATE_SCREEN_ON_WAIT_INIT_DATA2);
    }
    else
    {
        PutAlignedString(174 + SCREENX, 467 + SCREENX, 195 + 30 + SCREENY, UPDATE_SCREEN_ON_WAIT_INIT_DATA3);
    }

    DrawVersion();
}
